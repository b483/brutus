#include "cck_selection.h"

#include "core/file.h"
#include "core/image_group.h"
#include "core/string.h"
#include "editor/editor.h"
#include "game/file.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/scrollbar.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "scenario/scenario.h"
#include "sound/sound.h"
#include "widget/scenario_minimap.h"
#include "window/city.h"
#include "window/file_dialog.h"
#include "window/main_menu.h"
#include "window/mission_briefing.h"

#define MAX_SCENARIOS 15

static void button_select_item(int index, int param2);
static void button_start_scenario(int param1, int param2);
static void button_toggle_minimap(int param1, int param2);
static void on_scroll(void);

static struct image_button_t start_button =
{ 600, 440, 27, 27, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 56, button_start_scenario, button_none, 1, 0, 1, 0, 0, 0 };

static struct generic_button_t toggle_minimap_button =
{ 570, 87, 39, 28, button_toggle_minimap, button_none, 0, 0 };

static struct generic_button_t file_buttons[] = {
    {18, 220, 252, 16, button_select_item, button_none, 0, 0},
    {18, 236, 252, 16, button_select_item, button_none, 1, 0},
    {18, 252, 252, 16, button_select_item, button_none, 2, 0},
    {18, 268, 252, 16, button_select_item, button_none, 3, 0},
    {18, 284, 252, 16, button_select_item, button_none, 4, 0},
    {18, 300, 252, 16, button_select_item, button_none, 5, 0},
    {18, 316, 252, 16, button_select_item, button_none, 6, 0},
    {18, 332, 252, 16, button_select_item, button_none, 7, 0},
    {18, 348, 252, 16, button_select_item, button_none, 8, 0},
    {18, 364, 252, 16, button_select_item, button_none, 9, 0},
    {18, 380, 252, 16, button_select_item, button_none, 10, 0},
    {18, 396, 252, 16, button_select_item, button_none, 11, 0},
    {18, 412, 252, 16, button_select_item, button_none, 12, 0},
    {18, 428, 252, 16, button_select_item, button_none, 13, 0},
    {18, 444, 252, 16, button_select_item, button_none, 14, 0},
};

static struct scrollbar_type_t scrollbar = { 276, 210, 256, on_scroll, 8, 1, 0, 0, 0, 0 };

static struct {
    int focus_button_id;
    int focus_toggle_button;
    int selected_item;
    int show_minimap;
    char selected_scenario_filename[FILE_NAME_MAX];
    char selected_scenario_display[FILE_NAME_MAX];

    const struct dir_listing *scenarios;
} data;

static void init(void)
{
    data.scenarios = dir_list_files("map");
    data.focus_button_id = 0;
    data.focus_toggle_button = 0;
    data.show_minimap = 0;
    button_select_item(0, 0);
    scrollbar_init(&scrollbar, 0, data.scenarios->num_files - MAX_SCENARIOS);
}

static void draw_scenario_list(void)
{
    inner_panel_draw(16, 210, 16, 16);
    char file[FILE_NAME_MAX];
    for (int i = 0; i < MAX_SCENARIOS; i++) {
        if (i >= data.scenarios->num_files) {
            break;
        }
        int font = FONT_NORMAL_GREEN;
        if (data.focus_button_id == i + 1) {
            font = FONT_NORMAL_WHITE;
        }
        string_copy(data.scenarios->files[i + scrollbar.scroll_position], file, FILE_NAME_MAX - 1);
        file_remove_extension(file);
        text_ellipsize(file, font, 240);
        text_draw(file, 24, 220 + 16 * i, font, 0);
    }
    if (data.scenarios->file_overflow) {
        text_draw(too_many_files_string, 35, 186, FONT_NORMAL_PLAIN, COLOR_RED);
    }
}

static void draw_scenario_info(void)
{
    const int scenario_info_x = 335;
    const int scenario_info_width = 280;
    const int scenario_criteria_x = 420;

    button_border_draw(75, 35, 184, 144, 0);
    image_draw(image_group(GROUP_SCENARIO_IMAGE) + scenario.brief_description_image_id, 77, 37);

    text_ellipsize(data.selected_scenario_display, FONT_LARGE_BLACK, scenario_info_width + 10);
    text_draw_centered(data.selected_scenario_display, scenario_info_x, 25, scenario_info_width + 10, FONT_LARGE_BLACK, 0);
    text_draw_centered(scenario.brief_description, scenario_info_x, 60, scenario_info_width, FONT_NORMAL_WHITE, 0);
    lang_text_draw_year(scenario.start_year, scenario_criteria_x, 90, FONT_LARGE_BLACK);

    if (data.show_minimap) {
        widget_scenario_minimap_draw(332, 119, 286, 300);
        // minimap button: draw mission instructions image
        image_draw(image_group(GROUP_SIDEBAR_BRIEFING_ROTATE_BUTTONS),
            toggle_minimap_button.x + 3, toggle_minimap_button.y + 3);
    } else {
        // minimap button: draw minimap
        widget_scenario_minimap_draw(
            toggle_minimap_button.x + 3, toggle_minimap_button.y + 3,
            toggle_minimap_button.width - 6, toggle_minimap_button.height - 6
        );

        text_draw_centered(climate_types_strings[scenario.climate], scenario_info_x, 150, scenario_info_width, FONT_NORMAL_BLACK, COLOR_BLACK);

        // map size
        int text_id;
        switch (scenario.map.width) {
            case 40: text_id = 121; break;
            case 60: text_id = 122; break;
            case 80: text_id = 123; break;
            case 100: text_id = 124; break;
            case 120: text_id = 125; break;
            default: text_id = 126; break;
        }
        lang_text_draw_centered(44, text_id, scenario_info_x, 170, scenario_info_width, FONT_NORMAL_BLACK);

        // military
        int num_invasions = 0;
        for (int i = 0; i < MAX_INVASIONS; i++) {
            if (scenario.invasions[i].type) {
                num_invasions++;
            }
        }
        if (num_invasions <= 0) {
            text_id = 112;
        } else if (num_invasions <= 2) {
            text_id = 113;
        } else if (num_invasions <= 4) {
            text_id = 114;
        } else if (num_invasions <= 10) {
            text_id = 115;
        } else {
            text_id = 116;
        }
        lang_text_draw_centered(44, text_id, scenario_info_x, 190, scenario_info_width, FONT_NORMAL_BLACK);

        lang_text_draw_centered(32, 11 + scenario.player_rank,
            scenario_info_x, 210, scenario_info_width, FONT_NORMAL_BLACK);
        lang_text_draw_centered(44, 127, scenario_info_x, 262, scenario_info_width, FONT_NORMAL_BLACK);
        int width;
        if (scenario.culture_win_criteria.enabled) {
            width = text_draw_number(scenario.culture_win_criteria.goal, '@', " ",
                scenario_criteria_x, 290, FONT_NORMAL_BLACK);
            lang_text_draw(44, 129, scenario_criteria_x + width, 290, FONT_NORMAL_BLACK);
        }
        if (scenario.prosperity_win_criteria.enabled) {
            width = text_draw_number(scenario.prosperity_win_criteria.goal, '@', " ",
                scenario_criteria_x, 306, FONT_NORMAL_BLACK);
            lang_text_draw(44, 130, scenario_criteria_x + width, 306, FONT_NORMAL_BLACK);
        }
        if (scenario.peace_win_criteria.enabled) {
            width = text_draw_number(scenario.peace_win_criteria.goal, '@', " ",
                scenario_criteria_x, 322, FONT_NORMAL_BLACK);
            lang_text_draw(44, 131, scenario_criteria_x + width, 322, FONT_NORMAL_BLACK);
        }
        if (scenario.favor_win_criteria.enabled) {
            width = text_draw_number(scenario.favor_win_criteria.goal, '@', " ",
                scenario_criteria_x, 338, FONT_NORMAL_BLACK);
            lang_text_draw(44, 132, scenario_criteria_x + width, 338, FONT_NORMAL_BLACK);
        }
        if (scenario.population_win_criteria.enabled) {
            width = text_draw_number(scenario.population_win_criteria.goal, '@', " ",
                scenario_criteria_x, 354, FONT_NORMAL_BLACK);
            lang_text_draw(44, 133, scenario_criteria_x + width, 354, FONT_NORMAL_BLACK);
        }
        if (scenario.time_limit_win_criteria.enabled) {
            width = text_draw_number(scenario.time_limit_win_criteria.years, '@', " ",
                scenario_criteria_x, 370, FONT_NORMAL_BLACK);
            lang_text_draw(44, 134, scenario_criteria_x + width, 370, FONT_NORMAL_BLACK);
        }
        if (scenario.survival_time_win_criteria.enabled) {
            width = text_draw_number(scenario.survival_time_win_criteria.years, '@', " ",
                scenario_criteria_x, 386, FONT_NORMAL_BLACK);
            lang_text_draw(44, 135, scenario_criteria_x + width, 386, FONT_NORMAL_BLACK);
        }
    }
    lang_text_draw_centered(44, 136, scenario_info_x, 446, scenario_info_width, FONT_NORMAL_BLACK);
}

static void draw_background(void)
{
    image_draw_fullscreen_background(image_group(GROUP_INTERMEZZO_BACKGROUND) + 16);
    draw_version_string();

    graphics_set_clip_rectangle((screen_width() - 640) / 2, (screen_height() - 480) / 2, 640, 480);
    graphics_in_dialog();
    image_draw(image_group(GROUP_CCK_BACKGROUND), (640 - 1024) / 2, (480 - 768) / 2);
    graphics_reset_clip_rectangle();
    inner_panel_draw(280, 242, 2, 12);
    draw_scenario_list();
    draw_scenario_info();
    graphics_reset_dialog();
}

static void draw_foreground(void)
{
    graphics_in_dialog();
    image_buttons_draw(0, 0, &start_button, 1);
    button_border_draw(
        toggle_minimap_button.x, toggle_minimap_button.y,
        toggle_minimap_button.width, toggle_minimap_button.height,
        data.focus_toggle_button);
    scrollbar_draw(&scrollbar);
    draw_scenario_list();
    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (scrollbar_handle_mouse(&scrollbar, m_dialog)) {
        return;
    }
    if (image_buttons_handle_mouse(m_dialog, 0, 0, &start_button, 1, 0)) {
        return;
    }
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, &toggle_minimap_button, 1, &data.focus_toggle_button)) {
        return;
    }
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, file_buttons, MAX_SCENARIOS, &data.focus_button_id)) {
        return;
    }
    if (h->enter_pressed) {
        button_start_scenario(0, 0);
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_main_menu_show(0);
    }
}

static void button_select_item(int index, __attribute__((unused)) int param2)
{
    if (index >= data.scenarios->num_files) {
        return;
    }
    data.selected_item = scrollbar.scroll_position + index;
    string_copy(data.scenarios->files[data.selected_item], data.selected_scenario_filename, FILE_NAME_MAX - 1);
    game_file_load_scenario_data(data.selected_scenario_filename);
    string_copy(data.selected_scenario_filename, data.selected_scenario_display, FILE_NAME_MAX - 1);
    file_remove_extension(data.selected_scenario_display);
    window_invalidate();
}

static void button_start_scenario(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (game_file_start_scenario(data.selected_scenario_filename)) {
        update_music(1);
        window_mission_briefing_show();
    }
}

static void button_toggle_minimap(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    data.show_minimap = !data.show_minimap;
    window_invalidate();
}

static void on_scroll(void)
{
    window_invalidate();
}

void window_cck_selection_show(void)
{
    struct window_type_t window = {
        WINDOW_CCK_SELECTION,
        draw_background,
        draw_foreground,
        handle_input,
    };
    init();
    window_show(&window);
}
