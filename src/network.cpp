#include "network.h"

#include <WiFi.h>
#include <lwip/sockets.h>
#include "state.h"

NetworkService Network;

bool NetworkService::begin(AppConfig *config) {
  config_ = config;
  return config_ != nullptr;
}

bool NetworkService::connectWifi() {
  if (!config_) {
    return false;
  }
  uint32_t now = millis();
  if (nextWifiRetryMs_ != 0 && static_cast<int32_t>(now - nextWifiRetryMs_) < 0) {
    return false;
  }
  State.mode = DeviceMode::Connecting;
  strlcpy(State.statusText, "Connecting WiFi", sizeof(State.statusText));
  WiFi.mode(WIFI_STA);
  WiFi.begin(config_->wifi.ssid, config_->wifi.password);

  uint32_t start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < kWifiConnectTimeoutMs) {
    delay(100);
  }

  State.wifiConnected = WiFi.status() == WL_CONNECTED;
  if (State.wifiConnected) {
    wifiFailures_ = 0;
    nextWifiRetryMs_ = 0;
    State.ip = WiFi.localIP();
    State.rssi = WiFi.RSSI();
    strlcpy(State.statusText, "WiFi connected", sizeof(State.statusText));
  } else {
    if (wifiFailures_ < 0xFF) {
      ++wifiFailures_;
    }
    nextWifiRetryMs_ = millis() + kWifiRetryBackoffMs;
    State.mode = DeviceMode::Offline;
    strlcpy(State.statusText, "WiFi failed", sizeof(State.statusText));
  }
  return State.wifiConnected;
}

bool NetworkService::resolveServer() {
  ServerConfig &server = activeServer(*config_);
  IPAddress literal;
  if (literal.fromString(server.host)) {
    serverIp_ = literal;
    return true;
  }
  return WiFi.hostByName(server.host, serverIp_) == 1;
}

bool NetworkService::ensureUdp() {
  if (!config_ || !State.wifiConnected) {
    return false;
  }

  if (!serverResolved_) {
    ServerConfig &server = activeServer(*config_);
    if (!resolveServer()) {
      State.udpReady = false;
      return false;
    }
    udp_.stop();
    State.udpReady = udp_.begin(server.localPort) == 1;
    if (State.udpReady) {
      int tos = kUdpVoiceTos;
      udp_.setOption(IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
    }
    serverResolved_ = State.udpReady;
  }
  return State.udpReady;
}

void NetworkService::loop() {
  if (WiFi.status() != WL_CONNECTED) {
    State.wifiConnected = false;
    State.udpReady = false;
    return;
  }
  State.wifiConnected = true;
  State.rssi = WiFi.RSSI();
  if (!ensureUdp()) {
    return;
  }
  receivePackets();
  sendHeartbeatIfDue();
  if (State.mode == DeviceMode::Rx && millis() - State.lastRxMs > kRxVoiceTimeoutMs) {
    State.mode = DeviceMode::Idle;
    State.remoteCallsign[0] = '\0';
  }
}

void NetworkService::stop() {
  udp_.stop();
  serverResolved_ = false;
  State.udpReady = false;
}

bool NetworkService::sendPacket(uint8_t type, const uint8_t *payload, size_t len) {
  if (!config_ || !ensureUdp()) {
    return false;
  }
  uint8_t packet[kNrlPacketMaxBytes];
  if (len > kOpusMaxPacketBytes) {
    return false;
  }
  size_t packetLen = codec_.build(type, payload, len, *config_, packet, sizeof(packet));
  if (packetLen == 0) {
    return false;
  }
  ServerConfig &server = activeServer(*config_);
  udp_.beginPacket(serverIp_, server.port);
  udp_.write(packet, packetLen);
  return udp_.endPacket() == 1;
}

bool NetworkService::sendVoiceOpus(const uint8_t *payload, size_t len) {
  bool ok = sendPacket(kNrlTypeVoiceOpus, payload, len);
  if (ok) {
    State.lastTxMs = millis();
  }
  return ok;
}

bool NetworkService::sendChannelSwitch(uint32_t groupId) {
  if (!config_ || !ensureUdp()) {
    return false;
  }
  uint8_t packet[kNrlHeaderSize + 5];
  size_t packetLen = codec_.buildChannelSwitch(groupId, *config_, packet, sizeof(packet));
  if (packetLen == 0) {
    return false;
  }
  ServerConfig &server = activeServer(*config_);
  udp_.beginPacket(serverIp_, server.port);
  udp_.write(packet, packetLen);
  return udp_.endPacket() == 1;
}

void NetworkService::sendHeartbeatIfDue() {
  uint32_t now = millis();
  if (now - lastHeartbeatMs_ < kHeartbeatIntervalMs) {
    return;
  }
  lastHeartbeatMs_ = now;
  sendPacket(kNrlTypeHeartbeat, nullptr, 0);
}

void NetworkService::receivePackets() {
  uint8_t buffer[1500];
  int packetLen = udp_.parsePacket();
  while (packetLen > 0) {
    int readLen = udp_.read(buffer, min(packetLen, static_cast<int>(sizeof(buffer))));
    NrlPacketView view;
    if (readLen > 0 && codec_.parse(buffer, readLen, view)) {
      if (view.type == kNrlTypeVoiceOpus && voiceHandler_) {
        strlcpy(State.remoteCallsign, view.callsign, sizeof(State.remoteCallsign));
        State.remoteSsid = view.ssid;
        State.lastRxMs = millis();
        State.mode = State.pttPressed ? DeviceMode::Tx : DeviceMode::Rx;
        voiceHandler_(view.payload, view.payloadLen);
      }
    }
    packetLen = udp_.parsePacket();
  }
}

void NetworkService::setVoiceHandler(VoicePacketHandler handler) {
  voiceHandler_ = handler;
}
