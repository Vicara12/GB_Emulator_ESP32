#pragma once

#include <memory>
#include <Preferences.h>
#include <emulator/emulator.h>
#include "display.h"
#include "interfaceadapter.h"
#include "sdmodule.h"


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
  static SDModule sd;
  static bool sd_init_ok;

  static void launch_ (void*);

  static void loadCfg ();

  static void storeCfg ();

  static bool runEmulator (
    const std::string &game_name
  );

  static void mainMenu ();

  static void optionsMenu ();

  static void gameSelectMenu ();

  static std::pair<int, gb::Button> renderMenu (const ScreenMenu& sm, int selection = 0);

  static std::pair<int, gb::Button> renderErrorMenu (const ScreenMenu& sm, const std::string &msg);

  static inline std::string selector (int value, int max) {
    return std::string("<").append(value, '=').append(max - value, ' ') + ">";
  }

  static std::pair<bool, gb::Button> handleMenuNavigation(int n_opts, int &selection);

public:

  static void launch ();
};