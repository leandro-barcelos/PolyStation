#include <iostream>
#include <span>

#include "app.h"

#ifdef _WIN32
#include <windows.h>  // SetProcessDPIAware()
#endif

int main(const int argc, char** argv) {
  const std::span args(argv, argc);
  if (args.size() <= 1) {
    std::cerr << "Usage: " << args[0] << " <bios_path>" << '\n';
  }

  try {
    std::string const bios_path = args[1];
    app::Application app{bios_path};
    app.Run();
  } catch (const std::exception& e) {
    std::cerr << "Application error: " << e.what() << '\n';
    return -1;
  }

  return 0;
}