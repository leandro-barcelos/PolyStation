#ifndef POLYSTATION_CPU_H_
#define POLYSTATION_CPU_H_
#include <array>

#include "bios.h"
#include "bus.h"

namespace cpu {
constexpr uint32_t kNumberOfRegisters = 32;
constexpr uint32_t kInstructionLength = 4;
constexpr uint32_t kLuiOpcode = 0x0F;

class CPU {
 public:
  explicit CPU(const std::string& path) : bus_(path) {}

  void Cycle();

 private:
  uint32_t program_counter_ = bios::kBiosBase;
  std::array<uint32_t, kNumberOfRegisters> registers_{};
  bus::Bus bus_;

  [[nodiscard]] uint32_t Load(uint32_t address) const;
};
}  // namespace cpu

#endif  // POLYSTATION_CPU_H_