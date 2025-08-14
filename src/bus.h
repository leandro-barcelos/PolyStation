#ifndef POLYSTATION_BUS_H
#define POLYSTATION_BUS_H
#include <optional>

#include "bios.h"
#include "ram.h"

namespace bus {
struct MemoryRange {
  uint32_t base;
  uint32_t size;

  [[nodiscard]] bool InRange(uint32_t address) const;
} __attribute__((aligned(8)));

constexpr MemoryRange kBios = {.base = 0x1FC00000, .size = 0x80000};
constexpr MemoryRange kRam = {.base = 0x00000000, .size = 0x200000};
constexpr MemoryRange kMemoryControl{.base = 0x1F801000, .size = 0x24};
constexpr MemoryRange kRamSize{.base = 0x1F801060, .size = 0x4};
constexpr MemoryRange kCacheControl{.base = 0xFFFE0130, .size = 0x4};
constexpr MemoryRange kSpuControl{.base = 0x1F801D80, .size = 0x40};
constexpr MemoryRange kExpansionRegion2IntDipPost{.base = 0x1F802000,
                                                  .size = 0x71};
constexpr MemoryRange kExpansion1{.base = 0x1F000000, .size = 0xB0};

constexpr std::array<uint32_t, 8> kRegionMask{
    0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff,
    0x7fffffff, 0x1fffffff, 0xffffffff, 0xffffffff,
};

enum class MemoryRegion : uint8_t {
  kBios,
  kMemoryControl,
  kRamSize,
  kCacheControl,
  kRam,
  kSpuControl,
  kExpansionRegion2IntDipPost,
  kExpansion1
};

uint32_t MaskRegion(uint32_t address);

std::optional<MemoryRegion> GetMemoryRegionByAddress(uint32_t address);

class Bus {
 public:
  explicit Bus(const std::string& path) : bios_(path) {}

  [[nodiscard]] uint32_t Load32(uint32_t address) const;
  [[nodiscard]] uint8_t Load8(uint32_t address) const;

  void Store32(uint32_t address, uint32_t value);
  void Store16(uint32_t address, uint16_t value);
  void Store8(uint32_t address, uint8_t value);

 private:
  bios::Bios bios_;
  ram::Ram ram_;
};
}  // namespace bus

#endif  // POLYSTATION_BUS_H