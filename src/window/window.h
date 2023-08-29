#ifndef WINDOW_WINDOW_H
#define WINDOW_WINDOW_H

#include "empire/object.h"
#include "graphics/graphics.h"
#include "input/input.h"

#define BUILD_MENU_BUTTONS_COUNT 12
#define MAX_ITEMS_PER_BUILD_MENU 11
#define MAX_ITEMS_PER_SUBMENU 6
#define NO_COLUMN -1
#define SIDEBAR_COLLAPSED_WIDTH 42
#define SIDEBAR_EXPANDED_WIDTH 162
#define SIDEBAR_MAIN_SECTION_HEIGHT 450
#define SIDEBAR_FILLER_Y_OFFSET (SIDEBAR_MAIN_SECTION_HEIGHT + TOP_MENU_HEIGHT)
#define MINIMAP_WIDTH 146
#define MINIMAP_HEIGHT 111


enum {
    MENU_VACANT_HOUSE = 0,
    MENU_CLEAR_LAND = 1,
    MENU_ROAD = 2,
    MENU_WATER = 3,
    MENU_HEALTH = 4,
    MENU_TEMPLES = 5,
    MENU_EDUCATION = 6,
    MENU_ENTERTAINMENT = 7,
    MENU_ADMINISTRATION = 8,
    MENU_ENGINEERING = 9,
    MENU_SECURITY = 10,
    MENU_INDUSTRY = 11,
};

enum {
    FILE_DIALOG_SAVE = 0,
    FILE_DIALOG_LOAD = 1,
    FILE_DIALOG_DELETE = 2
};

enum {
    FILE_TYPE_SAVED_GAME = 0,
    FILE_TYPE_SCENARIO = 1
};

enum {
    MESSAGE_NONE = 0,
    MESSAGE_MISSING_PATCH = 1,
};

enum {
    MESSAGE_DIALOG_ABOUT = 0,
    MESSAGE_DIALOG_HELP = 10,
    MESSAGE_DIALOG_TOP_FUNDS = 15,
    MESSAGE_DIALOG_TOP_POPULATION = 16,
    MESSAGE_DIALOG_TOP_DATE = 17,
    MESSAGE_DIALOG_OVERLAYS = 18,
    MESSAGE_DIALOG_ADVISOR_LABOR = 20,
    MESSAGE_DIALOG_ADVISOR_MILITARY = 21,
    MESSAGE_DIALOG_ADVISOR_IMPERIAL = 22,
    MESSAGE_DIALOG_ADVISOR_RATINGS = 23,
    MESSAGE_DIALOG_ADVISOR_TRADE = 24,
    MESSAGE_DIALOG_ADVISOR_POPULATION = 25,
    MESSAGE_DIALOG_ADVISOR_HEALTH = 26,
    MESSAGE_DIALOG_ADVISOR_EDUCATION = 27,
    MESSAGE_DIALOG_ADVISOR_ENTERTAINMENT = 28,
    MESSAGE_DIALOG_ADVISOR_RELIGION = 29,
    MESSAGE_DIALOG_ADVISOR_FINANCIAL = 30,
    MESSAGE_DIALOG_ADVISOR_CHIEF = 31,
    MESSAGE_DIALOG_EMPIRE_MAP = 32,
    MESSAGE_DIALOG_MESSAGES = 34,
    MESSAGE_DIALOG_INDUSTRY = 46,
    MESSAGE_DIALOG_THEFT = 251,
    MESSAGE_DIALOG_EDITOR_ABOUT = 331,
    MESSAGE_DIALOG_EDITOR_HELP = 332,
};

enum {
    POPUP_DIALOG_NONE = -1,
    POPUP_DIALOG_QUIT = 0,
    POPUP_DIALOG_OPEN_TRADE = 2,
    POPUP_DIALOG_SEND_GOODS = 4,
    POPUP_DIALOG_NOT_ENOUGH_GOODS = 6,
    POPUP_DIALOG_NO_LEGIONS_AVAILABLE = 8,
    POPUP_DIALOG_NO_LEGIONS_SELECTED = 10,
    POPUP_DIALOG_SEND_TROOPS = 12,
    POPUP_DIALOG_DELETE_FORT = 14,
    POPUP_DIALOG_DELETE_BRIDGE = 18,
    POPUP_DIALOG_EDITOR_QUIT_WITHOUT_SAVING = 20,
};

enum {
    SLIDE_DIRECTION_IN = 0,
    SLIDE_DIRECTION_OUT = 1
};

struct advisor_window_type_t {
    /**
     * @return height of the advisor in blocks of 16px
     */
    int (*draw_background)(void);
    void (*draw_foreground)(void);
    int (*handle_mouse)(const struct mouse_t *m);
};

struct submenu_t {
    int building_id;
    char *submenu_string;
    int submenu_items[MAX_ITEMS_PER_SUBMENU];
};

struct build_menu_t {
    int is_enabled;
    struct submenu_t menu_items[MAX_ITEMS_PER_BUILD_MENU];
};

extern struct build_menu_t build_menus[BUILD_MENU_BUTTONS_COUNT];

struct city_overlay_t {
    int type;
    int column_type;
    int (*show_building)(const struct building_t *b);
    int (*show_figure)(const struct figure_t *f);
    int (*get_column_height)(const struct building_t *b);
    void (*draw_custom_footprint)(int x, int y, int grid_offset);
    void (*draw_custom_top)(int x, int y, int grid_offset);
};

struct pixel_coordinate_t {
    int x;
    int y;
};

struct input_box_t {
    int x;
    int y;
    int width_blocks;
    int height_blocks;
    int font;
    int allow_punctuation;
    char *text;
    int text_length;
};

void window_advisors_show(int advisor);

void map_building_menu_items(void);

int window_build_menu_image(void);

void window_build_menu_show(int submenu);

void window_build_menu_hide(void);

void window_city_draw_background(void);

int center_in_city(int element_width_pixels);

void replay_map(void);

void window_city_draw_all(void);

void window_city_show(void);

void window_city_military_show(int legion_formation_id);

void window_city_return(void);

void window_display_options_show(void (*close_callback)(void));

void window_empire_show(void);

void window_file_dialog_show(int type, int dialog_type);

void window_hotkey_editor_key_pressed(int key, int modifiers);

void window_hotkey_editor_key_released(int key, int modifiers);

void window_logo_show(int show_patch_message);

void window_main_menu_show(int restart_music);

void window_message_dialog_show(int text_id, void (*background_callback)(void));

void window_message_dialog_show_city_message(int text_id, int year, int month, int param1, int param2, int message_advisor, int use_popup);

void window_message_list_show(void);

void window_mission_briefing_show(void);

void window_mission_end_show_won(void);

void window_mission_end_show_fired(void);

void window_numeric_input_show(int x, int y, int max_digits, int max_value, void (*callback)(int));

void window_numeric_input_accept(void);

void window_overlay_menu_show(void);

void window_popup_dialog_show(int type, void (*close_func)(void), int has_ok_cancel_buttons);
void window_select_list_show(int x, int y, int group, int num_items, void (*callback)(int));
void window_select_list_show_text(int x, int y, char **items, int num_items, void (*callback)(int));

void window_sound_options_show(int from_editor);

void window_speed_options_show(int from_editor);

void window_victory_dialog_show(void);

int sidebar_common_get_x_offset_expanded(void);

void sidebar_common_draw_relief(int x_offset, int y_offset, int image, int is_collapsed);

typedef void (*back_sidebar_draw_function)(void);
typedef back_sidebar_draw_function slide_finished_function;
typedef void (*front_sidebar_draw_function)(int x_offset);

void city_draw_figure(struct figure_t *f, int x, int y, int hover);

void city_without_overlay_draw(int selected_figure_id, struct pixel_coordinate_t *figure_coord, const struct map_tile_t *tile);

void widget_city_draw(void);
void widget_city_draw_for_figure(int figure_id, struct pixel_coordinate_t *coord);

void update_city_view_coords(int x, int y, struct map_tile_t *tile);

void scroll_map(const struct mouse_t *m);

void input_box_start(struct input_box_t *box);
void input_box_stop(struct input_box_t *box);

void input_box_draw(const struct input_box_t *box);

void widget_minimap_invalidate(void);

void widget_minimap_draw(int x_offset, int y_offset, int width, int height, int force);

int widget_minimap_handle_mouse(const struct mouse_t *m);

#endif