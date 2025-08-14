#include "bus.h"

#include <format>
#include <gsl/util>
#include <iostream>

bool bus::MemoryRange::InRange(const uint32_t address) const {
  return address >= base && address < base + size;
}

uint32_t bus::MaskRegion(const uint32_t address) {
  const uint32_t index = address >> 29;
  return address & gsl::at(kRegionMask, index);
}

std::optional<bus::MemoryRegion> bus::GetMemoryRegionByAddress(
    const uint32_t address) {
  if (kRam.InRange(address)) {
    return MemoryRegion::kRam;
  }
  if (kMemoryControl.InRange(address)) {
    return MemoryRegion::kMemoryControl;
  }
  if (kRamSize.InRange(address)) {
    return MemoryRegion::kRamSize;
  }
  if (kSpuControl.InRange(address)) {
    return MemoryRegion::kSpuControl;
  }
  if (kBios.InRange(address)) {
    return MemoryRegion::kBios;
  }
  if (kCacheControl.InRange(address)) {
    return MemoryRegion::kCacheControl;
  }
  if (kExpansionRegion2IntDipPost.InRange(address)) {
    return MemoryRegion::kExpansionRegion2IntDipPost;
  }
  return std::nullopt;
}

uint32_t bus::Bus::Load32(uint32_t address) const {
  address = MaskRegion(address);

  if (address % 4 != 0) {
    throw std::runtime_error(
        std::format("unaligned load address: {:08X}", address));
  }

  const std::optional<MemoryRegion> region = GetMemoryRegionByAddress(address);

  if (!region.has_value()) {
    throw std::runtime_error(
        std::format("unhandled store into address: {:08X}", address));
  }

  switch (region.value()) {
    case MemoryRegion::kBios: {
      const uint32_t offset = address - kBios.base;
      return bios_.Load32(offset);
    }
    case MemoryRegion::kRam: {
      const uint32_t offset = address - kRam.base;
      return ram_.Load32(offset);
    }
    default:
      throw std::runtime_error(
          std::format("unhandled load in address: {:08X}", address));
  }
}

uint8_t bus::Bus::Load8(uint32_t address) const {
  address = MaskRegion(address);

  const std::optional<MemoryRegion> region = GetMemoryRegionByAddress(address);

  if (!region.has_value()) {
    throw std::runtime_error(
        std::format("unhandled store into address: {:08X}", address));
  }

  switch (region.value()) {
    case MemoryRegion::kBios: {
      const uint32_t offset = address - kBios.base;
      return bios_.Load8(offset);
    }
    default:
      throw std::runtime_error(
          std::format("unhandled load in address: {:08X}", address));
  }
}

void bus::Bus::Store32(uint32_t address, uint32_t value) {
  address = MaskRegion(address);

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
      switch (address - kMemoryControl.base) {
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
      const uint32_t offset = address - kRam.base;
      ram_.Store32(offset, value);
      break;
    }
    case MemoryRegion::kSpuControl:
      std::cout << "unhandled write to SPU control registers" << '\n';
      break;
    default:
      throw std::runtime_error(
          std::format("unhandled store into address: {:08X}", address));
  }
}

void bus::Bus::Store16(uint32_t address, uint16_t value) {
  address = MaskRegion(address);

  if (address % 2 != 0) {
    throw std::runtime_error(
        std::format("unaligned store address: {:08X}", address));
  }

  const std::optional<MemoryRegion> region = GetMemoryRegionByAddress(address);

  if (!region.has_value()) {
    throw std::runtime_error(
        std::format("unhandled store into address: {:08X}", address));
  }

  switch (region.value()) {
    case MemoryRegion::kSpuControl:
      std::cout << "unhandled write to SPU control registers" << '\n';
      break;
    default:
      throw std::runtime_error(
          std::format("unhandled store into address: {:08X}", address));
  }
}

void bus::Bus::Store8(uint32_t address, uint8_t value) {
  address = MaskRegion(address);

  const std::optional<MemoryRegion> region = GetMemoryRegionByAddress(address);

  if (!region.has_value()) {
    throw std::runtime_error(
        std::format("unhandled store into address: {:08X}", address));
  }

  switch (region.value()) {
    case MemoryRegion::kExpansionRegion2IntDipPost:
      std::cout << "unhandled write to Expansion Region 2 (Int/Dip/Post)"
                << '\n';
      break;
    default:
      throw std::runtime_error(
          std::format("unhandled store into address: {:08X}", address));
  }
}