#ifndef BUILDING_DOCK_H
#define BUILDING_DOCK_H

#include "building/building.h"
#include "map/point.h"

int building_dock_count_idle_dockers(const struct building_t *dock);

void building_dock_update_open_water_access(void);

int building_dock_get_free_destination(int ship_id, map_point *tile);

int building_dock_get_queue_destination(map_point *tile);

#endif // BUILDING_DOCK_H
