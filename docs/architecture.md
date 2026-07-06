# Architecture

## Runtime Roles

- `Storage`: loads and saves JSON configuration from LittleFS and AP credentials from NVS.
- `WebConfig`: starts a compact AP configuration portal when required.
- `Network`: owns WiFi, UDP socket, NRL2 heartbeat, voice send, and packet receive.
- `Audio`: owns I2S RX/TX and Opus encoder/decoder.
- `Keys`: scans and debounces the six buttons.
- `UI`: renders the ST7789V status screen.

## Voice Path

```text
INMP441 -> I2S RX 16 kHz mono -> Opus encode -> NRL2 Type 8 -> UDP
UDP -> NRL2 Type 8 -> Opus decode -> I2S TX 16 kHz mono -> MAX98357A
```

## Configuration Mode

AP mode remains visible on the TFT until the web portal saves valid settings and the station WiFi connection succeeds. The portal uses server-rendered HTML with minimal CSS to keep RAM and flash use low.
