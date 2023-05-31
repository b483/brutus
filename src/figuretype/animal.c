#include "animal.h"

#include "building/building.h"
#include "city/data.h"
#include "city/entertainment.h"
#include "city/view.h"
#include "core/calc.h"
#include "core/image.h"
#include "core/random.h"
#include "figure/combat.h"
#include "figure/formation_herd.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/point.h"
#include "map/terrain.h"
#include "scenario/map.h"
#include "sound/effect.h"

static const struct map_point_t SEAGULL_OFFSETS[] = {
    {0, 0}, {0, -2}, {-2, 0}, {1, 2}, {2, 0}, {-3, 1}, {4, -3}, {-2, 4}, {0, 0}
};

static const struct map_point_t HORSE_DESTINATION_1[] = {
    {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1}, {9, 1}, {10, 1}, {11, 1}, {12, 2},
    {12, 3}, {11, 3}, {10, 3}, {9, 3}, {8, 3}, {7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}, {2, 2}
};
static const struct map_point_t HORSE_DESTINATION_2[] = {
    {12, 3}, {11, 3}, {10, 3}, {9, 3}, {8, 3}, {7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}, {2, 2},
    {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1}, {9, 1}, {10, 1}, {11, 1}, {12, 2}
};

static const int SHEEP_IMAGE_OFFSETS[] = {
    0,  0,  1,  1,  2,  2,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,
    3,  3,  3,  3,  4,  4,  5,  5, -1, -1, -1, -1, -1, -1, -1, -1
};

enum {
    HORSE_CREATED = 0,
    HORSE_RACING = 1,
    HORSE_FINISHED = 2
};

void figure_create_fishing_points(void)
{
    for (int i = 0; i < MAX_FISH_POINTS; i++) {
        if (scenario.fishing_points[i].x > 0) {
            random_generate_next();
            struct figure_t *fish = figure_create(FIGURE_FISH_GULLS, scenario.fishing_points[i].x, scenario.fishing_points[i].y, DIR_0_TOP);
            fish->terrain_usage = TERRAIN_USAGE_ANY;
            fish->image_offset = random_byte() & 0x1f;
            fish->progress_on_tile = random_byte() & 7;
            figure_movement_set_cross_country_direction(fish,
                fish->cross_country_x, fish->cross_country_y,
                15 * fish->destination_x, 15 * fish->destination_y, 0);
        }
    }
}

void figure_seagulls_action(struct figure_t *f)
{
    f->use_cross_country = 1;
    if (!(f->image_offset & 3) && figure_movement_move_ticks_cross_country(f, 1)) {
        f->progress_on_tile++;
        if (f->progress_on_tile > 8) {
            f->progress_on_tile = 0;
        }
        figure_movement_set_cross_country_destination(f,
            f->source_x + SEAGULL_OFFSETS[f->progress_on_tile].x,
            f->source_y + SEAGULL_OFFSETS[f->progress_on_tile].y);
    }
    if (f->id & 1) {
        figure_image_increase_offset(f, 54);
        f->image_id = image_group(GROUP_FIGURE_SEAGULLS) + f->image_offset / 3;
    } else {
        figure_image_increase_offset(f, 72);
        f->image_id = image_group(GROUP_FIGURE_SEAGULLS) + 18 + f->image_offset / 3;
    }
}

void figure_wolf_action(struct figure_t *f)
{
    struct formation_t *m = &herd_formations[f->formation_id];
    figure_image_increase_offset(f, 12);

    switch (f->action_state) {
        case FIGURE_ACTION_HERD_ANIMAL_AT_REST:
            // replenish wolf pack
            if (m->num_figures < m->max_figures) {
                m->wolf_spawn_delay++;
                if (m->wolf_spawn_delay > 1500) {
                    int spawn_location_x = m->destination_x + HERD_FORMATION_LAYOUT_POSITION_X_OFFSETS[WOLF_PACK_SIZE - 1];
                    int spawn_location_y = m->destination_y + HERD_FORMATION_LAYOUT_POSITION_Y_OFFSETS[WOLF_PACK_SIZE - 1];
                    if (!map_terrain_is(map_grid_offset(spawn_location_x, spawn_location_y), TERRAIN_IMPASSABLE)) {
                        struct figure_t *wolf = figure_create(m->figure_type, spawn_location_x, spawn_location_y, f->direction);
                        wolf->action_state = FIGURE_ACTION_HERD_ANIMAL_AT_REST;
                        wolf->formation_id = m->id;
                        m->wolf_spawn_delay = 0;
                    }
                }
            }
            break;
        case FIGURE_ACTION_HERD_ANIMAL_MOVING:
            struct figure_t *target = melee_unit__set_closest_target(f);
            if (target) {
                figure_movement_move_ticks(f, 2);
                random_generate_next();
                if (random_byte() < 3) {
                    sound_effect_play(SOUND_EFFECT_WOLF_HOWL);
                }
                break;
            } else {
                figure_movement_move_ticks(f, 2);
                if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                    f->direction = f->previous_tile_direction;
                    f->action_state = FIGURE_ACTION_HERD_ANIMAL_AT_REST;
                    break;
                } else if (f->direction == DIR_FIGURE_REROUTE) {
                    figure_route_remove(f);
                    break;
                } else if (f->routing_path_current_tile > MAX_WOLF_ROAM_DISTANCE * 2) {
                    figure_route_remove(f);
                    m->destination_x = f->x;
                    m->destination_y = f->y;
                }
            }
            break;
    }
    int dir = figure_image_direction(f);
    if (f->action_state == FIGURE_ACTION_HERD_ANIMAL_AT_REST) {
        f->image_id = image_group(GROUP_FIGURE_WOLF) + 152 + dir;
    } else {
        f->image_id = image_group(GROUP_FIGURE_WOLF) + dir + 8 * f->image_offset;
    }
}

void figure_sheep_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 6);
    struct formation_t *m = &herd_formations[f->formation_id];
    switch (f->action_state) {
        case FIGURE_ACTION_HERD_ANIMAL_AT_REST:
            f->wait_ticks++;
            break;
        case FIGURE_ACTION_HERD_ANIMAL_MOVING:
            figure_movement_move_ticks(f, 2);
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                f->direction = f->previous_tile_direction;
                f->action_state = FIGURE_ACTION_HERD_ANIMAL_AT_REST;
                f->wait_ticks = f->id & 0x1f;
                break;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
                break;
            } else if (f->routing_path_current_tile > MAX_SHEEP_ROAM_DISTANCE * 2) {
                figure_route_remove(f);
                m->destination_x = f->x;
                m->destination_y = f->y;
            }
            break;
    }
    int dir = figure_image_direction(f);
    if (f->action_state == FIGURE_ACTION_HERD_ANIMAL_AT_REST) {
        if (f->id & 3) {
            f->image_id = image_group(GROUP_FIGURE_SHEEP) + 48 + dir + 8 * SHEEP_IMAGE_OFFSETS[f->wait_ticks & 0x3f];
        } else {
            f->image_id = image_group(GROUP_FIGURE_SHEEP) + 96 + dir;
        }
    } else {
        f->image_id = image_group(GROUP_FIGURE_SHEEP) + dir + 8 * f->image_offset;
    }
}

void figure_zebra_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    struct formation_t *m = &herd_formations[f->formation_id];
    switch (f->action_state) {
        case FIGURE_ACTION_HERD_ANIMAL_AT_REST:
            break;
        case FIGURE_ACTION_HERD_ANIMAL_MOVING:
            figure_movement_move_ticks(f, 2);
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                f->direction = f->previous_tile_direction;
                f->action_state = FIGURE_ACTION_HERD_ANIMAL_AT_REST;
                break;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
                break;
            } else if (f->routing_path_current_tile > MAX_SHEEP_ROAM_DISTANCE * 2) {
                figure_route_remove(f);
                m->destination_x = f->x;
                m->destination_y = f->y;
            }
            break;
    }
    int dir = figure_image_direction(f);
    if (f->action_state == FIGURE_ACTION_HERD_ANIMAL_AT_REST) {
        f->image_id = image_group(GROUP_FIGURE_ZEBRA) + dir;
    } else {
        f->image_id = image_group(GROUP_FIGURE_ZEBRA) + dir + 8 * f->image_offset;
    }
}

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
