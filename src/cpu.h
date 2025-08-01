#ifndef POLYSTATION_CPU_H_
#define POLYSTATION_CPU_H_
#include <array>

#include "bios.h"
#include "bus.h"

namespace cpu {
constexpr uint32_t kNumberOfRegisters = 32;
constexpr uint32_t kInstructionLength = 4;

class Instruction {
 public:
  explicit Instruction(const uint32_t instruction) : data_(instruction) {}

  enum class PrimaryOpcode : uint8_t {
    kSPECIAL = 0x00,
    kORI = 0x0D,
    kLUI = 0x0F,
    kSW = 0x2B
  };

  enum class SecondaryOpcode : uint8_t { kSLL = 0x00 };

  [[nodiscard]] PrimaryOpcode GetPrimaryOpcode() const;
  [[nodiscard]] SecondaryOpcode GetSecondaryOpcode() const;
  [[nodiscard]] uint8_t GetRegisterS() const;
  [[nodiscard]] uint8_t GetRegisterT() const;
  [[nodiscard]] uint8_t GetRegisterD() const;
  [[nodiscard]] uint16_t GetImmediate16() const;
  [[nodiscard]] uint32_t GetImmediate16SignExtend() const;

 private:
  uint32_t data_;
};

class CPU {
 public:
  explicit CPU(const std::string& path) : bus_(path) {}

  void Cycle();

 private:
  uint32_t program_counter_ = bus::kBiosBase;
  std::array<uint32_t, kNumberOfRegisters> registers_{};
  bus::Bus bus_;

  [[nodiscard]] uint32_t Load(uint32_t address) const;
  static void Store(uint32_t address, uint32_t value);

  void OpORI(const Instruction& instruction);
  void OpLUI(const Instruction& instruction);
  void OpSW(const Instruction& instruction);
  void OpSLL(const Instruction& instruction);
};
}  // namespace cpu

#endif  // POLYSTATION_CPU_H_