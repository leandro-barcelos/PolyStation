#include "bus.h"

std::optional<bus::MemoryRegion> bus::getMemoryRegionByAddress(
    const uint32_t address) {
  if (address >= bios::BIOS_BASE && address < bios::BIOS_BASE +
      bios::BIOS_SIZE) {
    return MemoryRegion::BIOS;
  }
  return std::nullopt;
}

uint32_t bus::Bus::load(const uint32_t address) const {
  const std::optional<MemoryRegion> region =
      bus::getMemoryRegionByAddress(address);

  if (!region.has_value()) {
    throw std::runtime_error("failed to access an unmapped memory region!");
  }

  switch (region.value()) {
    case MemoryRegion::BIOS: {
      const uint32_t offset = address - bios::BIOS_BASE;
      return bios.load(offset);
    }
  }

  throw std::runtime_error("failed to load memory address!");
}