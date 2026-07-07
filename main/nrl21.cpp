#include "nrl21.h"

#include <esp_mac.h>
#include <string.h>

static uint16_t s_count = 0;

static void put_u16(uint8_t *p, uint16_t v) { p[0] = v >> 8; p[1] = v; }
static void put_u32(uint8_t *p, uint32_t v) { p[0] = v >> 24; p[1] = v >> 16; p[2] = v >> 8; p[3] = v; }

size_t nrl_build_packet(uint8_t type, const uint8_t *payload, size_t payload_len, const AppConfig *config, uint8_t *out, size_t out_len)
{
    const size_t total = kNrlHeaderSize + payload_len;
    if (out == nullptr || config == nullptr || total > out_len || total > 0xFFFF) {
        return 0;
    }
    memset(out, 0, total);
    memcpy(out, "NRL2", 4);
    put_u16(out + 4, total);
    uint8_t mac[6] = {};
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    out[6] = mac[3]; out[7] = mac[4]; out[8] = mac[5];
    const ServerConfig *server = &config->servers[0];
    if (config->server_count > 0 && config->current_server < config->server_count) {
        server = &config->servers[config->current_server];
    }
    memcpy(out + 9, server->password, strnlen(server->password, 11));
    out[20] = type;
    out[21] = 1;
    put_u16(out + 22, ++s_count);
    memcpy(out + 24, config->callsign, strnlen(config->callsign, 6));
    out[30] = config->callsign_ssid;
    out[31] = kNrlDeviceModel;
    if (payload != nullptr && payload_len > 0) {
        memcpy(out + kNrlHeaderSize, payload, payload_len);
    }
    return total;
}

size_t nrl_build_channel_switch(uint32_t group_id, const AppConfig *config, uint8_t *out, size_t out_len)
{
    uint8_t payload[5] = {0x01, 0, 0, 0, 0};
    put_u32(payload + 1, group_id);
    return nrl_build_packet(kNrlTypeChannel, payload, sizeof(payload), config, out, out_len);
}

bool nrl_parse_packet(const uint8_t *data, size_t len, NrlPacketView *view)
{
    if (data == nullptr || view == nullptr || len < kNrlHeaderSize || memcmp(data, "NRL2", 4) != 0) {
        return false;
    }
    uint16_t declared = (static_cast<uint16_t>(data[4]) << 8) | data[5];
    if (declared < kNrlHeaderSize || declared > len) {
        return false;
    }
    if (declared == kNrlHeaderSize && len > kNrlHeaderSize) {
        declared = len;
    }
    memset(view, 0, sizeof(*view));
    view->type = data[20];
    memcpy(view->callsign, data + 24, 6);
    view->callsign[6] = '\0';
    view->ssid = data[30];
    view->payload = data + kNrlHeaderSize;
    view->payload_len = declared - kNrlHeaderSize;
    return true;
}
