#include "top_menu_editor.h"

#include "game/file_editor.h"
#include "game/game.h"
#include "game/system.h"
#include "graphics/image.h"
#include "graphics/menu.h"
#include "graphics/screen.h"
#include "graphics/window.h"
#include "scenario/data.h"
#include "widget/map_editor.h"
#include "window/display_options.h"
#include "window/file_dialog.h"
#include "window/message_dialog.h"
#include "window/popup_dialog.h"
#include "window/select_list.h"
#include "window/sound_options.h"
#include "window/speed_options.h"
#include "window/editor/map.h"

static void menu_file_new_map(int param);
static void menu_file_load_map(int param);
static void menu_file_save_map(int param);
static void menu_file_exit_editor(int param);

static void menu_options_display(int param);
static void menu_options_sound(int param);
static void menu_options_speed(int param);

static void menu_help_help(int param);
static void menu_help_about(int param);

static void menu_resets_herds(int param);
static void menu_resets_fish(int param);
static void menu_resets_invasions(int param);

static menu_item menu_file[] = {
    {7, 1, menu_file_new_map, 0, 0},
    {7, 2, menu_file_load_map, 0, 0},
    {7, 3, menu_file_save_map, 0, 0},
    {7, 4, menu_file_exit_editor, 0, 0},
};

static menu_item menu_options[] = {
    {2, 1, menu_options_display, 0, 0},
    {2, 2, menu_options_sound, 0, 0},
    {2, 3, menu_options_speed, 0, 0},
};

static menu_item menu_help[] = {
    {3, 1, menu_help_help, 0, 0},
    {3, 7, menu_help_about, 0, 0},
};

static menu_item menu_resets[] = {
    {10, 1, menu_resets_herds, 0, 0},
    {10, 2, menu_resets_fish, 0, 0},
    {10, 3, menu_resets_invasions, 0, 0},
};

static menu_bar_item top_menu_editor[] = {
    {7, menu_file, 4, 0, 0, 0, 0},
    {2, menu_options, 3, 0, 0, 0, 0},
    {3, menu_help, 2, 0, 0, 0, 0},
    {10, menu_resets, 3, 0, 0, 0, 0},
};

#define INDEX_OPTIONS 1

static struct {
    int open_sub_menu;
    int focus_menu_id;
    int focus_sub_menu_id;
} data;

static void clear_state(void)
{
    data.open_sub_menu = 0;
    data.focus_menu_id = 0;
    data.focus_sub_menu_id = 0;
}

static void init(void)
{
    top_menu_editor[INDEX_OPTIONS].items[0].hidden = 0;
}

static void draw_foreground(void)
{
    if (!data.open_sub_menu) {
        return;
    }
    menu_draw(&top_menu_editor[data.open_sub_menu - 1], data.focus_sub_menu_id);
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    widget_top_menu_editor_handle_input(m, h);
}

static void top_menu_window_show(void)
{
    window_type window = {
        WINDOW_EDITOR_TOP_MENU,
        window_editor_map_draw_all,
        draw_foreground,
        handle_input,
        0
    };
    init();
    window_show(&window);
}

void widget_top_menu_editor_draw(void)
{
    int block_width = 24;
    int image_base = image_group(GROUP_TOP_MENU);
    int s_width = screen_width();
    for (int i = 0; i * block_width < s_width; i++) {
        image_draw(image_base + i % 8, i * block_width, 0);
    }
    menu_bar_draw(top_menu_editor, sizeof(top_menu_editor) / sizeof(menu_bar_item), s_width);
}

static int handle_input_submenu(const mouse *m, const hotkeys *h)
{
    if (m->right.went_up || h->escape_pressed) {
        clear_state();
        window_go_back();
        return 1;
    }
    int menu_id = menu_bar_handle_mouse(m, top_menu_editor, sizeof(top_menu_editor) / sizeof(menu_bar_item), &data.focus_menu_id);
    if (menu_id && menu_id != data.open_sub_menu) {
        window_request_refresh();
        data.open_sub_menu = menu_id;
    }
    if (!menu_handle_mouse(m, &top_menu_editor[data.open_sub_menu - 1], &data.focus_sub_menu_id)) {
        if (m->left.went_up) {
            clear_state();
            window_go_back();
            return 1;
        }
    }
    return 0;
}

static int handle_mouse_menu(const mouse *m)
{
    int menu_id = menu_bar_handle_mouse(m, top_menu_editor, sizeof(top_menu_editor) / sizeof(menu_bar_item), &data.focus_menu_id);
    if (menu_id && m->left.went_up) {
        data.open_sub_menu = menu_id;
        top_menu_window_show();
        return 1;
    }
    return 0;
}

int widget_top_menu_editor_handle_input(const mouse *m, const hotkeys *h)
{
    if (data.open_sub_menu) {
        return handle_input_submenu(m, h);
    } else {
        return handle_mouse_menu(m);
    }
}

static void map_size_selected(int size)
{
    clear_state();
    if (size >= 0 && size <= 5) {
        game_file_editor_create_scenario(size);
        window_editor_map_show();
    } else {
        window_go_back();
    }
}

static void menu_file_new_map(__attribute__((unused)) int param)
{
    window_select_list_show(50, 50, 33, 7, map_size_selected);
}

static void menu_file_load_map(__attribute__((unused)) int param)
{
    clear_state();
    window_editor_map_show();
    window_file_dialog_show(FILE_TYPE_SCENARIO, FILE_DIALOG_LOAD);
}

static void menu_file_save_map(__attribute__((unused)) int param)
{
    clear_state();
    window_editor_map_show();
    window_file_dialog_show(FILE_TYPE_SCENARIO, FILE_DIALOG_SAVE);
}

static void menu_file_exit_editor(__attribute__((unused)) int param)
{
    clear_state();
    request_exit_editor();
}

static void menu_options_display(__attribute__((unused)) int param)
{
    clear_state();
    window_editor_map_show();
    window_display_options_show(window_editor_map_show);
}

static void menu_options_sound(__attribute__((unused)) int param)
{
    clear_state();
    window_editor_map_show();
    window_sound_options_show(1);
}

static void menu_options_speed(__attribute__((unused)) int param)
{
    clear_state();
    window_editor_map_show();
    window_speed_options_show(1);
}

static void menu_help_help(__attribute__((unused)) int param)
{
    clear_state();
    window_go_back();
    window_message_dialog_show(MESSAGE_DIALOG_EDITOR_HELP, window_editor_map_draw_all);
}

static void menu_help_about(__attribute__((unused)) int param)
{
    clear_state();
    window_go_back();
    window_message_dialog_show(MESSAGE_DIALOG_EDITOR_ABOUT, window_editor_map_draw_all);
}

static void menu_resets_herds(__attribute__((unused)) int param)
{
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        scenario.herd_points[i].x = -1;
        scenario.herd_points[i].y = -1;
    }
    scenario.is_saved = 0;
    clear_state();
    window_go_back();
}

static void menu_resets_fish(__attribute__((unused)) int param)
{
    for (int i = 0; i < MAX_FISH_POINTS; i++) {
        scenario.fishing_points[i].x = -1;
        scenario.fishing_points[i].y = -1;
    }
    scenario.is_saved = 0;
    clear_state();
    window_go_back();
}

static void menu_resets_invasions(__attribute__((unused)) int param)
{
    for (int i = 0; i < MAX_INVASION_POINTS; i++) {
        scenario.invasion_points[i].x = -1;
        scenario.invasion_points[i].y = -1;
    }
    scenario.is_saved = 0;
    clear_state();
    window_go_back();
}
