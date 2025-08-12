#ifndef POLYSTATION_RAM_H
#define POLYSTATION_RAM_H
#include <array>
#include <cstdint>

namespace ram {
class Ram {
 public:
  [[nodiscard]] uint32_t Load(uint32_t offset) const;

  void Store(uint32_t offset, uint32_t value);

 private:
  std::array<std::byte, 0x200000> data_{};
};
}  // namespace ram

#endif  // POLYSTATION_RAM_H
