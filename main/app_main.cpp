#include "app_config.h"
#include "audio.h"
#include "display.h"
#include "keys.h"
#include "network.h"

#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <nvs_flash.h>

static const char *TAG = "APP";
static AppConfig s_config;

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
    display_init();
    audio_init();

    if (!config_is_usable(&s_config)) {
        ESP_LOGW(TAG, "config incomplete, AP portal will be implemented next ssid=%s pass=%s", s_config.ap_ssid, s_config.ap_password);
        display_show_config_ap(&s_config);
        return;
    }

    display_show_boot(&s_config);
    network_start(&s_config);
}
