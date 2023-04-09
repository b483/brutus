#include "edit_earthquake.h"

#include "game/custom_strings.h"
#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "window/editor/map.h"
#include "window/editor/earthquakes.h"
#include "window/numeric_input.h"
#include "window/select_list.h"

#include <stdlib.h>

static void button_earthquake_severity(int param1, int param2);
static void button_earthquake_month(int param1, int param2);
static void button_earthquake_year(int param1, int param2);
static void button_earthquake_point(int param1, int param2);
static void button_delete_earthquake(int param1, int param2);


static generic_button buttons_edit_earthquake[] = {
    {130, 152, 100, 25, button_earthquake_year, button_none, 0, 0},
    {130, 182, 100, 25, button_earthquake_month, button_none, 0, 0},
    {130, 212, 100, 25, button_earthquake_severity, button_none, 0, 0},
    {130, 242, 200, 25, button_earthquake_point, button_none, 0, 0},
    {130, 282, 200, 25, button_delete_earthquake, button_none, 0, 0},
};

static struct {
    int id;
    const uint8_t *earthquake_point_names[MAX_EARTHQUAKE_POINTS];
    int focus_button_id;
} data;

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 100, 29, 14);
    // Scheduling an earthquake
    text_draw_centered(get_custom_string(TR_EDITOR_EDIT_EARTHQUAKE_TITLE), 0, 116, 464, FONT_LARGE_BLACK, COLOR_BLACK);

    // Year offset
    text_draw(get_custom_string(TR_EDITOR_OFFSET_YEAR), 30, 158, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 152, 100, 25, data.focus_button_id == 1);
    text_draw_number_centered_prefix(scenario.earthquakes[data.id].year, '+', 132, 158, 100, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.earthquakes[data.id].year, 240, 158, FONT_NORMAL_BLACK);

    // Month
    text_draw(get_custom_string(TR_EDITOR_MONTH), 30, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 182, 100, 25, data.focus_button_id == 2);
    text_draw_number_centered(scenario.earthquakes[data.id].month + 1, 130, 188, 100, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (scenario.earthquakes[data.id].year == 0 && scenario.earthquakes[data.id].month == 0) {
        text_draw(get_custom_string(TR_EDITOR_INVALID_YEAR_MONTH), 245, 188, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    // Severity
    lang_text_draw(38, 1, 30, 218, FONT_NORMAL_BLACK);
    button_border_draw(130, 212, 100, 25, data.focus_button_id == 3);
    lang_text_draw_centered(40, scenario.earthquakes[data.id].severity, 130, 218, 100, FONT_NORMAL_BLACK);

    // Point
    text_draw(get_custom_string(TR_EDITOR_EDIT_EARTHQUAKE_POINT), 30, 248, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 242, 200, 25, data.focus_button_id == 4);
    text_draw_centered(get_custom_string(TR_EDITOR_TOOL_SUBMENU_EARTHQUAKE_POINT_1 + scenario.earthquakes[data.id].point), 130, 248, 200, FONT_NORMAL_BLACK, COLOR_BLACK);

    // Cancel earthquake
    button_border_draw(130, 282, 200, 25, data.focus_button_id == 5);
    text_draw_centered(get_custom_string(TR_EDITOR_EDIT_EARTHQUAKE_CANCEL), 130, 288, 200, FONT_NORMAL_BLACK, COLOR_BLACK);

    graphics_reset_dialog();
}

static void scenario_editor_sort_earthquakes(void)
{
    for (int i = 0; i < MAX_EARTHQUAKES; i++) {
        for (int j = MAX_EARTHQUAKES - 1; j > 0; j--) {
            if (scenario.earthquakes[j].state) {
                // if no previous earthquake scheduled, move current back until first; if previous earthquake is later than current, swap
                if (!scenario.earthquakes[j - 1].state || scenario.earthquakes[j - 1].year > scenario.earthquakes[j].year
                || (scenario.earthquakes[j - 1].year == scenario.earthquakes[j].year && scenario.earthquakes[j - 1].month > scenario.earthquakes[j].month)) {
                    struct earthquake_t tmp = scenario.earthquakes[j];
                    scenario.earthquakes[j] = scenario.earthquakes[j - 1];
                    scenario.earthquakes[j - 1] = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_edit_earthquake, sizeof(buttons_edit_earthquake) / sizeof(generic_button), &data.focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        scenario_editor_sort_earthquakes();
        window_editor_earthquakes_show();
    }
}

static void button_earthquake_severity(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.earthquakes[data.id].severity++;
    switch (scenario.earthquakes[data.id].severity) {
        case 1:
            scenario.earthquakes[data.id].state = 1;
            scenario.earthquakes[data.id].max_duration = 25 + rand() % 32;
            scenario.earthquakes[data.id].max_delay = 10;
            break;
        case 2:
            scenario.earthquakes[data.id].state = 1;
            scenario.earthquakes[data.id].max_duration = 100 + rand() % 64;
            scenario.earthquakes[data.id].max_delay = 8;
            break;
        case 3:
            scenario.earthquakes[data.id].state = 1;
            scenario.earthquakes[data.id].max_duration = 250 + rand() % 128;
            scenario.earthquakes[data.id].max_delay = 6;
            break;
        default:
            scenario.earthquakes[data.id].state = 0;
            scenario.earthquakes[data.id].severity = 0;
            scenario.earthquakes[data.id].max_duration = 0;
            scenario.earthquakes[data.id].max_delay = 0;
            break;
    }
    scenario.is_saved = 0;
    window_request_refresh();
}

static void set_month(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.earthquakes[data.id].month = value - 1;
}

static void button_earthquake_month(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 95, 2, 12, set_month);
}

static void set_year(int value)
{
    scenario.earthquakes[data.id].year = value;
}

static void button_earthquake_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 65, 3, 999, set_year);
}

static void set_earthquake_point(int value)
{
    scenario.earthquakes[data.id].point = value;
}

static void button_earthquake_point(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show_text(screen_dialog_offset_x() + 330, screen_dialog_offset_y() + 165, data.earthquake_point_names, MAX_EARTHQUAKE_POINTS, set_earthquake_point);
}

static void button_delete_earthquake(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.earthquakes[data.id].state = 0;
    scenario.earthquakes[data.id].severity = 0;
    scenario.earthquakes[data.id].month = 0;
    scenario.earthquakes[data.id].year = 1;
    scenario.earthquakes[data.id].duration = 0;
    scenario.earthquakes[data.id].max_duration = 0;
    scenario.earthquakes[data.id].delay = 0;
    scenario.earthquakes[data.id].max_delay = 0;
    scenario.earthquakes[data.id].point = 0;
    for (int i = 0; i < MAX_EARTHQUAKE_BRANCHES; i++) {
        scenario.earthquakes[data.id].branch_coordinates[i].x = -1;
        scenario.earthquakes[data.id].branch_coordinates[i].y = -1;
    }
    scenario_editor_sort_earthquakes();
    window_editor_earthquakes_show();
}

void window_editor_edit_earthquake_show(int id)
{
    window_type window = {
        WINDOW_EDITOR_EDIT_EARTHQUAKE,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    data.id = id;
    for (int i = TR_EDITOR_TOOL_SUBMENU_EARTHQUAKE_POINT_1; i <= TR_EDITOR_TOOL_SUBMENU_EARTHQUAKE_POINT_8; i++) {
        data.earthquake_point_names[i - TR_EDITOR_TOOL_SUBMENU_EARTHQUAKE_POINT_1] = get_custom_string(i);
    }
    window_show(&window);
}
