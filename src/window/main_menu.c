#include "main_menu.h"

#include "core/file.h"
#include "core/image.h"
#include "core/string.h"
#include "editor/editor.h"
#include "game/game.h"
#include "game/settings.h"
#include "graphics/graphics.h"
#include "platform/brutus.h"
#include "sound/sound.h"
#include "window/cck_selection.h"
#include "window/config.h"
#include "window/file_dialog.h"
#include "window/plain_message_dialog.h"

#define MAX_EDITOR_FILES 9

#define MAX_BUTTONS 5

static const char EDITOR_FILES[MAX_EDITOR_FILES][32] = {
    "c3_map.eng",
    "c3_map_mm.eng",
    "c3map.sg2",
    "c3map.555",
    "c3map_north.sg2",
    "c3map_north.555",
    "c3map_south.sg2",
    "c3map_south.555",
    "map_panels.555"
};

static void button_click(int type, int param2);

static int focus_button_id;

static struct generic_button_t buttons[] = {
    {192, 140, 256, 25, button_click, button_none, 1, 0},
    {192, 180, 256, 25, button_click, button_none, 2, 0},
    {192, 220, 256, 25, button_click, button_none, 3, 0},
    {192, 260, 256, 25, button_click, button_none, 4, 0},
    {192, 300, 256, 25, button_click, button_none, 5, 0},
};

void draw_version_string(void)
{
    char version_string[100] = "Brutus v";
    int text_y = screen_height() - 30;

    string_copy(string_from_ascii(system_version()), version_string + string_length(version_string), 99);

    int text_width = text_get_width(version_string, FONT_SMALL_PLAIN);

    if (text_y <= 500 && (screen_width() - 640) / 2 < text_width + 18) {
        graphics_draw_rect(10, text_y, text_width + 14, 20, COLOR_BLACK);
        graphics_fill_rect(11, text_y + 1, text_width + 12, 18, COLOR_WHITE);
        text_draw(version_string, 18, text_y + 6, FONT_SMALL_PLAIN, COLOR_BLACK);
    } else {
        text_draw(version_string, 18, text_y + 6, FONT_SMALL_PLAIN, COLOR_FONT_LIGHT_GRAY);
    }
}

static void draw_background(void)
{
    graphics_clear_screen();
    image_draw_fullscreen_background(image_group(GROUP_INTERMEZZO_BACKGROUND) + 16);
    draw_version_string();
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    for (int i = 0; i < MAX_BUTTONS; i++) {
        large_label_draw(buttons[i].x, buttons[i].y, buttons[i].width / BLOCK_SIZE, focus_button_id == i + 1 ? 1 : 0);
    }

    lang_text_draw_centered(30, 3, 192, 146, 256, FONT_NORMAL_GREEN);
    lang_text_draw_centered(30, 2, 192, 186, 256, FONT_NORMAL_GREEN);
    lang_text_draw_centered(9, 8, 192, 226, 256, FONT_NORMAL_GREEN);
    lang_text_draw_centered(2, 0, 192, 266, 256, FONT_NORMAL_GREEN);
    lang_text_draw_centered(30, 5, 192, 306, 256, FONT_NORMAL_GREEN);

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons, MAX_BUTTONS, &focus_button_id)) {
        return;
    }
    if (h->escape_pressed) {
        post_event(USER_EVENT_QUIT);
    }
    if (h->load_file) {
        window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_LOAD);
    }
}

static void button_click(int type, __attribute__((unused)) int param2)
{
    if (type == 1) {
        window_cck_selection_show();
    } else if (type == 2) {
        window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_LOAD);
    } else if (type == 3) {
        int editor_present = 1;
        for (int i = 0; i < MAX_EDITOR_FILES; i++) {
            if (!file_exists(0, EDITOR_FILES[i])) {
                editor_present = 0;
                break;
            }
        }
        if (!editor_present || !game_init_editor()) {
            window_plain_message_dialog_show("Editor not installed", "Your Caesar 3 installation does not contain the editor files.\n\
                You can download them from: https://github.com/bvschaik/julius/wiki/Editor");
        } else {
            update_music(1);
        }
    } else if (type == 4) {
        window_config_show();
    } else if (type == 5) {
        post_event(USER_EVENT_QUIT);
    }
}

void window_main_menu_show(int restart_music)
{
    if (restart_music) {
        play_intro_music();
    }
    struct window_type_t window = {
        WINDOW_MAIN_MENU,
        draw_background,
        draw_foreground,
        handle_input,
    };
    window_show(&window);
}
