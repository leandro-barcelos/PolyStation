#include <iostream>
#include <span>

#include "app.h"
#include "logger.h"

#ifdef _WIN32
#include <windows.h>  // SetProcessDPIAware()
#endif

int main(const int argc, char** argv) {
  logger::Logger::init();

  const std::span args(argv, argc);
  if (args.size() <= 1) {
    LOG_FATAL_CORE("Usage: {} <bios_path>", args[0]);
    return -1;
  }

  LOG_INFO_CORE("PolyStation starting...");

  try {
    std::string const bios_path = args[1];
    app::Application app{bios_path};
    app.Run();
  } catch (const std::exception& e) {
    LOG_FATAL_CORE("{}", e.what());
    return -1;
  }

  logger::Logger::shutdown();

  return 0;
}