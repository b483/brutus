#include "donate_to_city.h"

#include "city/emperor.h"
#include "core/calc.h"
#include "game/resource.h"
#include "graphics/arrow_button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "window/advisors.h"

static void button_set_amount(int amount_id, int param2);
static void button_donate(int param1, int param2);
static void arrow_button_amount(int is_down, int param2);

static generic_button buttons_donate_to_city[] = {
    {128, 216, 64, 20, button_set_amount, button_none, 0, 0},
    {208, 216, 64, 20, button_set_amount, button_none, 1, 0},
    {288, 216, 64, 20, button_set_amount, button_none, 2, 0},
    {368, 216, 64, 20, button_set_amount, button_none, 3, 0},
    {448, 216, 64, 20, button_set_amount, button_none, 4, 0},
    {368, 243, 144, 20, button_donate, button_none, 0, 0},
};

static arrow_button arrow_buttons_donate_to_city[] = {
    {240, 242, 17, 24, arrow_button_amount, 1, 0, 0, 0},
    {264, 242, 15, 24, arrow_button_amount, 0, 0, 0, 0},
};

static struct {
    int focus_button_id;
    int focus_arrow_button_id;
} data;

static void draw_background(void)
{
    window_advisors_draw_dialog_background();

    graphics_in_dialog();

    outer_panel_draw(64, 160, 32, 9);
    image_draw(image_group(GROUP_RESOURCE_ICONS) + RESOURCE_DENARII, 80, 176);
    lang_text_draw_centered(52, 16, 112, 176, 448, FONT_LARGE_BLACK);

    inner_panel_draw(112, 208, 26, 4);

    text_draw_number_centered(0, 124, 221, 64, FONT_NORMAL_WHITE);
    text_draw_number_centered(500, 204, 221, 64, FONT_NORMAL_WHITE);
    text_draw_number_centered(2000, 284, 221, 64, FONT_NORMAL_WHITE);
    text_draw_number_centered(5000, 364, 221, 64, FONT_NORMAL_WHITE);
    lang_text_draw_centered(52, 19, 444, 221, 64, FONT_NORMAL_WHITE);

    int width = lang_text_draw(52, 17, 128, 248, FONT_NORMAL_WHITE);
    width += 129;
    arrow_buttons_donate_to_city[0].x_offset = width;
    arrow_buttons_donate_to_city[1].x_offset = arrow_buttons_donate_to_city[0].x_offset + arrow_buttons_donate_to_city[0].size;

    text_draw_number(city_emperor_donate_amount(), '@', " ", width + 71, 248, FONT_NORMAL_GREEN);

    lang_text_draw_centered(52, 18, 368, 248, 144, FONT_NORMAL_GREEN);

    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    button_border_draw(128, 216, 64, 20, data.focus_button_id == 1);
    button_border_draw(208, 216, 64, 20, data.focus_button_id == 2);
    button_border_draw(288, 216, 64, 20, data.focus_button_id == 3);
    button_border_draw(368, 216, 64, 20, data.focus_button_id == 4);
    button_border_draw(448, 216, 64, 20, data.focus_button_id == 5);

    button_border_draw(368, 243, 144, 20, data.focus_button_id == 6);

    arrow_buttons_draw(0, 0, arrow_buttons_donate_to_city, sizeof(arrow_buttons_donate_to_city) / sizeof(arrow_button));

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    data.focus_arrow_button_id = 0;
    const mouse *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_donate_to_city, sizeof(buttons_donate_to_city) / sizeof(generic_button), &data.focus_button_id)) {
        return;
    }
    if (arrow_buttons_handle_mouse(m_dialog, 0, 0, arrow_buttons_donate_to_city, sizeof(arrow_buttons_donate_to_city) / sizeof(arrow_button), &data.focus_arrow_button_id)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        window_advisors_show(ADVISOR_IMPERIAL);
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
    city_emperor_set_donation_amount(amount);
    window_invalidate();
}

static void button_donate(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    city_emperor_donate_savings_to_city();
    window_advisors_show(ADVISOR_IMPERIAL);
}

static void arrow_button_amount(int is_down, __attribute__((unused)) int param2)
{
    city_emperor_change_donation_amount(is_down ? -10 : 10);
    window_invalidate();
}

static void get_tooltip(tooltip_context *c)
{
    if (!data.focus_button_id && !data.focus_arrow_button_id) {
        return;
    }
    c->type = TOOLTIP_BUTTON;
    if (data.focus_button_id == 6) {
        c->text_id = 99;
    } else if (data.focus_button_id) {
        c->text_id = 100;
    } else if (data.focus_arrow_button_id) {
        c->text_id = 101;
    }
}

void window_donate_to_city_show(void)
{
    window_type window = {
        WINDOW_DONATE_TO_CITY,
        draw_background,
        draw_foreground,
        handle_input,
        get_tooltip
    };
    city_emperor_init_donation_amount();
    window_show(&window);
}
