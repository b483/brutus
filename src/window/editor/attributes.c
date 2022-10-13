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
#include "input/input.h"
#include "scenario/data.h"
#include "scenario/editor.h"
#include "scenario/property.h"
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

#define BRIEF_DESC_LENGTH 32

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
    {32, 76, 170, 30, button_briefing, button_none, 0, 0},
    {212, 76, 250, 30, button_starting_conditions, button_none, 0, 0},
    {212, 116, 250, 30, change_climate, button_none, 0, 0},
    {212, 156, 250, 30, button_requests, button_none, 0, 0},
    {212, 196, 250, 30, button_enemy, button_none, 0, 0},
    {212, 236, 250, 30, button_invasions, button_none, 0, 0},
    {212, 276, 250, 30, button_allowed_buildings, button_none, 0, 0},
    {212, 316, 250, 30, button_win_criteria, button_none, 0, 0},
    {212, 356, 250, 30, button_special_events, button_none, 0, 0},
    {212, 396, 250, 30, button_price_changes, button_none, 0, 0},
    {212, 436, 250, 30, button_demand_changes, button_none, 0, 0},
};

static arrow_button image_arrows[] = {
    {20, 424, 19, 24, change_image, 0, 0, 0, 0},
    {44, 424, 21, 24, change_image, 1, 0, 0, 0},
};

static struct {
    int is_paused;
    uint8_t brief_description[BRIEF_DESC_LENGTH];
    int focus_button_id;
} data;

static input_box scenario_description_input = {
    92, 40, 19, 2, FONT_NORMAL_WHITE, 1,
    data.brief_description, BRIEF_DESC_LENGTH
};

static void start(void)
{
    if (data.is_paused) {
        input_box_resume(&scenario_description_input);
    } else {
        string_copy(scenario_brief_description(), data.brief_description, BRIEF_DESC_LENGTH);
        input_box_start(&scenario_description_input);
    }
}

static void stop(int paused)
{
    if (paused) {
        input_box_pause(&scenario_description_input);
    } else {
        input_box_stop(&scenario_description_input);
    }
    data.is_paused = paused;
    scenario_editor_update_brief_description(data.brief_description);
}

static void draw_background(void)
{
    window_editor_map_draw_all();

    graphics_in_dialog();

    outer_panel_draw(0, 28, 30, 28);

    button_border_draw(18, 278, 184, 144, 0);
    image_draw(image_group(GROUP_EDITOR_SCENARIO_IMAGE) + scenario_image_id(), 20, 280);

    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    // Brief description
    input_box_draw(&scenario_description_input);

    // Briefing
    button_border_draw(32, 76, 170, 30, data.focus_button_id == 1);
    text_draw_centered(get_custom_string(TR_EDITOR_MAP_BRIEFING), 32, 85, 170, FONT_NORMAL_BLACK, COLOR_BLACK);

    // Starting conditions
    button_border_draw(212, 76, 250, 30, data.focus_button_id == 2);
    lang_text_draw_centered(44, 88, 212, 85, 250, FONT_NORMAL_BLACK);

    // Terrain set
    lang_text_draw(44, 76, 32, 125, FONT_NORMAL_BLACK);
    button_border_draw(212, 116, 250, 30, data.focus_button_id == 3);
    lang_text_draw_centered(44, 77 + scenario_property_climate(), 212, 125, 250, FONT_NORMAL_BLACK);

    // Requests
    lang_text_draw(44, 40, 32, 165, FONT_NORMAL_BLACK);
    button_border_draw(212, 156, 250, 30, data.focus_button_id == 4);

    if (scenario.requests[0].resource) {
        text_draw_centered(get_custom_string(TR_EDITOR_REQUEST_SCHEDULED), 212, 165, 250, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        lang_text_draw_centered(44, 19, 212, 165, 250, FONT_NORMAL_BLACK);
    }

    // Enemy
    lang_text_draw(44, 41, 32, 205, FONT_NORMAL_BLACK);
    button_border_draw(212, 196, 250, 30, data.focus_button_id == 5);
    lang_text_draw_centered(37, scenario_property_enemy(), 212, 205, 250, FONT_NORMAL_BLACK);

    lang_text_draw(44, 42, 32, 245, FONT_NORMAL_BLACK);
    button_border_draw(212, 236, 250, 30, data.focus_button_id == 6);

    if (scenario.invasions[0].type) {
        text_draw_centered(get_custom_string(TR_EDITOR_INVASION_SCHEDULED), 212, 245, 250, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        lang_text_draw_centered(44, 20, 212, 245, 250, FONT_NORMAL_BLACK);
    }

    // Buildings allowed
    button_border_draw(212, 276, 250, 30, data.focus_button_id == 7);
    lang_text_draw_centered(44, 44, 212, 285, 250, FONT_NORMAL_BLACK);

    // Win criteria
    button_border_draw(212, 316, 250, 30, data.focus_button_id == 8);
    lang_text_draw_centered(44, 45, 212, 325, 250, FONT_NORMAL_BLACK);

    // Special events
    button_border_draw(212, 356, 250, 30, data.focus_button_id == 9);
    lang_text_draw_centered(44, 49, 212, 365, 250, FONT_NORMAL_BLACK);

    // Price changes
    button_border_draw(212, 396, 250, 30, data.focus_button_id == 10);
    if (scenario.price_changes[0].resource) {
        text_draw_centered(get_custom_string(TR_EDITOR_PRICE_CHANGE_SCHEDULED), 212, 405, 250, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(get_custom_string(TR_EDITOR_NO_PRICE_CHANGE), 212, 405, 250, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Demand changes
    button_border_draw(212, 436, 250, 30, data.focus_button_id == 11);
    lang_text_draw_centered(44, 94, 212, 445, 250, FONT_NORMAL_BLACK);

    arrow_buttons_draw(0, 0, image_arrows, 2);

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
    if (input_go_back_requested(m, h)) {
        stop(0);
        window_editor_map_show();
    }
}

static void button_briefing(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop(1);
    window_editor_briefing_show();
}

static void button_starting_conditions(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop(1);
    window_editor_starting_conditions_show();
}

static void button_requests(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop(1);
    window_editor_requests_show();
}

static void set_enemy(int enemy)
{
    scenario_editor_set_enemy(enemy);
    start();
}

static void button_enemy(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop(1);
    window_select_list_show(screen_dialog_offset_x() + 12, screen_dialog_offset_y() + 40, 37, 20, set_enemy);
}

static void button_invasions(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop(1);
    window_editor_invasions_show();
}

static void button_allowed_buildings(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop(1);
    window_editor_allowed_buildings_show();
}

static void button_win_criteria(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop(1);
    window_editor_win_criteria_show();
}

static void button_special_events(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop(1);
    window_editor_special_events_show();
}

static void button_price_changes(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop(1);
    window_editor_price_changes_show();
}

static void button_demand_changes(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop(1);
    window_editor_demand_changes_show();
}

static void change_climate(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario_editor_cycle_climate();
    image_load_climate(scenario_property_climate(), 1, 0);
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
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    start();
    window_show(&window);
}
