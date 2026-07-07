#pragma once

#include <stdint.h>
#include <stddef.h>

static constexpr uint16_t kNrlDefaultPort = 60050;
static constexpr uint8_t kNrlDeviceModel = 22;
static constexpr uint32_t kHeartbeatIntervalMs = 2000;
static constexpr size_t kMaxChannels = 32;
static constexpr size_t kMaxServers = 3;

struct ChannelConfig {
    char name[16];
    uint32_t group_id;
};

struct ServerConfig {
    char name[16];
    char host[64];
    uint16_t port;
    uint16_t local_port;
    size_t channel_count;
    ChannelConfig channels[kMaxChannels];
};

struct AppConfig {
    char wifi_ssid[33];
    char wifi_password[65];
    char ap_ssid[32];
    char ap_password[16];
    char callsign[7];
    uint8_t callsign_ssid;
    uint8_t volume;
    size_t server_count;
    size_t current_server;
    size_t current_channel;
    ServerConfig servers[kMaxServers];
};

void config_load(AppConfig *config);
void config_save(const AppConfig *config);
bool config_is_usable(const AppConfig *config);
ServerConfig *config_active_server(AppConfig *config);
ChannelConfig *config_active_channel(AppConfig *config);
