#include "formation_legion.h"

#include "building/barracks.h"
#include "city/data_private.h"
#include "city/military.h"
#include "city/warning.h"
#include "core/calc.h"
#include "figure/figure.h"
#include "figure/route.h"
#include "map/building.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/routing.h"
#include "scenario/data.h"
#include "sound/speech.h"
#include "map/terrain.h"
#include "scenario/editor_events.h"

int formation_legion_create_for_fort(struct building_t *fort)
{
    struct formation_t *m = create_formation_type(fort->subtype.fort_figure_type);
    m->is_legion = 1;
    // assign first available legion_id (factors in fort deletes)
    for (int i = 0; i < MAX_LEGIONS; i++) {
        int legion_id_free = 1;
        for (int j = 1; j < MAX_FORMATIONS; j++) {
            if (formations[j].in_use && formations[j].is_legion && m->id != formations[j].id) {
                if (formations[j].legion_id == i) {
                    legion_id_free = 0;
                    break;
                }
            }
        }
        if (legion_id_free) {
            m->legion_id = i;
            break;
        }
    }
    m->layout = FORMATION_DOUBLE_LINE_1;
    if (fort->subtype.fort_figure_type == FIGURE_FORT_LEGIONARY) {
        m->max_morale = 80;
    } else {
        m->max_morale = 60;
    }
    m->morale = m->max_morale;
    m->max_figures = MAX_FORMATION_FIGURES;
    m->standard_x = fort->x + 3;
    m->standard_y = fort->y - 1;
    m->building_id = fort->id;

    struct figure_t *standard = figure_create(FIGURE_FORT_STANDARD, m->standard_x, m->standard_y, DIR_0_TOP);
    standard->building_id = fort->id;
    standard->formation_id = m->id;

    m->legion_standard__figure_id = standard->id;

    city_data.military.total_legions++;

    return m->id;
}

int formation_get_num_legions(void)
{
    int total = 0;
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && formations[i].is_legion) {
            total++;
        }
    }
    return total;
}

int get_legion_formation_by_index(int legion_index)
{
    int index = 1;
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && formations[i].is_legion) {
            if (index == legion_index) {
                return i;
            }
            index++;
        }
    }
    return 0;
}

static void update_legion_standard_map_location(struct formation_t *legion_formation)
{
    struct figure_t *standard = &figures[legion_formation->legion_standard__figure_id];
    map_figure_delete(standard);
    standard->grid_offset = map_grid_offset(legion_formation->standard_x, legion_formation->standard_y);
    map_figure_add(standard);
}

static void set_destination_for_soldier(struct figure_t *soldier, int dst_x, int dst_y, int dst_grid_offset)
{
    soldier->destination_x = dst_x;
    soldier->destination_y = dst_y;
    soldier->destination_grid_offset = dst_grid_offset;
    soldier->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_STANDARD;
}

static int destination_is_clear(struct figure_t *legion_unit, int target_grid_offset)
{
    // check terrain at destination
    if (map_terrain_is(target_grid_offset, TERRAIN_IMPASSABLE)) {
        return 0;
    }
    // check if any legion unit is heading towards the destination
    for (int i = 1; i < MAX_FIGURES; i++) {
        struct figure_t *f = &figures[i];
        if (!figure_is_dead(f) && figure_properties[f->type].is_player_legion_unit && f->id != legion_unit->id) {
            if (f->destination_grid_offset == target_grid_offset) {
                if (f->formation_id == legion_unit->formation_id) { // same formation, stationary units may not have received the command to move yet
                    if (f->action_state == FIGURE_ACTION_SOLDIER_GOING_TO_STANDARD) {
                        return 0;
                    }
                } else {
                    return 0;
                }
            }
        }
    }
    return 1;
}

void deploy_legion_unit_to_formation_location(struct figure_t *legion_unit, struct formation_t *legion_formation)
{
    figure_route_remove(legion_unit);
    int target_dst_x = legion_formation->standard_x + formation_layout_position_x(legion_formation->layout, legion_unit->index_in_formation);
    int target_dst_y = legion_formation->standard_y + formation_layout_position_y(legion_formation->layout, legion_unit->index_in_formation);
    int target_dst_grid_offset = map_grid_offset(target_dst_x, target_dst_y);
    if (destination_is_clear(legion_unit, target_dst_grid_offset)) {
        set_destination_for_soldier(legion_unit, target_dst_x, target_dst_y, target_dst_grid_offset);
    } else {
        // attempt to route to nearest clear tile
        for (int j = 1; j < 10; j++) {
            target_dst_grid_offset = map_grid_offset(target_dst_x + j, target_dst_y); // +x
            if (destination_is_clear(legion_unit, target_dst_grid_offset)) {
                set_destination_for_soldier(legion_unit, target_dst_x + j, target_dst_y, target_dst_grid_offset);
                break;
            }
            target_dst_grid_offset = map_grid_offset(target_dst_x - j, target_dst_y); // -x
            if (destination_is_clear(legion_unit, target_dst_grid_offset)) {
                set_destination_for_soldier(legion_unit, target_dst_x - j, target_dst_y, target_dst_grid_offset);
                break;
            }
            target_dst_grid_offset = map_grid_offset(target_dst_x, target_dst_y + j); // +y
            if (destination_is_clear(legion_unit, target_dst_grid_offset)) {
                set_destination_for_soldier(legion_unit, target_dst_x, target_dst_y + j, target_dst_grid_offset);
                break;
            }
            target_dst_grid_offset = map_grid_offset(target_dst_x, target_dst_y - j); // -y
            if (destination_is_clear(legion_unit, target_dst_grid_offset)) {
                set_destination_for_soldier(legion_unit, target_dst_x, target_dst_y - j, target_dst_grid_offset);
                break;
            }
            target_dst_grid_offset = map_grid_offset(target_dst_x + j, target_dst_y - j); // +x, -y
            if (destination_is_clear(legion_unit, target_dst_grid_offset)) {
                set_destination_for_soldier(legion_unit, target_dst_x + j, target_dst_y - j, target_dst_grid_offset);
                break;
            }
            target_dst_grid_offset = map_grid_offset(target_dst_x - j, target_dst_y + j); // -x, +y
            if (destination_is_clear(legion_unit, target_dst_grid_offset)) {
                set_destination_for_soldier(legion_unit, target_dst_x - j, target_dst_y + j, target_dst_grid_offset);
                break;
            }
            target_dst_grid_offset = map_grid_offset(target_dst_x + j, target_dst_y + j); // +x, +y
            if (destination_is_clear(legion_unit, target_dst_grid_offset)) {
                set_destination_for_soldier(legion_unit, target_dst_x + j, target_dst_y + j, target_dst_grid_offset);
                break;
            }
            target_dst_grid_offset = map_grid_offset(target_dst_x - j, target_dst_y - j); // -x, -y
            if (destination_is_clear(legion_unit, target_dst_grid_offset)) {
                set_destination_for_soldier(legion_unit, target_dst_x - j, target_dst_y - j, target_dst_grid_offset);
                break;
            }
        }
    }
}

void formation_legion_move_to(struct formation_t *m, map_tile *tile)
{
    sound_speech_play_file("wavs/cohort5.wav");

    m->standard_x = tile->x;
    m->standard_y = tile->y;
    update_legion_standard_map_location(m);

    for (int i = 0; i < m->num_figures && m->figures[i]; i++) {
        struct figure_t *f = &figures[m->figures[i]];
        if (f->action_state != FIGURE_ACTION_CORPSE && f->action_state != FIGURE_ACTION_ATTACK) {
            deploy_legion_unit_to_formation_location(f, m);
        }
    }
}

void formation_legion_return_home(struct formation_t *m)
{
    map_routing_calculate_distances(m->standard_x, m->standard_y);
    if (map_routing_distance(map_grid_offset(figures[m->figures[0]].x, figures[m->figures[0]].y)) <= 0) {
        return; // unable to route home
    }
    m->standard_x = all_buildings[m->building_id].x + 3;
    m->standard_y = all_buildings[m->building_id].y - 1;
    update_legion_standard_map_location(m);

    for (int i = 0; i < MAX_FORMATION_FIGURES && m->figures[i]; i++) {
        struct figure_t *f = &figures[m->figures[i]];
        if (f->action_state == FIGURE_ACTION_CORPSE ||
            f->action_state == FIGURE_ACTION_ATTACK) {
            continue;
        }
        f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_FORT;
        figure_route_remove(f);
    }
}

int formation_legion_at_grid_offset(int grid_offset)
{
    int figure_id = map_figures.items[grid_offset];
    while (figure_id) {
        struct figure_t *f = &figures[figure_id];
        if (figure_properties[f->type].is_player_legion_unit || f->type == FIGURE_FORT_STANDARD) {
            return f->formation_id;
        }
        figure_id = f->next_figure_id_on_same_tile;
    }
    return 0;
}

void formation_legion_update(void)
{
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && formations[i].is_legion) {
            struct formation_t *m = &formations[i];
            formation_adjust_counters(m);
            if (city_data.figure.enemies <= 0) {
                formation_clear_counters(m);
            }
            if (m->morale > ROUT_MORALE_THRESHOLD) {
                m->routed = 0;
            }
            // check formation military training status, send untrained units to train
            int formation_military_trained = 1;
            for (int n = 0; n < m->num_figures; n++) {
                struct figure_t *f = &figures[m->figures[n]];
                if (!f->is_military_trained) {
                    formation_military_trained = 0;
                    m->has_military_training = 0;
                    if (m->figure_type == FIGURE_FORT_LEGIONARY) {
                        m->max_morale = 80;
                    } else {
                        m->max_morale = 60;
                    }
                    if (scenario.allowed_buildings[BUILDING_MILITARY_ACADEMY] && f->action_state == FIGURE_ACTION_SOLDIER_AT_REST) {
                        map_point mil_acad_road = { 0 };
                        set_destination__closest_building_of_type(f->building_id, BUILDING_MILITARY_ACADEMY, &mil_acad_road);
                        if (mil_acad_road.x) {
                            f->destination_x = mil_acad_road.x;
                            f->destination_y = mil_acad_road.y;
                            f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_MILITARY_ACADEMY;
                            break; // causes a delay between sending units
                        }
                    }
                }
            }
            if (formation_military_trained) {
                m->has_military_training = 1;
                if (m->figure_type == FIGURE_FORT_LEGIONARY) {
                    m->max_morale = 100;
                } else {
                    m->max_morale = 80;
                }
            }

            // check if all units of a formation are at rest (not deployed)
            int formation_at_rest = 1;
            for (int n = 0; n < m->num_figures; n++) {
                struct figure_t *f = &figures[m->figures[n]];
                if (f->action_state == FIGURE_ACTION_ATTACK
                || f->action_state == FIGURE_ACTION_SOLDIER_GOING_TO_STANDARD
                || f->action_state == FIGURE_ACTION_SOLDIER_AT_STANDARD
                || f->action_state == FIGURE_ACTION_SOLDIER_MOPPING_UP
                || f->action_state == FIGURE_ACTION_SOLDIER_GOING_TO_DISTANT_BATTLE) {
                    formation_at_rest = 0;
                    m->is_at_rest = 0;
                    break;
                }
            }
            if (formation_at_rest) {
                m->is_at_rest = 1;
            }

            // decrease damage
            for (int n = 0; n < m->num_figures; n++) {
                struct figure_t *f = &figures[m->figures[n]];
                if (!figure_is_dead(f) && f->action_state == FIGURE_ACTION_SOLDIER_AT_REST) {
                    if (f->damage) {
                        f->damage--;
                    }
                }
            }

            if (m->morale <= ROUT_MORALE_THRESHOLD) {
                m->standard_x = all_buildings[m->building_id].x + 3;
                m->standard_y = all_buildings[m->building_id].y - 1;
                update_legion_standard_map_location(m);
                // flee back to fort
                for (int n = 0; n < m->num_figures; n++) {
                    struct figure_t *f = &figures[m->figures[n]];
                    if (f->action_state != FIGURE_ACTION_ATTACK &&
                        f->action_state != FIGURE_ACTION_CORPSE &&
                        f->action_state != FIGURE_ACTION_FLEEING) {
                        f->action_state = FIGURE_ACTION_FLEEING;
                        figure_route_remove(f);
                    }
                }
                // on formation rout, reduce morale of all legions, improve morale of all enemy formations
                if (!m->routed) {
                    for (int j = 1; j < MAX_FORMATIONS; j++) {
                        if (formations[j].in_use && !formations[j].is_herd) {
                            if (formations[j].is_legion) {
                                formations[j].morale = calc_bound(formations[j].morale - 5, 0, formations[j].max_morale);
                            } else {
                                formations[j].morale = calc_bound(formations[j].morale + 5, 0, formations[j].max_morale);
                            }
                        }
                    }
                    m->routed = 1;
                }
            }
        }
    }
}