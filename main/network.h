#pragma once

#include "app_config.h"

#include <stddef.h>
#include <stdint.h>

using VoicePacketHandler = void (*)(const uint8_t *payload, size_t len);

void network_start(AppConfig *config);
void network_send_channel_switch(uint32_t group_id);
bool network_send_voice_opus(const uint8_t *payload, size_t len);
void network_set_voice_handler(VoicePacketHandler handler);
