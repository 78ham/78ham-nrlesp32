#pragma once

#include "app_config.h"

void network_start(AppConfig *config);
void network_send_channel_switch(uint32_t group_id);
