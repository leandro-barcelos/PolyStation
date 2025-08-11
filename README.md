# PolyStation
A PlayStation 1 emulator written in modern C++.

## Status
🔧 **Active Development** - CPU emulation and debugging interface implemented

## About
PolyStation is a PlayStation 1 (PS1/PSX) emulator focused on accuracy and modern development practices. The project aims to faithfully reproduce the behavior of the original Sony PlayStation hardware while providing advanced debugging and development tools.

## Screenshots

![UI](https://github.com/leandro-barcelos/leandro-barcelos/blob/main/PolyStationUI.png)

## Building

### Prerequisites
- C++20 compatible compiler (GCC 10+, Clang 12+, MSVC 2022+)
- CMake 3.20 or higher
- Vulkan SDK
- SDL2 development libraries

### Dependencies
- **SDL2**: Window management and input
- **Vulkan**: Modern graphics API for rendering
- **ImGui**: Immediate mode GUI for debugging interface
- **GSL**: Microsoft Guidelines Support Library

### Build Instructions
```bash
git clone https://github.com/leandro-barcelos/PolyStation.git
cd PolyStation
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Running
```bash
./PolyStation path/to/bios.bin
```

**Note**: You'll need a PlayStation 1 BIOS file to run the emulator. This is not provided and must be obtained legally from your own PlayStation console.

## Usage

### Debug Controls
- **Run/Pause**: Start/stop CPU execution
- **Step One**: Execute single instruction
- **Step to PC**: Run until reaching specific program counter
- **Reset**: Reset CPU to initial state

### Debug Windows
- **CPU State**: View all 32 MIPS registers and COP0 status
- **Disassembler**: Real-time instruction disassembly with current PC highlighting
- **Controls**: Execution control and performance monitoring

## Contributing
This project is currently in early development. Contributions, bug reports, and suggestions are welcome!

## License
*To be determined*

## Acknowledgments
- PlayStation hardware documentation by various reverse engineering efforts
- [No$PSX](https://problemkaputt.de/psx.htm) specifications
- PSX development community resources
