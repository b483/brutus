#ifndef FIGURE_FORMATION_H
#define FIGURE_FORMATION_H

#include "core/buffer.h"
#include "figure/figure.h"

#define MAX_FORMATIONS 50

#define MAX_LEGIONS 6
#define MAX_FORMATION_FIGURES 16
#define ROUT_MORALE_THRESHOLD 20

#define WOLF_PACK_SIZE 8
#define SHEEP_HERD_SIZE 10
#define ZEBRA_HERD_SIZE 12
#define MAX_WOLF_ROAM_DISTANCE 16
#define MAX_SHEEP_ROAM_DISTANCE 8
#define MAX_ZEBRA_ROAM_DISTANCE 20

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
    FORMATION_HERD = 9,
    FORMATION_MAX = 10
};

/**
 * Formation data
 */
struct formation_t {
    uint8_t id;

    /* General variables */
    uint8_t in_use;
    uint8_t layout;
    uint8_t direction;
    uint8_t morale;
    uint8_t max_morale;
    uint8_t routed;

    /* Figures */
    uint8_t figure_type; /**< Type of figure in this formation */
    uint8_t num_figures; /**< Current number of figures in the formation */
    uint8_t max_figures; /**< Maximum number of figures */
    uint16_t figures[MAX_FORMATION_FIGURES]; /**< Figure IDs */

    /* Position */
    uint16_t building_id;
    uint16_t standard_x;
    uint16_t standard_y;
    uint16_t prev_standard_x;
    uint16_t prev_standard_y;
    uint16_t destination_x;
    uint16_t destination_y;

    /* Movement */
    int16_t wait_ticks;
    uint8_t recent_fight;
    uint8_t missile_attack_timeout;
    uint8_t missile_attack_formation_id;

    /* Legion-related */
    uint8_t is_legion; /**< Flag to indicate (own) legion */
    uint8_t legion_id; /**< Legion ID (0-5 for own troops) */
    uint16_t legion_standard__figure_id;
    uint8_t empire_service; /**< Flag to indicate this legion is selected for empire service */
    uint8_t in_distant_battle; /**< Flag to indicate this legion is away in a distant battle */
    uint8_t cursed_by_mars; /**< Flag to indicate this legion is cursed */
    uint8_t has_military_training; /**< Flag to indicate this legion has had military training */
    uint8_t is_at_rest;
    uint8_t deployed_duration_months;

    /* Enemy-related */
    uint8_t attack_priority;

    /* Herd-related */
    uint8_t is_herd; /**< Flag to indicate herd */
    uint16_t herd_wolf_spawn_delay;
};

extern struct formation_t formations[MAX_FORMATIONS];

int formation_layout_position_x(int layout, int index);

int formation_layout_position_y(int layout, int index);

void formations_clear(void);

void formation_clear(int formation_id);

struct formation_t *create_formation_type(int type);

void add_figure_to_formation(struct figure_t *f, struct formation_t *m);

void refresh_formation_figure_indexes(struct figure_t *unit_to_remove);

int formation_get_selected(void);
void formation_set_selected(int formation_id);

void formation_update_morale_after_death(struct formation_t *m);

void legions_update_morale_monthly(void);
void formation_adjust_counters(struct formation_t *m);
void formation_clear_counters(struct formation_t *m);

void formation_update_all(void);

void formations_save_state(buffer *buf);
void formations_load_state(buffer *buf);

#endif // FIGURE_FORMATION_H
