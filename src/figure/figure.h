#ifndef FIGURE_FIGURE_H
#define FIGURE_FIGURE_H

#include "core/buffer.h"
#include "game/orientation.h"
#include "figure/type.h"

#define MAX_FIGURES 1000
#define MAX_RANGED_TARGETERS_PER_UNIT 4
#define MAX_MELEE_TARGETERS_PER_UNIT 4
#define MAX_MELEE_COMBATANTS_PER_UNIT 4

struct figure_t {
    int id;
    uint8_t is_targetable;
    uint8_t is_unarmed_civilian_unit;
    uint8_t is_friendly_armed_unit; // excl. player legions
    uint8_t is_player_legion_unit;
    uint8_t is_criminal_unit;
    uint8_t is_empire_trader;
    uint8_t is_native_unit;
    uint8_t is_herd_animal;
    uint8_t is_enemy_unit; // excl. Caesar legions
    uint8_t is_caesar_legion_unit;
    uint8_t type;
    uint8_t state;
    uint8_t action_state;
    uint8_t action_state_before_attack;
    uint8_t formation_id;
    uint8_t index_in_formation;
    uint8_t damage;
    uint8_t max_damage;
    uint8_t melee_attack_value;
    uint8_t melee_defense_value;
    uint8_t missile_attack_value;
    uint8_t missile_defense_value;
    uint8_t missile_delay;
    uint8_t missile_type;
    uint8_t max_range;
    uint8_t is_military_trained;
    uint8_t mounted_charge_ticks;
    uint8_t mounted_charge_ticks_max;
    uint16_t target_figure_id;
    uint16_t melee_targeter_ids[MAX_MELEE_TARGETERS_PER_UNIT];
    uint16_t melee_combatant_ids[MAX_MELEE_COMBATANTS_PER_UNIT];
    uint8_t num_melee_combatants;
    uint16_t ranged_targeter_ids[MAX_RANGED_TARGETERS_PER_UNIT];
    uint8_t prefect_recent_guard_duty;
    int8_t attack_direction;
    uint8_t source_x;
    uint8_t source_y;
    uint16_t routing_path_id;
    uint16_t routing_path_current_tile;
    uint16_t routing_path_length;
    uint8_t terrain_usage;
    uint8_t speed_multiplier;
    int8_t previous_tile_direction;
    uint8_t previous_tile_x;
    uint8_t previous_tile_y;
    int8_t direction;
    uint8_t progress_on_tile;
    uint8_t x;
    uint8_t y;
    uint16_t grid_offset;
    uint8_t destination_x;
    uint8_t destination_y;
    uint16_t destination_grid_offset; // only used for soldiers
    uint8_t destination_building_id;
    uint8_t figure_is_halted;
    uint8_t use_cross_country;
    uint8_t cc_direction; // 1 = x, 2 = y
    uint16_t cross_country_x; // position = 15 * x + offset on tile
    uint16_t cross_country_y; // position = 15 * y + offset on tile
    int16_t cc_delta_x;
    int16_t cc_delta_y;
    int16_t cc_delta_xy;
    uint16_t cc_destination_x;
    uint16_t cc_destination_y;
    uint8_t missile_offset;
    uint16_t roam_length;
    uint16_t max_roam_length;
    uint8_t roam_choose_destination;
    uint8_t roam_random_counter;
    int8_t roam_turn_direction;
    int8_t roam_ticks_until_next_turn;
    uint8_t in_building_wait_ticks;
    uint8_t height_adjusted_ticks;
    uint8_t current_height;
    uint8_t target_height;
    uint8_t is_boat; // 1 for boat, 2 for flotsam
    uint16_t next_figure_id_on_same_tile;
    uint16_t image_id;
    uint8_t image_offset;
    uint8_t attack_image_offset;
    uint16_t cart_image_id;
    int8_t x_offset_cart;
    int8_t y_offset_cart;
    uint8_t enemy_image_type;
    uint8_t enemy_image_type_detailed;
    int16_t wait_ticks;
    uint8_t wait_ticks_missile;
    uint16_t name_id;
    uint8_t is_ghost;
    uint16_t building_id; // for missiles: building_id contains the figure that shot it
    uint16_t immigrant_building_id;
    uint8_t migrant_num_people;
    uint8_t min_max_seen;
    uint8_t phrase_sequence_exact;
    int8_t phrase_id;
    uint8_t phrase_sequence_city;
    uint8_t empire_city_id;
    uint8_t resource_id;
    uint8_t collecting_item_id; // NOT a resource ID for cartpushers! IS a resource ID for warehousemen
    uint8_t trader_id;
    uint16_t leading_figure_id;
    uint8_t trader_amount_bought;
    uint8_t loads_sold_or_carrying;
    uint8_t trade_ship_failed_dock_attempts;
    uint8_t flotsam_visible;
};

extern struct figure_t figures[MAX_FIGURES];

/**
 * Creates a figure
 * @param type Figure type
 * @param x X position
 * @param y Y position
 * @param dir Direction the figure faces
 * @return Always a figure. If figure->id is zero, it is an invalid one.
 */
struct figure_t *figure_create(int type, int x, int y, direction_type dir);

void figure_delete(struct figure_t *f);

int figure_is_dead(const struct figure_t *f);

void figure_handle_corpse(struct figure_t *f);

int city_figures_total_invading_enemies(void);

void figure_init_scenario(void);

void figure_save_state(buffer *list);

void figure_load_state(buffer *list);

#endif // FIGURE_FIGURE_H
