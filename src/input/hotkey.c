#include "hotkey.h"

#include "building/type.h"
#include "city/constants.h"
#include "game/settings.h"
#include "game/state.h"
#include "game/system.h"
#include "graphics/screenshot.h"
#include "graphics/video.h"
#include "graphics/window.h"
#include "input/scroll.h"
#include "window/hotkey_editor.h"

#include <stdlib.h>
#include <string.h>

typedef struct {
    int *action;
    int value;
    key_type key;
    key_modifier_type modifiers;
    int repeatable;
} hotkey_definition;

typedef struct {
    void (*action)(int is_down);
    key_type key;
} arrow_definition;

typedef struct {
    int toggle_fullscreen;
    int reset_window;
    int save_screenshot;
    int save_city_screenshot;
} global_hotkeys;

static struct {
    global_hotkeys global_hotkey_state;
    hotkeys hotkey_state;
    hotkey_definition *definitions;
    int num_definitions;
    arrow_definition *arrows;
    int num_arrows;
} data;

static void set_definition_for_action(hotkey_action action, hotkey_definition *def)
{
    def->value = 1;
    def->repeatable = 0;
    switch (action) {
        case HOTKEY_TOGGLE_FULLSCREEN:
            def->action = &data.global_hotkey_state.toggle_fullscreen;
            break;
        case HOTKEY_RESET_WINDOW:
            def->action = &data.global_hotkey_state.reset_window;
            break;
        case HOTKEY_SAVE_SCREENSHOT:
            def->action = &data.global_hotkey_state.save_screenshot;
            break;
        case HOTKEY_SAVE_CITY_SCREENSHOT:
            def->action = &data.global_hotkey_state.save_city_screenshot;
            break;
        case HOTKEY_LOAD_FILE:
            def->action = &data.hotkey_state.load_file;
            break;
        case HOTKEY_SAVE_FILE:
            def->action = &data.hotkey_state.save_file;
            break;
        case HOTKEY_DECREASE_GAME_SPEED:
            def->action = &data.hotkey_state.decrease_game_speed;
            def->repeatable = 1;
            break;
        case HOTKEY_INCREASE_GAME_SPEED:
            def->action = &data.hotkey_state.increase_game_speed;
            def->repeatable = 1;
            break;
        case HOTKEY_TOGGLE_PAUSE:
            def->action = &data.hotkey_state.toggle_pause;
            break;
        case HOTKEY_ROTATE_MAP_LEFT:
            def->action = &data.hotkey_state.rotate_map_left;
            break;
        case HOTKEY_ROTATE_MAP_RIGHT:
            def->action = &data.hotkey_state.rotate_map_right;
            break;
        case HOTKEY_REPLAY_MAP:
            def->action = &data.hotkey_state.replay_map;
            break;
        case HOTKEY_CYCLE_LEGION:
            def->action = &data.hotkey_state.cycle_legion;
            break;
        case HOTKEY_RETURN_LEGIONS_TO_FORT:
            def->action = &data.hotkey_state.return_legions_to_fort;
            break;
        case HOTKEY_SHOW_LAST_ADVISOR:
            def->action = &data.hotkey_state.show_last_advisor;
            break;
        case HOTKEY_SHOW_EMPIRE_MAP:
            def->action = &data.hotkey_state.show_empire_map;
            break;
        case HOTKEY_SHOW_MESSAGES:
            def->action = &data.hotkey_state.show_messages;
            break;
        case HOTKEY_BUILD_CLONE:
            def->action = &data.hotkey_state.clone_building;
            break;
        case HOTKEY_BUILD_VACANT_HOUSE:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_HOUSE_VACANT_LOT;
            break;
        case HOTKEY_BUILD_CLEAR_LAND:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_CLEAR_LAND;
            break;
        case HOTKEY_BUILD_ROAD:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_ROAD;
            break;
        case HOTKEY_BUILD_FOUNTAIN:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_FOUNTAIN;
            break;
        case HOTKEY_BUILD_BARBER:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_BARBER;
            break;
        case HOTKEY_BUILD_BATHHOUSE:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_BATHHOUSE;
            break;
        case HOTKEY_BUILD_DOCTOR:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_DOCTOR;
            break;
        case HOTKEY_BUILD_SMALL_TEMPLES:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_MENU_SMALL_TEMPLES;
            break;
        case HOTKEY_BUILD_SCHOOL:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_SCHOOL;
            break;
        case HOTKEY_BUILD_LIBRARY:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_LIBRARY;
            break;
        case HOTKEY_BUILD_THEATER:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_THEATER;
            break;
        case HOTKEY_BUILD_AMPHITHEATER:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_AMPHITHEATER;
            break;
        case HOTKEY_BUILD_GLADIATOR_SCHOOL:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_GLADIATOR_SCHOOL;
            break;
        case HOTKEY_BUILD_ACTOR_COLONY:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_ACTOR_COLONY;
            break;
        case HOTKEY_BUILD_FORUM:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_FORUM;
            break;
        case HOTKEY_BUILD_SMALL_STATUE:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_SMALL_STATUE;
            break;
        case HOTKEY_BUILD_MEDIUM_STATUE:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_MEDIUM_STATUE;
            break;
        case HOTKEY_BUILD_GARDENS:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_GARDENS;
            break;
        case HOTKEY_BUILD_PLAZA:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_PLAZA;
            break;
        case HOTKEY_BUILD_ENGINEERS_POST:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_ENGINEERS_POST;
            break;
        case HOTKEY_BUILD_PREFECTURE:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_PREFECTURE;
            break;
        case HOTKEY_BUILD_MARKET:
            def->action = &data.hotkey_state.building;
            def->value = BUILDING_MARKET;
            break;
        case HOTKEY_SHOW_OVERLAY_WATER:
            def->action = &data.hotkey_state.show_overlay;
            def->value = OVERLAY_WATER;
            break;
        case HOTKEY_SHOW_OVERLAY_FIRE:
            def->action = &data.hotkey_state.show_overlay;
            def->value = OVERLAY_FIRE;
            break;
        case HOTKEY_SHOW_OVERLAY_DAMAGE:
            def->action = &data.hotkey_state.show_overlay;
            def->value = OVERLAY_DAMAGE;
            break;
        case HOTKEY_SHOW_OVERLAY_CRIME:
            def->action = &data.hotkey_state.show_overlay;
            def->value = OVERLAY_CRIME;
            break;
        case HOTKEY_SHOW_OVERLAY_PROBLEMS:
            def->action = &data.hotkey_state.show_overlay;
            def->value = OVERLAY_PROBLEMS;
            break;
        case HOTKEY_GO_TO_BOOKMARK_1:
            def->action = &data.hotkey_state.go_to_bookmark;
            def->value = 1;
            break;
        case HOTKEY_GO_TO_BOOKMARK_2:
            def->action = &data.hotkey_state.go_to_bookmark;
            def->value = 2;
            break;
        case HOTKEY_GO_TO_BOOKMARK_3:
            def->action = &data.hotkey_state.go_to_bookmark;
            def->value = 3;
            break;
        case HOTKEY_GO_TO_BOOKMARK_4:
            def->action = &data.hotkey_state.go_to_bookmark;
            def->value = 4;
            break;
        case HOTKEY_SET_BOOKMARK_1:
            def->action = &data.hotkey_state.set_bookmark;
            def->value = 1;
            break;
        case HOTKEY_SET_BOOKMARK_2:
            def->action = &data.hotkey_state.set_bookmark;
            def->value = 2;
            break;
        case HOTKEY_SET_BOOKMARK_3:
            def->action = &data.hotkey_state.set_bookmark;
            def->value = 3;
            break;
        case HOTKEY_SET_BOOKMARK_4:
            def->action = &data.hotkey_state.set_bookmark;
            def->value = 4;
            break;
        case HOTKEY_EDITOR_TOGGLE_BATTLE_INFO:
            def->action = &data.hotkey_state.toggle_editor_battle_info;
            break;
        case HOTKEY_CHEAT_MONEY:
            def->action = &data.hotkey_state.cheat_money;
            break;
        case HOTKEY_CHEAT_INVASION:
            def->action = &data.hotkey_state.cheat_invasion;
            break;
        case HOTKEY_CHEAT_VICTORY:
            def->action = &data.hotkey_state.cheat_victory;
            break;
        default:
            def->action = 0;
    }
}

static void add_definition(const hotkey_mapping *mapping)
{
    hotkey_definition *def = &data.definitions[data.num_definitions];
    def->key = mapping->key;
    def->modifiers = mapping->modifiers;
    set_definition_for_action(mapping->action, def);
    if (def->action) {
        data.num_definitions++;
    }
}

static void add_arrow(const hotkey_mapping *mapping)
{
    arrow_definition *arrow = &data.arrows[data.num_arrows];
    arrow->key = mapping->key;
    switch (mapping->action) {
        case HOTKEY_ARROW_UP:
            arrow->action = scroll_arrow_up;
            break;
        case HOTKEY_ARROW_DOWN:
            arrow->action = scroll_arrow_down;
            break;
        case HOTKEY_ARROW_LEFT:
            arrow->action = scroll_arrow_left;
            break;
        case HOTKEY_ARROW_RIGHT:
            arrow->action = scroll_arrow_right;
            break;
        default:
            arrow->action = 0;
            break;
    }
    if (arrow->action) {
        data.num_arrows++;
    }
}

static int allocate_mapping_memory(int total_definitions, int total_arrows)
{
    free(data.definitions);
    free(data.arrows);
    data.num_definitions = 0;
    data.num_arrows = 0;
    data.definitions = malloc(sizeof(hotkey_definition) * total_definitions);
    data.arrows = malloc(sizeof(arrow_definition) * total_arrows);
    if (!data.definitions || !data.arrows) {
        free(data.definitions);
        free(data.arrows);
        return 0;
    }
    return 1;
}

void hotkey_install_mapping(hotkey_mapping *mappings, int num_mappings)
{
    int total_definitions = 2; // Enter and ESC are fixed hotkeys
    int total_arrows = 0;
    for (int i = 0; i < num_mappings; i++) {
        hotkey_action action = mappings[i].action;
        if (action == HOTKEY_ARROW_UP || action == HOTKEY_ARROW_DOWN ||
            action == HOTKEY_ARROW_LEFT || action == HOTKEY_ARROW_RIGHT) {
            total_arrows++;
        } else {
            total_definitions++;
        }
    }
    if (!allocate_mapping_memory(total_definitions, total_arrows)) {
        return;
    }

    // Fixed keys: Escape and Enter
    data.definitions[0].action = &data.hotkey_state.enter_pressed;
    data.definitions[0].key = KEY_TYPE_ENTER;
    data.definitions[0].modifiers = 0;
    data.definitions[0].repeatable = 0;
    data.definitions[0].value = 1;

    data.definitions[1].action = &data.hotkey_state.escape_pressed;
    data.definitions[1].key = KEY_TYPE_ESCAPE;
    data.definitions[1].modifiers = 0;
    data.definitions[1].repeatable = 0;
    data.definitions[1].value = 1;

    data.num_definitions = 2;

    for (int i = 0; i < num_mappings; i++) {
        hotkey_action action = mappings[i].action;
        if (action == HOTKEY_ARROW_UP || action == HOTKEY_ARROW_DOWN ||
            action == HOTKEY_ARROW_LEFT || action == HOTKEY_ARROW_RIGHT) {
            add_arrow(&mappings[i]);
        } else {
            add_definition(&mappings[i]);
        }
    }
}

const hotkeys *hotkey_state(void)
{
    return &data.hotkey_state;
}

void hotkey_reset_state(void)
{
    memset(&data.hotkey_state, 0, sizeof(data.hotkey_state));
    memset(&data.global_hotkey_state, 0, sizeof(data.global_hotkey_state));
}

void hotkey_key_pressed(key_type key, key_modifier_type modifiers, int repeat)
{
    if (window_is(WINDOW_HOTKEY_EDITOR)) {
        window_hotkey_editor_key_pressed(key, modifiers);
        return;
    }
    if (key == KEY_TYPE_NONE) {
        return;
    }
    int found_action = 0;
    for (int i = 0; i < data.num_definitions; i++) {
        hotkey_definition *def = &data.definitions[i];
        if (def->key == key && def->modifiers == modifiers && (!repeat || def->repeatable)) {
            *(def->action) = def->value;
            found_action = 1;
        }
    }
    if (found_action) {
        return;
    }
    for (int i = 0; i < data.num_arrows; i++) {
        arrow_definition *arrow = &data.arrows[i];
        if (arrow->key == key) {
            arrow->action(1);
        }
    }
}

void hotkey_key_released(key_type key, key_modifier_type modifiers)
{
    if (window_is(WINDOW_HOTKEY_EDITOR)) {
        window_hotkey_editor_key_released(key, modifiers);
        return;
    }
    if (key == KEY_TYPE_NONE) {
        return;
    }
    for (int i = 0; i < data.num_arrows; i++) {
        arrow_definition *arrow = &data.arrows[i];
        if (arrow->key == key) {
            arrow->action(0);
        }
    }
}

void hotkey_handle_global_keys(void)
{
    if (data.global_hotkey_state.reset_window) {
        system_resize(1280, 800);
        system_center();
    }
    if (data.global_hotkey_state.toggle_fullscreen) {
        system_set_fullscreen(!setting_fullscreen());
    }
    if (data.global_hotkey_state.save_screenshot) {
        graphics_save_screenshot(0);
    }
    if (data.global_hotkey_state.save_city_screenshot) {
        graphics_save_screenshot(1);
    }
}

void hotkey_set_value_for_action(hotkey_action action, int value)
{
    hotkey_definition def;
    set_definition_for_action(action, &def);
    *(def.action) = value ? def.value : 0;
}
