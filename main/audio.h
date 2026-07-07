#pragma once

#include <stddef.h>
#include <stdint.h>

void audio_init(void);
void audio_set_ptt(bool pressed);
void audio_enqueue_opus(const uint8_t *payload, size_t len);
