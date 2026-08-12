#pragma once

#include <atomic>
#include <math.h>
#include <memory>
#include <Arduino.h>
#include <driver/i2s.h>
#include <emulator/audio/audiostate.h>


class Audio {

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

  static int16_t volume;
  static TaskHandle_t task_handler;
  static std::array<int16_t, 8*2*gb::AUDIO_BUFFER_SIZE> audio_buffer;
  static constexpr size_t MASK = audio_buffer.size() - 1;
  static std::array<int16_t, gb::AUDIO_BUFFER_SIZE> playing_chunk;
  static std::atomic<size_t> write_idx;
  static std::atomic<size_t> read_idx;
  static bool alive;

  static void launch_ (void*);

  static void playBuffer (const void *data, size_t n_samples);

  static void init ();

public:

  static constexpr int MAX_VOL = 7;

  static void launch (int core_id);

  static void kill ();

  static void setVolume (int volume);

  static inline void pushData (const gb::AudioPacket &ap) {
    size_t r = read_idx.load(std::memory_order_acquire);
    size_t w = write_idx.load(std::memory_order_relaxed);

    size_t free_space = audio_buffer.size() - (w - r);
    size_t to_write = std::min(ap.buffer_l.size() + ap.buffer_r.size(), free_space);

    for (size_t i = 0; 2*i < to_write; i++) {
      // Convert from uint8_t to int16_t in such a way that 0 -> int16 min and 255 -> int16 max
      audio_buffer[w & MASK]       = (int16_t(ap.buffer_l[i]) - 128)*Audio::volume;
      audio_buffer[(w + 1) & MASK] = (int16_t(ap.buffer_r[i]) - 128)*Audio::volume;
      w += 2;
    }

    write_idx.store(w, std::memory_order_release);
  }

  static bool fillPlayingChunk ();

  static void beep ();
};