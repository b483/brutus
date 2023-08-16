#include "file_dialog.h"

#include "core/calc.h"
#include "core/file.h"
#include "core/image_group.h"
#include "core/lang.h"
#include "core/string.h"
#include "core/time.h"
#include "game/file.h"
#include "game/file_io.h"
#include "game/file_editor.h"
#include "graphics/graphics.h"
#include "editor/editor.h"
#include "sound/sound.h"
#include "widget/input_box.h"
#include "window/city.h"

#include <string.h>

#define NUM_FILES_IN_VIEW 12
#define MAX_FILE_WINDOW_TEXT_WIDTH (18 * BLOCK_SIZE)

static const uint32_t NOT_EXIST_MESSAGE_TIMEOUT = 500;

static void button_ok_cancel(int is_ok, int param2);
static void button_select_file(int index, int param2);
static void on_scroll(void);

static struct image_button_t image_buttons[] = {
    {344, 335, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 0, button_ok_cancel, button_none, 1, 0, 1, 0, 0, 0},
    {392, 335, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 4, button_ok_cancel, button_none, 0, 0, 1, 0, 0, 0},
};
static struct generic_button_t file_buttons[] = {
    {160, 128, 288, 16, button_select_file, button_none, 0, 0},
    {160, 144, 288, 16, button_select_file, button_none, 1, 0},
    {160, 160, 288, 16, button_select_file, button_none, 2, 0},
    {160, 176, 288, 16, button_select_file, button_none, 3, 0},
    {160, 192, 288, 16, button_select_file, button_none, 4, 0},
    {160, 208, 288, 16, button_select_file, button_none, 5, 0},
    {160, 224, 288, 16, button_select_file, button_none, 6, 0},
    {160, 240, 288, 16, button_select_file, button_none, 7, 0},
    {160, 256, 288, 16, button_select_file, button_none, 8, 0},
    {160, 272, 288, 16, button_select_file, button_none, 9, 0},
    {160, 288, 288, 16, button_select_file, button_none, 10, 0},
    {160, 304, 288, 16, button_select_file, button_none, 11, 0},
};

static struct scrollbar_type_t scrollbar = { 464, 120, 206, on_scroll, 0, 0, 0, 0, 0, 0 };

struct file_type_data_t {
    char extension[4];
    char last_loaded_file[FILE_NAME_MAX];
};

static struct {
    uint32_t message_not_exist_start_time;
    int type;
    int dialog_type;
    int focus_button_id;
    int double_click;
    const struct dir_listing *file_list;

    struct file_type_data_t *file_data;
    char typed_name[FILE_NAME_MAX];
    char previously_seen_typed_name[FILE_NAME_MAX];
    char selected_file[FILE_NAME_MAX];
} data;

static struct input_box_t file_name_input = { 144, 80, 20, 2, FONT_NORMAL_WHITE, 0, data.typed_name, FILE_NAME_MAX };

static struct file_type_data_t saved_game_data = { "sav", {0} };
static struct file_type_data_t scenario_data = { "map", {0} };

char *too_many_files_string = "Too many files. Showing 128.";

static int find_first_file_with_prefix(const char *prefix)
{
    int len = string_length(prefix);
    if (len == 0) {
        return -1;
    }
    int left = 0;
    int right = data.file_list->num_files;
    while (left < right) {
        int middle = (left + right) / 2;
        if (strncmp(data.file_list->files[middle], prefix, len) >= 0) {
            right = middle;
        } else {
            left = middle + 1;
        }
    }
    if (strncmp(data.file_list->files[left], prefix, len) == 0) {
        return left;
    } else {
        return -1;
    }
}

static void scroll_to_typed_text(void)
{
    if (data.file_list->num_files <= NUM_FILES_IN_VIEW) {
        // No need to scroll
        return;
    }
    int index = find_first_file_with_prefix((const char *) data.typed_name);
    if (index >= 0) {
        scrollbar_reset(&scrollbar, calc_bound(index, 0, data.file_list->num_files - NUM_FILES_IN_VIEW));
    }
}

static void init(int type, int dialog_type)
{
    data.type = type;
    data.file_data = type == FILE_TYPE_SCENARIO ? &scenario_data : &saved_game_data;
    data.dialog_type = dialog_type;

    data.message_not_exist_start_time = 0;
    data.double_click = 0;
    data.focus_button_id = 0;

    if (string_length(data.file_data->last_loaded_file) > 0) {
        string_copy(data.file_data->last_loaded_file, data.typed_name, FILE_NAME_MAX);
        file_remove_extension(data.typed_name);
    } else if (dialog_type == FILE_DIALOG_SAVE) {
        // Suggest default filename
        string_copy(lang_get_string(9, type == FILE_TYPE_SCENARIO ? 7 : 6), data.typed_name, FILE_NAME_MAX);
    } else {
        // Use empty string
        data.typed_name[0] = 0;
    }
    string_copy(data.typed_name, data.previously_seen_typed_name, FILE_NAME_MAX);

    data.file_list = dir_list_files(data.file_data->extension);
    scrollbar_init(&scrollbar, 0, data.file_list->num_files - NUM_FILES_IN_VIEW);
    scroll_to_typed_text();

    string_copy(data.file_data->last_loaded_file, data.selected_file, FILE_NAME_MAX - 1);
    input_box_start(&file_name_input);
}

static void draw_foreground(void)
{
    graphics_in_dialog();
    char file[FILE_NAME_MAX];

    outer_panel_draw(128, 40, 22, 21);
    input_box_draw(&file_name_input);
    inner_panel_draw(144, 120, 20, 13);

    // title
    if (data.message_not_exist_start_time
        && time_get_millis() - data.message_not_exist_start_time < NOT_EXIST_MESSAGE_TIMEOUT) {
        lang_text_draw_centered(43, 2, 160, 50, 304, FONT_LARGE_BLACK);
    } else if (data.dialog_type == FILE_DIALOG_DELETE) {
        lang_text_draw_centered(43, 6, 160, 50, 304, FONT_LARGE_BLACK);
    } else {
        int text_id = data.dialog_type + (data.type == FILE_TYPE_SCENARIO ? 3 : 0);
        lang_text_draw_centered(43, text_id, 160, 50, 304, FONT_LARGE_BLACK);
    }
    lang_text_draw(43, 5, 224, 342, FONT_NORMAL_BLACK);

    for (int i = 0; i < NUM_FILES_IN_VIEW; i++) {
        if (i >= data.file_list->num_files) {
            break;
        }
        int font = FONT_NORMAL_GREEN;
        if (data.focus_button_id == i + 1) {
            font = FONT_NORMAL_WHITE;
        }
        string_copy(data.file_list->files[scrollbar.scroll_position + i], file, FILE_NAME_MAX);
        file_remove_extension(file);
        text_ellipsize(file, font, MAX_FILE_WINDOW_TEXT_WIDTH);
        text_draw(file, 160, 130 + 16 * i, font, 0);
    }
    if (data.file_list->file_overflow) {
        inner_panel_draw(184, 22, 15, 1);
        text_draw_centered(too_many_files_string, 184, 25, 240, FONT_NORMAL_PLAIN, COLOR_RED);
    }
    image_buttons_draw(0, 0, image_buttons, 2);
    scrollbar_draw(&scrollbar);

    graphics_reset_dialog();
}

static int should_scroll_to_typed_text(void)
{
    if (string_equals(data.previously_seen_typed_name, data.typed_name)) {
        return 0;
    }
    int scroll = 0;
    // Only scroll when adding characters to the typed name
    if (string_length(data.typed_name) > string_length(data.previously_seen_typed_name)) {
        scroll = 1;
    }
    string_copy(data.typed_name, data.previously_seen_typed_name, FILE_NAME_MAX);
    return scroll;
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    data.double_click = m->left.double_click;

    if (input_box_is_accepted(&file_name_input)) {
        button_ok_cancel(1, 0);
        return;
    }

    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, file_buttons, NUM_FILES_IN_VIEW, &data.focus_button_id) ||
        image_buttons_handle_mouse(m_dialog, 0, 0, image_buttons, 2, 0) ||
        scrollbar_handle_mouse(&scrollbar, m_dialog)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        input_box_stop(&file_name_input);
        window_go_back();
    }

    if (should_scroll_to_typed_text()) {
        scroll_to_typed_text();
    }
}

static const char *get_chosen_filename(void)
{
    char selected_name[FILE_NAME_MAX];
    string_copy(data.selected_file, selected_name, FILE_NAME_MAX);
    file_remove_extension(selected_name);

    if (string_equals(selected_name, data.typed_name)) {
        // User has not modified the string after selecting it: use filename
        return data.selected_file;
    }

    // We should use the typed name
    static char typed_file[FILE_NAME_MAX];
    string_copy(data.typed_name, typed_file, FILE_NAME_MAX);
    file_append_extension(typed_file, data.file_data->extension);
    return typed_file;
}

static void button_ok_cancel(int is_ok, __attribute__((unused)) int param2)
{
    if (!is_ok) {
        input_box_stop(&file_name_input);
        window_go_back();
        return;
    }

    const char *filename = get_chosen_filename();

    if (data.type == FILE_TYPE_SAVED_GAME) {
        if (data.dialog_type != FILE_DIALOG_SAVE && !file_exists(SAVES_DIR_PATH, filename)) {
            data.message_not_exist_start_time = time_get_millis();
            return;
        }

        if (data.dialog_type == FILE_DIALOG_SAVE) {
            input_box_stop(&file_name_input);
            game_file_io_write_saved_game(SAVES_DIR_PATH, filename);
            window_city_show();
        } else if (data.dialog_type == FILE_DIALOG_LOAD) {
            if (game_file_load_saved_game(SAVES_DIR_PATH, filename)) {
                input_box_stop(&file_name_input);
                window_city_show();
            } else {
                data.message_not_exist_start_time = time_get_millis();
                return;
            }
        } else if (data.dialog_type == FILE_DIALOG_DELETE) {
            if (game_file_io_delete_saved_game(filename)) {
                dir_list_files(data.file_data->extension);
                if (scrollbar.scroll_position + NUM_FILES_IN_VIEW >= data.file_list->num_files) {
                    --scrollbar.scroll_position;
                }
                if (scrollbar.scroll_position < 0) {
                    scrollbar.scroll_position = 0;
                }
            }
        }
    } else if (data.type == FILE_TYPE_SCENARIO) {
        if (data.dialog_type != FILE_DIALOG_SAVE && !file_exists(MAPS_DIR_PATH, filename)) {
            data.message_not_exist_start_time = time_get_millis();
            return;
        }

        if (data.dialog_type == FILE_DIALOG_SAVE) {
            input_box_stop(&file_name_input);
            game_file_editor_write_scenario(MAPS_DIR_PATH, filename);
            show_editor_map();
        } else if (data.dialog_type == FILE_DIALOG_LOAD) {
            if (game_file_editor_load_scenario(MAPS_DIR_PATH, filename)) {
                input_box_stop(&file_name_input);
                show_editor_map();
            } else {
                data.message_not_exist_start_time = time_get_millis();
                return;
            }
        }
    }
    play_sound_effect(SOUND_EFFECT_ICON);
    string_copy(filename, data.file_data->last_loaded_file, FILE_NAME_MAX - 1);
}

static void on_scroll(void)
{
    data.message_not_exist_start_time = 0;
}

static void button_select_file(int index, __attribute__((unused)) int param2)
{
    if (index < data.file_list->num_files) {
        string_copy(data.file_list->files[scrollbar.scroll_position + index], data.selected_file, FILE_NAME_MAX - 1);
        string_copy(data.selected_file, data.typed_name, FILE_NAME_MAX);
        file_remove_extension(data.typed_name);
        string_copy(data.typed_name, data.previously_seen_typed_name, FILE_NAME_MAX);
        input_box_refresh_text(&file_name_input);
        data.message_not_exist_start_time = 0;
    }
    if (data.dialog_type != FILE_DIALOG_DELETE && data.double_click) {
        data.double_click = 0;
        button_ok_cancel(1, 0);
    }
}

void window_file_dialog_show(int type, int dialog_type)
{
    struct window_type_t window = {
        WINDOW_FILE_DIALOG,
        0,
        draw_foreground,
        handle_input,
    };
    init(type, dialog_type);
    window_show(&window);
}
