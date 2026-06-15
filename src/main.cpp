#include <Arduino.h>
#include <vector>

#include "program.h"


// const std::string game_path_ = "/Tetris.gb";
const std::string game_path_ = "/Dr_Mario.gb";
// const std::string game_path_ = "/Zelda.gb";
// const std::string game_path_ = "/dmg-acid2.gb";


void setup() {
  Serial.begin(115200);
  Program::launch(game_path_);
  // Remove Arduino's loopTask. Core 1 is fully dedicated to emulation and core 0 to hw interaction
  vTaskDelete(NULL);
}

void loop() {}