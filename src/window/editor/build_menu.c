#include "build_menu.h"

#include "city/view.h"
#include "editor/tool.h"
#include "graphics/generic_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "widget/map_editor.h"
#include "widget/sidebar/editor.h"
#include "window/editor/map.h"

#define MENU_X_OFFSET 170
#define MENU_Y_OFFSET 110
#define MENU_ITEM_HEIGHT 24
#define MENU_ITEM_WIDTH 160
#define MENU_CLICK_MARGIN 20
#define MAX_ITEMS_PER_MENU 16

static void button_menu_item(int index, int param2);

static struct generic_button_t build_menu_buttons[] = {
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

static char editor_menu_types_strings[][MAX_ITEMS_PER_MENU][19] = {
    {"Small shrub", "Medium shrub", "Large shrub", "Largest shrub"},
    {"Raise land", "Lower land", "Access ramp"},
    {"Small rock", "Medium rock", "Large rock"},
    {"Tiny brush", "Small brush", "Medium brush", "Big brush", "Biggest brush"},
    {"Earthquake point 1", "Earthquake point 2", "Earthquake point 3", "Earthquake point 4", "Earthquake point 5", "Earthquake point 6", "Earthquake point 7", "Earthquake point 8"},
    {"Invasion point 1", "Invasion point 2", "Invasion point 3", "Invasion point 4", "Invasion point 5", "Invasion point 6", "Invasion point 7", "Invasion point 8"},
    {"Entry point", "Exit point"},
    {"River entry", "River exit"},
    {"Native hut", "Native center", "Native field", "Vacant lot"},
    {"Fishing point 1", "Fishing point 2", "Fishing point 3", "Fishing point 4", "Fishing point 5", "Fishing point 6", "Fishing point 7", "Fishing point 8",
    "Herd point 1", "Herd point 2", "Herd point 3", "Herd point 4", "Herd point 5", "Herd point 6", "Herd point 7", "Herd point 8"},
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
    for (int i = 0; i < MAX_ITEMS_PER_MENU; i++) {
        if (editor_menu_types_strings[data.selected_submenu][i][0]) {
            data.num_items++;
        }
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
        label_draw(x_offset - MENU_X_OFFSET, data.y_offset + MENU_Y_OFFSET + MENU_ITEM_HEIGHT * i, 10, data.focus_button_id == i + 1 ? 1 : 2);
        text_draw_centered(editor_menu_types_strings[data.selected_submenu][i], x_offset - MENU_X_OFFSET, data.y_offset + MENU_Y_OFFSET + 3 + MENU_ITEM_HEIGHT * i,
            MENU_ITEM_WIDTH, FONT_NORMAL_GREEN, COLOR_BLACK);
    }
}

static void draw_foreground(void)
{
    window_editor_map_draw();
    draw_menu_buttons();
}

static int click_outside_menu(const struct mouse_t *m, int x_offset)
{
    return m->left.went_up &&
        (m->x < x_offset - MENU_X_OFFSET - MENU_CLICK_MARGIN ||
            m->x > x_offset + MENU_CLICK_MARGIN ||
            m->y < data.y_offset + MENU_Y_OFFSET - MENU_CLICK_MARGIN ||
            m->y > data.y_offset + MENU_Y_OFFSET + MENU_CLICK_MARGIN + MENU_ITEM_HEIGHT * data.num_items);
}

static int handle_build_submenu(const struct mouse_t *m)
{
    return generic_buttons_handle_mouse(
        m, get_sidebar_x_offset() - MENU_X_OFFSET, data.y_offset + MENU_Y_OFFSET,
               build_menu_buttons, data.num_items, &data.focus_button_id);
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (handle_build_submenu(m)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed || click_outside_menu(m, get_sidebar_x_offset())) {
        data.selected_submenu = MENU_NONE;
        window_editor_map_show();
    }
}

static void button_menu_item(int index, __attribute__((unused)) int param2)
{
    widget_map_editor_clear_current_tile();

    switch (data.selected_submenu) {
        case MENU_SHRUB:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_SMALL_SHRUB, 0); break;
                case 1: editor_tool_set_with_id(TOOL_MEDIUM_SHRUB, 0); break;
                case 2: editor_tool_set_with_id(TOOL_LARGE_SHRUB, 0); break;
                case 3: editor_tool_set_with_id(TOOL_LARGEST_SHRUB, 0); break;
            }
            break;
        case MENU_ELEVATION:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_RAISE_LAND, 0); break;
                case 1: editor_tool_set_with_id(TOOL_LOWER_LAND, 0); break;
                case 2: editor_tool_set_with_id(TOOL_ACCESS_RAMP, 0); break;
            }
            break;
        case MENU_ROCK:
            switch (index) {
                case 0: editor_tool_set_with_id(TOOL_SMALL_ROCK, 0); break;
                case 1: editor_tool_set_with_id(TOOL_MEDIUM_ROCK, 0); break;
                case 2: editor_tool_set_with_id(TOOL_LARGE_ROCK, 0); break;
            }
            break;
        case MENU_BRUSH_SIZE:
            editor_tool_set_brush_size(index);
            break;
        case MENU_EARTHQUAKE_POINTS:
            editor_tool_set_with_id(TOOL_EARTHQUAKE_POINT, index);
            break;
        case MENU_INVASION_POINTS:
            editor_tool_set_with_id(TOOL_INVASION_POINT, index);
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
    struct window_type_t window = {
        WINDOW_EDITOR_BUILD_MENU,
        0,
        draw_foreground,
        handle_input,
    };
    window_show(&window);
}

void window_editor_build_menu_hide(void)
{
    data.selected_submenu = MENU_NONE;
    window_editor_map_show();
}
