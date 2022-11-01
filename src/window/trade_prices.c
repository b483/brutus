#include "trade_prices.h"

#include "empire/trade_prices.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"

static void draw_foreground(void)
{
    graphics_in_dialog();

    graphics_shade_rect(33, 53, 574, 334, 0);
    outer_panel_draw(16, 140, 38, 10);

    // Coin image
    image_draw(image_group(GROUP_RESOURCE_ICONS) + RESOURCE_DENARII, 32, 156);

    // Prices set by Rome
    lang_text_draw_centered(54, 21, 26, 156, 608, FONT_LARGE_BLACK);

    // Buyers pay
    lang_text_draw(54, 22, 32, 233, FONT_NORMAL_BLACK);
    // Sellers receive
    lang_text_draw(54, 23, 32, 257, FONT_NORMAL_BLACK);

    for (int i = RESOURCE_MIN; i < RESOURCE_MAX; i++) {
        image_draw(image_group(GROUP_RESOURCE_ICONS) + i + resource_image_offset(i, RESOURCE_IMAGE_ICON), 126 + 30 * i, 201);
        text_draw_number_centered(trade_price_buy(i), 120 + 30 * i, 234, 30, FONT_SMALL_PLAIN);
        text_draw_number_centered(trade_price_sell(i), 120 + 30 * i, 259, 30, FONT_SMALL_PLAIN);
    }

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_go_back();
        return;
    }

    const mouse *m_dialog = mouse_in_dialog(m);
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 16 || m_dialog->y < 144 || m_dialog->x > 624 || m_dialog->y > 300)) {
        window_go_back();
        return;
    }
}

static int get_tooltip_resource(tooltip_context *c)
{
    int x_base = screen_dialog_offset_x() + 124;
    int y = screen_dialog_offset_y() + 203;
    int x_mouse = c->mouse_x;
    int y_mouse = c->mouse_y;

    for (int i = RESOURCE_MIN; i < RESOURCE_MAX; i++) {
        int x = x_base + 30 * i;
        if (x <= x_mouse && x + 24 > x_mouse && y <= y_mouse && y + 24 > y_mouse) {
            return i;
        }
    }
    return 0;
}

static void get_tooltip(tooltip_context *c)
{
    int resource = get_tooltip_resource(c);
    if (!resource) {
        return;
    }
    c->type = TOOLTIP_BUTTON;
    c->text_id = 131 + resource;
}

void window_trade_prices_show(void)
{
    window_type window = {
        WINDOW_TRADE_PRICES,
        window_draw_underlying_window,
        draw_foreground,
        handle_input,
        get_tooltip
    };
    window_show(&window);
}
