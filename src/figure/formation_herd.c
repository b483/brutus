#include "formation_herd.h"

#include "city/data_private.h"
#include "figure/combat.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "figure/route.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/routing.h"
#include "map/terrain.h"

#include <stdlib.h>

struct formation_t herd_formations[MAX_HERD_POINTS];

const int HERD_FORMATION_LAYOUT_POSITION_X_OFFSETS[MAX_FORMATION_FIGURES] = {
    0, 2, -1, 1, 1, -1, 3, -2, 0, -4, -1, 0, 1, 4, 2, -5
};

const int HERD_FORMATION_LAYOUT_POSITION_Y_OFFSETS[MAX_FORMATION_FIGURES] = {
    0, 1, -1, 1, 0, 1, 1, -1, 2, 0, 3, 5, 4, 0, 3, 2
};

void create_herds(void)
{
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        if (scenario.herd_points[i].x > -1) {
            int herd_type;
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
            struct formation_t *m = &herd_formations[i];
            m->in_use = 1;
            m->figure_type = herd_type;
            m->max_figures = num_animals;
            m->wait_ticks_movement = 24;

            // create animal figures and assign to formation
            for (int fig = 0; fig < num_animals; fig++) {
                struct figure_t *f = figure_create(herd_type,
                    scenario.herd_points[i].x + HERD_FORMATION_LAYOUT_POSITION_X_OFFSETS[fig],
                    scenario.herd_points[i].y + HERD_FORMATION_LAYOUT_POSITION_Y_OFFSETS[fig],
                    DIR_0_TOP);
                f->terrain_usage = TERRAIN_USAGE_ANIMAL;
                f->action_state = FIGURE_ACTION_HERD_ANIMAL_AT_REST;
                f->formation_id = m->id;
                add_figure_to_formation(f, m);
                city_data.figure.animals++;
            }
        }
    }
}

static int set_herd_roaming_destination(struct formation_t *m, int roam_distance)
{
    int target_tile_x, target_tile_y;
    switch (rand() % 8) {
        case DIR_0_TOP:
            target_tile_x = figures[m->figures[0]].x;
            target_tile_y = figures[m->figures[0]].y - roam_distance;
            break;
        case DIR_1_TOP_RIGHT:
            target_tile_x = figures[m->figures[0]].x + roam_distance;
            target_tile_y = figures[m->figures[0]].y - roam_distance;
            break;
        case DIR_2_RIGHT:
            target_tile_x = figures[m->figures[0]].x + roam_distance;
            target_tile_y = figures[m->figures[0]].y;
            break;
        case DIR_3_BOTTOM_RIGHT:
            target_tile_x = figures[m->figures[0]].x + roam_distance;
            target_tile_y = figures[m->figures[0]].y + roam_distance;
            break;
        case DIR_4_BOTTOM:
            target_tile_x = figures[m->figures[0]].x;
            target_tile_y = figures[m->figures[0]].y + roam_distance;
            break;
        case DIR_5_BOTTOM_LEFT:
            target_tile_x = figures[m->figures[0]].x - roam_distance;
            target_tile_y = figures[m->figures[0]].y + roam_distance;
            break;
        case DIR_6_LEFT:
            target_tile_x = figures[m->figures[0]].x - roam_distance;
            target_tile_y = figures[m->figures[0]].y;
            break;
        case DIR_7_TOP_LEFT:
            target_tile_x = figures[m->figures[0]].x - roam_distance;
            target_tile_y = figures[m->figures[0]].y - roam_distance;
            break;
        default:
            return 0;
    }
    if (target_tile_x <= 0 || target_tile_y <= 0) {
        return 0;
    } else if (target_tile_x >= map_data.width - 1) {
        target_tile_x = map_data.width - 2;
    } else if (target_tile_y >= map_data.height - 1) {
        target_tile_y = map_data.height - 2;
    }

    for (int fig = 0; fig < m->num_figures; fig++) {
        int fig_target_loc_grid_offset = map_grid_offset(target_tile_x + HERD_FORMATION_LAYOUT_POSITION_X_OFFSETS[fig], target_tile_y + HERD_FORMATION_LAYOUT_POSITION_Y_OFFSETS[fig]);
        if (!map_grid_is_valid_offset(fig_target_loc_grid_offset)) {
            return 0;
        }
        if (map_terrain_is(fig_target_loc_grid_offset, TERRAIN_IMPASSABLE)) {
            return 0;
        }
        if (map_has_figure_at(fig_target_loc_grid_offset)) {
            return 0;
        }
    }
    m->destination_x = target_tile_x;
    m->destination_y = target_tile_y;
    return 1;
}

static void set_herd_animals_in_motion(struct formation_t *m)
{
    for (int j = 0; j < m->num_figures; j++) {
        struct figure_t *f = &figures[m->figures[j]];
        if (f->action_state != FIGURE_ACTION_CORPSE && f->action_state != FIGURE_ACTION_ATTACK) {
            f->destination_x = m->destination_x + HERD_FORMATION_LAYOUT_POSITION_X_OFFSETS[f->index_in_formation];
            f->destination_y = m->destination_y + HERD_FORMATION_LAYOUT_POSITION_Y_OFFSETS[f->index_in_formation];
            f->action_state = FIGURE_ACTION_HERD_ANIMAL_MOVING;
        }
    }
}

static void set_herd_formation_in_motion(struct formation_t *m, int roam_distance)
{
    if (m->figure_type == FIGURE_WOLF) {
        // wolves hunt nearby units
        for (int n = 0; n < m->num_figures; n++) {
            struct figure_t *f = &figures[m->figures[n]];
            struct figure_t *target_unit = melee_unit__set_closest_target(f);
            if (target_unit) {
                m->destination_x = target_unit->x;
                m->destination_y = target_unit->y;
                set_herd_animals_in_motion(m);
                return;
            }
        }
    }
    if (set_herd_roaming_destination(m, roam_distance)) {
        for (int j = 0; j < m->num_figures; j++) {
            struct figure_t *f = &figures[m->figures[j]];
            if (f->type == FIGURE_SHEEP) {
                f->wait_ticks = f->id & 0x1f;
            }
            set_herd_animals_in_motion(m);
        }
        m->wait_ticks_movement = 0;
    }
}

void update_herd_formations(void)
{
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        if (herd_formations[i].in_use && herd_formations[i].num_figures) {
            struct formation_t *m = &herd_formations[i];
            int random_factor = rand();
            int roam_distance;
            int roam_delay;
            switch (m->figure_type) {
                case FIGURE_WOLF:
                    roam_distance = (random_factor % MAX_WOLF_ROAM_DISTANCE) >= MAX_WOLF_ROAM_DISTANCE / 2 ? (random_factor % MAX_WOLF_ROAM_DISTANCE) : MAX_WOLF_ROAM_DISTANCE;
                    roam_delay = WOLF_PACK_ROAM_DELAY;
                    break;
                case FIGURE_SHEEP:
                    roam_distance = (random_factor % MAX_SHEEP_ROAM_DISTANCE) >= MAX_SHEEP_ROAM_DISTANCE / 2 ? (random_factor % MAX_SHEEP_ROAM_DISTANCE) : MAX_SHEEP_ROAM_DISTANCE;
                    roam_delay = SHEEP_HERD_ROAM_DELAY;
                    break;
                case FIGURE_ZEBRA:
                    roam_distance = (random_factor % MAX_ZEBRA_ROAM_DISTANCE) >= MAX_ZEBRA_ROAM_DISTANCE / 2 ? (random_factor % MAX_ZEBRA_ROAM_DISTANCE) : MAX_ZEBRA_ROAM_DISTANCE;
                    roam_delay = ZEBRA_HERD_ROAM_DELAY;
                    break;
                default:
                    return;
            }

            decrease_formation_combat_counters(m);

            // being shot at or attacked prompts an immediate response
            if (m->missile_attack_timeout) {
                set_herd_formation_in_motion(m, roam_distance);
                return;
            }
            for (int j = 0; j < m->num_figures; j++) {
                struct figure_t *f = &figures[m->figures[j]];
                if (f->action_state == FIGURE_ACTION_ATTACK) {
                    set_herd_formation_in_motion(m, roam_distance);
                    return;
                }
            }

            m->wait_ticks_movement++;
            if (m->wait_ticks_movement > roam_delay) {
                set_herd_formation_in_motion(m, roam_distance);
            }
        }
    }
}

void herd_formations_save_state(buffer *buf)
{
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        formation_save_state(buf, &herd_formations[i]);
    }
}

void herd_formations_load_state(buffer *buf)
{
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        herd_formations[i].id = i;
        formation_load_state(buf, &herd_formations[i]);
    }
}