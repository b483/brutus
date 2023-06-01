#include "attributes.h"

#include "core/image.h"
#include "core/image_group_editor.h"
#include "core/string.h"
#include "city/resource.h"
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
#include "widget/input_box.h"
#include "widget/minimap.h"
#include "widget/sidebar/editor.h"
#include "window/editor/allowed_buildings.h"
#include "window/editor/briefing.h"
#include "window/editor/demand_changes.h"
#include "window/editor/custom_messages.h"
#include "window/editor/earthquakes.h"
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
static void button_custom_messages(int param1, int param2);
static void button_earthquakes(int param1, int param2);
static void button_invasions(int param1, int param2);
static void button_allowed_buildings(int param1, int param2);
static void button_win_criteria(int param1, int param2);
static void button_special_events(int param1, int param2);
static void button_price_changes(int param1, int param2);
static void button_demand_changes(int param1, int param2);
static void change_climate(int param1, int param2);
static void change_image(int value, int param2);

static struct generic_button_t buttons_editor_attributes[] = {
    {213, 60, 195, 30, button_briefing, button_none, 0, 0},
    {213, 100, 195, 30, change_climate, button_none, 0, 0},
    {213, 140, 195, 30, button_starting_conditions, button_none, 0, 0},
    {213, 180, 195, 30, button_win_criteria, button_none, 0, 0},
    {17, 220, 185, 30, button_allowed_buildings, button_none, 0, 0},
    {213, 220, 195, 30, button_special_events, button_none, 0, 0},
    {17, 260, 185, 30, button_requests, button_none, 0, 0},
    {213, 260, 195, 30, button_custom_messages, button_none, 0, 0},
    {17, 300, 185, 30, button_earthquakes, button_none, 0, 0},
    {213, 300, 195, 30, button_invasions, button_none, 0, 0},
    {17, 340, 185, 30, button_price_changes, button_none, 0, 0},
    {213, 340, 195, 30, button_demand_changes, button_none, 0, 0},
};

static struct arrow_button_t image_arrows[] = {
    {19, 16, 19, 24, change_image, -1, 0, 0, 0},
    {43, 16, 21, 24, change_image, 1, 0, 0, 0},
};

static struct {
    char brief_description[MAX_BRIEF_DESCRIPTION];
    int focus_button_id;
} data;

static struct input_box_t scenario_description_input = {
    90, 16, 20, 2, FONT_NORMAL_WHITE, 1,
    data.brief_description, MAX_BRIEF_DESCRIPTION
};

static char *attribute_window_strings[] = {
    "Scenario briefing", // 0
    "Requests scheduled", // 1
    "No requests", // 2
    "Messages scheduled", // 3
    "No messages", // 4
    "Earthquakes scheduled", // 5
    "No earthquakes", // 6
    "Invasions scheduled", // 7
    "No invasions", // 8
    "Price changes sch.", // 9
    "No price changes", // 10
    "Demand changes sch.", // 11
    "No demand changes", // 12
};

char *climate_types_strings[] = {
    "Northern provinces",
    "Central provinces",
    "Desert provinces",
};

char *common_editor_strings[] = {
    "Year offset:", // 0
    "Month:", // 1
    "Jan year 0 invalid", // 2
    "Amount:", // 3
    "Resource:", // 4
    "Hint: 'atL' for new line, 'atP' for new paragraph (replace 'at' with the symbol - will be hidden)", // 5
};

static void stop_brief_description_box_input(void)
{
    input_box_stop(&scenario_description_input);
    if (!string_equals(scenario.brief_description, data.brief_description)) {
        string_copy(data.brief_description, scenario.brief_description, MAX_BRIEF_DESCRIPTION);
        scenario.is_saved = 0;
    }
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
    text_draw_centered(attribute_window_strings[0], 213, 69, 195, FONT_NORMAL_BLACK, COLOR_BLACK);

    // Terrain set
    button_border_draw(213, 100, 195, 30, data.focus_button_id == 2);
    text_draw_centered(climate_types_strings[scenario.climate], 213, 109, 195, FONT_NORMAL_BLACK, COLOR_BLACK);

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
        text_draw_centered(attribute_window_strings[1], 17, 269, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[2], 17, 269, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Custom messages
    button_border_draw(213, 260, 195, 30, data.focus_button_id == 8);
    if (scenario.editor_custom_messages[0].enabled) {
        text_draw_centered(attribute_window_strings[3], 213, 269, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[4], 213, 269, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Earthquakes
    button_border_draw(17, 300, 185, 30, data.focus_button_id == 9);
    if (scenario.earthquakes[0].state) {
        text_draw_centered(attribute_window_strings[5], 17, 309, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[6], 17, 309, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Invasions
    button_border_draw(213, 300, 195, 30, data.focus_button_id == 10);
    if (scenario.invasions[0].type) {
        text_draw_centered(attribute_window_strings[7], 213, 309, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[8], 213, 309, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Price changes
    button_border_draw(17, 340, 185, 30, data.focus_button_id == 11);
    if (scenario.price_changes[0].resource) {
        text_draw_centered(attribute_window_strings[9], 17, 349, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[10], 17, 349, 185, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    // Demand changes
    button_border_draw(213, 340, 195, 30, data.focus_button_id == 12);
    if (scenario.demand_changes[0].resource && scenario.demand_changes[0].trade_city_id) {
        text_draw_centered(attribute_window_strings[11], 213, 349, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    } else {
        text_draw_centered(attribute_window_strings[12], 213, 349, 195, FONT_NORMAL_BLACK, COLOR_BLACK);
    }

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_editor_attributes, sizeof(buttons_editor_attributes) / sizeof(struct generic_button_t), &data.focus_button_id) ||
        arrow_buttons_handle_mouse(m_dialog, 0, 0, image_arrows, 2, 0) ||
        widget_sidebar_editor_handle_mouse_attributes(m)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        stop_brief_description_box_input();
        window_editor_map_show();
    }
}

static void button_briefing(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    window_editor_briefing_show();
}

static void button_starting_conditions(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    window_editor_starting_conditions_show();
}

static void button_custom_messages(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    window_editor_custom_messages_show();
}

static void button_requests(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    window_editor_requests_show();
}

static void button_earthquakes(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    window_editor_earthquakes_show();
}

static void button_invasions(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    window_editor_invasions_show();
}

static void button_allowed_buildings(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    window_editor_allowed_buildings_show();
}

static void button_win_criteria(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    window_editor_win_criteria_show();
}

static void button_special_events(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    window_editor_special_events_show();
}

static void button_price_changes(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    window_editor_price_changes_show();
}

static void button_demand_changes(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    stop_brief_description_box_input();
    window_editor_demand_changes_show();
}

static void change_climate(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.climate++;
    if (scenario.climate > 2) {
        scenario.climate = 0;
    }
    scenario.is_saved = 0;
    image_load_climate(scenario.climate, 1, 0);
    widget_minimap_invalidate();
    window_request_refresh();
}

static void change_image(int value, __attribute__((unused)) int param2)
{
    if (scenario.brief_description_image_id == 15 && value == 1) {
        scenario.brief_description_image_id = 0;
    } else if (scenario.brief_description_image_id == 0 && value == -1) {
        scenario.brief_description_image_id = 15;
    } else {
        scenario.brief_description_image_id += value;
    }
    scenario.is_saved = 0;
    window_request_refresh();
}

void window_editor_attributes_show(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_ATTRIBUTES,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
    };
    string_copy(scenario.brief_description, data.brief_description, MAX_BRIEF_DESCRIPTION);
    input_box_start(&scenario_description_input);
    window_show(&window);
}
