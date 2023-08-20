#include "entertainer.h"

#include "building/building.h"
#include "city/resource.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "map/grid.h"
#include "map/road_access.h"
#include "map/road_network.h"
#include "scenario/scenario.h"

static int determine_destination(int x, int y, int type1, int type2)
{
    int road_network = map_road_network_get(map_grid_offset(x, y));

    building_list_small_clear();

    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        if (b->type != type1 && b->type != type2) {
            continue;
        }
        if (b->distance_from_entry && b->road_network_id == road_network) {
            if (b->type == BUILDING_HIPPODROME && b->prev_part_building_id) {
                continue;
            }
            building_list_small_add(i);
        }
    }
    int total_venues = building_list_small_size();
    if (total_venues <= 0) {
        return 0;
    }
    const int *venues = building_list_small_items();
    int min_building_id = 0;
    int min_distance = 10000;
    for (int i = 0; i < total_venues; i++) {
        struct building_t *b = &all_buildings[venues[i]];
        int days_left;
        if (b->type == type1) {
            days_left = b->data.entertainment.days1;
        } else if (b->type == type2) {
            days_left = b->data.entertainment.days2;
        } else {
            days_left = 0;
        }
        int dist = days_left + calc_maximum_distance(x, y, b->x, b->y);
        if (dist < min_distance) {
            min_distance = dist;
            min_building_id = venues[i];
        }
    }
    return min_building_id;
}

static void update_shows(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->destination_building_id];
    if (b->type < BUILDING_AMPHITHEATER || b->type > BUILDING_COLOSSEUM) {
        return;
    }
    switch (f->type) {
        case FIGURE_ACTOR:
            b->data.entertainment.play++;
            if (b->data.entertainment.play >= 5) {
                b->data.entertainment.play = 0;
            }
            if (b->type == BUILDING_THEATER) {
                b->data.entertainment.days1 = 32;
            } else {
                b->data.entertainment.days2 = 32;
            }
            break;
        case FIGURE_GLADIATOR:
            if (b->type == BUILDING_AMPHITHEATER) {
                b->data.entertainment.days1 = 32;
            } else {
                b->data.entertainment.days2 = 32;
            }
            break;
        case FIGURE_LION_TAMER:
        case FIGURE_CHARIOTEER:
            b->data.entertainment.days1 = 32;
            break;
    }
}

static void update_image(struct figure_t *f)
{
    int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);

    if (f->type == FIGURE_CHARIOTEER) {
        f->cart_image_id = 0;
        f->image_id = image_group(GROUP_FIGURE_CHARIOTEER) + dir + 8 * f->image_offset;
        return;
    }
    int image_id;
    if (f->type == FIGURE_ACTOR) {
        image_id = image_group(GROUP_FIGURE_ACTOR);
    } else if (f->type == FIGURE_GLADIATOR) {
        image_id = image_group(GROUP_FIGURE_GLADIATOR);
    } else if (f->type == FIGURE_LION_TAMER) {
        image_id = image_group(GROUP_FIGURE_LION_TAMER);
        if (f->wait_ticks_missile >= 96) {
            image_id = image_group(GROUP_FIGURE_LION_TAMER_WHIP);
        }
        f->cart_image_id = image_group(GROUP_FIGURE_LION);
    } else {
        return;
    }
    f->image_id = image_id + dir + 8 * f->image_offset;
    if (f->cart_image_id) {
        f->cart_image_id += dir + 8 * f->image_offset;
        figure_image_set_cart_offset(f, dir);
    }
}

void figure_entertainer_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    f->cart_image_id = EMPTY_CART_IMG_ID;
    f->use_cross_country = 0;
    figure_image_increase_offset(f, 12);
    f->wait_ticks_missile++;
    if (f->wait_ticks_missile >= 120) {
        f->wait_ticks_missile = 0;
    }
    if (scenario.gladiator_revolt.state == EVENT_IN_PROGRESS && f->type == FIGURE_GLADIATOR) {
        if (f->action_state == FIGURE_ACTION_ENTERTAINER_GOING_TO_VENUE ||
            f->action_state == FIGURE_ACTION_ENTERTAINER_ROAMING ||
            f->action_state == FIGURE_ACTION_ENTERTAINER_RETURNING) {
            f->type = FIGURE_ENEMY_GLADIATOR;
            figure_route_remove(f);
            f->roam_length = 0;
            f->action_state = FIGURE_ACTION_NATIVE_CREATED;
            f->is_targetable = 1;
            f->terrain_usage = TERRAIN_USAGE_ANY;
            return;
        }
    }
    int speed_factor = f->type == FIGURE_CHARIOTEER ? 2 : 1;
    switch (f->action_state) {
        case FIGURE_ACTION_ENTERTAINER_AT_SCHOOL_CREATED:
            f->is_invisible = 1;
            f->image_offset = 0;
            f->wait_ticks_missile = 0;
            f->wait_ticks--;
            if (f->wait_ticks <= 0) {
                int x_road, y_road;
                if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                    f->action_state = FIGURE_ACTION_ENTERTAINER_EXITING_SCHOOL;
                    figure_movement_set_cross_country_destination(f, x_road, y_road);
                    f->roam_length = 0;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            break;
        case FIGURE_ACTION_ENTERTAINER_EXITING_SCHOOL:
            f->use_cross_country = 1;
            f->is_invisible = 1;
            if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                int dst_building_id = 0;
                switch (f->type) {
                    case FIGURE_ACTOR:
                        dst_building_id = determine_destination(f->x, f->y, BUILDING_THEATER, BUILDING_AMPHITHEATER);
                        break;
                    case FIGURE_GLADIATOR:
                        dst_building_id = determine_destination(f->x, f->y, BUILDING_AMPHITHEATER, BUILDING_COLOSSEUM);
                        break;
                    case FIGURE_LION_TAMER:
                        dst_building_id = determine_destination(f->x, f->y, BUILDING_COLOSSEUM, 0);
                        break;
                    case FIGURE_CHARIOTEER:
                        dst_building_id = determine_destination(f->x, f->y, BUILDING_HIPPODROME, 0);
                        break;
                }
                if (dst_building_id) {
                    struct building_t *b_dst = &all_buildings[dst_building_id];
                    int x_road, y_road;
                    if (map_closest_road_within_radius(b_dst->x, b_dst->y, b_dst->size, 2, &x_road, &y_road)) {
                        f->destination_building_id = dst_building_id;
                        f->action_state = FIGURE_ACTION_ENTERTAINER_GOING_TO_VENUE;
                        f->destination_x = x_road;
                        f->destination_y = y_road;
                        f->roam_length = 0;
                    } else {
                        figure_delete(f);
                        return;
                    }
                } else {
                    figure_delete(f);
                    return;
                }
            }
            f->is_invisible = 1;
            break;
        case FIGURE_ACTION_ENTERTAINER_GOING_TO_VENUE:
            f->is_invisible = 0;
            f->roam_length++;
            if (f->roam_length >= 3200) {
                figure_delete(f);
                return;
            }
            figure_movement_move_ticks(f, speed_factor);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                update_shows(f);
                figure_delete(f);
                return;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_ENTERTAINER_ROAMING:
            f->is_invisible = 0;
            f->roam_length++;
            if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                int x_road, y_road;
                if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                    f->action_state = FIGURE_ACTION_ENTERTAINER_RETURNING;
                    f->destination_x = x_road;
                    f->destination_y = y_road;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            figure_movement_roam_ticks(f, speed_factor);
            break;
        case FIGURE_ACTION_ENTERTAINER_RETURNING:
            figure_movement_move_ticks(f, speed_factor);
            if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
    }
    update_image(f);
}
