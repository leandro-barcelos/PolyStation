#include "bus.h"

#include <format>
#include <iostream>

std::optional<bus::MemoryRegion> bus::GetMemoryRegionByAddress(
    const uint32_t address) {
  if (address >= ram::kRamBase && address < ram::kRamBase + ram::kRamSize) {
    return MemoryRegion::kRam;
  }
  if (address >= kMemoryControlBase &&
      address < kMemoryControlBase + kMemoryControlSize) {
    return MemoryRegion::kMemoryControl;
  }
  if (address == kRamSizeBase) {
    return MemoryRegion::kRamSize;
  }
  if (address >= bios::kBiosBase &&
      address < bios::kBiosBase + bios::kBiosSize) {
    return MemoryRegion::kBios;
  }
  if (address == kCacheControlBase) {
    return MemoryRegion::kCacheControl;
  }
  return std::nullopt;
}

uint32_t bus::Bus::Load(const uint32_t address) const {
  if (address % 4 != 0) {
    throw std::runtime_error(
        std::format("unaligned load address: {:08X}", address));
  }

  const std::optional<MemoryRegion> region = GetMemoryRegionByAddress(address);

  if (!region.has_value()) {
    throw std::runtime_error("failed to access an unmapped memory region!");
  }

  switch (region.value()) {
    case MemoryRegion::kBios: {
      const uint32_t offset = address - bios::kBiosBase;
      return bios_.Load(offset);
    }
    case MemoryRegion::kRam: {
      const uint32_t offset = address - ram::kRamBase;
      return ram_.Load(offset);
    }
    default:
      break;
  }

  throw std::runtime_error("failed to Load memory address!");
}

void bus::Bus::Store(uint32_t address, uint32_t value) {
  if (address % 4 != 0) {
    throw std::runtime_error(
        std::format("unaligned store address: {:08X}", address));
  }

  const std::optional<MemoryRegion> region = GetMemoryRegionByAddress(address);

  if (!region.has_value()) {
    throw std::runtime_error(
        std::format("unhandled store into address: {:08X}", address));
  }

  switch (region.value()) {
    case MemoryRegion::kMemoryControl:
      switch (address - kMemoryControlBase) {
        case 0:
          if (value != 0x1f000000) {
            throw std::runtime_error(std::format(
                "tried remapping expansion 1 base address to {:08X}", value));
          }
          break;
        case 4:
          if (value != 0x1f802000) {
            throw std::runtime_error(std::format(
                "tried remapping expansion 2 base address to {:08X}", value));
          }
          break;
        default:
          std::cout << "unhandled write to memory control 1" << '\n';
          break;
      }
      break;
    case MemoryRegion::kRamSize:
      std::cout << "unhandled write to RAM_SIZE" << '\n';
      break;
    case MemoryRegion::kCacheControl:
      std::cout << "unhandled write to Cache Control register" << '\n';
      break;
    case MemoryRegion::kRam: {
      const uint32_t offset = address - ram::kRamBase;
      ram_.Store(offset, value);
      break;
    }
    default:
      throw std::runtime_error(
          std::format("unhandled store into address: {:08X}", address));
  }
}