#pragma once

#include <Arduino.h>
#include <WiFiUdp.h>
#include "app_config.h"
#include "nrl21.h"

using VoicePacketHandler = void (*)(const uint8_t *payload, size_t len);

class NetworkService {
public:
  bool begin(AppConfig *config);
  void loop();
  bool connectWifi();
  void stop();
  bool sendVoiceOpus(const uint8_t *payload, size_t len);
  bool sendChannelSwitch(uint32_t groupId);
  void setVoiceHandler(VoicePacketHandler handler);
  void resetUdp();

private:
  bool ensureUdp();
  bool resolveServer();
  void sendHeartbeatIfDue();
  void receivePackets();
  bool sendPacket(uint8_t type, const uint8_t *payload, size_t len);

  AppConfig *config_ = nullptr;
  WiFiUDP udp_;
  IPAddress serverIp_;
  bool serverResolved_ = false;
  uint8_t wifiFailures_ = 0;
  uint32_t nextWifiRetryMs_ = 0;
  uint32_t lastHeartbeatMs_ = 0;
  VoicePacketHandler voiceHandler_ = nullptr;
  Nrl21Codec codec_;
};

extern NetworkService NrlNetwork;