#ifndef CORE_CONFIG_H
#define CORE_CONFIG_H

#define CONFIG_STRING_VALUE_MAX 32
#define MAX_PLAYER_NAME_LENGTH 24

typedef enum {
    CONFIG_SCREEN_DISPLAY_SCALE,
    CONFIG_SCREEN_CURSOR_SCALE,
    CONFIG_UI_SIDEBAR_INFO,
    CONFIG_UI_SHOW_INTRO_VIDEO,
    CONFIG_UI_DISABLE_MOUSE_EDGE_SCROLLING,
    CONFIG_UI_DISABLE_RIGHT_CLICK_MAP_DRAG,
    CONFIG_UI_VISUAL_FEEDBACK_ON_DELETE,
    CONFIG_UI_HIGHLIGHT_LEGIONS,
    CONFIG_MAX_ENTRIES
} config_key;

typedef enum {
    CONFIG_STRING_PLAYER_NAME,
    CONFIG_STRING_MAX_ENTRIES
} config_string_key;

/**
 * Get an integer config value
 * @param key Integer key
 * @return Config value
 */
int config_get(config_key key);

/**
 * Set an integer config value
 * @param key Integer key
 * @param value Value to set
 */
void config_set(config_key key, int value);

/**
 * Get a string config value
 * @param key String key
 * @return Config value, is always non-NULL but may be an empty string
 */
const char *config_get_string(config_string_key key);

/**
 * Set a string config value
 * @param key String key
 * @param value Value to set
 */
void config_set_string(config_string_key key, const char *value);

/**
 * Set a default config value
 * @param key Integer key
 * @return Default config value
 */
int config_get_default_value(config_key key);

/**
 * Get a string default config value
 * @param key String key
 * @return Default config value, is always non-NULL but may be an empty string
 */
const char *config_get_default_string_value(config_string_key key);

void set_player_name_from_config(void);

/**
 * Load config from file
 */
void config_load(void);

/**
 * Save config to file
 */
void config_save(void);

#endif // CORE_CONFIG_H
