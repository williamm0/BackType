# BackType

BackType is an After Effects effect for edit-style typing animations on text layers. It reveals the layer text over time while pushing the rendered text backward, so the line can stay centered or keep the active character near one spot.

It shows up under:

`Effect > jx plugins > BackType`

<img width="2500" height="1080" alt="BackType" src="https://github.com/user-attachments/assets/16a92092-1de8-40e7-ac6f-0d628e239394" />

## What it does

- Uses the actual AE text layer as the source
- Keeps the layer's rendered font, size, color, and styling
- Reveals characters or words with `Progress`
- Moves the text backward while typing
- Supports line, block, and underscore cursor styles
- Uses Center Locked as the default anchor mode
- Has a Render Padding control for letters or cursors that get clipped

## Install

The current pre-release is macOS only. There is no Windows `.aex` release yet.

### macOS

Copy `BackType.plugin` into your After Effects plug-ins folder.

Common path:

`/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore`

Restart After Effects.

This build is not signed or notarized. If macOS blocks it because of quarantine, remove the quarantine flag after you decide you trust the file:

```bash
xattr -dr com.apple.quarantine "/Library/Application Support/Adobe/Common/Plug-ins/7.0/MediaCore/BackType.plugin"
```

## How to use

1. Create an After Effects text layer.
2. Type and style the text normally in AE.
3. Apply `Effect > jx plugins > BackType` to that text layer.
4. Keyframe `Progress` from 0 to 100.
5. Adjust `Anchor Mode`, `Backward Motion`, `Direction`, and `Position` as needed.

BackType no longer has its own text field. Edit the text layer's Source Text instead.

## Controls

- `Progress`: reveals the text from 0 to 100.
- `Anchor Mode`: Center Locked is the default.
- `Backward Motion`: controls how hard the text pushes backward.
- `Direction`: changes the reveal edge and backward push direction.
- `Character Reveal Mode`: character or word reveal.
- `Cursor Style`: line, block, or underscore.
- `Character Fade-In`: fades the newest revealed character without changing layout.
- `Render Padding`: expands the render-safe area if letters or the cursor get clipped at the edge.
- `Opacity`: fades the rendered result.

## Notes

- Descender letters like `y`, `g`, `j`, `p`, and `q` should no longer move the whole line up or down while typing.
- Direction now affects the actual reveal edge and push offset, not just the control value.
- Increase `Render Padding` if a font, cursor style, or pushed motion clips the edge of a letter.
- Cursor blinking does not affect layout.
- Empty text renders nothing.
- This version reads Source Text through the AE SDK, but it preserves styling by using the text layer's rendered pixels. Character reveal uses cached approximate advance widths, so unusual fonts, ligatures, RTL text, or multi-line layouts may need more testing in real AE projects.
- Applying BackType to a solid now renders nothing because there is no text layer Source Text to use.

## Windows

Windows is not released yet. When a real `BackType.aex` build exists, it should be copied into the normal After Effects plug-ins folder, but no `.aex` is included in this pre-release.

## Current state

This is v0.1.3-pre.2. Treat it as a macOS pre-release that still needs real AE runtime verification.

## License

MIT. See `LICENSE`.
