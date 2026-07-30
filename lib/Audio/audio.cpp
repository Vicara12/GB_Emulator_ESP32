#include "audio.h"
#include <limits>


std::mutex Audio::audio_mutex;
int16_t Audio::volume;
std::array<std::unique_ptr<Audio::AudioBuffer>, 3> Audio::audio_buffers;
TaskHandle_t Audio::task_handler;


void Audio::launch_ (void*) {
  init();

  while (true) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY); // Wait until new data sample has been fed
    { // Lock scope
      std::lock_guard<std::mutex> lock(Audio::audio_mutex);
      std::swap(audio_buffers[0], audio_buffers[1]);
    }
    Audio::playBuffer((*audio_buffers[0]).data(), (*audio_buffers[0]).size());
  }
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

  for (auto &buffer_ptr : Audio::audio_buffers) {
    buffer_ptr = std::make_unique<AudioBuffer>();
  }

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

  Audio::setVolume(2); // TODO change from config
}


void Audio::launch (int core_id) {
  xTaskCreatePinnedToCore(
    Audio::launch_,
    "AudioTask",
    0x800,                 // Stack size in words
    NULL,                  // Parameter to pass into the task
    1,                     // Priority (0 is lowest, 24 is highest. 1 is safe)
    &task_handler,
    core_id                // The Core ID to pin it to
  );
}


void Audio::kill () {
  if (Audio::task_handler != NULL) {
    vTaskDelete(Audio::task_handler); 
    Audio::task_handler = NULL; 
  }

  for (auto &buffer_ptr : Audio::audio_buffers) {
    buffer_ptr = nullptr;
  }
}


void Audio::setVolume (int volume) {
  volume = std::clamp(volume, 0, MAX_VOL);
  static constexpr int16_t conversion = std::numeric_limits<int16_t>::max()/std::numeric_limits<int8_t>::max();
  Audio::volume = (volume*conversion)/7;
}