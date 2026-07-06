#include "ui.h"

#include <Arduino_GFX_Library.h>
#include "pins.h"
#include "state.h"

UiService UI;

static Arduino_DataBus *s_bus = nullptr;
static Arduino_GFX *s_gfx = nullptr;

bool UiService::begin(AppConfig *config) {
  config_ = config;
  pinMode(PIN_TFT_BL, OUTPUT);
  digitalWrite(PIN_TFT_BL, HIGH);
  s_bus = new Arduino_HWSPI(PIN_TFT_DC, PIN_TFT_CS, PIN_TFT_SCLK, PIN_TFT_MOSI);
  s_gfx = new Arduino_ST7789(s_bus, PIN_TFT_RST, 0, true, 240, 320);
  if (!s_gfx->begin()) {
    return false;
  }
  s_gfx->fillScreen(BLACK);
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
  s_gfx->fillScreen(BLACK);
  s_gfx->setTextColor(WHITE, BLACK);
  s_gfx->setTextSize(2);
  s_gfx->setCursor(12, 20);
  s_gfx->print("78HAM ESP32");
  s_gfx->setTextSize(1);
  s_gfx->setCursor(12, 64);
  s_gfx->print("Config AP");
  s_gfx->setCursor(12, 92);
  s_gfx->print("SSID: ");
  s_gfx->print(ssid);
  s_gfx->setCursor(12, 116);
  s_gfx->print("PASS: ");
  s_gfx->print(password);
  s_gfx->setCursor(12, 148);
  s_gfx->print("Open 192.168.4.1");
}

void UiService::drawMain() {
  if (!s_gfx || State.mode == DeviceMode::ConfigPortal) {
    return;
  }
  s_gfx->fillScreen(BLACK);
  drawHeader();
  drawStatus();
  drawFooter();
}

void UiService::drawHeader() {
  s_gfx->setTextColor(CYAN, BLACK);
  s_gfx->setTextSize(1);
  s_gfx->setCursor(8, 8);
  s_gfx->print("78HAM NRL");
  s_gfx->setCursor(150, 8);
  s_gfx->print(State.wifiConnected ? "WiFi" : "NO WIFI");
}

void UiService::drawStatus() {
  s_gfx->setTextSize(4);
  uint16_t color = WHITE;
  if (State.mode == DeviceMode::Tx) {
    color = RED;
  } else if (State.mode == DeviceMode::Rx) {
    color = GREEN;
  } else if (State.mode == DeviceMode::Offline || State.mode == DeviceMode::Error) {
    color = ORANGE;
  }
  s_gfx->setTextColor(color, BLACK);
  s_gfx->setCursor(28, 48);
  s_gfx->print(modeName(State.mode));

  if (!config_) {
    return;
  }
  const ServerConfig &server = activeServer(*config_);
  const ChannelConfig &channel = activeChannel(*config_);
  s_gfx->setTextSize(2);
  s_gfx->setTextColor(WHITE, BLACK);
  s_gfx->setCursor(12, 126);
  s_gfx->print(config_->device.callsign);
  s_gfx->print('-');
  s_gfx->print(config_->device.ssid);
  s_gfx->setCursor(12, 158);
  s_gfx->print(server.name);
  s_gfx->setCursor(12, 190);
  s_gfx->print(channel.name);
  s_gfx->print(" #");
  s_gfx->print(channel.groupId);

  if (State.remoteCallsign[0]) {
    s_gfx->setCursor(12, 222);
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
  s_gfx->setTextColor(LIGHTGREY, BLACK);
  s_gfx->setCursor(8, 292);
  s_gfx->print("VOL ");
  s_gfx->print(config_->device.volume);
  s_gfx->print(" RSSI ");
  s_gfx->print(State.rssi);
  s_gfx->setCursor(8, 306);
  s_gfx->print(State.ip);
}
