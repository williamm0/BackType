# BackType

BackType is an After Effects effect for backward-moving typewriter animations. It preserves the rendered layer's font, shaping, color, alpha, and styling, then reveals complete raster groups over time.

The effect appears under `Effect > jx plugins > BackType`.

<img width="2500" height="1080" alt="BackType" src="https://github.com/user-attachments/assets/16a92092-1de8-40e7-ac6f-0d628e239394" />

## Features

- Character and word reveal modes based on rendered alpha boundaries
- First, newest, center, and last-character anchor modes
- Left, right, up, and down reveal directions
- Line, block, and underscore cursors
- Optional newest-character fade
- Output-buffer padding for pushed text and cursor edges
- Native 8-bpc and 16-bpc processing with premultiplied-alpha compositing
- After Effects Multi-Frame Rendering support
- Universal Apple Silicon and Intel macOS builds
- Windows x64 and ARM64 build configuration

BackType does not query Source Text through AEGP APIs during rendering. The render depends only on tracked effect parameters and the checked-out input frame, which avoids stale AE frame-cache results and permits concurrent frame rendering. Raster-derived boundaries also prevent the approximate font-width cutoffs that previously sliced letters.

## Requirements

- CMake 3.21 or newer
- A C++17 compiler
- Adobe After Effects C++ SDK, March 2021 or newer for Multi-Frame Rendering support
- macOS: Xcode Command Line Tools, including `Rez`
- Windows: Visual Studio 2022 and the SDK-provided `PiPLtool.exe`

The plugin currently compiles against the After Effects 25.6 SDK. After Effects 2022 or newer is the supported host range because that is the first public release with Multi-Frame Rendering.

## Build and test

A clean default configuration builds the platform-neutral library and tests without requiring the proprietary Adobe SDK:

```bash
cmake -S . -B build/tests -DBACKTYPE_WARNINGS_AS_ERRORS=ON
cmake --build build/tests --parallel
ctest --test-dir build/tests --output-on-failure
```

Build the macOS plugin:

```bash
cmake -S . -B build/macos \
  -DBACKTYPE_BUILD_AE_PLUGIN=ON \
  -DAE_SDK_ROOT=/path/to/AfterEffectsSDK \
  -DCMAKE_BUILD_TYPE=Release
cmake --build build/macos --parallel
```

The default macOS artifact is a universal `BackType.plugin` containing arm64 and x86_64 slices.

Build the Windows plugin from a Visual Studio developer shell:

```powershell
cmake -S . -B build\windows -G "Visual Studio 17 2022" -A x64 `
  -DBACKTYPE_BUILD_AE_PLUGIN=ON `
  -DAE_SDK_ROOT="C:\SDKs\AfterEffectsSDK"
cmake --build build\windows --config Release --parallel
```

Use `-A ARM64` for native Windows on ARM. CMake discovers `PiPLtool.exe` under the SDK's `Examples/Resources` directory. `AE_PIPL_TOOL` can override that path, and `BACKTYPE_PREBUILT_PIPL_RRC` remains available for controlled build environments.

Create a platform archive with CPack:

```bash
cmake --build build/macos --target package
```

## Sanitizers

AddressSanitizer and UndefinedBehaviorSanitizer:

```bash
cmake -S . -B build/sanitize \
  -DBACKTYPE_ENABLE_ASAN=ON \
  -DBACKTYPE_ENABLE_UBSAN=ON \
  -DBACKTYPE_WARNINGS_AS_ERRORS=ON
cmake --build build/sanitize --parallel
ctest --test-dir build/sanitize --output-on-failure
```

ThreadSanitizer must use a separate build:

```bash
cmake -S . -B build/tsan \
  -DBACKTYPE_ENABLE_TSAN=ON \
  -DBACKTYPE_WARNINGS_AS_ERRORS=ON
cmake --build build/tsan --parallel
ctest --test-dir build/tsan --output-on-failure
```

## Download and install

Use the latest published build from [Releases](../../releases) when available. For a local build, copy the plugin from the build directory to an After Effects plug-ins directory, then restart After Effects.

Common macOS location:

```text
/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore
```

Common Windows location:

```text
C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore
```

Production distribution still requires the publisher's signing credentials and notarization on macOS. Those credentials are intentionally not stored in this repository.

## Controls

- `Progress`: reveals raster groups from 0 to 100.
- `Position X` and `Position Y`: place the effect anchor as percentages of the input frame.
- `Anchor Mode`: controls which part of the revealed content stays attached to the anchor.
- `Backward Motion`: controls displacement strength.
- `Direction`: selects the active reveal edge and movement axis.
- `Character Reveal Mode`: selects character-like raster groups or groups separated by word-sized gaps.
- `Cursor Enabled`, `Cursor Style`, `Cursor Blink Speed`, and `Cursor Offset`: configure the active cursor.
- `Character Fade-In`: fades the newest revealed raster group.
- `Render Padding`: expands the effect output on every side, adjusted for preview downsampling.
- `Opacity`: scales the premultiplied output opacity.

## Rendering notes

- Glyph boundaries come from the rendered alpha image, so ligatures and touching glyphs remain intact rather than being cut at an estimated advance width.
- Word detection uses visible spacing because effects cannot safely query text-document state from MFR render threads. Highly tracked text or decorative layouts can therefore group differently from linguistic words.
- Multi-line text reveals along the selected visual axis.
- Empty or fully transparent input produces transparent output.
- Non-text alpha layers are supported because the renderer operates on the checked-out raster rather than private text-layer state.
- 32-bpc projects use After Effects' host conversion because this classic effect natively advertises 8-bpc and 16-bpc processing. Native 32-bpc float would require a separate SmartFX render path.

## License

MIT. See `LICENSE`.
