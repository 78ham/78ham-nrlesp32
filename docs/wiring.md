# Wiring

## ST7789V 7-Pin TFT

Panel silkscreen:

```text
CS DC RST SDA SCK VC GND
```

Connect it to ESP32-S3 as follows:

| TFT Pin | Function | ESP32-S3 |
|---|---|---:|
| `CS` | SPI chip select | `GPIO10` |
| `DC` | Data/command | `GPIO9` |
| `RST` | Reset | `GPIO8` |
| `SDA` | SPI MOSI | `GPIO11` |
| `SCK` | SPI clock | `GPIO12` |
| `VC` | Power | `3.3V` |
| `GND` | Ground | `GND` |

This panel has no separate backlight pin. `PIN_TFT_BL` is set to `-1` in `include/pins.h`, so the firmware does not drive a backlight GPIO.
