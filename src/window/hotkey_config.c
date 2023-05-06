#include "hotkey_config.h"

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
} hotkey_widget;

static hotkey_widget hotkey_widgets[] = {
    {HOTKEY_HEADER},
    {HOTKEY_ARROW_UP},
    {HOTKEY_ARROW_DOWN},
    {HOTKEY_ARROW_LEFT},
    {HOTKEY_ARROW_RIGHT},
    {HOTKEY_HEADER},
    {HOTKEY_TOGGLE_FULLSCREEN},
    {HOTKEY_RESET_WINDOW},
    {HOTKEY_SAVE_SCREENSHOT},
    {HOTKEY_SAVE_CITY_SCREENSHOT},
    {HOTKEY_LOAD_FILE},
    {HOTKEY_SAVE_FILE},
    {HOTKEY_HEADER},
    {HOTKEY_DECREASE_GAME_SPEED},
    {HOTKEY_INCREASE_GAME_SPEED},
    {HOTKEY_TOGGLE_PAUSE},
    {HOTKEY_ROTATE_MAP_LEFT},
    {HOTKEY_ROTATE_MAP_RIGHT},
    {HOTKEY_REPLAY_MAP},
    {HOTKEY_CYCLE_LEGION},
    {HOTKEY_RETURN_LEGIONS_TO_FORT},
    {HOTKEY_SHOW_LAST_ADVISOR},
    {HOTKEY_SHOW_EMPIRE_MAP},
    {HOTKEY_SHOW_MESSAGES},
    {HOTKEY_GO_TO_PROBLEM},
    {HOTKEY_HEADER},
    {HOTKEY_BUILD_CLONE},
    {HOTKEY_CYCLE_BUILDINGS},
    {HOTKEY_CYCLE_BUILDINGS_REVERSE},
    {HOTKEY_UNDO},
    {HOTKEY_BUILD_VACANT_HOUSE},
    {HOTKEY_BUILD_CLEAR_LAND},
    {HOTKEY_BUILD_ROAD},
    {HOTKEY_BUILD_RESERVOIR},
    {HOTKEY_BUILD_AQUEDUCT},
    {HOTKEY_BUILD_FOUNTAIN},
    {HOTKEY_BUILD_WELL},
    {HOTKEY_BUILD_BARBER},
    {HOTKEY_BUILD_BATHHOUSE},
    {HOTKEY_BUILD_DOCTOR},
    {HOTKEY_BUILD_HOSPITAL},
    {HOTKEY_BUILD_SMALL_TEMPLE_CERES},
    {HOTKEY_BUILD_SMALL_TEMPLE_NEPTUNE},
    {HOTKEY_BUILD_SMALL_TEMPLE_MERCURY},
    {HOTKEY_BUILD_SMALL_TEMPLE_MARS},
    {HOTKEY_BUILD_SMALL_TEMPLE_VENUS},
    {HOTKEY_BUILD_LARGE_TEMPLE_CERES},
    {HOTKEY_BUILD_LARGE_TEMPLE_NEPTUNE},
    {HOTKEY_BUILD_LARGE_TEMPLE_MERCURY},
    {HOTKEY_BUILD_LARGE_TEMPLE_MARS},
    {HOTKEY_BUILD_LARGE_TEMPLE_VENUS},
    {HOTKEY_BUILD_ORACLE},
    {HOTKEY_BUILD_SCHOOL},
    {HOTKEY_BUILD_ACADEMY},
    {HOTKEY_BUILD_LIBRARY},
    {HOTKEY_BUILD_MISSION_POST},
    {HOTKEY_BUILD_THEATER},
    {HOTKEY_BUILD_AMPHITHEATER},
    {HOTKEY_BUILD_COLOSSEUM},
    {HOTKEY_BUILD_HIPPODROME},
    {HOTKEY_BUILD_GLADIATOR_SCHOOL},
    {HOTKEY_BUILD_LION_HOUSE},
    {HOTKEY_BUILD_ACTOR_COLONY},
    {HOTKEY_BUILD_CHARIOT_MAKER},
    {HOTKEY_BUILD_FORUM},
    {HOTKEY_BUILD_SENATE},
    {HOTKEY_BUILD_GOVERNORS_HOUSE},
    {HOTKEY_BUILD_GOVERNORS_VILLA},
    {HOTKEY_BUILD_GOVERNORS_PALACE},
    {HOTKEY_BUILD_SMALL_STATUE},
    {HOTKEY_BUILD_MEDIUM_STATUE},
    {HOTKEY_BUILD_LARGE_STATUE},
    {HOTKEY_BUILD_TRIUMPHAL_ARCH},
    {HOTKEY_BUILD_GARDENS},
    {HOTKEY_BUILD_PLAZA},
    {HOTKEY_BUILD_ENGINEERS_POST},
    {HOTKEY_BUILD_LOW_BRIDGE},
    {HOTKEY_BUILD_SHIP_BRIDGE},
    {HOTKEY_BUILD_SHIPYARD},
    {HOTKEY_BUILD_DOCK},
    {HOTKEY_BUILD_WHARF},
    {HOTKEY_BUILD_WALL},
    {HOTKEY_BUILD_TOWER},
    {HOTKEY_BUILD_GATEHOUSE},
    {HOTKEY_BUILD_PREFECTURE},
    {HOTKEY_BUILD_FORT_LEGIONARIES},
    {HOTKEY_BUILD_FORT_JAVELIN},
    {HOTKEY_BUILD_FORT_MOUNTED},
    {HOTKEY_BUILD_MILITARY_ACADEMY},
    {HOTKEY_BUILD_BARRACKS},
    {HOTKEY_BUILD_WHEAT_FARM},
    {HOTKEY_BUILD_VEGETABLE_FARM},
    {HOTKEY_BUILD_FRUIT_FARM},
    {HOTKEY_BUILD_OLIVE_FARM},
    {HOTKEY_BUILD_VINES_FARM},
    {HOTKEY_BUILD_PIG_FARM},
    {HOTKEY_BUILD_CLAY_PIT},
    {HOTKEY_BUILD_MARBLE_QUARRY},
    {HOTKEY_BUILD_IRON_MINE},
    {HOTKEY_BUILD_TIMBER_YARD},
    {HOTKEY_BUILD_WINE_WORKSHOP},
    {HOTKEY_BUILD_OIL_WORKSHOP},
    {HOTKEY_BUILD_WEAPONS_WORKSHOP},
    {HOTKEY_BUILD_FURNITURE_WORKSHOP},
    {HOTKEY_BUILD_POTTERY_WORKSHOP},
    {HOTKEY_BUILD_MARKET},
    {HOTKEY_BUILD_GRANARY},
    {HOTKEY_BUILD_WAREHOUSE},
    {HOTKEY_HEADER},
    {HOTKEY_SHOW_OVERLAY_WATER},
    {HOTKEY_SHOW_OVERLAY_FIRE},
    {HOTKEY_SHOW_OVERLAY_DAMAGE},
    {HOTKEY_SHOW_OVERLAY_CRIME},
    {HOTKEY_SHOW_OVERLAY_PROBLEMS},
    {HOTKEY_HEADER},
    {HOTKEY_GO_TO_BOOKMARK_1},
    {HOTKEY_GO_TO_BOOKMARK_2},
    {HOTKEY_GO_TO_BOOKMARK_3},
    {HOTKEY_GO_TO_BOOKMARK_4},
    {HOTKEY_SET_BOOKMARK_1},
    {HOTKEY_SET_BOOKMARK_2},
    {HOTKEY_SET_BOOKMARK_3},
    {HOTKEY_SET_BOOKMARK_4},
    {HOTKEY_HEADER},
    {HOTKEY_EDITOR_TOGGLE_BATTLE_INFO},
    {HOTKEY_HEADER},
    {HOTKEY_CHEAT_MONEY},
    {HOTKEY_CHEAT_INVASION},
    {HOTKEY_CHEAT_VICTORY},
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

static struct {
    int focus_button;
    int bottom_focus_button;
    hotkey_mapping mappings[HOTKEY_MAX_ITEMS][2];
} data;

static uint8_t hotkey_strings[][28] = {
    "Brutus hotkey configuration", // 0
    "Hotkey", // 1
    "Alternative", // 2
    "Reset defaults", // 3
    "Cancel", // 4
    "OK", // 5
};

static uint8_t hotkey_widget_strings[][29] = {
    "Arrow keys", // 0
    "Up", // 1
    "Down", // 2
    "Left", // 3
    "Right", // 4
    "Global hotkeys", // 5
    "Toggle fullscreen", // 6
    "Reset window", // 7
    "Save screenshot", // 8
    "Save full city screenshot", // 9
    "Load file", // 10
    "Save file", // 11
    "City hotkeys", // 12
    "Decrease game speed", // 13
    "Increase game speed", // 14
    "Toggle pause", // 15
    "Rotate map left", // 16
    "Rotate map right", // 17
    "Replay map", // 18
    "Cycle through legions", // 19
    "Return legions to fort", // 20
    "Show last advisor", // 21
    "Show empire map", // 22
    "Show messages", // 23
    "Go to problem", // 24
    "Construction hotkeys", // 25
    "Clone building under cursor", // 26
    "Cycle through buildings", // 27
    "Cycle back through buildings", // 28
    "Undo last building", // 29
    "Housing", // 30 --- structures strings overlap with allowed_buildings
    "Clear terrain", // 31
    "Road", // 32
    "Reservoir", // 33
    "Aqueduct", // 34
    "Fountain", // 35
    "Well", // 36
    "Barber", // 37
    "Bathhouse", // 38
    "Doctor", // 39
    "Hospital", // 40
    "Small temple: Ceres", // 41
    "Small temple: Neptune", // 42
    "Small temple: Mercury", // 43
    "Small temple: Mars", // 44
    "Small temple: Venus", // 45
    "Large temple: Ceres", // 46
    "Large temple: Neptune", // 47
    "Large temple: Mercury", // 48
    "Large temple: Mars", // 49
    "Large temple: Venus", // 50
    "Oracle", // 51
    "School", // 52
    "Academy", // 53
    "Library", // 54
    "Mission post", // 55
    "Theater", // 56
    "Amphitheater", // 57
    "Colosseum", // 58
    "Hippodrome", // 59
    "Gladiator school", // 60
    "Lion house", // 61
    "Actor colony", // 62
    "Chariot maker", // 63
    "Forum", // 64
    "Senate", // 65
    "Governor's house", // 66
    "Governor's villa", // 67
    "Governor's palace", // 68
    "Statue: Small", // 69
    "Statue: Medium", // 70
    "Statue: Large", // 71
    "Triumphal arch", // 72
    "Gardens", // 73
    "Plaza", // 74
    "Engineers post", // 75
    "Bridge: Low", // 76
    "Bridge: Ship", // 77
    "Shipyard", // 78
    "Dock", // 79
    "Wharf", // 80
    "Wall", // 81
    "Tower", // 82
    "Gatehouse", // 83
    "Prefecture", // 84
    "Fort: Legionaries", // 85
    "Fort: Javelin", // 86
    "Fort: Mounted", // 87
    "Military academy", // 88
    "Barracks", // 89
    "Farm: Wheat", // 90
    "Farm: Vegetables", // 91
    "Farm: Fruit", // 92
    "Farm: Olives", // 93
    "Farm: Vines", // 94
    "Farm: Pigs", // 95
    "Clay pit", // 96
    "Marble quarry", // 97
    "Iron mine", // 98
    "Timber yard", // 99
    "Workshop: Wine", // 100
    "Workshop: Oil", // 101
    "Workshop: Weapons", // 102
    "Workshop: Furniture", // 103
    "Workshop: Pottery", // 104
    "Market", // 105
    "Granary", // 106
    "Warehouse", // 107 ---
    "Overlays", // 108
    "Show water overlay", // 109
    "Show fire overlay", // 110
    "Damage overlay", // 111
    "Crime overlay", // 112
    "Problems overlay", // 113
    "City map bookmarks", // 114
    "Go to bookmark 1", // 115
    "Go to bookmark 2", // 116
    "Go to bookmark 3", // 117
    "Go to bookmark 4", // 118
    "Set bookmark 1", // 119
    "Set bookmark 2", // 120
    "Set bookmark 3", // 121
    "Set bookmark 4", // 122
    "Editor", // 123
    "Toggle battle info", // 124
    "Cheats", // 125
    "Cheat: money", // 126
    "Cheat: invasion", // 127
    "Cheat: victory", // 128
};

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

    text_draw_centered(hotkey_strings[0], 16, 16, 608, FONT_LARGE_BLACK, 0);

    text_draw_centered(hotkey_strings[1], HOTKEY_X_OFFSET_1, 55, HOTKEY_BTN_WIDTH, FONT_NORMAL_BLACK, 0);
    text_draw_centered(hotkey_strings[2], HOTKEY_X_OFFSET_2, 55, HOTKEY_BTN_WIDTH, FONT_NORMAL_BLACK, 0);

    inner_panel_draw(20, 72, 35, 22);
    int y_base = 80;
    for (int i = 0; i < NUM_VISIBLE_OPTIONS; i++) {
        int current_pos = i + scrollbar.scroll_position;
        hotkey_widget *widget = &hotkey_widgets[current_pos];
        int text_offset = y_base + 6 + 24 * i;
        if (current_pos == 0 || current_pos == 5 || current_pos == 12 || current_pos == 25
        || current_pos == 108 || current_pos == 114 || current_pos == 123 || current_pos == 125) { // headers
            text_draw(hotkey_widget_strings[current_pos], 32, text_offset, FONT_NORMAL_WHITE, 0);
        } else {
            text_draw(hotkey_widget_strings[current_pos], 32, text_offset, FONT_NORMAL_GREEN, 0);
            const hotkey_mapping *mapping1 = &data.mappings[widget->action][0];
            if (mapping1->key) {
                const uint8_t *keyname = key_combination_display_name(mapping1->key, mapping1->modifiers);
                graphics_set_clip_rectangle(HOTKEY_X_OFFSET_1, text_offset, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT);
                text_draw_centered(keyname, HOTKEY_X_OFFSET_1 + 3, text_offset, HOTKEY_BTN_WIDTH - 6, FONT_NORMAL_WHITE, 0);
                graphics_reset_clip_rectangle();
            }

            const hotkey_mapping *mapping2 = &data.mappings[widget->action][1];
            if (mapping2->key) {
                graphics_set_clip_rectangle(HOTKEY_X_OFFSET_2, text_offset, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT);
                const uint8_t *keyname = key_combination_display_name(mapping2->key, mapping2->modifiers);
                text_draw_centered(keyname, HOTKEY_X_OFFSET_2 + 3, text_offset, HOTKEY_BTN_WIDTH - 6, FONT_NORMAL_WHITE, 0);
                graphics_reset_clip_rectangle();
            }
        }
    }

    for (int i = 0; i < NUM_BOTTOM_BUTTONS; i++) {
        text_draw_centered(hotkey_strings[i + 3], bottom_buttons[i].x, bottom_buttons[i].y + 9, bottom_buttons[i].width, FONT_NORMAL_BLACK, 0);
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

static void set_hotkey(hotkey_action action, int index, key_type key, key_modifier_type modifiers)
{
    // clear conflicting mappings
    for (int i = 0; i < HOTKEY_MAX_ITEMS; i++) {
        for (int j = 0; j < 2; j++) {
            if (data.mappings[i][j].key == key && data.mappings[i][j].modifiers == modifiers) {
                data.mappings[i][j].key = KEY_TYPE_NONE;
                data.mappings[i][j].modifiers = KEY_MOD_NONE;
            }
        }
    }
    // set new mapping
    data.mappings[action][index].key = key;
    data.mappings[action][index].modifiers = modifiers;
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
