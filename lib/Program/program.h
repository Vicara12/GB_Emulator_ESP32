#pragma once

#include <memory>
#include <Preferences.h>
#include <emulator/emulator.h>
#include "display.h"
#include "interfaceadapter.h"


class Program {

  struct Configuration {
    int volume;
    int brightness;
    gb::EmulatorConfig emu_cfg;

  };

  static constexpr size_t PROGRAM_CORE = 0;

  static Display display;
  static TaskHandle_t task_handler;
  static Configuration config;
  static Preferences persistent;
  static std::string rom_path; // TODO remove

  static void launch_ (void*);

  static std::unique_ptr<gb::GameRom> readGameRom (const std::string &game_path);

  static void loadCfg ();

  static void storeCfg ();

  static void runEmulator ();

  static void mainMenu ();

  static void optionsMenu ();

  static void gameSelectMenu ();

  static std::pair<int, gb::Button> renderMenu (const ScreenMenu& sm, int selection = 0);

  static inline std::string selector (int value, int max) {
    return std::string("<").append(value, '=').append(max - value, ' ') + ">";
  }

public:

  static void launch (const std::string rom_path);
};