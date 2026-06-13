# vcpkg Setup

Run these in PowerShell (installs vcpkg to `C:\vcpkg`, which is the conventional location):

```powershell
# 1. Clone
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg

# 2. Bootstrap (builds the vcpkg executable)
C:\vcpkg\bootstrap-vcpkg.bat

# 3. Add to PATH permanently so every terminal can find it
[Environment]::SetEnvironmentVariable("PATH", $env:PATH + ";C:\vcpkg", "User")

# 4. Set VCPKG_ROOT — CMake and other tools use this
[Environment]::SetEnvironmentVariable("VCPKG_ROOT", "C:\vcpkg", "User")
```

After running those, **close and reopen your terminal** so the environment variables take effect, then verify:

```powershell
vcpkg --version
```

You should see something like `vcpkg package management program version 2024-xx-xx-...`.
