#include "app_config.h"

#include <esp_mac.h>
#include <esp_random.h>
#include <nvs.h>
#include <nvs_flash.h>
#include <stdio.h>
#include <string.h>

static constexpr const char *kNs = "cfg";

static void set_default(AppConfig *config)
{
    memset(config, 0, sizeof(*config));
    strcpy(config->callsign, "NOCALL");
    config->callsign_ssid = 1;
    config->volume = 70;
    config->server_count = 1;
    config->current_server = 0;
    config->current_channel = 0;
    ServerConfig &server = config->servers[0];
    strcpy(server.name, "Main");
    strcpy(server.host, "101.133.166.204");
    server.port = kNrlDefaultPort;
    server.local_port = kNrlDefaultPort;
    server.available_channel_count = 2;
    server.channel_count = 2;
    strcpy(server.available_channels[0].name, "Local");
    server.available_channels[0].group_id = 1;
    strcpy(server.available_channels[1].name, "999");
    server.available_channels[1].group_id = 999;
    strcpy(server.channels[0].name, "Local");
    server.channels[0].group_id = 1;
    strcpy(server.channels[1].name, "999");
    server.channels[1].group_id = 999;

    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_SOFTAP);
    snprintf(config->ap_ssid, sizeof(config->ap_ssid), "78HAM-ESP32-%02X%02X%02X", mac[3], mac[4], mac[5]);
    static constexpr char alphabet[] = "23456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    for (int i = 0; i < 10; ++i) {
        config->ap_password[i] = alphabet[esp_random() % (sizeof(alphabet) - 1)];
    }
    config->ap_password[10] = '\0';
}

void config_load(AppConfig *config)
{
    set_default(config);
    nvs_handle_t h = 0;
    if (nvs_open(kNs, NVS_READONLY, &h) != ESP_OK) {
        config_save(config);
        return;
    }
    size_t size = sizeof(*config);
    esp_err_t err = nvs_get_blob(h, "app", config, &size);
    nvs_close(h);
    if (err != ESP_OK || size != sizeof(*config)) {
        set_default(config);
        config_save(config);
    }
}

void config_save(const AppConfig *config)
{
    nvs_handle_t h = 0;
    if (nvs_open(kNs, NVS_READWRITE, &h) != ESP_OK) {
        return;
    }
    nvs_set_blob(h, "app", config, sizeof(*config));
    nvs_commit(h);
    nvs_close(h);
}

bool config_is_usable(const AppConfig *config)
{
    if (config == nullptr || config->wifi_ssid[0] == '\0' || config->callsign[0] == '\0') {
        return false;
    }
    if (config->server_count == 0 || config->current_server >= config->server_count) {
        return false;
    }
    const ServerConfig &server = config->servers[config->current_server];
    return server.host[0] != '\0' && server.channel_count > 0 && config->current_channel < server.channel_count;
}

ServerConfig *config_active_server(AppConfig *config)
{
    if (config->server_count == 0 || config->current_server >= config->server_count) {
        config->server_count = 1;
        config->current_server = 0;
    }
    return &config->servers[config->current_server];
}

ChannelConfig *config_active_channel(AppConfig *config)
{
    ServerConfig *server = config_active_server(config);
    if (server->channel_count == 0 || config->current_channel >= server->channel_count) {
        config->current_channel = 0;
    }
    return &server->channels[config->current_channel];
}
