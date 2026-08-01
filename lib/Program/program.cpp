#include "program.h"

#include "SPIFFS.h"
#include "audio.h"
#include "emulauncher.h"
#include "buttons.h"


Display Program::display;
TaskHandle_t Program::task_handler;
Program::Configuration Program::config;
Preferences Program::persistent;
SDModule Program::sd;
bool Program::sd_init_ok;



void Program::launch_ (void*) {
  // Hardware setup
  loadCfg();
  display.init();
  sd.setBusHandover([&]{ display.releaseBus(); }, [&]{ display.acquireBus(); });
  sd_init_ok = sd.init();
  Audio::launch(Program::PROGRAM_CORE);
  Buttons::init();

  Program::mainMenu();
}


void Program::launch () {
  persistent.begin("config", false);
  xTaskCreatePinnedToCore(
    Program::launch_,
    "ProgramTask",
    0x18000,               // Stack size in words
    NULL,                  // Parameter to pass into the task
    1,                     // Priority (0 is lowest, 24 is highest. 1 is safe)
    &task_handler,
    PROGRAM_CORE           // The Core ID to pin it to
  );
}


bool Program::runEmulator (
  const std::string &game_name
) {
  // Emulator setup
  auto interface = std::make_shared<ESP32Interface>();
  auto emulator = std::make_shared<EmulatorLauncher>();
  auto game_rom = sd.loadGame(game_name);
  if (game_rom == nullptr or game_rom->empty()) {
    return false;
  }
  // TODO load saved game
  emulator->init(interface, std::move(game_rom));
  emulator->launch(Program::config.emu_cfg);

  bool exit_emu = false;
  display.clearScreen();

  while (not exit_emu) {
    if (interface->newScreenAvailable()) {
      display.printScreen(interface->getLatestScreen());
    }
    interface->setButtons(Buttons::readPadButtons());
    delay(1000/60);
    taskYIELD(); // Notify watchdog
  }

  return true;
}


void Program::loadCfg () {
  Program::config = Program::Configuration{
    .volume = persistent.getInt("volume", 2),
    .brightness = persistent.getInt("brightness", 7),
    .emu_cfg = gb::EmulatorConfig{
      .synch_execution = true,
      .skip_boot_room = persistent.getBool("skip_boot_room", false),
    },
  };
}


void Program::storeCfg () {
  persistent.putInt("volume", config.volume);
  persistent.putInt("brightness", config.brightness);
  persistent.putBool("skip_boot_room", config.emu_cfg.skip_boot_room);
}


void Program::mainMenu () {
  ScreenMenu main_menu = Display::beautifyMenu(ScreenMenu{
    .title = "Main menu",
    .options = {
      "Select game",
      "Options",
    }
  });

  Serial.println(sd_init_ok ? "SD ok!" : "SD fail :(");
  for (auto &game : sd.listGames()) Serial.println(game.c_str());

  while (true) {
    auto [selection, button] = renderMenu(main_menu);
    if (button != gb::Button::Start) {
      continue;
    }
    switch(selection) {
      case 0:
        Audio::beep();
        gameSelectMenu();
        display.clearScreen();
        break;
      case 1:
        Audio::beep();
        optionsMenu();
        display.clearScreen();
        break;
    }
  }
}


void Program::optionsMenu () {
  auto options_menu = ScreenMenu{
    .title = "Options",
    .options = {
      "Volume",
      "Brightness",
      "Skip logo",
      "Back"
    }
  };
  options_menu.options[0] = selector(config.volume, Audio::MAX_VOL)         + " " + options_menu.options[0];
  options_menu.options[1] = selector(config.brightness, Display::MAX_BRIGHTNESS) + " " + options_menu.options[1];
  options_menu.options[2] = (config.emu_cfg.skip_boot_room ? "[x] " : "[ ] ") + options_menu.options[2];
  bool back = false;

  display.clearScreen();
  int selection = 0;
  gb::Button button;

  while (not back) {
    std::tie(selection, button) = renderMenu(options_menu, selection);
    switch(selection) {
      case 0:
        if      (button == gb::Button::Left ) config.volume = std::max(0, config.volume-1);
        else if (button == gb::Button::Right) config.volume = std::min(Audio::MAX_VOL, config.volume+1);
        else break;
        Audio::setVolume(config.volume);
        Audio::beep();
        options_menu.options[0] = selector(config.volume, Audio::MAX_VOL) + " Volume";
        break;
      case 1:
        if      (button == gb::Button::Left ) config.brightness = std::max(0, config.brightness-1);
        else if (button == gb::Button::Right) config.brightness = std::min(Display::MAX_BRIGHTNESS, config.brightness+1);
        else break;
        Audio::beep();
        options_menu.options[1] = selector(config.brightness, Display::MAX_BRIGHTNESS) + " Brightness";
        // TODO change brightness
        break;
      case 2:
        if (button == gb::Button::Start) config.emu_cfg.skip_boot_room = not config.emu_cfg.skip_boot_room;
        else break;
        Audio::beep();
        options_menu.options[2] = config.emu_cfg.skip_boot_room ? "[x] Skip logo" : "[ ] Skip logo";
        break;
      case 3:
        Audio::beep();
        back = true;
        break;
    }
  }

  storeCfg();
}


void Program::gameSelectMenu () {
  display.clearScreen();
  ScreenMenu sm = ScreenMenu{.title = "Select Game", .options = {"Back"}};

  if (not sd_init_ok) {
    sd_init_ok = sd.init();
  }

  if (not sd_init_ok) {
    std::string msg = "Unable to open SD";
    int selection = 0;
    gb::Button button = gb::Button::A;
    while (button != gb::Button::Start) {
      std::tie(selection, button) = renderErrorMenu(sm, msg);
    }
    Audio::beep();
  }
  else {
    auto game_names = sd.listGames();
    for (auto game : game_names) {
      sm.options.push_back(game);
    }
    sm = Display::beautifyMenu(std::move(sm));
    int selection = 0;
    gb::Button button = gb::Button::A;
    while (button != gb::Button::Start) {
      std::tie(selection, button) = renderMenu(sm);
    }
    Audio::beep();
    // Option 0 is back
    if (selection == 0) {
      return;
    }
    selection--;
    if (not runEmulator(game_names[selection])) {
      display.clearScreen();
      ScreenMenu error_sm = ScreenMenu{.title = "Error", .options = {"Back"}};
      std::string msg = "Unable to load game";
      int selection = 0;
      gb::Button button = gb::Button::A;
      while (button != gb::Button::Start) {
        std::tie(selection, button) = renderErrorMenu(error_sm, msg);
      }
      Audio::beep();
    }
  }
}


std::pair<int, gb::Button> Program::renderMenu (const ScreenMenu& sm, int selection) {
  int first = 0;
  const int n_opts = sm.options.size();
  bool entered = false;
  gb::Byte prev_button_read = 0;
  gb::Button pressed;

  while (not entered) {
    // Nice scrolling
    if (n_opts > Display::maxMenuItems()) {
      first = std::clamp(first, selection - 4, selection - 1);
      first = std::clamp(first, 0, n_opts - int(Display::maxMenuItems()));
    }
    display.printMenu(sm, first, selection);
    std::tie(entered, pressed) = handleMenuNavigation(n_opts, selection);
  }

  return {selection, pressed};
}


std::pair<int, gb::Button> Program::renderErrorMenu (const ScreenMenu& sm, const std::string &msg) {
  const int n_opts = sm.options.size();
  bool entered = false;
  int selection = 0;
  gb::Button pressed;

  while (not entered) {
    display.printError(sm, msg, selection);
    std::tie(entered, pressed) = handleMenuNavigation(n_opts, selection);
  }

  return {selection, pressed};
}


std::pair<bool, gb::Button> Program::handleMenuNavigation(int n_opts, int &selection) {
  bool action = false;
  bool entered = false;
  gb::Button pressed;
  gb::Byte prev_button_read = 0;
  
  while (not action) {
    delay(10);
    gb::Byte buttons = Buttons::readPadButtons();
    gb::Byte falling_edge = (buttons ^ prev_button_read) & prev_button_read;
    prev_button_read = buttons;
    if (falling_edge != 0) {
      // Number of zeros to the right of the rightmost one
      int pos_one = __builtin_ctz(static_cast<unsigned int>(falling_edge));
      pressed = static_cast<gb::Button>(1 << pos_one);
      switch (pressed) {
        case gb::Button::Down:
          action = true;
          selection = (selection + 1) % n_opts;
          Audio::beep();
          break;
        case gb::Button::Up:
          action = true;
          selection = (selection + n_opts - 1) % n_opts;
          Audio::beep();
          break;
        default:
          action = true;
          entered = true;
          break;
      }
    }
  }

  return {entered, pressed};
}