#include "win_criteria.h"

#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/screen.h"
#include "graphics/window.h"
#include "scenario/scenario.h"
#include "window/editor/attributes.h"
#include "window/editor/map.h"
#include "window/numeric_input.h"

enum {
    RATING_CULTURE,
    RATING_PROSPERITY,
    RATING_PEACE,
    RATING_FAVOR
};

static void button_population_toggle(int param1, int param2);
static void button_population_value(int param1, int param2);
static void button_rating_toggle(int rating, int param2);
static void button_rating_value(int rating, int param2);
static void button_time_limit_toggle(int param1, int param2);
static void button_time_limit_years(int param1, int param2);
static void button_survival_toggle(int param1, int param2);
static void button_survival_years(int param1, int param2);

static struct generic_button_t buttons_win_criteria[] = {
    {260, 62, 80, 30, button_population_toggle, button_none, 0, 0},
    {360, 62, 150, 30, button_population_value, button_none, 0, 0},
    {260, 102, 80, 30, button_rating_toggle, button_none, RATING_CULTURE, 0},
    {360, 102, 150, 30, button_rating_value, button_none, RATING_CULTURE, 0},
    {260, 142, 80, 30, button_rating_toggle, button_none, RATING_PROSPERITY, 0},
    {360, 142, 150, 30, button_rating_value, button_none, RATING_PROSPERITY, 0},
    {260, 182, 80, 30, button_rating_toggle, button_none, RATING_PEACE, 0},
    {360, 182, 150, 30, button_rating_value, button_none, RATING_PEACE, 0},
    {260, 222, 80, 30, button_rating_toggle, button_none, RATING_FAVOR, 0},
    {360, 222, 150, 30, button_rating_value, button_none, RATING_FAVOR, 0},
    {260, 262, 80, 30, button_time_limit_toggle, button_none, 0, 0},
    {360, 262, 150, 30, button_time_limit_years, button_none, 0, 0},
    {260, 302, 80, 30, button_survival_toggle, button_none, 0, 0},
    {360, 302, 150, 30, button_survival_years, button_none, 0, 0},
};

static int focus_button_id;

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 34, 24);

    lang_text_draw_centered(44, 48, 0, 16, 544, FONT_LARGE_BLACK);

    // Winning population
    lang_text_draw(44, 56, 30, 70, FONT_NORMAL_BLACK);
    button_border_draw(260, 62, 80, 30, focus_button_id == 1);
    lang_text_draw_centered(18, scenario.population_win_criteria.enabled, 260, 70, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 62, 150, 30, focus_button_id == 2);
    text_draw_number_centered(scenario.population_win_criteria.goal, 360, 70, 150, FONT_NORMAL_BLACK);

    // Culture needed
    lang_text_draw(44, 50, 30, 110, FONT_NORMAL_BLACK);
    button_border_draw(260, 102, 80, 30, focus_button_id == 3);
    lang_text_draw_centered(18, scenario.culture_win_criteria.enabled, 260, 110, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 102, 150, 30, focus_button_id == 4);
    text_draw_number_centered(scenario.culture_win_criteria.goal, 360, 110, 150, FONT_NORMAL_BLACK);

    // Prosperity needed
    lang_text_draw(44, 51, 30, 150, FONT_NORMAL_BLACK);
    button_border_draw(260, 142, 80, 30, focus_button_id == 5);
    lang_text_draw_centered(18, scenario.prosperity_win_criteria.enabled, 260, 150, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 142, 150, 30, focus_button_id == 6);
    text_draw_number_centered(scenario.prosperity_win_criteria.goal, 360, 150, 150, FONT_NORMAL_BLACK);

    // Peace needed
    lang_text_draw(44, 52, 30, 190, FONT_NORMAL_BLACK);
    button_border_draw(260, 182, 80, 30, focus_button_id == 7);
    lang_text_draw_centered(18, scenario.peace_win_criteria.enabled, 260, 190, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 182, 150, 30, focus_button_id == 8);
    text_draw_number_centered(scenario.peace_win_criteria.goal, 360, 190, 150, FONT_NORMAL_BLACK);

    // Favor needed
    lang_text_draw(44, 53, 30, 230, FONT_NORMAL_BLACK);
    button_border_draw(260, 222, 80, 30, focus_button_id == 9);
    lang_text_draw_centered(18, scenario.favor_win_criteria.enabled, 260, 230, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 222, 150, 30, focus_button_id == 10);
    text_draw_number_centered(scenario.favor_win_criteria.goal, 360, 230, 150, FONT_NORMAL_BLACK);

    // Time limit (losing time)
    lang_text_draw(44, 54, 30, 270, FONT_NORMAL_BLACK);
    button_border_draw(260, 262, 80, 30, focus_button_id == 11);
    lang_text_draw_centered(18, scenario.time_limit_win_criteria.enabled, 260, 270, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 262, 150, 30, focus_button_id == 12);
    int width = text_draw_number(scenario.time_limit_win_criteria.years, '+', 0, 380, 270, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.time_limit_win_criteria.years, width + 404, 270, FONT_NORMAL_BLACK);

    // Survival (winning time)
    lang_text_draw(44, 55, 30, 310, FONT_NORMAL_BLACK);
    button_border_draw(260, 302, 80, 30, focus_button_id == 13);
    lang_text_draw_centered(18, scenario.survival_time_win_criteria.enabled, 260, 310, 80, FONT_NORMAL_BLACK);
    button_border_draw(360, 302, 150, 30, focus_button_id == 14);
    width = text_draw_number(scenario.survival_time_win_criteria.years, '+', 0, 380, 310, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.survival_time_win_criteria.years, width + 404, 310, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_win_criteria, sizeof(buttons_win_criteria) / sizeof(struct generic_button_t), &focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_editor_attributes_show();
    }
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
    struct window_type_t window = {
        WINDOW_EDITOR_WIN_CRITERIA,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
    };
    window_show(&window);
}
