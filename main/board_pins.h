#pragma once

// ST7789V 7-pin panel: CS DC RST SDA SCK VC GND
// VC powers both logic and backlight on this module.
#define PIN_TFT_SCLK 12
#define PIN_TFT_MOSI 11
#define PIN_TFT_CS   10
#define PIN_TFT_DC   9
#define PIN_TFT_RST  8
#define PIN_TFT_BL   -1

// INMP441 I2S microphone.
#define PIN_MIC_BCLK 4
#define PIN_MIC_WS   5
#define PIN_MIC_SD   6

// MAX98357AETE I2S amplifier.
#define PIN_AMP_BCLK 15
#define PIN_AMP_LRCLK 16
#define PIN_AMP_DIN 17
#define PIN_AMP_SD 18

// Buttons, active low.
#define PIN_KEY_PTT      1
#define PIN_KEY_VOL_UP   2
#define PIN_KEY_VOL_DOWN 3
#define PIN_KEY_UP       40
#define PIN_KEY_DOWN     41
#define PIN_KEY_POWER    42
