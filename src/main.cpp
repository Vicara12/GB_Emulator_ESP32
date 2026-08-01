#include <Arduino.h>
#include "program.h"


void setup() {
  Serial.begin(115200);
  Program::launch();
  // Remove Arduino's loopTask. Core 1 is fully dedicated to emulation and core 0 to hw interaction
  vTaskDelete(NULL);
}

void loop() {}