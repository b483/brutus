#include "build_menu.h"

#include "city/view.h"
#include "editor/tool.h"
#include "game/custom_strings.h"
#include "graphics/generic_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "scenario/data.h"
#include "widget/map_editor.h"
#include "widget/sidebar/editor.h"
#include "window/editor/map.h"

#define MENU_X_OFFSET 170
#define MENU_Y_OFFSET 110
#define MENU_ITEM_HEIGHT 24
#define MENU_ITEM_WIDTH 160
#define MENU_CLICK_MARGIN 20

static void button_menu_item(int index, int param2);

static generic_button build_menu_buttons[] = {
    {0, 0, 160, 20, button_menu_item, button_none, 0, 0},
    {0, 24, 160, 20, button_menu_item, button_none, 1, 0},
    {0, 48, 160, 20, button_menu_item, button_none, 2, 0},
    {0, 72, 160, 20, button_menu_item, button_none, 3, 0},
    {0, 96, 160, 20, button_menu_item, button_none, 4, 0},
    {0, 120, 160, 20, button_menu_item, button_none, 5, 0},
    {0, 144, 160, 20, button_menu_item, button_none, 6, 0},
    {0, 168, 160, 20, button_menu_item, button_none, 7, 0},
    {0, 192, 160, 20, button_menu_item, button_none, 8, 0},
    {0, 216, 160, 20, button_menu_item, button_none, 9, 0},
    {0, 240, 160, 20, button_menu_item, button_none, 10, 0},
    {0, 264, 160, 20, button_menu_item, button_none, 11, 0},
    {0, 288, 160, 20, button_menu_item, button_none, 12, 0},
    {0, 312, 160, 20, button_menu_item, button_none, 13, 0},
    {0, 336, 160, 20, button_menu_item, button_none, 14, 0},
    {0, 360, 160, 20, button_menu_item, button_none, 15, 0}
};

#define MAX_ITEMS_PER_MENU 17
static const int MENU_TYPES[MENU_NUM_ITEMS][MAX_ITEMS_PER_MENU] = {
    {TR_EDITOR_TOOL_SUBMENU_RAISE_LAND, TR_EDITOR_TOOL_SUBMENU_LOWER_LAND, TR_EDITOR_TOOL_SUBMENU_ACCESS_RAMP, -1},
    {TR_EDITOR_TOOL_SUBMENU_TINY_BRUSH, TR_EDITOR_TOOL_SUBMENU_SMALL_BRUSH,
    TR_EDITOR_TOOL_SUBMENU_MEDIUM_BRUSH, TR_EDITOR_TOOL_SUBMENU_BIG_BRUSH, TR_EDITOR_TOOL_SUBMENU_BIGGEST_BRUSH, -1},
    {TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_1, TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_2, TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_3,
    TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_4, TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_5, TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_6,
    TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_7, TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_8, -1},
    {TR_EDITOR_TOOL_SUBMENU_ENTRY_POINT, TR_EDITOR_TOOL_SUBMENU_EXIT_POINT, -1},
    {TR_EDITOR_TOOL_SUBMENU_RIVER_ENTRY, TR_EDITOR_TOOL_SUBMENU_RIVER_EXIT, -1},
    {TR_EDITOR_TOOL_SUBMENU_NATIVE_HUT, TR_EDITOR_TOOL_SUBMENU_NATIVE_CENTER, TR_EDITOR_TOOL_SUBMENU_NATIVE_FIELD,
    TR_EDITOR_TOOL_SUBMENU_VACANT_LOT, -1},
    {TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_1, TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_2, TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_3,
    TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_4, TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_5, TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_6,
    TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_7, TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_8,
    TR_EDITOR_TOOL_SUBMENU_HERD_POINT_1, TR_EDITOR_TOOL_SUBMENU_HERD_POINT_2, TR_EDITOR_TOOL_SUBMENU_HERD_POINT_3,
    TR_EDITOR_TOOL_SUBMENU_HERD_POINT_4, TR_EDITOR_TOOL_SUBMENU_HERD_POINT_5, TR_EDITOR_TOOL_SUBMENU_HERD_POINT_6,
    TR_EDITOR_TOOL_SUBMENU_HERD_POINT_7, TR_EDITOR_TOOL_SUBMENU_HERD_POINT_8, -1}
};

static struct {
    int selected_submenu;
    int num_items;
    int y_offset;
    int focus_button_id;
} data = { MENU_NONE, 0, 0, 0 };

static void init(int submenu)
{
    data.selected_submenu = submenu;
    data.num_items = 0;
    for (int i = 0; i < MAX_ITEMS_PER_MENU && MENU_TYPES[submenu][i] >= 0; i++) {
        data.num_items++;
    }
    data.y_offset = 180;
}

static int get_sidebar_x_offset(void)
{
    int view_x, view_y, view_width, view_height;
    city_view_get_viewport(&view_x, &view_y, &view_width, &view_height);
    return view_x + view_width;
}

static void draw_menu_buttons(void)
{
    int x_offset = get_sidebar_x_offset();
    for (int i = 0; i < data.num_items; i++) {
        label_draw(x_offset - MENU_X_OFFSET, data.y_offset + MENU_Y_OFFSET + MENU_ITEM_HEIGHT * i, 10,
            data.focus_button_id == i + 1 ? 1 : 2);
        text_draw_centered(get_custom_string(MENU_TYPES[data.selected_submenu][i]), x_offset - MENU_X_OFFSET,
            data.y_offset + MENU_Y_OFFSET + 3 + MENU_ITEM_HEIGHT * i,
            MENU_ITEM_WIDTH, FONT_NORMAL_GREEN, COLOR_BLACK);
    }
}

static void draw_foreground(void)
{
    window_editor_map_draw();
    draw_menu_buttons();
}

static int click_outside_menu(const mouse *m, int x_offset)
{
    return m->left.went_up &&
        (m->x < x_offset - MENU_X_OFFSET - MENU_CLICK_MARGIN ||
            m->x > x_offset + MENU_CLICK_MARGIN ||
            m->y < data.y_offset + MENU_Y_OFFSET - MENU_CLICK_MARGIN ||
            m->y > data.y_offset + MENU_Y_OFFSET + MENU_CLICK_MARGIN + MENU_ITEM_HEIGHT * data.num_items);
}

static int handle_build_submenu(const mouse *m)
{
    return generic_buttons_handle_mouse(
        m, get_sidebar_x_offset() - MENU_X_OFFSET, data.y_offset + MENU_Y_OFFSET,
               build_menu_buttons, data.num_items, &data.focus_button_id);
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (handle_build_submenu(m)) {
        return;
    }
    if (input_go_back_requested(m, h) || click_outside_menu(m, get_sidebar_x_offset())) {
        data.selected_submenu = MENU_NONE;
        window_editor_map_show();
    }
}

static void button_menu_item(int index, __attribute__((unused)) int param2)
{
    widget_map_editor_clear_current_tile();

    switch (data.selected_submenu) {
        case MENU_BRUSH_SIZE:
            editor_tool_set_brush_size(index + 1);
            break;
        case MENU_ELEVATION:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_RAISE_LAND, 0); break;
                case 1: editor_tool_set_with_id(TOOL_LOWER_LAND, 0); break;
                case 2: editor_tool_set_with_id(TOOL_ACCESS_RAMP, 0); break;
            }
            break;
        case MENU_PEOPLE_POINTS:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_ENTRY_POINT, 0); break;
                case 1: editor_tool_set_with_id(TOOL_EXIT_POINT, 0); break;
            }
            break;
        case MENU_RIVER_POINTS:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_RIVER_ENTRY_POINT, 0); break;
                case 1: editor_tool_set_with_id(TOOL_RIVER_EXIT_POINT, 0); break;
            }
            break;
        case MENU_NATIVE_BUILDINGS:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_NATIVE_HUT, 0); break;
                case 1: editor_tool_set_with_id(TOOL_NATIVE_CENTER, 0); break;
                case 2: editor_tool_set_with_id(TOOL_NATIVE_FIELD, 0); break;
                case 3: editor_tool_set_with_id(TOOL_HOUSE_VACANT_LOT, 0); break;
            }
            break;
        case MENU_INVASION_POINTS:
            editor_tool_set_with_id(TOOL_INVASION_POINT, index);
            break;
        case MENU_ANIMAL_POINTS:
            if (index < MAX_FISH_POINTS) {
                editor_tool_set_with_id(TOOL_FISHING_POINT, index);
            } else {
                editor_tool_set_with_id(TOOL_HERD_POINT, index - MAX_FISH_POINTS);
            }
            break;
    }
    data.selected_submenu = MENU_NONE;
    window_editor_map_show();
}

void window_editor_build_menu_show(int submenu)
{
    if (submenu == MENU_NONE || submenu == data.selected_submenu) {
        window_editor_build_menu_hide();
        return;
    }
    init(submenu);
    window_type window = {
        WINDOW_EDITOR_BUILD_MENU,
        0,
        draw_foreground,
        handle_input,
        0
    };
    window_show(&window);
}

void window_editor_build_menu_hide(void)
{
    data.selected_submenu = MENU_NONE;
    window_editor_map_show();
}
