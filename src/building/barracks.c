#include "barracks.h"

#include "building/count.h"
#include "building/model.h"
#include "city/buildings.h"
#include "city/data_private.h"
#include "city/military.h"
#include "city/resource.h"
#include "core/calc.h"
#include "figure/action.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "map/grid.h"
#include "map/road_access.h"

#define INFINITE 10000

static int tower_sentry_request = 0;

int building_get_barracks_for_weapon(int resource, int road_network_id, map_point *dst)
{
    if (resource != RESOURCE_WEAPONS) {
        return 0;
    }
    if (city_data.resource.stockpiled[RESOURCE_WEAPONS]) {
        return 0;
    }
    if (building_count_active(BUILDING_BARRACKS) <= 0) {
        return 0;
    }
    building *b = building_get(city_data.building.barracks_building_id);
    if (b->loads_stored < 5 && city_data.military.legionary_legions) {
        if (map_has_road_access(b->x, b->y, b->size, dst) && b->road_network_id == road_network_id) {
            return b->id;
        }
    }
    return 0;
}

void building_barracks_add_weapon(building *barracks)
{
    if (barracks->id > 0) {
        barracks->loads_stored++;
    }
}

static int get_closest_legion_needing_soldiers(const building *barracks)
{
    int recruit_type = LEGION_RECRUIT_NONE;
    int min_formation_id = 0;
    int min_distance = INFINITE;
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (!formations[i].in_use || !formations[i].is_legion) {
            continue;
        }
        if (formations[i].in_distant_battle || formations[i].legion_recruit_type == LEGION_RECRUIT_NONE) {
            continue;
        }
        if (formations[i].legion_recruit_type == LEGION_RECRUIT_LEGIONARY && barracks->loads_stored <= 0) {
            continue;
        }
        building *fort = building_get(formations[i].building_id);
        int dist = calc_maximum_distance(barracks->x, barracks->y, fort->x, fort->y);
        if (formations[i].legion_recruit_type > recruit_type ||
            (formations[i].legion_recruit_type == recruit_type && dist < min_distance)) {
            recruit_type = formations[i].legion_recruit_type;
            min_formation_id = formations[i].id;
            min_distance = dist;
        }
    }
    return min_formation_id;
}

void set_closest_military_academy_road_tile(map_point *building_road_tile, int final_destination_building_id)
{
    int min_distance = INFINITE;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = building_get(i);
        if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_MILITARY_ACADEMY && b->num_workers >= model_get_building(BUILDING_MILITARY_ACADEMY)->laborers) {
            building *final_dest_building = building_get(final_destination_building_id);
            int dist = calc_maximum_distance(final_dest_building->x, final_dest_building->y, b->x, b->y);
            if (dist < min_distance) {
                if (map_has_road_access(b->x, b->y, b->size, building_road_tile)) {
                    min_distance = dist;
                }
            }
        }
    }
}

int building_barracks_create_soldier(building *barracks, int x, int y)
{
    int formation_id = get_closest_legion_needing_soldiers(barracks);
    if (formation_id > 0) {
        figure *f = figure_create(formations[formation_id].figure_type, x, y, DIR_0_TOP);
        f->formation_id = formation_id;
        f->formation_at_rest = 1;
        switch (f->type) {
            case FIGURE_FORT_LEGIONARY:
                if (barracks->loads_stored > 0) {
                    barracks->loads_stored--;
                }
                break;
            case FIGURE_FORT_JAVELIN:
                f->speed_multiplier = 2;
                f->max_range = 10;
                break;
            case FIGURE_FORT_MOUNTED:
                f->speed_multiplier = 3;
                break;
        }
        f->building_id = formations[formation_id].building_id;
        if (f->type == FIGURE_FORT_MOUNTED) {
            f->mounted_charge_ticks = 10;
            f->mounted_charge_ticks_max = 10;
        }
        map_point mil_acad_road = { 0 };
        set_closest_military_academy_road_tile(&mil_acad_road, formations[formation_id].building_id);
        if (mil_acad_road.x) {
            f->action_state = FIGURE_ACTION_85_SOLDIER_GOING_TO_MILITARY_ACADEMY;
            f->destination_x = mil_acad_road.x;
            f->destination_y = mil_acad_road.y;
            f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
        } else {
            f->action_state = FIGURE_ACTION_81_SOLDIER_GOING_TO_FORT;
        }
    }
    formation_calculate_figures();
    return formation_id ? 1 : 0;
}

int building_barracks_create_tower_sentry(building *barracks, int x, int y)
{
    if (tower_sentry_request <= 0) {
        return 0;
    }
    building *tower = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = building_get(i);
        if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_TOWER && b->num_workers > 0 &&
            !b->figure_id && b->road_network_id == barracks->road_network_id) {
            tower = b;
            break;
        }
    }
    if (!tower) {
        return 0;
    }
    map_point tower_road;
    if (!map_has_road_access(tower->x, tower->y, tower->size, &tower_road)) {
        return 0;
    }
    figure *f = figure_create(FIGURE_TOWER_SENTRY, x, y, DIR_0_TOP);
    f->max_range = 12;
    tower->figure_id = f->id;
    f->building_id = tower->id;
    map_point mil_acad_road = { 0 };
    set_closest_military_academy_road_tile(&mil_acad_road, tower->id);
    if (mil_acad_road.x) {
        f->action_state = FIGURE_ACTION_85_SOLDIER_GOING_TO_MILITARY_ACADEMY;
        f->destination_x = mil_acad_road.x;
        f->destination_y = mil_acad_road.y;
        f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
    } else {
        f->action_state = FIGURE_ACTION_174_TOWER_SENTRY_GOING_TO_TOWER;
    }
    return 1;
}

void building_barracks_request_tower_sentry(void)
{
    tower_sentry_request = 2;
}

void building_barracks_decay_tower_sentry_request(void)
{
    if (tower_sentry_request > 0) {
        tower_sentry_request--;
    }
}

int building_barracks_has_tower_sentry_request(void)
{
    return tower_sentry_request;
}

void building_barracks_save_state(buffer *buf)
{
    buffer_write_i32(buf, tower_sentry_request);
}

void building_barracks_load_state(buffer *buf)
{
    tower_sentry_request = buffer_read_i32(buf);
}
