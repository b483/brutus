#include "demand_changes.h"

#include "core/image_group_editor.h"
#include "city/resource.h"
#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "window/editor/attributes.h"
#include "window/editor/edit_demand_change.h"
#include "window/editor/map.h"

static void button_demand_change(int id, int param2);

static struct generic_button_t buttons_demand_changes[] = {
    {-300, 48, 290, 25, button_demand_change, button_none, 0, 0},
    {-300, 78, 290, 25, button_demand_change, button_none, 1, 0},
    {-300, 108, 290, 25, button_demand_change, button_none, 2, 0},
    {-300, 138, 290, 25, button_demand_change, button_none, 3, 0},
    {-300, 168, 290, 25, button_demand_change, button_none, 4, 0},
    {-300, 198, 290, 25, button_demand_change, button_none, 5, 0},
    {-300, 228, 290, 25, button_demand_change, button_none, 6, 0},
    {-300, 258, 290, 25, button_demand_change, button_none, 7, 0},
    {-300, 288, 290, 25, button_demand_change, button_none, 8, 0},
    {-300, 318, 290, 25, button_demand_change, button_none, 9, 0},
    {0, 48, 290, 25, button_demand_change, button_none, 10, 0},
    {0, 78, 290, 25, button_demand_change, button_none, 11, 0},
    {0, 108, 290, 25, button_demand_change, button_none, 12, 0},
    {0, 138, 290, 25, button_demand_change, button_none, 13, 0},
    {0, 168, 290, 25, button_demand_change, button_none, 14, 0},
    {0, 198, 290, 25, button_demand_change, button_none, 15, 0},
    {0, 228, 290, 25, button_demand_change, button_none, 16, 0},
    {0, 258, 290, 25, button_demand_change, button_none, 17, 0},
    {0, 288, 290, 25, button_demand_change, button_none, 18, 0},
    {0, 318, 290, 25, button_demand_change, button_none, 19, 0},
    {300, 48, 290, 25, button_demand_change, button_none, 20, 0},
    {300, 78, 290, 25, button_demand_change, button_none, 21, 0},
    {300, 108, 290, 25, button_demand_change, button_none, 22, 0},
    {300, 138, 290, 25, button_demand_change, button_none, 23, 0},
    {300, 168, 290, 25, button_demand_change, button_none, 24, 0},
    {300, 198, 290, 25, button_demand_change, button_none, 25, 0},
    {300, 228, 290, 25, button_demand_change, button_none, 26, 0},
    {300, 258, 290, 25, button_demand_change, button_none, 27, 0},
    {300, 288, 290, 25, button_demand_change, button_none, 28, 0},
    {300, 318, 290, 25, button_demand_change, button_none, 29, 0},
    {600, 48, 290, 25, button_demand_change, button_none, 30, 0},
    {600, 78, 290, 25, button_demand_change, button_none, 31, 0},
    {600, 108, 290, 25, button_demand_change, button_none, 32, 0},
    {600, 138, 290, 25, button_demand_change, button_none, 33, 0},
    {600, 168, 290, 25, button_demand_change, button_none, 34, 0},
    {600, 198, 290, 25, button_demand_change, button_none, 35, 0},
    {600, 228, 290, 25, button_demand_change, button_none, 36, 0},
    {600, 258, 290, 25, button_demand_change, button_none, 37, 0},
    {600, 288, 290, 25, button_demand_change, button_none, 38, 0},
    {600, 318, 290, 25, button_demand_change, button_none, 39, 0},
};

static int focus_button_id;

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(-320, 0, 77, 23);
    lang_text_draw_centered(44, 94, -320, 16, 1232, FONT_LARGE_BLACK);

    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
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

        if (scenario.demand_changes[i].resource && scenario.demand_changes[i].trade_city_id) {
            int width = lang_text_draw(25, scenario.demand_changes[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw_year(scenario.start_year + scenario.demand_changes[i].year, x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            image_draw(resource_images[scenario.demand_changes[i].resource].editor_icon_img_id + resource_image_offset(scenario.demand_changes[i].resource, RESOURCE_IMAGE_ICON), x + 12 + width, y + 3);
            width += lang_text_draw(44, 97, x + 45 + width, y + 6, FONT_NORMAL_BLACK);
            width += text_draw_number(scenario.demand_changes[i].trade_city_id, 0, 0, x + 48 + width, y + 6, FONT_NORMAL_BLACK);
            lang_text_draw(44, scenario.demand_changes[i].is_rise ? 99 : 98, x + 48 + width, y + 6, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw_centered(44, 96, x, y + 6, 290, FONT_NORMAL_BLACK);
        }
    }

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_demand_changes, sizeof(buttons_demand_changes) / sizeof(struct generic_button_t), &focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_editor_attributes_show();
    }
}

static void button_demand_change(int id, __attribute__((unused)) int param2)
{
    window_editor_edit_demand_change_show(id);
}

void window_editor_demand_changes_show(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_DEMAND_CHANGES,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
    };
    window_show(&window);
}
