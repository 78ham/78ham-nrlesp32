#pragma once

#include <Arduino.h>
#include "app_config.h"

struct NrlPacketView {
  uint8_t type = 0;
  uint16_t count = 0;
  char callsign[kCallsignLen] = "";
  uint8_t ssid = 0;
  uint8_t devModel = 0;
  const uint8_t *payload = nullptr;
  size_t payloadLen = 0;
};

class Nrl21Codec {
public:
  size_t build(uint8_t type, const uint8_t *payload, size_t payloadLen, const AppConfig &config, uint8_t *out, size_t outLen);
  size_t buildChannelSwitch(uint32_t groupId, const AppConfig &config, uint8_t *out, size_t outLen);
  bool parse(const uint8_t *data, size_t len, NrlPacketView &view) const;

private:
  uint16_t counter_ = 0;
};
