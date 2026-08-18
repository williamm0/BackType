# Windows build notes

BackType's Windows target supports x64 and ARM64. It requires Windows, Visual Studio 2022, CMake, and the Adobe After Effects SDK.

Set these paths first:

```powershell
$env:AE_SDK_ROOT = "C:\SDKs\AfterEffectsSDK"
```

Then build:

```powershell
cmake -S . -B build\windows -G "Visual Studio 17 2022" -A x64 -DBACKTYPE_BUILD_AE_PLUGIN=ON -DAE_SDK_ROOT="$env:AE_SDK_ROOT"
cmake --build build\windows --config Release
```

Expected output:

```text
build\windows\Release\BackType.aex
```

The build preprocesses `BackType_PiPL.r` with MSVC, runs the SDK-provided `PiPLtool.exe`, and embeds the result in `BackType.aex`. Override `AE_PIPL_TOOL` only if the tool is outside `Examples\Resources`.

For native Windows on ARM, configure with `-A ARM64`.

Create the Windows archive after `BackType.aex` exists:

```powershell
cmake --build build\windows --config Release --target package
```
