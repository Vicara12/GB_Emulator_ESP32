#pragma once

#include <mutex>
#include <math.h>
#include <memory>
#include <Arduino.h>
#include <driver/i2s.h>
#include <emulator/audio/audiostate.h>


class Audio {

  using AudioBuffer = std::array<int16_t, 2*gb::AUDIO_BUFFER_SIZE>;

  static constexpr i2s_port_t I2S_NUM = I2S_NUM_0;
  // Hardware pin configs
  static constexpr size_t I2S_LRC  = 4;
  static constexpr size_t I2S_BCLK = 5;
  static constexpr size_t I2S_DOUT = 6;

  // Audio buffer containing a beeping sound
  static inline const auto BEEP_BUFFER = [] () {
    constexpr ulong duration_fraction = 8;
    constexpr float freq = 880.f;
    constexpr ulong n_samples = gb::SAMPLE_RATE/duration_fraction;
    std::array<int16_t, 2*n_samples> buffer;

    for (ulong i = 0; i < n_samples; i++) {
      float evelope = sqrt(sqrt(sqrt(sin(M_PI * float(i) / n_samples))));
      float audio_wave = sin(2.0f * M_PI * freq * float(i) / gb::SAMPLE_RATE);
      int16_t value = 128 * evelope * audio_wave;
      buffer[2*i] = value;
      buffer[2*i + 1] = value;
    }

    return buffer;
  }();

  static std::mutex audio_mutex;
  static int16_t volume;
  static std::array<std::unique_ptr<AudioBuffer>, 3> audio_buffers;
  static TaskHandle_t task_handler;

  static void launch_ (void*);

  static void playBuffer (const void *data, size_t n_samples);

  static void init ();

public:

  static constexpr int MAX_VOL = 7;

  static void launch (int core_id);

  static void kill ();

  static void setVolume (int volume);

  static inline void pushData (const gb::AudioPacket &ap) {
    for (size_t i = 0; i < gb::AUDIO_BUFFER_SIZE; i++) {
      (*audio_buffers[2])[2*i]     = (int16_t(ap.buffer_l[i]) - 128)*Audio::volume;
      (*audio_buffers[2])[2*i + 1] = (int16_t(ap.buffer_r[i]) - 128)*Audio::volume;
    }
    { // Lock scope
      std::lock_guard<std::mutex> lock(Audio::audio_mutex);
      std::swap(audio_buffers[1], audio_buffers[2]);
    }
    if (Audio::task_handler != NULL) {
      xTaskNotifyGive(Audio::task_handler); 
    }
  }

  static void beep ();
};