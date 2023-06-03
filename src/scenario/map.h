#ifndef SCENARIO_MAP_H
#define SCENARIO_MAP_H

#include "map/tiles.h"

void scenario_map_init(void);

void scenario_map_init_entry_exit(void);

int scenario_map_has_river_entry(void);

int scenario_map_has_river_exit(void);

int scenario_map_closest_fishing_point(int x, int y, struct map_point_t *fish);

#endif // SCENARIO_MAP_H
