#include "ram.h"

#include <cstddef>
#include <gsl/util>

uint32_t ram::Ram::Load32(const uint32_t offset) const {
  const auto byte_0 = std::to_integer<uint32_t>(data_[offset + 0]);
  const auto byte_1 = std::to_integer<uint32_t>(data_[offset + 1]);
  const auto byte_2 = std::to_integer<uint32_t>(data_[offset + 2]);
  const auto byte_3 = std::to_integer<uint32_t>(data_[offset + 3]);

  return byte_0 | byte_1 << 8U | byte_2 << 16U | byte_3 << 24U;
}

void ram::Ram::Store32(const uint32_t offset, const uint32_t value) {
  const unsigned char byte_0 = value & 0xFF;
  const unsigned char byte_1 = (value >> 8U) & 0xFF;
  const unsigned char byte_2 = (value >> 16U) & 0xFF;
  const unsigned char byte_3 = (value >> 24U) & 0xFF;

  gsl::at(data_, offset + 0) = static_cast<std::byte>(byte_0);
  gsl::at(data_, offset + 1) = static_cast<std::byte>(byte_1);
  gsl::at(data_, offset + 2) = static_cast<std::byte>(byte_2);
  gsl::at(data_, offset + 3) = static_cast<std::byte>(byte_3);
}