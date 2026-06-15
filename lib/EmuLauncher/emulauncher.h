#pragma once

#include <memory>
#include <emulator/emulator.h>
#include <emulator/interface.h>
#include "interfaceadapter.h"


class EmulatorLauncher {

  static constexpr size_t EMULATOR_CORE = 1;

  static std::shared_ptr<ESP32Interface> interface;
  static std::unique_ptr<gb::GameRom> cartridge_data;
  static gb::EmulatorConfig emu_cfg;
  static TaskHandle_t task_handler;

  static void launch_ (void *);

public:

  static void init (
    std::shared_ptr<ESP32Interface> interface,
    std::unique_ptr<gb::GameRom> cartridge_data
  );

  static void launch (gb::EmulatorConfig cfg);

  static void kill ();
};