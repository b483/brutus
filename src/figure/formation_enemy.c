#include "formation_enemy.h"

#include "building/building.h"
#include "city/buildings.h"
#include "city/data_private.h"
#include "city/gods.h"
#include "city/message.h"
#include "core/calc.h"
#include "figure/combat.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "figure/route.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/routing.h"
#include "map/routing_path.h"
#include "map/terrain.h"
#include "sound/effect.h"
#include "sound/speech.h"

#include <stdlib.h>

static const int ENEMY_ATTACK_PRIORITY[4][30] = {
    {BUILDING_GRANARY, BUILDING_WAREHOUSE, BUILDING_MARKET,
    BUILDING_WHEAT_FARM, BUILDING_VEGETABLE_FARM, BUILDING_FRUIT_FARM, BUILDING_OLIVE_FARM, BUILDING_VINES_FARM, BUILDING_PIG_FARM, 0},
    {BUILDING_SENATE, BUILDING_FORUM, 0},
    {BUILDING_TRIUMPHAL_ARCH, BUILDING_SENATE, BUILDING_GOVERNORS_PALACE, BUILDING_GOVERNORS_VILLA, BUILDING_GOVERNORS_HOUSE,
    BUILDING_HIPPODROME, 0},
    {BUILDING_BARRACKS, BUILDING_MILITARY_ACADEMY, BUILDING_PREFECTURE, 0}
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

static int LAYOUT_REGROUP_DURATION[] = {
    10, // FORMATION_TORTOISE
    8, // FORMATION_DOUBLE_LINE_1
    8, // FORMATION_DOUBLE_LINE_2
    0, // FORMATION_SINGLE_LINE_1 (not used by enemies)
    0, // FORMATION_SINGLE_LINE_2 (not used by enemies)
    0, // FORMATION_MOP_UP (not used by enemies)
    0, // FORMATION_AT_REST (not used by enemies)
    5, // FORMATION_ENEMY_MOB
    9, // FORMATION_ENEMY_WIDE_COLUMN
};

int formation_rioter_get_target_building(int *x_tile, int *y_tile)
{
    int best_type_index = 100;
    struct building_t *best_building = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
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

static struct building_t *get_building_target_for_enemy(struct formation_t *m)
{
    int attack = m->attack_priority;
    if (attack == FORMATION_ATTACK_RANDOM) {
        attack = rand() % 4;
    }
    int best_type_index = 100;
    struct building_t *best_building = 0;
    int min_distance = 10000;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        for (int n = 0; n < 100 && n <= best_type_index && ENEMY_ATTACK_PRIORITY[attack][n]; n++) {
            if (b->type == ENEMY_ATTACK_PRIORITY[attack][n]) {
                int distance = calc_maximum_distance(figures[m->figures[0]].x, figures[m->figures[0]].y, b->x, b->y);
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
        return best_building;
    } else {
        // no priority buildings left: target population
        for (int i = 1; i < MAX_BUILDINGS; i++) {
            struct building_t *b = &all_buildings[i];
            if (b->state != BUILDING_STATE_IN_USE) {
                continue;
            }
            if (building_is_house(b->type)) {
                return b;
            }
        }
    }
    return 0;
}

int formation_enemy_move_formation_to(const struct formation_t *m, int x, int y, int *x_tile, int *y_tile)
{
    int base_offset = map_grid_offset(formation_layout_position_x(m->layout, 0), formation_layout_position_y(m->layout, 0));
    int figure_offsets[MAX_FORMATION_FIGURES];
    for (int i = 0; i < m->num_figures; i++) {
        figure_offsets[i] = map_grid_offset(formation_layout_position_x(m->layout, i), formation_layout_position_y(m->layout, i)) - base_offset;
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
    int grid_offset = 0;
    for (int i = 1; i < MAX_FIGURES && city_data.religion.mars_spirit_power > 0; i++) {
        struct figure_t *f = &figures[i];
        if (figure_is_dead(f)) {
            continue;
        }
        if ((figure_properties[f->type].is_enemy_unit && f->type != FIGURE_ENEMY_GLADIATOR) || figure_properties[f->type].is_caesar_legion_unit) {
            f->action_state = FIGURE_ACTION_CORPSE;
            clear_targeting_on_unit_death(f);
            update_counters_on_unit_death(f);
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

void update_enemy_formations(void)
{
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && !formations[i].is_herd && !formations[i].is_legion) {
            struct formation_t *m = &formations[i];
            int formation_spawning = 0;
            for (int n = 0; n < m->num_figures; n++) {
                if (figures[m->figures[n]].is_ghost) {
                    formation_spawning = 1;
                    break;
                }
            }
            if (formation_spawning) {
                // delay all other (already spawned) formations so they move together after
                for (int j = 1; j < MAX_FORMATIONS; j++) {
                    if (formations[j].in_use && !formations[j].is_herd && !formations[j].is_legion) {
                        formations[j].wait_ticks = 0;
                    }
                }
                continue;
            }
            formation_adjust_counters(m);
            if (!city_data.figure.soldiers) {
                formation_clear_counters(m);
            }
            if (city_data.religion.mars_spirit_power && m->wait_ticks > 16) {
                mars_kill_enemies();
            }
            if (m->morale <= ROUT_MORALE_THRESHOLD) {
                for (int n = 0; n < MAX_FORMATION_FIGURES; n++) {
                    struct figure_t *f = &figures[m->figures[n]];
                    if (f->action_state != FIGURE_ACTION_ATTACK &&
                        f->action_state != FIGURE_ACTION_CORPSE &&
                        f->action_state != FIGURE_ACTION_FLEEING) {
                        f->action_state = FIGURE_ACTION_FLEEING;
                        figure_route_remove(f);
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
                    sound_effect_play(SOUND_EFFECT_HORN3);
                }
                continue;
            }

            if (m->recent_fight || m->missile_attack_timeout) {
                for (int j = 1; j < MAX_FORMATIONS; j++) {
                    if (formations[j].in_use && !formations[j].is_herd && !formations[j].is_legion) {
                        formations[j].wait_ticks += 2;
                    }
                }
            } else {
                m->wait_ticks++;
            }

            struct figure_t *target_unit = 0;
            for (int n = 0; n < m->num_figures; n++) {
                struct figure_t *f = &figures[m->figures[n]];
                if (!figure_properties[f->type].max_range) {
                    target_unit = melee_unit__set_closest_target(f);
                    if (target_unit) {
                        if (f->action_state != FIGURE_ACTION_CORPSE && f->action_state != FIGURE_ACTION_ATTACK && f->action_state != FIGURE_ACTION_FLEEING) {
                            if (m->layout == FORMATION_ENEMY_MOB  // melee units in a mob break rank to chase enemies
                            || (target_unit->type == FIGURE_FORT_JAVELIN && f->speed_multiplier >= target_unit->speed_multiplier)) { // melee units in other formations chase javelins only, and only if they are at least as fast as them
                                f->destination_x = target_unit->x;
                                f->destination_y = target_unit->y;
                                f->action_state = FIGURE_ACTION_ENEMY_ENGAGED;
                            }
                        }
                    }
                }
            }

            if (m->wait_ticks > LAYOUT_REGROUP_DURATION[m->layout]) {
                if (target_unit) {
                    int reinforcements_sent = 0;
                    for (int j = 1; j < MAX_FORMATIONS; j++) {
                        if (formations[j].in_use && !formations[j].is_herd && !formations[j].is_legion) {
                            for (int n = 0; n < formations[j].num_figures; n++) {
                                struct figure_t *f = &figures[formations[j].figures[n]];
                                if (f->action_state != FIGURE_ACTION_CORPSE && f->action_state != FIGURE_ACTION_ATTACK && f->action_state != FIGURE_ACTION_FLEEING) {
                                    f->destination_x = target_unit->x;
                                    f->destination_y = target_unit->y;
                                    f->target_figure_id = target_unit->id;
                                    f->action_state = FIGURE_ACTION_ENEMY_ENGAGED;
                                    reinforcements_sent++;
                                }
                            }
                            if (reinforcements_sent >= city_data.figure.soldiers
                            || (m->figure_type == FIGURE_ENEMY_CAESAR_LEGIONARY && reinforcements_sent >= city_data.figure.imperial_soldiers)
                            || (m->figure_type != FIGURE_ENEMY_CAESAR_LEGIONARY && reinforcements_sent >= city_data.figure.enemies)) {
                                break;
                            }
                        }
                    }
                }
                struct building_t *target_building = get_building_target_for_enemy(m);
                if (target_building) {
                    if (map_routing_noncitizen_can_travel_over_land(m->destination_x, m->destination_y, target_building->x, target_building->y, 0, 400)
                    || map_routing_noncitizen_can_travel_through_everything(m->destination_x, m->destination_y, target_building->x, target_building->y)) {
                        int x_tile, y_tile;
                        if (map_routing_get_closest_tile_within_range(m->destination_x, m->destination_y, target_building->x, target_building->y, 8, 20, &x_tile, &y_tile)) {
                            m->destination_x = x_tile;
                            m->destination_y = y_tile;
                            for (int n = 0; n < m->num_figures; n++) {
                                struct figure_t *f = &figures[m->figures[n]];
                                if (!f->is_ghost && (f->action_state == FIGURE_ACTION_ENEMY_SPAWNING || f->action_state == FIGURE_ACTION_ENEMY_REGROUPING)) {
                                    f->action_state = FIGURE_ACTION_ENEMY_ADVANCING;
                                }
                            }
                            m->wait_ticks = 0;
                        }
                    }
                }
            }
        }
    }
}