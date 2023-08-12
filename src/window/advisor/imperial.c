#include "imperial.h"

#include "city/data.h"
#include "city/finance.h"
#include "city/military.h"
#include "city/ratings.h"
#include "city/resource.h"
#include "empire/object.h"
#include "figure/figure.h"
#include "figure/formation_legion.h"
#include "graphics/generic_button.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/scrollbar.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/scenario.h"
#include "window/donate_to_city.h"
#include "window/empire.h"
#include "window/gift_to_emperor.h"
#include "window/popup_dialog.h"
#include "window/set_salary.h"

#define ADVISOR_HEIGHT 27

#define MAX_REQUESTS_SHOWN 5

enum {
    STATUS_NOT_ENOUGH_RESOURCES = -1,
    STATUS_CONFIRM_SEND_LEGIONS = -2,
    STATUS_NO_LEGIONS_SELECTED = -3,
    STATUS_NO_LEGIONS_AVAILABLE = -4,
};

static void on_scroll(void);

static void button_donate_to_city(int param1, int param2);
static void button_set_salary(int param1, int param2);
static void button_gift_to_emperor(int param1, int param2);
static void button_request(int index, int param2);

static struct scrollbar_type_t scrollbar = { 591, 90, 222, on_scroll, 0, 0, 0, 0, 0, 0 };

static struct generic_button_t imperial_buttons[] = {
    {38, 96, 550, 40, button_request, button_none, 0, 0},
    {38, 138, 550, 40, button_request, button_none, 1, 0},
    {38, 180, 550, 40, button_request, button_none, 2, 0},
    {38, 222, 550, 40, button_request, button_none, 3, 0},
    {38, 264, 550, 40, button_request, button_none, 4, 0},
    {312, 341, 250, 20, button_gift_to_emperor, button_none, 0, 0},
    {312, 367, 250, 20, button_donate_to_city, button_none, 0, 0},
    {62, 393, 500, 20, button_set_salary, button_none, 0, 0},
};

static int goods_requests_to_draw;
static int selected_request_id;
static int focus_button_id;

static void init(void)
{
    goods_requests_to_draw = 0;
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (scenario.requests[i].resource && scenario.requests[i].visible) {
            goods_requests_to_draw++;
        }
    }
    scrollbar_init(&scrollbar, 0, goods_requests_to_draw - MAX_REQUESTS_SHOWN);
}

static void draw_requests(void)
{
    int request_index = 0;

    // draw distant battle army request
    if (city_data.distant_battle.months_until_battle
        && !city_data.distant_battle.roman_months_to_travel_forth) {
        // can send to distant battle
        button_border_draw(38, 96, 550, 40, 0);
        image_draw(resource_images[RESOURCE_WEAPONS].icon_img_id, 50, 106);
        int width = lang_text_draw(52, 72, 80, 102, FONT_NORMAL_WHITE);
        lang_text_draw(21, empire_objects[city_data.distant_battle.city].city_name_id,
            80 + width, 102, FONT_NORMAL_WHITE);
        int strength_text_id;
        if (city_data.distant_battle.enemy_strength < 46) {
            strength_text_id = 73;
        } else if (city_data.distant_battle.enemy_strength < 89) {
            strength_text_id = 74;
        } else {
            strength_text_id = 75;
        }
        width = lang_text_draw(52, strength_text_id, 80, 120, FONT_NORMAL_WHITE);
        lang_text_draw_amount(8, 4, city_data.distant_battle.months_until_battle, 80 + width, 120, FONT_NORMAL_WHITE);
        request_index = 1;
    }

    if (!goods_requests_to_draw) {
        return;
    }

    // scroll max position depends on max goods drawn (MAX_REQUESTS_SHOWN, or MAX_REQUESTS_SHOWN - 1 when also drawing distant battle)
    scrollbar_update_max(&scrollbar, goods_requests_to_draw + request_index - MAX_REQUESTS_SHOWN);

    // min(scrollbar.scroll_position, max(goods_requests_to_draw + request_index - MAX_REQUESTS_SHOWN, 0)) without imports
    int scroll_adjusted_index;
    if (goods_requests_to_draw + request_index <= MAX_REQUESTS_SHOWN) {
        scroll_adjusted_index = 0;
    } else {
        scroll_adjusted_index = scrollbar.scroll_position < goods_requests_to_draw + request_index - MAX_REQUESTS_SHOWN ? scrollbar.scroll_position : goods_requests_to_draw + request_index - MAX_REQUESTS_SHOWN;
    }

    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (request_index >= MAX_REQUESTS_SHOWN) {
            return;
        }
        if (scenario.requests[i].resource && scenario.requests[i].visible && i >= scroll_adjusted_index) {
            button_border_draw(38, 96 + 42 * request_index, 550, 40, 0);
            text_draw_number(scenario.requests[i].amount, '@', " ", 40, 102 + 42 * request_index, FONT_NORMAL_WHITE);
            image_draw(resource_images[scenario.requests[i].resource].icon_img_id + resource_image_offset(scenario.requests[i].resource, RESOURCE_IMAGE_ICON), 110, 100 + 42 * request_index);
            lang_text_draw(23, scenario.requests[i].resource, 150, 102 + 42 * request_index, FONT_NORMAL_WHITE);

            int width = lang_text_draw_amount(8, 4, scenario.requests[i].months_to_comply, 310, 102 + 42 * request_index, FONT_NORMAL_WHITE);
            lang_text_draw(12, 2, 310 + width, 102 + 42 * request_index, FONT_NORMAL_WHITE);

            if (scenario.requests[i].resource == RESOURCE_DENARII) {
                // request for money
                width = text_draw_number(city_data.finance.treasury, '@', " ", 40, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                width += lang_text_draw(52, 44, 40 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                if (city_data.finance.treasury < scenario.requests[i].amount) {
                    lang_text_draw(52, 48, 105 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                } else {
                    lang_text_draw(52, 47, 80 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                }
            } else {
                // normal goods request
                int amount_stored = city_data.resource.stored_in_warehouses[scenario.requests[i].resource];
                width = text_draw_number(amount_stored, '@', " ", 40, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                width += lang_text_draw(52, 43, 40 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                if (amount_stored < scenario.requests[i].amount) {
                    lang_text_draw(52, 48, 165 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                } else {
                    lang_text_draw(52, 47, 127 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                }
            }
            request_index++;
        }
    }

    if (!request_index) {
        lang_text_draw_multiline(52, 21, 64, 160, 512, FONT_NORMAL_WHITE);
    }
}

static int draw_background(void)
{
    outer_panel_draw(0, 0, 40, ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 2, 10, 10);

    text_draw(scenario_settings.player_name, 60, 12, FONT_LARGE_BLACK, 0);

    int width = lang_text_draw(52, 0, 60, 44, FONT_NORMAL_BLACK);
    text_draw_number(city_data.ratings.favor, '@', " ", 60 + width, 44, FONT_NORMAL_BLACK);

    lang_text_draw_multiline(52, city_data.ratings.favor / 5 + 22, 60, 60, 544, FONT_NORMAL_BLACK);

    inner_panel_draw(32, 90, 35, 14);

    draw_requests();

    return ADVISOR_HEIGHT;
}

static int get_request_status(int index)
{
    int num_requests = 0;
    if (city_data.distant_battle.months_until_battle && !city_data.distant_battle.roman_months_to_travel_forth) {
        num_requests = 1;
        if (index == 0) {
            int legions_selected_emp_service = 0;
            for (int i = 0; i < MAX_LEGIONS; i++) {
                if (legion_formations[i].in_use && legion_formations[i].empire_service && legion_formations[i].num_figures) {
                    legions_selected_emp_service = 1;
                    break;
                }
            }
            if (!city_data.military.total_legions) {
                return STATUS_NO_LEGIONS_AVAILABLE;
            } else if (!legions_selected_emp_service) {
                return STATUS_NO_LEGIONS_SELECTED;
            } else {
                return STATUS_CONFIRM_SEND_LEGIONS;
            }
        }
    }
    int index_offset = index - num_requests;
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (scenario.requests[i].resource && scenario.requests[i].visible &&
            scenario.requests[i].state <= 1) {
            if (index_offset == 0) {
                if (scenario.requests[i].resource == RESOURCE_DENARII) {
                    if (city_data.finance.treasury <= scenario.requests[i].amount) {
                        return STATUS_NOT_ENOUGH_RESOURCES;
                    }
                } else {
                    if (city_data.resource.stored_in_warehouses[scenario.requests[i].resource] < scenario.requests[i].amount) {
                        return STATUS_NOT_ENOUGH_RESOURCES;
                    }
                }
                return i + 1;
            }
            index_offset--;
        }
    }
    return 0;
}

static void draw_foreground(void)
{
    inner_panel_draw(56, 324, 32, 6);

    // Request buttons
    if (get_request_status(0)) {
        button_border_draw(38, 96, 550, 40, focus_button_id == 1);
    }
    if (get_request_status(1)) {
        button_border_draw(38, 138, 550, 40, focus_button_id == 2);
    }
    if (get_request_status(2)) {
        button_border_draw(38, 180, 550, 40, focus_button_id == 3);
    }
    if (get_request_status(3)) {
        button_border_draw(38, 222, 550, 40, focus_button_id == 4);
    }
    if (get_request_status(4)) {
        button_border_draw(38, 264, 550, 40, focus_button_id == 5);
    }

    lang_text_draw(32, city_data.emperor.player_rank, 64, 338, FONT_LARGE_BROWN);

    int width = lang_text_draw(52, 1, 64, 372, FONT_NORMAL_WHITE);
    text_draw_money(city_data.emperor.personal_savings, 72 + width, 372, FONT_NORMAL_WHITE);

    // Send a gift
    button_border_draw(312, 341, 250, 20, focus_button_id == 6);
    lang_text_draw_centered(52, 49, 312, 346, 250, FONT_NORMAL_WHITE);

    // Give to city
    button_border_draw(312, 367, 250, 20, focus_button_id == 7);
    lang_text_draw_centered(52, 2, 312, 372, 250, FONT_NORMAL_WHITE);

    // Set salary
    button_border_draw(62, 393, 500, 20, focus_button_id == 8);
    width = lang_text_draw(52, city_data.emperor.salary_rank + 4, 112, 398, FONT_NORMAL_WHITE);
    width += text_draw_number(city_data.emperor.salary_amount, '@', " ", 112 + width, 398, FONT_NORMAL_WHITE);
    lang_text_draw(52, 3, 112 + width, 398, FONT_NORMAL_WHITE);

    scrollbar_draw(&scrollbar);
}

static int handle_mouse(const struct mouse_t *m)
{
    if (scrollbar_handle_mouse(&scrollbar, m)) {
        return 1;
    }
    if (generic_buttons_handle_mouse(m, 0, 0, imperial_buttons, sizeof(imperial_buttons) / sizeof(struct generic_button_t), &focus_button_id)) {
        return 1;
    }
    return 0;
}

static void on_scroll(void)
{
    window_invalidate();
}

static void button_donate_to_city(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_donate_to_city_show();
}

static void button_set_salary(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_set_salary_show();
}

static void button_gift_to_emperor(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_gift_to_emperor_show();
}

static void confirm_nothing(void)
{}

static void confirm_send_troops(void)
{
    int legions_sent = 0;
    int roman_strength = 0;
    for (int i = 0; i < MAX_LEGIONS; i++) {
        if (legion_formations[i].in_use && legion_formations[i].empire_service && legion_formations[i].num_figures) {
            struct formation_t *m = &legion_formations[i];
            m->in_distant_battle = 1;
            for (int fig = 0; fig < m->num_figures; fig++) {
                if (m->figures[fig] > 0) {
                    struct figure_t *f = &figures[m->figures[fig]];
                    if (figure_is_alive(f)) {
                        f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_DISTANT_BATTLE;
                    }
                }
            }
            int strength_factor;
            if (m->has_military_training) {
                strength_factor = m->figure_type == FIGURE_FORT_LEGIONARY ? 3 : 2;
            } else {
                strength_factor = m->figure_type == FIGURE_FORT_LEGIONARY ? 2 : 1;
            }
            roman_strength += strength_factor * m->num_figures;
            legions_sent++;
        }
    }
    if (legions_sent > 0) {
        city_data.distant_battle.roman_months_to_travel_forth = scenario.empire.distant_battle_roman_travel_months;
        city_data.distant_battle.roman_strength = roman_strength;
    }
    window_empire_show();
}

static void confirm_send_goods(void)
{
    dispatch_imperial_request(selected_request_id);
    goods_requests_to_draw--;
    scrollbar_update_max(&scrollbar, scrollbar.max_scroll_position - 1);
}

static void button_request(int index, __attribute__((unused)) int param2)
{
    int status = get_request_status(scrollbar.scroll_position + index);
    if (status) {
        switch (status) {
            case STATUS_NO_LEGIONS_AVAILABLE:
                window_popup_dialog_show(POPUP_DIALOG_NO_LEGIONS_AVAILABLE, confirm_nothing, 0);
                break;
            case STATUS_NO_LEGIONS_SELECTED:
                window_popup_dialog_show(POPUP_DIALOG_NO_LEGIONS_SELECTED, confirm_nothing, 0);
                break;
            case STATUS_CONFIRM_SEND_LEGIONS:
                window_popup_dialog_show(POPUP_DIALOG_SEND_TROOPS, confirm_send_troops, 2);
                break;
            case STATUS_NOT_ENOUGH_RESOURCES:
                window_popup_dialog_show(POPUP_DIALOG_NOT_ENOUGH_GOODS, confirm_nothing, 0);
                break;
            default:
                selected_request_id = status - 1;
                window_popup_dialog_show(POPUP_DIALOG_SEND_GOODS, confirm_send_goods, 2);
                break;
        }
    }
}

struct advisor_window_type_t *window_advisor_imperial(void)
{
    static struct advisor_window_type_t window = {
        draw_background,
        draw_foreground,
        handle_mouse,
    };
    init();
    return &window;
}
