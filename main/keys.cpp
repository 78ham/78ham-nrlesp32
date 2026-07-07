#include "keys.h"

#include "board_pins.h"
#include <driver/gpio.h>

void keys_init(void)
{
    const int pins[] = {PIN_KEY_PTT, PIN_KEY_VOL_UP, PIN_KEY_VOL_DOWN, PIN_KEY_UP, PIN_KEY_DOWN, PIN_KEY_POWER};
    for (int pin : pins) {
        gpio_config_t cfg = {};
        cfg.pin_bit_mask = 1ULL << pin;
        cfg.mode = GPIO_MODE_INPUT;
        cfg.pull_up_en = GPIO_PULLUP_ENABLE;
        cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
        cfg.intr_type = GPIO_INTR_DISABLE;
        gpio_config(&cfg);
    }
}
