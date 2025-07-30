#ifndef POLYSTATION_CPU_H_
#define POLYSTATION_CPU_H_
#include <array>
#include "bios.h"
#include "bus.h"

namespace cpu {
constexpr uint32_t NUMBER_OF_REGISTERS = 32;
constexpr uint32_t INSTRUCTION_LENGTH = 4;
constexpr uint32_t LUI_OPCODE = 0x0F;

class CPU {
public:
  explicit CPU(const std::string& path) : bus_(path) {
  }

  void cycle();

private:
  uint32_t program_counter_ = bios::BIOS_BASE;
  std::array<uint32_t, NUMBER_OF_REGISTERS> registers_{};
  bus::Bus bus_;

  [[nodiscard]] uint32_t load(uint32_t address) const;
};
} // namespace cpu

#endif  // POLYSTATION_CPU_H_