#include "display.h"
#include <algorithm>


Display::Display ()
  : gb_screen(std::make_unique<LGFX_Sprite>(this))
{
  // Configure the SPI Bus
  auto bus_cfg = _bus_instance.config();
  bus_cfg.spi_host = SPI2_HOST;
  bus_cfg.spi_mode = 0;
  bus_cfg.freq_write = 40'000'000; // Back up to 40MHz for emulator speed!
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


void Display::releaseBus () {
  // Make sure the panel is deselected so it can't react to SD traffic
  digitalWrite(10, HIGH); // CS pin
  _bus_instance.release();
}


void Display::acquireBus () {
  _bus_instance.init();
  _panel_instance.init(false);
}


void Display::init () {
  lgfx::LGFX_Device::init();
  setRotation(1);
  fillScreen(TFT_BLACK);

  gb_screen->setColorDepth(16);
  gb_screen->setPsram(false);
  gb_screen->createSprite(gb::SCREEN_PX_W, gb::SCREEN_PX_H);
  gb_screen->setPivot(0, 0);
}


void Display::printMenu (const ScreenMenu &menu, int first, int selection) {
  setTextSize(3);
  setTextColor(TFT_WHITE, TFT_BLACK);
  setCursor(MARGIN, MARGIN);
  println(menu.title.c_str());

  setTextSize(2);

  size_t max_elms = menu.options.size() - first;
  if (max_elms < 0) {
    return;
  }

  for (int i = 0; i < std::min(MAX_MENU_ITEMS, max_elms); i++) {
    int list_idx = i + first;
    size_t v_pos = 3*MARGIN + TEXT_SIZE*(i + 1);
    setCursor(2*MARGIN, v_pos);
    println(list_idx == selection ? ">" : " ");
    setCursor(ITEM_MARGIN, v_pos);
    println(menu.options[list_idx].c_str());
  }
}

void Display::printError (const ScreenMenu &menu, const std::string &msg, int selection) {
  setTextSize(3);
  setTextColor(TFT_WHITE, TFT_BLACK);
  setCursor(MARGIN, MARGIN);
  println(menu.title.c_str());

  setTextSize(2.5);

  setTextDatum(textdatum_t::top_center);
  drawString(msg.c_str(), width()/2, 3*MARGIN + 1.25*TEXT_SIZE);
  setTextDatum(textdatum_t::top_left);

  setTextSize(2);

  for (int i = 0; i < std::min(size_t(3), menu.options.size()); i++) {
    size_t v_pos = 3*MARGIN + TEXT_SIZE*(i + 3);
    setCursor(2*MARGIN, v_pos);
    println(i == selection ? ">" : " ");
    setCursor(ITEM_MARGIN, v_pos);
    println(menu.options[i].c_str());
  }
}


ScreenMenu Display::beautifyMenu (ScreenMenu &&menu) {
  if (menu.options.size() == 0) {
    return menu;
  }

  auto max_len_item = std::max_element(menu.options.begin(), menu.options.end(),
    [](const std::string &a, const std::string &b) {return a.length() < b.length();}
  );
  auto max_len = std::min(max_len_item->size(), MAX_TEXT_LEN);

  for (auto &opt : menu.options) {
    if (opt.length() > max_len) {
      opt = opt.substr(0, MAX_TEXT_LEN - 3) + "...";
    }
    else {
      opt.append(max_len - opt.length(), ' ');
    }
  }
  return menu;
}


void Display::clearScreen () {
  fillScreen(TFT_BLACK);
}


void Display::printScreen (const gb::ScreenPixels* pixels) {
  uint16_t* raw_buffer = (uint16_t*)gb_screen->getBuffer();
  size_t buffer_idx = 0;
  for (size_t y = 0; y < gb::SCREEN_PX_H; y++) {
    for (size_t x = 0; x < gb::SCREEN_PX_W; x++) {
      raw_buffer[buffer_idx++] = GB_COLOR[(*pixels)[y][x]];
    }
  }

  gb_screen->pushRotateZoom(0, 0, 0, SCREEN_UPSCALE, SCREEN_UPSCALE);
}