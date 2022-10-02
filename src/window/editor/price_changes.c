#include "price_changes.h"

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
#include "window/editor/edit_price_change.h"
#include "window/editor/map.h"

static void button_price_change(int id, int param2);

static generic_button buttons[] = {
    {20, 48, 290, 25, button_price_change, button_none, 0, 0},
    {20, 78, 290, 25, button_price_change, button_none, 1, 0},
    {20, 108, 290, 25, button_price_change, button_none, 2, 0},
    {20, 138, 290, 25, button_price_change, button_none, 3, 0},
    {20, 168, 290, 25, button_price_change, button_none, 4, 0},
    {20, 198, 290, 25, button_price_change, button_none, 5, 0},
    {20, 228, 290, 25, button_price_change, button_none, 6, 0},
    {20, 258, 290, 25, button_price_change, button_none, 7, 0},
    {20, 288, 290, 25, button_price_change, button_none, 8, 0},
    {20, 318, 290, 25, button_price_change, button_none, 9, 0},
    {320, 48, 290, 25, button_price_change, button_none, 10, 0},
    {320, 78, 290, 25, button_price_change, button_none, 11, 0},
    {320, 108, 290, 25, button_price_change, button_none, 12, 0},
    {320, 138, 290, 25, button_price_change, button_none, 13, 0},
    {320, 168, 290, 25, button_price_change, button_none, 14, 0},
    {320, 198, 290, 25, button_price_change, button_none, 15, 0},
    {320, 228, 290, 25, button_price_change, button_none, 16, 0},
    {320, 258, 290, 25, button_price_change, button_none, 17, 0},
    {320, 288, 290, 25, button_price_change, button_none, 18, 0},
    {320, 318, 290, 25, button_price_change, button_none, 19, 0},
};

static int focus_button_id;

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 40, 28);
    lang_text_draw_centered(44, 95, 0, 16, 640, FONT_LARGE_BLACK);

    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        int x, y;
        if (i < 10) {
            x = 20;
            y = 48 + 30 * i;
        } else {
            x = 320;
            y = 48 + 30 * (i - 10);
        }
        button_border_draw(x, y, 290, 25, focus_button_id == i + 1);
        editor_price_change price_change;
        scenario_editor_price_change_get(i, &price_change);
        if (price_change.year) {
            text_draw_number(price_change.year, '+', " ", x + 10, y + 6, FONT_NORMAL_BLACK);
            lang_text_draw_year(scenario_property_start_year() + price_change.year, x + 65, y + 6, FONT_NORMAL_BLACK);
            int offset = price_change.resource + resource_image_offset(price_change.resource, RESOURCE_IMAGE_ICON);
            image_draw(image_group(GROUP_EDITOR_RESOURCE_ICONS) + offset, x + 140, y + 3);
            int width = lang_text_draw(44, price_change.is_rise ? 104 : 103, x + 170, y + 6, FONT_NORMAL_BLACK);
            text_draw_number(price_change.amount, '@', " ", x + 170 + width, y + 6, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw_centered(44, 102, x, y + 6, 290, FONT_NORMAL_BLACK);
        }
    }

    // price changes hint
    lang_text_draw_multiline(152, 3, 32, 370, 576, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons, 20, &focus_button_id)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        window_editor_attributes_show();
    }
}

static void button_price_change(int id, __attribute__((unused)) int param2)
{
    window_editor_edit_price_change_show(id);
}

void window_editor_price_changes_show(void)
{
    window_type window = {
        WINDOW_EDITOR_PRICE_CHANGES,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    window_show(&window);
}
