#include "invasions.h"

#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "window/editor/attributes.h"
#include "window/editor/edit_invasion.h"
#include "window/editor/map.h"

static void button_invasion(int id, int param2);

static struct generic_button_t buttons_invasions[] = {
    {-300, 48, 290, 25, button_invasion, button_none, 0, 0},
    {-300, 78, 290, 25, button_invasion, button_none, 1, 0},
    {-300, 108, 290, 25, button_invasion, button_none, 2, 0},
    {-300, 138, 290, 25, button_invasion, button_none, 3, 0},
    {-300, 168, 290, 25, button_invasion, button_none, 4, 0},
    {-300, 198, 290, 25, button_invasion, button_none, 5, 0},
    {-300, 228, 290, 25, button_invasion, button_none, 6, 0},
    {-300, 258, 290, 25, button_invasion, button_none, 7, 0},
    {-300, 288, 290, 25, button_invasion, button_none, 8, 0},
    {-300, 318, 290, 25, button_invasion, button_none, 9, 0},
    {0, 48, 290, 25, button_invasion, button_none, 10, 0},
    {0, 78, 290, 25, button_invasion, button_none, 11, 0},
    {0, 108, 290, 25, button_invasion, button_none, 12, 0},
    {0, 138, 290, 25, button_invasion, button_none, 13, 0},
    {0, 168, 290, 25, button_invasion, button_none, 14, 0},
    {0, 198, 290, 25, button_invasion, button_none, 15, 0},
    {0, 228, 290, 25, button_invasion, button_none, 16, 0},
    {0, 258, 290, 25, button_invasion, button_none, 17, 0},
    {0, 288, 290, 25, button_invasion, button_none, 18, 0},
    {0, 318, 290, 25, button_invasion, button_none, 19, 0},
    {300, 48, 290, 25, button_invasion, button_none, 20, 0},
    {300, 78, 290, 25, button_invasion, button_none, 21, 0},
    {300, 108, 290, 25, button_invasion, button_none, 22, 0},
    {300, 138, 290, 25, button_invasion, button_none, 23, 0},
    {300, 168, 290, 25, button_invasion, button_none, 24, 0},
    {300, 198, 290, 25, button_invasion, button_none, 25, 0},
    {300, 228, 290, 25, button_invasion, button_none, 26, 0},
    {300, 258, 290, 25, button_invasion, button_none, 27, 0},
    {300, 288, 290, 25, button_invasion, button_none, 28, 0},
    {300, 318, 290, 25, button_invasion, button_none, 29, 0},
    {600, 48, 290, 25, button_invasion, button_none, 30, 0},
    {600, 78, 290, 25, button_invasion, button_none, 31, 0},
    {600, 108, 290, 25, button_invasion, button_none, 32, 0},
    {600, 138, 290, 25, button_invasion, button_none, 33, 0},
    {600, 168, 290, 25, button_invasion, button_none, 34, 0},
    {600, 198, 290, 25, button_invasion, button_none, 35, 0},
    {600, 228, 290, 25, button_invasion, button_none, 36, 0},
    {600, 258, 290, 25, button_invasion, button_none, 37, 0},
    {600, 288, 290, 25, button_invasion, button_none, 38, 0},
    {600, 318, 290, 25, button_invasion, button_none, 39, 0},
};

static int focus_button_id;

uint8_t invasions_enemy_type_strings[][14] = {
    "Barbarians", // 0
    "Carthaginians", // 1
    "Britons", // 2
    "Celts", // 3
    "Picts", // 4
    "Egyptians", // 5
    "Etruscans", // 6
    "Samnites", // 7
    "Gauls", // 8
    "Helvetii", // 9
    "Huns", // 10
    "Goths", // 11
    "Visigoths", // 12
    "Graeci", // 13
    "Macedonians", // 14
    "Numidians", // 15
    "Pergamum", // 16
    "Iberians", // 17
    "Judaeans", // 18
    "Seleucids", // 19
};

uint8_t invasions_enemy_army_type_strings[][17] = {
    "No invaders", // 0
    "Local raiders", // 1
    "Enemy army", // 2
    "Caesar's legions", // 3
    "Distant battle", // 4
};

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(-320, 0, 77, 26);
    lang_text_draw_centered(44, 15, -320, 16, 1232, FONT_LARGE_BLACK);

    for (int i = 0; i < MAX_INVASIONS; i++) {
        int x, y;
        if (i < 10) {
            x = -300;
            y = 48 + 30 * i;
        } else if (i < 20) {
            x = 0;
            y = 48 + 30 * (i - 10);
        } else if (i < 30) {
            x = 300;
            y = 48 + 30 * (i - 20);
        } else {
            x = 600;
            y = 48 + 30 * (i - 30);
        }
        button_border_draw(x, y, 290, 25, focus_button_id == i + 1);
        if (scenario.invasions[i].type) {
            int width = lang_text_draw(25, scenario.invasions[i].month, x + 12, y + 6, FONT_NORMAL_BLACK);
            width += lang_text_draw_year(scenario.start_year + scenario.invasions[i].year_offset, x + 6 + width, y + 6, FONT_NORMAL_BLACK);
            width += text_draw_number(scenario.invasions[i].amount, 0, 0, x + 12 + width, y + 6, FONT_NORMAL_BLACK);
            if (scenario.invasions[i].type == INVASION_TYPE_ENEMY_ARMY) {
                uint8_t *enemy_type_text = invasions_enemy_type_strings[scenario.invasions[i].enemy_type];
                text_draw(enemy_type_text, x - 12 + width + (290 - width - text_get_width(enemy_type_text, FONT_NORMAL_BLACK)), y + 6, FONT_NORMAL_BLACK, COLOR_BLACK);
            } else {
                uint8_t *invasions_type_text = invasions_enemy_army_type_strings[scenario.invasions[i].type];
                text_draw(invasions_type_text, x - 12 + width + (290 - width - text_get_width(invasions_type_text, FONT_NORMAL_BLACK)), y + 6, FONT_NORMAL_BLACK, COLOR_BLACK);
            }
        } else {
            lang_text_draw_centered(44, 23, x, y + 6, 290, FONT_NORMAL_BLACK);
        }
    }

    // invasions hint
    lang_text_draw_multiline(152, 2, -280, 360, 1200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, buttons_invasions, sizeof(buttons_invasions) / sizeof(struct generic_button_t), &focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_editor_attributes_show();
    }
}

static void button_invasion(int id, __attribute__((unused)) int param2)
{
    window_editor_edit_invasion_show(id);
}

void window_editor_invasions_show(void)
{
    struct window_type_t window = {
        WINDOW_EDITOR_INVASIONS,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
    };
    window_show(&window);
}
