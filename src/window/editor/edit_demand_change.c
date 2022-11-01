#include "edit_demand_change.h"

#include "core/lang.h"
#include "core/string.h"
#include "empire/object.h"
#include "empire/type.h"
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
#include "scenario/editor.h"
#include "window/editor/demand_changes.h"
#include "window/editor/map.h"
#include "window/numeric_input.h"
#include "window/select_list.h"

#define MAX_ROUTES 20
#define NAME_LENGTH 50

static void button_year(int param1, int param2);
static void button_month(int param1, int param2);
static void button_resource(int param1, int param2);
static void button_route(int param1, int param2);
static void button_toggle_rise(int param1, int param2);
static void button_delete(int param1, int param2);

static generic_button buttons_edit_demand_change[] = {
    {130, 152, 100, 25, button_year, button_none, 0, 0},
    {130, 182, 100, 25, button_month, button_none, 0, 0},
    {130, 212, 100, 25, button_resource, button_none, 0, 0},
    {130, 242, 200, 25, button_route, button_none, 0, 0},
    {230, 272, 100, 25, button_toggle_rise, button_none, 0, 0},
    {80, 312, 250, 25, button_delete, button_none, 0, 0},
};

static uint8_t route_display_names[MAX_ROUTES][NAME_LENGTH] = { {"None available"} };

static struct {
    int id;
    int focus_button_id;
    int route_ids[MAX_ROUTES];
    const uint8_t *route_names[MAX_ROUTES];
    int num_routes;
} data;

static void create_route_names(void)
{
    data.num_routes = 0;
    for (int i = 1; i < MAX_ROUTES; i++) {
        empire_object *object = empire_object_get_for_trade_route(i);
        if (object && (object->city_type == EMPIRE_CITY_TRADE || object->city_type == EMPIRE_CITY_FUTURE_TRADE)) {
            if (object->resources_buy_list.resource[scenario.demand_changes[data.id].resource] || object->resources_sell_list.resource[scenario.demand_changes[data.id].resource]) {
                uint8_t *dst = route_display_names[i];
                int offset = string_from_int(dst, i, 0);
                dst[offset++] = ' ';
                dst[offset++] = '-';
                dst[offset++] = ' ';
                string_copy(lang_get_string(21, object->city_name_id), &dst[offset], NAME_LENGTH - offset);
                data.route_ids[data.num_routes] = i;
                data.route_names[data.num_routes] = route_display_names[i];
                data.num_routes++;
            }
        }
    }
}

static void init(int id)
{
    data.id = id;
    create_route_names();
}

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 100, 26, 16);
    // Demand changes
    lang_text_draw_centered(44, 94, 0, 116, 416, FONT_LARGE_BLACK);

    // Year offset
    text_draw(get_custom_string(TR_EDITOR_OFFSET_YEAR), 30, 158, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 152, 100, 25, data.focus_button_id == 1);
    text_draw_number_centered_prefix(scenario.demand_changes[data.id].year, '+', 132, 158, 100, FONT_NORMAL_BLACK);
    lang_text_draw_year(scenario.start_year + scenario.demand_changes[data.id].year, 240, 158, FONT_NORMAL_BLACK);

    // Month
    text_draw(get_custom_string(TR_EDITOR_MONTH), 30, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 182, 100, 25, data.focus_button_id == 2);
    text_draw_number_centered(scenario.demand_changes[data.id].month + 1, 130, 188, 100, FONT_NORMAL_BLACK);

    // Invalid year/month combination
    if (scenario.demand_changes[data.id].year == 0 && scenario.demand_changes[data.id].month == 0) {
        text_draw(get_custom_string(TR_EDITOR_INVALID_YEAR_MONTH), 245, 188, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Resource
    text_draw(get_custom_string(TR_EDITOR_RESOURCE), 30, 218, FONT_NORMAL_BLACK, COLOR_BLACK);
    button_border_draw(130, 212, 100, 25, data.focus_button_id == 3);
    lang_text_draw_centered(23, scenario.demand_changes[data.id].resource, 130, 218, 100, FONT_NORMAL_BLACK);

    // in route
    lang_text_draw(44, 97, 30, 248, FONT_NORMAL_BLACK);
    button_border_draw(130, 242, 200, 25, data.focus_button_id == 4);
    if (scenario.demand_changes[data.id].route_id) {
        text_draw_centered(route_display_names[scenario.demand_changes[data.id].route_id], 130, 248, 200, FONT_NORMAL_BLACK, 0);
    }

    // demand for this good rises/falls
    lang_text_draw(44, 100, 30, 278, FONT_NORMAL_BLACK);
    button_border_draw(230, 272, 100, 25, data.focus_button_id == 5);
    lang_text_draw_centered(44, scenario.demand_changes[data.id].is_rise ? 99 : 98, 230, 278, 100, FONT_NORMAL_BLACK);

    // Cancel fluctuation
    button_border_draw(80, 312, 250, 25, data.focus_button_id == 6);
    lang_text_draw_centered(44, 101, 80, 318, 250, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_edit_demand_change, sizeof(buttons_edit_demand_change) / sizeof(generic_button), &data.focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        scenario_editor_sort_demand_changes();
        window_editor_demand_changes_show();
    }
}

static void set_year(int value)
{
    scenario.demand_changes[data.id].year = value;
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
    scenario.demand_changes[data.id].month = value - 1;
}

static void button_month(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 115, screen_dialog_offset_y() + 95, 2, 12, set_month);
}

static void set_resource(int value)
{
    // reset route_id to force re-selection (upon which valid routes for the resource are determined)
    scenario.demand_changes[data.id].route_id = 0;

    scenario.demand_changes[data.id].resource = value;
}

static void button_resource(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_select_list_show(screen_dialog_offset_x() + 230, screen_dialog_offset_y() + 55, 23, 16, set_resource);
}

static void set_route_id(int index)
{
    scenario.demand_changes[data.id].route_id = data.route_ids[index];
}

static void button_route(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    create_route_names();

    // if no routes available, reset to default text in pop-up window
    if (!data.num_routes) {
        data.route_ids[0] = 0;
        data.route_names[0] = route_display_names[0];
        data.num_routes++;
    }

    window_select_list_show_text(screen_dialog_offset_x() + 330, screen_dialog_offset_y() + 205,
        data.route_names, data.num_routes, set_route_id);
}

static void button_toggle_rise(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.demand_changes[data.id].is_rise = !scenario.demand_changes[data.id].is_rise;
}

static void button_delete(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.demand_changes[data.id].year = 1;
    scenario.demand_changes[data.id].month = 0;
    scenario.demand_changes[data.id].resource = 0;
    scenario.demand_changes[data.id].route_id = 0;
    scenario.demand_changes[data.id].is_rise = 0;
    scenario_editor_sort_demand_changes();
    window_editor_demand_changes_show();
}

void window_editor_edit_demand_change_show(int id)
{
    window_type window = {
        WINDOW_EDITOR_EDIT_DEMAND_CHANGE,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    init(id);
    window_show(&window);
}
