#include "top_menu.h"

#include "building/construction.h"
#include "city/finance.h"
#include "city/population.h"
#include "game/settings.h"
#include "game/state.h"
#include "game/system.h"
#include "game/time.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/menu.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "widget/city.h"
#include "window/city.h"
#include "window/difficulty_options.h"
#include "window/display_options.h"
#include "window/file_dialog.h"
#include "window/message_dialog.h"
#include "window/sound_options.h"
#include "window/speed_options.h"

enum {
    INFO_NONE = 0,
    INFO_FUNDS = 1,
    INFO_POPULATION = 2,
    INFO_DATE = 3
};

static void menu_file_replay_map(int param);
static void menu_file_load_game(int param);
static void menu_file_save_game(int param);
static void menu_file_delete_game(int param);
static void menu_file_exit_game(int param);

static void menu_options_display(int param);
static void menu_options_sound(int param);
static void menu_options_speed(int param);
static void menu_options_difficulty(int param);
static void menu_options_autosave(int param);

static void menu_help_help(int param);
static void menu_help_mouse_help(int param);
static void menu_help_warnings(int param);
static void menu_help_about(int param);

static menu_item menu_file[] = {
    {1, 2, menu_file_replay_map, 0, 0},
    {1, 3, menu_file_load_game, 0, 0},
    {1, 4, menu_file_save_game, 0, 0},
    {1, 6, menu_file_delete_game, 0, 0},
    {1, 5, menu_file_exit_game, 0, 0},
};

static menu_item menu_options[] = {
    {2, 1, menu_options_display, 0, 0},
    {2, 2, menu_options_sound, 0, 0},
    {2, 3, menu_options_speed, 0, 0},
    {2, 6, menu_options_difficulty, 0, 0},
    {19, 51, menu_options_autosave, 0, 0},
};

static menu_item menu_help[] = {
    {3, 1, menu_help_help, 0, 0},
    {3, 2, menu_help_mouse_help, 0, 0},
    {3, 5, menu_help_warnings, 0, 0},
    {3, 7, menu_help_about, 0, 0},
};

static menu_bar_item menu[] = {
    {1, menu_file, 5, 0, 0, 0, 0},
    {2, menu_options, 5, 0, 0, 0, 0},
    {3, menu_help, 4, 0, 0, 0, 0},
};

static const int INDEX_OPTIONS = 1;
static const int INDEX_HELP = 2;

static struct {
    int offset_funds;
    int offset_population;
    int offset_date;

    int open_sub_menu;
    int focus_menu_id;
    int focus_sub_menu_id;
} data;

static struct {
    int population;
    int treasury;
    int month;
} drawn;

static void clear_state(void)
{
    data.open_sub_menu = 0;
    data.focus_menu_id = 0;
    data.focus_sub_menu_id = 0;
}

static void set_text_for_autosave(void)
{
    menu_update_text(&menu[INDEX_OPTIONS], 4, setting_monthly_autosave() ? 51 : 52);
}

static void set_text_for_tooltips(void)
{
    int new_text;
    switch (setting_tooltips()) {
        case TOOLTIPS_NONE:
            new_text = 2;
            break;
        case TOOLTIPS_SOME:
            new_text = 3;
            break;
        case TOOLTIPS_FULL:
            new_text = 4;
            break;
        default:
            return;
    }
    menu_update_text(&menu[INDEX_HELP], 1, new_text);
}

static void set_text_for_warnings(void)
{
    menu_update_text(&menu[INDEX_HELP], 2, setting_warnings() ? 6 : 5);
}

static void init(void)
{
    menu[INDEX_OPTIONS].items[0].hidden = 0;
    set_text_for_autosave();
    set_text_for_tooltips();
    set_text_for_warnings();
}

static void draw_background(void)
{
    window_city_draw_panels();
    window_city_draw();
}

static void draw_foreground(void)
{
    if (data.open_sub_menu) {
        menu_draw(&menu[data.open_sub_menu - 1], data.focus_sub_menu_id);
    }
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    widget_top_menu_handle_input(m, h);
}

static void top_menu_window_show(void)
{
    window_type window = {
        WINDOW_TOP_MENU,
        draw_background,
        draw_foreground,
        handle_input,
        0
    };
    init();
    window_show(&window);
}

static void refresh_background(void)
{
    int block_width = 24;
    int image_base = image_group(GROUP_TOP_MENU);
    int s_width = screen_width();
    for (int i = 0; i * block_width < s_width; i++) {
        image_draw(image_base + i % 8, i * block_width, 0);
    }
    // black panels for funds/pop/time
    if (s_width < 800) {
        image_draw(image_base + 14, 336, 0);
    } else if (s_width < 1024) {
        image_draw(image_base + 14, 336, 0);
        image_draw(image_base + 14, 456, 0);
        image_draw(image_base + 14, 648, 0);
    } else {
        image_draw(image_base + 14, 480, 0);
        image_draw(image_base + 14, 624, 0);
        image_draw(image_base + 14, 840, 0);
    }
}

void widget_top_menu_draw(int force)
{
    if (!force && drawn.treasury == city_finance_treasury() &&
        drawn.population == city_population() &&
        drawn.month == game_time_month()) {
        return;
    }

    int s_width = screen_width();

    refresh_background();
    menu_bar_draw(menu, sizeof(menu) / sizeof(menu_bar_item), s_width < 1024 ? 338 : 493);

    color_t treasure_color = COLOR_WHITE;
    int treasury = city_finance_treasury();
    if (treasury < 0) {
        treasure_color = COLOR_FONT_RED;
    }
    if (s_width < 800) {
        data.offset_funds = 338;
        data.offset_population = 453;
        data.offset_date = 547;

        int width = lang_text_draw_colored(6, 0, 350, 5, FONT_NORMAL_PLAIN, treasure_color);
        text_draw_number_colored(treasury, '@', " ", 346 + width, 5, FONT_NORMAL_PLAIN, treasure_color);

        width = lang_text_draw(6, 1, 458, 5, FONT_NORMAL_GREEN);
        text_draw_number(city_population(), '@', " ", 450 + width, 5, FONT_NORMAL_GREEN);

        lang_text_draw_month_year_max_width(game_time_month(), game_time_year(), 540, 5, 100, FONT_NORMAL_GREEN, 0);
    } else if (s_width < 1024) {
        data.offset_funds = 338;
        data.offset_population = 458;
        data.offset_date = 652;

        int width = lang_text_draw_colored(6, 0, 350, 5, FONT_NORMAL_PLAIN, treasure_color);
        text_draw_number_colored(treasury, '@', " ", 346 + width, 5, FONT_NORMAL_PLAIN, treasure_color);

        width = lang_text_draw_colored(6, 1, 470, 5, FONT_NORMAL_PLAIN, COLOR_WHITE);
        text_draw_number_colored(city_population(), '@', " ", 466 + width, 5, FONT_NORMAL_PLAIN, COLOR_WHITE);

        lang_text_draw_month_year_max_width(game_time_month(), game_time_year(),
            655, 5, 110, FONT_NORMAL_PLAIN, COLOR_FONT_YELLOW);
    } else {
        data.offset_funds = 493;
        data.offset_population = 637;
        data.offset_date = 852;

        int width = lang_text_draw_colored(6, 0, 495, 5, FONT_NORMAL_PLAIN, treasure_color);
        text_draw_number_colored(treasury, '@', " ", 501 + width, 5, FONT_NORMAL_PLAIN, treasure_color);

        width = lang_text_draw_colored(6, 1, 645, 5, FONT_NORMAL_PLAIN, COLOR_WHITE);
        text_draw_number_colored(city_population(), '@', " ", 651 + width, 5, FONT_NORMAL_PLAIN, COLOR_WHITE);

        lang_text_draw_month_year_max_width(game_time_month(), game_time_year(),
            850, 5, 110, FONT_NORMAL_PLAIN, COLOR_FONT_YELLOW);
    }
    drawn.treasury = treasury;
    drawn.population = city_population();
    drawn.month = game_time_month();
}

static int handle_input_submenu(const mouse *m, const hotkeys *h)
{
    if (m->right.went_up || h->escape_pressed) {
        clear_state();
        window_go_back();
        return 1;
    }
    int menu_id = menu_bar_handle_mouse(m, menu, 4, &data.focus_menu_id);
    if (menu_id && menu_id != data.open_sub_menu) {
        window_request_refresh();
        data.open_sub_menu = menu_id;
    }
    if (!menu_handle_mouse(m, &menu[data.open_sub_menu - 1], &data.focus_sub_menu_id)) {
        if (m->left.went_up) {
            clear_state();
            window_go_back();
            return 1;
        }
    }
    return 0;
}

static int get_info_id(int mouse_x, int mouse_y)
{
    if (mouse_y < 4 || mouse_y >= 18) {
        return INFO_NONE;
    }
    if (mouse_x > data.offset_funds && mouse_x < data.offset_funds + 128) {
        return INFO_FUNDS;
    }
    if (mouse_x > data.offset_population && mouse_x < data.offset_population + 128) {
        return INFO_POPULATION;
    }
    if (mouse_x > data.offset_date && mouse_x < data.offset_date + 128) {
        return INFO_DATE;
    }
    return INFO_NONE;
}

static int handle_right_click(int type)
{
    if (type == INFO_NONE) {
        return 0;
    }
    if (type == INFO_FUNDS) {
        window_message_dialog_show(MESSAGE_DIALOG_TOP_FUNDS, window_city_draw_all);
    } else if (type == INFO_POPULATION) {
        window_message_dialog_show(MESSAGE_DIALOG_TOP_POPULATION, window_city_draw_all);
    } else if (type == INFO_DATE) {
        window_message_dialog_show(MESSAGE_DIALOG_TOP_DATE, window_city_draw_all);
    }
    return 1;
}

static int handle_mouse_menu(const mouse *m)
{
    int menu_id = menu_bar_handle_mouse(m, menu, 4, &data.focus_menu_id);
    if (menu_id && m->left.went_up) {
        data.open_sub_menu = menu_id;
        top_menu_window_show();
        return 1;
    }
    if (m->right.went_up) {
        return handle_right_click(get_info_id(m->x, m->y));
    }
    return 0;
}

int widget_top_menu_handle_input(const mouse *m, const hotkeys *h)
{
    if (widget_city_has_input()) {
        return 0;
    }
    if (data.open_sub_menu) {
        return handle_input_submenu(m, h);
    } else {
        return handle_mouse_menu(m);
    }
}

int widget_top_menu_get_tooltip_text(tooltip_context *c)
{
    if (data.focus_menu_id) {
        return 49 + data.focus_menu_id;
    }
    int button_id = get_info_id(c->mouse_x, c->mouse_y);
    if (button_id) {
        return 59 + button_id;
    }
    return 0;
}

static void menu_file_replay_map(__attribute__((unused)) int param)
{
    clear_state();
    replay_map();
}

static void menu_file_load_game(__attribute__((unused)) int param)
{
    clear_state();
    building_construction_clear_type();
    window_go_back();
    window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_LOAD);
}

static void menu_file_save_game(__attribute__((unused)) int param)
{
    clear_state();
    window_go_back();
    window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_SAVE);
}

static void menu_file_delete_game(__attribute__((unused)) int param)
{
    clear_state();
    window_go_back();
    window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_DELETE);
}

static void menu_file_exit_game(__attribute__((unused)) int param)
{
    clear_state();
    request_exit_scenario();
}

static void menu_options_display(__attribute__((unused)) int param)
{
    clear_state();
    window_display_options_show(window_city_return);
}

static void menu_options_sound(__attribute__((unused)) int param)
{
    clear_state();
    window_sound_options_show(window_city_return);
}

static void menu_options_speed(__attribute__((unused)) int param)
{
    clear_state();
    window_speed_options_show(window_city_return);
}

static void menu_options_difficulty(__attribute__((unused)) int param)
{
    clear_state();
    window_difficulty_options_show(window_city_return);
}

static void menu_options_autosave(__attribute__((unused)) int param)
{
    setting_toggle_monthly_autosave();
    set_text_for_autosave();
}

static void menu_help_help(__attribute__((unused)) int param)
{
    clear_state();
    window_go_back();
    window_message_dialog_show(MESSAGE_DIALOG_HELP, window_city_draw_all);
}

static void menu_help_mouse_help(__attribute__((unused)) int param)
{
    setting_cycle_tooltips();
    set_text_for_tooltips();
}

static void menu_help_warnings(__attribute__((unused)) int param)
{
    setting_toggle_warnings();
    set_text_for_warnings();
}

static void menu_help_about(__attribute__((unused)) int param)
{
    clear_state();
    window_go_back();
    window_message_dialog_show(MESSAGE_DIALOG_ABOUT, window_city_draw_all);
}
