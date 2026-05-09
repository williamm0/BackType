# BackType

BackType is an after effects effect for text-layer typing animations. It reveals the layer's real Source Text over time while pushing the rendered text backward, so the line can stay centered or keep the active character near one spot.

It shows up under:

`Effect > jx plugins > BackType`

<img width="2500" height="1080" alt="BackType" src="https://github.com/user-attachments/assets/16a92092-1de8-40e7-ac6f-0d628e239394" />

## Current state

Current build: **v0.1.3-pre.2**

This is a macOS pre-release. Treat it as early software that still needs more real ae project testing. Windows is not released yet.

## What it does

- Uses the actual after effects text layer as the source
- Keeps the layer's rendered font, size, color, and styling
- Reveals characters or words with `Progress`
- Moves the text backward while typing
- Supports line, block, and underscore cursor styles
- Uses Center Locked as the default anchor mode
- Includes Render Padding for letters or cursors that get clipped
- Keeps descender letters like `y`, `g`, `j`, `p`, and `q` from making the line jump vertically

## Download

Use the latest build from [Releases](../../releases) when available.

The current pre-release is macOS only. There is no Windows `.aex` release yet.

## Install

### macOS

Copy `BackType.plugin` into your after effects plug-ins folder.

Common path:

`/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore`

Restart after effects.

This build is not signed or notarized. If macOS blocks it because of quarantine, remove the quarantine flag after you decide you trust the file:

```bash
xattr -dr com.apple.quarantine "/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/BackType.plugin"
```

## How to use

1. Create an after effects text layer.
2. Type and style the text normally.
3. Apply `Effect > jx plugins > BackType` to that text layer.
4. Keyframe `Progress` from 0 to 100.
5. Adjust `Anchor Mode`, `Backward Motion`, `Direction`, and `Position` as needed.

BackType does not have its own text field anymore. Edit the layer's Source Text like you normally would.

## Controls

- `Progress`: reveals the text from 0 to 100.
- `Anchor Mode`: controls where the text stays locked while it reveals.
- `Backward Motion`: controls how strongly the text pushes backward.
- `Direction`: changes the reveal edge and backward push direction.
- `Character Reveal Mode`: switches between character and word reveal.
- `Cursor Style`: line, block, or underscore.
- `Character Fade-In`: fades the newest revealed character without changing layout.
- `Render Padding`: expands the render-safe area if letters or the cursor get clipped at the edge.
- `Opacity`: fades the rendered result.

## Known limits

- Character reveal uses cached approximate advance widths, so unusual fonts, ligatures, RTL text, and complex multi-line layouts may need more testing.
- Applying BackType to a solid renders nothing because there is no text layer Source Text to read.
- Cursor blinking does not affect layout.
- Empty text renders nothing.
- Windows is planned, but no `.aex` build is included in this pre-release.

## Roadmap

- More ae runtime testing across fonts and project types
- Windows `.aex` build
- Better multi-line behavior testing
- More stable release packaging

## License

MIT. See `LICENSE`.
