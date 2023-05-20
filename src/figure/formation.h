#ifndef FIGURE_FORMATION_H
#define FIGURE_FORMATION_H

#include "core/buffer.h"
#include "figure/figure.h"

#define MAX_FORMATION_FIGURES 16
#define ROUT_MORALE_THRESHOLD 20

enum {
    FORMATION_ATTACK_FOOD_CHAIN = 0,
    FORMATION_ATTACK_GOLD_STORES = 1,
    FORMATION_ATTACK_BEST_BUILDINGS = 2,
    FORMATION_ATTACK_TROOPS = 3,
    FORMATION_ATTACK_RANDOM = 4
};

enum {
    FORMATION_TORTOISE = 0,
    FORMATION_DOUBLE_LINE_1 = 1,
    FORMATION_DOUBLE_LINE_2 = 2,
    FORMATION_SINGLE_LINE_1 = 3,
    FORMATION_SINGLE_LINE_2 = 4,
    FORMATION_MOP_UP = 5,
    FORMATION_AT_REST = 6,
    FORMATION_ENEMY_MOB = 7,
    FORMATION_ENEMY_WIDE_COLUMN = 8,
    FORMATION_MAX = 9
};

struct formation_t {
    int id;
    uint8_t in_use;
    uint8_t layout;
    uint8_t figure_type;
    uint8_t num_figures;
    uint8_t max_figures;
    uint16_t figures[MAX_FORMATION_FIGURES];
    uint8_t has_military_training;
    uint8_t is_at_rest;
    uint8_t deployed_duration_months;
    uint8_t direction;
    uint8_t morale;
    uint8_t max_morale;
    uint8_t routed;
    int16_t wait_ticks_movement;
    uint16_t standard_x;
    uint16_t standard_y;
    uint16_t prev_standard_x;
    uint16_t prev_standard_y;
    uint16_t legion_standard__figure_id;
    uint16_t building_id;
    uint8_t empire_service;
    uint8_t in_distant_battle;
    uint8_t cursed_by_mars;
    uint8_t recent_fight;
    uint8_t missile_attack_timeout;
    uint16_t destination_x;
    uint16_t destination_y;
    uint16_t wolf_spawn_delay;
    uint8_t attack_priority;
};

int formation_layout_position_x(int layout, int index);

int formation_layout_position_y(int layout, int index);

void reset_all_formations(void);

void add_figure_to_formation(struct figure_t *f, struct formation_t *m);

void refresh_formation_figure_indexes(struct figure_t *unit_to_remove);

void decrease_formation_combat_counters(struct formation_t *m);

void clear_formation_combat_counters(struct formation_t *m);

void update_formation_morale_after_death(struct formation_t *m);

void formation_update_all(void);

void formation_save_state(buffer *buf, struct formation_t *m);
void formation_load_state(buffer *buf, struct formation_t *m);

#endif // FIGURE_FORMATION_H
