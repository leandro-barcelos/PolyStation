#include "cpu.h"

#include <format>
#include <gsl/gsl>
#include <iostream>

void cpu::CPU::Cycle() {
  const Instruction instruction{Load(program_counter_)};
  program_counter_ += kInstructionLength;

  switch (instruction.GetPrimaryOpcode()) {
    case Instruction::PrimaryOpcode::kSPECIAL:
      switch (instruction.GetSecondaryOpcode()) {
        default:
          throw std::runtime_error(std::format(
              "unhandled secondary opcode {:02X}",
              static_cast<uint8_t>(instruction.GetSecondaryOpcode())));
      }
      break;
    case Instruction::PrimaryOpcode::kORI:
      OpORI(instruction);
      break;
    case Instruction::PrimaryOpcode::kLUI:
      OpLUI(instruction);
      break;
    case Instruction::PrimaryOpcode::kSW:
      OpSW(instruction);
      break;
    default:
      throw std::runtime_error(
          std::format("unhandled primary opcode {:02X}",
                      static_cast<uint8_t>(instruction.GetPrimaryOpcode())));
  }
}

uint32_t cpu::CPU::Load(const uint32_t address) const {
  return bus_.Load(address);
}

void cpu::CPU::Store(const uint32_t address, const uint32_t value) {
  bus::Bus::Store(address, value);
}

void cpu::CPU::OpORI(const Instruction& instruction) {
  uint32_t register_s = instruction.GetRegisterS();
  uint32_t register_t = instruction.GetRegisterT();
  uint16_t immediate = instruction.GetImmediate16();

  gsl::at(registers_, register_t) = gsl::at(registers_, register_s) | immediate;

  std::cout << std::format("ori {:02X}, {:02X}, {:04X}", register_t, register_s,
                           immediate)
            << '\n';
}

void cpu::CPU::OpLUI(const Instruction& instruction) {
  uint32_t register_t = instruction.GetRegisterT();
  uint16_t immediate = instruction.GetImmediate16();

  gsl::at(registers_, register_t) = immediate << 16U;

  std::cout << std::format("lui {:02X}, {:04X}", register_t, immediate) << '\n';
}

void cpu::CPU::OpSW(const Instruction& instruction) {
  uint32_t register_s = instruction.GetRegisterS();
  uint32_t register_t = instruction.GetRegisterT();
  uint16_t immediate = instruction.GetImmediate16();

  const uint32_t address = gsl::at(registers_, register_s) + immediate;
  Store(address, gsl::at(registers_, register_t));

  std::cout << std::format("sw {:02X}, {:04X}({:02X})", register_t, immediate,
                           register_s)
            << '\n';
}

cpu::Instruction::PrimaryOpcode cpu::Instruction::GetPrimaryOpcode() const {
  return static_cast<PrimaryOpcode>(data_ >> 26U & 0x3FU);
}

cpu::Instruction::SecondaryOpcode cpu::Instruction::GetSecondaryOpcode() const {
  return static_cast<SecondaryOpcode>(data_ & 0x3FU);
}

uint8_t cpu::Instruction::GetRegisterS() const { return data_ >> 21U & 0x1FU; }

uint8_t cpu::Instruction::GetRegisterT() const { return data_ >> 16U & 0x1FU; }

uint16_t cpu::Instruction::GetImmediate16() const { return data_ & 0xFFFFU; }