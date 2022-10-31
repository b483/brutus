#ifndef SCENARIO_SCENARIO_H
#define SCENARIO_SCENARIO_H

#include "core/buffer.h"

void scenario_save_state(buffer *buf);

void scenario_load_state(buffer *buf);

void scenario_settings_set_player_name(const uint8_t *name);

void scenario_settings_save_state(buffer *player_name);

void scenario_settings_load_state(buffer *player_name);

#endif // SCENARIO_SCENARIO_H
