#ifndef CITY_BUILDINGS_H
#define CITY_BUILDINGS_H

#include "building/building.h"

void city_buildings_add_senate(building *senate);
void city_buildings_remove_senate(building *senate);

void city_buildings_add_barracks(building *barracks);
void city_buildings_remove_barracks(building *barracks);

void city_buildings_reset_dock_wharf_counters(void);
void city_buildings_add_working_wharf(int needs_fishing_boat);
void city_buildings_add_working_dock(int building_id);

#endif // CITY_BUILDINGS_H
