# Windows build notes

BackType's Windows target is ready in CMake, but it needs a Windows machine with Visual Studio, CMake, Ninja or MSBuild, and the Adobe After Effects SDK.

Set these paths first:

```powershell
$env:AE_SDK_ROOT = "C:\SDKs\AfterEffectsSDK"
$env:AE_PIPL_TOOL = "C:\SDKs\AfterEffectsSDK\...\cnvtpipl.exe"
```

Then build:

```powershell
cmake -S . -B build\windows -G "Visual Studio 17 2022" -A x64 -DAE_SDK_ROOT="$env:AE_SDK_ROOT" -DAE_PIPL_TOOL="$env:AE_PIPL_TOOL"
cmake --build build\windows --config Release
```

Expected output:

```text
build\windows\Release\BackType.aex
```

Create the Windows zip only after `BackType.aex` exists:

```powershell
$env:WINDOWS_ARTIFACT = "build\windows\Release\BackType.aex"
bash packaging/package_release.sh
```
