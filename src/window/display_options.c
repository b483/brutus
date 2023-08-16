#include "display_options.h"

#include "game/settings.h"
#include "platform/brutus.h"
#include "graphics/graphics.h"

static void button_fullscreen(int param1, int param2);
static void button_reset_window(int param1, int param2);

static struct generic_button_t display_top_menu_buttons[] = {
    {128, 136, 224, 20, button_fullscreen, button_none, 1, 0},
    {128, 160, 224, 20, button_reset_window, button_none, 0, 0},
};

static struct {
    int focus_button_id;
    void (*close_callback)(void);
} data;

static void init(void (*close_callback)(void))
{
    data.focus_button_id = 0;
    data.close_callback = close_callback;
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(96, 80, 18, 8);

    label_draw(128, 136, 14, data.focus_button_id == 1 ? 1 : 2);
    label_draw(128, 160, 14, data.focus_button_id == 2 ? 1 : 2);

    // Display Options
    lang_text_draw_centered(42, 0, 128, 94, 224, FONT_LARGE_BLACK);

    // Full screen/Windowed screen
    lang_text_draw_centered(42, setting_fullscreen() ? 2 : 1, 128, 140, 224, FONT_NORMAL_GREEN);

    // Reset resolution
    text_draw_centered("Reset window", 128, 164, 224, FONT_NORMAL_GREEN, COLOR_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, display_top_menu_buttons, sizeof(display_top_menu_buttons) / (sizeof(struct generic_button_t)), &data.focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        data.close_callback();
    }
}

static void button_fullscreen(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    post_event(setting_fullscreen() ? USER_EVENT_WINDOWED : USER_EVENT_FULLSCREEN);
}

static void button_reset_window(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    system_resize(1280, 800);
    post_event(USER_EVENT_CENTER_WINDOW);
}

void window_display_options_show(void (*close_callback)(void))
{
    struct window_type_t window = {
        WINDOW_DISPLAY_OPTIONS,
        window_draw_underlying_window,
        draw_foreground,
        handle_input,
    };
    init(close_callback);
    window_show(&window);
}
