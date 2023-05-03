#include "formation_enemy.h"

#include "building/building.h"
#include "city/buildings.h"
#include "city/data_private.h"
#include "city/gods.h"
#include "city/message.h"
#include "core/calc.h"
#include "core/random.h"
#include "figure/combat.h"
#include "figure/enemy_army.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "figure/route.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/routing.h"
#include "map/routing_path.h"
#include "map/soldier_strength.h"
#include "map/terrain.h"
#include "sound/effect.h"

static const int ENEMY_ATTACK_PRIORITY[4][100] = {
    {
        BUILDING_GRANARY, BUILDING_WAREHOUSE, BUILDING_MARKET,
        BUILDING_WHEAT_FARM, BUILDING_VEGETABLE_FARM, BUILDING_FRUIT_FARM,
        BUILDING_OLIVE_FARM, BUILDING_VINES_FARM, BUILDING_PIG_FARM,
        0
    },
    {
        BUILDING_SENATE, BUILDING_FORUM,
        0
    },
    {
        BUILDING_TRIUMPHAL_ARCH, BUILDING_SENATE, BUILDING_GOVERNORS_PALACE, BUILDING_GOVERNORS_VILLA, BUILDING_GOVERNORS_HOUSE,
        BUILDING_HIPPODROME,
        0
    },
    {
        BUILDING_BARRACKS, BUILDING_MILITARY_ACADEMY, BUILDING_PREFECTURE, 0
    }
};

static const int RIOTER_ATTACK_PRIORITY[100] = {
    BUILDING_GOVERNORS_PALACE, BUILDING_GOVERNORS_VILLA, BUILDING_GOVERNORS_HOUSE,
    BUILDING_SENATE, BUILDING_FORUM,
    BUILDING_HOUSE_LUXURY_PALACE, BUILDING_HOUSE_LARGE_PALACE, BUILDING_HOUSE_MEDIUM_PALACE, BUILDING_HOUSE_SMALL_PALACE,
    BUILDING_HOUSE_GRAND_VILLA, BUILDING_HOUSE_LARGE_VILLA, BUILDING_HOUSE_MEDIUM_VILLA, BUILDING_HOUSE_SMALL_VILLA,
    BUILDING_PREFECTURE,
    BUILDING_ACADEMY, BUILDING_LIBRARY,
    BUILDING_BATHHOUSE,
    BUILDING_HIPPODROME, BUILDING_COLOSSEUM, BUILDING_AMPHITHEATER, BUILDING_THEATER, BUILDING_GLADIATOR_SCHOOL, BUILDING_ACTOR_COLONY, BUILDING_CHARIOT_MAKER, BUILDING_LION_HOUSE,
    BUILDING_LARGE_TEMPLE_CERES, BUILDING_LARGE_TEMPLE_NEPTUNE, BUILDING_LARGE_TEMPLE_MERCURY, BUILDING_LARGE_TEMPLE_MARS, BUILDING_LARGE_TEMPLE_VENUS,
    BUILDING_HOSPITAL,
    BUILDING_HOUSE_GRAND_INSULA, BUILDING_HOUSE_LARGE_INSULA, BUILDING_HOUSE_MEDIUM_INSULA, BUILDING_HOUSE_SMALL_INSULA,
    BUILDING_WINE_WORKSHOP, BUILDING_OIL_WORKSHOP, BUILDING_WEAPONS_WORKSHOP, BUILDING_FURNITURE_WORKSHOP, BUILDING_POTTERY_WORKSHOP,
    BUILDING_GRANARY, BUILDING_WAREHOUSE, BUILDING_MARKET, BUILDING_DOCK,
    BUILDING_ENGINEERS_POST,
    BUILDING_HOUSE_LARGE_CASA, BUILDING_HOUSE_SMALL_CASA, BUILDING_HOUSE_LARGE_HOVEL, BUILDING_HOUSE_SMALL_HOVEL,
    BUILDING_CLAY_PIT, BUILDING_MARBLE_QUARRY, BUILDING_IRON_MINE, BUILDING_TIMBER_YARD,
    BUILDING_WHEAT_FARM, BUILDING_VEGETABLE_FARM, BUILDING_FRUIT_FARM, BUILDING_OLIVE_FARM, BUILDING_VINES_FARM, BUILDING_PIG_FARM,
    BUILDING_SMALL_TEMPLE_CERES, BUILDING_SMALL_TEMPLE_NEPTUNE, BUILDING_SMALL_TEMPLE_MERCURY, BUILDING_SMALL_TEMPLE_MARS, BUILDING_SMALL_TEMPLE_VENUS, BUILDING_ORACLE,
    BUILDING_RESERVOIR, BUILDING_FOUNTAIN, BUILDING_WELL,
    BUILDING_HOUSE_LARGE_SHACK, BUILDING_HOUSE_SMALL_SHACK, BUILDING_HOUSE_LARGE_TENT, BUILDING_HOUSE_SMALL_TENT,
    0
};

static const int LAYOUT_ORIENTATION_OFFSETS[13][4][40] = {
    {
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, 8, 3, 8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, 8, -3, 8, 3, 0},
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, -8, 3, -8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, -8, -3, -8, 3, 0},
    },
    {
        {0, 0, -6, 0, 6, 0, -6, 2, 6, 2, -2, 4, 4, 6, 0},
        {0, 0, 0, -6, 0, 6, 2, -6, 2, 6, 4, -2, 6, 4, 0},
        {0, 0, -6, 0, 6, 0, -6, -2, 6, -2, -4, -6, 4, -6, 0},
        {0, 0, 0, -6, 0, 6, -2, -6, -2, 6, -6, -4, -6, 4, 0},
    },
    {
        {0, 0, -6, 0, 6, 0, -6, 2, 6, 2, -2, 4, 4, 6, 0},
        {0, 0, 0, -6, 0, 6, 2, -6, 2, 6, 4, -2, 6, 4, 0},
        {0, 0, -6, 0, 6, 0, -6, -2, 6, -2, -4, -6, 4, -6, 0},
        {0, 0, 0, -6, 0, 6, -2, -6, -2, 6, -6, -4, -6, 4, 0},
    },
    {
        {0, 0, -6, 0, 6, 0, -6, 2, 6, 2, -2, 4, 4, 6, 0},
        {0, 0, 0, -6, 0, 6, 2, -6, 2, 6, 4, -2, 6, 4, 0},
        {0, 0, -6, 0, 6, 0, -6, -2, 6, -2, -4, -6, 4, -6, 0},
        {0, 0, 0, -6, 0, 6, -2, -6, -2, 6, -6, -4, -6, 4, 0},
    },
    {
        {0, 0, -6, 0, 6, 0, -6, 2, 6, 2, -2, 4, 4, 6, 0},
        {0, 0, 0, -6, 0, 6, 2, -6, 2, 6, 4, -2, 6, 4, 0},
        {0, 0, -6, 0, 6, 0, -6, -2, 6, -2, -4, -6, 4, -6, 0},
        {0, 0, 0, -6, 0, 6, -2, -6, -2, 6, -6, -4, -6, 4, 0},
    },
    {
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, 8, 3, 8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, 8, -3, 8, 3, 0},
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, -8, 3, -8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, -8, -3, -8, 3, 0},
    },
    {
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, 8, 3, 8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, 8, -3, 8, 3, 0},
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, -8, 3, -8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, -8, -3, -8, 3, 0},
    },
    {
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, 8, 3, 8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, 8, -3, 8, 3, 0},
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, -8, 3, -8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, -8, -3, -8, 3, 0},
    },
    {
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, 8, 3, 8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, 8, -3, 8, 3, 0},
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, -8, 3, -8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, -8, -3, -8, 3, 0},
    },
    {
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, 8, 3, 8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, 8, -3, 8, 3, 0},
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, -8, 3, -8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, -8, -3, -8, 3, 0},
    },
    {
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, 8, 3, 8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, 8, -3, 8, 3, 0},
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, -8, 3, -8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, -8, -3, -8, 3, 0},
    },
    {
        {0, 0, -4, 0, 4, 0, -12, 0, 12, 0, -4, 12, 4, 12, 0},
        {0, 0, 0, -4, 0, 4, 0, -12, 0, 12, 12, -4, 12, 4, 0},
        {0, 0, -4, 0, 4, 0, -12, 0, 12, 0, -4, -12, 4, -12, 0},
        {0, 0, 0, -4, 0, 4, 0, -12, 0, 12, -12, -4, -12, 4, 0},
    },
    {
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, 8, 3, 8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, 8, -3, 8, 3, 0},
        {0, 0, -3, 0, 3, 0, -8, 0, 8, 0, -3, -8, 3, -8, 0},
        {0, 0, 0, -3, 0, 3, 0, -8, 0, 8, -8, -3, -8, 3, 0},
    }
};

int formation_rioter_get_target_building(int *x_tile, int *y_tile)
{
    int best_type_index = 100;
    building *best_building = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = building_get(i);
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        for (int t = 0; t < 100 && t <= best_type_index && RIOTER_ATTACK_PRIORITY[t]; t++) {
            if (b->type == RIOTER_ATTACK_PRIORITY[t]) {
                if (t < best_type_index) {
                    best_type_index = t;
                    best_building = b;
                }
                break;
            }
        }
    }
    if (!best_building) {
        return 0;
    }
    if (best_building->type == BUILDING_WAREHOUSE) {
        *x_tile = best_building->x + 1;
        *y_tile = best_building->y;
        return best_building->id + 1;
    } else {
        *x_tile = best_building->x;
        *y_tile = best_building->y;
        return best_building->id;
    }
}

static void set_enemy_target_building(struct formation_t *m)
{
    int attack = m->attack_priority;
    if (attack == FORMATION_ATTACK_RANDOM) {
        attack = random_byte() & 3;
    }
    int best_type_index = 100;
    building *best_building = 0;
    int min_distance = 10000;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = building_get(i);
        if (b->state != BUILDING_STATE_IN_USE || map_soldier_strength_get(b->grid_offset)) {
            continue;
        }
        for (int n = 0; n < 100 && n <= best_type_index && ENEMY_ATTACK_PRIORITY[attack][n]; n++) {
            if (b->type == ENEMY_ATTACK_PRIORITY[attack][n]) {
                int distance = calc_maximum_distance(m->x_home, m->y_home, b->x, b->y);
                if (n < best_type_index) {
                    best_type_index = n;
                    best_building = b;
                    min_distance = distance;
                } else if (distance < min_distance) {
                    best_building = b;
                    min_distance = distance;
                }
                break;
            }
        }
    }
    if (best_building) {
        if (best_building->type == BUILDING_WAREHOUSE) {
            formation_set_destination_building(m, best_building->x + 1, best_building->y, best_building->id + 1);
        } else {
            formation_set_destination_building(m, best_building->x, best_building->y, best_building->id);
        }
    } else {
        // no priority buildings left: target population
        for (int i = 1; i < MAX_BUILDINGS; i++) {
            building *b = building_get(i);
            if (b->state != BUILDING_STATE_IN_USE || map_soldier_strength_get(b->grid_offset)) {
                continue;
            }
            if (building_is_house(b->type)) {
                formation_set_destination_building(m, b->x, b->y, b->id);
                break;
            }
        }
    }
}

static void set_native_target_building(struct formation_t *m)
{
    building *min_building = 0;
    int min_distance = 10000;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = building_get(i);
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
        formation_set_destination_building(m, min_building->x, min_building->y, min_building->id);
    }
}

static void approach_target(struct formation_t *m)
{
    if (map_routing_noncitizen_can_travel_over_land(m->x_home, m->y_home,
        m->destination_x, m->destination_y, m->destination_building_id, 400) ||
        map_routing_noncitizen_can_travel_through_everything(m->x_home, m->y_home,
            m->destination_x, m->destination_y)) {
        int x_tile, y_tile;
        if (map_routing_get_closest_tile_within_range(m->x_home, m->y_home,
            m->destination_x, m->destination_y, 8, 20, &x_tile, &y_tile)) {
            formation_set_destination(m, x_tile, y_tile);
        }
    }
}

static void set_figures_to_initial(const struct formation_t *m)
{
    for (int i = 0; i < MAX_FORMATION_FIGURES; i++) {
        if (m->figures[i] > 0) {
            struct figure_t *f = &figures[m->figures[i]];
            if (f->action_state != FIGURE_ACTION_CORPSE &&
                f->action_state != FIGURE_ACTION_ATTACK) {
                f->action_state = FIGURE_ACTION_ENEMY_INITIAL;
                f->wait_ticks = 0;
            }
        }
    }
}

int formation_enemy_move_formation_to(const struct formation_t *m, int x, int y, int *x_tile, int *y_tile)
{
    int base_offset = map_grid_offset(
        formation_layout_position_x(m->layout, 0),
        formation_layout_position_y(m->layout, 0));
    int figure_offsets[MAX_FORMATION_FIGURES];
    for (int i = 0; i < m->num_figures; i++) {
        figure_offsets[i] = map_grid_offset(
            formation_layout_position_x(m->layout, i),
            formation_layout_position_y(m->layout, i)) - base_offset;
    }
    map_routing_noncitizen_can_travel_over_land(x, y, -1, -1, 0, 600);
    for (int r = 0; r <= 10; r++) {
        int x_min, y_min, x_max, y_max;
        map_grid_get_area(x, y, 1, r, &x_min, &y_min, &x_max, &y_max);
        for (int yy = y_min; yy <= y_max; yy++) {
            for (int xx = x_min; xx <= x_max; xx++) {
                int can_move = 1;
                for (int fig = 0; fig < m->num_figures; fig++) {
                    int grid_offset = map_grid_offset(xx, yy) + figure_offsets[fig];
                    if (!map_grid_is_valid_offset(grid_offset)) {
                        can_move = 0;
                        break;
                    }
                    if (map_terrain_is(grid_offset, TERRAIN_IMPASSABLE)) {
                        can_move = 0;
                        break;
                    }
                    if (map_routing_distance(grid_offset) <= 0) {
                        can_move = 0;
                        break;
                    }
                    if (map_has_figure_at(grid_offset) && figures[map_figure_at(grid_offset)].formation_id != m->id) {
                        can_move = 0;
                        break;
                    }
                }
                if (can_move) {
                    *x_tile = xx;
                    *y_tile = yy;
                    return 1;
                }
            }
        }
    }
    return 0;
}

static void mars_kill_enemies(void)
{
    if (city_data.religion.mars_spirit_power <= 0) {
        return;
    }
    int grid_offset = 0;
    for (int i = 1; i < MAX_FIGURES && city_data.religion.mars_spirit_power > 0; i++) {
        struct figure_t *f = &figures[i];
        if (figure_is_dead(f)) {
            continue;
        }
        if ((f->is_enemy_unit && f->type != FIGURE_ENEMY_GLADIATOR) || f->is_caesar_legion_unit) {
            f->action_state = FIGURE_ACTION_CORPSE;
            clear_targeting_on_unit_death(f);
            refresh_formation_figure_indexes(f);
            city_data.religion.mars_spirit_power--;
            if (!grid_offset) {
                grid_offset = f->grid_offset;
            }
        }
    }
    city_data.religion.mars_spirit_power = 0;
    city_message_post(1, MESSAGE_SPIRIT_OF_MARS, 0, grid_offset);
}

static void update_enemy_movement(struct formation_t *m, int roman_distance)
{
    const enemy_army *army = enemy_army_get(m->invasion_id);
    formation_state *state = &m->enemy_state;
    int regroup = 0;
    int halt = 0;
    int pursue_target = 0;
    int advance = 0;
    int target_formation_id = 0;
    if (m->missile_fired) {
        halt = 1;
    } else if (m->missile_attack_timeout) {
        pursue_target = 1;
        target_formation_id = m->missile_attack_formation_id;
    } else if (m->wait_ticks < 32) {
        regroup = 1;
        state->duration_advance = 4;
    } else if (army->ignore_roman_soldiers) {
        halt = 0;
        regroup = 0;
        advance = 1;
    } else {
        int halt_duration, advance_duration, regroup_duration;
        if (army->layout == FORMATION_ENEMY_MOB) {
            switch (m->enemy_legion_index) {
                case 0:
                case 1:
                    regroup_duration = 2;
                    advance_duration = 4;
                    halt_duration = 2;
                    break;
                case 2:
                case 3:
                    regroup_duration = 2;
                    advance_duration = 5;
                    halt_duration = 3;
                    break;
                default:
                    regroup_duration = 2;
                    advance_duration = 6;
                    halt_duration = 4;
                    break;
            }
            if (!roman_distance) {
                advance_duration += 6;
                halt_duration--;
                regroup_duration--;
            }
        } else {
            if (roman_distance) {
                regroup_duration = 6;
                advance_duration = 4;
                halt_duration = 2;
            } else {
                regroup_duration = 1;
                advance_duration = 12;
                halt_duration = 1;
            }
        }
        if (state->duration_halt) {
            state->duration_advance = 0;
            state->duration_regroup = 0;
            halt = 1;
            state->duration_halt--;
            if (state->duration_halt <= 0) {
                state->duration_regroup = regroup_duration;
                set_figures_to_initial(m);
                regroup = 0;
                halt = 1;
            }
        } else if (state->duration_regroup) {
            state->duration_advance = 0;
            state->duration_halt = 0;
            regroup = 1;
            state->duration_regroup--;
            if (state->duration_regroup <= 0) {
                state->duration_advance = advance_duration;
                set_figures_to_initial(m);
                advance = 1;
                regroup = 0;
            }
        } else {
            state->duration_regroup = 0;
            state->duration_halt = 0;
            advance = 1;
            state->duration_advance--;
            if (state->duration_advance <= 0) {
                state->duration_halt = halt_duration;
                set_figures_to_initial(m);
                halt = 1;
                advance = 0;
            }
        }
    }

    if (m->wait_ticks > 32) {
        mars_kill_enemies();
    }
    if (halt) {
        formation_set_destination(m, m->x_home, m->y_home);
    } else if (pursue_target) {
        if (target_formation_id > 0) {
            if (formations[target_formation_id].num_figures > 0) {
                formation_set_destination(m, formations[target_formation_id].x_home, formations[target_formation_id].y_home);
            }
        } else {
            formation_set_destination(m, army->destination_x, army->destination_y);
        }
    } else if (regroup) {
        int layout = army->layout;
        int x_offset = LAYOUT_ORIENTATION_OFFSETS[layout][m->orientation / 2][2 * m->enemy_legion_index] +
            army->home_x;
        int y_offset = LAYOUT_ORIENTATION_OFFSETS[layout][m->orientation / 2][2 * m->enemy_legion_index + 1] +
            army->home_y;
        int x_tile, y_tile;
        if (formation_enemy_move_formation_to(m, x_offset, y_offset, &x_tile, &y_tile)) {
            formation_set_destination(m, x_tile, y_tile);
        }
    } else if (advance) {
        int layout = army->layout;
        int x_offset = LAYOUT_ORIENTATION_OFFSETS[layout][m->orientation / 2][2 * m->enemy_legion_index] +
            army->destination_x;
        int y_offset = LAYOUT_ORIENTATION_OFFSETS[layout][m->orientation / 2][2 * m->enemy_legion_index + 1] +
            army->destination_y;
        int x_tile, y_tile;
        if (formation_enemy_move_formation_to(m, x_offset, y_offset, &x_tile, &y_tile)) {
            formation_set_destination(m, x_tile, y_tile);
        }
    }
}

static void update_enemy_formation(struct formation_t *m, int *roman_distance)
{
    enemy_army *army = enemy_army_get_editable(m->invasion_id);
    if (enemy_army_is_stronger_than_legions()) {
        if (m->figure_type != FIGURE_FORT_JAVELIN) {
            army->ignore_roman_soldiers = 1;
        }
    }
    formation_adjust_counters(m);
    if (city_data.figure.soldiers <= 0) {
        formation_clear_counters(m);
    }
    for (int n = 0; n < MAX_FORMATION_FIGURES; n++) {
        struct figure_t *f = &figures[m->figures[n]];
        if (f->action_state == FIGURE_ACTION_ATTACK) {
            struct figure_t *opponent = &figures[f->target_figure_id];
            if (!figure_is_dead(opponent) && opponent->is_player_legion_unit) {
                m->recent_fight = 6;
            }
        }
    }
    if (m->morale <= ROUT_MORALE_THRESHOLD) {
        for (int n = 0; n < MAX_FORMATION_FIGURES; n++) {
            struct figure_t *f = &figures[m->figures[n]];
            if (f->action_state != FIGURE_ACTION_ATTACK &&
                f->action_state != FIGURE_ACTION_CORPSE &&
                f->action_state != FIGURE_ACTION_FLEEING) {
                f->action_state = FIGURE_ACTION_FLEEING;
                figure_route_remove(f);
                sound_effect_play(SOUND_EFFECT_HORN3);
            }
        }
        // on formation rout, reduce morale of all enemy formations, improve morale of all legions
        if (!m->routed) {
            for (int j = 1; j < MAX_FORMATIONS; j++) {
                if (formations[j].in_use && !formations[j].is_herd) {
                    if (formations[j].is_legion) {
                        formations[j].morale = calc_bound(formations[j].morale + 5, 0, formations[j].max_morale);
                    } else {
                        formations[j].morale = calc_bound(formations[j].morale - 5, 0, formations[j].max_morale);
                    }
                }
            }
            m->routed = 1;
        }
        return;
    }
    if (m->figures[0]) {
        struct figure_t *f = &figures[m->figures[0]];
        if (f->state == FIGURE_STATE_ALIVE) {
            formation_set_home(m, f->x, f->y);
        }
    }
    if (!army->formation_id) {
        army->formation_id = m->id;
        army->home_x = m->x_home;
        army->home_y = m->y_home;
        army->layout = m->layout;
        *roman_distance = 0;
        map_routing_noncitizen_can_travel_over_land(m->x_home, m->y_home, -2, -2, 100000, 300);
        int x_tile, y_tile;
        if (map_soldier_strength_get_max(m->x_home, m->y_home, 16, &x_tile, &y_tile)) {
            *roman_distance = 1;
        } else if (map_soldier_strength_get_max(m->x_home, m->y_home, 32, &x_tile, &y_tile)) {
            *roman_distance = 2;
        }
        if (army->ignore_roman_soldiers) {
            *roman_distance = 0;
        }
        if (*roman_distance == 1) {
            // attack roman legion
            army->destination_x = x_tile;
            army->destination_y = y_tile;
            army->destination_building_id = 0;
        } else {
            set_enemy_target_building(m);
            approach_target(m);
            army->destination_x = m->destination_x;
            army->destination_y = m->destination_y;
            army->destination_building_id = m->destination_building_id;
        }
    }
    m->enemy_legion_index = army->num_legions++;
    m->wait_ticks++;
    formation_set_destination_building(m,
        army->destination_x, army->destination_y, army->destination_building_id
    );
    update_enemy_movement(m, *roman_distance);
}

void formation_enemy_update(void)
{
    if (enemy_army_total_enemy_formations() <= 0) {
        enemy_armies_clear_ignore_roman_soldiers();
    } else {
        enemy_army_calculate_roman_influence();
        enemy_armies_clear_formations();
        int roman_distance = 0;
        for (int i = 1; i < MAX_FORMATIONS; i++) {
            if (formations[i].in_use && !formations[i].is_herd && !formations[i].is_legion) {
                update_enemy_formation(&formations[i], &roman_distance);
            }
        }
    }
    set_native_target_building(&formations[0]);
}
