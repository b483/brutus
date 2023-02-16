#include "special_events.h"

#include "core/random.h"
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
#include "scenario/editor_events.h"
#include "window/editor/attributes.h"
#include "window/editor/map.h"
#include "window/numeric_input.h"

static void button_earthquake_severity(int param1, int param2);
static void button_earthquake_month(int param1, int param2);
static void button_earthquake_year(int param1, int param2);
static void button_gladiator_toggle(int param1, int param2);
static void button_gladiator_month(int param1, int param2);
static void button_gladiator_year(int param1, int param2);
static void button_sea_trade_toggle(int param1, int param2);
static void button_land_trade_toggle(int param1, int param2);
static void button_raise_wages_toggle(int param1, int param2);
static void button_lower_wages_toggle(int param1, int param2);
static void button_contamination_toggle(int param1, int param2);

static generic_button buttons_special_events[] = {
    {200, 74, 75, 24, button_earthquake_severity, button_none, 0, 0},
    {280, 74, 45, 24, button_earthquake_month,button_none, 0, 0},
    {330, 74, 150, 24, button_earthquake_year, button_none, 0, 0},
    {200, 104, 75, 24, button_gladiator_toggle,button_none, 0, 0},
    {280, 104, 45, 24, button_gladiator_month,button_none, 0, 0},
    {330, 104, 150, 24, button_gladiator_year,button_none, 0, 0},
    {200, 134, 75, 24, button_sea_trade_toggle, button_none, 0, 0},
    {200, 164, 75, 24, button_land_trade_toggle, button_none, 0, 0},
    {200, 194, 75, 24, button_raise_wages_toggle, button_none, 0, 0},
    {200, 224, 75, 24, button_lower_wages_toggle, button_none, 0, 0},
    {200, 254, 75, 24, button_contamination_toggle, button_none, 0, 0},
};

static int focus_button_id;

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 32, 19);

    lang_text_draw_centered(38, 0, 0, 16, 480, FONT_LARGE_BLACK);

    // table header
    lang_text_draw(38, 11, 220, 60, FONT_SMALL_PLAIN);
    lang_text_draw(38, 12, 330, 60, FONT_SMALL_PLAIN);

    // Earthquake
    lang_text_draw(38, 1, 20, 80, FONT_NORMAL_BLACK);
    button_border_draw(200, 74, 75, 24, focus_button_id == 1);
    lang_text_draw_centered(40, scenario.earthquake.severity, 200, 80, 75, FONT_NORMAL_BLACK);

    button_border_draw(280, 74, 45, 24, focus_button_id == 2);
    lang_text_draw(25, scenario.earthquake.month, 288, 80, FONT_NORMAL_BLACK);

    button_border_draw(330, 74, 150, 24, focus_button_id == 3);
    int width = text_draw_number(scenario.earthquake.year, '+', 0, 346, 80, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.earthquake.year, width + 346, 80, FONT_NORMAL_BLACK);

    // Gladiator revolt
    lang_text_draw(38, 2, 20, 110, FONT_NORMAL_BLACK);
    button_border_draw(200, 104, 75, 24, focus_button_id == 4);
    lang_text_draw_centered(18, scenario.gladiator_revolt.state, 200, 110, 75, FONT_NORMAL_BLACK);

    button_border_draw(280, 104, 45, 24, focus_button_id == 5);
    lang_text_draw(25, scenario.gladiator_revolt.month, 288, 110, FONT_NORMAL_BLACK);

    button_border_draw(330, 104, 150, 24, focus_button_id == 6);
    width = text_draw_number(scenario.gladiator_revolt.year, '+', 0, 346, 110, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.gladiator_revolt.year, width + 346, 110, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if ((scenario.earthquake.year == 0 && scenario.earthquake.month == 0) || (scenario.gladiator_revolt.year == 0 && scenario.gladiator_revolt.month == 0)) {
        text_draw(get_custom_string(TR_EDITOR_INVALID_YEAR_MONTH), 346, 24, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    // random events
    // Sea trade problem
    lang_text_draw(38, 4, 20, 140, FONT_NORMAL_BLACK);
    button_border_draw(200, 134, 75, 24, focus_button_id == 7);
    lang_text_draw_centered(18, scenario.random_events.sea_trade_problem, 200, 140, 75, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 140, FONT_SMALL_PLAIN);

    // Land trade problem
    lang_text_draw(38, 5, 20, 170, FONT_NORMAL_BLACK);
    button_border_draw(200, 164, 75, 24, focus_button_id == 8);
    lang_text_draw_centered(18, scenario.random_events.land_trade_problem, 200, 170, 75, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 170, FONT_SMALL_PLAIN);

    // Rome raises wages
    lang_text_draw(38, 6, 20, 200, FONT_NORMAL_BLACK);
    button_border_draw(200, 194, 75, 24, focus_button_id == 9);
    lang_text_draw_centered(18, scenario.random_events.raise_wages, 200, 200, 75, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 200, FONT_SMALL_PLAIN);

    // Rome lowers wages
    lang_text_draw(38, 7, 20, 230, FONT_NORMAL_BLACK);
    button_border_draw(200, 224, 75, 24, focus_button_id == 10);
    lang_text_draw_centered(18, scenario.random_events.lower_wages, 200, 230, 75, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 230, FONT_SMALL_PLAIN);

    // Contaminated water
    lang_text_draw(38, 8, 20, 260, FONT_NORMAL_BLACK);
    button_border_draw(200, 254, 75, 24, focus_button_id == 11);
    lang_text_draw_centered(18, scenario.random_events.contaminated_water, 200, 260, 75, FONT_NORMAL_BLACK);
    lang_text_draw(38, 13, 330, 260, FONT_SMALL_PLAIN);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_special_events, sizeof(buttons_special_events) / sizeof(generic_button), &focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_editor_attributes_show();
    }
}

static void button_earthquake_severity(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.earthquake.severity++;
    switch (scenario.earthquake.severity) {
        case 1:
            scenario.earthquake.state = 1;
            scenario.earthquake.max_duration = 25 + (random_byte() & 0x1f);
            scenario.earthquake.max_delay = 10;
            break;
        case 2:
            scenario.earthquake.state = 1;
            scenario.earthquake.max_duration = 100 + (random_byte() & 0x3f);
            scenario.earthquake.max_delay = 8;
            break;
        case 3:
            scenario.earthquake.state = 1;
            scenario.earthquake.max_duration = 250 + random_byte();
            scenario.earthquake.max_delay = 6;
            break;
        default:
            scenario.earthquake.state = 0;
            scenario.earthquake.severity = 0;
            scenario.earthquake.max_duration = 0;
            scenario.earthquake.max_delay = 0;
            break;
    }
    scenario.is_saved = 0;
    window_request_refresh();
}

static void set_earthquake_month(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.earthquake.month = value - 1;
}

static void button_earthquake_month(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 237, screen_dialog_offset_y() - 13, 2, 12, set_earthquake_month);
}

static void set_earthquake_year(int year)
{
    scenario.earthquake.year = year;
    scenario.is_saved = 0;
}

static void button_earthquake_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 335, screen_dialog_offset_y() - 10,
                              3, 999, set_earthquake_year);
}

static void button_gladiator_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.gladiator_revolt.state = !scenario.gladiator_revolt.state;
    scenario.is_saved = 0;
    window_request_refresh();
}

static void set_gladiator_revolt_month(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.gladiator_revolt.month = value - 1;
}

static void button_gladiator_month(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 237, screen_dialog_offset_y() + 17, 2, 12, set_gladiator_revolt_month);
}

static void set_gladiator_revolt_year(int year)
{
    scenario.gladiator_revolt.year = year;
    scenario.is_saved = 0;
}

static void button_gladiator_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 335, screen_dialog_offset_y() + 20,
                              3, 999, set_gladiator_revolt_year);
}

static void button_sea_trade_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.random_events.sea_trade_problem = !scenario.random_events.sea_trade_problem;
    scenario.is_saved = 0;
    window_request_refresh();
}

static void button_land_trade_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.random_events.land_trade_problem = !scenario.random_events.land_trade_problem;
    scenario.is_saved = 0;
    window_request_refresh();
}

static void button_raise_wages_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.random_events.raise_wages = !scenario.random_events.raise_wages;
    scenario.is_saved = 0;
    window_request_refresh();
}

static void button_lower_wages_toggle(__attribute__((unused)) int param1, __attribute__((unused))  int param2)
{
    scenario.random_events.lower_wages = !scenario.random_events.lower_wages;
    scenario.is_saved = 0;
    window_request_refresh();
}

static void button_contamination_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.random_events.contaminated_water = !scenario.random_events.contaminated_water;
    scenario.is_saved = 0;
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
