#include "display_options.h"

#include "game/custom_strings.h"
#include "game/settings.h"
#include "game/system.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"

static void button_fullscreen(int param1, int param2);
static void button_reset_window(int param1, int param2);

static generic_button display_top_menu_buttons[] = {
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
    text_draw_centered(get_custom_string(TR_HOTKEY_RESET_WINDOW), 128, 164, 224, FONT_NORMAL_GREEN, COLOR_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, display_top_menu_buttons, sizeof(display_top_menu_buttons)/(sizeof(generic_button)), &data.focus_button_id)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        data.close_callback();
    }
}

static void button_fullscreen(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    system_set_fullscreen(!setting_fullscreen());
}

static void button_reset_window(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    system_resize(1280, 800);
    system_center();
}

void window_display_options_show(void (*close_callback)(void))
{
    window_type window = {
        WINDOW_DISPLAY_OPTIONS,
        window_draw_underlying_window,
        draw_foreground,
        handle_input,
        0
    };
    init(close_callback);
    window_show(&window);
}
