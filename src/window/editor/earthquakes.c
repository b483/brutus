#include "earthquakes.h"

#include "core/image_group_editor.h"
#include "game/custom_strings.h"
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
#include "window/editor/attributes.h"
#include "window/editor/edit_earthquake.h"
#include "window/editor/map.h"

static void button_schedule_earthquake(int id, int param2);

static generic_button buttons_earthquake[] = {
    {24, 64, 350, 25, button_schedule_earthquake, button_none, 0, 0},
    {24, 94, 350, 25, button_schedule_earthquake, button_none, 1, 0},
    {24, 124, 350, 25, button_schedule_earthquake, button_none, 2, 0},
    {24, 154, 350, 25, button_schedule_earthquake, button_none, 3, 0},
    {24, 184, 350, 25, button_schedule_earthquake, button_none, 4, 0},
    {24, 214, 350, 25, button_schedule_earthquake, button_none, 5, 0},
    {24, 244, 350, 25, button_schedule_earthquake, button_none, 6, 0},
    {24, 274, 350, 25, button_schedule_earthquake, button_none, 7, 0},
    {24, 304, 350, 25, button_schedule_earthquake, button_none, 8, 0},
    {24, 334, 350, 25, button_schedule_earthquake, button_none, 9, 0},
};

static int focus_button_id;

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 25, 25);
    text_draw_centered(get_custom_string(TR_EDITOR_EARTHQUAKES_TITLE), 0, 16, 400, FONT_LARGE_BLACK, COLOR_BLACK);

    for (int i = 0; i < MAX_EARTHQUAKES; i++) {
        int x, y;
        x = 24;
        y = 64 + 30 * i;
        button_border_draw(x, y, 350, 25, focus_button_id == i + 1);

        if (scenario.earthquakes[i].state) {
            int width = lang_text_draw(25, scenario.earthquakes[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw_year(scenario.start_year + scenario.earthquakes[i].year, x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw(40, scenario.earthquakes[i].severity, x + 15 + width, y + 6, FONT_NORMAL_BLACK);
            text_draw(get_custom_string(TR_EDITOR_TOOL_SUBMENU_EARTHQUAKE_POINT_1 + scenario.earthquakes[i].point), x + 15 + width, y + 6, FONT_NORMAL_BLACK, COLOR_BLACK);
        } else {
            text_draw_centered(get_custom_string(TR_EDITOR_EARTHQUAKES_FREE_SLOT), x, y + 6, 350, FONT_NORMAL_BLACK, COLOR_BLACK);
        }
    }

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_earthquake, sizeof(buttons_earthquake) / sizeof(generic_button), &focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_editor_attributes_show();
    }
}

static void button_schedule_earthquake(int id, __attribute__((unused)) int param2)
{
    window_editor_edit_earthquake_show(id);
}

void window_editor_earthquakes_show(void)
{
    window_type window = {
        WINDOW_EDITOR_EARTHQUAKES,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
        0
    };
    window_show(&window);
}
