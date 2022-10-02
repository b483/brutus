#include "win_criteria.h"

#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/screen.h"
#include "graphics/window.h"
#include "input/input.h"
#include "scenario/criteria.h"
#include "scenario/editor.h"
#include "scenario/property.h"
#include "window/editor/attributes.h"
#include "window/editor/map.h"
#include "window/numeric_input.h"

enum {
    RATING_CULTURE,
    RATING_PROSPERITY,
    RATING_PEACE,
    RATING_FAVOR
};

static void button_rating_toggle(int rating, int param2);
static void button_rating_value(int rating, int param2);
static void button_time_limit_toggle(int param1, int param2);
static void button_time_limit_years(int param1, int param2);
static void button_survival_toggle(int param1, int param2);
static void button_survival_years(int param1, int param2);
static void button_population_toggle(int param1, int param2);
static void button_population_value(int param1, int param2);
static void button_open_play_toggle(int param1, int param2);

static generic_button buttons[] = {
    {274, 100, 80, 30, button_rating_toggle, button_none, RATING_CULTURE, 0},
    {374, 100, 180, 30, button_rating_value, button_none, RATING_CULTURE, 0},
    {274, 140, 80, 30, button_rating_toggle, button_none, RATING_PROSPERITY, 0},
    {374, 140, 180, 30, button_rating_value, button_none, RATING_PROSPERITY, 0},
    {274, 180, 80, 30, button_rating_toggle, button_none, RATING_PEACE, 0},
    {374, 180, 180, 30, button_rating_value, button_none, RATING_PEACE, 0},
    {274, 220, 80, 30, button_rating_toggle, button_none, RATING_FAVOR, 0},
    {374, 220, 180, 30, button_rating_value, button_none, RATING_FAVOR, 0},
    {274, 260, 80, 30, button_time_limit_toggle, button_none, 0, 0},
    {374, 260, 180, 30, button_time_limit_years, button_none, 0, 0},
    {274, 300, 80, 30, button_survival_toggle, button_none, 0, 0},
    {374, 300, 180, 30, button_survival_years, button_none, 0, 0},
    {274, 340, 80, 30, button_population_toggle, button_none, 0, 0},
    {374, 340, 180, 30, button_population_value, button_none, 0, 0},
    {274, 60, 80, 30, button_open_play_toggle, button_none, 0, 0},
};

static int focus_button_id;

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 36, 25);

    lang_text_draw_centered(44, 48, 0, 16, 576, FONT_LARGE_BLACK);

    int is_open_play = scenario_is_open_play();
    // advanced feature: open play
    lang_text_draw(44, 107, 30, 69, FONT_NORMAL_BLACK);
    button_border_draw(274, 60, 80, 30, focus_button_id == 15);
    lang_text_draw_centered(18, is_open_play, 274, 69, 80, FONT_NORMAL_BLACK);

    // Culture needed
    lang_text_draw(44, 50, 30, 109, FONT_NORMAL_BLACK);
    button_border_draw(274, 100, 80, 30, focus_button_id == 1);
    lang_text_draw_centered(18, is_open_play ? 0 : scenario_criteria_culture_enabled(),
        274, 109, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 100, 180, 30, focus_button_id == 2);
    text_draw_number_centered(scenario_criteria_culture(), 374, 109, 180, FONT_NORMAL_BLACK);

    // Prosperity needed
    lang_text_draw(44, 51, 30, 149, FONT_NORMAL_BLACK);
    button_border_draw(274, 140, 80, 30, focus_button_id == 3);
    lang_text_draw_centered(18, is_open_play ? 0 : scenario_criteria_prosperity_enabled(),
        274, 149, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 140, 180, 30, focus_button_id == 4);
    text_draw_number_centered(scenario_criteria_prosperity(), 374, 149, 180, FONT_NORMAL_BLACK);

    // Peace needed
    lang_text_draw(44, 52, 30, 189, FONT_NORMAL_BLACK);
    button_border_draw(274, 180, 80, 30, focus_button_id == 5);
    lang_text_draw_centered(18, is_open_play ? 0 : scenario_criteria_peace_enabled(),
        274, 189, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 180, 180, 30, focus_button_id == 6);
    text_draw_number_centered(scenario_criteria_peace(), 374, 189, 180, FONT_NORMAL_BLACK);

    // Favor needed
    lang_text_draw(44, 53, 30, 229, FONT_NORMAL_BLACK);
    button_border_draw(274, 220, 80, 30, focus_button_id == 7);
    lang_text_draw_centered(18, is_open_play ? 0 : scenario_criteria_favor_enabled(),
        274, 229, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 220, 180, 30, focus_button_id == 8);
    text_draw_number_centered(scenario_criteria_favor(), 374, 229, 180, FONT_NORMAL_BLACK);

    // Time limit (losing time)
    lang_text_draw(44, 54, 30, 269, FONT_NORMAL_BLACK);
    button_border_draw(274, 260, 80, 30, focus_button_id == 9);
    lang_text_draw_centered(18, is_open_play ? 0 : scenario_criteria_time_limit_enabled(),
        274, 269, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 260, 180, 30, focus_button_id == 10);
    int width = text_draw_number(scenario_criteria_time_limit_years(), '+', " ", 394, 269, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario_property_start_year() + scenario_criteria_time_limit_years(),
        404 + width, 269, FONT_NORMAL_BLACK);

    // Survival (winning time)
    lang_text_draw(44, 55, 30, 309, FONT_NORMAL_BLACK);
    button_border_draw(274, 300, 80, 30, focus_button_id == 11);
    lang_text_draw_centered(18, is_open_play ? 0 : scenario_criteria_survival_enabled(),
        274, 309, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 300, 180, 30, focus_button_id == 12);
    width = text_draw_number(scenario_criteria_survival_years(), '+', " ", 394, 309, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario_property_start_year() + scenario_criteria_survival_years(),
        404 + width, 309, FONT_NORMAL_BLACK);

    // Winning population
    lang_text_draw(44, 56, 30, 349, FONT_NORMAL_BLACK);
    button_border_draw(274, 340, 80, 30, focus_button_id == 13);
    lang_text_draw_centered(18, is_open_play ? 0 : scenario_criteria_population_enabled(),
        274, 349, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 340, 180, 30, focus_button_id == 14);
    text_draw_number_centered(scenario_criteria_population(), 374, 349, 180, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons, 15, &focus_button_id)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        window_editor_attributes_show();
    }
}

static void button_rating_toggle(int rating, __attribute__((unused)) int param2)
{
    switch (rating) {
        case RATING_CULTURE:
            scenario_editor_toggle_culture();
            break;
        case RATING_PROSPERITY:
            scenario_editor_toggle_prosperity();
            break;
        case RATING_PEACE:
            scenario_editor_toggle_peace();
            break;
        case RATING_FAVOR:
            scenario_editor_toggle_favor();
            break;
    }
}

static void button_rating_value(int rating, __attribute__((unused)) int param2)
{
    void (*callback)(int);
    switch (rating) {
        case RATING_CULTURE:
            callback = scenario_editor_set_culture;
            break;
        case RATING_PROSPERITY:
            callback = scenario_editor_set_prosperity;
            break;
        case RATING_PEACE:
            callback = scenario_editor_set_peace;
            break;
        case RATING_FAVOR:
            callback = scenario_editor_set_favor;
            break;
        default:
            return;
    }
    window_numeric_input_show(screen_dialog_offset_x() + 280, screen_dialog_offset_y() + 100,
                              3, 100, callback);
}

static void button_time_limit_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_toggle_time_limit();
}

static void button_time_limit_years(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 280, screen_dialog_offset_y() + 200,
                              3, 999, scenario_editor_set_time_limit);
}

static void button_survival_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_toggle_survival_time();
}

static void button_survival_years(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 280, screen_dialog_offset_y() + 200,
                              3, 999, scenario_editor_set_survival_time);
}

static void button_population_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_toggle_population();
}

static void button_population_value(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 280, screen_dialog_offset_y() + 200,
                              5, 99999, scenario_editor_set_population);
}

static void button_open_play_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_toggle_open_play();
}

void window_editor_win_criteria_show(void)
{
    window_type window = {
        WINDOW_EDITOR_WIN_CRITERIA,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    window_show(&window);
}
