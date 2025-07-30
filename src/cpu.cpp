#include "cpu.h"

#include <format>
#include <gsl/gsl>
#include <iostream>

void cpu::CPU::Cycle() {
  const Instruction instruction{Load(program_counter_)};
  program_counter_ += cpu::kInstructionLength;

  switch (instruction.Opcode()) {
    case kOriOpcode: {
      uint32_t register_s = instruction.RegisterS();
      uint32_t register_t = instruction.RegisterT();
      uint16_t immediate = instruction.Immediate16();

      std::cout << std::format("ori {:02X}, {:02X}, {:04X}", register_t,
                               register_s, immediate)
                << '\n';

      gsl::at(registers_, register_t) =
          gsl::at(registers_, register_s) | immediate;

      break;
    }
    case kLuiOpcode: {
      uint32_t register_t = instruction.RegisterT();
      uint16_t immediate = instruction.Immediate16();

      std::cout << std::format("lui {:02X}, {:04X}", register_t, immediate)
                << '\n';

      gsl::at(registers_, register_t) = immediate << 16U;
      break;
    }
    default:
      throw std::runtime_error(
          std::format("unhandled instruction {:03X}", instruction.Opcode()));
  }
}

uint32_t cpu::CPU::Load(const uint32_t address) const {
  return bus_.Load(address);
}

uint8_t cpu::Instruction::Opcode() const { return data_ >> 26U & 0x3FU; }

uint8_t cpu::Instruction::RegisterS() const { return data_ >> 21U & 0x1FU; }

uint8_t cpu::Instruction::RegisterT() const { return data_ >> 16U & 0x1FU; }

uint16_t cpu::Instruction::Immediate16() const { return data_ & 0xFFFFU; }