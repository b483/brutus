#include "edit_invasion.h"

#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "scenario/editor.h"
#include "scenario/property.h"
#include "scenario/types.h"
#include "translation/translation.h"
#include "window/editor/invasions.h"
#include "window/editor/map.h"
#include "window/numeric_input.h"
#include "window/select_list.h"

static void button_year(int param1, int param2);
static void button_month(int param1, int param2);
static void button_amount(int param1, int param2);
static void button_type(int param1, int param2);
static void button_from(int param1, int param2);
static void button_attack(int param1, int param2);
static void button_delete(int param1, int param2);
static void button_save(int param1, int param2);

static generic_button buttons[] = {
    {145, 152, 60, 25, button_year, button_none},
    {145, 182, 60, 25, button_month, button_none},
    {145, 212, 60, 25, button_amount, button_none},
    {145, 242, 200, 25, button_type, button_none},
    {145, 272, 200, 25, button_from, button_none},
    {145, 302, 200, 25, button_attack, button_none},
    {30, 342, 200, 25, button_delete, button_none},
    {270, 342, 80, 25, button_save, button_none},
};

static struct {
    int id;
    editor_invasion invasion;
    int focus_button_id;
} data;

static void init(int id)
{
    data.id = id;
    scenario_editor_invasion_get(id, &data.invasion);
    // Jan is 1 for input/draw purposes
    data.invasion.month += 1;
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
    lang_text_draw_centered(44, 22, 0, 114, 384, FONT_LARGE_BLACK);

    // Year offset
    text_draw(translation_for(TR_EDITOR_INVASION_YEAR), 30, 158, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 152, 60, 25, data.focus_button_id == 1);
    text_draw_number_centered_prefix(data.invasion.year, '+', 145, 158, 60, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario_property_start_year() + data.invasion.year, 215, 158, FONT_NORMAL_BLACK);

    // Month
    text_draw(translation_for(TR_EDITOR_INVASION_MONTH), 30, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 182, 60, 25, data.focus_button_id == 2);
    text_draw_number_centered(data.invasion.month, 145, 188, 60, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (data.invasion.year == 0 && data.invasion.month == 1) {
        text_draw(translation_for(TR_EDITOR_INVASION_INVALID_MONTH), 220, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Amount
    text_draw(translation_for(TR_EDITOR_INVASION_AMOUNT), 30, 218, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 212, 60, 25, data.focus_button_id == 3);
    text_draw_number_centered(data.invasion.amount, 145, 218, 60, FONT_NORMAL_BLACK);

    // Type
    text_draw(translation_for(TR_EDITOR_INVASION_TYPE), 30, 248, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(145, 242, 200, 25, data.focus_button_id == 4);
    lang_text_draw_centered(34, data.invasion.type, 145, 248, 200, FONT_NORMAL_BLACK);

    if (data.invasion.type != INVASION_TYPE_DISTANT_BATTLE) {
        // From
        text_draw(translation_for(TR_EDITOR_INVASION_FROM), 30, 278, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(145, 272, 200, 25, data.focus_button_id == 5);
        lang_text_draw_centered(35, data.invasion.from, 145, 278, 200, FONT_NORMAL_BLACK);

        // Attack type
        text_draw(translation_for(TR_EDITOR_INVASION_ATTACK_TYPE), 30, 308, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(145, 302, 200, 25, data.focus_button_id == 6);
        lang_text_draw_centered(36, data.invasion.attack_type, 145, 308, 200, FONT_NORMAL_BLACK);
    }

    // Unschedule invasion
    button_border_draw(30, 342, 200, 25, data.focus_button_id == 7);
    lang_text_draw_centered(44, 26, 30, 350, 200, FONT_NORMAL_BLACK);

    // OK
    button_border_draw(270, 342, 80, 25, data.focus_button_id == 8);
    lang_text_draw_centered(18, 3, 270, 350, 80, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons, sizeof(buttons) / sizeof(generic_button), &data.focus_button_id)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        button_save(0, 0);
    }
}

static void set_year(int value)
{
    data.invasion.year = value;
}

static void button_year(int param1, int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 65, 3, 999, set_year);
}

static void set_month(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    data.invasion.month = value;
}

static void button_month(int param1, int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 95, 2, 12, set_month);
}

static void set_amount(int value)
{
    data.invasion.amount = value;
}

static void button_amount(int param1, int param2)
{
    // if this is set to 0, you get the incoming battle messages, but the enemies never show up... could be a cool trick for a map
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 125, 3, 200, set_amount);
}

static void set_type(int value)
{
    data.invasion.type = value == 3 ? 4 : value;
}

static void button_type(int param1, int param2)
{
    window_select_list_show(screen_dialog_offset_x() + 350, screen_dialog_offset_y() + 240, 34, 4, set_type);
}

static void set_from(int value)
{
    data.invasion.from = value;
}

static void button_from(int param1, int param2)
{
    if (data.invasion.type != INVASION_TYPE_DISTANT_BATTLE) {
        window_select_list_show(screen_dialog_offset_x() + 350, screen_dialog_offset_y() + 195, 35, 9, set_from);
    }
}

static void set_attack(int value)
{
    data.invasion.attack_type = value;
}

static void button_attack(int param1, int param2)
{
    if (data.invasion.type != INVASION_TYPE_DISTANT_BATTLE) {
        window_select_list_show(screen_dialog_offset_x() + 350, screen_dialog_offset_y() + 215, 36, 5, set_attack);
    }
}

static void button_delete(int param1, int param2)
{
    scenario_editor_invasion_delete(data.id);
    window_editor_invasions_show();
}

static void button_save(int param1, int param2)
{
    // change month back to 0 indexed before saving
    data.invasion.month -= 1;
    scenario_editor_invasion_save(data.id, &data.invasion);
    window_editor_invasions_show();
}

void window_editor_edit_invasion_show(int id)
{
    window_type window = {
        WINDOW_EDITOR_EDIT_INVASION,
        draw_background,
        draw_foreground,
        handle_input
    };
    init(id);
    window_show(&window);
}
