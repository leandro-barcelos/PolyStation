#include "bus.h"

std::optional<bus::MemoryRegion> bus::GetMemoryRegionByAddress(
    const uint32_t address) {
  if (address >= kBiosBase && address < kBiosBase + kBiosSize) {
    return MemoryRegion::kBios;
  }
  return std::nullopt;
}

uint32_t bus::Bus::Load(const uint32_t address) const {
  const std::optional<MemoryRegion> region =
      bus::GetMemoryRegionByAddress(address);

  if (!region.has_value()) {
    throw std::runtime_error("failed to access an unmapped memory region!");
  }

  switch (region.value()) {
    case MemoryRegion::kBios: {
      const uint32_t offset = address - kBiosBase;
      return bios_.Load(offset);
    }
  }

  throw std::runtime_error("failed to Load memory address!");
}