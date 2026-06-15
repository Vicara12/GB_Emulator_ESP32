#include <Arduino.h>
#include <vector>
#include "SPIFFS.h"
#include "interfaceadapter.h"
#include <emulator/emulator.h>
#include "display.h"
#include "emulauncher.h"


float er;
ESP32Interface *interface;
gb::GameRom game_rom;
auto emu_cfg = gb::EmulatorConfig{
  .synch_execution = false,
  .skip_boot_room = false
};
Display display;

// const std::string game_path_ = "/Tetris.gb";
const std::string game_path_ = "/Dr_Mario.gb";
// const std::string game_path_ = "/Zelda.gb";
// const std::string game_path_ = "/dmg-acid2.gb";

ScreenMenu sample_menu{
  .title = "Test menu!",
  .options = {
    "Option 1", "Option 2", "Option 3", "Option 4",
    "Option 5", "Option 6", "Option 7", "Option 8",
    "Option 9", "Option 10", "Option 11", "Option 12"
  },
  .first = 2,
  .selection = 5
};


void readGameRom(const std::string &game_path) {
  if (not SPIFFS.begin(true)) {
    Serial.println("Some error occurred while mounting SPIFFS");
    while (true) delay(1000);
  }
  
  File file = SPIFFS.open(game_path.c_str(), FILE_READ);
  if (not file || file.isDirectory())
    throw std::runtime_error("Could not open file: " + game_path);

  size_t file_size = file.size();
  game_rom.resize(file_size);
  file.read(game_rom.data(), file_size);
  file.close();
}


void setup() {
  Serial.begin(115200);
  Serial.println("\n\nBeginning setup...");
  interface = new ESP32Interface;
  Serial.println("Interface loaded");
  readGameRom(game_path_);
  Serial.println("Done reading game ROM");
  display.init();
  // display.printMenu(sample_menu);
  
  EmulatorLauncher emulator;

  emulator.init(interface, &game_rom);

  emulator.launch(emu_cfg);

  // gb::emulator<ESP32Interface,false>(*interface, game_rom, emu_cfg);
}

void loop() {
  display.printScreen(interface->getLatestScreen());
  delay(1000/30);
}