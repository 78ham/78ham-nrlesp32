#pragma once

#include <Arduino.h>
#include "config.h"

struct ChannelConfig {
  char name[kNameLen] = "Local";
  uint32_t groupId = 1;
};

struct ServerConfig {
  char name[kNameLen] = "Main";
  char host[kHostLen] = "101.133.166.204";
  uint16_t port = kNrlDefaultPort;
  uint16_t localPort = kNrlDefaultPort;
  size_t channelCount = 1;
  ChannelConfig channels[kMaxChannelsPerServer];
};

struct DeviceConfig {
  char callsign[kCallsignLen] = "NOCALL";
  uint8_t ssid = kDefaultCallsignSsid;
  uint8_t devModel = kDefaultDeviceModel;
  uint8_t volume = 70;
  uint8_t micGain = 100;
  bool opusEnabled = true;
};

struct WifiConfig {
  char ssid[kWifiSsidLen] = "";
  char password[kWifiPassLen] = "";
};

struct AppConfig {
  uint16_t version = 1;
  DeviceConfig device;
  WifiConfig wifi;
  size_t serverCount = 1;
  ServerConfig servers[kMaxServers];
  size_t currentServer = 0;
  size_t currentChannel = 0;
};

void setStringField(char *dest, size_t len, const String &value);
bool isConfigUsable(const AppConfig &config);
ServerConfig &activeServer(AppConfig &config);
const ServerConfig &activeServer(const AppConfig &config);
ChannelConfig &activeChannel(AppConfig &config);
const ChannelConfig &activeChannel(const AppConfig &config);
