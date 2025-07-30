#ifndef POLYSTATION_BUS_H
#define POLYSTATION_BUS_H
#include <optional>
#include "bios.h"

namespace bus {
enum class MemoryRegion { BIOS };

std::optional<MemoryRegion> getMemoryRegionByAddress(uint32_t address);

class Bus {
public:
  explicit Bus(const std::string& path) : bios(path) {
  }

  [[nodiscard]] uint32_t load(uint32_t address) const;

private:
  bios::Bios bios;
};
} // namespace bus

#endif  // POLYSTATION_BUS_H