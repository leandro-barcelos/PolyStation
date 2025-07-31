#ifndef POLYSTATION_CPU_H_
#define POLYSTATION_CPU_H_
#include <array>

#include "bios.h"
#include "bus.h"

namespace cpu {
constexpr uint32_t kNumberOfRegisters = 32;
constexpr uint32_t kInstructionLength = 4;

class CPU {
 public:
  explicit CPU(const std::string& path) : bus_(path) {}

  void Cycle();

 private:
  uint32_t program_counter_ = bus::kBiosBase;
  std::array<uint32_t, kNumberOfRegisters> registers_{};
  bus::Bus bus_;

  [[nodiscard]] uint32_t Load(uint32_t address) const;
};

class Instruction {
 public:
  explicit Instruction(const uint32_t instruction) : data_(instruction) {}

  enum class Opcode : uint8_t { kORI = 0x0D, kLUI = 0x0F };

  [[nodiscard]] Opcode GetOpcode() const;
  [[nodiscard]] uint8_t GetRegisterS() const;
  [[nodiscard]] uint8_t GetRegisterT() const;
  [[nodiscard]] uint16_t GetImmediate16() const;

 private:
  uint32_t data_;
};
}  // namespace cpu

#endif  // POLYSTATION_CPU_H_