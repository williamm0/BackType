# BackType

BackType is an After Effects text effect for edit-style typing animations. It types text forward while the text pushes backward, so the newest character can stay near the same spot.

## What it does

- Renders typewriter-style text
- Moves the text backward while typing
- Supports a blinking cursor
- Lets you control progress, size, color, position, direction, and backward motion
- Shows up under Effect > jx plugins > BackType

## Why I made it

Normal typewriter effects in After Effects are easy enough, but this specific edit-style motion is annoying to rebuild every time. BackType is meant to make that one look fast to set up.

## Install

### Windows

Copy `BackType.aex` into your After Effects plug-ins folder.

Common path:

`C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore`

Restart After Effects.

### macOS

Copy `BackType.plugin` into your After Effects plug-ins folder.

Common path:

`/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore`

Restart After Effects.

This repo does not sign or notarize the macOS plugin. If macOS blocks the unsigned build, remove quarantine after you have decided you trust the file:

```bash
xattr -dr com.apple.quarantine "/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/BackType.plugin"
```

That only removes the local quarantine flag. It is not code signing.

## How to use

1. Create a solid.
2. Apply Effect > jx plugins > BackType.
3. Type your text in the Text field.
4. Keyframe Progress from 0 to 100.
5. Adjust Backward Motion until the typing movement looks right.

## Current state

This is v0.1.0, so treat it as an early build.

Working target:
- Windows `.aex`
- macOS `.plugin`

Working features:
- Basic text rendering
- Progress-based typing
- Backward movement
- Cursor
- Size, color, position, direction, and opacity controls

Still rough:
- The current AE SDK does not have a simple native string effect parameter. For v0.1.0, Text and Cursor Character are fixed placeholders in the UI. Editable text needs an arbitrary-data/custom-UI control next.
- Unicode is handled as UTF-8 for reveal boundaries, but platform font fallback still needs testing
- Font selection is fixed to the platform default path for now
- Pop Amount is present in the UI but rendering support is still a TODO
- 16-bit and 32-bit output paths are sketched in the renderer helpers, but the first AE target is 8-bit
- Windows PiPL generation depends on the PiPL tool that comes with the Adobe SDK
- macOS signing/notarization is not included

## Build

You need the Adobe After Effects C++ SDK. This repo does not vendor SDK headers or Adobe tools.

The effect category is set in `src/BackType_PiPL.r`:

```text
jx plugins
```

### Tests only

The core reveal and anchor math can be built without the AE SDK:

```bash
cmake -S . -B build/tests -DBACKTYPE_BUILD_AE_PLUGIN=OFF
cmake --build build/tests
./build/tests/backtype_text_logic_tests
```

### macOS plugin

Install Command Line Tools, CMake, and Ninja. Point `AE_SDK_ROOT` at either the extracted SDK `Examples` folder or the folder that contains Adobe's extracted SDK package:

```bash
cmake -S . -B build/macos -G Ninja -DAE_SDK_ROOT="/Users/william/SDKs/AfterEffectsSDK"
cmake --build build/macos --config Release
```

The target output is `build/macos/BackType.plugin`. On macOS the default CMake build is universal (`arm64;x86_64`) so it matches the PiPL entries.

The build runs `Rez` on `src/BackType_PiPL.r` and writes the PiPL resource into the plugin bundle. If your SDK keeps `PiPL.r` somewhere unusual, update `AE_SDK_ROOT` or the include paths in `CMakeLists.txt`.

### Windows plugin

Use Visual Studio, CMake, the After Effects SDK, and the SDK PiPL conversion tool:

```powershell
cmake -S . -B build\windows -G "Visual Studio 17 2022" -A x64 `
  -DAE_SDK_ROOT="C:\path\to\AfterEffectsSDK" `
  -DAE_PIPL_TOOL="C:\path\to\AfterEffectsSDK\Resources\PiPLTool\pipltool.exe"
cmake --build build\windows --config Release
```

The target output is `BackType.aex`.

If your SDK uses `cnvtpipl.exe` or already has a generated `.rrc`, set `AE_PIPL_TOOL` to that tool or set `BACKTYPE_PREBUILT_PIPL_RRC` to the generated resource file.

## Release

The release zips should include:

- BackType.aex for Windows
- BackType.plugin for macOS
- README.md
- LICENSE, if included

After both plugin artifacts exist under `build/`, run:

```bash
./packaging/package_release.sh
```

That creates:

- `dist/BackType-v0.1.0-Windows.zip`
- `dist/BackType-v0.1.0-macOS.zip`

## License

No license has been chosen yet.
