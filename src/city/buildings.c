#include "buildings.h"

#include "city/data_private.h"

void city_buildings_add_senate(building *senate)
{
    city_data.building.senate_placed = 1;
    if (!city_data.building.senate_grid_offset) {
        city_data.building.senate_building_id = senate->id;
        city_data.building.senate_x = senate->x;
        city_data.building.senate_y = senate->y;
        city_data.building.senate_grid_offset = senate->grid_offset;
    }
}

void city_buildings_remove_senate(building *senate)
{
    if (senate->grid_offset == city_data.building.senate_grid_offset) {
        city_data.building.senate_grid_offset = 0;
        city_data.building.senate_x = 0;
        city_data.building.senate_y = 0;
        city_data.building.senate_placed = 0;
    }
}

void city_buildings_add_barracks(building *barracks)
{
    if (!city_data.building.barracks_grid_offset) {
        city_data.building.barracks_building_id = barracks->id;
        city_data.building.barracks_x = barracks->x;
        city_data.building.barracks_y = barracks->y;
        city_data.building.barracks_grid_offset = barracks->grid_offset;
    }
}

void city_buildings_remove_barracks(building *barracks)
{
    if (barracks->grid_offset == city_data.building.barracks_grid_offset) {
        city_data.building.barracks_grid_offset = 0;
        city_data.building.barracks_x = 0;
        city_data.building.barracks_y = 0;
        city_data.building.barracks_placed = 0;
    }
}

void city_buildings_reset_dock_wharf_counters(void)
{
    city_data.building.working_wharfs = 0;
    city_data.building.shipyard_boats_requested = 0;
    for (int i = 0; i < 8; i++) {
        city_data.building.working_dock_ids[i] = 0;
    }
    city_data.building.working_docks = 0;
}

void city_buildings_add_working_wharf(int needs_fishing_boat)
{
    ++city_data.building.working_wharfs;
    if (needs_fishing_boat) {
        ++city_data.building.shipyard_boats_requested;
    }
}

void city_buildings_add_working_dock(int building_id)
{
    if (city_data.building.working_docks < 10) {
        city_data.building.working_dock_ids[city_data.building.working_docks] = building_id;
    }
    ++city_data.building.working_docks;
}
