#include "wall.h"

#include "building/barracks.h"
#include "building/building.h"
#include "city/view.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/enemy_army.h"
#include "figure/image.h"
#include "figure/movement.h"
#include "figure/properties.h"
#include "figure/route.h"
#include "figuretype/missile.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/road_access.h"
#include "map/routing_terrain.h"
#include "map/terrain.h"
#include "sound/effect.h"

static const int BALLISTA_FIRING_OFFSETS[] = {
    0, 1, 2, 3, 4, 5, 6, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

static const int TOWER_SENTRY_FIRING_OFFSETS[] = {
    0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void figure_ballista_action(figure *f)
{
    building *b = building_get(f->building_id);
    f->terrain_usage = TERRAIN_USAGE_WALLS;
    f->use_cross_country = 0;
    f->is_ghost = 1;
    f->height_adjusted_ticks = 10;
    f->current_height = 45;

    if (b->state != BUILDING_STATE_IN_USE || b->figure_id4 != f->id) {
        f->state = FIGURE_STATE_DEAD;
    }
    if (b->num_workers <= 0 || b->figure_id <= 0) {
        f->state = FIGURE_STATE_DEAD;
    }
    map_figure_delete(f);
    switch (city_view_orientation()) {
        case DIR_0_TOP: f->x = b->x; f->y = b->y; break;
        case DIR_2_RIGHT: f->x = b->x + 1; f->y = b->y; break;
        case DIR_4_BOTTOM: f->x = b->x + 1; f->y = b->y + 1; break;
        case DIR_6_LEFT: f->x = b->x; f->y = b->y + 1; break;
    }
    f->grid_offset = map_grid_offset(f->x, f->y);
    map_figure_add(f);

    switch (f->action_state) {
        case FIGURE_ACTION_149_CORPSE:
            f->state = FIGURE_STATE_DEAD;
            break;
        case FIGURE_ACTION_180_BALLISTA_CREATED:
            f->wait_ticks++;
            if (f->wait_ticks > 20) {
                f->wait_ticks = 0;
                map_point tile;
                if (set_missile_target(f, &tile, 1) || set_missile_target(f, &tile, 0)) {
                    f->action_state = FIGURE_ACTION_181_BALLISTA_FIRING;
                    f->wait_ticks_missile = figure_properties_for_type(f->type)->missile_delay;
                }
            }
            break;
        case FIGURE_ACTION_181_BALLISTA_FIRING:
            f->wait_ticks_missile++;
            if (f->wait_ticks_missile > figure_properties_for_type(f->type)->missile_delay) {
                map_point tile;
                if (set_missile_target(f, &tile, 1) || set_missile_target(f, &tile, 0)) {
                    f->direction = calc_missile_shooter_direction(f->x, f->y, tile.x, tile.y);
                    f->wait_ticks_missile = 0;
                    figure_create_missile(f, &tile, FIGURE_BOLT);
                    sound_effect_play(SOUND_EFFECT_BALLISTA_SHOOT);
                } else {
                    f->action_state = FIGURE_ACTION_180_BALLISTA_CREATED;
                }
            }
            break;
    }
    int dir = figure_image_direction(f);
    if (f->action_state == FIGURE_ACTION_181_BALLISTA_FIRING) {
        f->image_id = image_group(GROUP_FIGURE_BALLISTA) + dir +
            8 * BALLISTA_FIRING_OFFSETS[f->wait_ticks_missile / 4];
    } else {
        f->image_id = image_group(GROUP_FIGURE_BALLISTA) + dir;
    }
}

static int tower_sentry_init_patrol(building *b, int *x_tile, int *y_tile)
{
    int dir = b->figure_roam_direction;
    int x = b->x;
    int y = b->y;
    switch (dir) {
        case DIR_0_TOP: y -= 8; break;
        case DIR_2_RIGHT: x += 8; break;
        case DIR_4_BOTTOM: y += 8; break;
        case DIR_6_LEFT: x -= 8; break;
    }
    map_grid_bound(&x, &y);

    if (map_routing_wall_tile_in_radius(x, y, 6, x_tile, y_tile)) {
        b->figure_roam_direction += 2;
        if (b->figure_roam_direction > 6) b->figure_roam_direction = 0;
        return 1;
    }
    for (int i = 0; i < 4; i++) {
        dir = b->figure_roam_direction;
        b->figure_roam_direction += 2;
        if (b->figure_roam_direction > 6) b->figure_roam_direction = 0;
        x = b->x;
        y = b->y;
        switch (dir) {
            case DIR_0_TOP: y -= 3; break;
            case DIR_2_RIGHT: x += 3; break;
            case DIR_4_BOTTOM: y += 3; break;
            case DIR_6_LEFT: x -= 3; break;
        }
        map_grid_bound(&x, &y);
        if (map_routing_wall_tile_in_radius(x, y, 6, x_tile, y_tile)) {
            return 1;
        }
    }
    return 0;
}

static int tower_sentry_shooting(figure *f)
{
    map_point tile;
    if (!f->in_building_wait_ticks) {
        int target_acquired;
        if (f->is_military_trained) {
            target_acquired = set_missile_target(f, &tile, 1) || set_missile_target(f, &tile, 0);
        } else {
            target_acquired = set_missile_target(f, &tile, 0);
        }
        if (target_acquired) {
            f->wait_ticks_missile++;
            f->progress_on_tile = 15; // align to wall
            f->image_id = image_group(GROUP_FIGURE_TOWER_SENTRY) + figure_image_direction(f) + 96 + 8 * TOWER_SENTRY_FIRING_OFFSETS[f->wait_ticks_missile / 2];
            if (f->wait_ticks_missile > figure_properties_for_type(f->type)->missile_delay) {
                f->direction = calc_missile_shooter_direction(f->x, f->y, tile.x, tile.y);
                f->wait_ticks_missile = 0;
                figure_create_missile(f, &tile, FIGURE_JAVELIN);
            }
            return 1;
        }
    }
    return 0;
}

void figure_tower_sentry_action(figure *f)
{
    building *b = building_get(f->building_id);
    f->height_adjusted_ticks = 10;
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
        f->state = FIGURE_STATE_DEAD;
    }
    figure_image_increase_offset(f, 12);

    switch (f->action_state) {
        case FIGURE_ACTION_149_CORPSE:
            f->image_id = image_group(GROUP_FIGURE_TOWER_SENTRY) + 136 + figure_image_corpse_offset(f);
            figure_combat_handle_corpse(f);
            return;
        case FIGURE_ACTION_150_ATTACK:
            int image_id = image_group(GROUP_FIGURE_TOWER_SENTRY);
            if (f->attack_image_offset < 12) {
                f->image_id = image_id + 96 + figure_image_direction(f);
            } else {
                f->image_id = image_id + 96 + figure_image_direction(f) + 8 * ((f->attack_image_offset - 12) / 2);
                // some buffer images missing, img ids 6841+ are for corpse
                if (f->image_id >= 6841) {
                    f->image_id = f->image_id - 8;
                }
            }
            figure_combat_handle_attack(f);
            return;
        case FIGURE_ACTION_170_TOWER_SENTRY_AT_REST:
            if (!f->is_military_trained) {
                map_point mil_acad_road = { 0 };
                set_closest_military_academy_road_tile(&mil_acad_road, f->building_id);
                if (mil_acad_road.x) {
                    map_point tower_road;
                    if (map_has_road_access(b->x, b->y, b->size, &tower_road)) {
                        f->terrain_usage = TERRAIN_USAGE_ROADS;
                        f->destination_x = tower_road.x;
                        f->destination_y = tower_road.y;
                        figure_movement_move_ticks(f, 1);
                        if (f->x == tower_road.x && f->y == tower_road.y) {
                            f->destination_x = mil_acad_road.x;
                            f->destination_y = mil_acad_road.y;
                            f->action_state = FIGURE_ACTION_85_SOLDIER_GOING_TO_MILITARY_ACADEMY;
                        }
                        return;
                    }
                }
            }
            f->terrain_usage = TERRAIN_USAGE_WALLS;
            f->image_offset = 0;
            f->wait_ticks++;
            if (f->wait_ticks > 40) {
                f->wait_ticks = 0;
                int x_tile, y_tile;
                if (tower_sentry_init_patrol(b, &x_tile, &y_tile)) {
                    f->action_state = FIGURE_ACTION_171_TOWER_SENTRY_PATROLLING;
                    f->destination_x = x_tile;
                    f->destination_y = y_tile;
                    figure_route_remove(f);
                }
            }
            break;
        case FIGURE_ACTION_171_TOWER_SENTRY_PATROLLING:
            f->terrain_usage = TERRAIN_USAGE_WALLS;
            if (tower_sentry_shooting(f)) {
                return;
            } else {
                figure_movement_move_ticks(f, 1);
                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                    f->action_state = FIGURE_ACTION_173_TOWER_SENTRY_RETURNING;
                    f->destination_x = f->source_x;
                    f->destination_y = f->source_y;
                    figure_route_remove(f);
                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                    f->action_state = FIGURE_ACTION_170_TOWER_SENTRY_AT_REST;
                }
            }
            break;
        case FIGURE_ACTION_173_TOWER_SENTRY_RETURNING:
            f->terrain_usage = TERRAIN_USAGE_WALLS;
            if (tower_sentry_shooting(f)) {
                return;
            } else {
                figure_movement_move_ticks(f, 1);
                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                    f->action_state = FIGURE_ACTION_170_TOWER_SENTRY_AT_REST;
                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                    f->state = FIGURE_STATE_DEAD;
                }
            }
            break;
        case FIGURE_ACTION_85_SOLDIER_GOING_TO_MILITARY_ACADEMY:
            if (tower_sentry_shooting(f)) {
                return;
            } else {
                figure_route_remove(f);
                f->terrain_usage = TERRAIN_USAGE_ROADS;
                f->is_military_trained = 1;
                f->height_adjusted_ticks = 0;
                figure_movement_move_ticks(f, f->speed_multiplier);
                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                    f->action_state = FIGURE_ACTION_174_TOWER_SENTRY_GOING_TO_TOWER;
                } else if (f->direction == DIR_FIGURE_REROUTE) {
                    figure_route_remove(f);
                } else if (f->direction == DIR_FIGURE_LOST) {
                    f->state = FIGURE_STATE_DEAD;
                }
            }
            break;
        case FIGURE_ACTION_174_TOWER_SENTRY_GOING_TO_TOWER:
            if (tower_sentry_shooting(f)) {
                return;
            } else {
                f->terrain_usage = TERRAIN_USAGE_ROADS;
                f->height_adjusted_ticks = 0;
                map_point road;
                if (map_has_road_access(b->x, b->y, b->size, &road)) {
                    f->destination_x = road.x;
                    f->destination_y = road.y;
                } else {
                    f->state = FIGURE_STATE_DEAD;
                }
                figure_movement_move_ticks(f, 1);
                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                    map_figure_delete(f);
                    f->source_x = f->x = b->x;
                    f->source_y = f->y = b->y;
                    f->grid_offset = map_grid_offset(f->x, f->y);
                    map_figure_add(f);
                    f->action_state = FIGURE_ACTION_170_TOWER_SENTRY_AT_REST;
                    figure_route_remove(f);
                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                    f->state = FIGURE_STATE_DEAD;
                }
            }
            break;
    }
    if (map_terrain_is(f->grid_offset, TERRAIN_WALL)) {
        f->current_height = 18;
    } else if (map_terrain_is(f->grid_offset, TERRAIN_GATEHOUSE)) { // in tower
        f->in_building_wait_ticks = 24;
    }
    if (f->in_building_wait_ticks) {
        f->in_building_wait_ticks--;
        f->height_adjusted_ticks = 0;
    }
    f->image_id = image_group(GROUP_FIGURE_TOWER_SENTRY) + figure_image_direction(f) + 8 * f->image_offset;
}

void figure_tower_sentry_reroute(void)
{
    for (int i = 1; i < MAX_FIGURES; i++) {
        figure *f = figure_get(i);
        if (f->type != FIGURE_TOWER_SENTRY || map_routing_is_wall_passable(f->grid_offset)) {
            continue;
        }
        // tower sentry got off wall due to rotation
        int x_tile, y_tile;
        if (map_routing_wall_tile_in_radius(f->x, f->y, 2, &x_tile, &y_tile)) {
            figure_route_remove(f);
            f->progress_on_tile = 0;
            map_figure_delete(f);
            f->previous_tile_x = f->x = x_tile;
            f->previous_tile_y = f->y = y_tile;
            f->cross_country_x = 15 * x_tile;
            f->cross_country_y = 15 * y_tile;
            f->grid_offset = map_grid_offset(x_tile, y_tile);
            map_figure_add(f);
            f->action_state = FIGURE_ACTION_173_TOWER_SENTRY_RETURNING;
            f->destination_x = f->source_x;
            f->destination_y = f->source_y;
        } else {
            // Teleport back to tower
            map_figure_delete(f);
            building *b = building_get(f->building_id);
            f->source_x = f->x = b->x;
            f->source_y = f->y = b->y;
            f->grid_offset = map_grid_offset(f->x, f->y);
            map_figure_add(f);
            f->action_state = FIGURE_ACTION_170_TOWER_SENTRY_AT_REST;
            figure_route_remove(f);
        }
    }
}

void figure_kill_tower_sentries_at(int x, int y)
{
    for (int i = 0; i < MAX_FIGURES; i++) {
        figure *f = figure_get(i);
        if (!figure_is_dead(f) && f->type == FIGURE_TOWER_SENTRY) {
            if (calc_maximum_distance(f->x, f->y, x, y) <= 1) {
                f->state = FIGURE_STATE_DEAD;
            }
        }
    }
}
