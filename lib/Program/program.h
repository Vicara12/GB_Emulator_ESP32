#pragma once

#include <memory>
#include <Preferences.h>
#include <emulator/emulator.h>
#include "display.h"
#include "interfaceadapter.h"
#include "sdmodule.h"
#include "emulauncher.h"


class Program {

  struct Configuration {
    int volume;
    int brightness;
    gb::EmulatorConfig emu_cfg;
  };

  static constexpr size_t PROGRAM_CORE = 0;
  static constexpr auto ACTION_BUTTON = gb::Button::A;
  static constexpr auto BACK_BUTTON = gb::Button::B;

  static Display display;
  static TaskHandle_t task_handler;
  static Configuration config;
  static Preferences persistent;
  static SDModule sd;
  static EmulatorLauncher emulator;
  static std::string current_game_name;
  static bool sd_init_ok;

  static void launch_ (void*);

  static void loadCfg ();

  static void storeCfg ();

  static bool runEmulator (
    const std::string &game_name,
    const std::string &save_name
  );

  static void mainMenu ();

  static void optionsMenu ();

  static void gameSelectMenu ();

  static bool emuPausedMenu (std::shared_ptr<ESP32Interface> interface);

  static std::tuple<int, gb::Button, bool> renderMenu (const ScreenMenu& sm, int selection = 0);

  static std::pair<int, gb::Button> renderErrorMenu (const ScreenMenu& sm, const std::string &msg);

  static inline std::string selector (int value, int max) {
    return std::string("<").append(value, '=').append(max - value, ' ') + ">";
  }

  static std::tuple<bool, gb::Button, bool> handleMenuNavigation(int n_opts, int &selection);

  static bool handleEmuButton (std::shared_ptr<ESP32Interface> interface);

  static bool saveGameMenu (std::shared_ptr<ESP32Interface> interface);

  static std::tuple<bool, std::string, int> savedGameSelector (
    const std::string &game,
    bool skip_if_no_saved = false
  );

  static void updateVolumeKnob (gb::Button button, std::string &knob_str);

  static void updateBrightnessKnob (gb::Button button, std::string &knob_str);

  static void errorMenu (std::string msg);

public:

  static void launch ();
};