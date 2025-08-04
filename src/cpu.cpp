#include "cpu.h"

#include <format>
#include <gsl/gsl>
#include <iostream>

void cpu::CPU::Reset() {
  program_counter_ = bus::kBiosBase;
  next_instruction_ = Instruction(0x0);
  registers_.fill(0);
  step_count_ = 0;
}

void cpu::CPU::Cycle() {
  const Instruction instruction = next_instruction_;
  next_instruction_ = Instruction(Load(program_counter_));
  program_counter_ += kInstructionLength;

  switch (instruction.GetPrimaryOpcode()) {
    case Instruction::PrimaryOpcode::kSPECIAL:
      switch (instruction.GetSecondaryOpcode()) {
        case Instruction::SecondaryOpcode::kSLL:
          OpSLL(instruction);
          break;
        case Instruction::SecondaryOpcode::kOR:
          OpOR(instruction);
          break;
        default:
          throw std::runtime_error(std::format(
              "unhandled secondary opcode {:02X}",
              static_cast<uint8_t>(instruction.GetSecondaryOpcode())));
      }
      break;
    case Instruction::PrimaryOpcode::kJ:
      OpJ(instruction);
      break;
    case Instruction::PrimaryOpcode::kADDIU:
      OpADDIU(instruction);
      break;
    case Instruction::PrimaryOpcode::kORI:
      OpORI(instruction);
      break;
    case Instruction::PrimaryOpcode::kLUI:
      OpLUI(instruction);
      break;
    case Instruction::PrimaryOpcode::kCOP0:
      switch (const bool flag = instruction.GetCoprocessorFlag();
              instruction.GetCoprocessorOpcode(flag)) {
        default:
          throw std::runtime_error(std::format(
              "unhandled coprocessor opcode {:02X}",
              static_cast<uint8_t>(instruction.GetSecondaryOpcode())));
      }
      break;
    case Instruction::PrimaryOpcode::kSW:
      OpSW(instruction);
      break;
    default:
      throw std::runtime_error(
          std::format("unhandled primary opcode {:02X}",
                      static_cast<uint8_t>(instruction.GetPrimaryOpcode())));
  }

  step_count_++;
}

uint32_t cpu::CPU::GetRegister(const uint32_t index) const {
  return gsl::at(registers_, index);
}

void cpu::CPU::SetRegister(const uint32_t index, const uint32_t value) {
  gsl::at(registers_, index) = value;
}

unsigned long long cpu::CPU::GetStepCount() const { return step_count_; }

uint32_t cpu::CPU::GetPC() const { return program_counter_; }

uint32_t cpu::CPU::Load(const uint32_t address) const {
  return bus_.Load(address);
}

void cpu::CPU::Store(const uint32_t address, const uint32_t value) {
  bus::Bus::Store(address, value);
}

void cpu::CPU::OpORI(const Instruction& instruction) {
  uint8_t register_s = instruction.GetRegisterS();
  uint8_t register_t = instruction.GetRegisterT();
  uint16_t immediate = instruction.GetImmediate16();

  gsl::at(registers_, register_t) = gsl::at(registers_, register_s) | immediate;

  std::cout << std::format("ori {:02X}, {:02X}, {:04X}", register_t, register_s,
                           immediate)
            << '\n';
}

void cpu::CPU::OpLUI(const Instruction& instruction) {
  uint8_t register_t = instruction.GetRegisterT();
  uint16_t immediate = instruction.GetImmediate16();

  gsl::at(registers_, register_t) = immediate << 16U;

  std::cout << std::format("lui {:02X}, {:04X}", register_t, immediate) << '\n';
}

void cpu::CPU::OpSW(const Instruction& instruction) {
  uint8_t register_s = instruction.GetRegisterS();
  uint8_t register_t = instruction.GetRegisterT();
  uint32_t immediate = instruction.GetImmediate16SignExtend();

  const uint32_t address = gsl::at(registers_, register_s) + immediate;
  Store(address, gsl::at(registers_, register_t));

  std::cout << std::format("sw {:02X}, {:04X}({:02X})", register_t, immediate,
                           register_s)
            << '\n';
}

void cpu::CPU::OpSLL(const Instruction& instruction) {
  uint8_t register_t = instruction.GetRegisterT();
  uint32_t register_d = instruction.GetRegisterD();
  uint16_t immediate = instruction.GetImmediate16();

  gsl::at(registers_, register_d) = gsl::at(registers_, register_t)
                                    << immediate;

  std::cout << std::format("sll {:02X}, {:02X}, {:04X}", register_d, register_t,
                           immediate)
            << '\n';
}

void cpu::CPU::OpADDIU(const Instruction& instruction) {
  uint8_t register_t = instruction.GetRegisterT();
  uint8_t register_s = instruction.GetRegisterS();
  uint16_t immediate = instruction.GetImmediate16();

  gsl::at(registers_, register_t) = gsl::at(registers_, register_s) + immediate;

  std::cout << std::format("addiu {:02X}, {:02X}, {:04X}", register_t,
                           register_s, immediate)
            << '\n';
}

void cpu::CPU::OpJ(const Instruction& instruction) {
  uint32_t immediate = instruction.GetImmediate26();

  program_counter_ = (program_counter_ & 0xF0000000) | (immediate << 2U);

  std::cout << std::format("j {:07X}", immediate) << '\n';
}

void cpu::CPU::OpOR(const Instruction& instruction) {
  uint8_t register_s = instruction.GetRegisterS();
  uint8_t register_t = instruction.GetRegisterT();
  uint8_t register_d = instruction.GetRegisterD();

  gsl::at(registers_, register_d) =
      gsl::at(registers_, register_s) | gsl::at(registers_, register_t);

  std::cout << std::format("or {:02X}, {:02X}, {:02X}", register_d, register_s,
                           register_t)
            << '\n';
}

cpu::Instruction::PrimaryOpcode cpu::Instruction::GetPrimaryOpcode() const {
  return static_cast<PrimaryOpcode>(data_ >> 26U & 0x3FU);
}

cpu::Instruction::SecondaryOpcode cpu::Instruction::GetSecondaryOpcode() const {
  return static_cast<SecondaryOpcode>(data_ & 0x3FU);
}
cpu::Instruction::CoprocessorOpcode cpu::Instruction::GetCoprocessorOpcode(
    const bool flag) const {
  if (flag) {
    return static_cast<CoprocessorOpcode>(data_ & 0x3FU);
  }

  return static_cast<CoprocessorOpcode>(data_ >> 21U & 0x1FU);
}

bool cpu::Instruction::GetCoprocessorFlag() const {
  return (data_ >> 25U & 0x1U) == 1;
}

uint8_t cpu::Instruction::GetRegisterS() const { return data_ >> 21U & 0x1FU; }

uint8_t cpu::Instruction::GetRegisterT() const { return data_ >> 16U & 0x1FU; }

uint8_t cpu::Instruction::GetRegisterD() const { return data_ >> 11U & 0x1FU; }

uint16_t cpu::Instruction::GetImmediate16() const { return data_ & 0xFFFFU; }

uint32_t cpu::Instruction::GetImmediate16SignExtend() const {
  const auto value = static_cast<int16_t>(data_ & 0xFFFFU);
  return static_cast<uint32_t>(value);
}

uint32_t cpu::Instruction::GetImmediate26() const { return data_ & 0x3FFFFFFU; }