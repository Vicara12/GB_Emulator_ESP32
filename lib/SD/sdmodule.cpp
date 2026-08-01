#include "sdmodule.h"

#include <SD.h>
#include <algorithm>
#include <cctype>
#include <cstdlib>


namespace {

// Strips a single leading '/' from a path, if present.
std::string stripLeadingSlash (const std::string &path) {
  if (not path.empty() and path.front() == '/') {
    return path.substr(1);
  }
  return path;
}

// Keeps only the last path component (i.e. the filename) of `path`.
std::string getBasename (const std::string &path) {
  size_t slash = path.find_last_of('/');
  if (slash == std::string::npos) {
    return path;
  }
  return path.substr(slash + 1);
}

bool endsWith (const std::string &name, const std::string &suffix) {
  if (name.size() < suffix.size()) {
    return false;
  }
  return name.compare(name.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string stripSuffix (const std::string &name, const std::string &suffix) {
  if (endsWith(name, suffix)) {
    return name.substr(0, name.size() - suffix.size());
  }
  return name;
}

bool isAllDigits (const std::string &s) {
  return not s.empty() and std::all_of(s.begin(), s.end(),
                                    [](unsigned char c) { return std::isdigit(c) != 0; });
}

} // namespace


bool SDModule::init () {
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  static SPIClass sdSpi(HSPI);
  sdSpi.begin(SD_SCLK, SD_MISO, SD_MOSI, SD_CS);

  if (not SD.begin(SD_CS, sdSpi)) {
      return false;
  }

  return SD.cardType() != CARD_NONE;
}

std::vector<std::string> SDModule::listGames () {
  std::vector<std::string> games;

  File root = SD.open("/");
  if (not root or not root.isDirectory()) {
    return games;
  }

  for (File entry = root.openNextFile(); entry; entry = root.openNextFile()) {
    if (not entry.isDirectory()) {
      std::string name = getBasename(stripLeadingSlash(entry.name()));
      if (endsWith(name, kGameExtension)) {
        games.push_back(stripSuffix(name, kGameExtension));
      }
    }
    entry.close();
  }

  root.close();
  return games;
}


std::unique_ptr<gb::GameRom> SDModule::loadGame (const std::string &game) {
  std::unique_ptr<gb::GameRom> data;

  std::string path = "/" + game + kGameExtension;
  File file = SD.open(path.c_str(), FILE_READ);
  if (not file) {
    return data;
  }

  data = std::make_unique<gb::GameRom>();

  data->resize(file.size());
  if (not data->empty()) {
    file.read(data->data(), data->size());
  }

  file.close();
  return data;
}


std::vector<std::string> SDModule::listSavedGames (const std::string &game) {
  std::vector<std::string> saves;

  std::string dirPath = savedGameDirForGame(game);
  if (not SD.exists(dirPath.c_str())) {
    return saves;
  }

  File dir = SD.open(dirPath.c_str());
  if (not dir or not dir.isDirectory()) {
    return saves;
  }

  for (File entry = dir.openNextFile(); entry; entry = dir.openNextFile()) {
    if (not entry.isDirectory()) {
      std::string name = getBasename(stripLeadingSlash(entry.name()));
      if (endsWith(name, kSaveExtension)) {
        saves.push_back(stripSuffix(name, kSaveExtension));
      }
    }
    entry.close();
  }

  dir.close();
  return saves;
}


SDModule::SavedGame SDModule::loadSavedGame (const std::string &game, const std::string &save) {
  SavedGame result{};

  std::string path = savedGamePath(game, save);
  File file = SD.open(path.c_str(), FILE_READ);
  if (not file) {
    return result;
  }

  for (auto &row : result.screen) {
    file.read(row.data(), row.size());
  }

  uint32_t ramSize = 0;
  file.read(reinterpret_cast<uint8_t *>(&ramSize), sizeof(ramSize));

  result.ram_data.resize(ramSize);
  if (ramSize > 0) {
    file.read(result.ram_data.data(), ramSize);
  }

  file.close();
  return result;
}


void SDModule::removeSavedGame (const std::string &game, const std::string &save) {
  std::string path = savedGamePath(game, save);
  if (SD.exists(path.c_str())) {
    SD.remove(path.c_str());
  }
}


std::string SDModule::newSavedGame (const std::string &game, SavedGame &&data) {
  std::string dirPath = savedGameDirForGame(game);
  if (not ensureDirectoryExists(dirPath)) {
    return "";
  }

  // Saves are always named as plain increasing integers; find the
  // current highest one so the new save can be numbered highest + 1.
  int highest = 0;
  for (const auto &save : listSavedGames(game)) {
    if (isAllDigits(save)) {
      highest = std::max(highest, std::atoi(save.c_str()));
    }
  }

  std::string newName = std::to_string(highest + 1);
  std::string path = savedGamePath(game, newName);

  File file = SD.open(path.c_str(), FILE_WRITE);
  if (not file) {
    return "";
  }

  for (const auto &row : data.screen) {
    file.write(row.data(), row.size());
  }

  uint32_t ramSize = static_cast<uint32_t>(data.ram_data.size());
  file.write(reinterpret_cast<const uint8_t *>(&ramSize), sizeof(ramSize));
  if (ramSize > 0) {
    file.write(data.ram_data.data(), ramSize);
  }

  file.close();
  return newName;
}


bool SDModule::ensureDirectoryExists (const std::string &path) {
  if (path.empty() or path == "/") {
    return true;
  }

  size_t pos = 0;
  while (pos < path.size()) {
    size_t next = path.find('/', pos + 1);
    if (next == std::string::npos) {
      next = path.size();
    }

    std::string current = path.substr(0, next);
    if (not current.empty() and not SD.exists(current.c_str())) {
      if (not SD.mkdir(current.c_str())) {
        return false;
      }
    }

    pos = next;
  }

  return true;
}


std::string SDModule::savedGameDirForGame (const std::string &game) const {
  return std::string(kSavedGamesDir) + "/" + game;
}


std::string SDModule::savedGamePath (const std::string &game, const std::string &save) const {
  return savedGameDirForGame(game) + "/" + save + kSaveExtension;
}