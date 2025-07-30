#ifndef POLYSTATION_CPU_H_
#define POLYSTATION_CPU_H_
#include <array>

#include "bios.h"
#include "bus.h"

namespace cpu {
constexpr uint32_t kNumberOfRegisters = 32;
constexpr uint32_t kInstructionLength = 4;
constexpr uint8_t kOriOpcode = 0x0D;
constexpr uint8_t kLuiOpcode = 0x0F;

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

class Instruction {
 public:
  explicit Instruction(const uint32_t instruction) : data_(instruction) {}

  [[nodiscard]] uint8_t Opcode() const;
  [[nodiscard]] uint8_t RegisterS() const;
  [[nodiscard]] uint8_t RegisterT() const;
  [[nodiscard]] uint16_t Immediate16() const;

 private:
  uint32_t data_;
};
}  // namespace cpu

#endif  // POLYSTATION_CPU_H_