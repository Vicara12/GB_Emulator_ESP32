#include "buttons.h"

#include <Arduino.h>


int Buttons::prev_emu_button_state = LOW;


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
  bool new_state = digitalRead(EMU_BUTTON_PIN);
  bool pressed = (new_state == LOW and prev_emu_button_state == HIGH);
  prev_emu_button_state = new_state;
  return pressed;
}