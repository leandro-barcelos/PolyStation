#ifndef POLYSTATION_BUS_H
#define POLYSTATION_BUS_H
#include <optional>

#include "bios.h"

namespace bus {
enum class MemoryRegion { kBios };
constexpr uint32_t kBiosBase = 0xBFC00000;
constexpr uint32_t kBiosSize = 0x80000;

std::optional<MemoryRegion> GetMemoryRegionByAddress(uint32_t address);

class Bus {
 public:
  explicit Bus(const std::string& path) : bios_(path) {}

  [[nodiscard]] uint32_t Load(uint32_t address) const;

 private:
  bios::Bios bios_;
};
}  // namespace bus

#endif  // POLYSTATION_BUS_H