#ifndef SCENARIO_SCENARIO_H
#define SCENARIO_SCENARIO_H

#include "core/buffer.h"

int scenario_is_saved(void);

void scenario_save_state(buffer *buf);

void scenario_load_state(buffer *buf);

void scenario_settings_save_state(buffer *player_name);

void scenario_settings_load_state(buffer *player_name);

#endif // SCENARIO_SCENARIO_H
