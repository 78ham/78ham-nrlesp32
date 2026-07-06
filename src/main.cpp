#include <Arduino.h>
#include "app_config.h"
#include "audio.h"
#include "keys.h"
#include "network.h"
#include "state.h"
#include "storage.h"
#include "ui.h"
#include "web_config.h"

static AppConfig g_config;
static String g_apSsid;
static String g_apPassword;
static bool g_configPortalActive = false;

static void enterConfigPortal();
static bool startRuntime();
static void handleKey(KeyId key, bool pressed, bool longPress);
static void switchChannel(int delta);
static void handleVoicePacket(const uint8_t *payload, size_t len);

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("%s %s\n", kFirmwareName, kFirmwareVersion);

  if (!Storage.begin()) {
    State.mode = DeviceMode::Error;
    strlcpy(State.statusText, "LittleFS failed", sizeof(State.statusText));
  }

  Storage.load(g_config);
  g_apSsid = Storage.apSsid();
  g_apPassword = Storage.apPassword();

  UI.begin(&g_config);
  Keys.begin();
  Keys.setHandler(handleKey);
  Network.begin(&g_config);
  Network.setVoiceHandler(handleVoicePacket);

  if (!isConfigUsable(g_config)) {
    enterConfigPortal();
    return;
  }

  if (!startRuntime()) {
    enterConfigPortal();
  }
}

void loop() {
  Keys.loop();

  if (g_configPortalActive) {
    WebConfig.loop();
    UI.loop();
    if (WebConfig.saved()) {
      WebConfig.stop();
      g_configPortalActive = false;
      delay(300);
      if (!startRuntime()) {
        enterConfigPortal();
      }
    }
    delay(5);
    return;
  }

  Network.loop();
  UI.loop();
  delay(5);
}

static void enterConfigPortal() {
  if (g_configPortalActive) {
    return;
  }
  Network.stop();
  g_configPortalActive = true;
  State.mode = DeviceMode::ConfigPortal;
  UI.showConfigPortal(g_apSsid, g_apPassword);
  WebConfig.begin(&g_config, g_apSsid, g_apPassword);
}

static bool startRuntime() {
  State.mode = DeviceMode::Connecting;
  if (!Network.connectWifi()) {
    return false;
  }

  if (!Audio.begin(&g_config)) {
    State.mode = DeviceMode::Error;
    strlcpy(State.statusText, "Audio failed", sizeof(State.statusText));
    Serial.println("Audio init failed");
    return false;
  }
  Audio.startTasks();
  State.mode = DeviceMode::Idle;
  strlcpy(State.statusText, "Ready", sizeof(State.statusText));
  return true;
}

static void handleKey(KeyId key, bool pressed, bool longPress) {
  if (key == KeyId::Ptt) {
    Audio.setPtt(pressed);
    return;
  }

  if (!pressed) {
    return;
  }

  if (key == KeyId::Power && longPress) {
    enterConfigPortal();
    return;
  }

  switch (key) {
  case KeyId::VolumeUp:
    if (g_config.device.volume < 100) {
      g_config.device.volume += 5;
    }
    Storage.save(g_config);
    break;
  case KeyId::VolumeDown:
    if (g_config.device.volume >= 5) {
      g_config.device.volume -= 5;
    }
    Storage.save(g_config);
    break;
  case KeyId::ChannelUp:
    switchChannel(-1);
    break;
  case KeyId::ChannelDown:
    switchChannel(1);
    break;
  default:
    break;
  }
}

static void switchChannel(int delta) {
  ServerConfig &server = activeServer(g_config);
  if (server.channelCount == 0) {
    return;
  }
  int next = static_cast<int>(g_config.currentChannel) + delta;
  if (next < 0) {
    next = static_cast<int>(server.channelCount) - 1;
  }
  if (next >= static_cast<int>(server.channelCount)) {
    next = 0;
  }
  g_config.currentChannel = static_cast<size_t>(next);
  ChannelConfig &channel = activeChannel(g_config);
  Network.sendChannelSwitch(channel.groupId);
  Storage.save(g_config);
}

static void handleVoicePacket(const uint8_t *payload, size_t len) {
  Audio.enqueueOpus(payload, len);
}
