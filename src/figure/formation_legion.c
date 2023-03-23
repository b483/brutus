#include "formation_legion.h"

#include "city/data_private.h"
#include "city/military.h"
#include "city/warning.h"
#include "core/calc.h"
#include "figure/enemy_army.h"
#include "figure/figure.h"
#include "figure/route.h"
#include "map/building.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/routing.h"
#include "scenario/data.h"
#include "sound/speech.h"

int formation_legion_create_for_fort(building *fort)
{
    formation_calculate_legion_totals();

    // create legion formation
    struct formation_t *m = create_formation_type(fort->subtype.fort_figure_type);
    m->is_legion = 1;
    m->legion_id = formation_get_num_legions() - 1;
    m->layout = FORMATION_DOUBLE_LINE_1;
    if (fort->subtype.fort_figure_type == FIGURE_FORT_LEGIONARY) {
        m->max_morale = 80;
    } else {
        m->max_morale = 60;
    }
    m->morale = m->max_morale;
    m->max_figures = MAX_FORMATION_FIGURES;
    m->x = m->standard_x = m->x_home = fort->x + 3;
    m->y = m->standard_y = m->y_home = fort->y - 1;
    m->building_id = fort->id;
    m->is_at_fort = 1;

    figure *standard = figure_create(FIGURE_FORT_STANDARD, 0, 0, DIR_0_TOP);
    standard->building_id = fort->id;
    standard->formation_id = m->id;

    return m->id;
}

void formation_calculate_legion_totals(void)
{
    city_data.military.legionary_legions = 0;
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use) {
            if (formations[i].is_legion) {
                if (formations[i].figure_type == FIGURE_FORT_LEGIONARY) {
                    city_data.military.legionary_legions++;
                }
            }
            if (formations[i].missile_attack_timeout <= 0 && formations[i].figures[0]) {
                figure *f = figure_get(formations[i].figures[0]);
                if (f->state == FIGURE_STATE_ALIVE) {
                    formation_set_home(&formations[i], f->x, f->y);
                }
            }
        }
    }
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

int formation_for_legion(int legion_index)
{
    int index = 1;
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && formations[i].is_legion) {
            if (index++ == legion_index) {
                return i;
            }
        }
    }
    return 0;
}

void formation_legion_restore_layout(struct formation_t *m)
{
    if (m->layout == FORMATION_MOP_UP) {
        m->layout = m->prev.layout;
    }
}

void formation_legion_move_to(struct formation_t *m, map_tile *tile)
{
    map_routing_calculate_distances(m->x_home, m->y_home);
    if (map_routing_distance(tile->grid_offset) <= 0) {
        return; // unable to route there
    }
    if (tile->x == m->x_home && tile->y == m->y_home) {
        return; // use formation_legion_return_home
    }
    if (m->cursed_by_mars) {
        return;
    }

    if (m->morale <= ROUT_MORALE_THRESHOLD) {
        city_warning_show(WARNING_LEGION_MORALE_TOO_LOW);
        return;
    }

    m->standard_x = tile->x;
    m->standard_y = tile->y;
    // prevent perfect stacking for legion by not allowing placement of standard on top of another
    int figure_id = map_figures.items[map_grid_offset(m->standard_x, m->standard_y)];
    while (figure_id) {
        figure *f = figure_get(figure_id);
        if (f->type == FIGURE_FORT_STANDARD) {
            return;
        }
        figure_id = f->next_figure_id_on_same_tile;
    }
    m->is_at_fort = 0;
    sound_speech_play_file("wavs/cohort5.wav");

    for (int i = 0; i < MAX_FORMATION_FIGURES && m->figures[i]; i++) {
        figure *f = figure_get(m->figures[i]);
        if (f->action_state == FIGURE_ACTION_CORPSE ||
            f->action_state == FIGURE_ACTION_ATTACK) {
            continue;
        }
        f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_STANDARD;
        figure_route_remove(f);
    }
}

void formation_legion_return_home(struct formation_t *m)
{
    map_routing_calculate_distances(m->x_home, m->y_home);
    if (map_routing_distance(map_grid_offset(m->x, m->y)) <= 0) {
        return; // unable to route home
    }
    if (m->cursed_by_mars) {
        return;
    }
    m->is_at_fort = 1;
    formation_legion_restore_layout(m);
    for (int i = 0; i < MAX_FORMATION_FIGURES && m->figures[i]; i++) {
        figure *f = figure_get(m->figures[i]);
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
        figure *f = figure_get(figure_id);
        if (f->is_player_legion_unit || f->type == FIGURE_FORT_STANDARD) {
            return f->formation_id;
        }
        figure_id = f->next_figure_id_on_same_tile;
    }
    return 0;
}

int formation_legion_at_building(int grid_offset)
{
    int building_id = map_building_at(grid_offset);
    if (building_id > 0) {
        building *b = building_get(building_id);
        if (b->state == BUILDING_STATE_IN_USE && (b->type == BUILDING_FORT || b->type == BUILDING_FORT_GROUND)) {
            return b->formation_id;
        }
    }
    return 0;
}

void formation_legion_update(void)
{
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && formations[i].is_legion) {
            formation_decrease_monthly_counters(&formations[i]);
            if (city_data.figure.enemies <= 0) {
                formation_clear_monthly_counters(&formations[i]);
            }
            if (formations[i].morale > ROUT_MORALE_THRESHOLD) {
                formations[i].routed = 0;
            }
            int formation_military_training = 1;
            for (int n = 0; n < formations[i].max_figures; n++) {
                figure *f = figure_get(formations[i].figures[n]);
                if (f->id) {
                    if (!f->is_military_trained) {
                        formation_military_training = 0;
                        formations[i].has_military_training = 0;
                        if (formations[i].figure_type == FIGURE_FORT_LEGIONARY) {
                            formations[i].max_morale = 80;
                        } else {
                            formations[i].max_morale = 60;
                        }
                    }
                    if (f->action_state == FIGURE_ACTION_ATTACK) {
                        formations[i].recent_fight = 6;
                    }
                    // decrease damage
                    if (f->state == FIGURE_STATE_ALIVE && f->action_state == FIGURE_ACTION_SOLDIER_AT_REST) {
                        if (f->damage) {
                            f->damage--;
                        }
                    }
                }
            }
            if (formation_military_training) {
                formations[i].has_military_training = 1;
                if (formations[i].figure_type == FIGURE_FORT_LEGIONARY) {
                    formations[i].max_morale = 100;
                } else {
                    formations[i].max_morale = 80;
                }
            }
            if (formations[i].morale <= ROUT_MORALE_THRESHOLD) {
                // flee back to fort
                for (int n = 0; n < formations[i].max_figures; n++) {
                    figure *f = figure_get(formations[i].figures[n]);
                    if (f->action_state != FIGURE_ACTION_ATTACK &&
                        f->action_state != FIGURE_ACTION_CORPSE &&
                        f->action_state != FIGURE_ACTION_FLEEING) {
                        f->action_state = FIGURE_ACTION_FLEEING;
                        figure_route_remove(f);
                    }
                }
                // on formation rout, reduce morale of all legions, improve morale of all enemy formations
                if (!formations[i].routed) {
                    for (int j = 1; j < MAX_FORMATIONS; j++) {
                        if (formations[j].in_use && !formations[j].is_herd) {
                            if (formations[j].is_legion) {
                                formations[j].morale = calc_bound(formations[j].morale - 5, 0, formations[j].max_morale);
                            } else {
                                formations[j].morale = calc_bound(formations[j].morale + 5, 0, formations[j].max_morale);
                            }
                        }
                    }
                    formations[i].routed = 1;
                }
            } else if (formations[i].layout == FORMATION_MOP_UP) {
                if (enemy_army_total_enemy_formations() +
                    city_data.figure.rioters +
                    city_data.figure.attacking_natives > 0) {
                    for (int n = 0; n < MAX_FORMATION_FIGURES; n++) {
                        if (formations[i].figures[n] != 0) {
                            figure *f = figure_get(formations[i].figures[n]);
                            if (f->action_state != FIGURE_ACTION_ATTACK &&
                                f->action_state != FIGURE_ACTION_CORPSE) {
                                f->action_state = FIGURE_ACTION_SOLDIER_MOPPING_UP;
                            }
                        }
                    }
                } else {
                    formation_legion_restore_layout(&formations[i]);
                }
            }
        }
    }
}