#include "donate_to_city.h"

#include "city/data_private.h"
#include "core/calc.h"
#include "graphics/arrow_button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "window/advisors.h"

static void button_set_amount(int amount_id, int param2);
static void button_donate(int param1, int param2);
static void arrow_button_amount(int is_down, int param2);

static struct generic_button_t buttons_donate_to_city[] = {
    {144, 230, 64, 20, button_set_amount, button_none, 0, 0},
    {144, 257, 64, 20, button_set_amount, button_none, 1, 0},
    {224, 230, 64, 20, button_set_amount, button_none, 2, 0},
    {224, 257, 64, 20, button_set_amount, button_none, 3, 0},
    {304, 257, 64, 20, button_set_amount, button_none, 4, 0},
    {384, 257, 120, 20, button_donate, button_none, 0, 0},
};

static struct arrow_button_t arrow_buttons_donate_to_city[] = {
    {455, 230, 17, 24, arrow_button_amount, -10, 0, 0, 0},
    {479, 230, 15, 24, arrow_button_amount, 10, 0, 0, 0},
};

static struct {
    int focus_button_id;
    int focus_arrow_button_id;
} data;

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(108, 172, 27, 8);

    // Coin image
    image_draw(COIN_IMAGE_ID, 124, 188);

    // Give money to the city
    lang_text_draw_centered(52, 16, 108, 188, 432, FONT_LARGE_BLACK);

    inner_panel_draw(124, 220, 25, 4);

    // 0
    button_border_draw(144, 230, 64, 20, data.focus_button_id == 1);
    text_draw_number_centered(0, 142, 235, 64, FONT_NORMAL_WHITE);
    // 500
    button_border_draw(144, 257, 64, 20, data.focus_button_id == 2);
    text_draw_number_centered(500, 142, 262, 64, FONT_NORMAL_WHITE);
    // 2000
    button_border_draw(224, 230, 64, 20, data.focus_button_id == 3);
    text_draw_number_centered(2000, 222, 235, 64, FONT_NORMAL_WHITE);
    // 5000
    button_border_draw(224, 257, 64, 20, data.focus_button_id == 4);
    text_draw_number_centered(5000, 222, 262, 64, FONT_NORMAL_WHITE);
    // All
    button_border_draw(304, 257, 64, 20, data.focus_button_id == 5);
    lang_text_draw_centered(52, 19, 304, 262, 64, FONT_NORMAL_WHITE);

    // Donation is
    lang_text_draw(52, 17, 304, 235, FONT_NORMAL_WHITE);
    text_draw_number(city_data.emperor.donate_amount, '@', " ", 394, 235, FONT_NORMAL_GREEN);

    arrow_buttons_draw(0, 0, arrow_buttons_donate_to_city, sizeof(arrow_buttons_donate_to_city) / sizeof(struct arrow_button_t));

    // Give money
    button_border_draw(384, 257, 120, 20, data.focus_button_id == 6);
    lang_text_draw_centered(52, 18, 384, 262, 120, FONT_NORMAL_GREEN);

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }
    data.focus_arrow_button_id = 0;
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_donate_to_city, sizeof(buttons_donate_to_city) / sizeof(struct generic_button_t), &data.focus_button_id)) {
        return;
    }
    if (arrow_buttons_handle_mouse(m_dialog, 0, 0, arrow_buttons_donate_to_city, sizeof(arrow_buttons_donate_to_city) / sizeof(struct arrow_button_t), &data.focus_arrow_button_id)) {
        return;
    }
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 108 || m_dialog->y < 172 || m_dialog->x > 540 || m_dialog->y > 300)) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }
}

static void button_set_amount(int amount_id, __attribute__((unused)) int param2)
{
    int amount;
    switch (amount_id) {
        case 0: amount = 0; break;
        case 1: amount = 500; break;
        case 2: amount = 2000; break;
        case 3: amount = 5000; break;
        case 4: amount = 1000000; break;
        default: return;
    }
    city_data.emperor.donate_amount = calc_bound(amount, 0, city_data.emperor.personal_savings);
    window_invalidate();
}

static void button_donate(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    city_finance_process_donation(city_data.emperor.donate_amount);
    city_data.emperor.personal_savings -= city_data.emperor.donate_amount;
    city_finance_calculate_totals();
    window_advisors_show(ADVISOR_IMPERIAL);
}

static void arrow_button_amount(int value, __attribute__((unused)) int param2)
{
    city_data.emperor.donate_amount = calc_bound(city_data.emperor.donate_amount + value, 0, city_data.emperor.personal_savings);
    window_invalidate();
}

void window_donate_to_city_show(void)
{
    struct window_type_t window = {
        WINDOW_DONATE_TO_CITY,
        window_advisors_draw_dialog_background,
        draw_foreground,
        handle_input,
    };
    city_data.emperor.donate_amount = 0;
    window_show(&window);
}
