#include "nrl21.h"

static void putU16(uint8_t *p, uint16_t v) {
  p[0] = static_cast<uint8_t>(v >> 8);
  p[1] = static_cast<uint8_t>(v);
}

static void putU32(uint8_t *p, uint32_t v) {
  p[0] = static_cast<uint8_t>(v >> 24);
  p[1] = static_cast<uint8_t>(v >> 16);
  p[2] = static_cast<uint8_t>(v >> 8);
  p[3] = static_cast<uint8_t>(v);
}

size_t Nrl21Codec::build(uint8_t type, const uint8_t *payload, size_t payloadLen, const AppConfig &config, uint8_t *out, size_t outLen) {
  const size_t total = kNrlHeaderSize + payloadLen;
  if (!out || outLen < total || total > UINT16_MAX) {
    return 0;
  }

  memset(out, 0, total);
  memcpy(out, "NRL2", 4);
  putU16(out + 4, static_cast<uint16_t>(total));

  uint32_t dmrid = static_cast<uint32_t>(ESP.getEfuseMac() & 0xFFFFFF);
  out[6] = static_cast<uint8_t>(dmrid >> 16);
  out[7] = static_cast<uint8_t>(dmrid >> 8);
  out[8] = static_cast<uint8_t>(dmrid);
  out[20] = type;
  out[21] = 1;
  putU16(out + 22, counter_++);
  memcpy(out + 24, config.device.callsign, min(strlen(config.device.callsign), static_cast<size_t>(6)));
  out[30] = config.device.ssid;
  out[31] = config.device.devModel;
  if (payload && payloadLen > 0) {
    memcpy(out + kNrlHeaderSize, payload, payloadLen);
  }
  return total;
}

size_t Nrl21Codec::buildChannelSwitch(uint32_t groupId, const AppConfig &config, uint8_t *out, size_t outLen) {
  uint8_t payload[5] = {0x01, 0, 0, 0, 0};
  putU32(payload + 1, groupId);
  return build(kNrlTypeChannel, payload, sizeof(payload), config, out, outLen);
}

bool Nrl21Codec::parse(const uint8_t *data, size_t len, NrlPacketView &view) const {
  if (!data || len < kNrlHeaderSize || memcmp(data, "NRL2", 4) != 0) {
    return false;
  }
  uint16_t declaredLen = (static_cast<uint16_t>(data[4]) << 8) | data[5];
  if (declaredLen < kNrlHeaderSize || declaredLen > len) {
    declaredLen = len;
  }
  view.type = data[20];
  view.count = (static_cast<uint16_t>(data[22]) << 8) | data[23];
  memset(view.callsign, 0, sizeof(view.callsign));
  memcpy(view.callsign, data + 24, 6);
  view.ssid = data[30];
  view.devModel = data[31];
  view.payload = data + kNrlHeaderSize;
  view.payloadLen = declaredLen - kNrlHeaderSize;
  return true;
}
