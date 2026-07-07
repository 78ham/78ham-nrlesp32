#include "app_config.h"
#include "audio.h"
#include "display.h"
#include "keys.h"
#include "network.h"
#include "web_config.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <nvs_flash.h>

static const char *TAG = "APP";
static AppConfig s_config;

static void switch_channel(int delta)
{
    ServerConfig *server = config_active_server(&s_config);
    if (server->channel_count == 0) {
        return;
    }
    int next = static_cast<int>(s_config.current_channel) + delta;
    if (next < 0) {
        next = static_cast<int>(server->channel_count) - 1;
    } else if (next >= static_cast<int>(server->channel_count)) {
        next = 0;
    }
    s_config.current_channel = static_cast<size_t>(next);
    ChannelConfig *channel = config_active_channel(&s_config);
    network_send_channel_switch(channel->group_id);
    config_save(&s_config);
    ESP_LOGI(TAG, "channel=%s group=%lu", channel->name, static_cast<unsigned long>(channel->group_id));
}

static void handle_key(KeyId key, bool pressed, bool long_press)
{
    if (key == KeyId::Ptt) {
        audio_set_ptt(pressed);
        return;
    }
    if (!pressed) {
        return;
    }
    if (key == KeyId::Power && long_press) {
        ESP_LOGW(TAG, "power long press detected, clearing WiFi config and restarting to AP portal");
        s_config.wifi_ssid[0] = '\0';
        config_save(&s_config);
        esp_restart();
        return;
    }
    switch (key) {
    case KeyId::VolumeUp:
        if (s_config.volume < 100) {
            s_config.volume += 5;
            config_save(&s_config);
        }
        break;
    case KeyId::VolumeDown:
        if (s_config.volume >= 5) {
            s_config.volume -= 5;
            config_save(&s_config);
        }
        break;
    case KeyId::ChannelUp:
        switch_channel(-1);
        break;
    case KeyId::ChannelDown:
        switch_channel(1);
        break;
    default:
        break;
    }
}

extern "C" void app_main(void)
{
    esp_log_level_set("gpio", ESP_LOG_WARN);
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    config_load(&s_config);
    ESP_LOGI(TAG, "78ham-nrlesp32 ESP-IDF boot callsign=%s-%u", s_config.callsign, s_config.callsign_ssid);

    keys_init();
    keys_set_handler(handle_key);
    display_init();
    audio_init();

    if (!config_is_usable(&s_config)) {
        ESP_LOGW(TAG, "config incomplete, starting AP portal ssid=%s pass=%s", s_config.ap_ssid, s_config.ap_password);
        display_show_config_ap(&s_config);
        web_config_start(&s_config);
        return;
    }

    display_show_boot(&s_config);
    network_start(&s_config);
}
