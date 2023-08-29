#include "figuretype.h"

#include "core/calc.h"
#include "core/image.h"
#include "core/random.h"
#include "empire/empire.h"
#include "figure/combat.h"
#include "figure/formation_enemy.h"
#include "figure/formation_herd.h"
#include "figure/formation_legion.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "figure/sound.h"
#include "figure/trader.h"
#include "game/game.h"
#include "scenario/scenario.h"
#include "sound/sound.h"

#include <stdlib.h>

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

static const int FLOTSAM_RESOURCE_IDS[] = {
    3, 1, 3, 2, 1, 3, 2, 3, 2, 1, 3, 3, 2, 3, 3, 3, 1, 2, 0, 1
};
static const int FLOTSAM_WAIT_TICKS[] = {
    10, 50, 100, 130, 200, 250, 400, 430, 500, 600, 70, 750, 820, 830, 900, 980, 1010, 1030, 1200, 1300
};

static const int CLOUD_TILE_OFFSETS[] = { 0, 0, 0, 1, 1, 2 };

static const int CLOUD_CC_OFFSETS[] = { 0, 7, 14, 7, 14, 7 };

static const int CLOUD_SPEED[] = {
    1, 2, 1, 3, 2, 1, 3, 2, 1, 1, 2, 1, 2, 1, 3, 1
};

static const struct map_point_t CLOUD_DIRECTION[] = {
    {0, -6}, {-2, -5}, {-4, -4}, {-5, -2}, {-6, 0}, {-5, -2}, {-4, -4}, {-2, -5},
    {0, -6}, {-2, -5}, {-4, -4}, {-5, -2}, {-6, 0}, {-5, -2}, {-4, -4}, {-2, -5}
};

static const struct map_point_t HORSE_DESTINATION_1[] = {
    {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1}, {9, 1}, {10, 1}, {11, 1}, {12, 2},
    {12, 3}, {11, 3}, {10, 3}, {9, 3}, {8, 3}, {7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}, {2, 2}
};
static const struct map_point_t HORSE_DESTINATION_2[] = {
    {12, 3}, {11, 3}, {10, 3}, {9, 3}, {8, 3}, {7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}, {2, 2},
    {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1}, {9, 1}, {10, 1}, {11, 1}, {12, 2}
};

enum {
    HORSE_CREATED = 0,
    HORSE_RACING = 1,
    HORSE_FINISHED = 2
};

static void set_horse_destination(struct figure_t *f, int state)
{
    struct building_t *b = &all_buildings[f->building_id];
    int orientation = city_view_orientation();
    if (state == HORSE_CREATED) {
        map_figure_delete(f);
        if (orientation == DIR_0_TOP || orientation == DIR_6_LEFT) {
            f->destination_x = b->x + HORSE_DESTINATION_1[f->wait_ticks_missile].x;
            f->destination_y = b->y + HORSE_DESTINATION_1[f->wait_ticks_missile].y;
        } else {
            f->destination_x = b->x + HORSE_DESTINATION_2[f->wait_ticks_missile].x;
            f->destination_y = b->y + HORSE_DESTINATION_2[f->wait_ticks_missile].y;
        }
        if (f->resource_id == 1) {
            f->destination_y++;
        }
        f->x = f->destination_x;
        f->y = f->destination_y;
        f->cross_country_x = 15 * f->x;
        f->cross_country_y = 15 * f->y;
        f->grid_offset = map_grid_offset(f->x, f->y);
        map_figure_add(f);
    } else if (state == HORSE_RACING) {
        if (orientation == DIR_0_TOP || orientation == DIR_6_LEFT) {
            f->destination_x = b->x + HORSE_DESTINATION_1[f->wait_ticks_missile].x;
            f->destination_y = b->y + HORSE_DESTINATION_1[f->wait_ticks_missile].y;
        } else {
            f->destination_x = b->x + HORSE_DESTINATION_2[f->wait_ticks_missile].x;
            f->destination_y = b->y + HORSE_DESTINATION_2[f->wait_ticks_missile].y;
        }
    } else if (state == HORSE_FINISHED) {
        if (orientation == DIR_0_TOP || orientation == DIR_6_LEFT) {
            if (f->resource_id) {
                f->destination_x = b->x + 1;
                f->destination_y = b->y + 2;
            } else {
                f->destination_x = b->x + 1;
                f->destination_y = b->y + 1;
            }
        } else {
            if (f->resource_id) {
                f->destination_x = b->x + 12;
                f->destination_y = b->y + 3;
            } else {
                f->destination_x = b->x + 12;
                f->destination_y = b->y + 2;
            }
        }
    }
}

void figure_hippodrome_horse_action(struct figure_t *f)
{
    city_data.entertainment.hippodrome_has_race = 1;
    f->use_cross_country = 1;
    figure_image_increase_offset(f, 8);

    switch (f->action_state) {
        case FIGURE_ACTION_HIPPODROME_HORSE_CREATED:
            f->image_offset = 0;
            f->wait_ticks_missile = 0;
            set_horse_destination(f, HORSE_CREATED);
            f->wait_ticks++;
            if (f->wait_ticks > 60 && f->resource_id == 0) {
                f->action_state = FIGURE_ACTION_HIPPODROME_HORSE_RACING;
                f->wait_ticks = 0;
            }
            f->wait_ticks++;
            if (f->wait_ticks > 20 && f->resource_id == 1) {
                f->action_state = FIGURE_ACTION_HIPPODROME_HORSE_RACING;
                f->wait_ticks = 0;
            }
            break;
        case FIGURE_ACTION_HIPPODROME_HORSE_RACING:
            f->direction = calc_general_direction(f->x, f->y, f->destination_x, f->destination_y);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->wait_ticks_missile++;
                if (f->wait_ticks_missile >= 22) {
                    f->wait_ticks_missile = 0;
                    f->leading_figure_id++;
                    if (f->leading_figure_id >= 6) {
                        f->wait_ticks = 0;
                        f->action_state = FIGURE_ACTION_HIPPODROME_HORSE_DONE;
                    }
                    if ((f->id + random_byte()) & 1) {
                        f->speed_multiplier = 3;
                    } else {
                        f->speed_multiplier = 4;
                    }
                } else if (f->wait_ticks_missile == 11) {
                    if ((f->id + random_byte()) & 1) {
                        f->speed_multiplier = 3;
                    } else {
                        f->speed_multiplier = 4;
                    }
                }
                set_horse_destination(f, HORSE_RACING);
                f->direction = calc_general_direction(f->x, f->y, f->destination_x, f->destination_y);
                figure_movement_set_cross_country_direction(f,
                    f->cross_country_x, f->cross_country_y, 15 * f->destination_x, 15 * f->destination_y, 0);
            }
            if (f->action_state != FIGURE_ACTION_HIPPODROME_HORSE_DONE) {
                figure_movement_move_ticks_cross_country(f, f->speed_multiplier);
            }
            break;
        case FIGURE_ACTION_HIPPODROME_HORSE_DONE:
            if (!f->wait_ticks) {
                set_horse_destination(f, HORSE_FINISHED);
                f->direction = calc_general_direction(f->x, f->y, f->destination_x, f->destination_y);
                figure_movement_set_cross_country_direction(f,
                    f->cross_country_x, f->cross_country_y, 15 * f->destination_x, 15 * f->destination_y, 0);
            }
            if (f->direction != DIR_FIGURE_AT_DESTINATION) {
                figure_movement_move_ticks_cross_country(f, 1);
            }
            f->wait_ticks++;
            if (f->wait_ticks > 30) {
                f->image_offset = 0;
            }
            f->wait_ticks++;
            if (f->wait_ticks > 150) {
                figure_delete(f);
                return;
            }
            break;
    }

    int dir = figure_image_direction(f);
    if (f->resource_id == 0) {
        f->image_id = image_group(GROUP_FIGURE_HIPPODROME_HORSE_1) + dir + 8 * f->image_offset;
        f->cart_image_id = image_group(GROUP_FIGURE_HIPPODROME_CART_1) + dir;
    } else {
        f->image_id = image_group(GROUP_FIGURE_HIPPODROME_HORSE_2) + dir + 8 * f->image_offset;
        f->cart_image_id = image_group(GROUP_FIGURE_HIPPODROME_CART_2) + dir;
    }
    int cart_dir = (dir + 4) % 8;
    figure_image_set_cart_offset(f, cart_dir);
}

void figure_hippodrome_horse_reroute(void)
{
    for (int i = 1; i < MAX_FIGURES; i++) {
        struct figure_t *f = &figures[i];
        if (figure_is_alive(f) && f->type == FIGURE_HIPPODROME_HORSES) {
            f->wait_ticks_missile = 0;
            set_horse_destination(f, HORSE_CREATED);
        }
    }
}

void figure_create_homeless(int x, int y, int num_people)
{
    struct figure_t *f = figure_create(FIGURE_HOMELESS, x, y, DIR_0_TOP);
    f->action_state = FIGURE_ACTION_HOMELESS_CREATED;
    f->is_targetable = 1;
    f->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
    f->wait_ticks = 0;
    f->migrant_num_people = num_people;
    city_population_remove_homeless(num_people);
}

void figure_create_explosion_cloud(int x, int y, int size)
{
    int tile_offset = CLOUD_TILE_OFFSETS[size];
    int cc_offset = CLOUD_CC_OFFSETS[size];
    for (int i = 0; i < 16; i++) {
        struct figure_t *f = figure_create(FIGURE_EXPLOSION,
            x + tile_offset, y + tile_offset, DIR_0_TOP);
        if (f->id) {
            f->cross_country_x += cc_offset;
            f->cross_country_y += cc_offset;
            f->destination_x += CLOUD_DIRECTION[i].x;
            f->destination_y += CLOUD_DIRECTION[i].y;
            figure_movement_set_cross_country_direction(f,
                f->cross_country_x, f->cross_country_y,
                15 * f->destination_x + cc_offset,
                15 * f->destination_y + cc_offset, 0);
            f->speed_multiplier = CLOUD_SPEED[i];
        }
    }
}

void figure_create_missile(struct figure_t *shooter, struct map_point_t *target_tile, int type)
{
    struct figure_t *missile = figure_create(type, shooter->x, shooter->y, DIR_0_TOP);
    if (missile->id) {
        missile->missile_offset = type == FIGURE_BOLT ? 60 : 10;
        missile->building_id = shooter->id;
        missile->destination_x = target_tile->x;
        missile->destination_y = target_tile->y;
        figure_movement_set_cross_country_direction(missile, missile->cross_country_x, missile->cross_country_y, 15 * target_tile->x, 15 * target_tile->y, 1);
    }
}

int figure_trade_caravan_can_buy(struct figure_t *trader, int warehouse_id, int city_id)
{
    struct building_t *warehouse = &all_buildings[warehouse_id];
    if (warehouse->type != BUILDING_WAREHOUSE) {
        return 0;
    }
    if (trader->trader_amount_bought >= 8) {
        return 0;
    }
    struct building_t *space = warehouse;
    for (int i = 0; i < 8; i++) {
        space = &all_buildings[space->next_part_building_id];
        if (space->id > 0 && space->loads_stored > 0 &&
            can_export_resource_to_trade_city(city_id, space->subtype.warehouse_resource_id)) {
            return 1;
        }
    }
    return 0;
}

int figure_trade_caravan_can_sell(struct figure_t *trader, int warehouse_id, int city_id)
{
    struct building_t *warehouse = &all_buildings[warehouse_id];
    if (warehouse->type != BUILDING_WAREHOUSE) {
        return 0;
    }
    if (trader->loads_sold_or_carrying >= 8) {
        return 0;
    }
    struct building_storage_t *storage = building_storage_get(warehouse->storage_id);
    if (storage->empty_all) {
        return 0;
    }
    int num_importable = 0;
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        if (storage->resource_state[r] != BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
            if (can_import_resource_from_trade_city(city_id, r)) {
                num_importable++;
            }
        }
    }
    if (num_importable <= 0) {
        return 0;
    }
    int can_import = 0;
    int resource = city_data.trade.caravan_import_resource;
    if (storage->resource_state[resource] != BUILDING_STORAGE_STATE_NOT_ACCEPTING &&
        can_import_resource_from_trade_city(city_id, resource)) {
        can_import = 1;
    } else {
        for (int i = RESOURCE_WHEAT; i < RESOURCE_TYPES_MAX; i++) {
            resource = city_trade_next_caravan_import_resource();
            if (storage->resource_state[resource] != BUILDING_STORAGE_STATE_NOT_ACCEPTING &&
                    can_import_resource_from_trade_city(city_id, resource)) {
                can_import = 1;
                break;
            }
        }
    }
    if (can_import) {
        // at least one resource can be imported and accepted by this warehouse
        // check if warehouse can store any importable goods
        struct building_t *space = warehouse;
        for (int s = 0; s < 8; s++) {
            space = &all_buildings[space->next_part_building_id];
            if (space->id > 0 && space->loads_stored < 4) {
                if (!space->loads_stored) {
                    // empty space
                    return 1;
                }
                if (can_import_resource_from_trade_city(city_id, space->subtype.warehouse_resource_id)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int map_routing_wall_tile_in_radius(int x, int y, int radius, int *x_wall, int *y_wall)
{
    for (int i = 1; i <= radius; i++) {
        int wall_tile_in_radius = 0;
        int size = 1;
        int x_min, y_min, x_max, y_max;
        map_grid_get_area(x, y, size, i, &x_min, &y_min, &x_max, &y_max);
        for (int yy = y_min; yy <= y_max; yy++) {
            for (int xx = x_min; xx <= x_max; xx++) {
                if (map_routing_is_wall_passable(map_grid_offset(xx, yy))) {
                    *x_wall = xx;
                    *y_wall = yy;
                    wall_tile_in_radius = 1;
                    break;
                }
            }
        }
        if (wall_tile_in_radius) {
            return 1;
        }
    }
    return 0;
}

static int tower_sentry_shooting(struct figure_t *f)
{
    struct map_point_t tile = { -1, -1 };
    if (f->is_shooting) {
        f->attack_image_offset++;
        if (f->attack_image_offset > 100) {
            f->attack_image_offset = 0;
            f->is_shooting = 0;
        }
        f->image_id = image_group(GROUP_FIGURE_TOWER_SENTRY) + figure_image_direction(f) + 96 + 8 * TOWER_SENTRY_FIRING_OFFSETS[f->attack_image_offset / 2];
        return 1;
    }
    if (!f->in_building_wait_ticks && f->wait_ticks_missile > figure_properties[f->type].missile_delay && set_missile_target(f, &tile)) {
        f->progress_on_tile = 15; // align to wall
        f->direction = calc_missile_shooter_direction(f->x, f->y, tile.x, tile.y);
        figure_create_missile(f, &tile, figure_properties[f->type].missile_type);
        f->wait_ticks_missile = 0;
        f->is_shooting = 1;
        return 1;
    }
    return 0;
}

void figure_tower_sentry_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    f->height_adjusted_ticks = 10;
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
        figure_delete(f);
        return;
    }
    figure_image_increase_offset(f, 12);
    switch (f->action_state) {
        case FIGURE_ACTION_TOWER_SENTRY_AT_REST:
            f->is_targetable = 0;
            if (!f->is_military_trained) {
                struct map_point_t mil_acad_road = { 0 };
                set_destination__closest_building_of_type(f->building_id, BUILDING_MILITARY_ACADEMY, &mil_acad_road);
                if (mil_acad_road.x) {
                    struct map_point_t tower_road;
                    if (map_has_road_access(b->x, b->y, b->size, &tower_road)) {
                        f->terrain_usage = TERRAIN_USAGE_ROADS;
                        f->destination_x = tower_road.x;
                        f->destination_y = tower_road.y;
                        figure_movement_move_ticks(f, 1);
                        if (f->x == tower_road.x && f->y == tower_road.y) {
                            f->destination_x = mil_acad_road.x;
                            f->destination_y = mil_acad_road.y;
                            f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_MILITARY_ACADEMY;
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
                if (map_routing_wall_tile_in_radius(x, y, 6, &x_tile, &y_tile)) {
                    b->figure_roam_direction += 2;
                    if (b->figure_roam_direction > 6) b->figure_roam_direction = 0;
                    f->action_state = FIGURE_ACTION_TOWER_SENTRY_PATROLLING;
                    f->destination_x = x_tile;
                    f->destination_y = y_tile;
                    figure_route_remove(f);
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
                    if (map_routing_wall_tile_in_radius(x, y, 6, &x_tile, &y_tile)) {
                        f->action_state = FIGURE_ACTION_TOWER_SENTRY_PATROLLING;
                        f->destination_x = x_tile;
                        f->destination_y = y_tile;
                        figure_route_remove(f);
                    }
                }
            }
            break;
        case FIGURE_ACTION_TOWER_SENTRY_PATROLLING:
            f->terrain_usage = TERRAIN_USAGE_WALLS;
            f->wait_ticks_missile++;
            if (f->wait_ticks_missile > 250) {
                f->wait_ticks_missile = 250;
            }
            if (tower_sentry_shooting(f)) {
                return;
            } else {
                figure_movement_move_ticks(f, 1);
                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                    f->action_state = FIGURE_ACTION_TOWER_SENTRY_RETURNING;
                    f->destination_x = f->source_x;
                    f->destination_y = f->source_y;
                    figure_route_remove(f);
                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                    f->action_state = FIGURE_ACTION_TOWER_SENTRY_AT_REST;
                }
            }
            break;
        case FIGURE_ACTION_TOWER_SENTRY_RETURNING:
            f->terrain_usage = TERRAIN_USAGE_WALLS;
            f->wait_ticks_missile++;
            if (f->wait_ticks_missile > 250) {
                f->wait_ticks_missile = 250;
            }
            if (tower_sentry_shooting(f)) {
                return;
            } else {
                figure_movement_move_ticks(f, 1);
                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                    f->action_state = FIGURE_ACTION_TOWER_SENTRY_AT_REST;
                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                    figure_delete(f);
                    return;
                }
            }
            break;
        case FIGURE_ACTION_SOLDIER_GOING_TO_MILITARY_ACADEMY:
            f->is_targetable = 1;
            f->wait_ticks_missile++;
            if (f->wait_ticks_missile > 250) {
                f->wait_ticks_missile = 250;
            }
            if (tower_sentry_shooting(f)) {
                return;
            } else {
                figure_route_remove(f);
                f->terrain_usage = TERRAIN_USAGE_ROADS;
                f->is_military_trained = 1;
                f->height_adjusted_ticks = 0;
                figure_movement_move_ticks(f, f->speed_multiplier);
                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                    f->action_state = FIGURE_ACTION_TOWER_SENTRY_GOING_TO_TOWER;
                } else if (f->direction == DIR_FIGURE_REROUTE) {
                    figure_route_remove(f);
                } else if (f->direction == DIR_FIGURE_LOST) {
                    figure_delete(f);
                    return;
                }
            }
            break;
        case FIGURE_ACTION_TOWER_SENTRY_GOING_TO_TOWER:
            f->is_targetable = 1;
            f->wait_ticks_missile++;
            if (f->wait_ticks_missile > 250) {
                f->wait_ticks_missile = 250;
            }
            if (tower_sentry_shooting(f)) {
                return;
            } else {
                f->terrain_usage = TERRAIN_USAGE_ROADS;
                f->height_adjusted_ticks = 0;
                struct map_point_t road;
                if (map_has_road_access(b->x, b->y, b->size, &road)) {
                    f->destination_x = road.x;
                    f->destination_y = road.y;
                } else {
                    figure_delete(f);
                    return;
                }
                figure_movement_move_ticks(f, 1);
                if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                    map_figure_delete(f);
                    f->source_x = f->x = b->x;
                    f->source_y = f->y = b->y;
                    f->grid_offset = map_grid_offset(f->x, f->y);
                    map_figure_add(f);
                    f->action_state = FIGURE_ACTION_TOWER_SENTRY_AT_REST;
                    figure_route_remove(f);
                } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                    figure_delete(f);
                    return;
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
        struct figure_t *f = &figures[i];
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
            f->action_state = FIGURE_ACTION_TOWER_SENTRY_RETURNING;
            f->destination_x = f->source_x;
            f->destination_y = f->source_y;
        } else {
            // Teleport back to tower
            map_figure_delete(f);
            struct building_t *b = &all_buildings[f->building_id];
            f->source_x = f->x = b->x;
            f->source_y = f->y = b->y;
            f->grid_offset = map_grid_offset(f->x, f->y);
            map_figure_add(f);
            f->action_state = FIGURE_ACTION_TOWER_SENTRY_AT_REST;
            figure_route_remove(f);
        }
    }
}

void figure_create_flotsam(void)
{
    if (scenario.river_entry_point.x == -1 || scenario.river_entry_point.y == -1 || scenario.river_exit_point.x == -1 || scenario.river_exit_point.y == -1 || !scenario.flotsam_enabled) {
        return;
    }
    for (int i = 1; i < MAX_FIGURES; i++) {
        struct figure_t *f = &figures[i];
        if (figure_is_alive(f) && f->type == FIGURE_FLOTSAM) {
            figure_delete(f);
        }
    }

    for (int i = 0; i < 20; i++) {
        struct figure_t *f = figure_create(FIGURE_FLOTSAM, scenario.river_entry_point.x, scenario.river_entry_point.y, DIR_0_TOP);
        f->terrain_usage = TERRAIN_USAGE_ANY;
        f->action_state = FIGURE_ACTION_FLOTSAM_CREATED;
        f->resource_id = FLOTSAM_RESOURCE_IDS[i];
        f->wait_ticks = FLOTSAM_WAIT_TICKS[i];
    }
}
