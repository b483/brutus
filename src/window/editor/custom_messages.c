#include "custom_messages.h"

#include "core/image_group_editor.h"
#include "core/string.h"
#include "game/resource.h"
#include "graphics/button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "window/editor/attributes.h"
#include "window/editor/edit_custom_message.h"
#include "window/editor/map.h"

static void button_message(int id, int category);

static generic_button buttons_attributes[] = {
    {-300, 48, 125, 25, button_message, button_none, 0, 0},
    {-300, 78, 125, 25, button_message, button_none, 1, 0},
    {-300, 108, 125, 25, button_message, button_none, 2, 0},
    {-300, 138, 125, 25, button_message, button_none, 3, 0},
    {-300, 168, 125, 25, button_message, button_none, 4, 0},
    {-300, 198, 125, 25, button_message, button_none, 5, 0},
    {-300, 228, 125, 25, button_message, button_none, 6, 0},
    {-300, 258, 125, 25, button_message, button_none, 7, 0},
    {-300, 288, 125, 25, button_message, button_none, 8, 0},
    {-300, 318, 125, 25, button_message, button_none, 9, 0},
    {-300, 348, 125, 25, button_message, button_none, 10, 0},
    {-300, 378, 125, 25, button_message, button_none, 11, 0},
    {-300, 408, 125, 25, button_message, button_none, 12, 0},
    {-300, 438, 125, 25, button_message, button_none, 13, 0},
    {-300, 468, 125, 25, button_message, button_none, 14, 0},
    {100, 48, 125, 25, button_message, button_none, 15, 0},
    {100, 78, 125, 25, button_message, button_none, 16, 0},
    {100, 108, 125, 25, button_message, button_none, 17, 0},
    {100, 138, 125, 25, button_message, button_none, 18, 0},
    {100, 168, 125, 25, button_message, button_none, 19, 0},
    {100, 198, 125, 25, button_message, button_none, 20, 0},
    {100, 228, 125, 25, button_message, button_none, 21, 0},
    {100, 258, 125, 25, button_message, button_none, 22, 0},
    {100, 288, 125, 25, button_message, button_none, 23, 0},
    {100, 318, 125, 25, button_message, button_none, 24, 0},
    {100, 348, 125, 25, button_message, button_none, 25, 0},
    {100, 378, 125, 25, button_message, button_none, 26, 0},
    {100, 408, 125, 25, button_message, button_none, 27, 0},
    {100, 438, 125, 25, button_message, button_none, 28, 0},
    {100, 468, 125, 25, button_message, button_none, 29, 0},
    {500, 48, 125, 25, button_message, button_none, 30, 0},
    {500, 78, 125, 25, button_message, button_none, 31, 0},
    {500, 108, 125, 25, button_message, button_none, 32, 0},
    {500, 138, 125, 25, button_message, button_none, 33, 0},
    {500, 168, 125, 25, button_message, button_none, 34, 0},
    {500, 198, 125, 25, button_message, button_none, 35, 0},
    {500, 228, 125, 25, button_message, button_none, 36, 0},
    {500, 258, 125, 25, button_message, button_none, 37, 0},
    {500, 288, 125, 25, button_message, button_none, 38, 0},
    {500, 318, 125, 25, button_message, button_none, 39, 0},
    {500, 348, 125, 25, button_message, button_none, 39, 0},
    {500, 378, 125, 25, button_message, button_none, 39, 0},
    {500, 408, 125, 25, button_message, button_none, 39, 0},
    {500, 438, 125, 25, button_message, button_none, 39, 0},
    {500, 468, 125, 25, button_message, button_none, 39, 0},
};

static generic_button buttons_custom_message_title[] = {
    {-170, 48, 125, 25, button_message, button_none, 0, 1},
    {-170, 78, 125, 25, button_message, button_none, 1, 1},
    {-170, 108, 125, 25, button_message, button_none, 2, 1},
    {-170, 138, 125, 25, button_message, button_none, 3, 1},
    {-170, 168, 125, 25, button_message, button_none, 4, 1},
    {-170, 198, 125, 25, button_message, button_none, 5, 1},
    {-170, 228, 125, 25, button_message, button_none, 6, 1},
    {-170, 258, 125, 25, button_message, button_none, 7, 1},
    {-170, 288, 125, 25, button_message, button_none, 8, 1},
    {-170, 318, 125, 25, button_message, button_none, 9, 1},
    {-170, 348, 125, 25, button_message, button_none, 10, 1},
    {-170, 378, 125, 25, button_message, button_none, 11, 1},
    {-170, 408, 125, 25, button_message, button_none, 12, 1},
    {-170, 438, 125, 25, button_message, button_none, 13, 1},
    {-170, 468, 125, 25, button_message, button_none, 14, 1},
    {230, 48, 125, 25, button_message, button_none, 15, 1},
    {230, 78, 125, 25, button_message, button_none, 16, 1},
    {230, 108, 125, 25, button_message, button_none, 17, 1},
    {230, 138, 125, 25, button_message, button_none, 18, 1},
    {230, 168, 125, 25, button_message, button_none, 19, 1},
    {230, 198, 125, 25, button_message, button_none, 20, 1},
    {230, 228, 125, 25, button_message, button_none, 21, 1},
    {230, 258, 125, 25, button_message, button_none, 22, 1},
    {230, 288, 125, 25, button_message, button_none, 23, 1},
    {230, 318, 125, 25, button_message, button_none, 24, 1},
    {230, 348, 125, 25, button_message, button_none, 25, 1},
    {230, 378, 125, 25, button_message, button_none, 26, 1},
    {230, 408, 125, 25, button_message, button_none, 27, 1},
    {230, 438, 125, 25, button_message, button_none, 28, 1},
    {230, 468, 125, 25, button_message, button_none, 29, 1},
    {630, 48, 125, 25, button_message, button_none, 30, 1},
    {630, 78, 125, 25, button_message, button_none, 31, 1},
    {630, 108, 125, 25, button_message, button_none, 32, 1},
    {630, 138, 125, 25, button_message, button_none, 33, 1},
    {630, 168, 125, 25, button_message, button_none, 34, 1},
    {630, 198, 125, 25, button_message, button_none, 35, 1},
    {630, 228, 125, 25, button_message, button_none, 36, 1},
    {630, 258, 125, 25, button_message, button_none, 37, 1},
    {630, 288, 125, 25, button_message, button_none, 38, 1},
    {630, 318, 125, 25, button_message, button_none, 39, 1},
    {630, 348, 125, 25, button_message, button_none, 39, 1},
    {630, 378, 125, 25, button_message, button_none, 39, 1},
    {630, 408, 125, 25, button_message, button_none, 39, 1},
    {630, 438, 125, 25, button_message, button_none, 39, 1},
    {630, 468, 125, 25, button_message, button_none, 39, 1},
};

static generic_button buttons_custom_message_text[] = {
    {-40, 48, 125, 25, button_message, button_none, 0, 2},
    {-40, 78, 125, 25, button_message, button_none, 1, 2},
    {-40, 108, 125, 25, button_message, button_none, 2, 2},
    {-40, 138, 125, 25, button_message, button_none, 3, 2},
    {-40, 168, 125, 25, button_message, button_none, 4, 2},
    {-40, 198, 125, 25, button_message, button_none, 5, 2},
    {-40, 228, 125, 25, button_message, button_none, 6, 2},
    {-40, 258, 125, 25, button_message, button_none, 7, 2},
    {-40, 288, 125, 25, button_message, button_none, 8, 2},
    {-40, 318, 125, 25, button_message, button_none, 9, 2},
    {-40, 348, 125, 25, button_message, button_none, 10, 2},
    {-40, 378, 125, 25, button_message, button_none, 11, 2},
    {-40, 408, 125, 25, button_message, button_none, 12, 2},
    {-40, 438, 125, 25, button_message, button_none, 13, 2},
    {-40, 468, 125, 25, button_message, button_none, 14, 2},
    {360, 48, 125, 25, button_message, button_none, 15, 2},
    {360, 78, 125, 25, button_message, button_none, 16, 2},
    {360, 108, 125, 25, button_message, button_none, 17, 2},
    {360, 138, 125, 25, button_message, button_none, 18, 2},
    {360, 168, 125, 25, button_message, button_none, 19, 2},
    {360, 198, 125, 25, button_message, button_none, 20, 2},
    {360, 228, 125, 25, button_message, button_none, 21, 2},
    {360, 258, 125, 25, button_message, button_none, 22, 2},
    {360, 288, 125, 25, button_message, button_none, 23, 2},
    {360, 318, 125, 25, button_message, button_none, 24, 2},
    {360, 348, 125, 25, button_message, button_none, 25, 2},
    {360, 378, 125, 25, button_message, button_none, 26, 2},
    {360, 408, 125, 25, button_message, button_none, 27, 2},
    {360, 438, 125, 25, button_message, button_none, 28, 2},
    {360, 468, 125, 25, button_message, button_none, 29, 2},
    {760, 48, 125, 25, button_message, button_none, 30, 2},
    {760, 78, 125, 25, button_message, button_none, 31, 2},
    {760, 108, 125, 25, button_message, button_none, 32, 2},
    {760, 138, 125, 25, button_message, button_none, 33, 2},
    {760, 168, 125, 25, button_message, button_none, 34, 2},
    {760, 198, 125, 25, button_message, button_none, 35, 2},
    {760, 228, 125, 25, button_message, button_none, 36, 2},
    {760, 258, 125, 25, button_message, button_none, 37, 2},
    {760, 288, 125, 25, button_message, button_none, 38, 2},
    {760, 318, 125, 25, button_message, button_none, 39, 2},
    {760, 348, 125, 25, button_message, button_none, 39, 2},
    {760, 378, 125, 25, button_message, button_none, 39, 2},
    {760, 408, 125, 25, button_message, button_none, 39, 2},
    {760, 438, 125, 25, button_message, button_none, 39, 2},
    {760, 468, 125, 25, button_message, button_none, 39, 2},
};

static int focus_button_id_attr;
static int focus_button_id_title;
static int focus_button_id_text;

uint8_t custom_messages_strings[][19] = {
    "Messages to player", // 0
    "Attributes", // 1
    "Title", // 2
    "Text", // 3
};

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(-320, 0, 77, 32);
    text_draw_centered(custom_messages_strings[0], -320, 16, 1232, FONT_LARGE_BLACK, COLOR_BLACK);

    for (int i = 0; i < MAX_EDITOR_CUSTOM_MESSAGES; i++) {
        int x, y;
        if (i < 15) {
            x = -300;
            y = 48 + 30 * i;
        } else if (i < 30) {
            x = 100;
            y = 48 + 30 * (i - 15);
        } else {
            x = 500;
            y = 48 + 30 * (i - 30);
        }
        button_border_draw(x, y, 125, 25, focus_button_id_attr == i + 1);
        if (scenario.editor_custom_messages[i].enabled) {
            int width = lang_text_draw(25, scenario.editor_custom_messages[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            lang_text_draw_year(scenario.start_year + scenario.editor_custom_messages[i].year, x + width + 6, y + 6, FONT_NORMAL_BLACK);
        } else {
            text_draw_centered(custom_messages_strings[1], x, y + 6, 125, FONT_NORMAL_BLACK, COLOR_BLACK);
        }
        int max_preview_length = 12;
        button_border_draw(x + 130, y, 125, 25, focus_button_id_title == i + 1);
        if (scenario.editor_custom_messages[i].title[0] != '\0') {
            uint8_t title_preview[max_preview_length + 3];
            string_copy(scenario.editor_custom_messages[i].title, title_preview, max_preview_length);
            if (string_length(scenario.editor_custom_messages[i].title) > max_preview_length) {
                title_preview[max_preview_length - 1] = '.';
                title_preview[max_preview_length] = '.';
                title_preview[max_preview_length + 1] = '.';
                title_preview[max_preview_length + 2] = '\0';
            }
            text_draw(title_preview, x + 138, y + 6, FONT_NORMAL_BLACK, COLOR_BLACK);
        } else {
            text_draw_centered(custom_messages_strings[2], x + 130, y + 6, 125, FONT_NORMAL_BLACK, COLOR_BLACK);
        }
        button_border_draw(x + 260, y, 125, 25, focus_button_id_text == i + 1);
        if (scenario.editor_custom_messages[i].text[0] != '\0') {
            uint8_t text_preview[max_preview_length + 3];
            string_copy(scenario.editor_custom_messages[i].text, text_preview, max_preview_length);
            if (string_length(scenario.editor_custom_messages[i].text) > max_preview_length) {
                text_preview[max_preview_length - 1] = '.';
                text_preview[max_preview_length] = '.';
                text_preview[max_preview_length + 1] = '.';
                text_preview[max_preview_length + 2] = '\0';
            }
            text_draw(text_preview, x + 268, y + 6, FONT_NORMAL_BLACK, COLOR_BLACK);
        } else {
            text_draw_centered(custom_messages_strings[3], x + 260, y + 6, 125, FONT_NORMAL_BLACK, COLOR_BLACK);
        }
    }

    graphics_reset_dialog();
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_attributes, sizeof(buttons_attributes) / sizeof(generic_button), &focus_button_id_attr)) {
        return;
    }
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_custom_message_title, sizeof(buttons_custom_message_title) / sizeof(generic_button), &focus_button_id_title)) {
        return;
    }
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_custom_message_text, sizeof(buttons_custom_message_text) / sizeof(generic_button), &focus_button_id_text)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_editor_attributes_show();
    }
}

static void button_message(int id, int category)
{
    window_editor_edit_custom_message_show(id, category);
}

void window_editor_custom_messages_show(void)
{
    window_type window = {
        WINDOW_EDITOR_CUSTOM_MESSAGES,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
        0
    };
    window_show(&window);
}
