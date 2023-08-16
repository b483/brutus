#include "popup_dialog.h"

#include "core/image_group.h"
#include "graphics/graphics.h"

#define GROUP 5

#define PROCEED_GROUP 43
#define PROCEED_TEXT 5

static void button_ok(int param1, int param2);
static void button_cancel(int param1, int param2);
static void confirm(void);

static struct image_button_t popup_dialog_buttons[] = {
    {192, 100, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 0, button_ok, button_none, 1, 0, 1, 0, 0, 0},
    {256, 100, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 4, button_cancel, button_none, 0, 0, 1, 0, 0, 0},
};

static struct {
    int type;
    int custom_text_group;
    int custom_text_id;
    int ok_clicked;
    void (*close_func)(void);
    int has_buttons;
} data;

static int init(int type, int custom_text_group, int custom_text_id,
        void (*close_func)(void), int has_ok_cancel_buttons)
{
    if (window_is(WINDOW_POPUP_DIALOG)) {
        // don't show popup over popup
        return 0;
    }
    data.type = type;
    data.custom_text_group = custom_text_group;
    data.custom_text_id = custom_text_id;
    data.ok_clicked = 0;
    data.close_func = close_func;
    data.has_buttons = has_ok_cancel_buttons;
    return 1;
}

static void draw_background(void)
{
    window_draw_underlying_window();
    graphics_in_dialog();
    if (data.has_buttons) {
        outer_panel_draw(80, 80, 30, 10);
    } else {
        outer_panel_draw(80, 80, 30, 7);
    }
    if (data.type >= 0) {
        lang_text_draw_centered(GROUP, data.type, 80, 100, 480, FONT_LARGE_BLACK);
        if (lang_text_get_width(GROUP, data.type + 1, FONT_NORMAL_BLACK) >= 420) {
            lang_text_draw_multiline(GROUP, data.type + 1, 110, 140, 420, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw_centered(GROUP, data.type + 1, 80, 140, 480, FONT_NORMAL_BLACK);
        }
    } else {
        lang_text_draw_centered(data.custom_text_group, data.custom_text_id, 80, 100, 480, FONT_LARGE_BLACK);
        lang_text_draw_centered(PROCEED_GROUP, PROCEED_TEXT, 80, 140, 480, FONT_NORMAL_BLACK);
    }
    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();
    if (data.has_buttons) {
        image_buttons_draw(80, 80, popup_dialog_buttons, sizeof(popup_dialog_buttons) / sizeof(struct image_button_t));
    }
    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (data.has_buttons && image_buttons_handle_mouse(mouse_in_dialog(m), 80, 80, popup_dialog_buttons, sizeof(popup_dialog_buttons) / sizeof(struct image_button_t), 0)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        button_cancel(0, 0);
    }
    if (h->enter_pressed) {
        confirm();
    }
}

static void button_ok(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    confirm();
}

static void button_cancel(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_go_back();
    // prevent getting stuck on top menu window (last active) when declining pop-up to exit scenario/editor
    if (window_is(WINDOW_TOP_MENU) || window_is(WINDOW_EDITOR_TOP_MENU)) {
        window_go_back();
    }
}

static void confirm(void)
{
    window_go_back();
    data.close_func();
}

void window_popup_dialog_show(int type,
        void (*close_func)(void), int has_ok_cancel_buttons)
{
    if (init(type, 0, 0, close_func, has_ok_cancel_buttons)) {
        struct window_type_t window = {
            WINDOW_POPUP_DIALOG,
            draw_background,
            draw_foreground,
            handle_input,
        };
        window_show(&window);
    }
}

void window_popup_dialog_show_confirmation(int text_group, int text_id,
        void (*close_func)(void))
{
    if (init(POPUP_DIALOG_NONE, text_group, text_id, close_func, 1)) {
        struct window_type_t window = {
            WINDOW_POPUP_DIALOG,
            draw_background,
            draw_foreground,
            handle_input,
        };
        window_show(&window);
    }
}
