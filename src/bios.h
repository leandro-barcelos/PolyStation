#ifndef POLYSTATION_BIOS_H
#define POLYSTATION_BIOS_H
#include <filesystem>
#include <vector>

namespace bios {
constexpr uint32_t BIOS_BASE = 0xBFC00000;
constexpr uint64_t BIOS_SIZE = 0x80000;

class Bios {
public:
  explicit Bios(const std::string& path);

  [[nodiscard]] uint32_t load(uint32_t offset) const;

private:
  std::vector<std::byte> data;
};
} // namespace bios

#endif  // POLYSTATION_BIOS_H