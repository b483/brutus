#ifndef FIGURE_FIGURE_H
#define FIGURE_FIGURE_H

#include "core/buffer.h"
#include "game/orientation.h"
#include "figure/type.h"

#define MAX_FIGURES 1000
#define MAX_RANGED_TARGETERS_PER_UNIT 4
#define MAX_MELEE_TARGETERS_PER_UNIT 4
#define MAX_MELEE_COMBATANTS_PER_UNIT 4

typedef struct {
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
    uint8_t figure_is_halted;
    short image_id;
    short cart_image_id;
    unsigned char image_offset;
    int enemy_image_type;
    int enemy_image_type_detailed;
    uint8_t is_military_trained;
    unsigned char flotsam_visible;
    short next_figure_id_on_same_tile;
    unsigned char type;
    unsigned char resource_id;
    unsigned char use_cross_country;
    unsigned char state;
    unsigned char action_state_before_attack;
    signed char direction;
    signed char previous_tile_direction;
    signed char attack_direction;
    unsigned char x;
    unsigned char y;
    unsigned char previous_tile_x;
    unsigned char previous_tile_y;
    unsigned char missile_offset;
    unsigned char damage;
    uint8_t max_damage;
    uint8_t melee_attack_value;
    uint8_t melee_defense_value;
    uint8_t missile_attack_value;
    uint8_t missile_defense_value;
    uint8_t missile_delay;
    uint8_t missile_type;
    short grid_offset;
    unsigned char destination_x;
    unsigned char destination_y;
    short destination_grid_offset; // only used for soldiers
    unsigned char source_x;
    unsigned char source_y;
    short wait_ticks;
    unsigned char action_state;
    unsigned char progress_on_tile;
    short routing_path_id;
    short routing_path_current_tile;
    short routing_path_length;
    unsigned char in_building_wait_ticks;
    unsigned char is_on_road;
    short max_roam_length;
    short roam_length;
    unsigned char roam_choose_destination;
    unsigned char roam_random_counter;
    signed char roam_turn_direction;
    signed char roam_ticks_until_next_turn;
    short cross_country_x; // position = 15 * x + offset on tile
    short cross_country_y; // position = 15 * y + offset on tile
    short cc_destination_x;
    short cc_destination_y;
    short cc_delta_x;
    short cc_delta_y;
    short cc_delta_xy;
    unsigned char cc_direction; // 1 = x, 2 = y
    unsigned char speed_multiplier;
    short building_id; // for missiles: building_id contains the figure that shot it
    short immigrant_building_id;
    short destination_building_id;
    short formation_id;
    unsigned char index_in_formation;
    unsigned char migrant_num_people;
    unsigned char is_ghost;
    unsigned char min_max_seen;
    short leading_figure_id;
    unsigned char attack_image_offset;
    uint8_t mounted_charge_ticks;
    uint8_t mounted_charge_ticks_max;
    unsigned char wait_ticks_missile;
    signed char x_offset_cart;
    signed char y_offset_cart;
    unsigned char empire_city_id;
    unsigned char trader_amount_bought;
    short name;
    unsigned char terrain_usage;
    unsigned char loads_sold_or_carrying;
    unsigned char is_boat; // 1 for boat, 2 for flotsam
    unsigned char height_adjusted_ticks;
    unsigned char current_height;
    unsigned char target_height;
    unsigned char collecting_item_id; // NOT a resource ID for cartpushers! IS a resource ID for warehousemen
    unsigned char trade_ship_failed_dock_attempts;
    unsigned char phrase_sequence_exact;
    signed char phrase_id;
    unsigned char phrase_sequence_city;
    unsigned char trader_id;
    uint8_t prefect_recent_guard_duty;
    uint8_t max_range;
    uint16_t ranged_targeter_ids[MAX_RANGED_TARGETERS_PER_UNIT];
    uint16_t target_figure_id;
    uint16_t melee_targeter_ids[MAX_MELEE_TARGETERS_PER_UNIT];
    uint16_t melee_combatant_ids[MAX_MELEE_COMBATANTS_PER_UNIT];
    uint8_t num_melee_combatants;
} figure;

figure *figure_get(int id);

/**
 * Creates a figure
 * @param type Figure type
 * @param x X position
 * @param y Y position
 * @param dir Direction the figure faces
 * @return Always a figure. If figure->id is zero, it is an invalid one.
 */
figure *figure_create(int type, int x, int y, direction_type dir);

void figure_delete(figure *f);

int figure_is_dead(const figure *f);

void figure_handle_corpse(figure *f);

int city_figures_total_invading_enemies(void);

void figure_init_scenario(void);

void figure_save_state(buffer *list);

void figure_load_state(buffer *list);

#endif // FIGURE_FIGURE_H
