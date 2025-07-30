#include <iostream>

#include "bios.h"
#include "bus.h"
#include "cpu.h"

int main(int argc, char* argv[]) {
  if (argc <= 1) {
    std::cout << "no bios file was provided!";
    return EXIT_FAILURE;
  }

  const std::string path = argv[1];

  cpu::CPU cpu{path};

  while (true) {
    cpu.Cycle();
  }
}