#pragma once

// Update these pins to match your PCB before first hardware bring-up.

// ST7789V 240x320 SPI TFT
static constexpr int PIN_TFT_SCLK = 12;
static constexpr int PIN_TFT_MOSI = 11;
static constexpr int PIN_TFT_CS = 10;
static constexpr int PIN_TFT_DC = 9;
static constexpr int PIN_TFT_RST = 8;
static constexpr int PIN_TFT_BL = 7;

// INMP441 I2S microphone
static constexpr int PIN_MIC_BCLK = 4;
static constexpr int PIN_MIC_WS = 5;
static constexpr int PIN_MIC_SD = 6;

// MAX98357AETE I2S amplifier
static constexpr int PIN_AMP_BCLK = 15;
static constexpr int PIN_AMP_LRCLK = 16;
static constexpr int PIN_AMP_DIN = 17;
static constexpr int PIN_AMP_SD = 18;

// Buttons, active low with internal pull-ups.
static constexpr int PIN_KEY_PTT = 1;
static constexpr int PIN_KEY_VOL_UP = 2;
static constexpr int PIN_KEY_VOL_DOWN = 3;
static constexpr int PIN_KEY_UP = 40;
static constexpr int PIN_KEY_DOWN = 41;
static constexpr int PIN_KEY_POWER = 42;
