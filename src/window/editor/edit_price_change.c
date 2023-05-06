#include "edit_price_change.h"

#include "graphics/button.h"
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
#include "window/editor/price_changes.h"
#include "window/numeric_input.h"
#include "window/select_list.h"

static void button_year(int param1, int param2);
static void button_month(int param1, int param2);
static void button_amount(int param1, int param2);
static void button_resource(int param1, int param2);
static void button_toggle_rise(int param1, int param2);
static void button_delete(int param1, int param2);

static generic_button buttons_edit_price_change[] = {
    {130, 152, 100, 25, button_year, button_none, 0, 0},
    {130, 182, 100, 25, button_month, button_none, 0, 0},
    {130, 212, 100, 25, button_resource, button_none, 0, 0},
    {250, 212, 100, 25, button_toggle_rise, button_none, 0, 0},
    {355, 212, 100, 25, button_amount, button_none, 0, 0},
    {138, 252, 200, 25, button_delete, button_none, 0, 0},
};

static struct {
    int id;
    int focus_button_id;
} data;

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 100, 29, 12);
    // Price changes
    lang_text_draw_centered(44, 95, 0, 116, 464, FONT_LARGE_BLACK);

    // Year offset
    text_draw(common_editor_strings[0], 30, 158, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 152, 100, 25, data.focus_button_id == 1);
    text_draw_number_centered_prefix(scenario.price_changes[data.id].year, '+', 132, 158, 100, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.price_changes[data.id].year, 240, 158, FONT_NORMAL_BLACK);

    // Month
    text_draw(common_editor_strings[1], 30, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 182, 100, 25, data.focus_button_id == 2);
    text_draw_number_centered(scenario.price_changes[data.id].month + 1, 130, 188, 100, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (scenario.price_changes[data.id].year == 0 && scenario.price_changes[data.id].month == 0) {
        text_draw(common_editor_strings[2], 245, 188, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    // Resource
    text_draw(common_editor_strings[4], 30, 218, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 212, 100, 25, data.focus_button_id == 3);
    lang_text_draw_centered(23, scenario.price_changes[data.id].resource, 130, 218, 100, FONT_NORMAL_BLACK);

    // Rises/falls by
    button_border_draw(235, 212, 100, 25, data.focus_button_id == 4);
    lang_text_draw_centered(44, scenario.price_changes[data.id].is_rise ? 104 : 103, 235, 218, 100, FONT_NORMAL_BLACK);

    // Amount
    button_border_draw(340, 212, 100, 25, data.focus_button_id == 5);
    text_draw_number_centered(scenario.price_changes[data.id].amount, 340, 218, 100, FONT_NORMAL_BLACK);

    // Cancel price change
    button_border_draw(130, 252, 200, 25, data.focus_button_id == 6);
    lang_text_draw_centered(44, 105, 130, 258, 200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void scenario_editor_sort_price_changes(void)
{
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        for (int j = MAX_PRICE_CHANGES - 1; j > 0; j--) {
            if (scenario.price_changes[j].resource) {
                // if no previous price change scheduled, move current back until first; if previous price change is later than current, swap
                if (!scenario.price_changes[j - 1].resource || scenario.price_changes[j - 1].year > scenario.price_changes[j].year
                || (scenario.price_changes[j - 1].year == scenario.price_changes[j].year && scenario.price_changes[j - 1].month > scenario.price_changes[j].month)) {
                    struct price_change_t tmp = scenario.price_changes[j];
                    scenario.price_changes[j] = scenario.price_changes[j - 1];
                    scenario.price_changes[j - 1] = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_edit_price_change, sizeof(buttons_edit_price_change) / sizeof(generic_button), &data.focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        scenario_editor_sort_price_changes();
        window_editor_price_changes_show();
    }
}

static void set_year(int value)
{
    scenario.price_changes[data.id].year = value;
}

static void button_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 65, 3, 999, set_year);
}

static void set_month(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.price_changes[data.id].month = value - 1;
}

static void button_month(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 95, 2, 12, set_month);
}

static void set_resource(int value)
{
    scenario.price_changes[data.id].resource = value;
}

static void button_resource(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show(screen_dialog_offset_x() + 80, screen_dialog_offset_y() + 55, 23, 16, set_resource);
}

static void button_toggle_rise(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.price_changes[data.id].is_rise = !scenario.price_changes[data.id].is_rise;
}

static void set_amount(int value)
{
    // don't allow 0
    scenario.price_changes[data.id].amount = value ? value : 1;
}

static void button_amount(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 330, screen_dialog_offset_y() + 125, 2, 99, set_amount);
}

static void button_delete(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.price_changes[data.id].year = 1;
    scenario.price_changes[data.id].month = 0;
    scenario.price_changes[data.id].resource = 0;
    scenario.price_changes[data.id].is_rise = 0;
    scenario.price_changes[data.id].amount = 1;
    scenario_editor_sort_price_changes();
    window_editor_price_changes_show();
}

void window_editor_edit_price_change_show(int id)
{
    window_type window = {
        WINDOW_EDITOR_EDIT_PRICE_CHANGE,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    data.id = id;
    window_show(&window);
}
