#ifndef GAME_TICK_H
#define GAME_TICK_H

#include "building/building.h"

void building_house_change_to(struct building_t *house, int type);

void game_tick_run(void);

#endif // GAME_TICK_H
