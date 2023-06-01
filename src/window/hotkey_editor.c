#include "hotkey_editor.h"

#include "core/hotkey_config.h"
#include "core/image_group.h"
#include "core/string.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"

#define NUM_BOTTOM_BUTTONS 2

static void button_close(int save, int param2);

static struct generic_button_t bottom_buttons[] = {
    {192, 228, 120, 24, button_close, button_none, 0, 0},
    {328, 228, 120, 24, button_close, button_none, 1, 0},
};

static struct {
    int action;
    int index;
    int key;
    int modifiers;
    void (*callback)(int, int, int, int);
    int focus_button;
} data;

static void init(int action, int index,
    void (*callback)(int, int, int, int))
{
    data.action = action;
    data.index = index;
    data.callback = callback;
    data.key = KEY_TYPE_NONE;
    data.modifiers = KEY_MOD_NONE;
    data.focus_button = 0;
}

static char *hotkey_editor_bottom_button_strings[] = {
    "Press new hotkey", // 0
    "Cancel", // 1
    "OK", // 2
};

static void draw_background(void)
{
    window_draw_underlying_window();

    graphics_in_dialog();
    outer_panel_draw(128, 128, 24, 9);

    text_draw_centered(hotkey_editor_bottom_button_strings[0], 136, 144, 376, FONT_LARGE_BLACK, 0);

    for (int i = 0; i < NUM_BOTTOM_BUTTONS; i++) {
        struct generic_button_t *btn = &bottom_buttons[i];
        text_draw_centered(hotkey_editor_bottom_button_strings[i + 1], btn->x, btn->y + 6, btn->width, FONT_NORMAL_BLACK, 0);
    }

    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    inner_panel_draw(192, 184, 16, 2);

    text_draw_centered(key_combination_display_name(data.key, data.modifiers),
        192, 193, 256, FONT_NORMAL_WHITE, 0);

    for (int i = 0; i < NUM_BOTTOM_BUTTONS; i++) {
        struct generic_button_t *btn = &bottom_buttons[i];
        button_border_draw(btn->x, btn->y, btn->width, btn->height, data.focus_button == i + 1);
    }
    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, __attribute__((unused)) const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);

    int handled = 0;
    handled |= generic_buttons_handle_mouse(m_dialog, 0, 0, bottom_buttons, NUM_BOTTOM_BUTTONS, &data.focus_button);
    if (!handled && m->right.went_up) {
        button_close(0, 0);
    }
}

static void button_close(int ok, __attribute__((unused)) int param2)
{
    // destroy window before callback call, because there may appear another popup window
    // by design new popup window can't be showed over another popup window
    window_go_back();
    if (ok) {
        data.callback(data.action, data.index, data.key, data.modifiers);
    }
}

void window_hotkey_editor_key_pressed(int key, int modifiers)
{
    if (key == KEY_TYPE_ENTER && modifiers == KEY_MOD_NONE) {
        button_close(1, 0);
    } else if (key == KEY_TYPE_ESCAPE && modifiers == KEY_MOD_NONE) {
        button_close(0, 0);
    } else {
        if (key != KEY_TYPE_NONE) {
            data.key = key;
        }
        data.modifiers = modifiers;
    }
}

void window_hotkey_editor_key_released(int key, int modifiers)
{
    // update modifiers as long as we don't have a proper keypress
    if (data.key == KEY_TYPE_NONE && key == KEY_TYPE_NONE) {
        data.modifiers = modifiers;
    }
}

void window_hotkey_editor_show(int action, int index,
    void (*callback)(int, int, int, int))
{
    struct window_type_t window = {
        WINDOW_HOTKEY_EDITOR,
        draw_background,
        draw_foreground,
        handle_input,
    };
    init(action, index, callback);
    window_show(&window);
}
