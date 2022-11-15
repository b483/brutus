#include "win_criteria.h"

#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/screen.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "window/editor/attributes.h"
#include "window/editor/map.h"
#include "window/numeric_input.h"

enum {
    RATING_CULTURE,
    RATING_PROSPERITY,
    RATING_PEACE,
    RATING_FAVOR
};

static void button_open_play_toggle(int param1, int param2);
static void button_population_toggle(int param1, int param2);
static void button_population_value(int param1, int param2);
static void button_rating_toggle(int rating, int param2);
static void button_rating_value(int rating, int param2);
static void button_time_limit_toggle(int param1, int param2);
static void button_time_limit_years(int param1, int param2);
static void button_survival_toggle(int param1, int param2);
static void button_survival_years(int param1, int param2);

static generic_button buttons_win_criteria[] = {
    {274, 60, 80, 30, button_open_play_toggle, button_none, 0, 0},
    {274, 100, 80, 30, button_population_toggle, button_none, 0, 0},
    {374, 100, 180, 30, button_population_value, button_none, 0, 0},
    {274, 140, 80, 30, button_rating_toggle, button_none, RATING_CULTURE, 0},
    {374, 140, 180, 30, button_rating_value, button_none, RATING_CULTURE, 0},
    {274, 180, 80, 30, button_rating_toggle, button_none, RATING_PROSPERITY, 0},
    {374, 180, 180, 30, button_rating_value, button_none, RATING_PROSPERITY, 0},
    {274, 220, 80, 30, button_rating_toggle, button_none, RATING_PEACE, 0},
    {374, 220, 180, 30, button_rating_value, button_none, RATING_PEACE, 0},
    {274, 260, 80, 30, button_rating_toggle, button_none, RATING_FAVOR, 0},
    {374, 260, 180, 30, button_rating_value, button_none, RATING_FAVOR, 0},
    {274, 300, 80, 30, button_time_limit_toggle, button_none, 0, 0},
    {374, 300, 180, 30, button_time_limit_years, button_none, 0, 0},
    {274, 340, 80, 30, button_survival_toggle, button_none, 0, 0},
    {374, 340, 180, 30, button_survival_years, button_none, 0, 0},
};

static int focus_button_id;

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 36, 25);

    lang_text_draw_centered(44, 48, 0, 16, 576, FONT_LARGE_BLACK);

    // advanced feature: open play
    lang_text_draw(44, 107, 30, 69, FONT_NORMAL_BLACK);
    button_border_draw(274, 60, 80, 30, focus_button_id == 1);
    lang_text_draw_centered(18, scenario.is_open_play, 274, 69, 80, FONT_NORMAL_BLACK);

    // Winning population
    lang_text_draw(44, 56, 30, 109, FONT_NORMAL_BLACK);
    button_border_draw(274, 100, 80, 30, focus_button_id == 2);
    lang_text_draw_centered(18, scenario.is_open_play ? 0 : scenario.population_win_criteria.enabled,
        274, 109, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 100, 180, 30, focus_button_id == 3);
    text_draw_number_centered(scenario.population_win_criteria.goal, 374, 109, 180, FONT_NORMAL_BLACK);

    // Culture needed
    lang_text_draw(44, 50, 30, 149, FONT_NORMAL_BLACK);
    button_border_draw(274, 140, 80, 30, focus_button_id == 4);
    lang_text_draw_centered(18, scenario.is_open_play ? 0 : scenario.culture_win_criteria.enabled,
        274, 149, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 140, 180, 30, focus_button_id == 5);
    text_draw_number_centered(scenario.culture_win_criteria.goal, 374, 149, 180, FONT_NORMAL_BLACK);

    // Prosperity needed
    lang_text_draw(44, 51, 30, 189, FONT_NORMAL_BLACK);
    button_border_draw(274, 180, 80, 30, focus_button_id == 6);
    lang_text_draw_centered(18, scenario.is_open_play ? 0 : scenario.prosperity_win_criteria.enabled,
        274, 189, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 180, 180, 30, focus_button_id == 7);
    text_draw_number_centered(scenario.prosperity_win_criteria.goal, 374, 189, 180, FONT_NORMAL_BLACK);

    // Peace needed
    lang_text_draw(44, 52, 30, 229, FONT_NORMAL_BLACK);
    button_border_draw(274, 220, 80, 30, focus_button_id == 8);
    lang_text_draw_centered(18, scenario.is_open_play ? 0 : scenario.peace_win_criteria.enabled,
        274, 229, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 220, 180, 30, focus_button_id == 9);
    text_draw_number_centered(scenario.peace_win_criteria.goal, 374, 229, 180, FONT_NORMAL_BLACK);

    // Favor needed
    lang_text_draw(44, 53, 30, 269, FONT_NORMAL_BLACK);
    button_border_draw(274, 260, 80, 30, focus_button_id == 10);
    lang_text_draw_centered(18, scenario.is_open_play ? 0 : scenario.favor_win_criteria.enabled,
        274, 269, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 260, 180, 30, focus_button_id == 11);
    text_draw_number_centered(scenario.favor_win_criteria.goal, 374, 269, 180, FONT_NORMAL_BLACK);

    // Time limit (losing time)
    lang_text_draw(44, 54, 30, 309, FONT_NORMAL_BLACK);
    button_border_draw(274, 300, 80, 30, focus_button_id == 12);
    lang_text_draw_centered(18, scenario.is_open_play ? 0 : scenario.time_limit_win_criteria.enabled,
        274, 309, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 300, 180, 30, focus_button_id == 13);
    int width = text_draw_number(scenario.time_limit_win_criteria.years, '+', 0, 394, 309, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.time_limit_win_criteria.years, width + 404, 309, FONT_NORMAL_BLACK);

    // Survival (winning time)
    lang_text_draw(44, 55, 30, 349, FONT_NORMAL_BLACK);
    button_border_draw(274, 340, 80, 30, focus_button_id == 14);
    lang_text_draw_centered(18, scenario.is_open_play ? 0 : scenario.survival_time_win_criteria.enabled,
        274, 349, 80, FONT_NORMAL_BLACK);
    button_border_draw(374, 340, 180, 30, focus_button_id == 15);
    width = text_draw_number(scenario.survival_time_win_criteria.years, '+', 0, 394, 349, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.survival_time_win_criteria.years, width + 404, 349, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_win_criteria, sizeof(buttons_win_criteria) / sizeof(generic_button), &focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_editor_attributes_show();
    }
}

static void button_open_play_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.is_open_play = !scenario.is_open_play;
    scenario.is_saved = 0;
}

static void button_population_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.population_win_criteria.enabled = !scenario.population_win_criteria.enabled;
    scenario.is_saved = 0;
}

static void set_population(int goal)
{
    scenario.population_win_criteria.goal = goal;
    scenario.is_saved = 0;
}

static void button_population_value(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 24, 5, 99999, set_population);
}

static void button_rating_toggle(int rating, __attribute__((unused)) int param2)
{
    switch (rating) {
        case RATING_CULTURE:
            scenario.culture_win_criteria.enabled = !scenario.culture_win_criteria.enabled;
            scenario.is_saved = 0;
            break;
        case RATING_PROSPERITY:
            scenario.prosperity_win_criteria.enabled = !scenario.prosperity_win_criteria.enabled;
            scenario.is_saved = 0;
            break;
        case RATING_PEACE:
            scenario.peace_win_criteria.enabled = !scenario.peace_win_criteria.enabled;
            scenario.is_saved = 0;
            break;
        case RATING_FAVOR:
            scenario.favor_win_criteria.enabled = !scenario.favor_win_criteria.enabled;
            scenario.is_saved = 0;
            break;
    }
}

static void set_culture(int goal)
{
    scenario.culture_win_criteria.goal = goal;
    scenario.is_saved = 0;
}

static void set_prosperity(int goal)
{
    scenario.prosperity_win_criteria.goal = goal;
    scenario.is_saved = 0;
}

static void set_peace(int goal)
{
    scenario.peace_win_criteria.goal = goal;
    scenario.is_saved = 0;
}

static void set_favor(int goal)
{
    scenario.favor_win_criteria.goal = goal;
    scenario.is_saved = 0;
}

static void button_rating_value(int rating, __attribute__((unused)) int param2)
{
    switch (rating) {
        case RATING_CULTURE:
            window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 104, 3, 100, set_culture);
            break;
        case RATING_PROSPERITY:
            window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 104, 3, 100, set_prosperity);
            break;
        case RATING_PEACE:
            window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 104, 3, 100, set_peace);
            break;
        case RATING_FAVOR:
            window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 104, 3, 100, set_favor);
            break;
        default:
            return;
    }
}

static void button_time_limit_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.time_limit_win_criteria.enabled = !scenario.time_limit_win_criteria.enabled;
    if (scenario.time_limit_win_criteria.enabled) {
        scenario.survival_time_win_criteria.enabled = 0;
    }
    scenario.is_saved = 0;
}

static void set_time_limit(int years)
{
    scenario.time_limit_win_criteria.years = years;
    scenario.is_saved = 0;
}

static void button_time_limit_years(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 224, 3, 999, set_time_limit);
}

static void button_survival_toggle(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.survival_time_win_criteria.enabled = !scenario.survival_time_win_criteria.enabled;
    if (scenario.survival_time_win_criteria.enabled) {
        scenario.time_limit_win_criteria.enabled = 0;
    }
    scenario.is_saved = 0;
}

static void set_survival_time(int years)
{
    scenario.survival_time_win_criteria.years = years;
    scenario.is_saved = 0;
}

static void button_survival_years(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 402, screen_dialog_offset_y() + 264, 3, 999, set_survival_time);
}

void window_editor_win_criteria_show(void)
{
    window_type window = {
        WINDOW_EDITOR_WIN_CRITERIA,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
        0
    };
    window_show(&window);
}
