# Hardware Bring-Up

## Pin Map

The default pins in `include/pins.h` are placeholders for early firmware work. Update them to match the PCB before powering peripherals.

Avoid ESP32-S3 pins used by USB, flash, PSRAM, boot strapping, or board-specific functions.

## Suggested Bring-Up Order

1. Serial boot log.
2. ST7789V display init and backlight.
3. AP portal display and web form.
4. WiFi station connection.
5. NRL2 Type 2 heartbeat to UDP port 60050.
6. MAX98357A 16 kHz playback test.
7. INMP441 16 kHz capture level test.
8. Local Opus encode/decode loopback.
9. NRL2 Type 8 Opus uplink/downlink.
10. NRL2 Type 7 channel switch.

## Opus Notes

The first milestone uses Opus Type 8 by default:

- 16 kHz mono
- 20 ms frames
- 320 PCM samples per frame
- 20 kbps VBR target
- complexity 4

The selected PlatformIO dependency is `pschatzmann/arduino-libopus`. If this library changes its include path or API, adapt only `src/audio.cpp`.

## Web Portal Constraints

The captive portal intentionally uses server-rendered HTML and small inline CSS. Do not add JavaScript frameworks, icon fonts, or external assets.

The first form edits one server and up to eight displayed channels. The configuration model supports more channels per server, so the portal can be expanded without changing the runtime model.
