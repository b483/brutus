#include "trade_opened.h"

#include "core/image_group.h"
#include "empire/object.h"
#include "graphics/graphics.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/window.h"
#include "window/advisors.h"
#include "window/empire.h"

static void button_advisor(int advisor, int param2);
static void button_close(int param1, int param2);

static image_button image_buttons[] = {
    {92, 248, 28, 28, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 12, button_advisor, button_none, ADVISOR_TRADE, 0, 1, 0, 0, 0},
    {522, 252, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_close, button_none, 0, 0, 1, 0, 0, 0},
};

static struct empire_object_t *selected_trade_city = 0;

static void draw_background(void)
{
    graphics_in_dialog();

    outer_panel_draw(80, 64, 30, 14);
    lang_text_draw_centered(142, 0, 80, 80, 480, FONT_LARGE_BLACK);
    if (empire_object_is_sea_trade_route(selected_trade_city->trade_route_id)) {
        lang_text_draw_multiline(142, 1, 112, 120, 416, FONT_NORMAL_BLACK);
        lang_text_draw_multiline(142, 3, 112, 184, 416, FONT_NORMAL_BLACK);
    } else {
        lang_text_draw_multiline(142, 1, 112, 152, 416, FONT_NORMAL_BLACK);
    }
    lang_text_draw(142, 2, 128, 256, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();
    image_buttons_draw(0, 0, image_buttons, 2);
    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (image_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, image_buttons, 2, 0)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_empire_show();
    }
}

static void button_advisor(int advisor, __attribute__((unused)) int param2)
{
    window_advisors_show(advisor);
}

static void button_close(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_empire_show();
}

void window_trade_opened_show(struct empire_object_t *trade_city)
{
    window_type window = {
        WINDOW_TRADE_OPENED,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    selected_trade_city = trade_city;
    window_show(&window);
}
