#ifndef GRAPHICS_GRAPHICS_H
#define GRAPHICS_GRAPHICS_H

#include "input/hotkey.h"
#include "input/mouse.h"
#include "figure/figure.h"

#include <stdint.h>

typedef uint32_t color_t;

#define COLOR_BLACK 0x000000
#define COLOR_RED 0xff0000
#define COLOR_WHITE 0xffffff

#define COLOR_SG2_TRANSPARENT 0xf700ff
#define COLOR_TOOLTIP 0x424242
#define COLOR_SIDEBAR 0xbdb592

#define COLOR_FONT_RED COLOR_RED
#define COLOR_FONT_BLUE 0x0055ff
#define COLOR_FONT_YELLOW 0xe7e75a
#define COLOR_FONT_ORANGE 0xff5a08
#define COLOR_FONT_ORANGE_LIGHT 0xffa500
#define COLOR_FONT_LIGHT_GRAY 0xb3b3b3

#define COLOR_INSET_LIGHT 0xffffff
#define COLOR_INSET_DARK 0x848484

#define COLOR_MASK_NONE 0xffffff
#define COLOR_MASK_RED 0xff0818
#define COLOR_MASK_GREEN 0x18ff18
#define COLOR_MASK_BLUE 0x663377ff
#define COLOR_MASK_LEGION_HIGHLIGHT 0x66ff3300

#define COLOR_MINIMAP_VIEWPORT 0xe7e75a
#define COLOR_MINIMAP_DARK 0x424242
#define COLOR_MINIMAP_LIGHT 0xc6c6c6
#define COLOR_MINIMAP_SOLDIER 0xf70000
#define COLOR_MINIMAP_SELECTED_SOLDIER 0xffffff
#define COLOR_MINIMAP_ENEMY_CENTRAL 0x7b0000
#define COLOR_MINIMAP_ENEMY_NORTHERN 0x1800ff
#define COLOR_MINIMAP_ENEMY_DESERT 0x08007b
#define COLOR_MINIMAP_WOLF COLOR_BLACK

#define COLOR_MOUSE_DARK_GRAY 0x3f3f3f
#define COLOR_MOUSE_MEDIUM_GRAY 0x737373
#define COLOR_MOUSE_LIGHT_GRAY 0xb3b3b3

#define ALPHA_OPAQUE 0xff000000
#define ALPHA_FONT_SEMI_TRANSPARENT 0x99000000
#define ALPHA_MASK_SEMI_TRANSPARENT 0x48000000
#define ALPHA_TRANSPARENT 0x00000000

#define BLOCK_SIZE 16

#define TOP_MENU_HEIGHT 24

enum {
    CLIP_NONE,
    CLIP_LEFT,
    CLIP_RIGHT,
    CLIP_TOP,
    CLIP_BOTTOM,
    CLIP_BOTH,
    CLIP_INVISIBLE
};

struct clip_info_t {
    int clip_x;
    int clip_y;
    int clipped_pixels_left;
    int clipped_pixels_right;
    int clipped_pixels_top;
    int clipped_pixels_bottom;
    int visible_pixels_x;
    int visible_pixels_y;
    int is_visible;
};

struct arrow_button_t {
    short x_offset;
    short y_offset;
    short image_id;
    short size;
    void (*left_click_handler)(int param1, int param2);
    int parameter1;
    int parameter2;
    // state
    int pressed;
    int repeats;
};

enum {
    FONT_NORMAL_PLAIN,
    FONT_NORMAL_BLACK,
    FONT_NORMAL_WHITE,
    FONT_NORMAL_RED,
    FONT_LARGE_PLAIN,
    FONT_LARGE_BLACK,
    FONT_LARGE_BROWN,
    FONT_SMALL_PLAIN,
    FONT_NORMAL_GREEN,
    FONT_NORMAL_BROWN,
    FONT_TYPES_MAX
};

struct font_definition_t {
    int font;
    int image_offset;
    int space_width;
    int letter_spacing;
    int line_height;

    int (*image_y_offset)(uint8_t c, int image_height, int line_height);
};

struct generic_button_t {
    short x;
    short y;
    short width;
    short height;
    void (*left_click_handler)(int param1, int param2);
    void (*right_click_handler)(int param1, int param2);
    int parameter1;
    int parameter2;
};

enum {
    IB_NORMAL = 4,
    IB_SCROLL = 6,
    IB_BUILD = 2
};

struct image_button_t {
    short x_offset;
    short y_offset;
    short width;
    short height;
    short button_type;
    short image_collection;
    short image_offset;
    void (*left_click_handler)(int param1, int param2);
    void (*right_click_handler)(int param1, int param2);
    int parameter1;
    int parameter2;
    char enabled;
    // state
    char pressed;
    char focused;
    uint32_t pressed_since;
};

struct menu_item_t {
    short text_group;
    short text_number;
    void (*left_click_handler)(int param);
    int parameter;
    int hidden;
};

struct menu_bar_item_t {
    short text_group;
    struct menu_item_t *items;
    int num_items;
    short x_start;
    short x_end;
    int calculated_width_blocks;
    int calculated_height_blocks;
};

struct scrollbar_type_t {
    int x;
    int y;
    int height;
    void (*on_scroll_callback)(void);
    int dot_padding;
    int always_visible;
    int max_scroll_position;
    int scroll_position;
    int is_dragging_scroll;
    int scroll_position_drag;
};

enum {
    WINDOW_LOGO,
    WINDOW_MAIN_MENU,
    WINDOW_CONFIG,
    WINDOW_HOTKEY_CONFIG,
    WINDOW_HOTKEY_EDITOR,
    WINDOW_CCK_SELECTION,
    WINDOW_FILE_DIALOG,
    WINDOW_POPUP_DIALOG,
    WINDOW_PLAIN_MESSAGE_DIALOG,
    WINDOW_INTRO_VIDEO,
    // mission start/end
    WINDOW_INTERMEZZO,
    WINDOW_MISSION_SELECTION,
    WINDOW_MISSION_BRIEFING,
    WINDOW_VICTORY_DIALOG,
    WINDOW_VICTORY_VIDEO,
    WINDOW_MISSION_END,
    // city
    WINDOW_CITY,
    WINDOW_CITY_MILITARY,
    WINDOW_TOP_MENU,
    WINDOW_OVERLAY_MENU,
    WINDOW_MILITARY_MENU,
    WINDOW_BUILD_MENU,
    WINDOW_SLIDING_SIDEBAR,
    WINDOW_MESSAGE_DIALOG,
    WINDOW_MESSAGE_LIST,
    WINDOW_BUILDING_INFO,
    // advisors and dialogs
    WINDOW_ADVISORS,
    WINDOW_LABOR_PRIORITY,
    WINDOW_SET_SALARY,
    WINDOW_DONATE_TO_CITY,
    WINDOW_GIFT_TO_EMPEROR,
    WINDOW_TRADE_PRICES,
    WINDOW_RESOURCE_SETTINGS,
    WINDOW_HOLD_FESTIVAL,
    // empire and dialog
    WINDOW_EMPIRE,
    WINDOW_TRADE_OPENED,
    // options dialogs
    WINDOW_DISPLAY_OPTIONS,
    WINDOW_SOUND_OPTIONS,
    WINDOW_SPEED_OPTIONS,
    // utility windows
    WINDOW_SELECT_LIST,
    WINDOW_NUMERIC_INPUT,
    // editor
    WINDOW_EDITOR_MAP,
    WINDOW_EDITOR_TOP_MENU,
    WINDOW_EDITOR_BUILD_MENU,
    WINDOW_EDITOR_EMPIRE,
    WINDOW_EDITOR_ATTRIBUTES,
    WINDOW_EDITOR_BRIEFING,
    WINDOW_EDITOR_STARTING_CONDITIONS,
    WINDOW_EDITOR_START_YEAR,
    WINDOW_EDITOR_WIN_CRITERIA,
    WINDOW_EDITOR_ALLOWED_BUILDINGS,
    WINDOW_EDITOR_SPECIAL_EVENTS,
    WINDOW_EDITOR_REQUESTS,
    WINDOW_EDITOR_EDIT_REQUEST,
    WINDOW_EDITOR_CUSTOM_MESSAGES,
    WINDOW_EDITOR_EDIT_CUSTOM_MESSAGE,
    WINDOW_EDITOR_EARTHQUAKES,
    WINDOW_EDITOR_EDIT_EARTHQUAKE,
    WINDOW_EDITOR_INVASIONS,
    WINDOW_EDITOR_EDIT_INVASION,
    WINDOW_EDITOR_PRICE_CHANGES,
    WINDOW_EDITOR_EDIT_PRICE_CHANGE,
    WINDOW_EDITOR_DEMAND_CHANGES,
    WINDOW_EDITOR_EDIT_DEMAND_CHANGE,
};

struct window_type_t {
    int id;
    void (*draw_background)(void);
    void (*draw_foreground)(void);
    void (*handle_input)(const struct mouse_t *m, const struct hotkeys_t *h);
};

void *graphics_canvas(void);

void graphics_in_dialog(void);
void graphics_reset_dialog(void);

void graphics_set_clip_rectangle(int x, int y, int width, int height);
void graphics_reset_clip_rectangle(void);
const struct clip_info_t *graphics_get_clip_info(int x, int y, int width, int height);

void graphics_save_to_buffer(int x, int y, int width, int height, color_t *buffer);
void graphics_draw_from_buffer(int x, int y, int width, int height, const color_t *buffer);

color_t *graphics_get_pixel(int x, int y);

void graphics_clear_screen(void);

void graphics_draw_vertical_line(int x, int y1, int y2, color_t color);
void graphics_draw_horizontal_line(int x1, int x2, int y, color_t color);

void graphics_draw_rect(int x, int y, int width, int height, color_t color);
void graphics_draw_inset_rect(int x, int y, int width, int height);

void graphics_fill_rect(int x, int y, int width, int height, color_t color);
void graphics_shade_rect(int x, int y, int width, int height, int darkness);

void arrow_buttons_draw(int x, int y, struct arrow_button_t *buttons, int num_buttons);

int arrow_buttons_handle_mouse(const struct mouse_t *m, int x, int y, struct arrow_button_t *buttons, int num_buttons, int *focus_button_id);

void button_none(int param1, int param2);

void button_border_draw(int x, int y, int width_pixels, int height_pixels, int has_focus);

void font_set_encoding(void);

const struct font_definition_t *font_definition_for(int font);

int font_can_display(const char *character);

int generic_buttons_handle_mouse(const struct mouse_t *m, int x, int y, struct generic_button_t *buttons, int num_buttons, int *focus_button_id);

void image_buttons_draw(int x, int y, struct image_button_t *buttons, int num_buttons);

int image_buttons_handle_mouse(const struct mouse_t *m, int x, int y, struct image_button_t *buttons, int num_buttons, int *focus_button_id);

void image_draw(int image_id, int x, int y);
void image_draw_enemy(struct figure_t *f, int x, int y);

void image_draw_masked(int image_id, int x, int y, color_t color_mask);
void image_draw_blend(int image_id, int x, int y, color_t color);
void image_draw_blend_alpha(int image_id, int x, int y, color_t color);

void image_draw_fullscreen_background(int image_id);

void image_draw_isometric_footprint(int image_id, int x, int y, color_t color_mask);
void image_draw_isometric_footprint_from_draw_tile(int image_id, int x, int y, color_t color_mask);

void image_draw_isometric_top(int image_id, int x, int y, color_t color_mask);
void image_draw_isometric_top_from_draw_tile(int image_id, int x, int y, color_t color_mask);

int lang_text_get_width(int group, int number, int font);

int lang_text_draw(int group, int number, int x_offset, int y_offset, int font);
int lang_text_draw_colored(int group, int number, int x_offset, int y_offset, int font, color_t color);

void lang_text_draw_centered(int group, int number, int x_offset, int y_offset, int box_width, int font);
void lang_text_draw_centered_colored(int group, int number, int x_offset, int y_offset, int box_width, int font, color_t color);

int lang_text_draw_amount(int group, int number, int amount, int x_offset, int y_offset, int font);

int lang_text_draw_year(int year, int x_offset, int y_offset, int font);
void lang_text_draw_month_year_max_width(int month, int year, int x_offset, int y_offset, int box_width, int font, color_t color);

int lang_text_draw_multiline(int group, int number, int x_offset, int y_offset, int box_width, int font);

void menu_bar_draw(struct menu_bar_item_t *items, int num_items, int max_width);
int menu_bar_handle_mouse(const struct mouse_t *m, struct menu_bar_item_t *items, int num_items, int *focus_menu_id);

void menu_draw(struct menu_bar_item_t *menu, int focus_item_id);
int menu_handle_mouse(const struct mouse_t *m, struct menu_bar_item_t *menu, int *focus_item_id);
void menu_update_text(struct menu_bar_item_t *menu, int index, int text_number);

void outer_panel_draw(int x, int y, int width_blocks, int height_blocks);

void inner_panel_draw(int x, int y, int width_blocks, int height_blocks);

void label_draw(int x, int y, int width_blocks, int type);

void large_label_draw(int x, int y, int width_blocks, int type);

int rich_text_init(const char *text, int x_text, int y_text, int width_blocks, int height_blocks, int adjust_width_on_no_scroll);

void rich_text_set_fonts(int normal_font, int link_font, int line_spacing);

void rich_text_reset(int scroll_position);

void rich_text_reset_lines_only(void);

void rich_text_clear_links(void);

int rich_text_get_clicked_link(const struct mouse_t *m);

int rich_text_draw(const char *text, int x_offset, int y_offset, int box_width, int height_lines, int measure_only);

int rich_text_draw_colored(const char *text, int x_offset, int y_offset, int box_width, int height_lines, color_t color);

void rich_text_draw_scrollbar(void);

int rich_text_handle_mouse(const struct mouse_t *m);

int rich_text_scroll_position(void);

void screen_set_resolution(int width, int height);

color_t *screen_pixel(int x, int y);

int screen_width(void);

int screen_height(void);

int screen_dialog_offset_x(void);

int screen_dialog_offset_y(void);

void graphics_save_screenshot(int full_city);

void scrollbar_init(struct scrollbar_type_t *scrollbar, int scroll_position, int max_scroll_position);

void scrollbar_reset(struct scrollbar_type_t *scrollbar, int scroll_position);

void scrollbar_update_max(struct scrollbar_type_t *scrollbar, int max_scroll_position);

void scrollbar_draw(struct scrollbar_type_t *scrollbar);

int scrollbar_handle_mouse(struct scrollbar_type_t *scrollbar, const struct mouse_t *m);

void text_capture_cursor(int cursor_position, int offset_start, int offset_end);
void text_draw_cursor(int x_offset, int y_offset);

int text_get_width(const char *str, int font);
unsigned int text_get_max_length_for_width(const char *str, int length, int font, unsigned int requested_width, int invert);
void text_ellipsize(char *str, int font, int requested_width);

int text_draw(const char *str, int x, int y, int font, color_t color);

void text_draw_centered(const char *str, int x, int y, int box_width, int font, color_t color);
void text_draw_ellipsized(const char *str, int x, int y, int box_width, int font, color_t color);

int text_draw_number(int value, char prefix, const char *postfix, int x_offset, int y_offset, int font);
int text_draw_number_colored(int value, char prefix, const char *postfix, int x_offset, int y_offset, int font, color_t color);
int text_draw_money(int value, int x_offset, int y_offset, int font);
int text_draw_percentage(int value, int x_offset, int y_offset, int font);

void text_draw_number_centered(int value, int x_offset, int y_offset, int box_width, int font);
void text_draw_number_centered_prefix(int value, char prefix, int x_offset, int y_offset, int box_width, int font);
void text_draw_number_centered_colored(int value, int x_offset, int y_offset, int box_width, int font, color_t color);

int text_draw_multiline(const char *str, int x_offset, int y_offset, int box_width, int font, uint32_t color);

int video_start(const char *filename);

void video_init(int restart_music);

int video_is_finished(void);

void video_stop(void);

void video_shutdown(void);

void video_draw(int x_offset, int y_offset);

void video_draw_fullscreen(void);

void window_draw(int force);

void window_invalidate(void);

int window_is_invalid(void);

void window_request_refresh(void);

int window_is(int id);

int window_get_id(void);

void window_show(const struct window_type_t *window);

void window_go_back(void);

void window_draw_underlying_window(void);

#endif // GRAPHICS_GRAPHICS_H
