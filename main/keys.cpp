#include "keys.h"

#include "board_pins.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/gpio.h>

static constexpr uint32_t kDebounceMs = 30;
static constexpr uint32_t kLongPressMs = 2500;

struct KeyState {
    gpio_num_t pin;
    KeyId id;
    bool stable;
    bool last_raw;
    bool long_sent;
    uint32_t changed_ms;
    uint32_t pressed_ms;
};

static KeyState s_keys[] = {
    {static_cast<gpio_num_t>(PIN_KEY_PTT), KeyId::Ptt, false, false, false, 0, 0},
    {static_cast<gpio_num_t>(PIN_KEY_VOL_UP), KeyId::VolumeUp, false, false, false, 0, 0},
    {static_cast<gpio_num_t>(PIN_KEY_VOL_DOWN), KeyId::VolumeDown, false, false, false, 0, 0},
    {static_cast<gpio_num_t>(PIN_KEY_UP), KeyId::ChannelUp, false, false, false, 0, 0},
    {static_cast<gpio_num_t>(PIN_KEY_DOWN), KeyId::ChannelDown, false, false, false, 0, 0},
    {static_cast<gpio_num_t>(PIN_KEY_POWER), KeyId::Power, false, false, false, 0, 0},
};

static KeyHandler s_handler = nullptr;

static uint32_t now_ms()
{
    return xTaskGetTickCount() * portTICK_PERIOD_MS;
}

static void update_key(KeyState *key, uint32_t now)
{
    bool raw = gpio_get_level(key->pin) == 0;
    if (raw != key->last_raw) {
        key->last_raw = raw;
        key->changed_ms = now;
    }

    if (now - key->changed_ms >= kDebounceMs && raw != key->stable) {
        key->stable = raw;
        key->long_sent = false;
        if (raw) {
            key->pressed_ms = now;
        }
        if (s_handler != nullptr) {
            s_handler(key->id, key->stable, false);
        }
    }

    if (key->stable && !key->long_sent && now - key->pressed_ms >= kLongPressMs) {
        key->long_sent = true;
        if (s_handler != nullptr) {
            s_handler(key->id, true, true);
        }
    }
}

static void key_task(void *)
{
    while (true) {
        uint32_t now = now_ms();
        for (KeyState &key : s_keys) {
            update_key(&key, now);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void keys_init(void)
{
    for (KeyState &key : s_keys) {
        gpio_config_t cfg = {};
        cfg.pin_bit_mask = 1ULL << key.pin;
        cfg.mode = GPIO_MODE_INPUT;
        cfg.pull_up_en = GPIO_PULLUP_ENABLE;
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&cfg);
        key.last_raw = gpio_get_level(key.pin) == 0;
        key.stable = key.last_raw;
        key.changed_ms = now_ms();
    }
    xTaskCreatePinnedToCore(key_task, "keys", 2048, nullptr, 4, nullptr, 0);
}

void keys_set_handler(KeyHandler handler)
{
    s_handler = handler;
}
