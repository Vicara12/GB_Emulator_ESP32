#include "storage.h"
#include <SD.h>
#include <string>



std::string Storage::fileName (const std::string &full_name) {
  size_t last_slash_pos = full_name.find_last_of('/');
  if (last_slash_pos != std::string::npos) {
    return full_name.substr(last_slash_pos + 1);
  }
  return full_name;
}


void Storage::init() {
  pinMode(SD_MISO, INPUT_PULLUP);
  pinMode(SD_MOSI, INPUT_PULLUP);
  pinMode(SD_CS,   INPUT_PULLUP);

  SPI.begin(SD_CLK, SD_MISO, SD_MOSI, SD_CS);

  if (not SD.begin(SD_CS, SPI)) {
    throw std::runtime_error("SD Card Mount Failed");
  }

  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    throw std::runtime_error("no SD card attached");
  }
}


std::vector<std::string> Storage::availableFiles(std::string path) {
  std::vector<std::string> fileList;
  File root = SD.open(path.c_str());
  if (not root or not root.isDirectory()) {
    throw std::runtime_error("failed to open directory:" + path);
  }

  File file = root.openNextFile();
  while (file) {
    if (not file.isDirectory()) {
      fileList.push_back(file.name());
    }
    file = root.openNextFile();
  }
  
  return fileList;
}


std::vector<uint8_t> Storage::readFile(const std::string &path) {
  std::vector<uint8_t> buffer;
  File file = SD.open(path.c_str(), FILE_READ);
  if (not file or file.isDirectory()) {
    throw std::runtime_error("failed to open file:" + path);
  }
  size_t fileSize = file.size();
  if (fileSize > 0) {
    buffer.resize(fileSize);
    file.read(buffer.data(), fileSize);
  }
  file.close();
  return buffer;
}