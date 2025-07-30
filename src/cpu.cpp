#include "cpu.h"

#include <format>
#include <gsl/gsl>
#include <iostream>

void cpu::CPU::Cycle() {
  const Instruction instruction{Load(program_counter_)};
  program_counter_ += cpu::kInstructionLength;

  switch (instruction.GetOpcode()) {
    case Instruction::Opcode::kORI: {
      uint32_t register_s = instruction.GetRegisterS();
      uint32_t register_t = instruction.GetRegisterT();
      uint16_t immediate = instruction.GetImmediate16();

      std::cout << std::format("ori {:02X}, {:02X}, {:04X}", register_t,
                               register_s, immediate)
                << '\n';

      gsl::at(registers_, register_t) =
          gsl::at(registers_, register_s) | immediate;

      break;
    }
    case Instruction::Opcode::kLUI: {
      uint32_t register_t = instruction.GetRegisterT();
      uint16_t immediate = instruction.GetImmediate16();

      std::cout << std::format("lui {:02X}, {:04X}", register_t, immediate)
                << '\n';

      gsl::at(registers_, register_t) = immediate << 16U;
      break;
    }
    default:
      throw std::runtime_error(
          std::format("unhandled instruction {:03X}",
                      static_cast<uint8_t>(instruction.GetOpcode())));
  }
}

uint32_t cpu::CPU::Load(const uint32_t address) const {
  return bus_.Load(address);
}

cpu::Instruction::Opcode cpu::Instruction::GetOpcode() const {
  return static_cast<Opcode>(data_ >> 26U & 0x3FU);
}

uint8_t cpu::Instruction::GetRegisterS() const { return data_ >> 21U & 0x1FU; }

uint8_t cpu::Instruction::GetRegisterT() const { return data_ >> 16U & 0x1FU; }

uint16_t cpu::Instruction::GetImmediate16() const { return data_ & 0xFFFFU; }