#include "editor.h"

#include "core/image_group_editor.h"
#include "editor/tool.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "scenario/editor_events.h"
#include "scenario/map.h"
#include "widget/map_editor.h"
#include "widget/minimap.h"
#include "widget/sidebar/common.h"
#include "window/editor/attributes.h"
#include "window/editor/build_menu.h"
#include "window/editor/empire.h"
#include "window/editor/map.h"

#define MINIMAP_Y_OFFSET 30

static void button_build_tool(int tool, int param2);
static void button_build_menu(int submenu, int param2);

static void button_attributes(int show, int param2);
static void button_empire(int param1, int param2);

static struct image_button_t buttons_build[] = {
    {7, 123, 71, 23, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 48, button_attributes, button_none, 1, 0, 1, 0, 0, 0},
    {84, 123, 71, 23, IB_NORMAL, GROUP_SIDEBAR_ADVISORS_EMPIRE, 3, button_empire, button_none, 0, 0, 1, 0, 0, 0},
    {13, 267, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 0, button_build_tool, button_none, TOOL_GRASS, 0, 1, 0, 0, 0},
    {63, 267, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 3, button_build_menu, button_none, MENU_SHRUB, 0, 1, 0, 0, 0},
    {113, 267, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 6, button_build_tool, button_none, TOOL_WATER, 0, 1, 0, 0, 0},
    {13, 303, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 21, button_build_menu, button_none, MENU_ELEVATION, 0, 1, 0, 0, 0},
    {63, 303, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 12, button_build_tool, button_none, TOOL_TREES, 0, 1, 0, 0, 0},
    {113, 303, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 15, button_build_menu, button_none, MENU_ROCK, 0, 1, 0, 0, 0},
    {13, 339, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 18, button_build_tool, button_none, TOOL_MEADOW, 0, 1, 0, 0, 0},
    {63, 339, 39, 26, IB_NORMAL, GROUP_EDITOR_SIDEBAR_BUTTONS, 30, button_build_tool, button_none, TOOL_ROAD, 0, 1, 0, 0, 0},
    {113, 339, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 24, button_build_menu, button_none, MENU_BRUSH_SIZE, 0, 1, 0, 0, 0},
    {13, 375, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 9, button_build_menu, button_none, MENU_EARTHQUAKE_POINTS, 0, 1, 0, 0, 0},
    {63, 375, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 39, button_build_menu, button_none, MENU_INVASION_POINTS, 0, 1, 0, 0, 0},
    {113, 375, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 42, button_build_menu, button_none, MENU_PEOPLE_POINTS, 0, 1, 0, 0, 0},
    {13, 411, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 33, button_build_menu, button_none, MENU_RIVER_POINTS, 0, 1, 0, 0, 0},
    {63, 411, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 27, button_build_menu, button_none, MENU_NATIVE_BUILDINGS, 0, 1, 0, 0, 0},
    {113, 411, 39, 26, IB_BUILD, GROUP_EDITOR_SIDEBAR_BUTTONS, 51, button_build_menu, button_none, MENU_ANIMAL_POINTS, 0, 1, 0, 0, 0},
};

static uint8_t editor_sidebar_menu_label_strings[][15] = {
    "Grass",
    "Small shrub",
    "Medium shrub",
    "Large shrub",
    "Largest shrub",
    "Water",
    "Raise land",
    "Lower land",
    "Access ramp",
    "Trees",
    "Small rock",
    "Medium rock",
    "Large rock",
    "Meadow",
    "Road",
    "Earthquake",
    "Invasion point",
    "Entry point",
    "Exit point",
    "River IN",
    "River OUT",
    "Native hut",
    "Native center",
    "Native field",
    "Vacant lot",
    "Fishing waters",
    "Herd point",
};

static void draw_buttons(void)
{
    image_buttons_draw(sidebar_common_get_x_offset_expanded(), TOP_MENU_HEIGHT, buttons_build, sizeof(buttons_build) / sizeof(struct image_button_t));
}

static void draw_status(void)
{
    int x_offset = sidebar_common_get_x_offset_expanded();
    inner_panel_draw(x_offset + 1, 175, 10, 7);
    int text_offset = x_offset + 6;

    int selected_tool = editor_tool_type();
    text_draw(editor_sidebar_menu_label_strings[selected_tool], text_offset, 178, FONT_NORMAL_WHITE, 0);
    switch (selected_tool) {
        case TOOL_GRASS:
        case TOOL_SMALL_SHRUB:
        case TOOL_MEDIUM_SHRUB:
        case TOOL_LARGE_SHRUB:
        case TOOL_LARGEST_SHRUB:
        case TOOL_WATER:
        case TOOL_TREES:
        case TOOL_SMALL_ROCK:
        case TOOL_MEDIUM_ROCK:
        case TOOL_LARGE_ROCK:
        case TOOL_MEADOW:
        case TOOL_RAISE_LAND:
        case TOOL_LOWER_LAND:
            lang_text_draw(48, editor_tool_brush_size(), text_offset, 194, FONT_NORMAL_GREEN);
            break;
        default:
            break;
    }

    int people_text;
    int people_font = FONT_NORMAL_RED;
    if (scenario.entry_point.x == -1) {
        if (scenario.exit_point.x == -1) {
            people_text = 60;
        } else {
            people_text = 59;
        }
    } else if (scenario.exit_point.x == -1) {
        people_text = 61;
    } else {
        people_text = 62;
        people_font = FONT_NORMAL_GREEN;
    }
    lang_text_draw(44, people_text, text_offset, 224, people_font);

    if (scenario.entry_point.x != -1 || scenario.exit_point.x != -1) {
        if (scenario.entry_point.x == -1) {
            lang_text_draw(44, 137, text_offset, 239, FONT_NORMAL_RED);
        } else if (scenario.exit_point.x == -1) {
            lang_text_draw(44, 138, text_offset, 239, FONT_NORMAL_RED);
        } else {
            lang_text_draw(44, 67, text_offset, 239, FONT_NORMAL_GREEN);
        }
    }

    int invasion_points = 0;
    for (int i = 0; i < MAX_INVASION_POINTS; i++) {
        if (scenario.invasion_points[i].x != -1) {
            invasion_points++;
        }
    }

    if (invasion_points == 1) {
        lang_text_draw(44, 64, text_offset, 254, FONT_NORMAL_GREEN);
    } else if (invasion_points > 1) {
        int width = text_draw_number(invasion_points, '@', " ", text_offset - 2, 254, FONT_NORMAL_GREEN);
        lang_text_draw(44, 65, text_offset + width - 8, 254, FONT_NORMAL_GREEN);
    } else {
        if (scenario.invasions[0].type) {
            lang_text_draw(44, 63, text_offset, 254, FONT_NORMAL_RED);
        }
    }
}

void widget_sidebar_editor_draw_background(void)
{
    int image_base = image_group(GROUP_EDITOR_SIDE_PANEL);
    int x_offset = sidebar_common_get_x_offset_expanded();
    image_draw(image_base, x_offset, TOP_MENU_HEIGHT);
    draw_buttons();
    widget_minimap_draw(x_offset + 8, MINIMAP_Y_OFFSET, MINIMAP_WIDTH, MINIMAP_HEIGHT, 1);
    draw_status();
    sidebar_common_draw_relief(x_offset, SIDEBAR_FILLER_Y_OFFSET, GROUP_EDITOR_SIDE_PANEL, 0);
}

void widget_sidebar_editor_draw_foreground(void)
{
    draw_buttons();
    widget_minimap_draw(sidebar_common_get_x_offset_expanded() + 8,
        MINIMAP_Y_OFFSET, MINIMAP_WIDTH, MINIMAP_HEIGHT, 0);
}

int widget_sidebar_editor_handle_mouse(const struct mouse_t *m)
{
    if (widget_minimap_handle_mouse(m)) {
        return 1;
    }
    return image_buttons_handle_mouse(m, sidebar_common_get_x_offset_expanded(), 24, buttons_build, sizeof(buttons_build) / sizeof(struct image_button_t), 0);
}

int widget_sidebar_editor_handle_mouse_attributes(const struct mouse_t *m)
{
    return image_buttons_handle_mouse(m, sidebar_common_get_x_offset_expanded(), 24, buttons_build, 1, 0);
}

static void button_attributes(int show, __attribute__((unused)) int param2)
{
    window_editor_build_menu_hide();
    if (show) {
        if (!window_is(WINDOW_EDITOR_ATTRIBUTES)) {
            window_editor_attributes_show();
        }
    } else {
        if (!window_is(WINDOW_EDITOR_MAP)) {
            window_editor_map_show();
        }
    }
}

static void button_empire(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_editor_empire_show();
}

static void button_build_tool(int tool, __attribute__((unused)) int param2)
{
    window_editor_build_menu_hide();
    widget_map_editor_clear_current_tile();
    editor_tool_set_with_id(tool, 0);
    if (window_is(WINDOW_EDITOR_BUILD_MENU)) {
        window_editor_map_show();
    } else {
        window_request_refresh();
    }
}

static void button_build_menu(int submenu, __attribute__((unused)) int param2)
{
    window_editor_build_menu_show(submenu);
}
