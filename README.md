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

The current pre-release is macOS only.

### macOS

Copy `BackType.plugin` into your After Effects plug-ins folder.

Common path:

`/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore`

Restart After Effects.

This build is not signed or notarized. If macOS blocks it because of quarantine, remove the quarantine flag after you decide you trust the file:

```bash
xattr -dr com.apple.quarantine "/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/BackType.plugin"
```

## How to use on a solid

1. Create a solid.
2. Apply `Effect > jx plugins > BackType`.
3. Click `Text > Edit Text...`.
4. Type your text in the dialog and press OK.
5. Keyframe `Progress` from 0 to 100.

Spaces and basic punctuation are stored in the plugin text data. Empty text renders nothing and should not crash.

## How to use on a text layer

You can apply BackType directly to an After Effects text layer.

For v0.1.2-pre.2, the reliable mode is still `Text Source > Plugin Text`. You can apply BackType to a solid or directly to an AE text layer, but it does not read the text layer's Source Text yet. `Layer Text` is present so the UI is ready for that mode later, but it currently falls back to the plugin's editable text.

## Notes

- Center Locked is now the default anchor mode.
- Descender letters such as `y`, `g`, `j`, `p`, and `q` no longer shift the line upward while typing.
- v0.1.2-pre.2 removes the hidden arbitrary text parameter too. Text editing now uses a normal AE button, a small macOS dialog, and per-effect sequence data.
- Unicode text is stored as UTF-8, but font fallback and non-ASCII editing still need more real AE testing.
- `Pop Amount` is still a parameter, but the render-side pop animation is not finished yet.

## Windows

There is no Windows release yet. No `.aex` file is included in the current pre-release.

## Current state

This is v0.1.2-pre.2, so treat it as an early build.

Released target:

- macOS `.plugin`

Windows is not released yet.

## License

MIT. See `LICENSE`.
