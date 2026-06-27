### Prerequisites

- [Visual Studio 2022](https://visualstudio.microsoft.com/) with the **Desktop development with C++** workload
- [vcpkg](doc/Setup/vcpkg_setup.md) installed and `VCPKG_ROOT` set

### Build

> Run all commands below from the **x64 Native Tools Command Prompt for VS 2022** (or any terminal where the MSVC toolchain is on the PATH), from the **project root** (`TradingSimulator\`).

**Configure** (one-liner):
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static -S . -B build

On first run vcpkg will automatically download and build all dependencies (curl, nlohmann-json, gtest). This takes a few minutes once.

> The static triplet (`x64-windows-static`) is required: GTest built as a DLL has a known MSVC issue where `TEST`/`TEST_F` registration silently fails across the DLL boundary, so all dependencies are statically linked.

**Compile:**
cmake --build build --config Debug

**Run:**
.\build\Debug\trading_simulator.exe

**Run tests:**
ctest --test-dir build -C Debug -V

### Reconfigure from scratch

Remove-Item -Recurse -Force build
cmake -G "Visual Studio 17 2022" -A x64 -DCMAKE_TOOLCHAIN_FILE="C:/vcpkg/scripts/buildsystems/vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-static -S . -B build

