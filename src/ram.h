#ifndef POLYSTATION_RAM_H
#define POLYSTATION_RAM_H
#include <array>
#include <cstdint>

namespace ram {
constexpr uint32_t kRamBase = 0x00000000;
constexpr uint32_t kRamSize = 0x200000;

class Ram {
 public:
  [[nodiscard]] uint32_t Load(uint32_t offset) const;

  void Store(uint32_t offset, uint32_t value);

 private:
  std::array<std::byte, 0x200000> data_{};
};
}  // namespace ram

#endif  // POLYSTATION_RAM_H
