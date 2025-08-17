#include "cpu.h"

#include <format>
#include <gsl/gsl>
#include <iostream>

bool cpu::COP0::IsCacheIsolated() const {
  return (status_register & 0x10000) != 0U;
}

void cpu::CPU::Reset() {
  program_counter_ = bios::kBiosBase;
  next_instruction_ = Instruction(0x0);
  read_registers_.fill(0);
  step_count_ = 0;
}

void cpu::CPU::Cycle() {
  const Instruction instruction = next_instruction_;
  next_instruction_ = Instruction(Load32(program_counter_));
  prev_program_counter_ = program_counter_;
  program_counter_ += kInstructionLength;

  auto [index, value] = load_delay_slots_;
  SetRegister(index, value);
  load_delay_slots_ = LoadDelaySlots();

  switch (instruction.GetPrimaryOpcode()) {
    case Instruction::PrimaryOpcode::kSPECIAL:
      OpSPECIAL(instruction);
      break;
    case Instruction::PrimaryOpcode::kJ:
      OpJ(instruction);
      break;
    case Instruction::PrimaryOpcode::kJAL:
      OpJAL(instruction);
      break;
    case Instruction::PrimaryOpcode::kBEQ:
      OpBEQ(instruction);
      break;
    case Instruction::PrimaryOpcode::kBNE:
      OpBNE(instruction);
      break;
    case Instruction::PrimaryOpcode::kADDI:
      OpADDI(instruction);
      break;
    case Instruction::PrimaryOpcode::kADDIU:
      OpADDIU(instruction);
      break;
    case Instruction::PrimaryOpcode::kANDI:
      OpANDI(instruction);
      break;
    case Instruction::PrimaryOpcode::kORI:
      OpORI(instruction);
      break;
    case Instruction::PrimaryOpcode::kLUI:
      OpLUI(instruction);
      break;
    case Instruction::PrimaryOpcode::kCOP0:
      OpCOP0(instruction);
      break;
    case Instruction::PrimaryOpcode::kLB:
      OpLB(instruction);
      break;
    case Instruction::PrimaryOpcode::kLW:
      OpLW(instruction);
      break;
    case Instruction::PrimaryOpcode::kSB:
      OpSB(instruction);
      break;
    case Instruction::PrimaryOpcode::kSH:
      OpSH(instruction);
      break;
    case Instruction::PrimaryOpcode::kSW:
      OpSW(instruction);
      break;
    default:
      throw std::runtime_error(
          std::format("unhandled primary opcode {:02X}",
                      static_cast<uint8_t>(instruction.GetPrimaryOpcode())));
  }

  read_registers_ = write_registers_;

  step_count_++;
}

uint32_t cpu::CPU::GetRegister(const uint32_t index) const {
  return gsl::at(read_registers_, index);
}

void cpu::CPU::SetRegister(const uint32_t index, const uint32_t value) {
  gsl::at(write_registers_, 0) = 0;

  if (index == 0 && value != 0) {
    return;
  }

  gsl::at(write_registers_, index) = value;
}

unsigned long long cpu::CPU::GetStepCount() const { return step_count_; }

uint32_t cpu::CPU::GetPC() const { return program_counter_; }

uint32_t cpu::CPU::GetPrevPC() const { return prev_program_counter_; }

cpu::COP0 cpu::CPU::GetCop0() const { return cop0_; }

uint32_t cpu::CPU::Load32(const uint32_t address) const {
  return bus_.Load32(address);
}

uint8_t cpu::CPU::Load8(const uint32_t address) const {
  return bus_.Load8(address);
}

void cpu::CPU::Store32(const uint32_t address, const uint32_t value) {
  if (cop0_.IsCacheIsolated()) {
    std::cout << "ignoring store while cache is isolated" << '\n';
    return;
  }

  bus_.Store32(address, value);
}

void cpu::CPU::Store16(const uint32_t address, const uint16_t value) {
  if (cop0_.IsCacheIsolated()) {
    std::cout << "ignoring store while cache is isolated" << '\n';
    return;
  }

  bus_.Store16(address, value);
}

void cpu::CPU::Store8(const uint32_t address, const uint8_t value) {
  if (cop0_.IsCacheIsolated()) {
    std::cout << "ignoring store while cache is isolated" << '\n';
    return;
  }

  bus_.Store8(address, value);
}

void cpu::CPU::Branch(uint32_t offset) {
  offset <<= 2U;

  program_counter_ += offset;
  program_counter_ -= 4;
}

void cpu::CPU::OpSPECIAL(const Instruction& instruction) {
  switch (instruction.GetSecondaryOpcode()) {
    case Instruction::SecondaryOpcode::kSLL:
      OpSLL(instruction);
      break;
    case Instruction::SecondaryOpcode::kJR:
      OpJR(instruction);
      break;
    case Instruction::SecondaryOpcode::kADD:
      OpADD(instruction);
      break;
    case Instruction::SecondaryOpcode::kADDU:
      OpADDU(instruction);
      break;
    case Instruction::SecondaryOpcode::kAND:
      OpAND(instruction);
      break;
    case Instruction::SecondaryOpcode::kOR:
      OpOR(instruction);
      break;
    case Instruction::SecondaryOpcode::kSLTU:
      OpSLTU(instruction);
      break;
    default:
      throw std::runtime_error(
          std::format("unhandled secondary opcode {:02X}",
                      static_cast<uint8_t>(instruction.GetSecondaryOpcode())));
  }
}

void cpu::CPU::OpSLL(const Instruction& instruction) {
  const uint32_t register_t = GetRegister(instruction.GetT());
  const uint16_t immediate = instruction.GetShift();

  const uint32_t result = register_t << immediate;
  SetRegister(instruction.GetD(), result);
}

void cpu::CPU::OpJR(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());

  program_counter_ = register_s;
}

void cpu::CPU::OpADD(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t register_t = GetRegister(instruction.GetT());

  int32_t result = 0;
  if (CheckedSum<int32_t>(static_cast<int32_t>(register_s),
                          static_cast<int32_t>(register_t), &result)) {
    throw std::runtime_error("overflow in ADD");
  }

  SetRegister(instruction.GetD(), static_cast<uint32_t>(result));
}

void cpu::CPU::OpADDU(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t register_t = GetRegister(instruction.GetT());

  const uint32_t result = register_s + register_t;
  SetRegister(instruction.GetD(), result);
}

void cpu::CPU::OpAND(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t register_t = GetRegister(instruction.GetT());

  const uint32_t result = register_s & register_t;
  SetRegister(instruction.GetD(), result);
}

void cpu::CPU::OpOR(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t register_t = GetRegister(instruction.GetT());

  const uint32_t result = register_s | register_t;
  SetRegister(instruction.GetD(), result);
}

void cpu::CPU::OpSLTU(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t register_t = GetRegister(instruction.GetT());

  SetRegister(instruction.GetD(), register_s < register_t ? 1 : 0);
}

void cpu::CPU::OpJ(const Instruction& instruction) {
  const uint32_t immediate = instruction.GetImmediate26();

  program_counter_ = (program_counter_ & 0xF0000000) | (immediate << 2U);
}

void cpu::CPU::OpJAL(const Instruction& instruction) {
  SetRegister(kReturnAddress, GetPC());

  OpJ(instruction);
}

void cpu::CPU::OpBEQ(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t register_t = GetRegister(instruction.GetT());
  const uint32_t immediate = instruction.GetImmediate16SignExtend();

  if (register_s == register_t) {
    Branch(immediate);
  }
}

void cpu::CPU::OpBNE(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t register_t = GetRegister(instruction.GetT());
  const uint32_t immediate = instruction.GetImmediate16SignExtend();

  if (register_s != register_t) {
    Branch(immediate);
  }
}

void cpu::CPU::OpADDI(const Instruction& instruction) {
  const auto register_s = static_cast<int32_t>(GetRegister(instruction.GetS()));
  const auto immediate =
      static_cast<int32_t>(instruction.GetImmediate16SignExtend());

  int32_t sum = 0;
  if (CheckedSum<int32_t>(register_s, immediate, &sum)) {
    throw std::runtime_error("overflow in ADDI");
  }

  SetRegister(instruction.GetT(), static_cast<uint32_t>(sum));
}

void cpu::CPU::OpADDIU(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t immediate = instruction.GetImmediate16SignExtend();

  const uint32_t result = register_s + immediate;
  SetRegister(instruction.GetT(), result);
}

void cpu::CPU::OpANDI(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t immediate = instruction.GetImmediate16();

  const uint32_t result = register_s & immediate;
  SetRegister(instruction.GetT(), result);
}

void cpu::CPU::OpORI(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint16_t immediate = instruction.GetImmediate16();

  const uint32_t result = register_s | immediate;
  SetRegister(instruction.GetT(), result);
}

void cpu::CPU::OpLUI(const Instruction& instruction) {
  const uint16_t immediate = instruction.GetImmediate16();

  const uint32_t result = immediate << 16U;
  SetRegister(instruction.GetT(), result);
}

void cpu::CPU::OpCOP0(const Instruction& instruction) {
  switch (const bool flag = instruction.GetCoprocessorFlag();
          instruction.GetCoprocessorOpcode(flag)) {
    case Instruction::CoprocessorOpcode::kMFC:
      OpMFC0(instruction);
      break;
    case Instruction::CoprocessorOpcode::kMTC:
      OpMTC0(instruction);
      break;
    default:
      throw std::runtime_error(std::format(
          "unhandled coprocessor opcode {:02X}",
          static_cast<uint8_t>(instruction.GetCoprocessorOpcode(flag))));
  }
}

void cpu::CPU::OpMFC0(const Instruction& instruction) {
  switch (instruction.GetD()) {
    case COP0::Registers::kStatusRegister:
      SetRegister(instruction.GetT(), cop0_.status_register);
      break;
    default:
      throw std::runtime_error(
          std::format("unhandled cop0 register {}", instruction.GetD()));
  }
}

void cpu::CPU::OpMTC0(const Instruction& instruction) {
  const uint32_t register_t = GetRegister(instruction.GetT());

  switch (instruction.GetD()) {
    case COP0::Registers::kStatusRegister:
      cop0_.status_register = register_t;
      break;
    case COP0::Registers::kBPC:
    case COP0::Registers::kBDA:
    case COP0::Registers::kTAR:
    case COP0::Registers::kDCIC:
    case COP0::Registers::kBDAM:
    case COP0::Registers::kBPCM:
    case COP0::Registers::kCAUSE:
      std::cout << "ignoring write to debug COP0 register" << '\n';
      break;
    default:
      throw std::runtime_error(
          std::format("unhandled cop0 register {}", instruction.GetD()));
  }
}

void cpu::CPU::OpLB(const Instruction& instruction) {
  if (cop0_.IsCacheIsolated()) {
    std::cout << "ignoring load while cache is isolated" << '\n';
    return;
  }

  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t immediate = instruction.GetImmediate16SignExtend();

  const uint32_t address = register_s + immediate;
  const uint8_t value = bus_.Load8(address);

  load_delay_slots_ = LoadDelaySlots(instruction.GetT(), value);
}

void cpu::CPU::OpLW(const Instruction& instruction) {
  if (cop0_.IsCacheIsolated()) {
    std::cout << "ignoring load while cache is isolated" << '\n';
    return;
  }

  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t immediate = instruction.GetImmediate16SignExtend();

  const uint32_t address = register_s + immediate;
  const uint32_t value = bus_.Load32(address);

  load_delay_slots_ = LoadDelaySlots(instruction.GetT(), value);
}

void cpu::CPU::OpSB(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t register_t = GetRegister(instruction.GetT());
  const uint32_t immediate = instruction.GetImmediate16SignExtend();

  const uint32_t address = register_s + immediate;
  Store8(address, static_cast<uint8_t>(register_t & 0xFF));
}

void cpu::CPU::OpSH(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t register_t = GetRegister(instruction.GetT());
  const uint32_t immediate = instruction.GetImmediate16SignExtend();

  const uint32_t address = register_s + immediate;
  Store16(address, static_cast<uint16_t>(register_t & 0xFFFF));
}

void cpu::CPU::OpSW(const Instruction& instruction) {
  const uint32_t register_s = GetRegister(instruction.GetS());
  const uint32_t register_t = GetRegister(instruction.GetT());
  const uint32_t immediate = instruction.GetImmediate16SignExtend();

  const uint32_t address = register_s + immediate;
  Store32(address, register_t);
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

uint8_t cpu::Instruction::GetS() const { return data_ >> 21U & 0x1FU; }

uint8_t cpu::Instruction::GetT() const { return data_ >> 16U & 0x1FU; }

uint8_t cpu::Instruction::GetD() const { return data_ >> 11U & 0x1FU; }

uint8_t cpu::Instruction::GetShift() const { return (data_ >> 6U) & 0x1FU; }

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
          return outs << std::format("sll R{}, R{}, {:04X}", instruction.GetD(),
                                     instruction.GetT(),
                                     instruction.GetImmediate16());
        case Instruction::SecondaryOpcode::kJR:
          return outs << std::format("jr R{}", instruction.GetS());
        case Instruction::SecondaryOpcode::kADD:
          return outs << std::format("add R{}, R{}, R{}", instruction.GetD(),
                                     instruction.GetS(), instruction.GetT());
        case Instruction::SecondaryOpcode::kADDU:
          return outs << std::format("addu R{}, R{}, R{}", instruction.GetD(),
                                     instruction.GetS(), instruction.GetT());
        case Instruction::SecondaryOpcode::kAND:
          return outs << std::format("and R{}, R{}, R{}", instruction.GetD(),
                                     instruction.GetS(), instruction.GetT());
        case Instruction::SecondaryOpcode::kOR:
          return outs << std::format("or R{}, R{}, R{}", instruction.GetD(),
                                     instruction.GetS(), instruction.GetT());
        case Instruction::SecondaryOpcode::kSLTU:
          return outs << std::format("sltu R{}, R{}, R{}", instruction.GetD(),
                                     instruction.GetS(), instruction.GetT());
        default:
          return outs << std::format("0x{:08X}", instruction.GetRawData());
      }
    case Instruction::PrimaryOpcode::kJ:
      return outs << std::format("j {:07X}", instruction.GetImmediate26());
    case Instruction::PrimaryOpcode::kJAL:
      return outs << std::format("jal {:07X}", instruction.GetImmediate26());
    case Instruction::PrimaryOpcode::kBEQ:
      return outs << std::format(
                 "beq R{}, R{}, {}", instruction.GetS(), instruction.GetT(),
                 static_cast<int32_t>(instruction.GetImmediate16SignExtend()));
    case Instruction::PrimaryOpcode::kBNE:
      return outs << std::format(
                 "bne R{}, R{}, {}", instruction.GetS(), instruction.GetT(),
                 static_cast<int32_t>(instruction.GetImmediate16SignExtend()));
    case Instruction::PrimaryOpcode::kADDI:
      return outs << std::format(
                 "addi R{}, R{}, {}", instruction.GetT(), instruction.GetS(),
                 static_cast<int32_t>(instruction.GetImmediate16SignExtend()));
    case Instruction::PrimaryOpcode::kADDIU:
      return outs << std::format("addiu R{}, R{}, {:04X}", instruction.GetT(),
                                 instruction.GetS(),
                                 instruction.GetImmediate16());
    case Instruction::PrimaryOpcode::kANDI:
      return outs << std::format("andi R{}, R{}, {:04X}", instruction.GetT(),
                                 instruction.GetS(),
                                 instruction.GetImmediate16());
    case Instruction::PrimaryOpcode::kORI:
      return outs << std::format("ori R{}, R{}, {:04X}", instruction.GetT(),
                                 instruction.GetS(),
                                 instruction.GetImmediate16());
    case Instruction::PrimaryOpcode::kLUI:
      return outs << std::format("lui R{}, {:04X}", instruction.GetT(),
                                 instruction.GetImmediate16());
    case Instruction::PrimaryOpcode::kCOP0:
      switch (const bool flag = instruction.GetCoprocessorFlag();
              instruction.GetCoprocessorOpcode(flag)) {
        case Instruction::CoprocessorOpcode::kMFC:
          return outs << std::format(
                     "mfc{} R{}, cop{}dat{}", instruction.GetCoprocessor(),
                     instruction.GetT(), instruction.GetCoprocessor(),
                     instruction.GetD());
        case Instruction::CoprocessorOpcode::kMTC:
          return outs << std::format(
                     "mtc{} R{}, cop{}dat{}", instruction.GetCoprocessor(),
                     instruction.GetT(), instruction.GetCoprocessor(),
                     instruction.GetD());
        default:
          return outs << std::format("0x{:08X}", instruction.GetRawData());
      }
    case Instruction::PrimaryOpcode::kLB:
      return outs << std::format("lb R{}, {:04X}(R{})", instruction.GetT(),
                                 instruction.GetImmediate16SignExtend(),
                                 instruction.GetS());
    case Instruction::PrimaryOpcode::kLW:
      return outs << std::format("lw R{}, {:04X}(R{})", instruction.GetT(),
                                 instruction.GetImmediate16SignExtend(),
                                 instruction.GetS());
    case Instruction::PrimaryOpcode::kSB:
      return outs << std::format("sb R{}, {:04X}(R{})", instruction.GetT(),
                                 instruction.GetImmediate16SignExtend(),
                                 instruction.GetS());
    case Instruction::PrimaryOpcode::kSH:
      return outs << std::format("sh R{}, {:04X}(R{})", instruction.GetT(),
                                 instruction.GetImmediate16SignExtend(),
                                 instruction.GetS());
    case Instruction::PrimaryOpcode::kSW:
      return outs << std::format("sw R{}, {:04X}(R{})", instruction.GetT(),
                                 instruction.GetImmediate16SignExtend(),
                                 instruction.GetS());
    default:
      return outs << std::format("0x{:08X}", instruction.GetRawData());
  }
}

template <typename T>
bool cpu::CheckedSum(T value_a, T value_b, T* result) {
  *result = value_a + value_b;

  return (value_b > 0 && value_a > std::numeric_limits<T>::max() - value_b) ||
         (value_b < 0 && value_a < std::numeric_limits<T>::min() - value_b);
}

std::string cpu::Instruction::ToString() const {
  std::ostringstream string_stream;
  string_stream << *this;
  return string_stream.str();
}