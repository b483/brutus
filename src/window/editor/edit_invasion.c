#include "edit_invasion.h"

#include "game/custom_strings.h"
#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "scenario/data.h"
#include "scenario/editor.h"
#include "scenario/property.h"
#include "window/editor/invasions.h"
#include "window/editor/map.h"
#include "window/numeric_input.h"
#include "window/select_list.h"

#define INVASION_TYPE_MAX_COUNT 5

static void button_year(int param1, int param2);
static void button_month(int param1, int param2);
static void button_amount(int param1, int param2);
static void button_type(int param1, int param2);
static void button_from(int param1, int param2);
static void button_attack_type(int param1, int param2);
static void button_delete(int param1, int param2);

static generic_button buttons_edit_invasion[] = {
    {145, 152, 60, 25, button_year, button_none, 0, 0},
    {145, 182, 60, 25, button_month, button_none, 0, 0},
    {145, 212, 60, 25, button_amount, button_none, 0, 0},
    {145, 242, 200, 25, button_type, button_none, 0, 0},
    {145, 272, 200, 25, button_from, button_none, 0, 0},
    {145, 302, 200, 25, button_attack_type, button_none, 0, 0},
    {90, 342, 200, 25, button_delete, button_none, 0, 0},
};

static struct {
    int id;
    const uint8_t *invasion_type_names[INVASION_TYPE_MAX_COUNT];
    int focus_button_id;
} data;

static void init(int id)
{
    data.id = id;
    for (int i = TR_EDITOR_INVASION_TYPE_NO_INVADERS; i <= TR_EDITOR_INVASION_TYPE_DISTANT_BATTLE; i++) {
        data.invasion_type_names[i - TR_EDITOR_INVASION_TYPE_NO_INVADERS] = get_custom_string(i);
    }
}

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 100, 24, 18);
    // Scheduling an invasion
    lang_text_draw_centered(44, 22, 0, 116, 384, FONT_LARGE_BLACK);

    // Year offset
    text_draw(get_custom_string(TR_EDITOR_OFFSET_YEAR), 30, 158, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 152, 60, 25, data.focus_button_id == 1);
    text_draw_number_centered_prefix(scenario.invasions[data.id].year, '+', 147, 158, 60, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario_property_start_year() + scenario.invasions[data.id].year, 215, 158, FONT_NORMAL_BLACK);

    // Month
    text_draw(get_custom_string(TR_EDITOR_MONTH), 30, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 182, 60, 25, data.focus_button_id == 2);
    text_draw_number_centered(scenario.invasions[data.id].month + 1, 145, 188, 60, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (scenario.invasions[data.id].year == 0 && scenario.invasions[data.id].month == 0) {
        text_draw(get_custom_string(TR_EDITOR_INVALID_YEAR_MONTH), 220, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Amount
    text_draw(get_custom_string(TR_EDITOR_AMOUNT), 30, 218, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 212, 60, 25, data.focus_button_id == 3);
    text_draw_number_centered(scenario.invasions[data.id].amount, 145, 218, 60, FONT_NORMAL_BLACK);

    // Type
    text_draw(get_custom_string(TR_EDITOR_INVASION_TYPE), 30, 248, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 242, 200, 25, data.focus_button_id == 4);
    text_draw_centered(get_custom_string(TR_EDITOR_INVASION_TYPE_NO_INVADERS + scenario.invasions[data.id].type), 145, 248, 200, FONT_NORMAL_BLACK, COLOR_BLACK);

    if (scenario.invasions[data.id].type != INVASION_TYPE_DISTANT_BATTLE) {
        if (scenario.invasions[data.id].type != INVASION_TYPE_CAESAR) {
            // From
            text_draw(get_custom_string(TR_EDITOR_INVASION_FROM), 30, 278, FONT_NORMAL_BLACK, COLOR_BLACK);
            button_border_draw(145, 272, 200, 25, data.focus_button_id == 5);
            lang_text_draw_centered(35, scenario.invasions[data.id].from, 145, 278, 200, FONT_NORMAL_BLACK);
        }
        // Attack type
        text_draw(get_custom_string(TR_EDITOR_INVASION_ATTACK_TYPE), 30, 308, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(145, 302, 200, 25, data.focus_button_id == 6);
        lang_text_draw_centered(36, scenario.invasions[data.id].attack_type, 145, 308, 200, FONT_NORMAL_BLACK);
    }

    // Unschedule invasion
    button_border_draw(90, 342, 200, 25, data.focus_button_id == 7);
    lang_text_draw_centered(44, 26, 90, 348, 200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_edit_invasion, sizeof(buttons_edit_invasion) / sizeof(generic_button), &data.focus_button_id)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        scenario_editor_sort_invasions();
        window_editor_invasions_show();
    }
}

static void set_year(int value)
{
    scenario.invasions[data.id].year = value;
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
    scenario.invasions[data.id].month = value - 1;
}

static void button_month(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 95, 2, 12, set_month);
}

static void set_amount(int value)
{
    scenario.invasions[data.id].amount = value;
}

static void button_amount(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    // if this is set to 0, you get the incoming battle messages, but the enemies never show up... could be a cool trick for a map
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 125, 3, 200, set_amount);
}

static void set_type(int value)
{
    scenario.invasions[data.id].type = value;
}

static void button_type(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show_text(screen_dialog_offset_x() + 350, screen_dialog_offset_y() + 240, data.invasion_type_names, INVASION_TYPE_MAX_COUNT, set_type);
}

static void set_from(int value)
{
    scenario.invasions[data.id].from = value;
}

static void button_from(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (scenario.invasions[data.id].type != INVASION_TYPE_DISTANT_BATTLE && scenario.invasions[data.id].type != INVASION_TYPE_CAESAR) {
        window_select_list_show(screen_dialog_offset_x() + 350, screen_dialog_offset_y() + 195, 35, 9, set_from);
    }
}

static void set_attack(int value)
{
    scenario.invasions[data.id].attack_type = value;
}

static void button_attack_type(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (scenario.invasions[data.id].type != INVASION_TYPE_DISTANT_BATTLE) {
        window_select_list_show(screen_dialog_offset_x() + 350, screen_dialog_offset_y() + 215, 36, 5, set_attack);
    }
}

static void button_delete(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.invasions[data.id].year = 1;
    scenario.invasions[data.id].month = 0;
    scenario.invasions[data.id].amount = 0;
    scenario.invasions[data.id].type = 0;
    scenario.invasions[data.id].from = 8;
    scenario.invasions[data.id].attack_type = 0;
    scenario_editor_sort_invasions();
    window_editor_invasions_show();
}

void window_editor_edit_invasion_show(int id)
{
    window_type window = {
        WINDOW_EDITOR_EDIT_INVASION,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    init(id);
    window_show(&window);
}
