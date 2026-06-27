#pragma once

#include <cstdint>
#include <emulator/types.h>
#include <emulator/cpu/buttoninputs.h>


class Buttons {
  static constexpr std::array<std::pair<gb::Button, uint16_t>, 8> PAD_BUTTON_PINS{{
    {gb::Button::Right , 21},
    {gb::Button::Left  , 39},
    {gb::Button::Up    , 38},
    {gb::Button::Down  , 40},
    {gb::Button::A     ,  1},
    {gb::Button::B     ,  2},
    {gb::Button::Select, 41},
    {gb::Button::Start , 42}
  }};
  static constexpr uint16_t EMU_BUTTON_PIN = 18;


public:

  static void init ();

  static gb::Byte readPadButtons ();

  static bool emuButtonPressed ();
};