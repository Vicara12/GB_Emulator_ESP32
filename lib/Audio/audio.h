#pragma once

#include <mutex>
#include <Arduino.h>
#include <driver/i2s.h>
#include <emulator/audio/audiostate.h>


class Audio {

  static constexpr i2s_port_t I2S_NUM = I2S_NUM_0;
  // Hardware pin configs
  static constexpr size_t I2S_LRC  = 4;
  static constexpr size_t I2S_BCLK = 5;
  static constexpr size_t I2S_DOUT = 6;

  static std::mutex audio_mutex;
  static gb::AudioPacket latest_audio;
  static int16_t volume;
  static std::array<int16_t, 2*gb::AUDIO_BUFFER_SIZE> sample_buffer;
  static TaskHandle_t task_handler;

  static void launch_ (void*);

  static void playBuffer ();

  static void init ();

public:

  static void launch (int core_id);

  static void kill ();

  static void setVolume (int volume);

  static inline void pushData (const gb::AudioPacket &ap) {
    { // Lock scope
      std::lock_guard<std::mutex> lock(Audio::audio_mutex);
      Audio::latest_audio = ap;
    }
    if (Audio::task_handler != NULL) {
      xTaskNotifyGive(Audio::task_handler); 
    }
  }
};