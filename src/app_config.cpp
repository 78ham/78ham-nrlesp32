#include "app_config.h"

void setStringField(char *dest, size_t len, const String &value) {
  if (len == 0) {
    return;
  }
  strlcpy(dest, value.c_str(), len);
}

bool isConfigUsable(const AppConfig &config) {
  if (strlen(config.wifi.ssid) == 0 || strlen(config.device.callsign) == 0) {
    return false;
  }
  if (config.serverCount == 0 || config.currentServer >= config.serverCount) {
    return false;
  }
  const ServerConfig &server = config.servers[config.currentServer];
  return strlen(server.host) > 0 && server.channelCount > 0 && config.currentChannel < server.channelCount;
}

ServerConfig &activeServer(AppConfig &config) {
  if (config.serverCount == 0) {
    config.serverCount = 1;
  }
  if (config.currentServer >= config.serverCount) {
    config.currentServer = 0;
  }
  return config.servers[config.currentServer];
}

const ServerConfig &activeServer(const AppConfig &config) {
  size_t index = config.currentServer < config.serverCount ? config.currentServer : 0;
  return config.servers[index];
}

ChannelConfig &activeChannel(AppConfig &config) {
  ServerConfig &server = activeServer(config);
  if (server.channelCount == 0) {
    server.channelCount = 1;
  }
  if (config.currentChannel >= server.channelCount) {
    config.currentChannel = 0;
  }
  return server.channels[config.currentChannel];
}

const ChannelConfig &activeChannel(const AppConfig &config) {
  const ServerConfig &server = activeServer(config);
  size_t index = config.currentChannel < server.channelCount ? config.currentChannel : 0;
  return server.channels[index];
}
