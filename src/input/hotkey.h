#ifndef INPUT_HOTKEY_H
#define INPUT_HOTKEY_H

#include "core/hotkey_config.h"
#include "input/keys.h"

typedef struct {
    // fixed keys with multiple functions
    int enter_pressed;
    int escape_pressed;
    // keys with specific function
    int load_file;
    int save_file;
    int decrease_game_speed;
    int increase_game_speed;
    int toggle_pause;
    int rotate_map_left;
    int rotate_map_right;
    int replay_map;
    int cycle_legion;
    int return_legions_to_fort;
    int show_last_advisor;
    int show_empire_map;
    int show_messages;
    int clone_building;
    int building;
    int show_overlay;
    int go_to_bookmark;
    int set_bookmark;
    int toggle_editor_battle_info;
    int cheat_money;
    int cheat_invasion;
    int cheat_victory;
} hotkeys;

void hotkey_install_mapping(hotkey_mapping *mappings, int num_mappings);

const hotkeys *hotkey_state(void);
void hotkey_reset_state(void);

void hotkey_key_pressed(key_type key, key_modifier_type modifiers, int repeat);
void hotkey_key_released(key_type key, key_modifier_type modifiers);

void hotkey_set_value_for_action(hotkey_action action, int value);

void hotkey_handle_global_keys(void);

#endif // INPUT_HOTKEY_H
