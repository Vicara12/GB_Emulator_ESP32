#include "program.h"

#include "SPIFFS.h"
#include "audio.h"
#include "emulauncher.h"
#include "buttons.h"


Display Program::display;
TaskHandle_t Program::task_handler;
Program::Configuration Program::config;
std::string Program::rom_path;



void Program::launch_ (void*) {
  // Hardware setup
  display.init();
  Audio::launch(Program::PROGRAM_CORE);
  Buttons::init();

  Program::runEmulator();
}


void Program::launch (const std::string rom_path) {
  Program::rom_path = rom_path; // TODO remove
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
    display.printScreen(interface->getLatestScreen());
    interface->setButtons(Buttons::readPadButtons()); // TODO increase frequency
    delay(1000/30); // TODO update to 50 fps
    taskYIELD(); // Notify watchdog
  }
}


void Program::loadCfg () {
  // TODO
  Program::config = Program::Configuration{
    .emu_cfg = gb::EmulatorConfig{
      .synch_execution = false,
      .skip_boot_room = false,
    },
  };
}

void Program::storeCfg () {
  // TODO
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