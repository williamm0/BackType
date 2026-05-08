BackType v0.1.2-pre.2

- Removes the custom Effect Controls drawing path added in v0.1.1 because it could crash After Effects when applying the effect.
- Removes the hidden arbitrary text parameter too; text is now stored in per-effect sequence data.
- Keeps editable text on macOS through a standard `Text` button that opens a small native dialog.
- Keeps BackType applyable on solids and text layers, while still rendering from Plugin Text for now.
- Keeps Center Locked as the default anchor mode and the descender baseline fix from v0.1.1.
- macOS-only pre-release. No Windows `.aex` is included yet.
