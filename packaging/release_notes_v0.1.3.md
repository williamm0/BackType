BackType v0.1.3-pre.1

- Removes the custom plugin text editor, Text Source popup, Jitter, Pop, font-size, and text-color controls.
- Uses the applied AE text layer's Source Text for reveal timing.
- Preserves text-layer styling by revealing and moving the text layer's rendered pixels.
- Uses cached approximate character advances for reveal boundaries; unusual text shaping still needs AE project testing.
- Adds Cursor Style, Character Fade-In, and Push Easing.
- Keeps Center Locked as the default anchor mode.
- Sets code and PiPL outflags to zero because the effect now depends on whole-layer text bounds.
- macOS-only pre-release. No Windows `.aex` is included.
