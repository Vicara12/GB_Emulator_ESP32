#include "program.h"

#include "SPIFFS.h"
#include "audio.h"
#include "buttons.h"


Display Program::display;
TaskHandle_t Program::task_handler;
Program::Configuration Program::config;
Preferences Program::persistent;
SDModule Program::sd;
EmulatorLauncher Program::emulator;
std::string Program::current_game_name;
bool Program::sd_init_ok;



void Program::launch_ (void*) {
  // Hardware setup
  loadCfg();
  emulator.launch();
  Audio::launch(Program::PROGRAM_CORE);
  display.init();
  sd.setBusHandover([&]{ display.releaseBus(); }, [&]{ display.acquireBus(); });
  sd_init_ok = sd.init();
  Buttons::init();
  Audio::setVolume(config.volume);

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


bool Program::handleEmuButton (std::shared_ptr<ESP32Interface> interface) {
  bool exit_emu = false;

  if (Buttons::emuButtonPressed()) {
    interface->pauseEmulation();
    exit_emu = emuPausedMenu(interface);
    interface->resumeEmulation();
  }

  return exit_emu;
}


bool Program::emuPausedMenu (std::shared_ptr<ESP32Interface> interface) {
  ScreenMenu sm = ScreenMenu{
    .title = "Game Paused",
    .options = {
      "Resume",
      "Volume",
      "Brightness",
      "Save",
      "Save & Exit",
      "Exit"
    }
  };
  updateVolumeKnob(gb::Button::A, sm.options[1]);
  updateBrightnessKnob(gb::Button::A, sm.options[2]);
  bool quit = false;
  bool exit_emu = false;
  int selection = 0;
  gb::Button button;

  display.clearScreen();
  while (not quit) {
    std::tie(selection, button) = renderMenu(sm, selection);
    switch(selection) {
      case 0:
        if (button != gb::Button::Start) break;
        quit = true;
        break;
      case 1:
        updateVolumeKnob(button, sm.options[1]);
        break;
      case 2:
        updateBrightnessKnob(button, sm.options[2]);
        break;
      case 3:
        if (button != gb::Button::Start) break;
        Audio::beep();
        saveGameMenu(interface);
        display.clearScreen();
        break;
      case 4:
        if (button != gb::Button::Start) break;
        Audio::beep();
        // Only quit if game could be saved
        quit = saveGameMenu(interface);
        exit_emu = quit;
        break;
      case 5:
        if (button != gb::Button::Start) break;
        Audio::beep();
        quit = true;
        exit_emu = true;
        break;
    }
  }

  if (exit_emu) {
    interface->requestEmulationEnd();
  }
  storeCfg();
  display.clearScreen();

  return exit_emu;
}


bool Program::saveGameMenu (std::shared_ptr<ESP32Interface> interface) {
  interface->forceRAMCopy();
  std::unique_ptr<gb::GameRom> ram = nullptr;
  while (not ram) {
    ram = interface->getRAM();
    delay(100);
  }
  display.clearScreen();
  if (ram->size() == 0) {
    ScreenMenu error_sm = ScreenMenu{.title = "ERROR", .options = {"Back"}};
    std::string msg = "Game can't be saved";
    int selection = 0;
    gb::Button button = gb::Button::A;
    while (button != gb::Button::Start) {
      std::tie(selection, button) = renderErrorMenu(error_sm, msg);
    }
    Audio::beep();
    display.clearScreen();
    return false;
  }

  auto [save, save_file_name] = savedGameSelector(current_game_name);
  if (save) {
    gb::ScreenPixels last_screen = *interface->getLatestScreen();
    auto data = SDModule::SavedGame{
      .info = SDModule::SavedGameInfo{.screen = last_screen},
      .ram_data = *std::move(ram),
    };

    // Save as new file
    if (save_file_name.empty()) {
      sd.newSavedGame(current_game_name, std::move(data));
    }
    // Overwrite other saved game
    else {
      sd.saveGame(current_game_name, save_file_name, std::move(data));
    }
  }
  else {
    return false;
  }

  return true;
}


std::tuple<bool, std::string> Program::savedGameSelector (const std::string &game, bool skip_if_no_saved) {
  auto saved_games = sd.listSavedGames(game);
  if (skip_if_no_saved and saved_games.empty()) {
    return {true, ""};
  }

  auto sm = ScreenMenu{
    .title = "Saved Games",
    .options = {
      "Back",
      "New Game",
    }
  };
  for (const auto &save : saved_games) {
    sm.options.push_back(save.name);
  }

  int first = 0;
  int selection = 0;
  const int n_opts = sm.options.size();
  bool entered = false;
  gb::Button pressed;

  display.clearScreen();

  while (true) {
    // Nice scrolling
    if (n_opts > Display::maxMenuItems()) {
      first = std::clamp(first, selection - 4, selection - 1);
      first = std::clamp(first, 0, n_opts - int(Display::maxMenuItems()));
    }
    display.printMenu(sm, first, selection);
    // Render miniature for game saves
    if (selection > 1) {
      display.printMiniature(&saved_games[selection-2].screen);
    }
    else {
      gb::ScreenPixels black_screen;
      for (auto &row : black_screen) {
        row.fill(static_cast<gb::Byte>(gb::BWColors::BLACK));
      }
      display.printMiniature(&black_screen);
    }
    std::tie(entered, pressed) = handleMenuNavigation(n_opts, selection);
    if (pressed == gb::Button::Start) {
      if (selection == 0) {
        return {false, ""}; // Back
      }
      else if (selection == 1) {
        return {true, ""}; // New game
      }
      else {
        return {true, saved_games[selection-2].name}; // Actual saved game file
      }
    }
  }
}


bool Program::runEmulator (
  const std::string &game_name,
  const std::string &save_name
) {
  current_game_name = game_name;
  // Emulator setup
  auto interface = std::make_shared<ESP32Interface>();
  auto game_rom = sd.loadGame(game_name);
  if (game_rom == nullptr or game_rom->empty()) {
    return false;
  }
  auto save_data = sd.loadSavedGame(game_name, save_name);
  if (not save_name.empty() and save_data.ram_data.empty()) {
    return false;
  }

  bool exit_emu = false;
  display.clearScreen();

  emulator.emulate(
    interface,
    Program::config.emu_cfg,
    std::move(game_rom)
    // std::make_unique<gb::GameRom>(save_data.ram_data)
  );
  while (not exit_emu) {
    if (interface->newScreenAvailable()) {
      display.printScreen(interface->getLatestScreen());
    }
    interface->setButtons(Buttons::readPadButtons());
    exit_emu = handleEmuButton(interface);
    delay(1000/60);
    taskYIELD(); // Notify watchdog
  }

  while (not interface->emulationEnded()) {
    delay(100);
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
  // Format knob strings
  updateVolumeKnob(gb::Button::A, options_menu.options[0]);
  updateBrightnessKnob(gb::Button::A, options_menu.options[1]);
  options_menu.options[2] = config.emu_cfg.skip_boot_room ? "[x] Skip logo" : "[ ] Skip logo";
  bool back = false;

  display.clearScreen();
  int selection = 0;
  gb::Button button;

  while (not back) {
    std::tie(selection, button) = renderMenu(options_menu, selection);
    switch(selection) {
      case 0:
        updateVolumeKnob(button, options_menu.options[0]);
        break;
      case 1:
        updateBrightnessKnob(button, options_menu.options[1]);
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


void Program::updateVolumeKnob (gb::Button button, std::string &knob_str) {
  bool valid_command = true;
  if      (button == gb::Button::Left ) config.volume = std::max(0, config.volume-1);
  else if (button == gb::Button::Right) config.volume = std::min(Audio::MAX_VOL, config.volume+1);
  else valid_command = false;
  Audio::setVolume(config.volume);
  knob_str = selector(config.volume, Audio::MAX_VOL) + " Volume";
  if (valid_command) {
    Audio::beep();
  }
}


void Program::updateBrightnessKnob (gb::Button button, std::string &knob_str) {
  bool valid_command = true;
  if      (button == gb::Button::Left ) config.brightness = std::max(0, config.brightness-1);
  else if (button == gb::Button::Right) config.brightness = std::min(Display::MAX_BRIGHTNESS, config.brightness+1);
  else valid_command = false;
  knob_str = selector(config.brightness, Display::MAX_BRIGHTNESS) + " Brightness";
  // TODO change brightness
  if (valid_command) {
    Audio::beep();
  }
}


void Program::gameSelectMenu () {
  ScreenMenu sm = ScreenMenu{.title = "Select Game", .options = {"Back"}};

  if (not sd_init_ok) {
    sd_init_ok = sd.init();
  }

  if (not sd_init_ok) {
    sm.title = "ERROR";
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
    while (true) {
      display.clearScreen();
      while (button != gb::Button::Start) {
        std::tie(selection, button) = renderMenu(sm);
      }
      button = gb::Button::A; // Change action button
      Audio::beep();
      // Option 0 is back
      if (selection == 0) {
        return;
      }
      std::string game_name = game_names[selection-1];
      // Save game selector (if any)
      auto [load, save_name] = savedGameSelector(game_name, true);
      if (load) {
        if (not runEmulator(game_name, save_name)) {
          display.clearScreen();
          ScreenMenu error_sm = ScreenMenu{.title = "Error", .options = {"Back"}};
          std::string msg = "Unable to load game";
          int selection_2 = 0;
          gb::Button button = gb::Button::A;
          while (button != gb::Button::Start) {
            std::tie(selection_2, button) = renderErrorMenu(error_sm, msg);
          }
          Audio::beep();
        }
      }
      
    }
  }
}


std::pair<int, gb::Button> Program::renderMenu (const ScreenMenu& sm, int selection) {
  int first = 0;
  const int n_opts = sm.options.size();
  bool entered = false;
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