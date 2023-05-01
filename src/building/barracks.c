#include "barracks.h"

#include "building/count.h"
#include "city/buildings.h"
#include "city/data_private.h"
#include "city/military.h"
#include "city/resource.h"
#include "core/calc.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "figure/formation_legion.h"
#include "map/grid.h"
#include "map/road_access.h"

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

static struct formation_t *get_closest_legion_needing_soldiers(const building *barracks)
{
    struct formation_t *closest_formation = 0;
    int closest_formation_distance = 10000;
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && formations[i].is_legion && !formations[i].in_distant_battle && !formations[i].cursed_by_mars && formations[i].num_figures < formations[i].max_figures) {
            if (formations[i].figure_type == FIGURE_FORT_LEGIONARY && !barracks->loads_stored) {
                continue;
            }
            building *fort = building_get(formations[i].building_id);
            int dist = calc_maximum_distance(barracks->x, barracks->y, fort->x, fort->y);
            if (dist < closest_formation_distance) {
                // prefer legionaries
                if (closest_formation && closest_formation->figure_type == FIGURE_FORT_LEGIONARY && formations[i].figure_type != FIGURE_FORT_LEGIONARY) {
                    continue;
                }
                closest_formation = &formations[i];
                closest_formation_distance = dist;
            }
        }
    }
    return closest_formation;
}

void building_barracks_create_soldier(building *barracks, int x, int y)
{
    struct formation_t *m = get_closest_legion_needing_soldiers(barracks);
    if (m) {
        struct figure_t *f = figure_create(m->figure_type, x, y, DIR_0_TOP);
        f->formation_id = m->id;
        if (f->type == FIGURE_FORT_LEGIONARY && barracks->loads_stored) {
            barracks->loads_stored--;
        }
        f->building_id = m->building_id;
        if (f->type == FIGURE_FORT_MOUNTED) {
            f->mounted_charge_ticks = 10;
            f->mounted_charge_ticks_max = 10;
        }
        add_figure_to_formation(f, m);
        map_point mil_acad_road = { 0 };
        set_destination__closest_building_of_type(m->building_id, BUILDING_MILITARY_ACADEMY, &mil_acad_road);
        if (mil_acad_road.x) {
            f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_MILITARY_ACADEMY;
            f->destination_x = mil_acad_road.x;
            f->destination_y = mil_acad_road.y;
            f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
        } else {
            if (m->is_at_rest) {
                f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_FORT;
            } else {
                deploy_legion_unit_to_formation_location(f, m);
            }
        }
    }
    formation_calculate_figures();
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
    struct figure_t *f = figure_create(FIGURE_TOWER_SENTRY, x, y, DIR_0_TOP);
    tower->figure_id = f->id;
    f->building_id = tower->id;
    map_point mil_acad_road = { 0 };
    set_destination__closest_building_of_type(tower->id, BUILDING_MILITARY_ACADEMY, &mil_acad_road);
    if (mil_acad_road.x) {
        f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_MILITARY_ACADEMY;
        f->destination_x = mil_acad_road.x;
        f->destination_y = mil_acad_road.y;
        f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
    } else {
        f->action_state = FIGURE_ACTION_TOWER_SENTRY_GOING_TO_TOWER;
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
