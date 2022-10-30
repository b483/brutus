#include "config.h"

#include "building/type.h"
#include "core/hotkey_config.h"
#include "core/image_group.h"
#include "core/lang.h"
#include "core/string.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/scrollbar.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "window/config.h"
#include "window/hotkey_editor.h"
#include "window/main_menu.h"
#include "window/plain_message_dialog.h"

#define HOTKEY_HEADER -1
#define TR_NONE -1
#define GROUP_BUILDINGS 28

#define NUM_VISIBLE_OPTIONS 14
#define NUM_BOTTOM_BUTTONS 3

static void on_scroll(void);

static void button_hotkey(int row, int is_alternative);
static void button_reset_defaults(int param1, int param2);
static void button_close(int save, int param2);

static scrollbar_type scrollbar = { 580, 72, 352, on_scroll, 0, 0, 0, 0, 0, 0 };

typedef struct {
    int action;
    int name_translation;
    int name_text_group;
    int name_text_id;
} hotkey_widget;

static hotkey_widget hotkey_widgets[] = {
    {HOTKEY_HEADER, TR_HOTKEY_HEADER_ARROWS, 0, 0},
    {HOTKEY_ARROW_UP, TR_HOTKEY_ARROW_UP, 0, 0},
    {HOTKEY_ARROW_DOWN, TR_HOTKEY_ARROW_DOWN, 0, 0},
    {HOTKEY_ARROW_LEFT, TR_HOTKEY_ARROW_LEFT, 0, 0},
    {HOTKEY_ARROW_RIGHT, TR_HOTKEY_ARROW_RIGHT, 0, 0},
    {HOTKEY_HEADER, TR_HOTKEY_HEADER_GLOBAL, 0, 0},
    {HOTKEY_TOGGLE_FULLSCREEN, TR_HOTKEY_TOGGLE_FULLSCREEN, 0, 0},
    {HOTKEY_RESET_WINDOW, TR_HOTKEY_RESET_WINDOW, 0, 0},
    {HOTKEY_SAVE_SCREENSHOT, TR_HOTKEY_SAVE_SCREENSHOT, 0, 0},
    {HOTKEY_SAVE_CITY_SCREENSHOT, TR_HOTKEY_SAVE_CITY_SCREENSHOT, 0, 0},
    {HOTKEY_LOAD_FILE, TR_HOTKEY_LOAD_FILE, 0, 0},
    {HOTKEY_SAVE_FILE, TR_HOTKEY_SAVE_FILE, 0, 0},
    {HOTKEY_HEADER, TR_HOTKEY_HEADER_CITY, 0, 0},
    {HOTKEY_DECREASE_GAME_SPEED, TR_HOTKEY_DECREASE_GAME_SPEED, 0, 0},
    {HOTKEY_INCREASE_GAME_SPEED, TR_HOTKEY_INCREASE_GAME_SPEED, 0, 0},
    {HOTKEY_TOGGLE_PAUSE, TR_HOTKEY_TOGGLE_PAUSE, 0, 0},
    {HOTKEY_ROTATE_MAP_LEFT, TR_HOTKEY_ROTATE_MAP_LEFT, 0, 0},
    {HOTKEY_ROTATE_MAP_RIGHT, TR_HOTKEY_ROTATE_MAP_RIGHT, 0, 0},
    {HOTKEY_REPLAY_MAP, TR_HOTKEY_REPLAY_MAP, 0, 0},
    {HOTKEY_CYCLE_LEGION, TR_HOTKEY_CYCLE_LEGION, 0, 0},
    {HOTKEY_RETURN_LEGIONS_TO_FORT, TR_HOTKEY_RETURN_LEGIONS_TO_FORT, 0, 0},
    {HOTKEY_SHOW_LAST_ADVISOR, TR_HOTKEY_SHOW_LAST_ADVISOR, 0, 0},
    {HOTKEY_SHOW_EMPIRE_MAP, TR_HOTKEY_SHOW_EMPIRE_MAP, 0, 0},
    {HOTKEY_SHOW_MESSAGES, TR_HOTKEY_SHOW_MESSAGES, 0, 0},
    {HOTKEY_HEADER, TR_HOTKEY_HEADER_BUILD, 0, 0},
    {HOTKEY_BUILD_CLONE, TR_HOTKEY_BUILD_CLONE, 0, 0},
    {HOTKEY_BUILD_VACANT_HOUSE, TR_NONE, 67, 7},
    {HOTKEY_BUILD_CLEAR_LAND, TR_NONE, 68, 21},
    {HOTKEY_BUILD_ROAD, TR_NONE, GROUP_BUILDINGS, BUILDING_ROAD},
    {HOTKEY_BUILD_FOUNTAIN, TR_NONE, GROUP_BUILDINGS, BUILDING_FOUNTAIN},
    {HOTKEY_BUILD_BARBER, TR_NONE, GROUP_BUILDINGS, BUILDING_BARBER},
    {HOTKEY_BUILD_BATHHOUSE, TR_NONE, GROUP_BUILDINGS, BUILDING_BATHHOUSE},
    {HOTKEY_BUILD_DOCTOR, TR_NONE, GROUP_BUILDINGS, BUILDING_DOCTOR},
    {HOTKEY_BUILD_SMALL_TEMPLES, TR_NONE, GROUP_BUILDINGS, BUILDING_MENU_SMALL_TEMPLES},
    {HOTKEY_BUILD_SCHOOL, TR_NONE, GROUP_BUILDINGS, BUILDING_SCHOOL},
    {HOTKEY_BUILD_LIBRARY, TR_NONE, GROUP_BUILDINGS, BUILDING_LIBRARY},
    {HOTKEY_BUILD_THEATER, TR_NONE, GROUP_BUILDINGS, BUILDING_THEATER},
    {HOTKEY_BUILD_AMPHITHEATER, TR_NONE, GROUP_BUILDINGS, BUILDING_AMPHITHEATER},
    {HOTKEY_BUILD_GLADIATOR_SCHOOL, TR_NONE, GROUP_BUILDINGS, BUILDING_GLADIATOR_SCHOOL},
    {HOTKEY_BUILD_ACTOR_COLONY, TR_NONE, GROUP_BUILDINGS, BUILDING_ACTOR_COLONY},
    {HOTKEY_BUILD_FORUM, TR_NONE, GROUP_BUILDINGS, BUILDING_FORUM},
    {HOTKEY_BUILD_SMALL_STATUE, TR_NONE, GROUP_BUILDINGS, BUILDING_SMALL_STATUE},
    {HOTKEY_BUILD_MEDIUM_STATUE, TR_NONE, GROUP_BUILDINGS, BUILDING_MEDIUM_STATUE},
    {HOTKEY_BUILD_GARDENS, TR_NONE, GROUP_BUILDINGS, BUILDING_GARDENS},
    {HOTKEY_BUILD_PLAZA, TR_NONE, GROUP_BUILDINGS, BUILDING_PLAZA},
    {HOTKEY_BUILD_ENGINEERS_POST, TR_NONE, GROUP_BUILDINGS, BUILDING_ENGINEERS_POST},
    {HOTKEY_BUILD_PREFECTURE, TR_NONE, GROUP_BUILDINGS, BUILDING_PREFECTURE},
    {HOTKEY_BUILD_MARKET, TR_NONE, GROUP_BUILDINGS, BUILDING_MARKET},
    {HOTKEY_HEADER, TR_HOTKEY_HEADER_OVERLAYS, 0, 0},
    {HOTKEY_SHOW_OVERLAY_WATER, TR_HOTKEY_SHOW_OVERLAY_WATER, 0, 0},
    {HOTKEY_SHOW_OVERLAY_FIRE, TR_HOTKEY_SHOW_OVERLAY_FIRE, 0, 0},
    {HOTKEY_SHOW_OVERLAY_DAMAGE, TR_HOTKEY_SHOW_OVERLAY_DAMAGE, 0, 0},
    {HOTKEY_SHOW_OVERLAY_CRIME, TR_HOTKEY_SHOW_OVERLAY_CRIME, 0, 0},
    {HOTKEY_SHOW_OVERLAY_PROBLEMS, TR_HOTKEY_SHOW_OVERLAY_PROBLEMS, 0, 0},
    {HOTKEY_HEADER, TR_HOTKEY_HEADER_BOOKMARKS, 0, 0},
    {HOTKEY_GO_TO_BOOKMARK_1, TR_HOTKEY_GO_TO_BOOKMARK_1, 0, 0},
    {HOTKEY_GO_TO_BOOKMARK_2, TR_HOTKEY_GO_TO_BOOKMARK_2, 0, 0},
    {HOTKEY_GO_TO_BOOKMARK_3, TR_HOTKEY_GO_TO_BOOKMARK_3, 0, 0},
    {HOTKEY_GO_TO_BOOKMARK_4, TR_HOTKEY_GO_TO_BOOKMARK_4, 0, 0},
    {HOTKEY_SET_BOOKMARK_1, TR_HOTKEY_SET_BOOKMARK_1, 0, 0},
    {HOTKEY_SET_BOOKMARK_2, TR_HOTKEY_SET_BOOKMARK_2, 0, 0},
    {HOTKEY_SET_BOOKMARK_3, TR_HOTKEY_SET_BOOKMARK_3, 0, 0},
    {HOTKEY_SET_BOOKMARK_4, TR_HOTKEY_SET_BOOKMARK_4, 0, 0},
    {HOTKEY_HEADER, TR_HOTKEY_HEADER_EDITOR, 0, 0},
    {HOTKEY_EDITOR_TOGGLE_BATTLE_INFO, TR_HOTKEY_EDITOR_TOGGLE_BATTLE_INFO, 0, 0},
    {HOTKEY_HEADER, TR_HOTKEY_HEADER_CHEATS, 0, 0},
    {HOTKEY_CHEAT_MONEY, TR_HOTKEY_CHEAT_MONEY, 0, 0},
    {HOTKEY_CHEAT_INVASION, TR_HOTKEY_CHEAT_INVASION, 0, 0},
    {HOTKEY_CHEAT_VICTORY, TR_HOTKEY_CHEAT_VICTORY, 0, 0},
};

#define HOTKEY_X_OFFSET_1 270
#define HOTKEY_X_OFFSET_2 420
#define HOTKEY_BTN_WIDTH 150
#define HOTKEY_BTN_HEIGHT 22

static generic_button hotkey_buttons[] = {
    {HOTKEY_X_OFFSET_1, 80 + 24 * 0, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 0, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 0, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 0, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 1, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 1, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 1, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 1, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 2, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 2, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 2, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 2, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 3, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 3, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 3, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 3, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 4, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 4, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 4, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 4, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 5, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 5, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 5, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 5, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 6, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 6, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 6, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 6, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 7, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 7, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 7, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 7, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 8, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 8, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 8, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 8, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 9, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 9, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 9, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 9, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 10, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 10, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 10, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 10, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 11, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 11, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 11, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 11, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 12, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 12, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 12, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 12, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 13, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 13, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 13, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 13, 1},
};

static generic_button bottom_buttons[] = {
    {230, 430, 180, 30, button_reset_defaults, button_none, 0, 0},
    {415, 430, 100, 30, button_close, button_none, 0, 0},
    {520, 430, 100, 30, button_close, button_none, 1, 0},
};

static custom_string_key bottom_button_texts[] = {
    TR_BUTTON_RESET_DEFAULTS,
    TR_BUTTON_CANCEL,
    TR_BUTTON_OK
};

static struct {
    int focus_button;
    int bottom_focus_button;
    hotkey_mapping mappings[HOTKEY_MAX_ITEMS][2];
} data;

static void init(void)
{
    scrollbar_init(&scrollbar, 0, sizeof(hotkey_widgets) / sizeof(hotkey_widget) - NUM_VISIBLE_OPTIONS);

    for (int i = 0; i < HOTKEY_MAX_ITEMS; i++) {
        hotkey_mapping empty = { KEY_TYPE_NONE, KEY_MOD_NONE, i };

        const hotkey_mapping *mapping = hotkey_for_action(i, 0);
        data.mappings[i][0] = mapping ? *mapping : empty;

        mapping = hotkey_for_action(i, 1);
        data.mappings[i][1] = mapping ? *mapping : empty;
    }
}

static void draw_background(void)
{
    graphics_clear_screen();

    image_draw_fullscreen_background(image_group(GROUP_INTERMEZZO_BACKGROUND) + 16);
    draw_version_string();

    graphics_in_dialog();
    outer_panel_draw(0, 0, 40, 30);

    text_draw_centered(get_custom_string(TR_HOTKEY_TITLE), 16, 16, 608, FONT_LARGE_BLACK, 0);

    text_draw_centered(get_custom_string(TR_HOTKEY_LABEL), HOTKEY_X_OFFSET_1, 55,
        HOTKEY_BTN_WIDTH, FONT_NORMAL_BLACK, 0);
    text_draw_centered(get_custom_string(TR_HOTKEY_ALTERNATIVE_LABEL), HOTKEY_X_OFFSET_2, 55,
        HOTKEY_BTN_WIDTH, FONT_NORMAL_BLACK, 0);

    inner_panel_draw(20, 72, 35, 22);
    int y_base = 80;
    for (int i = 0; i < NUM_VISIBLE_OPTIONS; i++) {
        hotkey_widget *widget = &hotkey_widgets[i + scrollbar.scroll_position];
        int text_offset = y_base + 6 + 24 * i;
        if (widget->action == HOTKEY_HEADER) {
            text_draw(get_custom_string(widget->name_translation), 32, text_offset, FONT_NORMAL_WHITE, 0);
        } else {
            if (widget->name_translation != TR_NONE) {
                text_draw(get_custom_string(widget->name_translation),
                    32, text_offset, FONT_NORMAL_GREEN, 0);
            } else {
                lang_text_draw(widget->name_text_group, widget->name_text_id,
                    32, text_offset, FONT_NORMAL_GREEN);
            }

            const hotkey_mapping *mapping1 = &data.mappings[widget->action][0];
            if (mapping1->key) {
                const uint8_t *keyname = key_combination_display_name(mapping1->key, mapping1->modifiers);
                graphics_set_clip_rectangle(HOTKEY_X_OFFSET_1, text_offset, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT);
                text_draw_centered(keyname, HOTKEY_X_OFFSET_1 + 3, text_offset,
                    HOTKEY_BTN_WIDTH - 6, FONT_NORMAL_WHITE, 0);
                graphics_reset_clip_rectangle();
            }

            const hotkey_mapping *mapping2 = &data.mappings[widget->action][1];
            if (mapping2->key) {
                graphics_set_clip_rectangle(HOTKEY_X_OFFSET_2, text_offset, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT);
                const uint8_t *keyname = key_combination_display_name(mapping2->key, mapping2->modifiers);
                text_draw_centered(keyname, HOTKEY_X_OFFSET_2 + 3, text_offset,
                    HOTKEY_BTN_WIDTH - 6, FONT_NORMAL_WHITE, 0);
                graphics_reset_clip_rectangle();
            }
        }
    }

    for (int i = 0; i < NUM_BOTTOM_BUTTONS; i++) {
        text_draw_centered(get_custom_string(bottom_button_texts[i]),
            bottom_buttons[i].x, bottom_buttons[i].y + 9,
            bottom_buttons[i].width, FONT_NORMAL_BLACK, 0);
    }

    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    scrollbar_draw(&scrollbar);

    for (int i = 0; i < NUM_VISIBLE_OPTIONS; i++) {
        hotkey_widget *widget = &hotkey_widgets[i + scrollbar.scroll_position];
        if (widget->action != HOTKEY_HEADER) {
            generic_button *btn = &hotkey_buttons[2 * i];
            button_border_draw(btn->x, btn->y, btn->width, btn->height, data.focus_button == 1 + 2 * i);
            btn++;
            button_border_draw(btn->x, btn->y, btn->width, btn->height, data.focus_button == 2 + 2 * i);
        }
    }

    for (int i = 0; i < NUM_BOTTOM_BUTTONS; i++) {
        button_border_draw(bottom_buttons[i].x, bottom_buttons[i].y,
            bottom_buttons[i].width, bottom_buttons[i].height,
            data.bottom_focus_button == i + 1);
    }
    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    const mouse *m_dialog = mouse_in_dialog(m);
    if (scrollbar_handle_mouse(&scrollbar, m_dialog)) {
        return;
    }

    int handled = 0;
    handled |= generic_buttons_handle_mouse(m_dialog, 0, 0,
        hotkey_buttons, NUM_VISIBLE_OPTIONS * 2, &data.focus_button);
    handled |= generic_buttons_handle_mouse(m_dialog, 0, 0,
        bottom_buttons, NUM_BOTTOM_BUTTONS, &data.bottom_focus_button);
    if (!handled && (m->right.went_up || h->escape_pressed)) {
        window_config_show();
    }
}

static const uint8_t *hotkey_action_name_for(hotkey_action action)
{
    const uint8_t *name = 0;
    for (int i = 0; i < NUM_VISIBLE_OPTIONS + scrollbar.max_scroll_position; i++) {
        hotkey_widget *widget = &hotkey_widgets[i];
        if ((unsigned) widget->action == action) {
            if (widget->name_translation != TR_NONE) {
                name = get_custom_string(widget->name_translation);
            } else {
                name = lang_get_string(widget->name_text_group, widget->name_text_id);
            }
            break;
        }
    }
    return name;
}

static void set_hotkey(hotkey_action action, int index, key_type key, key_modifier_type modifiers)
{
    int is_duplicate_hotkey = 0;
    // check if new key combination already assigned to another action
    if (key != KEY_TYPE_NONE) {
        for (unsigned test_action = 0; test_action < HOTKEY_MAX_ITEMS; test_action++) {
            for (int test_index = 0; test_index < 2; test_index++) {
                if (data.mappings[test_action][test_index].key == key
                    && data.mappings[test_action][test_index].modifiers == modifiers) {
                    is_duplicate_hotkey = 1;
                    // example explanation next "if" check:
                    // "Fire overlay" already has hotkey "F" and user tries set same hotkey "F" again to "Fire overlay"
                    // we must skip show warning window for better user experience
                    if (!(test_action == action && test_index == index)) {
                        window_plain_message_dialog_show_with_extra(
                            TR_HOTKEY_DUPLICATE_TITLE, TR_HOTKEY_DUPLICATE_MESSAGE,
                            hotkey_action_name_for(test_action));
                    }
                    break;
                }
            }
            if (is_duplicate_hotkey) {
                break;
            }
        }
    }
    if (!is_duplicate_hotkey) {
        data.mappings[action][index].key = key;
        data.mappings[action][index].modifiers = modifiers;
    }
}

static void button_hotkey(int row, int is_alternative)
{
    hotkey_widget *widget = &hotkey_widgets[row + scrollbar.scroll_position];
    if (widget->action == HOTKEY_HEADER) {
        return;
    }
    window_hotkey_editor_show(widget->action, is_alternative, set_hotkey);
}

static void button_reset_defaults(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    for (int action = 0; action < HOTKEY_MAX_ITEMS; action++) {
        for (int index = 0; index < 2; index++) {
            data.mappings[action][index] = *hotkey_default_for_action(action, index);
        }
    }
    window_invalidate();
}

static void on_scroll(void)
{
    window_invalidate();
}

static void button_close(int save, __attribute__((unused)) int param2)
{
    if (!save) {
        window_go_back();
        return;
    }
    hotkey_config_clear();
    for (int action = 0; action < HOTKEY_MAX_ITEMS; action++) {
        for (int index = 0; index < 2; index++) {
            if (data.mappings[action][index].key != KEY_TYPE_NONE) {
                hotkey_config_add_mapping(&data.mappings[action][index]);
            }
        }
    }
    hotkey_config_save();
    window_go_back();
}

void window_hotkey_config_show(void)
{
    window_type window = {
        WINDOW_HOTKEY_CONFIG,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    init();
    window_show(&window);
}
