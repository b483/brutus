#include "formation.h"

#include "city/data_private.h"
#include "city/military.h"
#include "core/calc.h"
#include "figure/enemy_army.h"
#include "figure/figure.h"
#include "figure/formation_enemy.h"
#include "figure/formation_herd.h"
#include "figure/formation_legion.h"
#include "figure/properties.h"
#include "figure/route.h"
#include "map/grid.h"
#include "sound/effect.h"

#include <string.h>

struct formation_t formations[MAX_FORMATIONS];

static int selected_formation;

void formations_clear(void)
{
    for (int i = 0; i < MAX_FORMATIONS; i++) {
        memset(&formations[i], 0, sizeof(struct formation_t));
        formations[i].id = i;
    }
    selected_formation = 0;
}

void formation_clear(int formation_id)
{
    memset(&formations[formation_id], 0, sizeof(struct formation_t));
    formations[formation_id].id = formation_id;
}

struct formation_t *create_formation_type(figure_type type)
{
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (!formations[i].in_use) {
            formations[i].in_use = 1;
            formations[i].figure_type = type;
            return &formations[i];
        }
    }
    return 0;
}

struct formation_t *formation_create_enemy(figure_type type, int max_num_figures, int x, int y, int layout, direction_type orientation, int enemy_type, int attack_priority, int invasion_id)
{
    struct formation_t *m = create_formation_type(type);
    m->faction_id = 0;
    if (layout == FORMATION_ENEMY_DOUBLE_LINE) {
        if (orientation == DIR_0_TOP || orientation == DIR_4_BOTTOM) {
            m->layout = FORMATION_DOUBLE_LINE_1;
        } else {
            m->layout = FORMATION_DOUBLE_LINE_2;
        }
    } else {
        m->layout = layout;
    }
    m->orientation = orientation;
    m->morale = 100;
    m->max_figures = max_num_figures;
    m->x = x;
    m->y = y;
    m->enemy_img_group = enemy_type;
    m->attack_priority = attack_priority;
    m->invasion_id = invasion_id;
    return &formations[m->id];
}

int formation_get_selected(void)
{
    return selected_formation;
}

void formation_set_selected(int formation_id)
{
    selected_formation = formation_id;
}

int formation_grid_offset_for_invasion(void)
{
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use == 1 && formations[i].is_legion && formations[i].is_herd) {
            if (formations[i].x_home > 0 || formations[i].y_home > 0) {
                return map_grid_offset(formations[i].x_home, formations[i].y_home);
            } else {
                return 0;
            }
        }
    }
    return 0;
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

void formation_change_morale(struct formation_t *m, int amount)
{
    int max_morale;
    if (m->figure_type == FIGURE_FORT_LEGIONARY) {
        max_morale = m->has_military_training ? 100 : 80;
    } else if (m->figure_type == FIGURE_ENEMY_CAESAR_LEGIONARY) {
        max_morale = 100;
    } else if (m->figure_type == FIGURE_FORT_JAVELIN || m->figure_type == FIGURE_FORT_MOUNTED) {
        max_morale = m->has_military_training ? 80 : 60;
    } else {
        switch (m->enemy_img_group) {
            case ENEMY_TYPE_BARBARIAN:
            case ENEMY_TYPE_NUMIDIAN:
            case ENEMY_TYPE_GAUL:
            case ENEMY_TYPE_CELT:
            case ENEMY_TYPE_GOTH:
                max_morale = 80;
                break;
            case ENEMY_TYPE_GREEK:
            case ENEMY_TYPE_CARTHAGINIAN:
                max_morale = 90;
                break;
            default:
                max_morale = 70;
                break;
        }
    }
    m->morale = calc_bound(m->morale + amount, 0, max_morale);
}

void formation_update_morale_after_death(struct formation_t *m)
{
    formation_calculate_figures();
    int pct_dead = calc_percentage(1, m->num_figures);
    int morale;
    if (pct_dead < 8) {
        morale = -5;
    } else if (pct_dead < 10) {
        morale = -7;
    } else if (pct_dead < 14) {
        morale = -10;
    } else if (pct_dead < 20) {
        morale = -12;
    } else if (pct_dead < 30) {
        morale = -15;
    } else {
        morale = -20;
    }
    formation_change_morale(m, morale);
}

static void change_all_morale(int legion, int enemy)
{
    for (int j = 1; j < MAX_FORMATIONS; j++) {
        if (formations[j].in_use && !formations[j].is_herd) {
            if (formations[j].is_legion) {
                formation_change_morale(&formations[j], legion);
            } else {
                formation_change_morale(&formations[j], enemy);
            }
        }
    }
}

void formation_update_monthly_morale_deployed(void)
{
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (!formations[i].in_use || formations[i].is_herd) {
            continue;
        }
        if (formations[i].is_legion) {
            if (!formations[i].is_at_fort && !formations[i].in_distant_battle) {
                if (formations[i].morale <= 20 && !formations[i].months_low_morale && !formations[i].months_very_low_morale) {
                    change_all_morale(-10, 10);
                }
                if (formations[i].morale <= 10) {
                    formations[i].months_very_low_morale++;
                } else if (formations[i].morale <= 20) {
                    formations[i].months_low_morale++;
                }
            }
        } else { // enemy
            if (formations[i].morale <= 20 && !formations[i].months_low_morale && !formations[i].months_very_low_morale) {
                change_all_morale(10, -10);
            }
            if (formations[i].morale <= 10) {
                formations[i].months_very_low_morale++;
            } else if (formations[i].morale <= 20) {
                formations[i].months_low_morale++;
            }
        }
    }
}

void formation_update_monthly_morale_at_rest(void)
{
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (!formations[i].in_use || formations[i].is_herd) {
            continue;
        }
        if (formations[i].is_legion) {
            if (formations[i].is_at_fort) {
                formations[i].months_from_home = 0;
                formations[i].months_very_low_morale = 0;
                formations[i].months_low_morale = 0;
                formation_change_morale(&formations[i], 5);
                formation_legion_restore_layout(&formations[i]);
            } else if (!formations[i].recent_fight) {
                formations[i].months_from_home++;
                if (formations[i].months_from_home > 3) {
                    if (formations[i].months_from_home > 100) {
                        formations[i].months_from_home = 100;
                    }
                    formation_change_morale(&formations[i], -5);
                }
            }
        } else {
            formation_change_morale(&formations[i], 0);
        }
    }
}

void formation_decrease_monthly_counters(struct formation_t *m)
{
    if (m->is_legion) {
        if (m->cursed_by_mars) {
            m->cursed_by_mars--;
        }
    }
    if (m->missile_fired) {
        m->missile_fired--;
    }
    if (m->missile_attack_timeout) {
        m->missile_attack_timeout--;
    }
    if (m->recent_fight) {
        m->recent_fight--;
    }
}

void formation_clear_monthly_counters(struct formation_t *m)
{
    m->missile_fired = 0;
    m->missile_attack_timeout = 0;
    m->recent_fight = 0;
}

void formation_set_destination(struct formation_t *m, int x, int y)
{
    m->destination_x = x;
    m->destination_y = y;
}

void formation_set_destination_building(struct formation_t *m, int x, int y, int building_id)
{
    m->destination_x = x;
    m->destination_y = y;
    m->destination_building_id = building_id;
}

void formation_set_home(struct formation_t *m, int x, int y)
{
    m->x_home = x;
    m->y_home = y;
}

void formation_move_herds_away(int from_x, int from_y)
{
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && (formations[i].figure_type == FIGURE_SHEEP || formations[i].figure_type == FIGURE_ZEBRA) && formations[i].num_figures > 0) {
            if (calc_maximum_distance(from_x, from_y, formations[i].destination_x, formations[i].destination_y) <= 6) {
                // force new roaming destination search
                formations[i].wait_ticks = 50;
            }
        }
    }
}

// static int add_figure(int formation_id, int figure_id, int deployed, int damage, int max_damage)
// {
//     struct formation_t *f = &formations[formation_id];
//     f->num_figures++;
//     f->total_damage += damage;
//     f->max_total_damage += max_damage;
//     if (deployed) {
//         f->is_at_fort = 0;
//     }
//     for (int fig = 0; fig < MAX_FORMATION_FIGURES; fig++) {
//         if (!f->figures[fig]) {
//             f->figures[fig] = figure_id;
//             return fig;
//         }
//     }
//     return 0; // shouldn't happen
// }

void formation_calculate_figures(void)
{
    // clear figures
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        for (int fig = 0; fig < MAX_FORMATION_FIGURES; fig++) {
            formations[i].figures[fig] = 0;
        }
        formations[i].num_figures = 0;
        formations[i].is_at_fort = 1;
        formations[i].total_damage = 0;
        formations[i].max_total_damage = 0;
    }

    for (int i = 1; i < MAX_FIGURES; i++) {
        figure *f = figure_get(i);
        if (f->state != FIGURE_STATE_ALIVE) {
            continue;
        }
        if (!figure_is_legion(f) && !figure_is_enemy(f) && !figure_is_herd(f)) {
            continue;
        }
        formations[f->formation_id].num_figures++;
        formations[f->formation_id].total_damage += f->damage;
        formations[f->formation_id].max_total_damage += figure_properties_for_type(f->type)->max_damage;
        if (!f->formation_at_rest) {
            formations[f->formation_id].is_at_fort = 0;
        }
        for (int fig = 0; fig < formations[f->formation_id].max_figures; fig++) {
            if (!formations[f->formation_id].figures[fig]) {
                formations[f->formation_id].figures[fig] = f->id;
                f->index_in_formation = fig;
                break;
            }
        }

        // int index = add_figure(f->formation_id, i, f->formation_at_rest != 1, f->damage, figure_properties_for_type(f->type)->max_damage);
        // f->index_in_formation = index;
    }

    enemy_army_totals_clear();
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && !formations[i].is_herd) {
            if (formations[i].is_legion) {
                if (formations[i].num_figures > 0) {
                    int was_halted = formations[i].is_halted;
                    formations[i].is_halted = 1;
                    for (int fig = 0; fig < formations[i].num_figures; fig++) {
                        int figure_id = formations[i].figures[fig];
                        if (figure_id && figure_get(figure_id)->direction != DIR_8_NONE) {
                            formations[i].is_halted = 0;
                        }
                    }
                    int total_strength = formations[i].num_figures;
                    if (formations[i].figure_type == FIGURE_FORT_LEGIONARY) {
                        total_strength += formations[i].num_figures / 2;
                    }
                    enemy_army_totals_add_legion_formation(total_strength);
                    if (formations[i].figure_type == FIGURE_FORT_LEGIONARY) {
                        if (!was_halted && formations[i].is_halted) {
                            sound_effect_play(SOUND_EFFECT_FORMATION_SHIELD);
                        }
                    }
                }
            } else {
                // enemy
                if (formations[i].num_figures <= 0) {
                    formation_clear(formations[i].id);
                } else {
                    enemy_army_totals_add_enemy_formation(formations[i].num_figures);
                }
            }
        }
    }

    city_military_update_totals();
}

static void update_direction(int formation_id, int first_figure_direction)
{
    struct formation_t *m = &formations[formation_id];
    if (m->missile_fired) {
        m->direction = first_figure_direction;
    } else if (m->layout == FORMATION_DOUBLE_LINE_1 || m->layout == FORMATION_SINGLE_LINE_1) {
        if (m->y_home < m->prev.y_home) {
            m->direction = DIR_0_TOP;
        } else if (m->y_home > m->prev.y_home) {
            m->direction = DIR_4_BOTTOM;
        }
    } else if (m->layout == FORMATION_DOUBLE_LINE_2 || m->layout == FORMATION_SINGLE_LINE_2) {
        if (m->x_home < m->prev.x_home) {
            m->direction = DIR_6_LEFT;
        } else if (m->x_home > m->prev.x_home) {
            m->direction = DIR_2_RIGHT;
        }
    } else if (m->layout == FORMATION_TORTOISE) {
        int dx = (m->x_home < m->prev.x_home) ? (m->prev.x_home - m->x_home) : (m->x_home - m->prev.x_home);
        int dy = (m->y_home < m->prev.y_home) ? (m->prev.y_home - m->y_home) : (m->y_home - m->prev.y_home);
        if (dx > dy) {
            if (m->x_home < m->prev.x_home) {
                m->direction = DIR_6_LEFT;
            } else if (m->x_home > m->prev.x_home) {
                m->direction = DIR_2_RIGHT;
            }
        } else {
            if (m->y_home < m->prev.y_home) {
                m->direction = DIR_0_TOP;
            } else if (m->y_home > m->prev.y_home) {
                m->direction = DIR_4_BOTTOM;
            }
        }
    }
    m->prev.x_home = m->x_home;
    m->prev.y_home = m->y_home;
}

static void formation_legion_update(void)
{
    for (int i = 1; i <= MAX_LEGIONS; i++) {
        if (formations[i].in_use != 1 || !formations[i].is_legion) {
            continue;
        }
        formation_decrease_monthly_counters(&formations[i]);
        if (city_data.figure.enemies <= 0) {
            formation_clear_monthly_counters(&formations[i]);
        }
        for (int n = 0; n < MAX_FORMATION_FIGURES; n++) {
            if (figure_get(formations[i].figures[n])->action_state == FIGURE_ACTION_150_ATTACK) {
                formations[i].recent_fight = 6;
            }
        }
        if (formations[i].months_low_morale || formations[i].months_very_low_morale) {
            // flee back to fort
            for (int n = 0; n < MAX_FORMATION_FIGURES; n++) {
                figure *f = figure_get(formations[i].figures[n]);
                if (f->action_state != FIGURE_ACTION_150_ATTACK &&
                    f->action_state != FIGURE_ACTION_149_CORPSE &&
                    f->action_state != FIGURE_ACTION_148_FLEEING) {
                    f->action_state = FIGURE_ACTION_148_FLEEING;
                    figure_route_remove(f);
                }
            }
        } else if (formations[i].layout == FORMATION_MOP_UP) {
            if (enemy_army_total_enemy_formations() +
                city_data.figure.rioters +
                city_data.figure.attacking_natives > 0) {
                for (int n = 0; n < MAX_FORMATION_FIGURES; n++) {
                    if (formations[i].figures[n] != 0) {
                        figure *f = figure_get(formations[i].figures[n]);
                        if (f->action_state != FIGURE_ACTION_150_ATTACK &&
                            f->action_state != FIGURE_ACTION_149_CORPSE) {
                            f->action_state = FIGURE_ACTION_86_SOLDIER_MOPPING_UP;
                        }
                    }
                }
            } else {
                formation_legion_restore_layout(&formations[i]);
            }
        }
    }
}

void formation_update_all(int second_time)
{
    formation_calculate_legion_totals();
    formation_calculate_figures();
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && !formations[i].is_herd) {
            update_direction(formations[i].id, figure_get(formations[i].figures[0])->direction);
        }
    }
    formation_legion_decrease_damage();
    if (!second_time) {
        formation_update_monthly_morale_deployed();
    }
    formation_legion_update();
    formation_enemy_update();
    formation_herd_update();
}

void formations_save_state(buffer *buf)
{
    for (int i = 0; i < MAX_FORMATIONS; i++) {
        struct formation_t *m = &formations[i];
        buffer_write_u8(buf, m->in_use);
        buffer_write_u8(buf, m->faction_id);
        buffer_write_u8(buf, m->legion_id);
        buffer_write_u8(buf, m->is_at_fort);
        buffer_write_i16(buf, m->figure_type);
        buffer_write_i16(buf, m->building_id);
        for (int fig = 0; fig < MAX_FORMATION_FIGURES; fig++) {
            buffer_write_i16(buf, m->figures[fig]);
        }
        buffer_write_u8(buf, m->num_figures);
        buffer_write_u8(buf, m->max_figures);
        buffer_write_i16(buf, m->layout);
        buffer_write_i16(buf, m->morale);
        buffer_write_u8(buf, m->x_home);
        buffer_write_u8(buf, m->y_home);
        buffer_write_u8(buf, m->standard_x);
        buffer_write_u8(buf, m->standard_y);
        buffer_write_u8(buf, m->x);
        buffer_write_u8(buf, m->y);
        buffer_write_u8(buf, m->destination_x);
        buffer_write_u8(buf, m->destination_y);
        buffer_write_i16(buf, m->destination_building_id);
        buffer_write_i16(buf, m->standard_figure_id);
        buffer_write_u8(buf, m->is_legion);
        buffer_write_i16(buf, m->attack_priority);
        buffer_write_i16(buf, m->legion_recruit_type);
        buffer_write_i16(buf, m->has_military_training);
        buffer_write_i16(buf, m->total_damage);
        buffer_write_i16(buf, m->max_total_damage);
        buffer_write_i16(buf, m->wait_ticks);
        buffer_write_i16(buf, m->recent_fight);
        buffer_write_i16(buf, m->enemy_state.duration_advance);
        buffer_write_i16(buf, m->enemy_state.duration_regroup);
        buffer_write_i16(buf, m->enemy_state.duration_halt);
        buffer_write_i16(buf, m->enemy_legion_index);
        buffer_write_i16(buf, m->is_halted);
        buffer_write_i16(buf, m->missile_fired);
        buffer_write_i16(buf, m->missile_attack_timeout);
        buffer_write_i16(buf, m->missile_attack_formation_id);
        buffer_write_i16(buf, m->prev.layout);
        buffer_write_i16(buf, m->cursed_by_mars);
        buffer_write_u8(buf, m->months_low_morale);
        buffer_write_u8(buf, m->empire_service);
        buffer_write_u8(buf, m->in_distant_battle);
        buffer_write_u8(buf, m->is_herd);
        buffer_write_u8(buf, m->enemy_img_group);
        buffer_write_u8(buf, m->direction);
        buffer_write_u8(buf, m->prev.x_home);
        buffer_write_u8(buf, m->prev.y_home);
        buffer_write_u8(buf, m->orientation);
        buffer_write_u8(buf, m->months_from_home);
        buffer_write_u8(buf, m->months_very_low_morale);
        buffer_write_u8(buf, m->invasion_id);
        buffer_write_u8(buf, m->herd_wolf_spawn_delay);
    }
}

void formations_load_state(buffer *buf)
{
    selected_formation = 0;
    for (int i = 0; i < MAX_FORMATIONS; i++) {
        struct formation_t *m = &formations[i];
        m->id = i;
        m->in_use = buffer_read_u8(buf);
        m->faction_id = buffer_read_u8(buf);
        m->legion_id = buffer_read_u8(buf);
        m->is_at_fort = buffer_read_u8(buf);
        m->figure_type = buffer_read_i16(buf);
        m->building_id = buffer_read_i16(buf);
        for (int fig = 0; fig < MAX_FORMATION_FIGURES; fig++) {
            m->figures[fig] = buffer_read_i16(buf);
        }
        m->num_figures = buffer_read_u8(buf);
        m->max_figures = buffer_read_u8(buf);
        m->layout = buffer_read_i16(buf);
        m->morale = buffer_read_i16(buf);
        m->x_home = buffer_read_u8(buf);
        m->y_home = buffer_read_u8(buf);
        m->standard_x = buffer_read_u8(buf);
        m->standard_y = buffer_read_u8(buf);
        m->x = buffer_read_u8(buf);
        m->y = buffer_read_u8(buf);
        m->destination_x = buffer_read_u8(buf);
        m->destination_y = buffer_read_u8(buf);
        m->destination_building_id = buffer_read_i16(buf);
        m->standard_figure_id = buffer_read_i16(buf);
        m->is_legion = buffer_read_u8(buf);
        m->attack_priority = buffer_read_i16(buf);
        m->legion_recruit_type = buffer_read_i16(buf);
        m->has_military_training = buffer_read_i16(buf);
        m->total_damage = buffer_read_i16(buf);
        m->max_total_damage = buffer_read_i16(buf);
        m->wait_ticks = buffer_read_i16(buf);
        m->recent_fight = buffer_read_i16(buf);
        m->enemy_state.duration_advance = buffer_read_i16(buf);
        m->enemy_state.duration_regroup = buffer_read_i16(buf);
        m->enemy_state.duration_halt = buffer_read_i16(buf);
        m->enemy_legion_index = buffer_read_i16(buf);
        m->is_halted = buffer_read_i16(buf);
        m->missile_fired = buffer_read_i16(buf);
        m->missile_attack_timeout = buffer_read_i16(buf);
        m->missile_attack_formation_id = buffer_read_i16(buf);
        m->prev.layout = buffer_read_i16(buf);
        m->cursed_by_mars = buffer_read_i16(buf);
        m->months_low_morale = buffer_read_u8(buf);
        m->empire_service = buffer_read_u8(buf);
        m->in_distant_battle = buffer_read_u8(buf);
        m->is_herd = buffer_read_u8(buf);
        m->enemy_img_group = buffer_read_u8(buf);
        m->direction = buffer_read_u8(buf);
        m->prev.x_home = buffer_read_u8(buf);
        m->prev.y_home = buffer_read_u8(buf);
        m->orientation = buffer_read_u8(buf);
        m->months_from_home = buffer_read_u8(buf);
        m->months_very_low_morale = buffer_read_u8(buf);
        m->invasion_id = buffer_read_u8(buf);
        m->herd_wolf_spawn_delay = buffer_read_u8(buf);
    }
}
