#include "audio.h"
#include <limits>


int16_t Audio::volume;
std::array<int16_t, 8*2*gb::AUDIO_BUFFER_SIZE> Audio::audio_buffer;
std::array<int16_t, gb::AUDIO_BUFFER_SIZE> Audio::playing_chunk;
alignas(64) std::atomic<size_t> Audio::write_idx{0};
alignas(64) std::atomic<size_t> Audio::read_idx{0};
TaskHandle_t Audio::task_handler;
bool Audio::alive = false;


void Audio::launch_ (void*) {
  init();
  size_t time = micros();

  while (true) {
    if (fillPlayingChunk()) {
      Audio::playBuffer(playing_chunk.data(), playing_chunk.size());
    } else {
      // Give the emulator some time to produce new audio chunks
      delay((2 * 1000 * playing_chunk.size()) / gb::SAMPLE_RATE);
    }
  }
}


bool Audio::fillPlayingChunk () {
  size_t w = write_idx.load(std::memory_order_acquire);
  size_t r = read_idx.load(std::memory_order_relaxed);
  if (w != r) {
    // Return a maximum of one sound buffer so as to not drain the margin in the audio queue
    size_t to_read = std::min(w - r, size_t(gb::AUDIO_BUFFER_SIZE));

    for (size_t i = 0; i < playing_chunk.size(); i++) {
      playing_chunk[i] = audio_buffer[(r + i) & MASK];
    }
    read_idx.store(r + to_read, std::memory_order_release);
    return true;
  }
  return false;
}


void Audio::playBuffer (const void *data, size_t n_samples) {
  size_t bytes_written;

  i2s_write(
    I2S_NUM,
    data,        // Pointer to the audio data
    2*n_samples, // How many bytes to send
    &bytes_written,         // Returns how many bytes were actually written
    portMAX_DELAY
  );
}


void Audio::beep () {
  // Adjust samples so that they have the correct volume
  auto vol_adjusted_beep_buffer = [] () {
    auto new_buffer = BEEP_BUFFER;
    for (auto &sample : new_buffer) {
      sample *= Audio::volume;
    }
    return new_buffer;
  }();
  
  Audio::playBuffer(vol_adjusted_beep_buffer.data(), vol_adjusted_beep_buffer.size());
}


void Audio::init () {
  static_assert(gb::AUDIO_BUFFER_SIZE <= 1024, "Audio buffer size larger than 1024");

  // Configure the I2S peripheral
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
    .sample_rate = gb::SAMPLE_RATE,
    .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT, // Stereo output
    .communication_format = I2S_COMM_FORMAT_STAND_I2S,
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = 2,                   // Number of DMA buffers
    .dma_buf_len = gb::AUDIO_BUFFER_SIZE, // Size of each buffer (in samples)
    .use_apll = false,
    .tx_desc_auto_clear = true // Emits silence if no data is playing
  };

  // Configure Pins
  i2s_pin_config_t pin_config = {
    .mck_io_num = I2S_PIN_NO_CHANGE,
    .bck_io_num = I2S_BCLK,
    .ws_io_num = I2S_LRC,
    .data_out_num = I2S_DOUT,
    .data_in_num = I2S_PIN_NO_CHANGE
  };

  // Install the driver and map the pins
  i2s_driver_install(Audio::I2S_NUM, &i2s_config, 0, NULL);
  i2s_set_pin(Audio::I2S_NUM, &pin_config);
  
  i2s_zero_dma_buffer(Audio::I2S_NUM); // Set zero volume initially to prevent startup pops
}


void Audio::launch (int core_id) {
  if (alive) {
    return;
  }

  xTaskCreatePinnedToCore(
    Audio::launch_,
    "AudioTask",
    0x800,                 // Stack size in words
    NULL,                  // Parameter to pass into the task
    1,                     // Priority (0 is lowest, 24 is highest. 1 is safe)
    &task_handler,
    core_id                // The Core ID to pin it to
  );

  alive = true;
}


void Audio::kill () {
  if (Audio::task_handler != NULL) {
    vTaskDelete(Audio::task_handler); 
    Audio::task_handler = NULL; 
  }
  alive = false;
}


void Audio::setVolume (int volume) {
  volume = std::clamp(volume, 0, MAX_VOL);
  static constexpr int16_t conversion = std::numeric_limits<int16_t>::max()/std::numeric_limits<int8_t>::max();
  Audio::volume = (volume*conversion)/7;
}