#pragma once

#include "app_config.h"
#include <stddef.h>
#include <stdint.h>

static constexpr size_t kNrlHeaderSize = 48;
static constexpr uint8_t kNrlTypeVoiceOpus = 8;
static constexpr uint8_t kNrlTypeHeartbeat = 2;
static constexpr uint8_t kNrlTypeChannel = 7;
static constexpr size_t kNrlMaxPayload = 640;
static constexpr size_t kNrlMaxPacket = kNrlHeaderSize + kNrlMaxPayload;

struct NrlPacketView {
    uint8_t type;
    const uint8_t *payload;
    size_t payload_len;
    char callsign[7];
    uint8_t ssid;
};

size_t nrl_build_packet(uint8_t type, const uint8_t *payload, size_t payload_len, const AppConfig *config, uint8_t *out, size_t out_len);
size_t nrl_build_channel_switch(uint32_t group_id, const AppConfig *config, uint8_t *out, size_t out_len);
bool nrl_parse_packet(const uint8_t *data, size_t len, NrlPacketView *view);
