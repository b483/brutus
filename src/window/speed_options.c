#include "speed_options.h"

#include "game/settings.h"
#include "graphics/graphics.h"
#include "editor/editor.h"
#include "window/city.h"

static void arrow_button_game(int is_down, int param2);
static void arrow_button_scroll(int is_down, int param2);

static struct arrow_button_t arrow_buttons_speed_options[] = {
    {112, 100, 17, 24, arrow_button_game, 1, 0, 0, 0},
    {136, 100, 15, 24, arrow_button_game, 0, 0, 0, 0},
    {112, 136, 17, 24, arrow_button_scroll, 1, 0, 0, 0},
    {136, 136, 15, 24, arrow_button_scroll, 0, 0, 0, 0},
};

static struct {
    int focus_button_id;
    int from_editor;
    int original_game_speed;
    int original_scroll_speed;
} data;

static void init(int from_editor)
{
    data.focus_button_id = 0;
    data.from_editor = from_editor;
    data.original_game_speed = setting_game_speed();
    data.original_scroll_speed = setting_scroll_speed();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(80, 80, 20, 10);

    // title
    lang_text_draw_centered(45, 0, 96, 92, 288, FONT_LARGE_BLACK);
    // game speed
    lang_text_draw(45, 2, 112, 146, FONT_NORMAL_PLAIN);
    text_draw_percentage(setting_game_speed(), 328, 146, FONT_NORMAL_PLAIN);
    // scroll speed
    lang_text_draw(45, 3, 112, 182, FONT_NORMAL_PLAIN);
    text_draw_percentage(setting_scroll_speed(), 328, 182, FONT_NORMAL_PLAIN);

    arrow_buttons_draw(160, 40, arrow_buttons_speed_options, sizeof(arrow_buttons_speed_options) / sizeof(struct arrow_button_t));
    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (arrow_buttons_handle_mouse(m_dialog, 160, 40, arrow_buttons_speed_options, sizeof(arrow_buttons_speed_options) / sizeof(struct arrow_button_t), 0)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        if (data.from_editor) {
            show_editor_map();
        } else {
            window_city_return();
        }
    }
}

static void arrow_button_game(int is_down, __attribute__((unused)) int param2)
{
    if (is_down) {
        setting_decrease_game_speed();
    } else {
        setting_increase_game_speed();
    }
}

static void arrow_button_scroll(int is_down, __attribute__((unused)) int param2)
{
    if (is_down) {
        setting_decrease_scroll_speed();
    } else {
        setting_increase_scroll_speed();
    }
}

void window_speed_options_show(int from_editor)
{
    struct window_type_t window = {
        WINDOW_SPEED_OPTIONS,
        window_draw_underlying_window,
        draw_foreground,
        handle_input,
    };
    init(from_editor);
    window_show(&window);
}
