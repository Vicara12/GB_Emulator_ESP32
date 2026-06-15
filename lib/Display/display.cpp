#include "display.h"


Display::Display ()
  : gb_screen(std::make_unique<LGFX_Sprite>(this))
{
  // Configure the SPI Bus
  auto bus_cfg = _bus_instance.config();
  bus_cfg.spi_host = SPI2_HOST;
  bus_cfg.spi_mode = 0;
  bus_cfg.freq_write = 20000000; // Back up to 40MHz for emulator speed!
  bus_cfg.pin_sclk = 12;
  bus_cfg.pin_mosi = 11;
  bus_cfg.pin_miso = 13;
  bus_cfg.pin_dc   = 9;
  _bus_instance.config(bus_cfg);
  _panel_instance.setBus(&_bus_instance);

  // Configure the Panel
  auto panel_cfg = _panel_instance.config();
  panel_cfg.pin_cs   = 10;
  panel_cfg.pin_rst  = 14;
  panel_cfg.pin_busy = -1;
  panel_cfg.panel_width  = 240;
  panel_cfg.panel_height = 320;
  panel_cfg.offset_x     = 0;
  panel_cfg.offset_y     = 0;
  panel_cfg.invert       = false;
  panel_cfg.rgb_order    = false;
  _panel_instance.config(panel_cfg);

  setPanel(&_panel_instance);
}


void Display::init () {
  lgfx::LGFX_Device::init();
  setRotation(1);
  fillScreen(TFT_BLACK);

  gb_screen->setColorDepth(16);
  gb_screen->createSprite(gb::SCREEN_PX_W, gb::SCREEN_PX_H);
  gb_screen->setPivot(0, 0);
}


void Display::printMenu (const ScreenMenu &menu) {
  setTextSize(3);
  setTextColor(TFT_WHITE, TFT_BLACK);
  setCursor(MARGIN, MARGIN);
  println(menu.title.c_str());

  setTextSize(2);

  size_t max_elms = menu.options.size() - menu.first;
  if (max_elms < 0) {
    return;
  }

  for (int i = 0; i < std::min(MAX_MENU_ITEMS, max_elms); i++) {
    int list_idx = i + menu.first;
    size_t v_pos = 3*MARGIN + TEXT_SIZE*(i + 1);
    if (list_idx == menu.selection) {
      setCursor(2*MARGIN, v_pos);
      println(">");
    }
    setCursor(ITEM_MARGIN, v_pos);
    println(menu.options[list_idx].c_str());
  }
}




void Display::printScreen (const gb::ScreenPixels* pixels) {
  for (size_t y = 0; y < gb::SCREEN_PX_H; y++) {
    for (size_t x = 0; x < gb::SCREEN_PX_W; x++) {
      gb_screen->drawPixel(x, y, GB_COLOR[(*pixels)[y][x]]);
    }
  }

  gb_screen->pushRotateZoom(0, 0, 0, SCREEN_UPSCALE, SCREEN_UPSCALE);
}