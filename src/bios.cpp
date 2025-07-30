#include "bios.h"
#include <fstream>

bios::Bios::Bios(const std::string& path) {
  data.resize(BIOS_SIZE);
  std::ifstream input(path, std::ios::binary);
  input.read(reinterpret_cast<char*>(data.data()), BIOS_SIZE);
  input.close();
}

uint32_t bios::Bios::load(const uint32_t offset) const {
  const auto byte_0 = std::to_integer<uint32_t>(data[offset + 0]);
  const auto byte_1 = std::to_integer<uint32_t>(data[offset + 1]);
  const auto byte_2 = std::to_integer<uint32_t>(data[offset + 2]);
  const auto byte_3 = std::to_integer<uint32_t>(data[offset + 3]);

  return byte_0 | byte_1 << 8U | byte_2 << 16U | byte_3 << 24U;
}