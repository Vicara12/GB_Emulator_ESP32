#pragma once

#include <vector>
#include <string>

class Storage {
  static constexpr uint16_t SD_CS   =  8;
  static constexpr uint16_t SD_MOSI = 11;
  static constexpr uint16_t SD_CLK  = 12;
  static constexpr uint16_t SD_MISO = 13;

public:

  static std::string fileName (const std::string &full_name);

  static void init ();

  static std::vector<std::string> availableFiles (std::string path);

  static std::vector<uint8_t> readFile (const std::string &path);
};