#include "buttons.h"

#include <Arduino.h>


void Buttons::init () {
  pinMode(EMU_BUTTON_PIN, INPUT_PULLUP);

  for (const auto &[BUTTON, PIN] : PAD_BUTTON_PINS) {
    pinMode(PIN, INPUT_PULLUP);
  }
}


gb::Byte Buttons::readPadButtons () {
  gb::Byte pad_buttons = 0x00;
  for (const auto &[BUTTON, PIN] : PAD_BUTTON_PINS) {
    pad_buttons |= (digitalRead(PIN) == LOW ? static_cast<gb::Byte>(BUTTON) : 0);
  }
  return pad_buttons;
}


bool Buttons::emuButtonPressed () {
  return digitalRead(EMU_BUTTON_PIN) == LOW;
}