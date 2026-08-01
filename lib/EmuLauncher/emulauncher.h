#pragma once

#include <memory>
#include <emulator/emulator.h>
#include <emulator/interface.h>
#include "interfaceadapter.h"


class EmulatorLauncher {

  static constexpr size_t EMULATOR_CORE = 1;

  static std::shared_ptr<ESP32Interface> interface;
  static std::unique_ptr<gb::GameRom> cartridge_data;
  static std::unique_ptr<gb::GameRom> save_game;
  static gb::EmulatorConfig emu_cfg;
  static TaskHandle_t task_handler;
  static bool start_emu;

  static void launch_ (void *);

public:

  static void emulate (
    std::shared_ptr<ESP32Interface> interface,
    gb::EmulatorConfig cfg,
    std::unique_ptr<gb::GameRom> cartridge_data,
    std::unique_ptr<gb::GameRom> save_game = nullptr
  );

  static void launch ();

  static void kill ();
};