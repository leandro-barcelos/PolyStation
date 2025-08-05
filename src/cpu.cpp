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
    case Instruction::PrimaryOpcode::kCOP0: {
      switch (const bool flag = instruction.GetCoprocessorFlag();
              instruction.GetCoprocessorOpcode(flag)) {
        case Instruction::CoprocessorOpcode::kMTC:
          OpMTC(instruction);
          break;
        default:
          throw std::runtime_error(std::format(
              "unhandled coprocessor opcode {:02X}",
              static_cast<uint8_t>(instruction.GetCoprocessorOpcode(flag))));
      }
      break;
    }
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

cpu::COP0 cpu::CPU::GetCop0() const { return cop0_; }

void cpu::CPU::Store(const uint32_t address, const uint32_t value) const {
  if ((cop0_.status_register & 0x10000) != 0U) {
    std::cout << "ignoring store while cache is isolated" << '\n';
    return;
  }

  bus::Bus::Store(address, value);
}

void cpu::CPU::OpORI(const Instruction& instruction) {
  const uint8_t register_s = instruction.GetRegisterS();
  const uint8_t register_t = instruction.GetRegisterT();
  const uint16_t immediate = instruction.GetImmediate16();

  gsl::at(registers_, register_t) = gsl::at(registers_, register_s) | immediate;
}

void cpu::CPU::OpLUI(const Instruction& instruction) {
  const uint8_t register_t = instruction.GetRegisterT();
  const uint16_t immediate = instruction.GetImmediate16();

  gsl::at(registers_, register_t) = immediate << 16U;
}

void cpu::CPU::OpSW(const Instruction& instruction) {
  const uint8_t register_s = instruction.GetRegisterS();
  const uint8_t register_t = instruction.GetRegisterT();
  const uint32_t immediate = instruction.GetImmediate16SignExtend();

  const uint32_t address = gsl::at(registers_, register_s) + immediate;
  Store(address, gsl::at(registers_, register_t));
}

void cpu::CPU::OpSLL(const Instruction& instruction) {
  const uint8_t register_t = instruction.GetRegisterT();
  const uint32_t register_d = instruction.GetRegisterD();
  const uint16_t immediate = instruction.GetImmediate16();

  gsl::at(registers_, register_d) = gsl::at(registers_, register_t)
                                    << immediate;
}

void cpu::CPU::OpADDIU(const Instruction& instruction) {
  const uint8_t register_t = instruction.GetRegisterT();
  const uint8_t register_s = instruction.GetRegisterS();
  const uint16_t immediate = instruction.GetImmediate16();

  gsl::at(registers_, register_t) = gsl::at(registers_, register_s) + immediate;
}

void cpu::CPU::OpJ(const Instruction& instruction) {
  const uint32_t immediate = instruction.GetImmediate26();

  program_counter_ = (program_counter_ & 0xF0000000) | (immediate << 2U);
}

void cpu::CPU::OpOR(const Instruction& instruction) {
  const uint8_t register_s = instruction.GetRegisterS();
  const uint8_t register_t = instruction.GetRegisterT();
  const uint8_t register_d = instruction.GetRegisterD();

  gsl::at(registers_, register_d) =
      gsl::at(registers_, register_s) | gsl::at(registers_, register_t);
}

void cpu::CPU::OpMTC(const Instruction& instruction) {
  switch (instruction.GetCoprocessor()) {
    case 0: {
      const uint8_t register_t = instruction.GetRegisterT();
      switch (uint8_t register_d = instruction.GetRegisterD()) {
        case COP0::Registers::kStatusRegister:
          cop0_.status_register = gsl::at(registers_, register_t);
          break;
        default:
          throw std::runtime_error(
              std::format("unhandled cop0 register {}", register_d));
      }
      break;
    }
    default:
      throw std::runtime_error(std::format("unhandled coprocessor COP{}",
                                           instruction.GetCoprocessor()));
  }
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

uint8_t cpu::Instruction::GetCoprocessor() const { return data_ >> 26U & 0x3U; }

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

std::ostream& cpu::operator<<(std::ostream& outs,
                              const Instruction& instruction) {
  switch (instruction.GetPrimaryOpcode()) {
    case Instruction::PrimaryOpcode::kSPECIAL:
      switch (instruction.GetSecondaryOpcode()) {
        case Instruction::SecondaryOpcode::kSLL:
          return outs << std::format(
                     "sll R{}, R{}, {:04X}", instruction.GetRegisterD(),
                     instruction.GetRegisterT(), instruction.GetImmediate16());
        case Instruction::SecondaryOpcode::kOR:
          return outs << std::format(
                     "or R{}, R{}, R{}", instruction.GetRegisterD(),
                     instruction.GetRegisterS(), instruction.GetRegisterT());
        default:
          return outs << std::format("0x{:08X}", instruction.GetRawData());
      }
    case Instruction::PrimaryOpcode::kJ:
      return outs << std::format("j {:07X}", instruction.GetImmediate26());
    case Instruction::PrimaryOpcode::kADDIU:
      return outs << std::format(
                 "addiu R{}, R{}, {:04X}", instruction.GetRegisterT(),
                 instruction.GetRegisterS(), instruction.GetImmediate16());
    case Instruction::PrimaryOpcode::kORI:
      return outs << std::format(
                 "ori R{}, R{}, {:04X}", instruction.GetRegisterT(),
                 instruction.GetRegisterS(), instruction.GetImmediate16());
    case Instruction::PrimaryOpcode::kLUI:
      return outs << std::format("lui R{}, {:04X}", instruction.GetRegisterT(),
                                 instruction.GetImmediate16());
    case Instruction::PrimaryOpcode::kCOP0:
      switch (const bool flag = instruction.GetCoprocessorFlag();
              instruction.GetCoprocessorOpcode(flag)) {
        case Instruction::CoprocessorOpcode::kMTC:
          return outs << std::format(
                     "mtc{} R{}, cop{}dat{}", instruction.GetCoprocessor(),
                     instruction.GetRegisterT(), instruction.GetCoprocessor(),
                     instruction.GetRegisterD());
        default:
          return outs << std::format("0x{:08X}", instruction.GetRawData());
      }
    case Instruction::PrimaryOpcode::kSW:
      return outs << std::format("sw R{}, {:04X}(R{})",
                                 instruction.GetRegisterT(),
                                 instruction.GetImmediate16SignExtend(),
                                 instruction.GetRegisterS());
    default:
      return outs << std::format("0x{:08X}", instruction.GetRawData());
  }
}

std::string cpu::Instruction::ToString() const {
  std::ostringstream string_stream;
  string_stream << *this;
  return string_stream.str();
}