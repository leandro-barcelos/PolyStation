#ifndef POLYSTATION_CPU_H_
#define POLYSTATION_CPU_H_
#include <array>

#include "bios.h"
#include "bus.h"

namespace cpu {
constexpr uint32_t kNumberOfRegisters = 32;
constexpr uint32_t kInstructionLength = 4;
constexpr uint32_t kReturnAddress = 31;

class Instruction {
 public:
  Instruction() : data_(0) {}
  explicit Instruction(const uint32_t instruction) : data_(instruction) {}

  [[nodiscard]] std::string ToString() const;

  enum class PrimaryOpcode : uint8_t {
    kSPECIAL = 0x00,
    kJ = 0x02,
    kJAL = 0x03,
    kBEQ = 0x04,
    kBNE = 0x05,
    kBLEZ = 0x06,
    kBGTZ = 0x07,
    kADDI = 0x08,
    kADDIU = 0x09,
    kANDI = 0x0C,
    kORI = 0x0D,
    kLUI = 0x0F,
    kCOP0 = 0x10,
    kLB = 0x20,
    kLW = 0x23,
    kSB = 0x28,
    kSH = 0x29,
    kSW = 0x2B
  };

  enum class SecondaryOpcode : uint8_t {
    kSLL = 0x00,
    kJR = 0x08,
    kADD = 0x20,
    kADDU = 0x21,
    kAND = 0x24,
    kOR = 0x25,
    kSLTU = 0x2B
  };

  enum class CoprocessorOpcode : uint8_t { kMFC = 0x00, kMTC = 0x04 };

  [[nodiscard]] uint32_t GetRawData() const { return data_; }
  [[nodiscard]] PrimaryOpcode GetPrimaryOpcode() const;
  [[nodiscard]] SecondaryOpcode GetSecondaryOpcode() const;
  [[nodiscard]] CoprocessorOpcode GetCoprocessorOpcode(bool flag) const;
  [[nodiscard]] uint8_t GetCoprocessor() const;
  [[nodiscard]] bool GetCoprocessorFlag() const;
  [[nodiscard]] uint8_t GetS() const;
  [[nodiscard]] uint8_t GetT() const;
  [[nodiscard]] uint8_t GetD() const;
  [[nodiscard]] uint8_t GetShift() const;
  [[nodiscard]] uint16_t GetImmediate16() const;
  [[nodiscard]] uint32_t GetImmediate16SignExtend() const;
  [[nodiscard]] uint32_t GetImmediate26() const;

 private:
  uint32_t data_;
};

struct COP0 {
  uint32_t status_register = 0;

  enum Registers : uint8_t {
    kBPC = 0x3,
    kBDA = 0x5,
    kTAR = 0x6,
    kDCIC = 0x7,
    kBDAM = 0x9,
    kBPCM = 0xB,
    kStatusRegister = 0xC,
    kCAUSE = 0xD
  };

  [[nodiscard]] bool IsCacheIsolated() const;
};

struct LoadDelaySlots {
  uint32_t index = 0;
  uint32_t value = 0;
} __attribute__((aligned(8)));

class CPU {
 public:
  explicit CPU(const std::string& path) : bus_(path) { read_registers_[0] = 0; }

  void Reset();
  void Cycle();
  [[nodiscard]] uint32_t GetRegister(uint32_t index) const;
  void SetRegister(uint32_t index, uint32_t value);
  [[nodiscard]] unsigned long long GetStepCount() const;
  [[nodiscard]] uint32_t GetPC() const;
  [[nodiscard]] uint32_t GetPrevPC() const;
  [[nodiscard]] COP0 GetCop0() const;

  [[nodiscard]] uint32_t Load32(uint32_t address) const;

 private:
  uint32_t program_counter_ = bios::kBiosBase;
  uint32_t prev_program_counter_ = bios::kBiosBase - 4;
  Instruction next_instruction_{0x0};
  std::array<uint32_t, kNumberOfRegisters> read_registers_{};
  std::array<uint32_t, kNumberOfRegisters> write_registers_ = read_registers_;
  LoadDelaySlots load_delay_slots_{};
  bus::Bus bus_;
  COP0 cop0_;
  unsigned long long step_count_ = 0;

  [[nodiscard]] uint8_t Load8(uint32_t address) const;

  void Store32(uint32_t address, uint32_t value);
  void Store16(uint32_t address, uint16_t value);
  void Store8(uint32_t address, uint8_t value);

  void Branch(uint32_t offset);

  void OpSPECIAL(const Instruction& instruction);
  void OpSLL(const Instruction& instruction);
  void OpJR(const Instruction& instruction);
  void OpADD(const Instruction& instruction);
  void OpADDU(const Instruction& instruction);
  void OpAND(const Instruction& instruction);
  void OpOR(const Instruction& instruction);
  void OpSLTU(const Instruction& instruction);
  void OpJ(const Instruction& instruction);
  void OpJAL(const Instruction& instruction);
  void OpBEQ(const Instruction& instruction);
  void OpBNE(const Instruction& instruction);
  void OpBLEZ(const Instruction& instruction);
  void OpBGTZ(const Instruction& instruction);
  void OpADDI(const Instruction& instruction);
  void OpADDIU(const Instruction& instruction);
  void OpANDI(const Instruction& instruction);
  void OpORI(const Instruction& instruction);
  void OpLUI(const Instruction& instruction);
  void OpCOP0(const Instruction& instruction);
  void OpMFC0(const Instruction& instruction);
  void OpMTC0(const Instruction& instruction);
  void OpLB(const Instruction& instruction);
  void OpLW(const Instruction& instruction);
  void OpSB(const Instruction& instruction);
  void OpSH(const Instruction& instruction);
  void OpSW(const Instruction& instruction);
};

std::ostream& operator<<(std::ostream& outs, const Instruction& instruction);

template <typename T>
bool CheckedSum(T value_a, T value_b, T* result);
}  // namespace cpu

#endif  // POLYSTATION_CPU_H_