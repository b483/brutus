#include "briefing.h"

#include "core/string.h"
#include "game/custom_strings.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/rich_text.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/input.h"
#include "scenario/data.h"
#include "scenario/editor.h"
#include "scenario/property.h"
#include "widget/input_box.h"
#include "widget/sidebar/editor.h"
#include "window/editor/attributes.h"
#include "window/editor/map.h"

static void button_reset_briefing_text(int param1, int param2);

uint8_t briefing[MAX_BRIEFING];
static int focus_button_id;

static input_box scenario_briefing_input = {
    -260, -150, 55, 2, FONT_NORMAL_WHITE, 1,
    briefing, MAX_BRIEFING
};

static generic_button button_reset[] = {
    {455, 540, 190, 35, button_reset_briefing_text, button_none, 0, 0}
};

static void start_briefing_box_input(void)
{
    string_copy(scenario_briefing(), briefing, MAX_BRIEFING);
    input_box_start(&scenario_briefing_input);
}

static void stop_briefing_box_input(void)
{
    input_box_stop(&scenario_briefing_input);
    scenario_editor_update_briefing(briefing);
}

static void draw_background(void)
{
    window_editor_map_draw_all();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(-300, -165, 60, 4);
    input_box_draw(&scenario_briefing_input);

    outer_panel_draw(-300, -100, 60, 43);
    // Formatted typed in text
    rich_text_set_fonts(FONT_NORMAL_BLACK, FONT_NORMAL_RED, 6);
    rich_text_init(briefing, -350, -75, 60, 38, 0);
    graphics_set_clip_rectangle(-300, -100, 970, 635);
    rich_text_draw(briefing, -260, -70, 912, 35, 0);
    rich_text_reset_lines_only();
    rich_text_draw_scrollbar();
    graphics_reset_clip_rectangle();

    // @L, @P hint
    text_draw(get_custom_string(TR_EDITOR_SCENARIO_BRIEFING_HINT), -285, 550, FONT_NORMAL_PLAIN, COLOR_TOOLTIP);

    // Reset briefing
    button_border_draw(455, 540, 190, 35, focus_button_id);
    text_draw_centered(get_custom_string(TR_EDITOR_SCENARIO_BRIEFING_RESET), 455, 545, 190, FONT_LARGE_PLAIN, COLOR_TOOLTIP);

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    const mouse *m_dialog = mouse_in_dialog(m);
    if (input_box_handle_mouse(m_dialog, &scenario_briefing_input)) {
        return;
    }
    if (input_go_back_requested(m, h)) {
        stop_briefing_box_input();
        rich_text_reset(0);
        window_editor_attributes_show();
    }
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, button_reset, 1, &focus_button_id)) {
        return;
    }
    rich_text_handle_mouse(m_dialog);
}

static void button_reset_briefing_text(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    briefing[0] = '\0';
    stop_briefing_box_input();
    rich_text_reset(0);
    start_briefing_box_input();
}

void window_editor_briefing_show(void)
{
    window_type window = {
        WINDOW_EDITOR_BRIEFING,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    start_briefing_box_input();
    window_show(&window);
}
