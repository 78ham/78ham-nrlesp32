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

## NRL-ESP32 References

This Arduino project intentionally follows several proven behaviours from the ESP-IDF `NRL-ESP32` firmware while keeping the implementation smaller:

- NRL2 packets use the same 48-byte header and Type 8 Opus voice frame layout.
- Packet parsing accepts the compatibility case where the header length is `48` but the UDP datagram carries a payload.
- Opus packet buffers are sized to 640 bytes, matching `OPUS_VOICE_MAX_FRAME_BYTES` in `NRL-ESP32`.
- Opus decode uses 3-frame PCM headroom for 40/60 ms senders.
- Audio tasks use 12 KB stacks, close to the upstream 16 KB bridge-task guidance for Opus RX.
- UDP packets are marked with DSCP CS6 / TOS `0xC0` when the Arduino core exposes the socket option API.
- Runtime configuration keeps NRL identity, WiFi, server, local port, codec, and channel concepts aligned with the upstream `ExternalRadioConfig` model.

## Configuration Mode

AP mode remains visible on the TFT until the web portal saves valid settings and the station WiFi connection succeeds. The portal uses server-rendered HTML with minimal CSS to keep RAM and flash use low.
