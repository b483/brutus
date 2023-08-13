#include "hotkey_config.h"

#include "core/file.h"
#include "core/string.h"
#include "platform/brutus.h"
#include "input/hotkey.h"

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
    "go_to_problem",
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
    "clone_building",
    "cycle_buildings",
    "cycle_buildings_reverse",
    "undo",
    "build_vacant_house",
    "build_clear_land",
    "build_road",
    "build_reservoir",
    "build_aqueduct",
    "build_fountain",
    "build_well",
    "build_doctor",
    "build_bathhouse",
    "build_barber",
    "build_hospital",
    "build_small_temple_ceres",
    "build_small_temple_neptune",
    "build_small_temple_mercury",
    "build_small_temple_mars",
    "build_small_temple_venus",
    "build_large_temple_ceres",
    "build_large_temple_neptune",
    "build_large_temple_mercury",
    "build_large_temple_mars",
    "build_large_temple_venus",
    "build_oracle",
    "build_school",
    "build_library",
    "build_academy",
    "build_mission_post",
    "build_theater",
    "build_actor_colony",
    "build_amphitheater",
    "build_gladiator_school",
    "build_lion_house",
    "build_colosseum",
    "build_chariot_maker",
    "build_hippodrome",
    "build_gardens",
    "build_plaza",
    "build_small_statue",
    "build_medium_statue",
    "build_large_statue",
    "build_governor's house",
    "build_governor's villa",
    "build_governor's palace",
    "build_forum",
    "build_senate",
    "build_triumphal_arch",
    "build_engineers_post",
    "build_low_bridge",
    "build_ship_bridge",
    "build_shipyard",
    "build_wharf",
    "build_dock",
    "build_prefecture",
    "build_wall",
    "build_tower",
    "build_gatehouse",
    "build_fort_legionaries",
    "build_fort_javelin",
    "build_fort_mounted",
    "build_barracks",
    "build_military_academy",
    "build_wheat_farm",
    "build_vegetable_farm",
    "build_fruit_farm",
    "build_pig_farm",
    "build_olive_farm",
    "build_vines_farm",
    "build_clay_pit",
    "build_timber_yard",
    "build_marble_quarry",
    "build_iron_mine",
    "build_oil_workshop",
    "build_wine_workshop",
    "build_pottery_workshop",
    "build_furniture_workshop",
    "build_weapons_workshop",
    "build_market",
    "build_granary",
    "build_warehouse",
};

static struct {
    struct hotkey_mapping_t default_mappings[HOTKEY_MAX_ITEMS][2];
    struct hotkey_mapping_t mappings[MAX_MAPPINGS];
    int num_mappings;
} data;

static void set_mapping(int key, int modifiers, int action)
{
    struct hotkey_mapping_t *mapping = &data.default_mappings[action][0];
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
    set_mapping(KEY_TYPE_1, KEY_MOD_ALT, HOTKEY_DECREASE_GAME_SPEED);
    set_mapping(KEY_TYPE_2, KEY_MOD_ALT, HOTKEY_INCREASE_GAME_SPEED);
    set_mapping(KEY_TYPE_KP_MINUS, KEY_MOD_NONE, HOTKEY_DECREASE_GAME_SPEED);
    set_mapping(KEY_TYPE_KP_PLUS, KEY_MOD_NONE, HOTKEY_INCREASE_GAME_SPEED);
    set_mapping(KEY_TYPE_P, KEY_MOD_NONE, HOTKEY_TOGGLE_PAUSE);
    set_mapping(KEY_TYPE_CAPSLOCK, KEY_MOD_NONE, HOTKEY_TOGGLE_PAUSE);
    set_mapping(KEY_TYPE_HOME, KEY_MOD_NONE, HOTKEY_ROTATE_MAP_LEFT);
    set_mapping(KEY_TYPE_END, KEY_MOD_NONE, HOTKEY_ROTATE_MAP_RIGHT);
    set_mapping(KEY_TYPE_R, KEY_MOD_CTRL, HOTKEY_REPLAY_MAP);
    set_mapping(KEY_TYPE_SPACE, KEY_MOD_NONE, HOTKEY_CYCLE_LEGION);
    set_mapping(KEY_TYPE_B, KEY_MOD_NONE, HOTKEY_RETURN_LEGIONS_TO_FORT);
    set_mapping(KEY_TYPE_1, KEY_MOD_NONE, HOTKEY_SHOW_LAST_ADVISOR);
    set_mapping(KEY_TYPE_2, KEY_MOD_NONE, HOTKEY_SHOW_EMPIRE_MAP);
    set_mapping(KEY_TYPE_GRAVE, KEY_MOD_NONE, HOTKEY_SHOW_MESSAGES);
    set_mapping(KEY_TYPE_GRAVE, KEY_MOD_ALT, HOTKEY_GO_TO_PROBLEM);
    // Overlays
    set_mapping(KEY_TYPE_W, KEY_MOD_SHIFT, HOTKEY_SHOW_OVERLAY_WATER);
    set_mapping(KEY_TYPE_F, KEY_MOD_SHIFT, HOTKEY_SHOW_OVERLAY_FIRE);
    set_mapping(KEY_TYPE_D, KEY_MOD_SHIFT, HOTKEY_SHOW_OVERLAY_DAMAGE);
    set_mapping(KEY_TYPE_C, KEY_MOD_SHIFT, HOTKEY_SHOW_OVERLAY_CRIME);
    set_mapping(KEY_TYPE_R, KEY_MOD_SHIFT, HOTKEY_SHOW_OVERLAY_PROBLEMS);
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
    // Construction hotkeys
    set_mapping(KEY_TYPE_Q, KEY_MOD_ALT, HOTKEY_BUILD_CLONE);
    set_mapping(KEY_TYPE_TAB, KEY_MOD_NONE, HOTKEY_CYCLE_BUILDINGS);
    set_mapping(KEY_TYPE_TAB, KEY_MOD_SHIFT, HOTKEY_CYCLE_BUILDINGS_REVERSE);
    set_mapping(KEY_TYPE_Z, KEY_MOD_CTRL, HOTKEY_UNDO);
    set_mapping(KEY_TYPE_Q, KEY_MOD_NONE, HOTKEY_BUILD_VACANT_HOUSE);
    set_mapping(KEY_TYPE_W, KEY_MOD_NONE, HOTKEY_BUILD_CLEAR_LAND);
    set_mapping(KEY_TYPE_E, KEY_MOD_NONE, HOTKEY_BUILD_ROAD);
    set_mapping(KEY_TYPE_R, KEY_MOD_NONE, HOTKEY_BUILD_RESERVOIR);
    set_mapping(KEY_TYPE_R, KEY_MOD_ALT, HOTKEY_BUILD_FOUNTAIN);
    set_mapping(KEY_TYPE_S, KEY_MOD_ALT, HOTKEY_BUILD_DOCTOR);
    set_mapping(KEY_TYPE_F, KEY_MOD_NONE, HOTKEY_BUILD_BATHHOUSE);
    set_mapping(KEY_TYPE_G, KEY_MOD_ALT, HOTKEY_BUILD_BARBER);
    set_mapping(KEY_TYPE_S, KEY_MOD_NONE, HOTKEY_BUILD_SMALL_TEMPLE_CERES);
    set_mapping(KEY_TYPE_D, KEY_MOD_ALT, HOTKEY_BUILD_SCHOOL);
    set_mapping(KEY_TYPE_G, KEY_MOD_NONE, HOTKEY_BUILD_LIBRARY);
    set_mapping(KEY_TYPE_D, KEY_MOD_NONE, HOTKEY_BUILD_THEATER);
    set_mapping(KEY_TYPE_F, KEY_MOD_ALT, HOTKEY_BUILD_AMPHITHEATER);
    set_mapping(KEY_TYPE_Z, KEY_MOD_ALT, HOTKEY_BUILD_LION_HOUSE);
    set_mapping(KEY_TYPE_Z, KEY_MOD_NONE, HOTKEY_BUILD_COLOSSEUM);
    set_mapping(KEY_TYPE_W, KEY_MOD_ALT, HOTKEY_BUILD_GARDENS);
    set_mapping(KEY_TYPE_E, KEY_MOD_ALT, HOTKEY_BUILD_PLAZA);
    set_mapping(KEY_TYPE_T, KEY_MOD_NONE, HOTKEY_BUILD_FORUM);
    set_mapping(KEY_TYPE_A, KEY_MOD_NONE, HOTKEY_BUILD_ENGINEERS_POST);
    set_mapping(KEY_TYPE_A, KEY_MOD_ALT, HOTKEY_BUILD_PREFECTURE);
    set_mapping(KEY_TYPE_X, KEY_MOD_ALT, HOTKEY_BUILD_WHEAT_FARM);
    set_mapping(KEY_TYPE_C, KEY_MOD_NONE, HOTKEY_BUILD_CLAY_PIT);
    set_mapping(KEY_TYPE_C, KEY_MOD_ALT, HOTKEY_BUILD_WINE_WORKSHOP);
    set_mapping(KEY_TYPE_X, KEY_MOD_NONE, HOTKEY_BUILD_MARKET);
    set_mapping(KEY_TYPE_V, KEY_MOD_NONE, HOTKEY_BUILD_GRANARY);
    set_mapping(KEY_TYPE_V, KEY_MOD_ALT, HOTKEY_BUILD_WAREHOUSE);
}

const struct hotkey_mapping_t *hotkey_for_action(int action, int index)
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

const struct hotkey_mapping_t *hotkey_default_for_action(int action, int index)
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

void hotkey_config_add_mapping(const struct hotkey_mapping_t *mapping)
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
    FILE *fp = fopen(HOTKEY_CONFIGS_FILE_PATH, "rt");
    if (!fp) {
        return;
    }
    char line_buffer[MAX_LINE];
    char *line;
    while ((line = fgets(line_buffer, MAX_LINE, fp))) {
        // Remove newline from string
        size_t size = string_length(line);
        while (size > 0 && (line[size - 1] == '\n' || line[size - 1] == '\r')) {
            line[--size] = 0;
        }
        char *equals = get_first_char_occurrence_in_string(line, '=');
        if (!equals) {
            continue;
        }
        *equals = 0;
        char *value = &equals[1];
        for (int i = 0; i < HOTKEY_MAX_ITEMS; i++) {
            if (string_equals(ini_keys[i], line)) {
                struct hotkey_mapping_t mapping;
                if (key_combination_from_name(value, &mapping.key, &mapping.modifiers)) {
                    mapping.action = i;
                    hotkey_config_add_mapping(&mapping);
                }
                break;
            }
        }
    }
    fclose(fp);
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
    FILE *fp = fopen(HOTKEY_CONFIGS_FILE_PATH, "wt");
    if (!fp) {
        log_error("Unable to write hotkey configuration file", HOTKEY_CONFIGS_FILE_PATH, 0);
        return;
    }
    for (int i = 0; i < data.num_mappings; i++) {
        const char *key_name = key_combination_name(data.mappings[i].key, data.mappings[i].modifiers);
        fprintf(fp, "%s=%s\n", ini_keys[data.mappings[i].action], key_name);
    }
    fclose(fp);
}
