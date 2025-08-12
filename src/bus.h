#ifndef POLYSTATION_BUS_H
#define POLYSTATION_BUS_H
#include <optional>

#include "bios.h"
#include "ram.h"

namespace bus {
constexpr uint32_t kMemoryControlBase = 0x1F801000;
constexpr uint32_t kMemoryControlSize = 0x24;
constexpr uint32_t kRamSizeBase = 0x1F801060;
constexpr uint32_t kCacheControlBase = 0xFFFE0130;

constexpr std::array<uint32_t, 8> kRegionMask{
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0x7fffffff, 0x1fffffff, 0xffffffff, 0xffffffff,
};

enum class MemoryRegion : uint8_t {
  kBios,
  kMemoryControl,
  kRamSize,
  kCacheControl,
  kRam
};

uint32_t MaskRegion(uint32_t address);

std::optional<MemoryRegion> GetMemoryRegionByAddress(uint32_t address);

class Bus {
 public:
  explicit Bus(const std::string& path) : bios_(path) {}

  [[nodiscard]] uint32_t Load(uint32_t address) const;
  void Store(uint32_t address, uint32_t value);
  void Store(uint32_t address, uint16_t value);

 private:
  bios::Bios bios_;
  ram::Ram ram_;
};
}  // namespace bus

#endif  // POLYSTATION_BUS_H