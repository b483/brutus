#include "special_events.h"

#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "scenario/editor_events.h"
#include "scenario/property.h"
#include "window/editor/attributes.h"
#include "window/editor/map.h"
#include "window/numeric_input.h"

static void button_earthquake_severity(int param1, int param2);
static void button_earthquake_year(int param1, int param2);
static void button_gladiator_toggle(int param1, int param2);
static void button_gladiator_year(int param1, int param2);
static void button_emperor_toggle(int param1, int param2);
static void button_emperor_year(int param1, int param2);
static void button_sea_trade_toggle(int param1, int param2);
static void button_land_trade_toggle(int param1, int param2);
static void button_raise_wages_toggle(int param1, int param2);
static void button_lower_wages_toggle(int param1, int param2);
static void button_contamination_toggle(int param1, int param2);

static generic_button buttons[] = {
    {200, 74, 100, 24, button_earthquake_severity, button_none, 0, 0},
    {310, 74, 150, 24, button_earthquake_year, button_none, 0, 0},
    {200, 104, 100, 24, button_gladiator_toggle,button_none, 0, 0},
    {310, 104, 150, 24, button_gladiator_year,button_none, 0, 0},
    {200, 134, 100, 24, button_emperor_toggle,button_none, 0, 0},
    {310, 134, 150, 24, button_emperor_year, button_none, 0, 0},
    {200, 164, 100, 24, button_sea_trade_toggle, button_none, 0, 0},
    {200, 194, 100, 24, button_land_trade_toggle, button_none, 0, 0},
    {200, 224, 100, 24, button_raise_wages_toggle, button_none, 0, 0},
    {200, 254, 100, 24, button_lower_wages_toggle, button_none, 0, 0},
    {200, 284, 100, 24, button_contamination_toggle, button_none, 0, 0},
};

static int focus_button_id;

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 30, 21);

    lang_text_draw_centered(38, 0, 0, 16, 480, FONT_LARGE_BLACK);

    // table header
    lang_text_draw(38, 11, 220, 60, FONT_SMALL_PLAIN);
    lang_text_draw(38, 12, 330, 60, FONT_SMALL_PLAIN);

    // Earthquake
    lang_text_draw(38, 1, 20, 80, FONT_NORMAL_BLACK);
    button_border_draw(200, 74, 100, 24, focus_button_id == 1);
    lang_text_draw_centered(40, scenario_editor_earthquake_severity(), 200, 80, 100, FONT_NORMAL_BLACK);

    button_border_draw(310, 74, 150, 24, focus_button_id == 2);
    int width = text_draw_number(scenario_editor_earthquake_year(), '+', " ", 330, 80, FONT_NORMAL_BLACK);
    lang_text_draw_year(
        scenario_property_start_year() + scenario_editor_earthquake_year(),
        340 + width, 80, FONT_NORMAL_BLACK);

    // Gladiator revolt
    lang_text_draw(38, 2, 20, 110, FONT_NORMAL_BLACK);
    button_border_draw(200, 104, 100, 24, focus_button_id == 3);
    lang_text_draw_centered(18, scenario_editor_gladiator_revolt_enabled(), 200, 110, 100, FONT_NORMAL_BLACK);

    button_border_draw(310, 104, 150, 24, focus_button_id == 4);
    width = text_draw_number(scenario_editor_gladiator_revolt_year(), '+', " ", 330, 110, FONT_NORMAL_BLACK);
    lang_text_draw_year(
        scenario_property_start_year() + scenario_editor_gladiator_revolt_year(),
        340 + width, 110, FONT_NORMAL_BLACK);

    // Change of Emperor
    lang_text_draw(38, 3, 20, 140, FONT_NORMAL_BLACK);
    button_border_draw(200, 134, 100, 24, focus_button_id == 5);
    lang_text_draw_centered(18, scenario_editor_emperor_change_enabled(), 200, 140, 100, FONT_NORMAL_BLACK);

    button_border_draw(310, 134, 150, 24, focus_button_id == 6);
    width = text_draw_number(scenario_editor_emperor_change_year(), '+', " ", 330, 140, FONT_NORMAL_BLACK);
    lang_text_draw_year(
        scenario_property_start_year() + scenario_editor_emperor_change_year(),
        340 + width, 140, FONT_NORMAL_BLACK);

    // random events
    // Sea trade problem
    lang_text_draw(38, 4, 20, 170, FONT_NORMAL_BLACK);
    button_border_draw(200, 164, 100, 24, focus_button_id == 7);
    lang_text_draw_centered(18, scenario_editor_sea_trade_problem_enabled(), 200, 170, 100, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 172, FONT_SMALL_PLAIN);

    // Land trade problem
    lang_text_draw(38, 5, 20, 200, FONT_NORMAL_BLACK);
    button_border_draw(200, 194, 100, 24, focus_button_id == 8);
    lang_text_draw_centered(18, scenario_editor_land_trade_problem_enabled(), 200, 200, 100, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 202, FONT_SMALL_PLAIN);

    // Rome raises wages
    lang_text_draw(38, 6, 20, 230, FONT_NORMAL_BLACK);
    button_border_draw(200, 224, 100, 24, focus_button_id == 9);
    lang_text_draw_centered(18, scenario_editor_raise_wages_enabled(), 200, 230, 100, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 232, FONT_SMALL_PLAIN);

    // Rome lowers wages
    lang_text_draw(38, 7, 20, 260, FONT_NORMAL_BLACK);
    button_border_draw(200, 254, 100, 24, focus_button_id == 10);
    lang_text_draw_centered(18, scenario_editor_lower_wages_enabled(), 200, 260, 100, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 262, FONT_SMALL_PLAIN);

    // Contaminated water
    lang_text_draw(38, 8, 20, 290, FONT_NORMAL_BLACK);
    button_border_draw(200, 284, 100, 24, focus_button_id == 11);
    lang_text_draw_centered(18, scenario_editor_contaminated_water_enabled(), 200, 290, 100, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 292, FONT_SMALL_PLAIN);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons, 11, &focus_button_id)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        window_editor_attributes_show();
    }
}

static void button_earthquake_severity(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_earthquake_cycle_severity();
    window_request_refresh();
}

static void button_earthquake_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 190, screen_dialog_offset_y() + 100,
                              3, 999, scenario_editor_earthquake_set_year);
}

static void button_gladiator_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_gladiator_revolt_toggle_enabled();
    window_request_refresh();
}

static void button_gladiator_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 190, screen_dialog_offset_y() + 100,
                              3, 999, scenario_editor_gladiator_revolt_set_year);
}

static void button_emperor_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_emperor_change_toggle_enabled();
    window_request_refresh();
}

static void button_emperor_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 190, screen_dialog_offset_y() + 100,
                              3, 999, scenario_editor_emperor_change_set_year);
}

static void button_sea_trade_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_sea_trade_problem_toggle_enabled();
    window_request_refresh();
}

static void button_land_trade_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_land_trade_problem_toggle_enabled();
    window_request_refresh();
}

static void button_raise_wages_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_raise_wages_toggle_enabled();
    window_request_refresh();
}

static void button_lower_wages_toggle(__attribute__((unused)) int param1, __attribute__((unused))  int param2)
{
    scenario_editor_lower_wages_toggle_enabled();
    window_request_refresh();
}

static void button_contamination_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_contaminated_water_toggle_enabled();
    window_request_refresh();
}

void window_editor_special_events_show(void)
{
    window_type window = {
        WINDOW_EDITOR_SPECIAL_EVENTS,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    window_show(&window);
}
