#include "victory_dialog.h"

#include "city/victory.h"
#include "graphics/graphics.h"
#include "scenario/scenario.h"
#include "sound/sound.h"
#include "window/city.h"

static void button_accept(int param1, int param2);
static void button_continue_governing(int months, int param2);

static struct generic_button_t victory_buttons[] = {
    {32, 112, 480, 20, button_accept, button_none, 0, 0},
    {32, 144, 480, 20, button_continue_governing, button_none, 24, 0},
    {32, 176, 480, 20, button_continue_governing, button_none, 60, 0},
};

static int focus_button_id = 0;

static void draw_background(void)
{
    graphics_in_dialog();

    outer_panel_draw(48, 128, 34, 15);
    lang_text_draw_centered(62, 0, 48, 144, 544, FONT_LARGE_BLACK);
    text_draw_centered(scenario_settings.player_name, 48, 194, 544, FONT_LARGE_BLACK, 0);
    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    // Accept promotion
    large_label_draw(80, 240, 30, focus_button_id == 1);
    lang_text_draw_centered(62, 3, 80, 246, 480, FONT_NORMAL_GREEN);
    // Continue for 2 years
    large_label_draw(80, 272, 30, focus_button_id == 2);
    lang_text_draw_centered(62, 4, 80, 278, 480, FONT_NORMAL_GREEN);
    // Continue for 5 years
    large_label_draw(80, 304, 30, focus_button_id == 3);
    lang_text_draw_centered(62, 5, 80, 310, 480, FONT_NORMAL_GREEN);

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, __attribute__((unused)) const struct hotkeys_t *h)
{
    generic_buttons_handle_mouse(mouse_in_dialog(m), 48, 128, victory_buttons, 3, &focus_button_id);
}

static void button_accept(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_city_show();
}

static void button_continue_governing(int months, __attribute__((unused)) int param2)
{
    city_victory_continue_governing(months);
    window_city_show();
    city_victory_reset();
    update_music(1);
}

void window_victory_dialog_show(void)
{
    struct window_type_t window = {
        WINDOW_VICTORY_DIALOG,
        draw_background,
        draw_foreground,
        handle_input,
    };
    window_show(&window);
}
