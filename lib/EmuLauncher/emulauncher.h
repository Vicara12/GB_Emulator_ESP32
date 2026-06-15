#pragma once

#include <emulator/emulator.h>
#include <emulator/interface.h>
#include "interfaceadapter.h"


class EmulatorLauncher {
  static ESP32Interface *interface;
  static gb::GameRom *cartridge_data;
  static gb::EmulatorConfig emu_cfg;
  static TaskHandle_t task_handler;

  static void launch_ (void *);

public:

  static void init (ESP32Interface *interface, gb::GameRom *cartridge_data);

  static void launch (gb::EmulatorConfig cfg);
};