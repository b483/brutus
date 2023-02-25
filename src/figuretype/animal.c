#include "animal.h"

#include "building/building.h"
#include "city/data_private.h"
#include "city/entertainment.h"
#include "city/sound.h"
#include "city/view.h"
#include "core/calc.h"
#include "core/image.h"
#include "core/random.h"
#include "figure/combat.h"
#include "figure/formation.h"
#include "figure/formation_layout.h"
#include "figure/image.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/point.h"
#include "map/terrain.h"
#include "scenario/data.h"
#include "scenario/map.h"
#include "sound/effect.h"

static const map_point SEAGULL_OFFSETS[] = {
    {0, 0}, {0, -2}, {-2, 0}, {1, 2}, {2, 0}, {-3, 1}, {4, -3}, {-2, 4}, {0, 0}
};

static const map_point HORSE_DESTINATION_1[] = {
    {2, 1}, {3, 1}, {4, 1}, {5, 1}, {6, 1}, {7, 1}, {8, 1}, {9, 1}, {10, 1}, {11, 1}, {12, 2},
    {12, 3}, {11, 3}, {10, 3}, {9, 3}, {8, 3}, {7, 3}, {6, 3}, {5, 3}, {4, 3}, {3, 3}, {2, 2}
};
static const map_point HORSE_DESTINATION_2[] = {
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
            figure *fish = figure_create(FIGURE_FISH_GULLS, scenario.fishing_points[i].x, scenario.fishing_points[i].y, DIR_0_TOP);
            fish->image_offset = random_byte() & 0x1f;
            fish->progress_on_tile = random_byte() & 7;
            figure_movement_set_cross_country_direction(fish,
                fish->cross_country_x, fish->cross_country_y,
                15 * fish->destination_x, 15 * fish->destination_y, 0);
        }
    }
}

void figure_create_herds(void)
{
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        if (scenario.herd_points[i].x > 0) {
            figure_type herd_type;
            int num_animals;
            switch (scenario.climate) {
                case CLIMATE_NORTHERN:
                    herd_type = FIGURE_WOLF;
                    num_animals = WOLF_PACK_SIZE;
                    break;
                case CLIMATE_CENTRAL:
                    herd_type = FIGURE_SHEEP;
                    num_animals = SHEEP_HERD_SIZE;
                    break;
                case CLIMATE_DESERT:
                    herd_type = FIGURE_ZEBRA;
                    num_animals = ZEBRA_HERD_SIZE;
                    break;
                default:
                    return;
            }

            // create herd formation
            struct formation_t *m = create_formation_type(herd_type);
            m->faction_id = 0;
            m->is_herd = 1;
            m->layout = FORMATION_HERD;
            m->max_figures = num_animals;
            m->x = scenario.herd_points[i].x;
            m->y = scenario.herd_points[i].y;
            m->wait_ticks = 24;

            // create animal figures and assign to formation
            for (int fig = 0; fig < num_animals; fig++) {
                figure *f = figure_create(herd_type, scenario.herd_points[i].x, scenario.herd_points[i].y, DIR_0_TOP);
                f->action_state = FIGURE_ACTION_196_HERD_ANIMAL_AT_REST;
                f->formation_id = m->id;
            }
        }
    }
}

void figure_seagulls_action(figure *f)
{
    f->terrain_usage = TERRAIN_USAGE_ANY;
    f->is_ghost = 0;
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

void figure_wolf_action(figure *f)
{
    struct formation_t *m = &formations[f->formation_id];
    f->terrain_usage = TERRAIN_USAGE_ANIMAL;
    f->use_cross_country = 0;
    f->is_ghost = 0;
    city_data.figure.animals++;
    figure_image_increase_offset(f, 12);

    switch (f->action_state) {
        case FIGURE_ACTION_149_CORPSE:
            figure_combat_handle_corpse(f);
            break;
        case FIGURE_ACTION_150_ATTACK:
            figure_combat_handle_attack(f);
            break;
        case FIGURE_ACTION_196_HERD_ANIMAL_AT_REST:
            if (m->missile_attack_formation_id || melee_unit__set_closest_target(f)) {
                f->action_state = FIGURE_ACTION_197_HERD_ANIMAL_MOVING;
                break;
            } else {
                if (m->wait_ticks == 0) {
                    f->action_state = FIGURE_ACTION_197_HERD_ANIMAL_MOVING;
                    break;
                } else {
                    // replenish wolf pack
                    if (m->num_figures < m->max_figures) {
                        m->herd_wolf_spawn_delay++;
                        int spawn_location_x = m->destination_x + formation_layout_position_x(FORMATION_HERD, WOLF_PACK_SIZE - 1);
                        int spawn_location_y = m->destination_y + formation_layout_position_y(FORMATION_HERD, WOLF_PACK_SIZE - 1);
                        if (m->herd_wolf_spawn_delay > 1500 && !map_terrain_is(map_grid_offset(spawn_location_x, spawn_location_y), TERRAIN_IMPASSABLE_HERD)) {
                            figure *wolf = figure_create(m->figure_type, spawn_location_x, spawn_location_y, f->direction);
                            wolf->action_state = FIGURE_ACTION_196_HERD_ANIMAL_AT_REST;
                            wolf->formation_id = m->id;
                            m->herd_wolf_spawn_delay = 0;
                        }
                    }
                }
            }
            break;
        case FIGURE_ACTION_197_HERD_ANIMAL_MOVING:
            figure *target = melee_unit__set_closest_target(f);
            if (target) {
                m->destination_x = target->x;
                m->destination_y = target->y;
                m->missile_attack_formation_id = 0;
                figure_movement_move_ticks(f, 2);
                random_generate_next();
                if (city_sound_update_march_wolf() && (random_byte() < 3)) {
                    sound_effect_play(SOUND_EFFECT_WOLF_HOWL);
                }
            } else if (m->missile_attack_formation_id) {
                figure_route_remove(f);
                struct formation_t *target_formation = &formations[m->missile_attack_formation_id];
                f->destination_x = target_formation->x_home;
                f->destination_y = target_formation->y_home;
                m->destination_x = target_formation->x_home;
                m->destination_y = target_formation->y_home;
                figure_movement_move_ticks(f, 2);
            } else {
                // roaming
                if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                    f->direction = f->previous_tile_direction;
                    f->action_state = FIGURE_ACTION_196_HERD_ANIMAL_AT_REST;
                    break;
                }
                if (f->direction == DIR_FIGURE_REROUTE) {
                    figure_route_remove(f);
                } else if (f->routing_path_current_tile > MAX_WOLF_ROAM_DISTANCE * 2) {
                    figure_route_remove(f);
                    m->destination_x = f->x;
                    m->destination_y = f->y;
                }
                f->destination_x = m->destination_x + formation_layout_position_x(FORMATION_HERD, f->index_in_formation);
                f->destination_y = m->destination_y + formation_layout_position_y(FORMATION_HERD, f->index_in_formation);
                figure_movement_move_ticks(f, 2);
            }
            break;
    }
    int dir = figure_image_direction(f);
    if (f->action_state == FIGURE_ACTION_149_CORPSE) {
        f->image_id = image_group(GROUP_FIGURE_WOLF) + 96 + figure_image_corpse_offset(f);
    } else if (f->action_state == FIGURE_ACTION_150_ATTACK) {
        f->image_id = image_group(GROUP_FIGURE_WOLF) + 104 + dir + 8 * (f->attack_image_offset / 4);
    } else if (f->action_state == FIGURE_ACTION_196_HERD_ANIMAL_AT_REST) {
        f->image_id = image_group(GROUP_FIGURE_WOLF) + 152 + dir;
    } else {
        f->image_id = image_group(GROUP_FIGURE_WOLF) + dir + 8 * f->image_offset;
    }
}

void figure_sheep_action(figure *f)
{
    f->terrain_usage = TERRAIN_USAGE_ANIMAL;
    f->use_cross_country = 0;
    f->is_ghost = 0;
    city_data.figure.animals++;
    figure_image_increase_offset(f, 6);

    switch (f->action_state) {
        case FIGURE_ACTION_150_ATTACK:
            figure_combat_handle_attack(f);
            break;
        case FIGURE_ACTION_149_CORPSE:
            figure_combat_handle_corpse(f);
            break;
        case FIGURE_ACTION_196_HERD_ANIMAL_AT_REST:
            f->wait_ticks++;
            if (formations[f->formation_id].wait_ticks == 0) {
                f->wait_ticks = f->id & 0x1f;
                f->action_state = FIGURE_ACTION_197_HERD_ANIMAL_MOVING;
            }
            break;
        case FIGURE_ACTION_197_HERD_ANIMAL_MOVING:
            // roaming
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                f->direction = f->previous_tile_direction;
                f->action_state = FIGURE_ACTION_196_HERD_ANIMAL_AT_REST;
                break;
            }
            if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->routing_path_current_tile > MAX_SHEEP_ROAM_DISTANCE * 2) {
                figure_route_remove(f);
                formations[f->formation_id].destination_x = f->x;
                formations[f->formation_id].destination_y = f->y;
            }
            f->destination_x = formations[f->formation_id].destination_x + formation_layout_position_x(FORMATION_HERD, f->index_in_formation);
            f->destination_y = formations[f->formation_id].destination_y + formation_layout_position_y(FORMATION_HERD, f->index_in_formation);
            figure_movement_move_ticks(f, 2);
            break;
    }
    int dir = figure_image_direction(f);
    if (f->action_state == FIGURE_ACTION_149_CORPSE) {
        f->image_id = image_group(GROUP_FIGURE_SHEEP) + 104 + figure_image_corpse_offset(f);
    } else if (f->action_state == FIGURE_ACTION_196_HERD_ANIMAL_AT_REST) {
        if (f->id & 3) {
            f->image_id = image_group(GROUP_FIGURE_SHEEP) + 48 + dir + 8 * SHEEP_IMAGE_OFFSETS[f->wait_ticks & 0x3f];
        } else {
            f->image_id = image_group(GROUP_FIGURE_SHEEP) + 96 + dir;
        }
    } else {
        f->image_id = image_group(GROUP_FIGURE_SHEEP) + dir + 8 * f->image_offset;
    }
}

void figure_zebra_action(figure *f)
{
    f->terrain_usage = TERRAIN_USAGE_ANIMAL;
    f->use_cross_country = 0;
    f->is_ghost = 0;
    city_data.figure.animals++;
    figure_image_increase_offset(f, 12);

    switch (f->action_state) {
        case FIGURE_ACTION_150_ATTACK:
            figure_combat_handle_attack(f);
            break;
        case FIGURE_ACTION_149_CORPSE:
            figure_combat_handle_corpse(f);
            break;
        case FIGURE_ACTION_196_HERD_ANIMAL_AT_REST:
            if (formations[f->formation_id].wait_ticks == 0) {
                f->action_state = FIGURE_ACTION_197_HERD_ANIMAL_MOVING;
            }
            break;
        case FIGURE_ACTION_197_HERD_ANIMAL_MOVING:
            // roaming
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                f->direction = f->previous_tile_direction;
                f->action_state = FIGURE_ACTION_196_HERD_ANIMAL_AT_REST;
                break;
            }
            if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->routing_path_current_tile > MAX_ZEBRA_ROAM_DISTANCE * 2) {
                figure_route_remove(f);
                formations[f->formation_id].destination_x = f->x;
                formations[f->formation_id].destination_y = f->y;
            }
            f->destination_x = formations[f->formation_id].destination_x + formation_layout_position_x(FORMATION_HERD, f->index_in_formation);
            f->destination_y = formations[f->formation_id].destination_y + formation_layout_position_y(FORMATION_HERD, f->index_in_formation);
            figure_movement_move_ticks(f, 2);
            break;
    }
    int dir = figure_image_direction(f);
    if (f->action_state == FIGURE_ACTION_149_CORPSE) {
        f->image_id = image_group(GROUP_FIGURE_ZEBRA) + 96 + figure_image_corpse_offset(f);
    } else if (f->action_state == FIGURE_ACTION_196_HERD_ANIMAL_AT_REST) {
        f->image_id = image_group(GROUP_FIGURE_ZEBRA) + dir;
    } else {
        f->image_id = image_group(GROUP_FIGURE_ZEBRA) + dir + 8 * f->image_offset;
    }
}

static void set_horse_destination(figure *f, int state)
{
    building *b = building_get(f->building_id);
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

void figure_hippodrome_horse_action(figure *f)
{
    city_entertainment_set_hippodrome_has_race(1);
    f->use_cross_country = 1;
    f->is_ghost = 0;
    figure_image_increase_offset(f, 8);

    switch (f->action_state) {
        case FIGURE_ACTION_200_HIPPODROME_HORSE_CREATED:
            f->image_offset = 0;
            f->wait_ticks_missile = 0;
            set_horse_destination(f, HORSE_CREATED);
            f->wait_ticks++;
            if (f->wait_ticks > 60 && f->resource_id == 0) {
                f->action_state = FIGURE_ACTION_201_HIPPODROME_HORSE_RACING;
                f->wait_ticks = 0;
            }
            f->wait_ticks++;
            if (f->wait_ticks > 20 && f->resource_id == 1) {
                f->action_state = FIGURE_ACTION_201_HIPPODROME_HORSE_RACING;
                f->wait_ticks = 0;
            }
            break;
        case FIGURE_ACTION_201_HIPPODROME_HORSE_RACING:
            f->direction = calc_general_direction(f->x, f->y, f->destination_x, f->destination_y);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->wait_ticks_missile++;
                if (f->wait_ticks_missile >= 22) {
                    f->wait_ticks_missile = 0;
                    f->leading_figure_id++;
                    if (f->leading_figure_id >= 6) {
                        f->wait_ticks = 0;
                        f->action_state = FIGURE_ACTION_202_HIPPODROME_HORSE_DONE;
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
            if (f->action_state != FIGURE_ACTION_202_HIPPODROME_HORSE_DONE) {
                figure_movement_move_ticks_cross_country(f, f->speed_multiplier);
            }
            break;
        case FIGURE_ACTION_202_HIPPODROME_HORSE_DONE:
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
                f->state = FIGURE_STATE_DEAD;
            }
            break;
    }

    int dir = figure_image_direction(f);
    if (f->resource_id == 0) {
        f->image_id = image_group(GROUP_FIGURE_HIPPODROME_HORSE_1) +
            dir + 8 * f->image_offset;
        f->cart_image_id = image_group(GROUP_FIGURE_HIPPODROME_CART_1) + dir;
    } else {
        f->image_id = image_group(GROUP_FIGURE_HIPPODROME_HORSE_2) +
            dir + 8 * f->image_offset;
        f->cart_image_id = image_group(GROUP_FIGURE_HIPPODROME_CART_2) + dir;
    }
    int cart_dir = (dir + 4) % 8;
    figure_image_set_cart_offset(f, cart_dir);
}

void figure_hippodrome_horse_reroute(void)
{
    if (!city_entertainment_hippodrome_has_race()) {
        return;
    }
    for (int i = 1; i < MAX_FIGURES; i++) {
        figure *f = figure_get(i);
        if (f->state == FIGURE_STATE_ALIVE && f->type == FIGURE_HIPPODROME_HORSES) {
            f->wait_ticks_missile = 0;
            set_horse_destination(f, HORSE_CREATED);
        }
    }
}
