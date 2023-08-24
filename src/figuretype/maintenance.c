#include "maintenance.h"

#include "building/building.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "map/map.h"
#include "sound/sound.h"

#define PREFECT_LEASH_RANGE 20

void figure_engineer_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];

    f->use_cross_country = 0;
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
        figure_delete(f);
        return;
    }
    figure_image_increase_offset(f, 12);

    switch (f->action_state) {
        case FIGURE_ACTION_ENGINEER_CREATED:
            f->is_invisible = 1;
            f->image_offset = 0;
            f->wait_ticks--;
            if (f->wait_ticks <= 0) {
                int x_road, y_road;
                if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                    f->action_state = FIGURE_ACTION_ENGINEER_ENTERING_EXITING;
                    figure_movement_set_cross_country_destination(f, x_road, y_road);
                    f->roam_length = 0;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            break;
        case FIGURE_ACTION_ENGINEER_ENTERING_EXITING:
            f->use_cross_country = 1;
            f->is_invisible = 1;
            if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                if (map_building_at(f->grid_offset) == f->building_id) {
                    // returned to own building
                    figure_delete(f);
                    return;
                } else {
                    f->action_state = FIGURE_ACTION_ENGINEER_ROAMING;
                    figure_movement_init_roaming(f);
                    f->roam_length = 0;
                }
            }
            break;
        case FIGURE_ACTION_ENGINEER_ROAMING:
            f->is_invisible = 0;
            f->roam_length++;
            if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                int x_road, y_road;
                if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                    f->action_state = FIGURE_ACTION_ENGINEER_RETURNING;
                    f->destination_x = x_road;
                    f->destination_y = y_road;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            figure_movement_roam_ticks(f, 1);
            break;
        case FIGURE_ACTION_ENGINEER_RETURNING:
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_ENGINEER_ENTERING_EXITING;
                figure_movement_set_cross_country_destination(f, b->x, b->y);
                f->roam_length = 0;
            } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
    }
    f->image_id = image_group(GROUP_FIGURE_ENGINEER) + figure_image_direction(f) + 8 * f->image_offset;
}

static int building_maintenance_get_closest_burning_ruin(int x, int y, int *distance)
{
    int min_free_building_id = 0;
    int min_occupied_building_id = 0;
    int min_occupied_dist = *distance = 10000;

    const int *burning = building_list_burning_items();
    int burning_size = building_list_burning_size();
    for (int i = 0; i < burning_size; i++) {
        int building_id = burning[i];
        struct building_t *b = &all_buildings[building_id];
        if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_BURNING_RUIN && !b->ruin_has_plague) {
            int dist = calc_maximum_distance(x, y, b->x, b->y);
            if (b->figure_id4) {
                if (dist < min_occupied_dist) {
                    min_occupied_dist = dist;
                    min_occupied_building_id = building_id;
                }
            } else if (dist < *distance) {
                *distance = dist;
                min_free_building_id = building_id;
            }
        }
    }
    if (!min_free_building_id && min_occupied_dist <= 2) {
        min_free_building_id = min_occupied_building_id;
        *distance = 2;
    }
    return min_free_building_id;
}

static int fight_fire(struct figure_t *f)
{
    if (building_list_burning_size() <= 0) {
        return 0;
    }
    switch (f->action_state) {
        case FIGURE_ACTION_PREFECT_CREATED:
        case FIGURE_ACTION_PREFECT_ENTERING_EXITING:
        case FIGURE_ACTION_PREFECT_GOING_TO_FIRE:
        case FIGURE_ACTION_PREFECT_AT_FIRE:
            return 0;
    }
    f->wait_ticks_missile++;
    if (f->wait_ticks_missile < 20) {
        return 0;
    }
    int distance;
    int ruin_id = building_maintenance_get_closest_burning_ruin(f->x, f->y, &distance);
    if (ruin_id > 0 && distance <= 25) {
        struct building_t *ruin = &all_buildings[ruin_id];
        f->wait_ticks_missile = 0;
        f->action_state = FIGURE_ACTION_PREFECT_GOING_TO_FIRE;
        f->destination_x = ruin->road_access_x;
        f->destination_y = ruin->road_access_y;
        f->destination_building_id = ruin_id;
        figure_route_remove(f);
        ruin->figure_id4 = f->id;
        return 1;
    }
    return 0;
}

static void extinguish_fire(struct figure_t *f)
{
    struct building_t *burn = &all_buildings[f->destination_building_id];
    int distance = calc_maximum_distance(f->x, f->y, burn->x, burn->y);
    if (burn->state == BUILDING_STATE_IN_USE && burn->type == BUILDING_BURNING_RUIN && distance < 2) {
        burn->fire_duration = 32;
        play_sound_effect(SOUND_EFFECT_FIRE_SPLASH);
    } else {
        f->wait_ticks = 1;
    }
    f->attack_direction = calc_general_direction(f->x, f->y, burn->x, burn->y);
    if (f->attack_direction >= 8) {
        f->attack_direction = 0;
    }
    f->wait_ticks--;
    if (f->wait_ticks <= 0) {
        f->wait_ticks_missile = 20;
        if (!fight_fire(f)) {
            struct building_t *b = &all_buildings[f->building_id];
            int x_road, y_road;
            if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                f->action_state = FIGURE_ACTION_PREFECT_RETURNING;
                f->destination_x = x_road;
                f->destination_y = y_road;
                figure_route_remove(f);
            } else {
                figure_delete(f);
                return;
            }
        }
    }
}

void figure_prefect_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
        figure_delete(f);
        return;
    }

    f->terrain_usage = TERRAIN_USAGE_ROADS;
    f->use_cross_country = 0;

    figure_image_increase_offset(f, 12);

    switch (f->action_state) {
        case FIGURE_ACTION_PREFECT_CREATED:
            f->is_invisible = 1;
            f->image_offset = 0;
            f->wait_ticks--;
            if (f->wait_ticks <= 0) {
                int x_road, y_road;
                if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                    f->action_state = FIGURE_ACTION_PREFECT_ENTERING_EXITING;
                    figure_movement_set_cross_country_destination(f, x_road, y_road);
                    f->roam_length = 0;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            break;
        case FIGURE_ACTION_PREFECT_ENTERING_EXITING:
            f->use_cross_country = 1;
            f->is_invisible = 1;
            if (figure_movement_move_ticks_cross_country(f, f->speed_multiplier) == 1) {
                if (map_building_at(f->grid_offset) == f->building_id) {
                    // returned to own building
                    figure_delete(f);
                    return;
                } else {
                    f->action_state = FIGURE_ACTION_PREFECT_ROAMING;
                    figure_movement_init_roaming(f);
                    f->roam_length = 0;
                }
            }
            break;
        case FIGURE_ACTION_PREFECT_ROAMING:
            f->is_invisible = 0;
            struct figure_t *target = melee_unit__set_closest_target(f);
            if (target && calc_maximum_distance(f->x, f->y, b->x, b->y) < PREFECT_LEASH_RANGE) {
                f->terrain_usage = TERRAIN_USAGE_ANY;
                f->roam_length = figure_properties[f->type].max_roam_length;
                f->prefect_recent_guard_duty = 1;
                figure_movement_move_ticks(f, f->speed_multiplier);
                if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                    f->action_state = FIGURE_ACTION_PREFECT_RETURNING;
                }
                break;
            }
            if (fight_fire(f)) {
                break;
            }
            f->roam_length++;
            if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                f->action_state = FIGURE_ACTION_PREFECT_RETURNING;
            }
            figure_movement_roam_ticks(f, f->speed_multiplier);
            break;
        case FIGURE_ACTION_PREFECT_RETURNING:
            if (f->prefect_recent_guard_duty) {
                f->terrain_usage = TERRAIN_USAGE_ANY;
            }
            int x_road, y_road;
            if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                f->destination_x = x_road;
                f->destination_y = y_road;
                figure_route_remove(f);
            }
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_PREFECT_ENTERING_EXITING;
                figure_movement_set_cross_country_destination(f, b->x, b->y);
                f->roam_length = 0;
                f->prefect_recent_guard_duty = 0;
                f->target_figure_id = 0;
            } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_PREFECT_GOING_TO_FIRE:
            f->terrain_usage = TERRAIN_USAGE_ANY;
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_PREFECT_AT_FIRE;
                figure_route_remove(f);
                f->roam_length = 0;
                f->wait_ticks = 50;
            } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_PREFECT_AT_FIRE:
            extinguish_fire(f);
            break;
    }
    // graphic id
    int dir;
    if (f->action_state == FIGURE_ACTION_PREFECT_AT_FIRE) {
        dir = f->attack_direction;
    } else if (f->direction < 8) {
        dir = f->direction;
    } else {
        dir = f->previous_tile_direction;
    }
    dir = figure_image_normalize_direction(dir);
    switch (f->action_state) {
        case FIGURE_ACTION_PREFECT_GOING_TO_FIRE:
            f->image_id = image_group(GROUP_FIGURE_PREFECT_WITH_BUCKET) + dir + 8 * f->image_offset;
            break;
        case FIGURE_ACTION_PREFECT_AT_FIRE:
            f->image_id = image_group(GROUP_FIGURE_PREFECT_WITH_BUCKET) + dir + 96 + 8 * (f->image_offset / 2);
            break;
        default:
            f->image_id = image_group(GROUP_FIGURE_PREFECT) + dir + 8 * f->image_offset;
            break;
    }
}
