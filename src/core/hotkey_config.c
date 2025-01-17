#include "hotkey_config.h"

#include "core/file.h"
#include "core/log.h"
#include "game/system.h"
#include "input/hotkey.h"

#include <stdio.h>
#include <string.h>

#define MAX_LINE 100
#define MAX_MAPPINGS HOTKEY_MAX_ITEMS * 2

// Keep this in the same order as the actions in hotkey_config.h
static const char *ini_keys[] = {
    "arrow_up",
    "arrow_down",
    "arrow_left",
    "arrow_right",
    "toggle_fullscreen",
    "reset_window",
    "save_screenshot",
    "save_city_screenshot",
    "load_file",
    "save_file",
    "decrease_game_speed",
    "increase_game_speed",
    "toggle_pause",
    "rotate_map_left",
    "rotate_map_right",
    "replay_map",
    "cycle_legion",
    "return_legions_to_fort",
    "show_last_advisor",
    "show_empire_map",
    "show_messages",
    "clone_building",
    "build_vacant_house",
    "build_clear_land",
    "build_road",
    "build_fountain",
    "build_barber",
    "build_bathhouse",
    "build_doctor",
    "build_small_temples",
    "build_school",
    "build_library",
    "build_theater",
    "build_amphitheater",
    "build_gladiator_school",
    "build_actor_colony",
    "build_forum",
    "build_small_statue",
    "build_medium_statue",
    "build_gardens",
    "build_plaza",
    "build_engineers_post",
    "build_prefecture",
    "build_market",
    "show_overlay_water",
    "show_overlay_fire",
    "show_overlay_damage",
    "show_overlay_crime",
    "show_overlay_problems",
    "go_to_bookmark_1",
    "go_to_bookmark_2",
    "go_to_bookmark_3",
    "go_to_bookmark_4",
    "set_bookmark_1",
    "set_bookmark_2",
    "set_bookmark_3",
    "set_bookmark_4",
    "editor_toggle_battle_info",
    "cheat money",
    "cheat invasion",
    "cheat victory",
};

static struct {
    hotkey_mapping default_mappings[HOTKEY_MAX_ITEMS][2];
    hotkey_mapping mappings[MAX_MAPPINGS];
    int num_mappings;
} data;

static void set_mapping(key_type key, key_modifier_type modifiers, hotkey_action action)
{
    hotkey_mapping *mapping = &data.default_mappings[action][0];
    if (mapping->key) {
        mapping = &data.default_mappings[action][1];
    }
    if (mapping->key) {
        return;
    }
    mapping->key = key;
    mapping->modifiers = modifiers;
    mapping->action = action;
}

static void init_defaults(void)
{
    memset(data.default_mappings, 0, sizeof(data.default_mappings));
    // Arrow keys
    set_mapping(KEY_TYPE_UP, KEY_MOD_NONE, HOTKEY_ARROW_UP);
    set_mapping(KEY_TYPE_DOWN, KEY_MOD_NONE, HOTKEY_ARROW_DOWN);
    set_mapping(KEY_TYPE_LEFT, KEY_MOD_NONE, HOTKEY_ARROW_LEFT);
    set_mapping(KEY_TYPE_RIGHT, KEY_MOD_NONE, HOTKEY_ARROW_RIGHT);
    set_mapping(KEY_TYPE_KP_8, KEY_MOD_NONE, HOTKEY_ARROW_UP);
    set_mapping(KEY_TYPE_KP_2, KEY_MOD_NONE, HOTKEY_ARROW_DOWN);
    set_mapping(KEY_TYPE_KP_4, KEY_MOD_NONE, HOTKEY_ARROW_LEFT);
    set_mapping(KEY_TYPE_KP_6, KEY_MOD_NONE, HOTKEY_ARROW_RIGHT);
    // Global hotkeys
    set_mapping(KEY_TYPE_ENTER, KEY_MOD_ALT, HOTKEY_TOGGLE_FULLSCREEN);
    set_mapping(KEY_TYPE_ENTER, KEY_MOD_CTRL, HOTKEY_RESET_WINDOW);
    set_mapping(KEY_TYPE_LEFTBRACKET, KEY_MOD_CTRL, HOTKEY_SAVE_SCREENSHOT);
    set_mapping(KEY_TYPE_RIGHTBRACKET, KEY_MOD_CTRL, HOTKEY_SAVE_CITY_SCREENSHOT);
    set_mapping(KEY_TYPE_L, KEY_MOD_CTRL, HOTKEY_LOAD_FILE);
    set_mapping(KEY_TYPE_S, KEY_MOD_CTRL, HOTKEY_SAVE_FILE);
    // City hotkeys
    set_mapping(KEY_TYPE_D, KEY_MOD_NONE, HOTKEY_DECREASE_GAME_SPEED);
    set_mapping(KEY_TYPE_F, KEY_MOD_NONE, HOTKEY_INCREASE_GAME_SPEED);
    set_mapping(KEY_TYPE_KP_MINUS, KEY_MOD_NONE, HOTKEY_DECREASE_GAME_SPEED);
    set_mapping(KEY_TYPE_KP_PLUS, KEY_MOD_NONE, HOTKEY_INCREASE_GAME_SPEED);
    set_mapping(KEY_TYPE_SPACE, KEY_MOD_NONE, HOTKEY_TOGGLE_PAUSE);
    set_mapping(KEY_TYPE_HOME, KEY_MOD_NONE, HOTKEY_ROTATE_MAP_LEFT);
    set_mapping(KEY_TYPE_END, KEY_MOD_NONE, HOTKEY_ROTATE_MAP_RIGHT);
    set_mapping(KEY_TYPE_R, KEY_MOD_CTRL, HOTKEY_REPLAY_MAP);
    set_mapping(KEY_TYPE_TAB, KEY_MOD_NONE, HOTKEY_CYCLE_LEGION);
    set_mapping(KEY_TYPE_B, KEY_MOD_NONE, HOTKEY_RETURN_LEGIONS_TO_FORT);
    set_mapping(KEY_TYPE_D, KEY_MOD_ALT, HOTKEY_SHOW_LAST_ADVISOR);
    set_mapping(KEY_TYPE_F, KEY_MOD_ALT, HOTKEY_SHOW_EMPIRE_MAP);
    set_mapping(KEY_TYPE_GRAVE, KEY_MOD_NONE, HOTKEY_SHOW_MESSAGES);
    // Construction hotkeys
    set_mapping(KEY_TYPE_G, KEY_MOD_NONE, HOTKEY_BUILD_CLONE);
    set_mapping(KEY_TYPE_Q, KEY_MOD_NONE, HOTKEY_BUILD_VACANT_HOUSE);
    set_mapping(KEY_TYPE_W, KEY_MOD_NONE, HOTKEY_BUILD_CLEAR_LAND);
    set_mapping(KEY_TYPE_E, KEY_MOD_NONE, HOTKEY_BUILD_ROAD);
    set_mapping(KEY_TYPE_R, KEY_MOD_NONE, HOTKEY_BUILD_FOUNTAIN);
    set_mapping(KEY_TYPE_T, KEY_MOD_NONE, HOTKEY_BUILD_BARBER);
    set_mapping(KEY_TYPE_A, KEY_MOD_NONE, HOTKEY_BUILD_BATHHOUSE);
    set_mapping(KEY_TYPE_S, KEY_MOD_NONE, HOTKEY_BUILD_DOCTOR);
    set_mapping(KEY_TYPE_Z, KEY_MOD_NONE, HOTKEY_BUILD_SMALL_TEMPLES);
    set_mapping(KEY_TYPE_X, KEY_MOD_NONE, HOTKEY_BUILD_SCHOOL);
    set_mapping(KEY_TYPE_C, KEY_MOD_NONE, HOTKEY_BUILD_LIBRARY);
    set_mapping(KEY_TYPE_V, KEY_MOD_NONE, HOTKEY_BUILD_THEATER);
    set_mapping(KEY_TYPE_Q, KEY_MOD_ALT, HOTKEY_BUILD_AMPHITHEATER);
    set_mapping(KEY_TYPE_W, KEY_MOD_ALT, HOTKEY_BUILD_GLADIATOR_SCHOOL);
    set_mapping(KEY_TYPE_E, KEY_MOD_ALT, HOTKEY_BUILD_ACTOR_COLONY);
    set_mapping(KEY_TYPE_R, KEY_MOD_ALT, HOTKEY_BUILD_FORUM);
    set_mapping(KEY_TYPE_T, KEY_MOD_ALT, HOTKEY_BUILD_SMALL_STATUE);
    set_mapping(KEY_TYPE_A, KEY_MOD_ALT, HOTKEY_BUILD_MEDIUM_STATUE);
    set_mapping(KEY_TYPE_S, KEY_MOD_ALT, HOTKEY_BUILD_GARDENS);
    set_mapping(KEY_TYPE_Z, KEY_MOD_ALT, HOTKEY_BUILD_PLAZA);
    set_mapping(KEY_TYPE_X, KEY_MOD_ALT, HOTKEY_BUILD_ENGINEERS_POST);
    set_mapping(KEY_TYPE_C, KEY_MOD_ALT, HOTKEY_BUILD_PREFECTURE);
    set_mapping(KEY_TYPE_V, KEY_MOD_ALT, HOTKEY_BUILD_MARKET);
    // Overlays
    set_mapping(KEY_TYPE_1, KEY_MOD_NONE, HOTKEY_SHOW_OVERLAY_WATER);
    set_mapping(KEY_TYPE_2, KEY_MOD_NONE, HOTKEY_SHOW_OVERLAY_FIRE);
    set_mapping(KEY_TYPE_3, KEY_MOD_NONE, HOTKEY_SHOW_OVERLAY_DAMAGE);
    set_mapping(KEY_TYPE_4, KEY_MOD_NONE, HOTKEY_SHOW_OVERLAY_CRIME);
    set_mapping(KEY_TYPE_5, KEY_MOD_NONE, HOTKEY_SHOW_OVERLAY_PROBLEMS);
    // City map bookmarks
    set_mapping(KEY_TYPE_F1, KEY_MOD_NONE, HOTKEY_GO_TO_BOOKMARK_1);
    set_mapping(KEY_TYPE_F2, KEY_MOD_NONE, HOTKEY_GO_TO_BOOKMARK_2);
    set_mapping(KEY_TYPE_F3, KEY_MOD_NONE, HOTKEY_GO_TO_BOOKMARK_3);
    set_mapping(KEY_TYPE_F4, KEY_MOD_NONE, HOTKEY_GO_TO_BOOKMARK_4);
    set_mapping(KEY_TYPE_F1, KEY_MOD_CTRL, HOTKEY_SET_BOOKMARK_1);
    set_mapping(KEY_TYPE_F2, KEY_MOD_CTRL, HOTKEY_SET_BOOKMARK_2);
    set_mapping(KEY_TYPE_F3, KEY_MOD_CTRL, HOTKEY_SET_BOOKMARK_3);
    set_mapping(KEY_TYPE_F4, KEY_MOD_CTRL, HOTKEY_SET_BOOKMARK_4);
    // Editor
    set_mapping(KEY_TYPE_A, KEY_MOD_CTRL, HOTKEY_EDITOR_TOGGLE_BATTLE_INFO);
    // Cheats
    set_mapping(KEY_TYPE_COMMA, KEY_MOD_CTRL, HOTKEY_CHEAT_MONEY);
    set_mapping(KEY_TYPE_PERIOD, KEY_MOD_CTRL, HOTKEY_CHEAT_INVASION);
    set_mapping(KEY_TYPE_SLASH, KEY_MOD_CTRL, HOTKEY_CHEAT_VICTORY);
}

const hotkey_mapping *hotkey_for_action(hotkey_action action, int index)
{
    int num = 0;
    for (int i = 0; i < data.num_mappings; i++) {
        if (data.mappings[i].action == action) {
            if (num == index) {
                return &data.mappings[i];
            }
            num++;
        }
    }
    return 0;
}

const hotkey_mapping *hotkey_default_for_action(hotkey_action action, int index)
{
    if (index < 0 || index >= 2 || (int) action < 0 || action >= HOTKEY_MAX_ITEMS) {
        return 0;
    }
    return &data.default_mappings[action][index];
}

void hotkey_config_clear(void)
{
    data.num_mappings = 0;
}

void hotkey_config_add_mapping(const hotkey_mapping *mapping)
{
    if (data.num_mappings < MAX_MAPPINGS) {
        data.mappings[data.num_mappings] = *mapping;
        data.num_mappings++;
    }
}

static void load_defaults(void)
{
    hotkey_config_clear();
    for (int action = 0; action < HOTKEY_MAX_ITEMS; action++) {
        for (int index = 0; index < 2; index++) {
            if (data.default_mappings[action][index].key) {
                hotkey_config_add_mapping(&data.default_mappings[action][index]);
            }
        }
    }
}

static void load_file(void)
{
    hotkey_config_clear();
    FILE *fp = file_open(HOTKEY_CONFIGS_FILE_PATH, "rt");
    if (!fp) {
        return;
    }
    char line_buffer[MAX_LINE];
    char *line;
    while ((line = fgets(line_buffer, MAX_LINE, fp))) {
        // Remove newline from string
        size_t size = strlen(line);
        while (size > 0 && (line[size - 1] == '\n' || line[size - 1] == '\r')) {
            line[--size] = 0;
        }
        char *equals = strchr(line, '=');
        if (!equals) {
            continue;
        }
        *equals = 0;
        char *value = &equals[1];
        for (int i = 0; i < HOTKEY_MAX_ITEMS; i++) {
            if (strcmp(ini_keys[i], line) == 0) {
                hotkey_mapping mapping;
                if (key_combination_from_name(value, &mapping.key, &mapping.modifiers)) {
                    mapping.action = i;
                    hotkey_config_add_mapping(&mapping);
                }
                break;
            }
        }
    }
    file_close(fp);
}

void hotkey_config_load(void)
{
    init_defaults();
    load_file();
    if (data.num_mappings == 0) {
        load_defaults();
    }
    hotkey_install_mapping(data.mappings, data.num_mappings);
}

void hotkey_config_save(void)
{
    hotkey_install_mapping(data.mappings, data.num_mappings);
    FILE *fp = file_open(HOTKEY_CONFIGS_FILE_PATH, "wt");
    if (!fp) {
        log_error("Unable to write hotkey configuration file", HOTKEY_CONFIGS_FILE_PATH, 0);
        return;
    }
    for (int i = 0; i < data.num_mappings; i++) {
        const char *key_name = key_combination_name(data.mappings[i].key, data.mappings[i].modifiers);
        fprintf(fp, "%s=%s\n", ini_keys[data.mappings[i].action], key_name);
    }
    file_close(fp);
}
