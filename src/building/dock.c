#include "dock.h"

#include "city/buildings.h"
#include "city/data.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/routing.h"
#include "map/terrain.h"
#include "scenario/scenario.h"

int building_dock_count_idle_dockers(const struct building_t *dock)
{
    int num_idle = 0;
    for (int i = 0; i < 3; i++) {
        if (dock->data.dock.docker_ids[i]) {
            struct figure_t *f = &figures[dock->data.dock.docker_ids[i]];
            if (f->action_state == FIGURE_ACTION_DOCKER_IDLING ||
                f->action_state == FIGURE_ACTION_DOCKER_IMPORT_QUEUE) {
                num_idle++;
            }
        }
    }
    return num_idle;
}

void building_dock_update_open_water_access(void)
{
    map_routing_calculate_distances_water_boat(scenario.river_entry_point.x, scenario.river_entry_point.y);
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && !b->house_size && b->type == BUILDING_DOCK) {
            if (map_terrain_is_adjacent_to_open_water(b->x, b->y, 3)) {
                b->has_water_access = 1;
            } else {
                b->has_water_access = 0;
            }
        }
    }
}

int building_dock_get_free_destination(int ship_id, struct map_point_t *tile)
{
    if (!city_data.building.working_docks) {
        return 0;
    }
    int dock_id = 0;
    for (int i = 0; i < 10; i++) {
        dock_id = city_data.building.working_dock_ids[i];
        if (!dock_id) continue;
        struct building_t *dock = &all_buildings[dock_id];
        if (!dock->data.dock.trade_ship_id || dock->data.dock.trade_ship_id == ship_id) {
            break;
        }
    }
    // BUG: when 10 docks in city, always takes last one... regardless of whether it is free
    if (dock_id <= 0) {
        return 0;
    }
    struct building_t *dock = &all_buildings[dock_id];
    int dx, dy;
    switch (dock->data.dock.orientation) {
        case 0: dx = 1; dy = -1; break;
        case 1: dx = 3; dy = 1; break;
        case 2: dx = 1; dy = 3; break;
        default: dx = -1; dy = 1; break;
    }
    tile->x = dock->x + dx;
    tile->y = dock->y + dy;
    dock->data.dock.trade_ship_id = ship_id;
    return dock_id;
}

int building_dock_get_queue_destination(struct map_point_t *tile)
{
    if (!city_data.building.working_docks) {
        return 0;
    }
    // first queue position
    for (int i = 0; i < 10; i++) {
        int dock_id = city_data.building.working_dock_ids[i];
        if (!dock_id) continue;
        struct building_t *dock = &all_buildings[dock_id];
        int dx, dy;
        switch (dock->data.dock.orientation) {
            case 0: dx = 2; dy = -2; break;
            case 1: dx = 4; dy = 2; break;
            case 2: dx = 2; dy = 4; break;
            default: dx = -2; dy = 2; break;
        }
        tile->x = dock->x + dx;
        tile->y = dock->y + dy;
        if (!map_has_figure_at(map_grid_offset(tile->x, tile->y))) {
            return dock_id;
        }
    }
    // second queue position
    for (int i = 0; i < 10; i++) {
        int dock_id = city_data.building.working_dock_ids[i];
        if (!dock_id) continue;
        struct building_t *dock = &all_buildings[dock_id];
        int dx, dy;
        switch (dock->data.dock.orientation) {
            case 0: dx = 2; dy = -3; break;
            case 1: dx = 5; dy = 2; break;
            case 2: dx = 2; dy = 5; break;
            default: dx = -3; dy = 2; break;
        }
        tile->x = dock->x + dx;
        tile->y = dock->y + dy;
        if (!map_has_figure_at(map_grid_offset(tile->x, tile->y))) {
            return dock_id;
        }
    }
    return 0;
}
