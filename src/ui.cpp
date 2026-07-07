#include "ui.h"

#include <Arduino_GFX_Library.h>
#include <qrcode.h>
#include "pins.h"
#include "state.h"

UiService UI;

static Arduino_DataBus *s_bus = nullptr;
static Arduino_GFX *s_gfx = nullptr;

static constexpr uint16_t COLOR_BLACK = 0x0000;
static constexpr uint16_t COLOR_WHITE = 0xFFFF;
static constexpr uint16_t COLOR_RED = 0xF800;
static constexpr uint16_t COLOR_GREEN = 0x07E0;
static constexpr uint16_t COLOR_CYAN = 0x07FF;
static constexpr uint16_t COLOR_ORANGE = 0xFD20;
static constexpr uint16_t COLOR_LIGHTGREY = 0xC618;

static String wifiQrPayload(const String &ssid, const String &password) {
  String payload = "WIFI:T:WPA;S:";
  payload += ssid;
  payload += ";P:";
  payload += password;
  payload += ";;";
  return payload;
}

static void drawQrCode(const String &payload, int x, int y, int maxSize) {
  static constexpr uint8_t kQrVersion = 4;
  static uint8_t qrcodeData[256];
  QRCode qrcode;
  qrcode_initText(&qrcode, qrcodeData, kQrVersion, ECC_LOW, payload.c_str());

  int scale = maxSize / qrcode.size;
  if (scale < 1) {
    scale = 1;
  }
  int qrSize = qrcode.size * scale;
  int offsetX = x + (maxSize - qrSize) / 2;
  int offsetY = y + (maxSize - qrSize) / 2;

  s_gfx->fillRect(x - 4, y - 4, maxSize + 8, maxSize + 8, COLOR_WHITE);
  for (uint8_t row = 0; row < qrcode.size; ++row) {
    for (uint8_t col = 0; col < qrcode.size; ++col) {
      if (qrcode_getModule(&qrcode, col, row)) {
        s_gfx->fillRect(offsetX + col * scale, offsetY + row * scale, scale, scale, COLOR_BLACK);
      }
    }
  }
}

bool UiService::begin(AppConfig *config) {
  config_ = config;
  if (PIN_TFT_BL >= 0) {
    pinMode(PIN_TFT_BL, OUTPUT);
    digitalWrite(PIN_TFT_BL, HIGH);
  }
  s_bus = new Arduino_HWSPI(PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_SCLK, PIN_TFT_MOSI);
  s_gfx = new Arduino_ST7789(s_bus, PIN_TFT_RST, 1, true, 320, 240);
  if (!s_gfx->begin()) {
    return false;
  }
  s_gfx->fillScreen(COLOR_BLACK);
  s_gfx->setTextWrap(false);
  return true;
}

void UiService::loop() {
  if (millis() - lastDrawMs_ < 150) {
    return;
  }
  lastDrawMs_ = millis();
  drawMain();
}

void UiService::showConfigPortal(const String &ssid, const String &password) {
  portalSsid_ = ssid;
  portalPassword_ = password;
  if (!s_gfx) {
    return;
  }
  s_gfx->fillScreen(COLOR_BLACK);
  s_gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  s_gfx->setTextSize(2);
  s_gfx->setCursor(10, 4);
  s_gfx->print("78HAM ESP32");
  s_gfx->setTextSize(1);
  s_gfx->setCursor(10, 26);
  s_gfx->print("Scan QR to join AP");
  drawQrCode(wifiQrPayload(ssid, password), 8, 42, 100);
  s_gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);
  s_gfx->setTextSize(1);
  s_gfx->setCursor(120, 50);
  s_gfx->print("SSID: ");
  s_gfx->print(ssid);
  s_gfx->setCursor(120, 74);
  s_gfx->print("PASS: ");
  s_gfx->print(password);
  s_gfx->setCursor(120, 98);
  s_gfx->print("Open 192.168.4.1");
}

void UiService::drawMain() {
  if (!s_gfx || State.mode == DeviceMode::ConfigPortal) {
    return;
  }
  s_gfx->fillScreen(COLOR_BLACK);
  drawHeader();
  drawStatus();
  drawFooter();
}

void UiService::drawHeader() {
  s_gfx->setTextColor(COLOR_CYAN, COLOR_BLACK);
  s_gfx->setTextSize(1);
  s_gfx->setCursor(4, 4);
  s_gfx->print("78HAM NRL");
  s_gfx->setCursor(250, 4);
  s_gfx->print(State.wifiConnected ? "WiFi" : "NO WIFI");
}

void UiService::drawStatus() {
  s_gfx->setTextSize(3);
  uint16_t color = COLOR_WHITE;
  if (State.mode == DeviceMode::Tx) {
    color = COLOR_RED;
  } else if (State.mode == DeviceMode::Rx) {
    color = COLOR_GREEN;
  } else if (State.mode == DeviceMode::Offline || State.mode == DeviceMode::Error) {
    color = COLOR_ORANGE;
  }
  s_gfx->setTextColor(color, COLOR_BLACK);
  s_gfx->setCursor(100, 22);
  s_gfx->print(modeName(State.mode));

  if (!config_) {
    return;
  }
  const ServerConfig &server = activeServer(*config_);
  const ChannelConfig &channel = activeChannel(*config_);

  s_gfx->setTextSize(2);
  s_gfx->setTextColor(COLOR_WHITE, COLOR_BLACK);

  s_gfx->setCursor(4, 58);
  s_gfx->print(config_->device.callsign);
  s_gfx->print('-');
  s_gfx->print(config_->device.ssid);

  s_gfx->setCursor(4, 82);
  s_gfx->print(server.name);
  if (config_->serverCount > 1) {
    s_gfx->print(" [");
    s_gfx->print(config_->currentServer + 1);
    s_gfx->print("/");
    s_gfx->print(config_->serverCount);
    s_gfx->print("]");
  }

  s_gfx->setCursor(4, 106);
  s_gfx->print(channel.name);
  s_gfx->print(" #");
  s_gfx->print(channel.groupId);

  if (State.remoteCallsign[0]) {
    s_gfx->setCursor(4, 132);
    s_gfx->print("RX ");
    s_gfx->print(State.remoteCallsign);
    s_gfx->print('-');
    s_gfx->print(State.remoteSsid);
  }
}

void UiService::drawFooter() {
  if (!config_) {
    return;
  }
  s_gfx->setTextSize(1);
  s_gfx->setTextColor(COLOR_LIGHTGREY, COLOR_BLACK);
  s_gfx->setCursor(4, 218);
  s_gfx->print("VOL ");
  s_gfx->print(config_->device.volume);
  s_gfx->print("  RSSI ");
  s_gfx->print(State.rssi);
  s_gfx->print("  ");
  s_gfx->print(State.ip);
}