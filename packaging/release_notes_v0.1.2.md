BackType v0.1.2-pre.1

- Removes the custom Effect Controls drawing path added in v0.1.1 because it could crash After Effects when applying the effect.
- Keeps editable text on macOS through a standard `Text` button that opens a small native dialog.
- Stores the plugin text in hidden arbitrary data and renders from that value.
- Keeps Center Locked as the default anchor mode and the descender baseline fix from v0.1.1.
- Windows still needs its text-edit dialog implementation before that build should be treated as ready.
