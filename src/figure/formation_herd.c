#include "formation_herd.h"

#include "city/data_private.h"
#include "city/sound.h"
#include "core/random.h"
#include "figure/combat.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "figure/formation_layout.h"
#include "figure/formation_enemy.h"
#include "figure/route.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/routing.h"
#include "map/soldier_strength.h"
#include "map/terrain.h"

static int set_herd_roaming_destination(formation *m, int roam_distance)
{
    random_generate_next();
    int target_direction = random_byte() % 8;
    int target_tile_x, target_tile_y;
    switch (target_direction) {
        case DIR_0_TOP:
            target_tile_x = m->x;
            target_tile_y = m->y - roam_distance;
            break;
        case DIR_1_TOP_RIGHT:
            target_tile_x = m->x + roam_distance;
            target_tile_y = m->y - roam_distance;
            break;
        case DIR_2_RIGHT:
            target_tile_x = m->x + roam_distance;
            target_tile_y = m->y;
            break;
        case DIR_3_BOTTOM_RIGHT:
            target_tile_x = m->x + roam_distance;
            target_tile_y = m->y + roam_distance;
            break;
        case DIR_4_BOTTOM:
            target_tile_x = m->x;
            target_tile_y = m->y + roam_distance;
            break;
        case DIR_5_BOTTOM_LEFT:
            target_tile_x = m->x - roam_distance;
            target_tile_y = m->y + roam_distance;
            break;
        case DIR_6_LEFT:
            target_tile_x = m->x - roam_distance;
            target_tile_y = m->y;
            break;
        case DIR_7_TOP_LEFT:
            target_tile_x = m->x - roam_distance;
            target_tile_y = m->y - roam_distance;
            break;
        default:
            return 0;
    }
    if (target_tile_x <= 0 || target_tile_y <= 0) {
        return 0;
    } else if (target_tile_x >= map_grid_width() - 1) {
        target_tile_x = map_grid_width() - 2;
    } else if (target_tile_y >= map_grid_height() - 1) {
        target_tile_y = map_grid_height() - 2;
    }
    if (map_soldier_strength_get(map_grid_offset(target_tile_x, target_tile_y))) {
        return 0;
    }

    int base_offset = map_grid_offset(formation_layout_position_x(m->layout, 0), formation_layout_position_y(m->layout, 0));
    int figure_offsets[m->num_figures];
    figure_offsets[0] = 0;
    for (int i = 1; i < m->num_figures; i++) {
        figure_offsets[i] = map_grid_offset(formation_layout_position_x(m->layout, i), formation_layout_position_y(m->layout, i)) - base_offset;
    }
    for (int fig = 0; fig < m->num_figures; fig++) {
        int fig_target_loc_grid_offset = map_grid_offset(target_tile_x, target_tile_y) + figure_offsets[fig];
        if (!map_grid_is_valid_offset(fig_target_loc_grid_offset)) {
            return 0;
        }
        if (map_terrain_is(fig_target_loc_grid_offset, TERRAIN_IMPASSABLE_HERD)) {
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

void formation_herd_update(void)
{
    if (city_data.figure.animals <= 0) {
        return;
    }
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        formation *m = formation_get(i);
        if (m->in_use && m->is_herd && m->num_figures > 0) {
            random_generate_next();
            int random_factor = random_byte();
            int roam_distance;
            int roam_delay;
            switch (m->figure_type) {
                case FIGURE_WOLF:
                    roam_distance = (random_factor % MAX_WOLF_ROAM_DISTANCE) >= MAX_WOLF_ROAM_DISTANCE / 2 ? (random_factor % MAX_WOLF_ROAM_DISTANCE) : MAX_WOLF_ROAM_DISTANCE;
                    roam_delay = 12;
                    break;
                case FIGURE_SHEEP:
                    roam_distance = (random_factor % MAX_SHEEP_ROAM_DISTANCE) >= MAX_SHEEP_ROAM_DISTANCE / 2 ? (random_factor % MAX_SHEEP_ROAM_DISTANCE) : MAX_SHEEP_ROAM_DISTANCE;
                    roam_delay = 24;
                    break;
                case FIGURE_ZEBRA:
                    roam_distance = (random_factor % MAX_ZEBRA_ROAM_DISTANCE) >= MAX_ZEBRA_ROAM_DISTANCE / 2 ? (random_factor % MAX_ZEBRA_ROAM_DISTANCE) : MAX_ZEBRA_ROAM_DISTANCE;
                    roam_delay = 6;
                    break;
                default:
                    return;
            }
            m->wait_ticks++;
            if (m->wait_ticks > roam_delay) {
                if (set_herd_roaming_destination(m, roam_distance)) {
                    m->wait_ticks = 0;
                }
            }
        }
    }
}