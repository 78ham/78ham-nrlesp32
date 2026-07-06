# 78HAM NRL ESP32

Open-source ESP32-S3 network radio terminal for NRLLink.

Target hardware:

- ESP32-S3-N16R8
- 1.77 inch 240x320 COG/ST7789V SPI TFT
- INMP441 I2S microphone
- MAX98357AETE I2S amplifier
- Six buttons: PTT, volume up/down, channel up/down, power/function

The first firmware milestone focuses on pure network voice: WiFi, local AP web configuration, NRL2/NRL21 UDP heartbeat, Opus Type 8 voice uplink/downlink, local server/channel presets, and a compact status UI.

## Status

This repository is an early project skeleton. The code is organized as a long-term open-source firmware project, with hardware pins and protocol defaults kept in `include/pins.h` and `include/config.h`.

## Build

Install PlatformIO, then run:

```powershell
pio run
```

## Wiring

See [docs/wiring.md](docs/wiring.md) for the current ST7789V screen wiring.

Upload filesystem defaults, if needed:

```powershell
pio run -t uploadfs
```

Upload firmware:

```powershell
pio run -t upload
```

## First Boot

On first boot, or when WiFi/server configuration is missing, the device enters AP configuration mode.

- AP SSID: `78HAM-ESP32-XXXXXX`, generated from the ESP32 MAC suffix
- AP password: 10 random readable characters, generated once and stored
- TFT shows a WiFi QR code for joining the AP
- Config URL: `http://192.168.4.1/`

After saving valid WiFi, callsign, server, and channel settings, the device connects to WiFi and enters the main screen.

## Voice Protocol

Default voice codec is Opus over NRL2 Type 8:

- Sample rate: 16 kHz
- Channels: mono
- Frame size: 20 ms / 320 samples
- NRL2 payload: Opus packet
- UDP port: 60050 by default

G.711 Type 1 is intentionally left out of the first milestone implementation path, but the project layout keeps space for codec fallback later.

## Configuration Model

The firmware stores only channels that should appear on the device. If a server has many channels but only a few are useful on this terminal, configure only those channels.

See [data/config.example.json](data/config.example.json).

## Source Layout

- `78ham-nrlesp32.ino` is a small Arduino IDE compatibility wrapper.
- `src/main.cpp` wires boot, config portal, WiFi, audio, keys, and UI together.
- `src/network.*` handles WiFi, UDP, NRL2 heartbeat, Type 8 voice, and Type 7 channel switching.
- `src/audio.*` handles INMP441/MAX98357A I2S and Opus encode/decode.
- `src/web_config.*` serves the minimal AP configuration portal.
- `src/storage.*` stores JSON config in LittleFS and AP credentials in NVS.
- `src/ui.*` renders the ST7789V status screen.
- `src/keys.*` debounces the six buttons.
- `include/pins.h` contains board pin assignments.

## Important Notes

- The default GPIO map is a placeholder. Update `include/pins.h` for your PCB.
- PlatformIO is required to build this project.
- Opus library compatibility must be verified on the target toolchain before hardware audio work.
- The web portal currently edits one server and up to eight displayed channels to keep RAM use low.

## License

MIT. See [LICENSE](LICENSE).
