#ifndef CORE_HOTKEY_CONFIG_H
#define CORE_HOTKEY_CONFIG_H

#include "input/keys.h"

typedef enum {
    HOTKEY_ARROW_UP,
    HOTKEY_ARROW_DOWN,
    HOTKEY_ARROW_LEFT,
    HOTKEY_ARROW_RIGHT,
    HOTKEY_TOGGLE_FULLSCREEN,
    HOTKEY_RESET_WINDOW,
    HOTKEY_SAVE_SCREENSHOT,
    HOTKEY_SAVE_CITY_SCREENSHOT,
    HOTKEY_LOAD_FILE,
    HOTKEY_SAVE_FILE,
    HOTKEY_DECREASE_GAME_SPEED,
    HOTKEY_INCREASE_GAME_SPEED,
    HOTKEY_TOGGLE_PAUSE,
    HOTKEY_ROTATE_MAP_LEFT,
    HOTKEY_ROTATE_MAP_RIGHT,
    HOTKEY_REPLAY_MAP,
    HOTKEY_CYCLE_LEGION,
    HOTKEY_RETURN_LEGIONS_TO_FORT,
    HOTKEY_SHOW_LAST_ADVISOR,
    HOTKEY_SHOW_EMPIRE_MAP,
    HOTKEY_SHOW_MESSAGES,
    HOTKEY_BUILD_CLONE,
    HOTKEY_BUILD_VACANT_HOUSE,
    HOTKEY_BUILD_CLEAR_LAND,
    HOTKEY_BUILD_ROAD,
    HOTKEY_BUILD_FOUNTAIN,
    HOTKEY_BUILD_BARBER,
    HOTKEY_BUILD_BATHHOUSE,
    HOTKEY_BUILD_DOCTOR,
    HOTKEY_BUILD_SMALL_TEMPLES,
    HOTKEY_BUILD_SCHOOL,
    HOTKEY_BUILD_LIBRARY,
    HOTKEY_BUILD_THEATER,
    HOTKEY_BUILD_AMPHITHEATER,
    HOTKEY_BUILD_GLADIATOR_SCHOOL,
    HOTKEY_BUILD_ACTOR_COLONY,
    HOTKEY_BUILD_FORUM,
    HOTKEY_BUILD_SMALL_STATUE,
    HOTKEY_BUILD_MEDIUM_STATUE,
    HOTKEY_BUILD_GARDENS,
    HOTKEY_BUILD_PLAZA,
    HOTKEY_BUILD_ENGINEERS_POST,
    HOTKEY_BUILD_PREFECTURE,
    HOTKEY_BUILD_MARKET,
    HOTKEY_SHOW_OVERLAY_WATER,
    HOTKEY_SHOW_OVERLAY_FIRE,
    HOTKEY_SHOW_OVERLAY_DAMAGE,
    HOTKEY_SHOW_OVERLAY_CRIME,
    HOTKEY_SHOW_OVERLAY_PROBLEMS,
    HOTKEY_GO_TO_BOOKMARK_1,
    HOTKEY_GO_TO_BOOKMARK_2,
    HOTKEY_GO_TO_BOOKMARK_3,
    HOTKEY_GO_TO_BOOKMARK_4,
    HOTKEY_SET_BOOKMARK_1,
    HOTKEY_SET_BOOKMARK_2,
    HOTKEY_SET_BOOKMARK_3,
    HOTKEY_SET_BOOKMARK_4,
    HOTKEY_EDITOR_TOGGLE_BATTLE_INFO,
    HOTKEY_CHEAT_MONEY,
    HOTKEY_CHEAT_INVASION,
    HOTKEY_CHEAT_VICTORY,
    HOTKEY_MAX_ITEMS
} hotkey_action;

typedef struct {
    key_type key;
    key_modifier_type modifiers;
    hotkey_action action;
} hotkey_mapping;

/**
 * Get mapping for action at the specified index
 * @param action Action
 * @param index Index
 * @return Mapping or NULL if not set
 */
const hotkey_mapping *hotkey_for_action(hotkey_action action, int index);

/**
 * Get default mapping for action
 * @param action Action
 * @param index Index, can be 0 or 1
 * @return Mapping, may be an empty mapping. Only returns NULL on invalid input
 */
const hotkey_mapping *hotkey_default_for_action(hotkey_action action, int index);

/**
 * Clear all hotkey mappings
 */
void hotkey_config_clear(void);

/**
 * Add a mapping
 * @param mapping Mapping to add
 */
void hotkey_config_add_mapping(const hotkey_mapping *mapping);

/**
 * Load hotkey config from file and install hotkeys
 */
void hotkey_config_load(void);

/**
 * Save hotkey config to file and install hotkeys
 */
void hotkey_config_save(void);

#endif // CORE_HOTKEY_CONFIG_H
