#include "gift_to_emperor.h"

#include "city/data_private.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "window/advisors.h"

static void button_send_gift(int gift_id, int param2);

static generic_button buttons_gift_to_emperor[] = {
    {210, 180, 325, 15, button_send_gift, button_none, 0, 0},
    {210, 200, 325, 15, button_send_gift, button_none, 1, 0},
    {210, 220, 325, 15, button_send_gift, button_none, 2, 0},
};

static int focus_button_id;

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(84, 108, 30, 12);

    // Coin image
    image_draw(image_group(GROUP_RESOURCE_ICONS) + RESOURCE_DENARII, 100, 124);

    // Send the Emperor a gift
    lang_text_draw_centered(52, 69, 84, 124, 480, FONT_LARGE_BLACK);

    inner_panel_draw(100, 165, 28, 5);

    // Modest gift
    if (city_data.emperor.gifts[GIFT_MODEST].cost <= city_data.emperor.personal_savings) {
        lang_text_draw(52, 63, 120, 180, FONT_NORMAL_WHITE);
        font_t font = focus_button_id == 1 ? FONT_NORMAL_RED : FONT_NORMAL_WHITE;
        lang_text_draw(52, 51 + city_data.emperor.gifts[GIFT_MODEST].id, 210, 180, font);
        text_draw_money(city_data.emperor.gifts[GIFT_MODEST].cost, 460, 180, font);
    } else {
        lang_text_draw_multiline(52, 70, 110, 180, 448, FONT_NORMAL_WHITE);
    }
    // Generous gift
    if (city_data.emperor.gifts[GIFT_GENEROUS].cost <= city_data.emperor.personal_savings) {
        lang_text_draw(52, 64, 120, 200, FONT_NORMAL_WHITE);
        font_t font = focus_button_id == 2 ? FONT_NORMAL_RED : FONT_NORMAL_WHITE;
        lang_text_draw(52, 55 + city_data.emperor.gifts[GIFT_GENEROUS].id, 210, 200, font);
        text_draw_money(city_data.emperor.gifts[GIFT_GENEROUS].cost, 460, 200, font);
    }
    // Lavish gift
    if (city_data.emperor.gifts[GIFT_LAVISH].cost <= city_data.emperor.personal_savings) {
        lang_text_draw(52, 65, 120, 220, FONT_NORMAL_WHITE);
        font_t font = focus_button_id == 3 ? FONT_NORMAL_RED : FONT_NORMAL_WHITE;
        lang_text_draw(52, 59 + city_data.emperor.gifts[GIFT_LAVISH].id, 210, 220, font);
        text_draw_money(city_data.emperor.gifts[GIFT_LAVISH].cost, 460, 220, font);
    }

    // Time since last gift
    int width = lang_text_draw(52, 50, 200, 260, FONT_NORMAL_BLACK);
    lang_text_draw_amount(8, 4, city_data.emperor.months_since_gift, width + 200, 260, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }

    const mouse *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_gift_to_emperor, sizeof(buttons_gift_to_emperor) / sizeof(generic_button), &focus_button_id)) {
        return;
    }
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 84 || m_dialog->y < 108 || m_dialog->x > 564 || m_dialog->y > 300)) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }
}

static void button_send_gift(int gift_id, __attribute__((unused)) int param2)
{
    if (city_data.emperor.gifts[gift_id].cost <= city_data.emperor.personal_savings) {
        city_data.emperor.selected_gift_size = gift_id;
        city_emperor_send_gift();
        window_advisors_show(ADVISOR_IMPERIAL);
    }
}

void window_gift_to_emperor_show(void)
{
    window_type window = {
        WINDOW_GIFT_TO_EMPEROR,
        window_advisors_draw_dialog_background,
        draw_foreground,
        handle_input,
        0
    };

    // calculate gift costs
    city_data.emperor.gifts[GIFT_MODEST].cost = city_data.emperor.personal_savings / 8 + 20;
    city_data.emperor.gifts[GIFT_GENEROUS].cost = city_data.emperor.personal_savings / 4 + 50;
    city_data.emperor.gifts[GIFT_LAVISH].cost = city_data.emperor.personal_savings / 2 + 100;

    window_show(&window);
}
