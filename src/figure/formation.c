#include "formation.h"

#include "city/data_private.h"
#include "city/military.h"
#include "core/calc.h"
#include "figure/enemy_army.h"
#include "figure/formation_enemy.h"
#include "figure/formation_herd.h"
#include "figure/formation_legion.h"
#include "figure/route.h"
#include "map/grid.h"
#include "sound/effect.h"

#include <string.h>

struct formation_t formations[MAX_FORMATIONS];

static int selected_formation;

static const int FORMATION_LAYOUT_POSITION_X[FORMATION_MAX][MAX_FORMATION_FIGURES] = {
    {0, 1, 0, 1, -1, -1, 0, 1, -1, 2, 2, 2, 0, 1, -1, 2}, // FORMATION_TORTOISE
    {0, 0, -1, 1, -1, 1, -2, -2, 2, 2, -3, -3, 3, 3, -4, -4}, // FORMATION_DOUBLE_LINE_1
    {0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, // FORMATION_DOUBLE_LINE_2
    {0, 2, -2, 1, -1, 3, -3, 4, -4, 5, 6, -5, -6, 7, 8, -7}, // FORMATION_SINGLE_LINE_1
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1}, // FORMATION_SINGLE_LINE_2
    {0, 1, 0, 1, 2, 2, 1, 0, 2, 3, 3, 3, 1, 2, 0, 3}, // FORMATION_MOP_UP
    {0, 1, 0, 1, 2, 2, 1, 0, 2, 3, 3, 3, 1, 2, 0, 3}, // FORMATION_AT_REST
    {0, -3, -1, 0, 2, 2, 3, 4, 2, 3, 0, -3, 2, -1, -3, 0}, // FORMATION_ENEMY_MOB
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // FORMATION_ENEMY_DOUBLE_LINE (FORMATION_DOUBLE_LINE_1 or 2 based on map orientation)
    {0, 2, 0, 2, -2, -2, 0, 2, -2, 4, 4, 4, 0, 2, -2, 4}, // FORMATION_ENEMY_WIDE_COLUMN
    {0, 2, -1, 1, 1, -1, 3, -2, 0, -4, -1, 0, 1, 4, 2, -5}, // FORMATION_HERD
};
static const int FORMATION_LAYOUT_POSITION_Y[FORMATION_MAX][MAX_FORMATION_FIGURES] = {
    {0, 0, 1, 1, 0, 1, -1, -1, -1, -1, 0, 1, 2, 2, 2, 2}, // FORMATION_TORTOISE
    {0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, // FORMATION_DOUBLE_LINE_1
    {0, -1, 1, 0, -1, 1, -2, -2, 2, 2, -3, -3, 3, 3, -4, -4}, // FORMATION_DOUBLE_LINE_2
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1}, // FORMATION_SINGLE_LINE_1
    {0, -2, 2, -1, 1, -3, 3, -4, 4, -5, -6, 5, 6, -7, -8, 7}, // FORMATION_SINGLE_LINE_2
    {0, 0, 1, 1, 0, 1, 2, 2, 2, 0, 1, 2, 3, 3, 3, 3}, // FORMATION_MOP_UP
    {0, 0, 1, 1, 0, 1, 2, 2, 2, 0, 1, 2, 3, 3, 3, 3}, // FORMATION_AT_REST
    {0, -2, 0, 1, 0, 1, 1, 2, -2, -1, -3, 1, -1, 2, 2, -2}, // FORMATION_ENEMY_MOB
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // FORMATION_ENEMY_DOUBLE_LINE (FORMATION_DOUBLE_LINE_1 or 2 based on map orientation)
    {0, 0, 2, 2, 0, 2, -2, -2, -2, -2, 0, 2, 4, 4, 4, 4}, // FORMATION_ENEMY_WIDE_COLUMN
    {0, 1, -1, 1, 0, 1, 1, -1, 2, 0, 3, 5, 4, 0, 3, 2}, // FORMATION_HERD
};

int formation_layout_position_x(int layout, int index)
{
    return FORMATION_LAYOUT_POSITION_X[layout][index];
}

int formation_layout_position_y(int layout, int index)
{
    return FORMATION_LAYOUT_POSITION_Y[layout][index];
}

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

struct formation_t *create_formation_type(int type)
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

void add_figure_to_formation(figure *f, struct formation_t *m)
{
    for (int i = 0; i < m->max_figures; i++) {
        if (!m->figures[i]) {
            m->figures[i] = f->id;
            m->num_figures++;
            f->index_in_formation = i;
            return;
        }
    }
}

void refresh_formation_figure_indexes(figure *unit_to_remove)
{
    struct formation_t *m = &formations[unit_to_remove->formation_id];
    if (m->in_use) {
        for (int i = 0; i < m->num_figures; i++) {
            m->figures[i] = 0;
        }
        m->num_figures = 0;
        for (int i = 1; i < MAX_FIGURES; i++) {
            figure *unit = figure_get(i);
            if (!figure_is_dead(unit) && unit->type == m->figure_type && unit->formation_id == m->id) {
                for (int j = 0; j < m->max_figures; j++) {
                    if (!m->figures[j]) {
                        m->figures[j] = unit->id;
                        m->num_figures++;
                        unit->index_in_formation = j;
                        break;
                    }
                }
            }
        }
    }
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
            if (formations[i].is_at_rest) {
                formations[i].deployed_duration_months = 0;
                formations[i].morale = calc_bound(formations[i].morale + 5, 0, formations[i].max_morale);
            } else if (!formations[i].recent_fight) {
                formations[i].deployed_duration_months++;
                if (formations[i].deployed_duration_months > 3) {
                    formations[i].morale = calc_bound(formations[i].morale - 5, 0, formations[i].max_morale);
                }
            }
        }
    }
}

void formation_adjust_counters(struct formation_t *m)
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

    if (m->figure_type == FIGURE_FORT_MOUNTED && m->is_at_rest) {
        for (int i = 0; i < m->num_figures; i++) {
            figure *f = figure_get(m->figures[i]);
            if (f->mounted_charge_ticks < f->mounted_charge_ticks_max) {
                f->mounted_charge_ticks += 2;
            }
        }
    }
}

void formation_clear_counters(struct formation_t *m)
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
    enemy_army_totals_clear();
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && !formations[i].is_herd) {
            if (formations[i].is_legion) {
                if (formations[i].num_figures) {
                    int total_strength = formations[i].num_figures;
                    if (formations[i].figure_type == FIGURE_FORT_LEGIONARY) {
                        total_strength += formations[i].num_figures / 2;
                    }
                    enemy_army_totals_add_legion_formation(total_strength);
                }
            } else {
                // enemy
                enemy_army_totals_add_enemy_formation(formations[i].num_figures);
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
    // clear empty non-legion formations
    for (int i = 1; i < MAX_FORMATIONS; i++) {
        if (formations[i].in_use && !formations[i].num_figures && !formations->is_legion) {
            int all_units_decayed = 1;
            for (int j = 0; j < formations[i].max_figures; j++) {
                if (figure_get(formations[i].figures[j])->state != FIGURE_STATE_DEAD) {
                    all_units_decayed = 0;
                    break;
                }
            }
            if (all_units_decayed) {
                formation_clear(formations[i].id);
            }
        }
    }
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
        buffer_write_u8(buf, m->is_at_rest);
        buffer_write_u8(buf, m->legion_standard__figure_id);
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
        buffer_write_i16(buf, m->has_military_training);
        buffer_write_i16(buf, m->wait_ticks);
        buffer_write_i16(buf, m->recent_fight);
        buffer_write_i16(buf, m->enemy_state.duration_advance);
        buffer_write_i16(buf, m->enemy_state.duration_regroup);
        buffer_write_i16(buf, m->enemy_state.duration_halt);
        buffer_write_i16(buf, m->enemy_legion_index);
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
        m->is_at_rest = buffer_read_u8(buf);
        m->legion_standard__figure_id = buffer_read_u8(buf);
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
        m->has_military_training = buffer_read_i16(buf);
        m->wait_ticks = buffer_read_i16(buf);
        m->recent_fight = buffer_read_i16(buf);
        m->enemy_state.duration_advance = buffer_read_i16(buf);
        m->enemy_state.duration_regroup = buffer_read_i16(buf);
        m->enemy_state.duration_halt = buffer_read_i16(buf);
        m->enemy_legion_index = buffer_read_i16(buf);
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
