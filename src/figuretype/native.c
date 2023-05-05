#include "native.h"

#include "building/building.h"
#include "city/data_private.h"
#include "city/military.h"
#include "figure/combat.h"
#include "figure/formation.h"
#include "figure/image.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "map/terrain.h"

void figure_indigenous_native_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    f->terrain_usage = TERRAIN_USAGE_ANY;
    f->use_cross_country = 0;
    f->max_roam_length = 800;
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
                if (!city_data.military.native_attack_duration) {
                    int x_tile, y_tile;
                    struct building_t *meeting = &all_buildings[b->subtype.native_meeting_center_id];
                    if (map_terrain_get_adjacent_road_or_clear_land(
                        meeting->x, meeting->y, meeting->size, &x_tile, &y_tile)) {
                        f->action_state = FIGURE_ACTION_NATIVE_GOING_TO_MEETING_CENTER;
                        f->destination_x = x_tile;
                        f->destination_y = y_tile;
                    }
                } else {
                    f->action_state = FIGURE_ACTION_NATIVE_ATTACKING;
                    f->destination_x = formations[0].destination_x;
                    f->destination_y = formations[0].destination_y;
                    f->destination_building_id = formations[0].destination_building_id;
                }
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_NATIVE_ATTACKING:
            city_data.figure.attacking_natives++;
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
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = 441 + figure_image_corpse_offset(f);
    } else if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 393 + dir + 8 * (f->image_offset / 2);
    } else if (f->action_state == FIGURE_ACTION_NATIVE_ATTACKING) {
        f->image_id = 297 + dir + 8 * f->image_offset;
    } else {
        f->image_id = 201 + dir + 8 * f->image_offset;
    }
}
