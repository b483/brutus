#include "plain_message_dialog.h"

#include "core/image_group.h"
#include "core/string.h"
#include "graphics/graphics.h"
#include "graphics/image_button.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"

static void button_ok(int param1, int param2);

static image_button buttons[] = {
    {223, 140, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 0, button_ok, button_none, 1, 0, 1, 0, 0, 0},
};

static struct {
    const uint8_t *title;
    const uint8_t *message;
    const uint8_t *extra;
} data;

static int init(custom_string_key title, custom_string_key message, const uint8_t *extra)
{
    if (window_is(WINDOW_PLAIN_MESSAGE_DIALOG)) {
        // don't show popup over popup
        return 0;
    }
    data.title = get_custom_string(title);
    data.message = get_custom_string(message);
    data.extra = extra;
    return 1;
}

static void draw_background(void)
{
    graphics_in_dialog();
    outer_panel_draw(80, 80, 30, 12);
    text_draw_centered(data.title, 80, 100, 480, FONT_LARGE_BLACK, 0);
    text_draw_multiline(data.message, 100, 140, 450, FONT_NORMAL_BLACK, 0);
    if (data.extra) {
        text_draw_centered(data.extra, 100, 180, 450, FONT_NORMAL_BLACK, 0);
    }
    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();
    image_buttons_draw(80, 80, buttons, 1);
    graphics_reset_dialog();
}

static void close(void)
{
    window_go_back();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (image_buttons_handle_mouse(mouse_in_dialog(m), 80, 80, buttons, 1, 0)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed || h->enter_pressed) {
        close();
    }
}

static void button_ok(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    close();
}

void window_plain_message_dialog_show(custom_string_key title, custom_string_key message)
{
    if (init(title, message, 0)) {
        window_type window = {
            WINDOW_PLAIN_MESSAGE_DIALOG,
            draw_background,
            draw_foreground,
            handle_input,
            0
        };
        window_show(&window);
    }
}

void window_plain_message_dialog_show_with_extra(custom_string_key title, custom_string_key message, const uint8_t *extra)
{
    if (init(title, message, extra)) {
        window_type window = {
            WINDOW_PLAIN_MESSAGE_DIALOG,
            draw_background,
            draw_foreground,
            handle_input,
            0
        };
        window_show(&window);
    }
}
