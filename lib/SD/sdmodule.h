#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include <SPI.h>
#include <SD.h>
#include <emulator/emulator.h>


class SDModule {

  static constexpr uint8_t SD_CS   = 8;
  static constexpr uint8_t SD_SCLK = 12;
  static constexpr uint8_t SD_MISO = 13;
  static constexpr uint8_t SD_MOSI = 11;

  static constexpr const char *kSavedGamesDir = "/saved_games";
  static constexpr const char *kGameExtension  = ".gb";
  static constexpr const char *kSaveExtension  = ".save";

  SPIClass _sdSpi{HSPI};
  std::function<void()> _releaseBus;
  std::function<void()> _acquireBus;
  int _busDepth = 0;


  struct BusGuard {
    SDModule *self;
    explicit BusGuard (SDModule *self) : self(self) {
      if (self->_busDepth++ == 0) {
        if (self->_releaseBus) self->_releaseBus();
        self->_sdSpi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);
        SD.begin(SD_CS, self->_sdSpi, 20'000'000);
      }
    }
    ~BusGuard () {
      if (--self->_busDepth == 0) {
        SD.end();
        self->_sdSpi.end();
        if (self->_acquireBus) self->_acquireBus();
      }
    }
  };


  bool ensureDirectoryExists (const std::string &path);

  std::string savedGameDirForGame (const std::string &game) const;

  std::string savedGamePath (const std::string &game, const std::string &save) const;

  gb::ScreenPixels loadScreen (File &file);

public:

  struct SavedGameInfo {
    std::string name;
    gb::ScreenPixels screen;
  };

  struct SavedGame {
    SavedGameInfo info;
    std::vector<uint8_t> ram_data;
  };

  bool init ();

  void setBusHandover (std::function<void()> releaseBus, std::function<void()> acquireBus);

  std::vector<std::string> listGames ();

  std::unique_ptr<gb::GameRom> loadGame (const std::string &game);

  std::vector<SavedGameInfo> listSavedGames (const std::string &game);

  SavedGame loadSavedGame (const std::string &game, const std::string &save);

  void removeSavedGame (const std::string &game, const std::string &save);

  std::string newSavedGame (const std::string &game, SavedGame &&data);

};