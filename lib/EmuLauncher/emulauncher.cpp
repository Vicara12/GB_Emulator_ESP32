#include "emulauncher.h"
#include <esp_task_wdt.h>


std::shared_ptr<ESP32Interface> EmulatorLauncher::interface;
std::unique_ptr<gb::GameRom> EmulatorLauncher::cartridge_data;
std::unique_ptr<gb::GameRom> EmulatorLauncher::save_game;
gb::EmulatorConfig EmulatorLauncher::emu_cfg;
TaskHandle_t EmulatorLauncher::task_handler;
bool EmulatorLauncher::start_emu = false;



void EmulatorLauncher::emulate (
  std::shared_ptr<ESP32Interface> interface,
  gb::EmulatorConfig cfg,
  std::unique_ptr<gb::GameRom> cartridge_data,
  std::unique_ptr<gb::GameRom> save_game
) {
  EmulatorLauncher::interface = interface;
  emu_cfg = cfg;
  EmulatorLauncher::cartridge_data = std::move(cartridge_data);
  EmulatorLauncher::save_game = std::move(save_game);
  start_emu = true;
}


void EmulatorLauncher::launch () {
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
  while (true) {
    vTaskDelay(100);
    if (start_emu) {
      start_emu = false;
      gb::emulator<ESP32Interface, gb::BuildCfb::FastGraphics>(
        *interface,
        *cartridge_data,
        std::move(emu_cfg)
      );
    }
  }
}


void EmulatorLauncher::kill () {
  if (EmulatorLauncher::task_handler != NULL) {
    vTaskDelete(EmulatorLauncher::task_handler); 
    EmulatorLauncher::task_handler = NULL; 
  }
}