#include "web_config.h"

#include <esp_http_server.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdlib.h>
#include <string.h>

static const char *TAG = "WEB";
static AppConfig *s_config = nullptr;
static httpd_handle_t s_httpd = nullptr;

static int hex_value(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

static void url_decode(char *dst, size_t dst_len, const char *src, size_t src_len)
{
    if (dst_len == 0) {
        return;
    }
    size_t out = 0;
    for (size_t i = 0; i < src_len && out + 1 < dst_len; ++i) {
        if (src[i] == '+') {
            dst[out++] = ' ';
        } else if (src[i] == '%' && i + 2 < src_len) {
            int hi = hex_value(src[i + 1]);
            int lo = hex_value(src[i + 2]);
            if (hi >= 0 && lo >= 0) {
                dst[out++] = static_cast<char>((hi << 4) | lo);
                i += 2;
            }
        } else {
            dst[out++] = src[i];
        }
    }
    dst[out] = '\0';
}

static bool form_get(const char *body, const char *key, char *out, size_t out_len)
{
    const size_t key_len = strlen(key);
    const char *p = body;
    while (*p) {
        const char *eq = strchr(p, '=');
        if (eq == nullptr) {
            break;
        }
        const char *amp = strchr(eq + 1, '&');
        if (amp == nullptr) {
            amp = body + strlen(body);
        }
        if (static_cast<size_t>(eq - p) == key_len && memcmp(p, key, key_len) == 0) {
            url_decode(out, out_len, eq + 1, amp - eq - 1);
            return true;
        }
        p = *amp == '&' ? amp + 1 : amp;
    }
    if (out_len > 0) {
        out[0] = '\0';
    }
    return false;
}

static uint32_t form_get_u32(const char *body, const char *key, uint32_t fallback)
{
    char value[16];
    if (!form_get(body, key, value, sizeof(value)) || value[0] == '\0') {
        return fallback;
    }
    return strtoul(value, nullptr, 10);
}

static void append(char *page, size_t page_len, size_t *used, const char *text)
{
    int wrote = snprintf(page + *used, page_len - *used, "%s", text);
    if (wrote > 0) {
        *used += static_cast<size_t>(wrote);
        if (*used >= page_len) {
            *used = page_len - 1;
        }
    }
}

static void append_escaped(char *page, size_t page_len, size_t *used, const char *text)
{
    for (const char *p = text; *p != '\0' && *used + 1 < page_len; ++p) {
        switch (*p) {
        case '&':
            append(page, page_len, used, "&amp;");
            break;
        case '<':
            append(page, page_len, used, "&lt;");
            break;
        case '>':
            append(page, page_len, used, "&gt;");
            break;
        case '\"':
            append(page, page_len, used, "&quot;");
            break;
        case '\'':
            append(page, page_len, used, "&#39;");
            break;
        default:
            page[(*used)++] = *p;
            page[*used] = '\0';
            break;
        }
    }
}

static void append_input(char *page, size_t page_len, size_t *used, const char *label, const char *name, const char *value, const char *attrs = "")
{
    int wrote = snprintf(page + *used, page_len - *used, "<label>%s</label><input name='%s' value='", label, name);
    if (wrote > 0) {
        *used += static_cast<size_t>(wrote);
        if (*used >= page_len) {
            *used = page_len - 1;
        }
    }
    append_escaped(page, page_len, used, value);
    wrote = snprintf(page + *used, page_len - *used, "' %s>", attrs);
    if (wrote > 0) {
        *used += static_cast<size_t>(wrote);
        if (*used >= page_len) {
            *used = page_len - 1;
        }
    }
}

static esp_err_t root_handler(httpd_req_t *req)
{
    static char page[7000];
    size_t used = 0;
    ServerConfig *server = config_active_server(s_config);
    append(page, sizeof(page), &used, "<!doctype html><html><head><meta charset='utf-8'><meta name='viewport' content='width=device-width,initial-scale=1'>");
    append(page, sizeof(page), &used, "<title>78HAM ESP32</title><style>body{font-family:sans-serif;margin:16px;max-width:520px}input,button{width:100%;box-sizing:border-box;margin:5px 0 12px;padding:10px}button{background:#111;color:#fff;border:0}fieldset{border:1px solid #ccc;margin:12px 0}small{color:#666}</style></head><body>");
    append(page, sizeof(page), &used, "<h3>78HAM ESP32</h3><form method='post' action='/save'>");
    append_input(page, sizeof(page), &used, "WiFi SSID", "wifi_ssid", s_config->wifi_ssid);
    append_input(page, sizeof(page), &used, "WiFi Password", "wifi_password", s_config->wifi_password, "type='password'");
    append_input(page, sizeof(page), &used, "Callsign", "callsign", s_config->callsign, "maxlength='6'");
    char number[16];
    snprintf(number, sizeof(number), "%u", s_config->callsign_ssid);
    append_input(page, sizeof(page), &used, "Callsign SSID", "callsign_ssid", number, "type='number' min='1' max='99'");
    append(page, sizeof(page), &used, "<fieldset><legend>Server</legend>");
    append_input(page, sizeof(page), &used, "Name", "server_name", server->name, "maxlength='15'");
    append_input(page, sizeof(page), &used, "Host", "server_host", server->host, "maxlength='63'");
    snprintf(number, sizeof(number), "%u", server->port);
    append_input(page, sizeof(page), &used, "Port", "server_port", number, "type='number'");
    append(page, sizeof(page), &used, "</fieldset><fieldset><legend>Displayed Channels</legend><small>Only channels listed here appear on the device.</small>");
    for (size_t i = 0; i < 8; ++i) {
        char field[24];
        char label[32];
        snprintf(label, sizeof(label), "Channel %u Name", static_cast<unsigned>(i + 1));
        snprintf(field, sizeof(field), "ch_name_%u", static_cast<unsigned>(i));
        append_input(page, sizeof(page), &used, label, field, i < server->channel_count ? server->channels[i].name : "", "maxlength='15'");
        snprintf(label, sizeof(label), "Channel %u Group ID", static_cast<unsigned>(i + 1));
        snprintf(field, sizeof(field), "ch_group_%u", static_cast<unsigned>(i));
        if (i < server->channel_count) {
            snprintf(number, sizeof(number), "%lu", static_cast<unsigned long>(server->channels[i].group_id));
        } else {
            number[0] = '\0';
        }
        append_input(page, sizeof(page), &used, label, field, number, "type='number'");
    }
    append(page, sizeof(page), &used, "</fieldset><button type='submit'>Save</button></form></body></html>");
    return httpd_resp_send(req, page, HTTPD_RESP_USE_STRLEN);
}

static void restart_task(void *)
{
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

static esp_err_t save_handler(httpd_req_t *req)
{
    static char body[4096];
    int remaining = req->content_len;
    int offset = 0;
    while (remaining > 0 && offset + 1 < static_cast<int>(sizeof(body))) {
        int got = httpd_req_recv(req, body + offset, sizeof(body) - offset - 1);
        if (got <= 0) {
            return ESP_FAIL;
        }
        offset += got;
        remaining -= got;
    }
    body[offset] = '\0';

    form_get(body, "wifi_ssid", s_config->wifi_ssid, sizeof(s_config->wifi_ssid));
    form_get(body, "wifi_password", s_config->wifi_password, sizeof(s_config->wifi_password));
    form_get(body, "callsign", s_config->callsign, sizeof(s_config->callsign));
    uint32_t ssid = form_get_u32(body, "callsign_ssid", s_config->callsign_ssid);
    if (ssid < 1) {
        ssid = 1;
    } else if (ssid > 99) {
        ssid = 99;
    }
    s_config->callsign_ssid = static_cast<uint8_t>(ssid);

    s_config->server_count = 1;
    s_config->current_server = 0;
    s_config->current_channel = 0;
    ServerConfig *server = &s_config->servers[0];
    form_get(body, "server_name", server->name, sizeof(server->name));
    form_get(body, "server_host", server->host, sizeof(server->host));
    uint32_t port = form_get_u32(body, "server_port", kNrlDefaultPort);
    server->port = port > 0 && port <= 65535 ? static_cast<uint16_t>(port) : kNrlDefaultPort;
    server->local_port = kNrlDefaultPort;
    server->channel_count = 0;

    for (size_t i = 0; i < 8 && server->channel_count < kMaxChannels; ++i) {
        char name_key[16];
        char group_key[16];
        char name[sizeof(server->channels[0].name)];
        snprintf(name_key, sizeof(name_key), "ch_name_%u", static_cast<unsigned>(i));
        snprintf(group_key, sizeof(group_key), "ch_group_%u", static_cast<unsigned>(i));
        form_get(body, name_key, name, sizeof(name));
        uint32_t group_id = form_get_u32(body, group_key, 0);
        if (name[0] == '\0' || group_id == 0) {
            continue;
        }
        ChannelConfig *channel = &server->channels[server->channel_count++];
        strlcpy(channel->name, name, sizeof(channel->name));
        channel->group_id = group_id;
    }
    if (server->channel_count == 0) {
        server->channel_count = 1;
        strlcpy(server->channels[0].name, "Local", sizeof(server->channels[0].name));
        server->channels[0].group_id = 1;
    }

    config_save(s_config);
    httpd_resp_sendstr(req, "<html><body><h3>Saved</h3><p>Device will restart and connect to WiFi.</p></body></html>");
    xTaskCreate(restart_task, "cfg_restart", 2048, nullptr, 1, nullptr);
    return ESP_OK;
}

void web_config_start(AppConfig *config)
{
    s_config = config;
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t init = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&init));

    wifi_config_t ap = {};
    strlcpy(reinterpret_cast<char *>(ap.ap.ssid), config->ap_ssid, sizeof(ap.ap.ssid));
    strlcpy(reinterpret_cast<char *>(ap.ap.password), config->ap_password, sizeof(ap.ap.password));
    ap.ap.ssid_len = strlen(config->ap_ssid);
    ap.ap.channel = 1;
    ap.ap.max_connection = 4;
    ap.ap.authmode = strlen(config->ap_password) >= 8 ? WIFI_AUTH_WPA_WPA2_PSK : WIFI_AUTH_OPEN;
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    ESP_ERROR_CHECK(esp_wifi_start());

    httpd_config_t http_cfg = HTTPD_DEFAULT_CONFIG();
    http_cfg.stack_size = 8192;
    ESP_ERROR_CHECK(httpd_start(&s_httpd, &http_cfg));
    httpd_uri_t root = {};
    root.uri = "/";
    root.method = HTTP_GET;
    root.handler = root_handler;
    httpd_uri_t save = {};
    save.uri = "/save";
    save.method = HTTP_POST;
    save.handler = save_handler;
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &root));
    ESP_ERROR_CHECK(httpd_register_uri_handler(s_httpd, &save));
    ESP_LOGI(TAG, "config portal started ssid=%s pass=%s url=http://192.168.4.1/", config->ap_ssid, config->ap_password);
}
