#include "audio.h"

#include <esp_log.h>

static const char *TAG = "AUD";
static bool s_ptt = false;

void audio_init(void)
{
    ESP_LOGI(TAG, "audio placeholder: INMP441/MAX98357A + Opus Type 8 next");
}

void audio_set_ptt(bool pressed)
{
    if (s_ptt == pressed) {
        return;
    }
    s_ptt = pressed;
    ESP_LOGI(TAG, "ptt %s", pressed ? "down" : "up");
}
