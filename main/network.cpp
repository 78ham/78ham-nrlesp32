#include "network.h"

#include "nrl21.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <string.h>
#include <unistd.h>

static const char *TAG = "NET";
static AppConfig *s_config = nullptr;
static int s_sock = -1;
static sockaddr_in s_server = {};

static bool resolve_server(const char *host, sockaddr_in *out)
{
    memset(out, 0, sizeof(*out));
    out->sin_family = AF_INET;
    ServerConfig *server = config_active_server(s_config);
    out->sin_port = htons(server->port);
    if (inet_pton(AF_INET, host, &out->sin_addr) == 1) {
        return true;
    }
    addrinfo hints = {};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    addrinfo *res = nullptr;
    if (getaddrinfo(host, nullptr, &hints, &res) != 0 || res == nullptr) {
        return false;
    }
    out->sin_addr = reinterpret_cast<sockaddr_in *>(res->ai_addr)->sin_addr;
    freeaddrinfo(res);
    return true;
}

static bool open_udp()
{
    ServerConfig *server = config_active_server(s_config);
    if (!resolve_server(server->host, &s_server)) {
        ESP_LOGE(TAG, "resolve failed: %s", server->host);
        return false;
    }
    s_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s_sock < 0) {
        ESP_LOGE(TAG, "socket failed");
        return false;
    }
    sockaddr_in local = {};
    local.sin_family = AF_INET;
    local.sin_port = htons(server->local_port);
    local.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(s_sock, reinterpret_cast<sockaddr *>(&local), sizeof(local)) < 0) {
        ESP_LOGE(TAG, "bind failed");
        close(s_sock);
        s_sock = -1;
        return false;
    }
    timeval timeout = {.tv_sec = 0, .tv_usec = 20000};
    setsockopt(s_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    int tos = 0xC0;
    setsockopt(s_sock, IPPROTO_IP, IP_TOS, &tos, sizeof(tos));
    ESP_LOGI(TAG, "udp local=%u remote=%s:%u", server->local_port, server->host, server->port);
    return true;
}

static void send_packet(uint8_t type, const uint8_t *payload, size_t len)
{
    if (s_sock < 0) {
        return;
    }
    uint8_t packet[kNrlMaxPacket];
    size_t packet_len = nrl_build_packet(type, payload, len, s_config, packet, sizeof(packet));
    if (packet_len > 0) {
        sendto(s_sock, packet, packet_len, 0, reinterpret_cast<sockaddr *>(&s_server), sizeof(s_server));
    }
}

static void network_task(void *)
{
    while (s_sock < 0) {
        if (open_udp()) {
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
    uint32_t last_heartbeat = 0;
    uint8_t rx[kNrlMaxPacket];
    while (true) {
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        if (now - last_heartbeat >= kHeartbeatIntervalMs) {
            last_heartbeat = now;
            send_packet(kNrlTypeHeartbeat, nullptr, 0);
        }
        sockaddr_in from = {};
        socklen_t from_len = sizeof(from);
        int got = recvfrom(s_sock, rx, sizeof(rx), 0, reinterpret_cast<sockaddr *>(&from), &from_len);
        if (got > 0) {
            NrlPacketView view = {};
            if (nrl_parse_packet(rx, got, &view) && view.type == kNrlTypeVoiceOpus) {
                ESP_LOGI(TAG, "opus rx from=%s-%u bytes=%u", view.callsign, view.ssid, static_cast<unsigned>(view.payload_len));
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static void wifi_event(void *, esp_event_base_t event_base, int32_t event_id, void *)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG, "wifi disconnected, retry");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ESP_LOGI(TAG, "wifi got ip");
        xTaskCreatePinnedToCore(network_task, "nrl_net", 8192, nullptr, 5, nullptr, 0);
    }
}

void network_start(AppConfig *config)
{
    s_config = config;
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event, nullptr, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event, nullptr, nullptr));
    wifi_config_t sta = {};
    strlcpy(reinterpret_cast<char *>(sta.sta.ssid), config->wifi_ssid, sizeof(sta.sta.ssid));
    strlcpy(reinterpret_cast<char *>(sta.sta.password), config->wifi_password, sizeof(sta.sta.password));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    ESP_ERROR_CHECK(esp_wifi_start());
}

void network_send_channel_switch(uint32_t group_id)
{
    if (s_sock < 0 || s_config == nullptr) {
        return;
    }
    uint8_t packet[kNrlHeaderSize + 5];
    size_t len = nrl_build_channel_switch(group_id, s_config, packet, sizeof(packet));
    if (len > 0) {
        sendto(s_sock, packet, len, 0, reinterpret_cast<sockaddr *>(&s_server), sizeof(s_server));
    }
}
