### Build

```powershell
# From x64 Native Tools Command Prompt
cd c:\Users\vasil\Documents\projects\TradingSimulator
mkdir build
cd build
cmake ..
cmake --build .
```

cmake -G "Visual Studio 17 2022" -A x64 .. (if needed)