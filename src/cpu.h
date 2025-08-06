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
  Instruction() : data_(0) {}
  explicit Instruction(const uint32_t instruction) : data_(instruction) {}

  std::string ToString() const;

  enum class PrimaryOpcode : uint8_t {
    kSPECIAL = 0x00,
    kJ = 0x02,
    kBNE = 0x05,
    kADDIU = 0x09,
    kORI = 0x0D,
    kLUI = 0x0F,
    kCOP0 = 0x10,
    kSW = 0x2B,
    kNOP = 0xFF
  };

  enum class SecondaryOpcode : uint8_t { kSLL = 0x00, kOR = 0x25 };

  enum class CoprocessorOpcode : uint8_t { kMTC = 0x04 };

  [[nodiscard]] uint32_t GetRawData() const { return data_; }
  [[nodiscard]] PrimaryOpcode GetPrimaryOpcode() const;
  [[nodiscard]] SecondaryOpcode GetSecondaryOpcode() const;
  [[nodiscard]] CoprocessorOpcode GetCoprocessorOpcode(bool flag) const;
  [[nodiscard]] uint8_t GetCoprocessor() const;
  [[nodiscard]] bool GetCoprocessorFlag() const;
  [[nodiscard]] uint8_t GetRegisterS() const;
  [[nodiscard]] uint8_t GetRegisterT() const;
  [[nodiscard]] uint8_t GetRegisterD() const;
  [[nodiscard]] uint16_t GetImmediate16() const;
  [[nodiscard]] uint32_t GetImmediate16SignExtend() const;
  [[nodiscard]] uint32_t GetImmediate26() const;

 private:
  uint32_t data_;
};

struct COP0 {
  uint32_t status_register = 0;

  enum Registers : uint8_t { kStatusRegister = 0xC };
};

class CPU {
 public:
  explicit CPU(const std::string& path) : bus_(path) { registers_[0] = 0; }

  void Reset();
  void Cycle();
  [[nodiscard]] uint32_t GetRegister(uint32_t index) const;
  void SetRegister(uint32_t index, uint32_t value);
  [[nodiscard]] unsigned long long GetStepCount() const;
  [[nodiscard]] uint32_t GetPC() const;
  [[nodiscard]] uint32_t Load(uint32_t address) const;
  [[nodiscard]] COP0 GetCop0() const;

 private:
  uint32_t program_counter_ = bus::kBiosBase;
  Instruction next_instruction_{0x0};
  std::array<uint32_t, kNumberOfRegisters> registers_{};
  bus::Bus bus_;
  COP0 cop0_;
  unsigned long long step_count_ = 0;

  void Store(uint32_t address, uint32_t value) const;

  void OpORI(const Instruction& instruction);
  void OpLUI(const Instruction& instruction);
  void OpSW(const Instruction& instruction);
  void OpSLL(const Instruction& instruction);
  void OpADDIU(const Instruction& instruction);
  void OpJ(const Instruction& instruction);
  void OpOR(const Instruction& instruction);
  void OpMTC(const Instruction& instruction);
  void OpBNE(const Instruction& instruction);
};

std::ostream& operator<<(std::ostream& outs, const Instruction& instruction);
}  // namespace cpu

#endif  // POLYSTATION_CPU_H_