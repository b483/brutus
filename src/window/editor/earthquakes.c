#include "earthquakes.h"

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
#include "window/editor/edit_earthquake.h"
#include "window/editor/map.h"

static void button_schedule_earthquake(int id, int param2);

static struct generic_button_t buttons_earthquake[] = {
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

char *earthquakes_strings[] = {
    "Earthquakes", // 0
    "Free earthquake slot", // 1
    "Earthquake point 1", // 2
    "Earthquake point 2", // 3
    "Earthquake point 3", // 4
    "Earthquake point 4", // 5
    "Earthquake point 5", // 6
    "Earthquake point 6", // 7
    "Earthquake point 7", // 8
    "Earthquake point 8", // 9
};

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 25, 25);
    text_draw_centered(earthquakes_strings[0], 0, 16, 400, FONT_LARGE_BLACK, COLOR_BLACK);

    for (int i = 0; i < MAX_EARTHQUAKES; i++) {
        int x, y;
        x = 24;
        y = 64 + 30 * i;
        button_border_draw(x, y, 350, 25, focus_button_id == i + 1);

        if (scenario.earthquakes[i].state) {
            int width = lang_text_draw(25, scenario.earthquakes[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw_year(scenario.start_year + scenario.earthquakes[i].year, x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw(40, scenario.earthquakes[i].severity, x + 15 + width, y + 6, FONT_NORMAL_BLACK);
            text_draw(earthquakes_strings[scenario.earthquakes[i].point + 2], x + 15 + width, y + 6, FONT_NORMAL_BLACK, COLOR_BLACK);
        } else {
            text_draw_centered(earthquakes_strings[1], x, y + 6, 350, FONT_NORMAL_BLACK, COLOR_BLACK);
        }
    }

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_earthquake, sizeof(buttons_earthquake) / sizeof(struct generic_button_t), &focus_button_id)) {
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
    struct window_type_t window = {
        WINDOW_EDITOR_EARTHQUAKES,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
    };
    window_show(&window);
}
