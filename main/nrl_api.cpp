#include "nrl_api.h"

#include <cJSON.h>
#include <esp_http_client.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "API";

struct HttpResponse {
    char *data;
    int len;
    int cap;
};

static esp_err_t http_event(esp_http_client_event_t *event)
{
    if (event->event_id != HTTP_EVENT_ON_DATA || event->data == nullptr || event->data_len <= 0) {
        return ESP_OK;
    }
    HttpResponse *res = static_cast<HttpResponse *>(event->user_data);
    if (res == nullptr || res->data == nullptr || res->len + event->data_len >= res->cap) {
        return ESP_OK;
    }
    memcpy(res->data + res->len, event->data, event->data_len);
    res->len += event->data_len;
    res->data[res->len] = '\0';
    return ESP_OK;
}

static bool http_post_json(const char *host, const char *path, const char *token, const char *body, char *out, int out_len)
{
    char url[160];
    snprintf(url, sizeof(url), "http://%s%s", host, path);
    HttpResponse response = {out, 0, out_len};
    out[0] = '\0';
    esp_http_client_config_t cfg = {};
    cfg.url = url;
    cfg.timeout_ms = 8000;
    cfg.event_handler = http_event;
    cfg.user_data = &response;
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == nullptr) {
        return false;
    }
    esp_http_client_set_method(client, HTTP_METHOD_POST);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    if (token != nullptr && token[0] != '\0') {
        esp_http_client_set_header(client, "x-token", token);
    }
    esp_http_client_set_post_field(client, body, strlen(body));
    esp_err_t err = esp_http_client_perform(client);
    int status = esp_http_client_get_status_code(client);
    esp_http_client_cleanup(client);
    if (err != ESP_OK || status != 200) {
        ESP_LOGW(TAG, "http post failed url=%s err=%s status=%d", url, esp_err_to_name(err), status);
        return false;
    }
    return true;
}

static bool login(const AppConfig *config, char *token, size_t token_len)
{
    token[0] = '\0';
    if (config->user_name[0] == '\0' || config->user_password[0] == '\0') {
        ESP_LOGI(TAG, "skip group sync: NRLLink user/password not configured");
        return false;
    }
    const ServerConfig *server = &config->servers[config->current_server];
    char body[160];
    snprintf(body, sizeof(body), "{\"username\":\"%s\",\"password\":\"%s\"}", config->user_name, config->user_password);
    static char response[2048];
    if (!http_post_json(server->host, "/user/login", nullptr, body, response, sizeof(response))) {
        return false;
    }
    cJSON *root = cJSON_Parse(response);
    if (root == nullptr) {
        return false;
    }
    cJSON *data = cJSON_GetObjectItem(root, "data");
    cJSON *tok = data != nullptr ? cJSON_GetObjectItem(data, "token") : nullptr;
    bool ok = cJSON_IsString(tok) && tok->valuestring[0] != '\0';
    if (ok) {
        strlcpy(token, tok->valuestring, token_len);
    }
    cJSON_Delete(root);
    ESP_LOGI(TAG, "login %s", ok ? "ok" : "failed");
    return ok;
}

static void update_channels_from_groups(AppConfig *config, cJSON *groups)
{
    if (!cJSON_IsArray(groups)) {
        return;
    }
    ServerConfig *server = config_active_server(config);
    size_t count = 0;
    cJSON *group = nullptr;
    cJSON_ArrayForEach(group, groups) {
        if (count >= kMaxChannels) {
            break;
        }
        cJSON *id = cJSON_GetObjectItem(group, "id");
        cJSON *name = cJSON_GetObjectItem(group, "name");
        if (!cJSON_IsNumber(id) || !cJSON_IsString(name)) {
            continue;
        }
        ChannelConfig *channel = &server->available_channels[count++];
        strlcpy(channel->name, name->valuestring, sizeof(channel->name));
        channel->group_id = static_cast<uint32_t>(id->valuedouble);
    }
    if (count > 0) {
        server->available_channel_count = count;
        server->channel_count = count;
        for (size_t i = 0; i < count; ++i) {
            server->channels[i] = server->available_channels[i];
        }
        config->current_channel = 0;
        config_save(config);
        ESP_LOGI(TAG, "synced %u groups as selectable rooms", static_cast<unsigned>(count));
    }
}

static void sync_task(void *arg)
{
    AppConfig *config = static_cast<AppConfig *>(arg);
    char token[768];
    if (!login(config, token, sizeof(token))) {
        vTaskDelete(nullptr);
        return;
    }
    const ServerConfig *server = &config->servers[config->current_server];
    static char response[8192];
    if (http_post_json(server->host, "/group/list/mini", token, "{}", response, sizeof(response))) {
        cJSON *root = cJSON_Parse(response);
        cJSON *data = root != nullptr ? cJSON_GetObjectItem(root, "data") : nullptr;
        update_channels_from_groups(config, data);
        cJSON_Delete(root);
    }
    vTaskDelete(nullptr);
}

void nrl_api_sync_groups_async(AppConfig *config)
{
    if (config == nullptr || !config_is_usable(config)) {
        return;
    }
    xTaskCreatePinnedToCore(sync_task, "nrl_api", 8192, config, 4, nullptr, 0);
}
