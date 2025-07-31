#ifndef POLYSTATION_BIOS_H
#define POLYSTATION_BIOS_H
#include <filesystem>
#include <vector>

namespace bios {
class Bios {
 public:
  explicit Bios(const std::string& path);

  [[nodiscard]] uint32_t Load(uint32_t offset) const;

 private:
  std::vector<std::byte> data_;
};
}  // namespace bios

#endif  // POLYSTATION_BIOS_H