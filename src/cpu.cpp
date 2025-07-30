#include "cpu.h"
#include <format>
#include <iostream>
#include <gsl/gsl>

void cpu::CPU::cycle() {
  const uint32_t instruction = load(program_counter_);
  program_counter_ += cpu::INSTRUCTION_LENGTH;

  switch (instruction >> 26U & 0x3FU) {
    case LUI_OPCODE: {
      uint32_t register_index = instruction >> 16U & 0x1FU;
      uint16_t immediate = instruction & 0xFFFFU;

      std::cout << std::format(
          "lui {:02X}, {:04X}",
          instruction, register_index, immediate) << '\n';

      gsl::at(registers_, register_index) = immediate << 16U;
      break;
    }
    default:
      throw std::runtime_error(
          std::format("unhandled instruction {:08X}", instruction));
  }
}

uint32_t cpu::CPU::load(const uint32_t address) const {
  return bus_.load(address);
}