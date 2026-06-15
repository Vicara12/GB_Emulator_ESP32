#pragma once

#include <memory>
#include <emulator/emulator.h>
#include "display.h"
#include "interfaceadapter.h"


class Program {

  struct Configuration {
    gb::EmulatorConfig emu_cfg;
  };

  static constexpr size_t PROGRAM_CORE = 0;

  static Display display;
  static TaskHandle_t task_handler;
  static Configuration config;
  static std::string rom_path; // TODO remove

  static void launch_ (void*);

  static std::unique_ptr<gb::GameRom> readGameRom (const std::string &game_path);

  static void loadCfg ();

  static void storeCfg ();

  static void runEmulator ();

public:

  static void launch (const std::string rom_path);
};