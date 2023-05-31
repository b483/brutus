#include "edit_request.h"

#include "city/resource.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "window/editor/attributes.h"
#include "window/editor/map.h"
#include "window/editor/requests.h"
#include "window/numeric_input.h"
#include "window/select_list.h"

static void button_year(int param1, int param2);
static void button_month(int param1, int param2);
static void button_amount(int param1, int param2);
static void button_resource(int param1, int param2);
static void button_deadline_years(int param1, int param2);
static void button_favor(int param1, int param2);
static void button_delete(int param1, int param2);

static struct generic_button_t buttons_edit_request[] = {
    {155, 152, 100, 25, button_year, button_none, 0, 0},
    {155, 182, 100, 25, button_month, button_none, 0, 0},
    {155, 212, 100, 25, button_amount, button_none, 0, 0},
    {155, 242, 100, 25, button_resource, button_none, 0, 0},
    {155, 272, 100, 25, button_deadline_years, button_none, 0, 0},
    {155, 302, 100, 25, button_favor, button_none, 0, 0},
    {105, 342, 200, 25, button_delete, button_none, 0, 0},
};

static struct {
    int id;
    int focus_button_id;
} data;

static uint8_t edit_request_strings[][25] = {
    "Request from the Emperor", // 0
    "Years deadline:", // 1
    "Favor granted:", // 2
};

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 100, 26, 18);
    // Request from the Emperor
    text_draw_centered(edit_request_strings[0], 0, 116, 416, FONT_LARGE_BLACK, COLOR_BLACK);

    // Year offset
    text_draw(common_editor_strings[0], 30, 158, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 152, 100, 25, data.focus_button_id == 1);
    text_draw_number_centered_prefix(scenario.requests[data.id].year, '+', 157, 158, 100, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.requests[data.id].year, 275, 158, FONT_NORMAL_BLACK);

    // Month
    text_draw(common_editor_strings[1], 30, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 182, 100, 25, data.focus_button_id == 2);
    text_draw_number_centered(scenario.requests[data.id].month + 1, 155, 188, 100, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (scenario.requests[data.id].year == 0 && scenario.requests[data.id].month == 0) {
        text_draw(common_editor_strings[2], 260, 188, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    // Amount
    text_draw(common_editor_strings[3], 30, 218, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 212, 100, 25, data.focus_button_id == 3);
    text_draw_number_centered(scenario.requests[data.id].amount, 155, 218, 100, FONT_NORMAL_BLACK);

    // Resource
    text_draw(common_editor_strings[4], 30, 248, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 242, 100, 25, data.focus_button_id == 4);
    lang_text_draw_centered(23, scenario.requests[data.id].resource, 155, 248, 100, FONT_NORMAL_BLACK);

    // Years deadline
    text_draw(edit_request_strings[1], 30, 278, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 272, 100, 25, data.focus_button_id == 5);
    lang_text_draw_amount(8, 8, scenario.requests[data.id].years_deadline, 160, 278, FONT_NORMAL_BLACK);

    // Favor granted
    text_draw(edit_request_strings[2], 30, 308, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(155, 302, 100, 25, data.focus_button_id == 6);
    text_draw_number_centered_prefix(scenario.requests[data.id].favor, '+', 157, 308, 100, FONT_NORMAL_BLACK);

    // Unschedule request
    button_border_draw(105, 342, 200, 25, data.focus_button_id == 7);
    lang_text_draw_centered(44, 25, 105, 348, 200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void scenario_editor_sort_requests(void)
{
    for (int i = 0; i < MAX_REQUESTS; i++) {
        for (int j = MAX_REQUESTS - 1; j > 0; j--) {
            if (scenario.requests[j].resource) {
                // if no previous request scheduled, move current back until first; if previous request is later than current, swap
                if (!scenario.requests[j - 1].resource || scenario.requests[j - 1].year > scenario.requests[j].year
                || (scenario.requests[j - 1].year == scenario.requests[j].year && scenario.requests[j - 1].month > scenario.requests[j].month)) {
                    struct request_t tmp = scenario.requests[j];
                    scenario.requests[j] = scenario.requests[j - 1];
                    scenario.requests[j - 1] = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_edit_request, sizeof(buttons_edit_request) / sizeof(struct generic_button_t), &data.focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        scenario_editor_sort_requests();
        window_editor_requests_show();
    }
}

static void set_year(int value)
{
    scenario.requests[data.id].year = value;
}

static void button_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 140, screen_dialog_offset_y() + 65, 3, 999, set_year);
}

static void set_month(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.requests[data.id].month = value - 1;
}

static void button_month(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 140, screen_dialog_offset_y() + 95, 2, 12, set_month);
}

static void set_amount(int value)
{
    // don't allow 0
    scenario.requests[data.id].amount = value ? value : 1;
}

static void button_amount(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    int max_digits = 3;
    int max_amount = 999;
    if (scenario.requests[data.id].resource == RESOURCE_DENARII) {
        max_digits = 5;
        max_amount = 99999;
    }
    window_numeric_input_show(
        screen_dialog_offset_x() + 140, screen_dialog_offset_y() + 125,
        max_digits, max_amount, set_amount
    );
}

static void set_resource(int value)
{
    scenario.requests[data.id].resource = value;
    if (scenario.requests[data.id].amount > 999 && scenario.requests[data.id].resource != RESOURCE_DENARII) {
        scenario.requests[data.id].amount = 999;
    }
}

static void button_resource(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show(screen_dialog_offset_x() + 255, screen_dialog_offset_y() + 77, 23, 17, set_resource);
}

static void set_deadline_years(int value)
{
    // don't allow 0
    scenario.requests[data.id].years_deadline = value ? value : 1;
    scenario.requests[data.id].months_to_comply = 12 * scenario.requests[data.id].years_deadline;
}

static void button_deadline_years(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 140, screen_dialog_offset_y() + 185, 3, 999, set_deadline_years);
}

static void set_favor(int value)
{
    scenario.requests[data.id].favor = value;
}

static void button_favor(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 140, screen_dialog_offset_y() + 215, 3, 100, set_favor);
}

static void button_delete(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.requests[data.id].year = 1;
    scenario.requests[data.id].month = 0;
    scenario.requests[data.id].amount = 1;
    scenario.requests[data.id].resource = 0;
    scenario.requests[data.id].years_deadline = 5;
    scenario.requests[data.id].months_to_comply = 60;
    scenario.requests[data.id].favor = 8;
    scenario_editor_sort_requests();
    window_editor_requests_show();
}

void window_editor_edit_request_show(int id)
{
    struct window_type_t window = {
        WINDOW_EDITOR_EDIT_REQUEST,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
    };
    data.id = id;
    window_show(&window);
}
