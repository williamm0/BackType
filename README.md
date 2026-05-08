# BackType

BackType is an After Effects text effect for edit-style typing animations. It types text forward while the text pushes backward, so the newest character can stay near the same spot.

<img width="2500" height="1080" alt="BackType" src="https://github.com/user-attachments/assets/16a92092-1de8-40e7-ac6f-0d628e239394" />


## What it does

- Renders typewriter-style text
- Moves the text backward while typing
- Supports a blinking cursor
- Lets you control progress, size, color, position, direction, and backward motion
- Shows up under Effect > jx plugins > BackType

## Why I made it

Normal typewriter effects in After Effects are easy enough, but this specific edit-style motion is annoying to rebuild every time. BackType is meant to make that one look fast to set up.

## Install

Use the plugin file for your system:

- Windows: `BackType.aex`
- macOS: `BackType.plugin`

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

Bugfix note:

- The current macOS build fixes the outflags mismatch warning and recent text placement/cropping issues in After Effects Beta.

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
- Text and Cursor Character are fixed placeholders in v0.1.0. Editable text is next.
- Unicode is handled as UTF-8 for reveal boundaries, but platform font fallback still needs testing
- Font selection is fixed to the platform default path for now
- Pop Amount is present in the UI but rendering support is still a TODO
- 16-bit and 32-bit output paths are sketched in the renderer helpers, but the first AE target is 8-bit
- macOS signing/notarization is not included

## License

MIT. See `LICENSE`.
