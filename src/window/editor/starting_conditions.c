#include "starting_conditions.h"

#include "game/custom_strings.h"
#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/screen.h"
#include "graphics/window.h"
#include "input/input.h"
#include "scenario/editor.h"
#include "scenario/map.h"
#include "scenario/property.h"
#include "window/editor/attributes.h"
#include "window/editor/map.h"
#include "window/editor/start_year.h"
#include "window/numeric_input.h"
#include "window/select_list.h"

static void button_rank(int param1, int param2);
static void button_start_year(int param1, int param2);
static void button_initial_favor(int param1, int param2);
static void button_initial_funds(int param1, int param2);
static void button_rescue_loan(int param1, int param2);
static void button_initial_personal_savings(int param1, int param2);
static void button_wheat(int param1, int param2);
static void button_flotsam(int param1, int param2);
static void button_milestone(int milestone_pct, int param2);

static generic_button buttons[] = {
    {262, 48, 200, 30, button_rank, button_none, 0, 0},
    {262, 88, 200, 30, button_start_year, button_none, 0, 0},
    {262, 128, 200, 30, button_initial_favor, button_none, 0, 0},
    {262, 168, 200, 30, button_initial_funds,button_none, 0, 0},
    {262, 208, 200, 30, button_rescue_loan,button_none, 0, 0},
    {262, 248, 200, 30, button_initial_personal_savings, button_none, 0, 0},
    {262, 288, 200, 30, button_wheat,button_none, 0, 0},
    {262, 328, 200, 30, button_flotsam, button_none, 0, 0},
    {262, 368, 200, 30, button_milestone, button_none, 25, 0},
    {262, 408, 200, 30, button_milestone, button_none, 50, 0},
    {262, 448, 200, 30, button_milestone, button_none, 75, 0}
};

static int focus_button_id;

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 30, 31);

    lang_text_draw_centered(44, 88, 0, 16, 480, FONT_LARGE_BLACK);

    // Initial rank
    lang_text_draw(44, 108, 32, 57, FONT_NORMAL_BLACK);
    button_border_draw(262, 48, 200, 30, focus_button_id == 1);
    lang_text_draw_centered(32, scenario_property_player_rank(), 262, 57, 200, FONT_NORMAL_BLACK);

    // Adjust the start date
    lang_text_draw(44, 89, 32, 97, FONT_NORMAL_BLACK);
    button_border_draw(262, 88, 200, 30, focus_button_id == 2);
    lang_text_draw_year(scenario_property_start_year(), 330, 97, FONT_NORMAL_BLACK);

    // Initial favor
    text_draw(get_custom_string(TR_EDITOR_INITIAL_FAVOR), 32, 137, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(262, 128, 200, 30, focus_button_id == 3);
    text_draw_number_centered(scenario_initial_favor(), 262, 137, 200, FONT_NORMAL_BLACK);

    // Initial funds
    lang_text_draw(44, 39, 32, 177, FONT_NORMAL_BLACK);
    button_border_draw(262, 168, 200, 30, focus_button_id == 4);
    text_draw_number_centered(scenario_initial_funds(), 262, 177, 200, FONT_NORMAL_BLACK);

    // Rescue loan
    lang_text_draw(44, 68, 32, 217, FONT_NORMAL_BLACK);
    button_border_draw(262, 208, 200, 30, focus_button_id == 5);
    text_draw_number_centered(scenario_rescue_loan(), 262, 217, 200, FONT_NORMAL_BLACK);

    // Initial personal savings
    text_draw(get_custom_string(TR_EDITOR_INITIAL_PERSONAL_SAVINGS), 32, 257, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(262, 248, 200, 30, focus_button_id == 6);
    text_draw_number_centered(scenario_initial_personal_savings(), 262, 257, 200, FONT_NORMAL_BLACK);

    // Rome supplies wheat?
    lang_text_draw(44, 43, 32, 297, FONT_NORMAL_BLACK);
    button_border_draw(262, 288, 200, 30, focus_button_id == 7);
    lang_text_draw_centered(18, scenario_property_rome_supplies_wheat(), 262, 297, 200, FONT_NORMAL_BLACK);

    // Flotsam on?
    lang_text_draw(44, 80, 32, 337, FONT_NORMAL_BLACK);
    button_border_draw(262, 328, 200, 30, focus_button_id == 8);
    lang_text_draw_centered(18, scenario_map_has_flotsam(), 262, 337, 200, FONT_NORMAL_BLACK);

    // Milestone - 25%
    lang_text_draw(44, 91, 32, 377, FONT_NORMAL_BLACK);
    button_border_draw(262, 368, 200, 30, focus_button_id == 9);
    int width = text_draw_number(scenario_editor_milestone_year(25), '+', " ", 297, 379, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario_property_start_year() + scenario_editor_milestone_year(25),
        307 + width, 379, FONT_SMALL_PLAIN);

    // Milestone - 50%
    lang_text_draw(44, 92, 32, 417, FONT_NORMAL_BLACK);
    button_border_draw(262, 408, 200, 30, focus_button_id == 10);
    width = text_draw_number(scenario_editor_milestone_year(50), '+', " ", 297, 419, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario_property_start_year() + scenario_editor_milestone_year(50),
        307 + width, 419, FONT_SMALL_PLAIN);

    // Milestone - 75%
    lang_text_draw(44, 93, 32, 457, FONT_NORMAL_BLACK);
    button_border_draw(262, 448, 200, 30, focus_button_id == 11);
    width = text_draw_number(scenario_editor_milestone_year(75), '+', " ", 297, 459, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario_property_start_year() + scenario_editor_milestone_year(75),
        307 + width, 459, FONT_SMALL_PLAIN);

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

static void button_rank(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show(screen_dialog_offset_x() + 40, screen_dialog_offset_y() + 56,
                            32, 11, scenario_editor_set_player_rank);
}

static void button_start_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_editor_start_year_show();
}

static void button_initial_favor(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 120, screen_dialog_offset_y() + 56,
                              3, 100, scenario_editor_set_initial_favor);
}

static void button_initial_funds(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 120, screen_dialog_offset_y() + 56,
                              5, 99999, scenario_editor_set_initial_funds);
}

static void button_rescue_loan(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 120, screen_dialog_offset_y() + 56,
                              5, 99999, scenario_editor_set_rescue_loan);
}

static void button_initial_personal_savings(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 120, screen_dialog_offset_y() + 56,
                              5, 99999, scenario_editor_set_initial_personal_savings);
}

static void button_wheat(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_toggle_rome_supplies_wheat();
}

static void button_flotsam(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_toggle_flotsam();
}

static int dialog_milestone_pct;
static void set_milestone_year(int value)
{
    scenario_editor_set_milestone_year(dialog_milestone_pct, value);
}

static void button_milestone(int milestone_pct, __attribute__((unused)) int param2)
{
    dialog_milestone_pct = milestone_pct;
    window_numeric_input_show(screen_dialog_offset_x() + 120, screen_dialog_offset_y() + 210,
                              3, 999, set_milestone_year);
}

void window_editor_starting_conditions_show(void)
{
    window_type window = {
        WINDOW_EDITOR_STARTING_CONDITIONS,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    window_show(&window);
}
