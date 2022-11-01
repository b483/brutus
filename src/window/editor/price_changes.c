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
#include "scenario/data.h"
#include "scenario/editor.h"
#include "window/editor/attributes.h"
#include "window/editor/edit_price_change.h"
#include "window/editor/map.h"

static void button_price_change(int id, int param2);

static generic_button buttons_price_changes[] = {
    {-300, 48, 290, 25, button_price_change, button_none, 0, 0},
    {-300, 78, 290, 25, button_price_change, button_none, 1, 0},
    {-300, 108, 290, 25, button_price_change, button_none, 2, 0},
    {-300, 138, 290, 25, button_price_change, button_none, 3, 0},
    {-300, 168, 290, 25, button_price_change, button_none, 4, 0},
    {-300, 198, 290, 25, button_price_change, button_none, 5, 0},
    {-300, 228, 290, 25, button_price_change, button_none, 6, 0},
    {-300, 258, 290, 25, button_price_change, button_none, 7, 0},
    {-300, 288, 290, 25, button_price_change, button_none, 8, 0},
    {-300, 318, 290, 25, button_price_change, button_none, 9, 0},
    {0, 48, 290, 25, button_price_change, button_none, 10, 0},
    {0, 78, 290, 25, button_price_change, button_none, 11, 0},
    {0, 108, 290, 25, button_price_change, button_none, 12, 0},
    {0, 138, 290, 25, button_price_change, button_none, 13, 0},
    {0, 168, 290, 25, button_price_change, button_none, 14, 0},
    {0, 198, 290, 25, button_price_change, button_none, 15, 0},
    {0, 228, 290, 25, button_price_change, button_none, 16, 0},
    {0, 258, 290, 25, button_price_change, button_none, 17, 0},
    {0, 288, 290, 25, button_price_change, button_none, 18, 0},
    {0, 318, 290, 25, button_price_change, button_none, 19, 0},
    {300, 48, 290, 25, button_price_change, button_none, 20, 0},
    {300, 78, 290, 25, button_price_change, button_none, 21, 0},
    {300, 108, 290, 25, button_price_change, button_none, 22, 0},
    {300, 138, 290, 25, button_price_change, button_none, 23, 0},
    {300, 168, 290, 25, button_price_change, button_none, 24, 0},
    {300, 198, 290, 25, button_price_change, button_none, 25, 0},
    {300, 228, 290, 25, button_price_change, button_none, 26, 0},
    {300, 258, 290, 25, button_price_change, button_none, 27, 0},
    {300, 288, 290, 25, button_price_change, button_none, 28, 0},
    {300, 318, 290, 25, button_price_change, button_none, 29, 0},
    {600, 48, 290, 25, button_price_change, button_none, 30, 0},
    {600, 78, 290, 25, button_price_change, button_none, 31, 0},
    {600, 108, 290, 25, button_price_change, button_none, 32, 0},
    {600, 138, 290, 25, button_price_change, button_none, 33, 0},
    {600, 168, 290, 25, button_price_change, button_none, 34, 0},
    {600, 198, 290, 25, button_price_change, button_none, 35, 0},
    {600, 228, 290, 25, button_price_change, button_none, 36, 0},
    {600, 258, 290, 25, button_price_change, button_none, 37, 0},
    {600, 288, 290, 25, button_price_change, button_none, 38, 0},
    {600, 318, 290, 25, button_price_change, button_none, 39, 0},
};

static int focus_button_id;

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(-320, 0, 77, 26);
    lang_text_draw_centered(44, 95, -320, 16, 1232, FONT_LARGE_BLACK);

    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        int x, y;
        if (i < 10) {
            x = -300;
            y = 48 + 30 * i;
        } else if (i < 20) {
            x = 0;
            y = 48 + 30 * (i - 10);
        } else if (i < 30) {
            x = 300;
            y = 48 + 30 * (i - 20);
        } else {
            x = 600;
            y = 48 + 30 * (i - 30);
        }
        button_border_draw(x, y, 290, 25, focus_button_id == i + 1);

        if (scenario.price_changes[i].resource) {
            int width = lang_text_draw(25, scenario.price_changes[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw_year(scenario.start_year + scenario.price_changes[i].year, x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            image_draw(image_group(GROUP_EDITOR_RESOURCE_ICONS) + scenario.price_changes[i].resource + resource_image_offset(scenario.price_changes[i].resource, RESOURCE_IMAGE_ICON), x + 12 + width, y + 3);
            width += lang_text_draw(44, scenario.price_changes[i].is_rise ? 104 : 103, x + 45 + width, y + 6, FONT_NORMAL_BLACK);
            text_draw_number(scenario.price_changes[i].amount, 0, 0, x + 45 + width, y + 6, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw_centered(44, 102, x, y + 6, 290, FONT_NORMAL_BLACK);
        }
    }

    // price changes hint
    lang_text_draw_multiline(152, 3, -280, 360, 1200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_price_changes, sizeof(buttons_price_changes) / sizeof(generic_button), &focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
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
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
        0
    };
    window_show(&window);
}
