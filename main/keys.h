#pragma once

#include <stdbool.h>
#include <stdint.h>

enum class KeyId : uint8_t {
    Ptt,
    VolumeUp,
    VolumeDown,
    ChannelUp,
    ChannelDown,
    Power,
};

using KeyHandler = void (*)(KeyId key, bool pressed, bool long_press);

void keys_init(void);
void keys_set_handler(KeyHandler handler);
