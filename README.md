# BackType

BackType is an After Effects effect for edit-style typing animations. It types text forward while the text pushes backward, so the line can stay centered or keep the newest character near one spot.

It shows up under:

`Effect > jx plugins > BackType`

<img width="2500" height="1080" alt="BackType" src="https://github.com/user-attachments/assets/16a92092-1de8-40e7-ac6f-0d628e239394" />


## What it does

- Renders typewriter-style text onto transparency
- Moves the text backward while typing
- Supports a blinking cursor
- Lets you control progress, size, color, position, direction, anchor mode, and opacity
- Uses Center Locked as the default anchor mode

## [Install](https://github.com/williamm0/BackType/releases)

### macOS

Copy `BackType.plugin` into your After Effects plug-ins folder.

Common path:

`/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore`

Restart After Effects.

This build is not signed or notarized. If macOS blocks it because of quarantine, remove the quarantine flag after you decide you trust the file:

```bash
xattr -dr com.apple.quarantine "/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/BackType.plugin"
```

### Windows

Copy `BackType.aex` into your After Effects plug-ins folder.

Common path:

`C:\Program Files\Adobe\Common\Plug-ins\7.0\MediaCore`

Restart After Effects.

## How to use on a solid

1. Create a solid.
2. Apply `Effect > jx plugins > BackType`.
3. Click the `Text` row in Effect Controls and type your text.
4. Use Backspace/Delete to remove characters.
5. Keyframe `Progress` from 0 to 100.

Spaces and basic punctuation are stored in the plugin text data. Empty text renders nothing and should not crash.

## How to use on a text layer

You can apply BackType directly to an After Effects text layer.

For v0.1.1, the reliable mode is still `Text Source > Plugin Text`. The plugin keeps a `Text Source` control with `Plugin Text` and `Layer Text`, but normal effect render callbacks do not give this plugin a clean, safe source-text read path yet. `Layer Text` is present so the UI is ready for that mode later, but it currently falls back to the plugin's editable text.

## Notes

- Center Locked is now the default anchor mode.
- Descender letters such as `y`, `g`, `j`, `p`, and `q` no longer shift the line upward while typing.
- Unicode text is stored as UTF-8, but font fallback and non-ASCII editing still need more real AE testing.
- `Pop Amount` is still a parameter, but the render-side pop animation is not finished yet.

## Windows

There is no official Windows build yet. Windows users currently need to build the plugin themselves from source.

A prebuilt .aex version is planned for a future release.

## Current state

This is v0.1.1, so treat it as an early build.

Working target:

- macOS `.plugin`

Prepared target:

- Windows `.aex`

The Windows CMake setup is in the repo, but the Windows binary has to be built on Windows with the After Effects SDK and PiPL tool available. Do not use a Windows zip unless it actually contains `BackType.aex`.

## License

MIT. See `LICENSE`.
