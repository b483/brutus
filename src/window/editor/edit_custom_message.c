#include "edit_custom_message.h"

#include "core/string.h"
#include "game/custom_strings.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/rich_text.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "scenario/editor.h"
#include "widget/input_box.h"
#include "window/editor/custom_messages.h"
#include "window/editor/map.h"
#include "window/numeric_input.h"

static void button_year(int param1, int param2);
static void button_month(int param1, int param2);
static void button_urgent(int param1, int param2);
static void button_enabled(int param1, int param2);
static void button_reset_message(int param1, int param2);
static void button_reset_title(int param1, int param2);
static void button_reset_text(int param1, int param2);

static generic_button buttons_edit_custom_message[] = {
    {36, 58, 75, 25, button_year, button_none, 0, 0},
    {36, 88, 75, 25, button_month, button_none, 0, 0},
    {36, 118, 75, 25, button_urgent, button_none, 0, 0},
    {36, 194, 75, 25, button_enabled, button_none, 0, 0},
    {135, 194, 125, 25, button_reset_message, button_none, 0, 0},
};

static generic_button reset_title_button[] = {
    {5, 110, 125, 25, button_reset_title, button_none, 0, 0},
};

static generic_button reset_text_button[] = {
    {515, 16, 150, 35, button_reset_text, button_none, 0, 0},
};

uint8_t editor_custom_message_video_file[MAX_CUSTOM_MESSAGE_VIDEO_TEXT];
uint8_t editor_custom_message_title[MAX_CUSTOM_MESSAGE_TEXT];
uint8_t editor_custom_message_text[MAX_CUSTOM_MESSAGE_TEXT];

static input_box editor_custom_message_input_video_file = { 36, 154, 10, 2, FONT_NORMAL_WHITE, 1, editor_custom_message_video_file, MAX_CUSTOM_MESSAGE_TITLE };
static input_box editor_custom_message_input_title = { -68, 64, 17, 2, FONT_NORMAL_WHITE, 1, editor_custom_message_title, MAX_CUSTOM_MESSAGE_TITLE };
static input_box editor_custom_message_input_text = { -68, 64, 46, 2, FONT_NORMAL_WHITE, 1, editor_custom_message_text, MAX_CUSTOM_MESSAGE_TEXT };

static struct {
    int id;
    int category;
    int focus_button_id;
} data;

static void init(int id, int category)
{
    data.id = id;
    data.category = category;
    if (data.category == CUSTOM_MESSAGE_ATTRIBUTES) {
        string_copy(scenario.editor_custom_messages[id].video_file, editor_custom_message_video_file, MAX_CUSTOM_MESSAGE_TITLE);
        input_box_start(&editor_custom_message_input_video_file);
    } else if (data.category == CUSTOM_MESSAGE_TITLE) {
        string_copy(scenario.editor_custom_messages[id].title, editor_custom_message_title, MAX_CUSTOM_MESSAGE_TITLE);
        input_box_start(&editor_custom_message_input_title);
    } else if (data.category == CUSTOM_MESSAGE_TEXT) {
        string_copy(scenario.editor_custom_messages[id].text, editor_custom_message_text, MAX_CUSTOM_MESSAGE_TEXT);
        input_box_start(&editor_custom_message_input_text);
    }
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    if (data.category == CUSTOM_MESSAGE_ATTRIBUTES) {
        outer_panel_draw(-100, 0, 24, 15);

        // Attributes
        text_draw_centered(get_custom_string(TR_EDITOR_CUSTOM_MESSAGE_ATTRIBUTES), -100, 16, 384, FONT_LARGE_BLACK, COLOR_BLACK);

        // Year offset
        text_draw(get_custom_string(TR_EDITOR_OFFSET_YEAR), -68, 64, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(36, 58, 75, 25, data.focus_button_id == 1);
        text_draw_number_centered_prefix(scenario.editor_custom_messages[data.id].year, '+', 38, 64, 75, FONT_NORMAL_BLACK);
        lang_text_draw_year(scenario.start_year + scenario.editor_custom_messages[data.id].year, 118, 64, FONT_NORMAL_BLACK);

        // Month
        text_draw(get_custom_string(TR_EDITOR_MONTH), -68, 94, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(36, 88, 75, 25, data.focus_button_id == 2);
        text_draw_number_centered(scenario.editor_custom_messages[data.id].month + 1, 36, 94, 75, FONT_NORMAL_BLACK);

        // Invalid year/month combination
        if (scenario.editor_custom_messages[data.id].year == 0 && scenario.editor_custom_messages[data.id].month == 0) {
            text_draw(get_custom_string(TR_EDITOR_INVALID_YEAR_MONTH), 122, 94, FONT_NORMAL_PLAIN, COLOR_RED);
        }

        // Urgent
        text_draw(get_custom_string(TR_EDITOR_CUSTOM_MESSAGE_URGENT), -68, 124, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(36, 118, 75, 25, data.focus_button_id == 3);
        lang_text_draw_centered(18, scenario.editor_custom_messages[data.id].urgent, 36, 124, 75, FONT_NORMAL_BLACK);

        // Video file
        text_draw(get_custom_string(TR_EDITOR_CUSTOM_MESSAGE_VIDEO_FILE), -68, 160, FONT_NORMAL_BLACK, COLOR_BLACK);
        input_box_draw(&editor_custom_message_input_video_file);

        // Video file hint
        text_draw(get_custom_string(TR_EDITOR_CUSTOM_MESSAGE_VIDEO_FILE_HINT), 200, 160, FONT_NORMAL_PLAIN, COLOR_TOOLTIP);

        // Enabled
        text_draw(get_custom_string(TR_EDITOR_CUSTOM_MESSAGE_ENABLED), -68, 200, FONT_NORMAL_BLACK, COLOR_BLACK);
        button_border_draw(36, 194, 75, 25, data.focus_button_id == 4);
        lang_text_draw_centered(18, scenario.editor_custom_messages[data.id].enabled, 36, 200, 75, FONT_NORMAL_BLACK);

        // Reset message
        button_border_draw(135, 194, 125, 25, data.focus_button_id == 5);
        text_draw_centered(get_custom_string(TR_EDITOR_CUSTOM_MESSAGE_RESET), 135, 200, 125, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    else if (data.category == CUSTOM_MESSAGE_TITLE) {
        outer_panel_draw(-100, 0, 21, 10);
        // Title
        text_draw_centered(get_custom_string(TR_EDITOR_CUSTOM_MESSAGE_TITLE), -100, 16, 336, FONT_LARGE_BLACK, COLOR_BLACK);
        input_box_draw(&editor_custom_message_input_title);

        // Reset title
        button_border_draw(5, 110, 125, 25, data.focus_button_id == 1);
        text_draw_centered(get_custom_string(TR_EDITOR_CUSTOM_MESSAGE_RESET_TITLE), 5, 116, 125, FONT_NORMAL_PLAIN, COLOR_RED);
    }

    else if (data.category == CUSTOM_MESSAGE_TEXT) {
        outer_panel_draw(-100, 0, 50, 32);
        // Text
        text_draw_centered(get_custom_string(TR_EDITOR_CUSTOM_MESSAGE_TEXT), -100, 16, 800, FONT_LARGE_BLACK, COLOR_BLACK);
        input_box_draw(&editor_custom_message_input_text);

        // Formatted typed in text
        rich_text_set_fonts(FONT_NORMAL_BLACK, FONT_NORMAL_RED, 6);
        rich_text_init(editor_custom_message_text, -76, 112, 44, 22, 0);
        graphics_set_clip_rectangle(-76, 112, 750, 350);
        rich_text_draw(editor_custom_message_text, -68, 112, 736, 20, 0);
        rich_text_reset_lines_only();
        rich_text_draw_scrollbar();
        graphics_reset_clip_rectangle();

        // @L, @P hint
        text_draw(get_custom_string(TR_EDITOR_SCENARIO_RICH_TEXT_HINT), -60, 475, FONT_NORMAL_PLAIN, COLOR_TOOLTIP);

        // Reset text
        button_border_draw(515, 16, 150, 35, data.focus_button_id == 1);
        text_draw_centered(get_custom_string(TR_EDITOR_CUSTOM_MESSAGE_RESET_TEXT), 515, 21, 150, FONT_LARGE_PLAIN, COLOR_RED);
    }

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (m->right.went_up || h->escape_pressed) {
        if (data.category == CUSTOM_MESSAGE_ATTRIBUTES) {
            input_box_stop(&editor_custom_message_input_video_file);
            if (!string_equals(scenario.editor_custom_messages[data.id].video_file, editor_custom_message_video_file)) {
                string_copy(editor_custom_message_video_file, scenario.editor_custom_messages[data.id].video_file, MAX_CUSTOM_MESSAGE_VIDEO_TEXT);
            }
        } else if (data.category == CUSTOM_MESSAGE_TITLE) {
            input_box_stop(&editor_custom_message_input_title);
            if (!string_equals(scenario.editor_custom_messages[data.id].title, editor_custom_message_title)) {
                string_copy(editor_custom_message_title, scenario.editor_custom_messages[data.id].title, MAX_CUSTOM_MESSAGE_TITLE);
            }
        } else if (data.category == CUSTOM_MESSAGE_TEXT) {
            input_box_stop(&editor_custom_message_input_text);
            if (!string_equals(scenario.editor_custom_messages[data.id].text, editor_custom_message_text)) {
                string_copy(editor_custom_message_text, scenario.editor_custom_messages[data.id].text, MAX_CUSTOM_MESSAGE_TEXT);
            }
            rich_text_reset(0);
        }
        scenario_editor_sort_custom_messages();
        window_editor_custom_messages_show();
    }

    const mouse *m_dialog = mouse_in_dialog(m);
    if (data.category == CUSTOM_MESSAGE_ATTRIBUTES) {
        if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_edit_custom_message, sizeof(buttons_edit_custom_message) / sizeof(generic_button), &data.focus_button_id)) {
            return;
        }
    } else if (data.category == CUSTOM_MESSAGE_TITLE) {
        if (generic_buttons_handle_mouse(m_dialog, 0, 0, reset_title_button, 1, &data.focus_button_id)) {
            return;
        }
    } else if (data.category == CUSTOM_MESSAGE_TEXT) {
        rich_text_handle_mouse(m_dialog);
        if (generic_buttons_handle_mouse(m_dialog, 0, 0, reset_text_button, 1, &data.focus_button_id)) {
            return;
        }
    }
}

static void set_year(int value)
{
    scenario.editor_custom_messages[data.id].year = value;
}

static void button_year(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 15, screen_dialog_offset_y() - 28, 3, 999, set_year);
}

static void set_month(int value)
{
    // Jan is 1 for input/draw purposes
    if (value == 0) {
        value = 1;
    }
    // change month back to 0 indexed before saving
    scenario.editor_custom_messages[data.id].month = value - 1;
}

static void button_month(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_numeric_input_show(screen_dialog_offset_x() + 15, screen_dialog_offset_y() + 2, 2, 12, set_month);
}

static void button_urgent(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.editor_custom_messages[data.id].urgent = !scenario.editor_custom_messages[data.id].urgent;
}

static void button_enabled(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.editor_custom_messages[data.id].enabled = !scenario.editor_custom_messages[data.id].enabled;
}

static void button_reset_message(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.editor_custom_messages[data.id].year = 1;
    scenario.editor_custom_messages[data.id].month = 0;
    scenario.editor_custom_messages[data.id].urgent = 0;
    scenario.editor_custom_messages[data.id].enabled = 0;
    scenario.editor_custom_messages[data.id].title[0] = '\0';
    scenario.editor_custom_messages[data.id].text[0] = '\0';
    scenario.editor_custom_messages[data.id].video_file[0] = '\0';
    scenario_editor_sort_custom_messages();
    window_editor_custom_messages_show();
}

static void button_reset_title(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.editor_custom_messages[data.id].title[0] = '\0';
    input_box_stop(&editor_custom_message_input_title);
    string_copy(scenario.editor_custom_messages[data.id].title, editor_custom_message_title, MAX_CUSTOM_MESSAGE_TITLE);
    input_box_start(&editor_custom_message_input_title);
}

static void button_reset_text(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    scenario.editor_custom_messages[data.id].text[0] = '\0';
    input_box_stop(&editor_custom_message_input_text);
    string_copy(scenario.editor_custom_messages[data.id].text, editor_custom_message_text, MAX_CUSTOM_MESSAGE_TITLE);
    rich_text_reset(0);
    input_box_start(&editor_custom_message_input_text);
}

void window_editor_edit_custom_message_show(int id, int category)
{
    window_type window = {
        WINDOW_EDITOR_EDIT_CUSTOM_MESSAGE,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
        0
    };
    init(id, category);
    window_show(&window);
}
