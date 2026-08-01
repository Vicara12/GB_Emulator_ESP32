#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include <emulator/emulator.h>


class SDModule {

  static constexpr uint8_t SD_CS = 8;
  static constexpr uint8_t SD_SCLK = 12;
  static constexpr uint8_t SD_MISO = 13;
  static constexpr uint8_t SD_MOSI = 11;

  static constexpr const char *kSavedGamesDir = "/saved_games";
  static constexpr const char *kGameExtension  = ".gb";
  static constexpr const char *kSaveExtension  = ".save";

  bool ensureDirectoryExists (const std::string &path);

  std::string savedGameDirForGame (const std::string &game) const;

  std::string savedGamePath (const std::string &game, const std::string &save) const;

public:

  struct SavedGame {
    std::array<std::array<uint8_t, gb::SCREEN_PX_W>, gb::SCREEN_PX_H> screen;
    std::vector<uint8_t> ram_data;
  };

  bool init ();

  std::vector<std::string> listGames ();

  std::unique_ptr<gb::GameRom> loadGame (const std::string &game);

  std::vector<std::string> listSavedGames (const std::string &game);

  SavedGame loadSavedGame (const std::string &game, const std::string &save);

  void removeSavedGame (const std::string &game, const std::string &save);

  std::string newSavedGame (const std::string &game, SavedGame &&data);

};