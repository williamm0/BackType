BackType v0.1.4-pre.1

- Replaces render-time AEGP Source Text queries with frame-local raster analysis, fixing AE cache dependency hazards and enabling Multi-Frame Rendering.
- Snaps reveal edges to complete alpha runs so glyphs are not cut at approximate character widths.
- Fixes premultiplied-alpha compositing and adds native AE 16-bpc processing.
- Makes Render Padding expand the output buffer at the current preview resolution.
- Fixes vertical center anchoring and cursor placement for reverse and vertical directions.
- Adds deterministic concurrent-frame coverage, CTest registration, strict warnings, ASan, UBSan, and TSan options.
- Produces universal macOS builds and adds Windows x64/ARM64 PiPL generation through the SDK toolchain.
- Centralizes version metadata and adds reproducible CPack archives.

After Effects runtime testing and native Windows compilation remain environment-level release checks.
