#include "cpu.h"
#include <format>

void cpu::CPU::cycle() {
  const uint32_t instruction = load(program_counter_);
  program_counter_ += cpu::INSTRUCTION_LENGTH;

  switch (instruction) {
    default:
      throw std::runtime_error(
          std::format("unhandled instruction {:08X}", instruction));
  }
}

uint32_t cpu::CPU::load(const uint32_t address) const {
  return bus_.load(address);
}