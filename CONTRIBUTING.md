# Contributing

This firmware is intended to stay small, readable, and hardware-friendly.

Guidelines:

- Keep hardware pin changes in `include/pins.h`.
- Keep protocol constants in `include/config.h`.
- Avoid large web assets, external fonts, and client-side frameworks in the captive portal.
- Prefer fixed-size buffers on the audio path.
- Keep the default voice path Opus Type 8 unless a change explicitly targets compatibility mode.
- Document hardware assumptions in `docs/` when adding board variants.
