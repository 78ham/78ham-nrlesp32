#pragma once

#include <Arduino.h>

static constexpr const char *kFirmwareName = NRL_FIRMWARE_NAME;
static constexpr const char *kFirmwareVersion = NRL_FIRMWARE_VERSION;

static constexpr uint16_t kNrlDefaultPort = 60050;
static constexpr uint16_t kNrlHeaderSize = 48;
static constexpr uint8_t kNrlTypeVoiceG711 = 1;
static constexpr uint8_t kNrlTypeHeartbeat = 2;
static constexpr uint8_t kNrlTypeChannel = 7;
static constexpr uint8_t kNrlTypeVoiceOpus = 8;

static constexpr uint8_t kDefaultDeviceModel = 22;
static constexpr uint8_t kDefaultCallsignSsid = 1;
static constexpr uint32_t kHeartbeatIntervalMs = 2000;
static constexpr uint32_t kWifiConnectTimeoutMs = 15000;
static constexpr uint32_t kRxVoiceTimeoutMs = 180;

static constexpr int kOpusSampleRate = 16000;
static constexpr int kOpusChannels = 1;
static constexpr int kOpusFrameMs = 20;
static constexpr int kOpusFrameSamples = kOpusSampleRate * kOpusFrameMs / 1000;
static constexpr int kOpusMaxPacketBytes = 640;
static constexpr int kOpusBitrate = 20000;
static constexpr int kOpusComplexity = 4;
static constexpr int kNrlPacketMaxBytes = kNrlHeaderSize + kOpusMaxPacketBytes;
static constexpr uint8_t kWifiFallbackFailureThreshold = 3;
static constexpr uint32_t kWifiRetryBackoffMs = 30000;
static constexpr uint8_t kUdpVoiceTos = 0xC0;

static constexpr size_t kMaxServers = 3;
static constexpr size_t kMaxChannelsPerServer = 32;
static constexpr size_t kNameLen = 16;
static constexpr size_t kHostLen = 64;
static constexpr size_t kWifiSsidLen = 32;
static constexpr size_t kWifiPassLen = 64;
static constexpr size_t kCallsignLen = 7;
