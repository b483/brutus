#include "gift_to_emperor.h"

#include "city/emperor.h"
#include "game/resource.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "window/advisors.h"

static void button_send_gift(int gift_id, int param2);

static generic_button buttons_gift_to_emperor[] = {
    {208, 213, 300, 20, button_send_gift, button_none, 1, 0},
    {208, 233, 300, 20, button_send_gift, button_none, 2, 0},
    {208, 253, 300, 20, button_send_gift, button_none, 3, 0},
};

static int focus_button_id;

static void init(void)
{
    city_emperor_init_selected_gift();
}

static void draw_background(void)
{
    window_advisors_draw_dialog_background();

    graphics_in_dialog();

    outer_panel_draw(96, 144, 30, 12);
    image_draw(image_group(GROUP_RESOURCE_ICONS) + RESOURCE_DENARII, 112, 160);
    lang_text_draw_centered(52, 69, 144, 160, 416, FONT_LARGE_BLACK);

    int width = lang_text_draw(52, 50, 210, 300, FONT_NORMAL_BLACK);
    lang_text_draw_amount(8, 4, city_emperor_months_since_gift(), 210 + width, 300, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    inner_panel_draw(112, 208, 28, 5);

    if (city_emperor_can_send_gift(GIFT_MODEST)) {
        const emperor_gift *gift = city_emperor_get_gift(GIFT_MODEST);
        lang_text_draw(52, 63, 128, 218, FONT_NORMAL_WHITE);
        font_t font = focus_button_id == 1 ? FONT_NORMAL_RED : FONT_NORMAL_WHITE;
        int width = lang_text_draw(52, 51 + gift->id, 224, 218, font);
        text_draw_money(gift->cost, 224 + width, 218, font);
    } else {
        lang_text_draw_multiline(52, 70, 160, 224, 352, FONT_NORMAL_WHITE);
    }
    if (city_emperor_can_send_gift(GIFT_GENEROUS)) {
        const emperor_gift *gift = city_emperor_get_gift(GIFT_GENEROUS);
        lang_text_draw(52, 64, 128, 238, FONT_NORMAL_WHITE);
        font_t font = focus_button_id == 2 ? FONT_NORMAL_RED : FONT_NORMAL_WHITE;
        int width = lang_text_draw(52, 55 + gift->id, 224, 238, font);
        text_draw_money(gift->cost, 224 + width, 238, font);
    }
    if (city_emperor_can_send_gift(GIFT_LAVISH)) {
        const emperor_gift *gift = city_emperor_get_gift(GIFT_LAVISH);
        lang_text_draw(52, 65, 128, 258, FONT_NORMAL_WHITE);
        font_t font = focus_button_id == 3 ? FONT_NORMAL_RED : FONT_NORMAL_WHITE;
        int width = lang_text_draw(52, 59 + gift->id, 224, 258, font);
        text_draw_money(gift->cost, 224 + width, 258, font);
    }

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_gift_to_emperor, sizeof(buttons_gift_to_emperor) / sizeof(generic_button), &focus_button_id)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        window_advisors_show(ADVISOR_IMPERIAL);
    }
}

static void button_send_gift(int gift_id, __attribute__((unused)) int param2)
{
    if (city_emperor_set_gift_size(gift_id - 1)) {
        window_invalidate();
    }
    if (city_emperor_can_send_gift(GIFT_MODEST)) {
        city_emperor_send_gift();
        window_advisors_show(ADVISOR_IMPERIAL);
    }
}

void window_gift_to_emperor_show(void)
{
    window_type window = {
        WINDOW_GIFT_TO_EMPEROR,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    init();
    window_show(&window);
}
