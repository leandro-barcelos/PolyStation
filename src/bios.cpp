#include "bios.h"

#include <fstream>

bios::Bios::Bios(const std::string& path) {
  data_.resize(kBiosSize);
  std::ifstream input(path, std::ios::binary);
  input.read(reinterpret_cast<char*>(data_.data()), kBiosSize);
  input.close();
}

uint32_t bios::Bios::Load32(const uint32_t offset) const {
  const auto byte_0 = std::to_integer<uint32_t>(data_[offset + 0]);
  const auto byte_1 = std::to_integer<uint32_t>(data_[offset + 1]);
  const auto byte_2 = std::to_integer<uint32_t>(data_[offset + 2]);
  const auto byte_3 = std::to_integer<uint32_t>(data_[offset + 3]);

  return byte_0 | byte_1 << 8U | byte_2 << 16U | byte_3 << 24U;
}