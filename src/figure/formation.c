#include "formation.h"

#include "city/data_private.h"
#include "city/military.h"
#include "core/calc.h"
#include "figure/enemy_army.h"
#include "figure/figure.h"
#include "figure/formation_enemy.h"
#include "figure/formation_herd.h"
#include "figure/formation_legion.h"
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
    m->morale = calc_bound(m->morale + morale, 0, m->max_morale);
}

void legions_update_morale_monthly(void)
{
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && formations[i].is_legion && !formations[i].in_distant_battle) {
            if (formations[i].is_at_fort) {
                formations[i].deployed_duration_months = 0;
                formations[i].morale = calc_bound(formations[i].morale + 5, 0, formations[i].max_morale);
                formation_legion_restore_layout(&formations[i]);
            } else if (!formations[i].recent_fight) {
                formations[i].deployed_duration_months++;
                if (formations[i].deployed_duration_months > 3) {
                    formations[i].morale = calc_bound(formations[i].morale - 5, 0, formations[i].max_morale);
                }
            }
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
        if (!f->is_player_legion_unit && !f->is_enemy_unit && !f->is_herd_animal) {
            continue;
        }
        formations[f->formation_id].num_figures++;
        formations[f->formation_id].total_damage += f->damage;
        formations[f->formation_id].max_total_damage += f->max_damage;
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
        if (formations[f->formation_id].figure_type == FIGURE_FORT_MOUNTED && formations[f->formation_id].is_at_fort
            && f->mounted_charge_ticks < f->mounted_charge_ticks_max) {
            f->mounted_charge_ticks++;
        }
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

void formation_update_all(void)
{
    formation_calculate_legion_totals();
    formation_calculate_figures();
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && !formations[i].is_herd) {
            update_direction(formations[i].id, figure_get(formations[i].figures[0])->direction);
        }
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
        buffer_write_u8(buf, m->max_morale);
        buffer_write_u8(buf, m->x_home);
        buffer_write_u8(buf, m->y_home);
        buffer_write_u8(buf, m->standard_x);
        buffer_write_u8(buf, m->standard_y);
        buffer_write_u8(buf, m->x);
        buffer_write_u8(buf, m->y);
        buffer_write_u8(buf, m->destination_x);
        buffer_write_u8(buf, m->destination_y);
        buffer_write_i16(buf, m->destination_building_id);
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
        buffer_write_u8(buf, m->empire_service);
        buffer_write_u8(buf, m->in_distant_battle);
        buffer_write_u8(buf, m->is_herd);
        buffer_write_u8(buf, m->enemy_img_group);
        buffer_write_u8(buf, m->direction);
        buffer_write_u8(buf, m->prev.x_home);
        buffer_write_u8(buf, m->prev.y_home);
        buffer_write_u8(buf, m->orientation);
        buffer_write_u8(buf, m->deployed_duration_months);
        buffer_write_u8(buf, m->routed);
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
        m->max_morale = buffer_read_u8(buf);
        m->x_home = buffer_read_u8(buf);
        m->y_home = buffer_read_u8(buf);
        m->standard_x = buffer_read_u8(buf);
        m->standard_y = buffer_read_u8(buf);
        m->x = buffer_read_u8(buf);
        m->y = buffer_read_u8(buf);
        m->destination_x = buffer_read_u8(buf);
        m->destination_y = buffer_read_u8(buf);
        m->destination_building_id = buffer_read_i16(buf);
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
        m->empire_service = buffer_read_u8(buf);
        m->in_distant_battle = buffer_read_u8(buf);
        m->is_herd = buffer_read_u8(buf);
        m->enemy_img_group = buffer_read_u8(buf);
        m->direction = buffer_read_u8(buf);
        m->prev.x_home = buffer_read_u8(buf);
        m->prev.y_home = buffer_read_u8(buf);
        m->orientation = buffer_read_u8(buf);
        m->deployed_duration_months = buffer_read_u8(buf);
        m->routed = buffer_read_u8(buf);
        m->invasion_id = buffer_read_u8(buf);
        m->herd_wolf_spawn_delay = buffer_read_u8(buf);
    }
}
