#ifndef WIDGET_CITY_BUILDING_GHOST_H
#define WIDGET_CITY_BUILDING_GHOST_H

#include "map/point.h"

int city_building_ghost_mark_deleting(const struct map_tile_t *tile);
void city_building_ghost_draw(const struct map_tile_t *tile);

#endif // WIDGET_CITY_BUILDING_GHOST_H
