#include "attributes.h"

#include "core/image.h"
#include "core/image_group_editor.h"
#include "core/string.h"
#include "game/custom_strings.h"
#include "game/resource.h"
#include "graphics/arrow_button.h"
#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "scenario/editor.h"
#include "widget/input_box.h"
#include "widget/minimap.h"
#include "widget/sidebar/editor.h"
#include "window/editor/allowed_buildings.h"
#include "window/editor/briefing.h"
#include "window/editor/demand_changes.h"
#include "window/editor/invasions.h"
#include "window/editor/map.h"
#include "window/editor/price_changes.h"
#include "window/editor/requests.h"
#include "window/editor/special_events.h"
#include "window/editor/starting_conditions.h"
#include "window/editor/win_criteria.h"
#include "window/select_list.h"

static void button_briefing(int param1, int param2);
static void button_starting_conditions(int param1, int param2);
static void button_requests(int param1, int param2);
static void button_enemy(int param1, int param2);
static void button_invasions(int param1, int param2);
static void button_allowed_buildings(int param1, int param2);
static void button_win_criteria(int param1, int param2);
static void button_special_events(int param1, int param2);
static void button_price_changes(int param1, int param2);
static void button_demand_changes(int param1, int param2);
static void change_climate(int param1, int param2);
static void change_image(int forward, int param2);

static generic_button buttons_editor_attributes[] = {
    {213, 60, 195, 30, button_briefing, button_none, 0, 0},
    {213, 100, 195, 30, change_climate, button_none, 0, 0},
    {213, 140, 195, 30, button_starting_conditions, button_none, 0, 0},
    {213, 180, 195, 30, button_win_criteria, button_none, 0, 0},
    {17, 220, 185, 30, button_allowed_buildings, button_none, 0, 0},
    {213, 220, 195, 30, button_special_events, button_none, 0, 0},
    {17, 260, 185, 30, button_requests, button_none, 0, 0},
    {17, 300, 185, 30, button_enemy, button_none, 0, 0},
    {213, 300, 195, 30, button_invasions, button_none, 0, 0},
    {17, 340, 185, 30, button_price_changes, button_none, 0, 0},
    {213, 340, 195, 30, button_demand_changes, button_none, 0, 0},
};

static arrow_button image_arrows[] = {
    {19, 16, 19, 24, change_image, 0, 0, 0, 0},
    {43, 16, 21, 24, change_image, 1, 0, 0, 0},
};

static struct {
    int is_paused;
    uint8_t brief_description[MAX_BRIEF_DESCRIPTION];
    int focus_button_id;
} data;

static input_box scenario_description_input = {
    90, 16, 20, 2, FONT_NORMAL_WHITE, 1,
    data.brief_description, MAX_BRIEF_DESCRIPTION
};

static void start_brief_description_box_input(void)
{
    if (data.is_paused) {
        input_box_resume(&scenario_description_input);
    } else {
        string_copy(scenario.brief_description, data.brief_description, MAX_BRIEF_DESCRIPTION);
        input_box_start(&scenario_description_input);
    }
}

static void stop_brief_description_box_input(int paused)
{
    if (paused) {
        input_box_pause(&scenario_description_input);
    } else {
        input_box_stop(&scenario_description_input);
    }
    data.is_paused = paused;
    scenario_editor_update_brief_description(data.brief_description);
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(0, 0, 27, 25);

    arrow_buttons_draw(0, 0, image_arrows, 2);

    // Brief description
    input_box_draw(&scenario_description_input);

    // Brief description image
    button_border_draw(18, 60, 184, 144, 0);
    image_draw(image_group(GROUP_EDITOR_SCENARIO_IMAGE) + scenario.brief_description_image_id, 20, 62);

    // Briefing
    button_border_draw(213, 60, 195, 30, data.focus_button_id == 1);
    text_draw_centered(get_custom_string(TR_EDITOR_SCENARIO_BRIEFING), 213, 69, 195, FONT_NORMAL_BLACK, COLOR_BLACK);

    // Terrain set
    button_border_draw(213, 100, 195, 30, data.focus_button_id == 2);
    lang_text_draw_centered(44, 77 + scenario.climate, 213, 109, 195, FONT_NORMAL_BLACK);

    // Starting conditions
    button_border_draw(213, 140, 195, 30, data.focus_button_id == 3);
    lang_text_draw_centered(44, 88, 213, 149, 195, FONT_NORMAL_BLACK);

    // Win criteria
    button_border_draw(213, 180, 195, 30, data.focus_button_id == 4);
    lang_text_draw_centered(44, 45, 213, 189, 195, FONT_NORMAL_BLACK);

    // Buildings allowed
    button_border_draw(17, 220, 185, 30, data.focus_button_id == 5);
    lang_text_draw_centered(44, 44, 17, 229, 185, FONT_NORMAL_BLACK);

    // Special events
    button_border_draw(213, 220, 195, 30, data.focus_button_id == 6);
    lang_text_draw_centered(44, 49, 213, 229, 195, FONT_NORMAL_BLACK);

    // Requests
    button_border_draw(17, 260, 185, 30, data.focus_button_id == 7);
    if (scenario.requests[0].resource) {
        text_draw_centered(get_custom_string(TR_EDITOR_REQUEST_SCHEDULED), 17, 269, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        lang_text_draw_centered(44, 19, 17, 269, 185, FONT_NORMAL_BLACK);
    }

    // Messages

    // Enemy
    button_border_draw(17, 300, 185, 30, data.focus_button_id == 8);
    lang_text_draw_centered(37, scenario.enemy_id, 17, 309, 185, FONT_NORMAL_BLACK);

    // Invasions
    button_border_draw(213, 300, 195, 30, data.focus_button_id == 9);
    if (scenario.invasions[0].type) {
        text_draw_centered(get_custom_string(TR_EDITOR_INVASION_SCHEDULED), 213, 309, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        lang_text_draw_centered(44, 20, 213, 309, 195, FONT_NORMAL_BLACK);
    }

    // Price changes
    button_border_draw(17, 340, 185, 30, data.focus_button_id == 10);
    if (scenario.price_changes[0].resource) {
        text_draw_centered(get_custom_string(TR_EDITOR_PRICE_CHANGE_SCHEDULED), 17, 349, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(get_custom_string(TR_EDITOR_NO_PRICE_CHANGE), 17, 349, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Demand changes
    button_border_draw(213, 340, 195, 30, data.focus_button_id == 11);
    if (scenario.demand_changes[0].resource && scenario.demand_changes[0].route_id) {
        text_draw_centered(get_custom_string(TR_EDITOR_DEMAND_CHANGE_SCHEDULED), 213, 349, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(get_custom_string(TR_EDITOR_NO_DEMAND_CHANGE), 213, 349, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    const mouse *m_dialog = mouse_in_dialog(m);
    if (input_box_handle_mouse(m_dialog, &scenario_description_input) ||
        generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_editor_attributes, sizeof(buttons_editor_attributes) / sizeof(generic_button), &data.focus_button_id) ||
        arrow_buttons_handle_mouse(m_dialog, 0, 0, image_arrows, 2, 0) ||
        widget_sidebar_editor_handle_mouse_attributes(m)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        stop_brief_description_box_input(0);
        window_editor_map_show();
    }
}

static void button_briefing(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input(0);
    window_editor_briefing_show();
}

static void button_starting_conditions(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input(1);
    window_editor_starting_conditions_show();
}

static void button_requests(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input(1);
    window_editor_requests_show();
}

static void set_enemy(int enemy)
{
    scenario_editor_set_enemy(enemy);
    start_brief_description_box_input();
}

static void button_enemy(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input(1);
    window_select_list_show(screen_dialog_offset_x() - 190, screen_dialog_offset_y() + 110, 37, 20, set_enemy);
}

static void button_invasions(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input(1);
    window_editor_invasions_show();
}

static void button_allowed_buildings(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input(1);
    window_editor_allowed_buildings_show();
}

static void button_win_criteria(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input(1);
    window_editor_win_criteria_show();
}

static void button_special_events(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input(1);
    window_editor_special_events_show();
}

static void button_price_changes(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input(1);
    window_editor_price_changes_show();
}

static void button_demand_changes(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input(1);
    window_editor_demand_changes_show();
}

static void change_climate(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_cycle_climate();
    image_load_climate(scenario.climate, 1, 0);
    widget_minimap_invalidate();
    window_request_refresh();
}

static void change_image(int forward, __attribute__((unused)) int param2)
{
    scenario_editor_cycle_image(forward);
    window_request_refresh();
}

void window_editor_attributes_show(void)
{
    window_type window = {
        WINDOW_EDITOR_ATTRIBUTES,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
        0
    };
    start_brief_description_box_input();
    window_show(&window);
}
