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

enum class MemoryRegion : uint8_t {
  kBios,
  kMemoryControl,
  kRamSize,
  kCacheControl,
  kRam
};

std::optional<MemoryRegion> GetMemoryRegionByAddress(uint32_t address);

class Bus {
 public:
  explicit Bus(const std::string& path) : bios_(path) {}

  [[nodiscard]] uint32_t Load(uint32_t address) const;
  static void Store(uint32_t address, uint32_t value);

 private:
  bios::Bios bios_;
  ram::Ram ram_;
};
}  // namespace bus

#endif  // POLYSTATION_BUS_H