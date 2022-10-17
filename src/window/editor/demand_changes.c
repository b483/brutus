#include "demand_changes.h"

#include "core/image_group_editor.h"
#include "game/resource.h"
#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "scenario/data.h"
#include "scenario/editor.h"
#include "scenario/property.h"
#include "window/editor/attributes.h"
#include "window/editor/edit_demand_change.h"
#include "window/editor/map.h"

static void button_demand_change(int id, int param2);

static generic_button buttons_demand_changes[] = {
    {20, 48, 290, 25, button_demand_change, button_none, 0, 0},
    {20, 78, 290, 25, button_demand_change, button_none, 1, 0},
    {20, 108, 290, 25, button_demand_change, button_none, 2, 0},
    {20, 138, 290, 25, button_demand_change, button_none, 3, 0},
    {20, 168, 290, 25, button_demand_change, button_none, 4, 0},
    {20, 198, 290, 25, button_demand_change, button_none, 5, 0},
    {20, 228, 290, 25, button_demand_change, button_none, 6, 0},
    {20, 258, 290, 25, button_demand_change, button_none, 7, 0},
    {20, 288, 290, 25, button_demand_change, button_none, 8, 0},
    {20, 318, 290, 25, button_demand_change, button_none, 9, 0},
    {320, 48, 290, 25, button_demand_change, button_none, 10, 0},
    {320, 78, 290, 25, button_demand_change, button_none, 11, 0},
    {320, 108, 290, 25, button_demand_change, button_none, 12, 0},
    {320, 138, 290, 25, button_demand_change, button_none, 13, 0},
    {320, 168, 290, 25, button_demand_change, button_none, 14, 0},
    {320, 198, 290, 25, button_demand_change, button_none, 15, 0},
    {320, 228, 290, 25, button_demand_change, button_none, 16, 0},
    {320, 258, 290, 25, button_demand_change, button_none, 17, 0},
    {320, 288, 290, 25, button_demand_change, button_none, 18, 0},
    {320, 318, 290, 25, button_demand_change, button_none, 19, 0},
};

static int focus_button_id;

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 40, 23);
    lang_text_draw_centered(44, 94, 0, 16, 640, FONT_LARGE_BLACK);

    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        int x, y;
        if (i < 10) {
            x = 20;
            y = 48 + 30 * i;
        } else {
            x = 320;
            y = 48 + 30 * (i - 10);
        }
        button_border_draw(x, y, 290, 25, focus_button_id == i + 1);

        if (scenario.demand_changes[i].resource && scenario.demand_changes[i].route_id) {
            int width = lang_text_draw(25, scenario.demand_changes[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw_year(scenario_property_start_year() + scenario.demand_changes[i].year, x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            image_draw(image_group(GROUP_EDITOR_RESOURCE_ICONS) + scenario.demand_changes[i].resource + resource_image_offset(scenario.demand_changes[i].resource, RESOURCE_IMAGE_ICON), x + 12 + width, y + 3);
            width += lang_text_draw(44, 97, x + 45 + width, y + 6, FONT_NORMAL_BLACK);
            width += text_draw_number(scenario.demand_changes[i].route_id, '@', " ", x + 40 + width, y + 6, FONT_NORMAL_BLACK);
            lang_text_draw(44, scenario.demand_changes[i].is_rise ? 99 : 98, x + 40 + width, y + 6, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw_centered(44, 96, x, y + 6, 290, FONT_NORMAL_BLACK);
        }
    }

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_demand_changes, sizeof(buttons_demand_changes) / sizeof(generic_button), &focus_button_id)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        window_editor_attributes_show();
    }
}

static void button_demand_change(int id, __attribute__((unused)) int param2)
{
    window_editor_edit_demand_change_show(id);
}

void window_editor_demand_changes_show(void)
{
    window_type window = {
        WINDOW_EDITOR_DEMAND_CHANGES,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    window_show(&window);
}
