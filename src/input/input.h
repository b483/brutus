#ifndef INPUT_INPUT_H
#define INPUT_INPUT_H

#include "city/view.h"

struct hotkey_mapping_t {
    int key;
    int modifiers;
    int action;
};

enum {
    CURSOR_ARROW = 0,
    CURSOR_SHOVEL = 1,
    CURSOR_SWORD = 2,
    CURSOR_MAX,
};

enum {
    CURSOR_SCALE_1 = 0,
    CURSOR_SCALE_1_5 = 1,
    CURSOR_SCALE_2 = 2,
};

struct cursor_t {
    int hotspot_x;
    int hotspot_y;
    int width;
    int height;
    const char *data;
};

const struct cursor_t *input_cursor_data(int cursor_id, int scale);

void input_cursor_update(int window);

struct hotkeys_t {
    // fixed keys with multiple functions
    int enter_pressed;
    int escape_pressed;
    // keys with specific function
    int load_file;
    int save_file;
    int decrease_game_speed;
    int increase_game_speed;
    int toggle_pause;
    int rotate_map_left;
    int rotate_map_right;
    int replay_map;
    int cycle_legion;
    int return_legions_to_fort;
    int show_last_advisor;
    int show_empire_map;
    int show_messages;
    int go_to_problem;
    int clone_building;
    int cycle_buildings;
    int cycle_buildings_reverse;
    int undo;
    int building;
    int show_overlay;
    int go_to_bookmark;
    int set_bookmark;
    int toggle_editor_battle_info;
    int cheat_money;
    int cheat_invasion;
    int cheat_victory;
};

void hotkey_install_mapping(struct hotkey_mapping_t *mappings, int num_mappings);

const struct hotkeys_t *hotkey_state(void);
void hotkey_reset_state(void);

void hotkey_key_pressed(int key, int modifiers, int repeat);
void hotkey_key_released(int key, int modifiers);

void hotkey_handle_global_keys(void);

void keyboard_start_capture(char *text, int max_length, int allow_punctuation, int box_width, int font);
void keyboard_refresh(void);
void keyboard_stop_capture(void);

void keyboard_start_capture_numeric(void (*callback)(int));
void keyboard_stop_capture_numeric(void);

int keyboard_input_is_accepted(void);
int keyboard_is_insert(void);
int keyboard_cursor_position(void);
int keyboard_offset_start(void);
int keyboard_offset_end(void);

void keyboard_return(void);

void keyboard_backspace(void);
void keyboard_delete(void);

void keyboard_left(void);
void keyboard_right(void);
void keyboard_home(void);
void keyboard_end(void);

void keyboard_text(const char *text);

enum {
    KEY_TYPE_NONE = 0,
    KEY_TYPE_A,
    KEY_TYPE_B,
    KEY_TYPE_C,
    KEY_TYPE_D,
    KEY_TYPE_E,
    KEY_TYPE_F,
    KEY_TYPE_G,
    KEY_TYPE_H,
    KEY_TYPE_I,
    KEY_TYPE_J,
    KEY_TYPE_K,
    KEY_TYPE_L,
    KEY_TYPE_M,
    KEY_TYPE_N,
    KEY_TYPE_O,
    KEY_TYPE_P,
    KEY_TYPE_Q,
    KEY_TYPE_R,
    KEY_TYPE_S,
    KEY_TYPE_T,
    KEY_TYPE_U,
    KEY_TYPE_V,
    KEY_TYPE_W,
    KEY_TYPE_X,
    KEY_TYPE_Y,
    KEY_TYPE_Z,
    KEY_TYPE_1,
    KEY_TYPE_2,
    KEY_TYPE_3,
    KEY_TYPE_4,
    KEY_TYPE_5,
    KEY_TYPE_6,
    KEY_TYPE_7,
    KEY_TYPE_8,
    KEY_TYPE_9,
    KEY_TYPE_0,
    KEY_TYPE_MINUS,
    KEY_TYPE_EQUALS,
    KEY_TYPE_ENTER,
    KEY_TYPE_ESCAPE,
    KEY_TYPE_BACKSPACE,
    KEY_TYPE_TAB,
    KEY_TYPE_CAPSLOCK,
    KEY_TYPE_SPACE,
    KEY_TYPE_LEFTBRACKET,
    KEY_TYPE_RIGHTBRACKET,
    KEY_TYPE_BACKSLASH,
    KEY_TYPE_SEMICOLON,
    KEY_TYPE_APOSTROPHE,
    KEY_TYPE_GRAVE,
    KEY_TYPE_COMMA,
    KEY_TYPE_PERIOD,
    KEY_TYPE_SLASH,
    KEY_TYPE_F1,
    KEY_TYPE_F2,
    KEY_TYPE_F3,
    KEY_TYPE_F4,
    KEY_TYPE_F5,
    KEY_TYPE_F6,
    KEY_TYPE_F7,
    KEY_TYPE_F8,
    KEY_TYPE_F9,
    KEY_TYPE_F10,
    KEY_TYPE_F11,
    KEY_TYPE_F12,
    KEY_TYPE_INSERT,
    KEY_TYPE_DELETE,
    KEY_TYPE_HOME,
    KEY_TYPE_END,
    KEY_TYPE_PAGEUP,
    KEY_TYPE_PAGEDOWN,
    // arrow keys
    KEY_TYPE_RIGHT,
    KEY_TYPE_LEFT,
    KEY_TYPE_DOWN,
    KEY_TYPE_UP,
    // keypad keys
    KEY_TYPE_KP_1,
    KEY_TYPE_KP_2,
    KEY_TYPE_KP_3,
    KEY_TYPE_KP_4,
    KEY_TYPE_KP_5,
    KEY_TYPE_KP_6,
    KEY_TYPE_KP_7,
    KEY_TYPE_KP_8,
    KEY_TYPE_KP_9,
    KEY_TYPE_KP_0,
    KEY_TYPE_KP_PERIOD,
    KEY_TYPE_KP_PLUS,
    KEY_TYPE_KP_MINUS,
    KEY_TYPE_KP_MULTIPLY,
    KEY_TYPE_KP_DIVIDE,
    // the key next to left shift on ISO (Non-US) keyboards, usually \ or <
    KEY_TYPE_NON_US,
    KEY_TYPE_MAX_ITEMS
};

enum {
    KEY_MOD_NONE = 0,
    KEY_MOD_SHIFT = 1,
    KEY_MOD_CTRL = 2,
    KEY_MOD_ALT = 4,
    KEY_MOD_GUI = 8,
};

const char *key_combination_name(int key, int modifiers);

int key_combination_from_name(const char *name, int *key, int *modifiers);

struct mouse_button_t {
    int is_down; /**< mouse button is down */
    int went_down; /**< mouse button went down during this cycle */
    int went_up; /**< mouse button went up during this cycle */
    int double_click; /**< mouse double clicked during this cycle */
    int system_change;
};

enum {
    SCROLL_NONE = 0,
    SCROLL_UP = -1,
    SCROLL_DOWN = 1
};

struct mouse_t {
    int x; /**< Global position X */
    int y; /**< Global position Y */
    int scrolled; /**< Scroll state (up/down/none) */
    struct mouse_button_t left; /**< Left mouse button */
    struct mouse_button_t right; /**< Right mouse button */
    int is_inside_window; /**< Whether the mouse is in the window */
};

/**
 * Gets the mouse state
 * @return mouse state
 */
const struct mouse_t *mouse_get(void);

/**
 * Sets the mouse position
 * @param x X
 * @param y Y
 */
void mouse_set_position(int x, int y);

void mouse_set_left_down(int down);

void mouse_set_right_down(int down);

void mouse_set_scroll(int state);

void mouse_set_inside_window(int inside);

void mouse_reset_up_state(void);

void mouse_reset_scroll(void);

void mouse_reset_button_state(void);

void mouse_determine_button_state(void);

const struct mouse_t *mouse_in_dialog(const struct mouse_t *m);

enum {
    SCROLL_TYPE_CITY = 0,
    SCROLL_TYPE_EMPIRE = 1,
    SCROLL_TYPE_MAX = 2
};

int scroll_in_progress(void);

int scroll_get_delta(const struct mouse_t *m, struct pixel_view_coordinates_t *delta, int type);

void scroll_drag_start(void);
int scroll_drag_end(void);

void scroll_stop(void);

#endif