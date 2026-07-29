#include "program.h"

#include "SPIFFS.h"
#include "audio.h"
#include "emulauncher.h"
#include "buttons.h"


Display Program::display;
TaskHandle_t Program::task_handler;
Program::Configuration Program::config;
Preferences Program::persistent;
std::string Program::rom_path;



void Program::launch_ (void*) {
  // Hardware setup
  display.init();
  Audio::launch(Program::PROGRAM_CORE);
  Buttons::init();

  Program::mainMenu();
}


void Program::launch (const std::string rom_path) {
  Program::rom_path = rom_path; // TODO remove
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


void Program::runEmulator () {
  // Emulator setup
  auto interface = std::make_shared<ESP32Interface>();
  auto emulator = std::make_shared<EmulatorLauncher>();
  auto game_rom = Program::readGameRom(rom_path);
  emulator->init(interface, std::move(game_rom));
  emulator->launch(Program::config.emu_cfg);

  bool exit_emu = false;

  while (not exit_emu) {
    if (interface->newScreenAvailable()) {
      display.printScreen(interface->getLatestScreen());
    }
    interface->setButtons(Buttons::readPadButtons()); // TODO increase frequency
    delay(1000/60);
    taskYIELD(); // Notify watchdog
  }
}


void Program::loadCfg () {
  Program::config = Program::Configuration{
    .volume = persistent.getInt("volume", 2),
    .emu_cfg = gb::EmulatorConfig{
      .synch_execution = true,
      .skip_boot_room = persistent.getBool("skip_boot_room", false),
    },
  };
}


void Program::storeCfg () {
  persistent.putInt("volume", config.volume);
  persistent.putBool("skip_boot_room", config.emu_cfg.skip_boot_room);
}


void Program::mainMenu () {
  ScreenMenu main_menu = Display::beautifyMenu(ScreenMenu{
    .title = "Main menu",
    .options = {
      "Select game",
      "Options",
      "1234567890123456789012345",
      "Extra2",
      "Extra3",
      "123456789012345678901234",
      "Extra5",
      "Extra6",
      "Extra7",
      "Extra8",
    }
  });

  while (true) {
    int opt = renderMenu(main_menu);
  }
}


void Program::optionsMenu (bool ingame) {

}


void Program::gameSelectMenu () {

}


int Program::renderMenu (const ScreenMenu& sm) {
  int first = 0;
  int selection = 0;
  const int n_opts = sm.options.size();
  bool entered = false;
  gb::Byte prev_button_read = 0;

  display.clearScreen();

  while (not entered) {
    // Nice scrolling
    if (n_opts > Display::maxMenuItems()) {
      first = std::clamp(first, selection - 4, selection - 1);
      first = std::clamp(first, 0, n_opts - int(Display::maxMenuItems()));
    }
    display.printMenu(sm, first, selection);
    bool action = false;
    while (not action) {
      delay(50);
      gb::Byte buttons = Buttons::readPadButtons();
      gb::Byte falling_edge = (buttons ^ prev_button_read) & prev_button_read;
      prev_button_read = buttons;
      if (falling_edge != 0) {
        // Number of zeros to the right of the rightmost one
        int pos_one = __builtin_ctz(static_cast<unsigned int>(falling_edge));
        switch (static_cast<gb::Button>(1 << pos_one)) {
          case gb::Button::Down:
            action = true;
            selection = (selection + 1) % n_opts;
            break;
          case gb::Button::Up:
            action = true;
            selection = (selection + sm.options.size() - 1) % n_opts;
            break;
          case gb::Button::Select:
            action = true;
            entered = true;
            break;
        }
      }
    }
  }

  return selection;
}


std::unique_ptr<gb::GameRom> Program::readGameRom(const std::string &game_path) {
  if (not SPIFFS.begin(true)) {
    Serial.println("Some error occurred while mounting SPIFFS");
    while (true) delay(1000);
  }
  
  File file = SPIFFS.open(game_path.c_str(), FILE_READ);
  if (not file || file.isDirectory())
    throw std::runtime_error("Could not open file: " + game_path);

  size_t file_size = file.size();
  auto game_rom = std::make_unique<gb::GameRom>();
  game_rom->resize(file_size);
  file.read(game_rom->data(), file_size);
  file.close();
  return game_rom;
}