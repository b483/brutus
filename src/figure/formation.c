#include "formation.h"

#include "city/data_private.h"
#include "city/military.h"
#include "core/calc.h"
#include "figure/formation_enemy.h"
#include "figure/formation_herd.h"
#include "figure/formation_legion.h"
#include "figure/route.h"
#include "map/grid.h"
#include "sound/effect.h"

#include <string.h>

static const int FORMATION_LAYOUT_POSITION_X[FORMATION_MAX][MAX_FORMATION_FIGURES] = {
    {0, 1, 0, 1, -1, -1, 0, 1, -1, 2, 2, 2, 0, 1, -1, 2}, // FORMATION_TORTOISE
    {0, 0, -1, 1, -1, 1, -2, -2, 2, 2, -3, -3, 3, 3, -4, -4}, // FORMATION_DOUBLE_LINE_1
    {0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, // FORMATION_DOUBLE_LINE_2
    {0, 2, -2, 1, -1, 3, -3, 4, -4, 5, 6, -5, -6, 7, 8, -7}, // FORMATION_SINGLE_LINE_1
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1}, // FORMATION_SINGLE_LINE_2
    {0, 1, 0, 1, 2, 2, 1, 0, 2, 3, 3, 3, 1, 2, 0, 3}, // FORMATION_MOP_UP
    {0, 1, 0, 1, 2, 2, 1, 0, 2, 3, 3, 3, 1, 2, 0, 3}, // FORMATION_AT_REST
    {0, -3, -1, 0, 2, 2, 3, 4, 2, 3, 0, -3, 2, -1, -3, 0}, // FORMATION_ENEMY_MOB
    {0, 2, 0, 2, -2, -2, 0, 2, -2, 4, 4, 4, 0, 2, -2, 4}, // FORMATION_ENEMY_WIDE_COLUMN
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
    {0, 0, 2, 2, 0, 2, -2, -2, -2, -2, 0, 2, 4, 4, 4, 4}, // FORMATION_ENEMY_WIDE_COLUMN
};

int formation_layout_position_x(int layout, int index)
{
    return FORMATION_LAYOUT_POSITION_X[layout][index];
}

int formation_layout_position_y(int layout, int index)
{
    return FORMATION_LAYOUT_POSITION_Y[layout][index];
}

void reset_all_formations(void)
{
    selected_legion_formation = -1;
    for (int i = 0; i < MAX_LEGIONS; i++) {
        memset(&legion_formations[i], 0, sizeof(struct formation_t));
        legion_formations[i].id = i;
    }
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        memset(&herd_formations[i], 0, sizeof(struct formation_t));
        herd_formations[i].id = i;
    }
    for (int i = 0; i < MAX_ENEMY_FORMATIONS; i++) {
        memset(&enemy_formations[i], 0, sizeof(struct formation_t));
        enemy_formations[i].id = i;
    }
}

void add_figure_to_formation(struct figure_t *f, struct formation_t *m)
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

void decrease_formation_combat_counters(struct formation_t *m)
{
    if (m->missile_attack_timeout) {
        m->missile_attack_timeout--;
    }
    if (m->recent_fight) {
        m->recent_fight--;
    }
}

void clear_formation_combat_counters(struct formation_t *m)
{
    m->missile_attack_timeout = 0;
    m->recent_fight = 0;
}

void update_formation_morale_after_death(struct formation_t *m)
{
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

void formation_update_all(void)
{
    update_legion_formations();
    update_enemy_formations();
    if (city_data.figure.animals) {
        update_herd_formations();
    }

}

void formation_save_state(buffer *buf, struct formation_t *m)
{
    buffer_write_u8(buf, m->in_use);
    buffer_write_u8(buf, m->layout);
    buffer_write_u8(buf, m->figure_type);
    buffer_write_u8(buf, m->num_figures);
    buffer_write_u8(buf, m->max_figures);
    for (int fig = 0; fig < MAX_FORMATION_FIGURES; fig++) {
        buffer_write_u16(buf, m->figures[fig]);
    }
    buffer_write_u8(buf, m->has_military_training);
    buffer_write_u8(buf, m->is_at_rest);
    buffer_write_u8(buf, m->deployed_duration_months);
    buffer_write_u8(buf, m->direction);
    buffer_write_u8(buf, m->morale);
    buffer_write_u8(buf, m->max_morale);
    buffer_write_u8(buf, m->routed);
    buffer_write_i16(buf, m->wait_ticks_movement);
    buffer_write_u16(buf, m->standard_x);
    buffer_write_u16(buf, m->standard_y);
    buffer_write_u16(buf, m->prev_standard_x);
    buffer_write_u16(buf, m->prev_standard_y);
    buffer_write_u16(buf, m->legion_standard__figure_id);
    buffer_write_u16(buf, m->building_id);
    buffer_write_u8(buf, m->empire_service);
    buffer_write_u8(buf, m->in_distant_battle);
    buffer_write_u8(buf, m->cursed_by_mars);
    buffer_write_u8(buf, m->recent_fight);
    buffer_write_u8(buf, m->missile_attack_timeout);
    buffer_write_u16(buf, m->destination_x);
    buffer_write_u16(buf, m->destination_y);
    buffer_write_u16(buf, m->wolf_spawn_delay);
    buffer_write_u8(buf, m->attack_priority);
}

void formation_load_state(buffer *buf, struct formation_t *m)
{
    m->in_use = buffer_read_u8(buf);
    m->layout = buffer_read_u8(buf);
    m->figure_type = buffer_read_u8(buf);
    m->num_figures = buffer_read_u8(buf);
    m->max_figures = buffer_read_u8(buf);
    for (int fig = 0; fig < MAX_FORMATION_FIGURES; fig++) {
        m->figures[fig] = buffer_read_u16(buf);
    }
    m->has_military_training = buffer_read_u8(buf);
    m->is_at_rest = buffer_read_u8(buf);
    m->deployed_duration_months = buffer_read_u8(buf);
    m->direction = buffer_read_u8(buf);
    m->morale = buffer_read_u8(buf);
    m->max_morale = buffer_read_u8(buf);
    m->routed = buffer_read_u8(buf);
    m->wait_ticks_movement = buffer_read_i16(buf);
    m->standard_x = buffer_read_u16(buf);
    m->standard_y = buffer_read_u16(buf);
    m->prev_standard_x = buffer_read_u16(buf);
    m->prev_standard_y = buffer_read_u16(buf);
    m->legion_standard__figure_id = buffer_read_u16(buf);
    m->building_id = buffer_read_u16(buf);
    m->empire_service = buffer_read_u8(buf);
    m->in_distant_battle = buffer_read_u8(buf);
    m->cursed_by_mars = buffer_read_u8(buf);
    m->recent_fight = buffer_read_u8(buf);
    m->missile_attack_timeout = buffer_read_u8(buf);
    m->destination_x = buffer_read_u16(buf);
    m->destination_y = buffer_read_u16(buf);
    m->wolf_spawn_delay = buffer_read_u16(buf);
    m->attack_priority = buffer_read_u8(buf);
}