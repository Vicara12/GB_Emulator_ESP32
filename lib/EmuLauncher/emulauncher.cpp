#include "emulauncher.h"
#include <esp_task_wdt.h>


std::shared_ptr<ESP32Interface> EmulatorLauncher::interface;
std::unique_ptr<gb::GameRom> EmulatorLauncher::cartridge_data;
gb::EmulatorConfig EmulatorLauncher::emu_cfg;
TaskHandle_t EmulatorLauncher::task_handler;



void EmulatorLauncher::init (
  std::shared_ptr<ESP32Interface> interface,
  std::unique_ptr<gb::GameRom> cartridge_data
) {
  EmulatorLauncher::interface = interface;
  EmulatorLauncher::cartridge_data = std::move(cartridge_data);
}


void EmulatorLauncher::launch (gb::EmulatorConfig cfg) {
  emu_cfg = cfg;

  xTaskCreatePinnedToCore(
    EmulatorLauncher::launch_,
    "EmulatorTask",
    0x18000,                 // Stack size in words
    NULL,                  // Parameter to pass into the task
    1,                     // Priority (0 is lowest, 24 is highest. 1 is safe)
    &task_handler,
    EMULATOR_CORE          // The Core ID to pin it to
  );
}


void EmulatorLauncher::launch_ (void *) {
  esp_task_wdt_delete(NULL); // Disable watchdog
  esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0)); // Disable watchdog for idle task in core 0
  gb::emulator<ESP32Interface, false>(*interface, *cartridge_data, emu_cfg);

  while (true) {
    vTaskDelay(1000);
  }
}


void EmulatorLauncher::kill () {
  if (EmulatorLauncher::task_handler != NULL) {
    vTaskDelete(EmulatorLauncher::task_handler); 
    EmulatorLauncher::task_handler = NULL; 
  }
}