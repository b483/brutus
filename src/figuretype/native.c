#include "native.h"

#include "building/building.h"
#include "city/data_private.h"
#include "city/military.h"
#include "core/calc.h"
#include "figure/formation.h"
#include "figure/image.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "map/terrain.h"

static void set_target_building_for_native(struct figure_t *f)
{
    struct building_t *min_building = 0;
    int min_distance = 10000;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        switch (b->type) {
            case BUILDING_MISSION_POST:
            case BUILDING_NATIVE_HUT:
            case BUILDING_NATIVE_CROPS:
            case BUILDING_NATIVE_MEETING:
            case BUILDING_WAREHOUSE:
            case BUILDING_FORT:
                break;
            default:
            {
                int distance = calc_maximum_distance(city_data.building.main_native_meeting.x, city_data.building.main_native_meeting.y, b->x, b->y);
                if (distance < min_distance) {
                    min_building = b;
                    min_distance = distance;
                }
            }
        }
    }
    if (min_building) {
        f->destination_x = min_building->x;
        f->destination_y = min_building->y;
    }
}

void figure_indigenous_native_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
        f->state = FIGURE_STATE_DEAD;
    }
    figure_image_increase_offset(f, 12);
    switch (f->action_state) {
        case FIGURE_ACTION_NATIVE_GOING_TO_MEETING_CENTER:
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_NATIVE_RETURNING_FROM_MEETING;
                f->destination_x = f->source_x;
                f->destination_y = f->source_y;
            } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
        case FIGURE_ACTION_NATIVE_RETURNING_FROM_MEETING:
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                f->direction == DIR_FIGURE_REROUTE ||
                f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
        case FIGURE_ACTION_NATIVE_CREATED:
            f->image_offset = 0;
            f->wait_ticks++;
            if (f->wait_ticks > 10 + (f->id & 3)) {
                f->wait_ticks = 0;
                if (city_data.military.native_attack_duration) {
                    f->action_state = FIGURE_ACTION_NATIVE_ATTACKING;
                    set_target_building_for_native(f);
                } else {
                    int x_tile, y_tile;
                    struct building_t *meeting = &all_buildings[b->subtype.native_meeting_center_id];
                    if (map_terrain_get_adjacent_road_or_clear_land(
                        meeting->x, meeting->y, meeting->size, &x_tile, &y_tile)) {
                        f->action_state = FIGURE_ACTION_NATIVE_GOING_TO_MEETING_CENTER;
                        f->destination_x = x_tile;
                        f->destination_y = y_tile;
                    }
                }
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_NATIVE_ATTACKING:
            f->terrain_usage = TERRAIN_USAGE_ENEMY;
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                f->direction == DIR_FIGURE_REROUTE ||
                f->direction == DIR_FIGURE_LOST) {
                f->action_state = FIGURE_ACTION_NATIVE_CREATED;
            }
            break;
    }
    int dir;
    if (f->action_state == FIGURE_ACTION_ATTACK || f->direction == DIR_FIGURE_ATTACK) {
        dir = f->attack_direction;
    } else if (f->direction < 8) {
        dir = f->direction;
    } else {
        dir = f->previous_tile_direction;
    }
    dir = figure_image_normalize_direction(dir);

    if (f->action_state == FIGURE_ACTION_ATTACK) {
        if (f->attack_image_offset >= 12) {
            f->image_id = 393 + dir + 8 * ((f->attack_image_offset - 12) / 2);
        } else {
            f->image_id = 393 + dir;
        }
    } else if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 393 + dir + 8 * (f->image_offset / 2);
    } else if (f->action_state == FIGURE_ACTION_NATIVE_ATTACKING) {
        f->image_id = 297 + dir + 8 * f->image_offset;
    } else {
        f->image_id = 201 + dir + 8 * f->image_offset;
    }
}
