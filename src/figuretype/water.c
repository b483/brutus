#include "water.h"

#include "building/building.h"
#include "city/gods.h"
#include "city/message.h"
#include "core/calc.h"
#include "core/image.h"
#include "core/random.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/water.h"
#include "scenario/data.h"
#include "scenario/map.h"

static const int FLOTSAM_RESOURCE_IDS[] = {
    3, 1, 3, 2, 1, 3, 2, 3, 2, 1, 3, 3, 2, 3, 3, 3, 1, 2, 0, 1
};
static const int FLOTSAM_WAIT_TICKS[] = {
    10, 50, 100, 130, 200, 250, 400, 430, 500, 600, 70, 750, 820, 830, 900, 980, 1010, 1030, 1200, 1300
};

static const int FLOTSAM_TYPE_0[] = { 0, 1, 2, 3, 4, 4, 4, 3, 2, 1, 0, 0 };
static const int FLOTSAM_TYPE_12[] = {
    0, 1, 1, 2, 2, 3, 3, 4, 4, 4, 3, 2, 1, 0, 0, 1, 1, 2, 2, 1, 1, 0, 0, 0
};
static const int FLOTSAM_TYPE_3[] = {
    0, 0, 1, 1, 2, 2, 3, 3, 4, 4, -1, -1,
    -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1
};

void figure_create_flotsam(void)
{
    if (!scenario_map_has_river_entry() || !scenario_map_has_river_exit() || !scenario.flotsam_enabled) {
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

void figure_flotsam_action(struct figure_t *f)
{
    f->is_invisible = 0;
    switch (f->action_state) {
        case FIGURE_ACTION_FLOTSAM_CREATED:
            f->is_invisible = 1;
            f->wait_ticks--;
            if (f->wait_ticks <= 0) {
                f->action_state = FIGURE_ACTION_FLOTSAM_FLOATING;
                f->wait_ticks = 0;
                if (!f->resource_id && city_god_neptune_create_shipwreck_flotsam()) {
                    f->min_max_seen = 1;
                }
                f->destination_x = scenario.river_exit_point.x;
                f->destination_y = scenario.river_exit_point.y;
            }
            break;
        case FIGURE_ACTION_FLOTSAM_FLOATING:
            if (f->flotsam_visible) {
                f->flotsam_visible = 0;
            } else {
                f->flotsam_visible = 1;
                f->wait_ticks++;
                figure_movement_move_ticks(f, 1);
                f->is_invisible = 0;
                f->height_adjusted_ticks = 0;
                if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                    f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                    f->action_state = FIGURE_ACTION_FLOTSAM_OFF_MAP;
                }
            }
            break;
        case FIGURE_ACTION_FLOTSAM_OFF_MAP:
            f->is_invisible = 1;
            f->min_max_seen = 0;
            f->action_state = FIGURE_ACTION_FLOTSAM_CREATED;
            if (f->wait_ticks >= 400) {
                f->wait_ticks = random_byte() & 7;
            } else if (f->wait_ticks >= 200) {
                f->wait_ticks = 50 + (random_byte() & 0xf);
            } else if (f->wait_ticks >= 100) {
                f->wait_ticks = 100 + (random_byte() & 0x1f);
            } else if (f->wait_ticks >= 50) {
                f->wait_ticks = 200 + (random_byte() & 0x3f);
            } else {
                f->wait_ticks = 300 + random_byte();
            }
            map_figure_delete(f);
            f->x = scenario.river_entry_point.x;
            f->y = scenario.river_entry_point.y;
            f->grid_offset = map_grid_offset(f->x, f->y);
            f->cross_country_x = 15 * f->x;
            f->cross_country_y = 15 * f->y;
            break;
    }
    if (f->resource_id == 0) {
        figure_image_increase_offset(f, 12);
        if (f->min_max_seen) {
            f->image_id = image_group(GROUP_FIGURE_FLOTSAM_SHEEP) + FLOTSAM_TYPE_0[f->image_offset];
        } else {
            f->image_id = image_group(GROUP_FIGURE_FLOTSAM_0) + FLOTSAM_TYPE_0[f->image_offset];
        }
    } else if (f->resource_id == 1) {
        figure_image_increase_offset(f, 24);
        f->image_id = image_group(GROUP_FIGURE_FLOTSAM_1) + FLOTSAM_TYPE_12[f->image_offset];
    } else if (f->resource_id == 2) {
        figure_image_increase_offset(f, 24);
        f->image_id = image_group(GROUP_FIGURE_FLOTSAM_2) + FLOTSAM_TYPE_12[f->image_offset];
    } else if (f->resource_id == 3) {
        figure_image_increase_offset(f, 24);
        if (FLOTSAM_TYPE_3[f->image_offset] == -1) {
            f->image_id = 0;
        } else {
            f->image_id = image_group(GROUP_FIGURE_FLOTSAM_3) + FLOTSAM_TYPE_3[f->image_offset];
        }
    }
}

void figure_shipwreck_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 128);
    if (f->wait_ticks < 1000) {
        map_figure_delete(f);
        map_point tile;
        if (map_water_find_shipwreck_tile(f, &tile)) {
            f->x = tile.x;
            f->y = tile.y;
            f->grid_offset = map_grid_offset(f->x, f->y);
            f->cross_country_x = 15 * f->x + 7;
            f->cross_country_y = 15 * f->y + 7;
        }
        map_figure_add(f);
        f->wait_ticks = 1000;
    }
    f->wait_ticks++;
    if (f->wait_ticks > 2000) {
        figure_delete(f);
        return;
    }
    f->image_id = image_group(GROUP_FIGURE_SHIPWRECK) + f->image_offset / 16;
}

void figure_fishing_boat_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    if (b->state != BUILDING_STATE_IN_USE) {
        figure_delete(f);
        return;
    }
    if (f->action_state != FIGURE_ACTION_FISHING_BOAT_CREATED && b->data.industry.fishing_boat_id != f->id) {
        map_point tile;
        b = &all_buildings[map_water_get_wharf_for_new_fishing_boat(f, &tile)];
        if (b->id) {
            f->building_id = b->id;
            b->data.industry.fishing_boat_id = f->id;
            f->action_state = FIGURE_ACTION_FISHING_BOAT_GOING_TO_WHARF;
            f->destination_x = tile.x;
            f->destination_y = tile.y;
            f->source_x = tile.x;
            f->source_y = tile.y;
            figure_route_remove(f);
        } else {
            figure_delete(f);
            return;
        }
    }
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    switch (f->action_state) {
        case FIGURE_ACTION_FISHING_BOAT_CREATED:
            f->wait_ticks++;
            if (f->wait_ticks >= 50) {
                f->wait_ticks = 0;
                map_point tile;
                int wharf_id = map_water_get_wharf_for_new_fishing_boat(f, &tile);
                if (wharf_id) {
                    b->figure_id = 0; // remove from original building
                    f->building_id = wharf_id;
                    all_buildings[wharf_id].data.industry.fishing_boat_id = f->id;
                    f->action_state = FIGURE_ACTION_FISHING_BOAT_GOING_TO_WHARF;
                    f->destination_x = tile.x;
                    f->destination_y = tile.y;
                    f->source_x = tile.x;
                    f->source_y = tile.y;
                    figure_route_remove(f);
                }
            }
            break;
        case FIGURE_ACTION_FISHING_BOAT_GOING_TO_FISH:
            figure_movement_move_ticks(f, 1);
            f->height_adjusted_ticks = 0;
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                map_point tile;
                if (map_water_find_alternative_fishing_boat_tile(f, &tile)) {
                    figure_route_remove(f);
                    f->destination_x = tile.x;
                    f->destination_y = tile.y;
                    f->direction = f->previous_tile_direction;
                } else {
                    f->action_state = FIGURE_ACTION_FISHING_BOAT_FISHING;
                    f->wait_ticks = 0;
                }
            } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                f->action_state = FIGURE_ACTION_FISHING_BOAT_AT_WHARF;
                f->destination_x = f->source_x;
                f->destination_y = f->source_y;
            }
            break;
        case FIGURE_ACTION_FISHING_BOAT_FISHING:
            f->wait_ticks++;
            if (f->wait_ticks >= 200) {
                f->wait_ticks = 0;
                f->action_state = FIGURE_ACTION_FISHING_BOAT_RETURNING_WITH_FISH;
                f->destination_x = f->source_x;
                f->destination_y = f->source_y;
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_FISHING_BOAT_GOING_TO_WHARF:
            figure_movement_move_ticks(f, 1);
            f->height_adjusted_ticks = 0;
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_FISHING_BOAT_AT_WHARF;
                f->wait_ticks = 0;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                // cannot reach grounds
                city_message_post_with_message_delay(MESSAGE_CAT_FISHING_BLOCKED, 1, MESSAGE_FISHING_BOAT_BLOCKED, 12);
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_FISHING_BOAT_AT_WHARF:
        {
            int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].laborers);
            int max_wait_ticks = 5 * (102 - pct_workers);
            if (b->data.industry.has_fish > 0) {
                pct_workers = 0;
            }
            if (pct_workers > 0) {
                f->wait_ticks++;
                if (f->wait_ticks >= max_wait_ticks) {
                    f->wait_ticks = 0;
                    map_point tile;
                    if (scenario_map_closest_fishing_point(f->x, f->y, &tile)) {
                        f->action_state = FIGURE_ACTION_FISHING_BOAT_GOING_TO_FISH;
                        f->destination_x = tile.x;
                        f->destination_y = tile.y;
                        figure_route_remove(f);
                    }
                }
            }
        }
        break;
        case FIGURE_ACTION_FISHING_BOAT_RETURNING_WITH_FISH:
            figure_movement_move_ticks(f, 1);
            f->height_adjusted_ticks = 0;
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_FISHING_BOAT_AT_WHARF;
                f->wait_ticks = 0;
                b->figure_spawn_delay = 1;
                b->data.industry.has_fish++;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
    }
    int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);

    if (f->action_state == FIGURE_ACTION_FISHING_BOAT_FISHING) {
        f->image_id = image_group(GROUP_FIGURE_SHIP) + dir + 16;
    } else {
        f->image_id = image_group(GROUP_FIGURE_SHIP) + dir + 8;
    }
}

void figure_sink_all_ships(void)
{
    for (int i = 1; i < MAX_FIGURES; i++) {
        struct figure_t *f = &figures[i];
        if (!figure_is_alive(f)) {
            continue;
        }
        if (f->type == FIGURE_TRADE_SHIP) {
            all_buildings[f->destination_building_id].data.dock.trade_ship_id = 0;
        } else if (f->type == FIGURE_FISHING_BOAT) {
            all_buildings[f->building_id].data.industry.fishing_boat_id = 0;
        } else {
            continue;
        }
        f->building_id = 0;
        f->type = FIGURE_SHIPWRECK;
        f->wait_ticks = 0;
    }
}
