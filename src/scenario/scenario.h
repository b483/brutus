#ifndef SCENARIO_SCENARIO_H
#define SCENARIO_SCENARIO_H

#include "core/buffer.h"

void scenario_editor_create(int map_size);

void scenario_save_state(struct buffer_t *buf);

void scenario_load_state(struct buffer_t *buf);

void scenario_settings_set_player_name(const char *name);

void scenario_settings_save_state(struct buffer_t *player_name);

void scenario_settings_load_state(struct buffer_t *player_name);

#endif // SCENARIO_SCENARIO_H
