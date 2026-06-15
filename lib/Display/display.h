#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <vector>
#include <array>
#include <memory>
#include <emulator/graphics/graphicstate.h>


struct ScreenMenu {
  std::string title;
  std::vector<std::string> options;
  int first = 0;
  int selection = -1;
};


class Display : public lgfx::LGFX_Device {

  static constexpr size_t MARGIN = 5;
  static constexpr size_t MAX_MENU_ITEMS = 6;
  static constexpr size_t ITEM_MARGIN = 30;
  static constexpr size_t TEXT_SIZE = 30;
  static constexpr std::array<uint16_t,4> GB_COLOR = {0xFFFF, 0xAD55, 0x52AA, 0x0000};
  static constexpr float SCREEN_UPSCALE = 240.0 / gb::SCREEN_PX_H;


  lgfx::Panel_ILI9341 _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
  std::unique_ptr<LGFX_Sprite> gb_screen;

public:

  Display();

  void init ();

  void printMenu (const ScreenMenu &menu);

  void printScreen (const gb::ScreenPixels* pixels);
};