#include "input.h"

#include "building/building.h"
#include "core/calc.h"
#include "core/config.h"
#include "core/hotkey_config.h"
#include "core/speed.h"
#include "core/string.h"
#include "core/time.h"
#include "editor/editor.h"
#include "game/game.h"
#include "graphics/graphics.h"
#include "platform/brutus.h"
#include "window/hotkey_editor.h"

#include "math.h"
#include <string.h>

#define DOUBLE_CLICK_TIME 300

#define MOUSE_BORDER 5
#define TOUCH_BORDER 100
#define SCROLL_DRAG_MIN_DELTA 4
#define SCROLL_KEY_PRESSED 1
#define SCROLL_KEY_MAX_VALUE 30000.0f

static const struct cursor_t ARROW[] = {
    {
        0, 0, 13, 21,
        "#            "
        "##           "
        "#'#          "
        "#''#         "
        "#'''#        "
        "#''''#       "
        "#'''''#      "
        "#''''''#     "
        "#'''''''#    "
        "#''''''''#   "
        "#'''''''''#  "
        "#''''''''''# "
        "#''''''######"
        "#'''#''#     "
        "#''# #''#    "
        "#'#  #''#    "
        "##    #''#   "
        "#     #''#   "
        "       #''#  "
        "       #''#  "
        "        ##   "
    },
    {
        0, 0, 18, 30,
        "#                 "
        "##                "
        "#&#               "
        "#'&#              "
        "#''&#             "
        "#'''&#            "
        "#''''&#           "
        "#'''''&#          "
        "#''''''&#         "
        "#'''''''&#        "
        "#''''''''&#       "
        "#'''''''''&#      "
        "#''''''''''&#     "
        "#'''''''''''&#    "
        "#''''''''''''&#   "
        "#'''''''''''''&#  "
        "#''''''''''''''&# "
        "#'''''''''''''''$#"
        "#'''''''&#########"
        "#''''$%''$        "
        "#'''$##&'&#       "
        "#''$#  $''%#      "
        "#&#    #&''#      "
        "##      #''%#     "
        "#       #%''#     "
        "         #''&#    "
        "         #%''$    "
        "          #&'&#   "
        "           $'&#   "
        "           ###    "
    },
    {
        0, 0, 24, 40,
        "#                       "
        "##                      "
        "#&#                     "
        "#'&#                    "
        "#''&#                   "
        "#'''&#                  "
        "#''''&#                 "
        "#'''''&#                "
        "#''''''&#               "
        "#'''''''&#              "
        "#''''''''&#             "
        "#'''''''''&#            "
        "#''''''''''&#           "
        "#'''''''''''&#          "
        "#''''''''''''&#         "
        "#'''''''''''''&#        "
        "#''''''''''''''&#       "
        "#'''''''''''''''&#      "
        "#''''''''''''''''&#     "
        "#'''''''''''''''''&#    "
        "#''''''''''''''''''&#   "
        "#'''''''''''''''''''&#  "
        "#''''''''''''''''''''&# "
        "#'''''''''''''''''''''&#"
        "#''''''''''&############"
        "#''''''&&'''$           "
        "#'''''&##'''&#          "
        "#''''&# #%'''%#         "
        "#''''#   #''''#         "
        "#'''#    #%'''%#        "
        "#''#      #&'''#        "
        "#'$#       $'''&#       "
        "#$#        #&'''$       "
        "##          #'''&#      "
        "#           #%'''%#     "
        "             #''''#     "
        "             #%'''%#    "
        "              #&'''#    "
        "               $'&$#    "
        "               ###      "
    }
};

static const struct cursor_t SWORD[] = {
    {
        0, 0, 22, 22,
        "#####                 "
        "#'''##                "
        "#''''##               "
        "#'''''##              "
        "##'''''##             "
        " ##'''''##            "
        "  ##'''''##           "
        "   ##'''''##          "
        "    ##'''''##         "
        "     ##'''''##        "
        "      ##'''''##       "
        "       ##'''''##      "
        "        ##'''''#####  "
        "         ##'''''#''#  "
        "          ##'''#'''#  "
        "           ##'#'''##  "
        "            ##'''###  "
        "            #'''##'###"
        "            #''##''''#"
        "            ######'''#"
        "                 #''##"
        "                 #### "
    },
    {
        0, 0, 34, 34,
        "######                            "
        "#%&&&##                           "
        "#&'''&##                          "
        "#&''''&##                         "
        "#&'''''&##                        "
        "##&'''''&##                       "
        " ##&'''''&##                      "
        "  ##&'''''&##                     "
        "   ##&'''''&##                    "
        "    ##&'''''&##                   "
        "     ##&'''''&##                  "
        "      ##&'''''&##                 "
        "       ##&'''''&##                "
        "        ##&'''''&##               "
        "         ##&'''''&##              "
        "          ##&'''''&##             "
        "           ##&'''''&##            "
        "            ##&'''''&##           "
        "             ##&'''''&##          "
        "              ##&'''''&########   "
        "               ##&''''''&##&&$#   "
        "                ##&''''&##&''$#   "
        "                 ##&''&##&''&##   "
        "                  ##%&##&''&###   "
        "                   #&##&''&####   "
        "                   ###&''&#####   "
        "                   ##&''&###%$### "
        "                   #&''&###%'%$$##"
        "                   #&'&###%'''''$#"
        "                   #$$####$%''''$#"
        "                   ########$'''&##"
        "                          #$''&#  "
        "                          ##$$##  "
        "                           ####   "
    },
    {
        0, 0, 46, 44,
        "########                                      "
        "#%%%%%%##                                     "
        "#%'''''&##                                    "
        "#%''''''&##                                   "
        "#%'''''''%##                                  "
        "#%''''''''%##                                 "
        "#%'''''''''%##                                "
        "##%'''''''''%##                               "
        " ##%'''''''''%##                              "
        "  ##%'''''''''%##                             "
        "   ##%'''''''''%##                            "
        "    ##%'''''''''%##                           "
        "     ##%'''''''''%##                          "
        "      ##%'''''''''%##                         "
        "       ##%'''''''''%##                        "
        "        ##%'''''''''%##                       "
        "         ##%'''''''''%##                      "
        "          ##%'''''''''%##                     "
        "           ##%'''''''''%##                    "
        "            ##%'''''''''%##                   "
        "             ##%'''''''''%##                  "
        "              ##%'''''''''%##                 "
        "               ##%'''''''''%##                "
        "                ##%'''''''''%##               "
        "                 ##%'''''''''%##              "
        "                  ##%'''''''''%##########     "
        "                   ##%'''''''''%$$##$$$##     "
        "                    ##%''''''''''%#%'''$#     "
        "                     ##%''''''''%#%''''$#     "
        "                      ##%''''''%#%'''''$#     "
        "                       ##%''''%#%'''''%##     "
        "                        ##%''%#%'''''%###     "
        "                         #$'%#%'''''%####     "
        "                         #$%#%'''''%#####     "
        "                         ###%'''''%###%$#     "
        "                         ##%'''''%###%'$####  "
        "                         #$'''''%###%''%$$$## "
        "                         #$''''%###%'''''''$##"
        "                         #$'''%###%''''''''$##"
        "                         ##$$$####$$%'''''''$#"
        "                         ###########$''''$####"
        "                                   #$'''$&#   "
        "                                    #$''$#    "
        "                                     #####    "
    }
};

static const struct cursor_t SHOVEL[] = {
    {
        1, 26, 28, 28,
        "                       ##   "
        "                      ####  "
        "                     ##'### "
        "                     #'''###"
        "                     ##'''##"
        "                    ####'## "
        "                   ##'####  "
        "                  ##'##     "
        "                 ##'##      "
        "                ##'##       "
        "               ##'##        "
        "              ##'##         "
        "             ##'##          "
        "       #    ##'##           "
        "      #### ##'##            "
        "     #''####'##             "
        "    #''''##'##              "
        "   #''''##'##               "
        "  #''''#'#####              "
        " #''''#'''#'##              "
        " #'''#'''#'''##             "
        "#'''''''#''''#              "
        "#''''''#''''#               "
        "#''''''''''#                "
        "#'''''''''#                 "
        "#''''''''#                  "
        " #'''''##                   "
        "  #####                     "
    },
    {
        2, 39, 44, 41,
        "                                   ####     "
        "                                 #######    "
        "                                ##%&#####   "
        "                               ##&''$#####  "
        "                               #$'''''$#### "
        "                               #$''''''$####"
        "                               ##&''''''%## "
        "                              ####&''''$##  "
        "                             ##$''##&&&$#   "
        "                            ##$'''$#####    "
        "                           ##$'''$##        "
        "                          ##$'''$##         "
        "                         ##$'''$##          "
        "                        ##$'''$##           "
        "                       ##$'''$##            "
        "                      ##$'''$##             "
        "                     ##$'''$##              "
        "                    ##$'''$##               "
        "         ###       ##$'''$##                "
        "        ##$##     ##$'''$##                 "
        "       ##%'%##   ##$'''$##                  "
        "      ##%'''%## ##$'''$##                   "
        "     ##%'''''%##%''''$##                    "
        "    ##%''''''%#$''''$##                     "
        "   ##%''''''%#$''''$##                      "
        "  ##%''''''%#$#%''$##                       "
        "  #$''''''%#%'%#$###                        "
        "  #&'''''%#%'''$#%%##                       "
        " ##'''''%#%'''%#%''%##                      "
        " #$''''%#%'''%#%''''%##                     "
        " #&''''%%'''%#%''''''$#                     "
        " #&''''''''%#%''''''%##                     "
        "##''''''''%#%''''''%##                      "
        "#$''''''''%%''''''%##                       "
        "#$'''''''''''''''%##                        "
        "#$''''''''''''''%##                         "
        "#$'''''''''''''%##                          "
        "##'''''''''''&$##                           "
        " #%''''''''&$###                            "
        " ##%&''&%$###                               "
        "  #########                                 "
    },
    {
        3, 52, 58, 55,
        "                                               ##         "
        "                                             ######       "
        "                                            ###&#####     "
        "                                           ##$''&#####    "
        "                                          ##$''''&#####   "
        "                                         ##$'''''''$####  "
        "                                         ##$''''''''$#### "
        "                                         ##$'''''''''$####"
        "                                         ##$''''''''''$###"
        "                                         ###%'''''''''$## "
        "                                        ##%%#%''''''&##   "
        "                                       ##&''%#%''''&##    "
        "                                      ##&''''%#$$$$##     "
        "                                     ##&''''%#######      "
        "                                    ##&''''&###           "
        "                                   ##&''''&##             "
        "                                  ##&''''&##              "
        "                                 ##&''''&##               "
        "                                ##&''''&##                "
        "                               ##&''''&##                 "
        "                              ##&''''&##                  "
        "                             ##&''''&##                   "
        "                            ##&''''&##                    "
        "                           ##&''''&##                     "
        "             ##           ##&''''&##                      "
        "            ####         ##&''''&##                       "
        "           ##%%##       ##&''''&##                        "
        "          ##%''%##     ##&''''&##                         "
        "         ##%''''%##   ##&''''&##                          "
        "        ##%''''''%## ##%''''&##                           "
        "       ##%''''''''%##%'''''&##                            "
        "      ##%'''''''''%#$'''''&##                             "
        "     ##%'''''''''%#%'''''&##                              "
        "    ##%'''''''''%##%''''%##                               "
        "   ##%'''''''''%#%%#%''$##                                "
        "   #$'''''''''%#%''%#%%###                                "
        "   #&''''''''%#%''''%##%%##                               "
        "  ##''''''''%#%'''''%#%''%##                              "
        "  #%'''''''%#%'''''%#%''''%##                             "
        "  #&''''''%#%'''''%#%''''''%##                            "
        "  #''''''&#%'''''%#%''''''''%##                           "
        " #$'''''''&'''''%#%'''''''''%##                           "
        " #%''''''''''''%#%'''''''''%##                            "
        " #&'''''''''''%#%'''''''''%##                             "
        " #&''''''''''&#%'''''''''%##                              "
        " #''''''''''''&'''''''''%##                               "
        " #'''''''''''''''''''''%##                                "
        "##''''''''''''''''''''%##                                 "
        " #'''''''''''''''''''%##                                  "
        " #&'''''''''''''''''%##                                   "
        " #&'''''''''''''''&$##                                    "
        " #$'''''''''''''&$###                                     "
        "  #%'''''''''&%###                                        "
        "   #$%&&&&%$####                                          "
        "    #########                                             "
    }
};

struct hotkey_definition_t {
    int *action;
    int value;
    int key;
    int modifiers;
    int repeatable;
};

struct arrow_definition_t {
    void (*action)(int is_down);
    int key;
};

struct global_hotkeys_t {
    int toggle_fullscreen;
    int reset_window;
    int save_screenshot;
    int save_city_screenshot;
};

static struct {
    struct global_hotkeys_t global_hotkey_state;
    struct hotkeys_t hotkey_state;
    struct hotkey_definition_t *definitions;
    int num_definitions;
    struct arrow_definition_t *arrows;
    int num_arrows;
} hotkey_data;

static struct {
    int capture;
    int accepted;

    int capture_numeric;
    void (*capture_numeric_callback)(int);

    char *text;
    int cursor_position;
    int length;
    int max_length;
    int allow_punctuation;

    int viewport_start;
    int viewport_end;
    int viewport_cursor_position;

    int box_width;
    int font;
} keyboard_data;

static const char *key_names[KEY_TYPE_MAX_ITEMS] = {
    "", "A", "B", "C", "D", "E", "F", "G", "H", "I",
    "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S",
    "T", "U", "V", "W", "X", "Y", "Z", "1", "2", "3",
    "4", "5", "6", "7", "8", "9", "0", "-", "=", "Enter",
    "Esc", "Backspace", "Tab", "Space", "[", "]", "\\", ";", "'", "`",
    ",", ".", "/", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
    "F8", "F9", "F10", "F11", "F12", "Insert", "Delete", "Home", "End", "PageUp",
    "PageDown", "Right", "Left", "Down", "Up",
    "Kp1", "Kp2", "Kp3", "Kp4", "Kp5", "Kp6", "Kp7", "Kp8", "Kp9", "Kp0",
    "Kp.", "Kp+", "Kp-", "Kp*", "Kp/", "NonUS"
};

struct modifier_name_t {
    int modifier;
    const char *name;
};

static const struct modifier_name_t modifier_names[] = {
    {KEY_MOD_CTRL, "Ctrl"},
    {KEY_MOD_ALT, "Alt"},
    {KEY_MOD_GUI, "Gui"},
    {KEY_MOD_SHIFT, "Shift"},
    {KEY_MOD_NONE, 0}
};

enum {
    SYSTEM_NONE = 0,
    SYSTEM_UP = 1,
    SYSTEM_DOWN = 2,
    SYSTEM_DOUBLE_CLICK = 4
};

static struct mouse_t mouse_data;
static struct mouse_t dialog;
static uint32_t last_click;

static const int DIRECTION_X[] = { 0,  1,  1,  1,  0, -1, -1, -1, 0 };
static const int DIRECTION_Y[] = { -1, -1,  0,  1,  1,  1,  0, -1, 0 };
static const int SCROLL_STEP[SCROLL_TYPE_MAX][11] = {
    {60, 44, 30, 20, 16, 12, 10, 8, 6, 4, 2},
    {20, 15, 10,  7,  5,  4,  3, 3, 2, 2, 1}
};

enum {
    KEY_STATE_UNPRESSED = 0,
    KEY_STATE_PRESSED = 1,
    KEY_STATE_HELD = 2,
    KEY_STATE_AXIS = 3
};

struct key_t {
    int state;
    int value;
    uint32_t last_change;
};

static struct {
    int is_scrolling;
    int constant_input;
    struct {
        struct key_t up;
        struct key_t down;
        struct key_t left;
        struct key_t right;
    } arrow_key;
    struct {
        int active;
        int has_started;
        struct pixel_view_coordinates_t delta;
    } drag;
    struct {
        struct speed_type_t x;
        struct speed_type_t y;
        float modifier_x;
        float modifier_y;
    } speed;
    int x_align_direction;
    int y_align_direction;
    uint32_t last_time;
    struct {
        int active;
        int x;
        int y;
        int width;
        int height;
    } limits;
} scroll_data;

const struct cursor_t *input_cursor_data(int cursor_id, int scale)
{
    switch (cursor_id) {
        case CURSOR_ARROW: return &ARROW[scale];
        case CURSOR_SHOVEL: return &SHOVEL[scale];
        case CURSOR_SWORD: return &SWORD[scale];
        default: return 0;
    }
}

void input_cursor_update(int window)
{
    if (window == WINDOW_CITY_MILITARY) {
        set_cursor(CURSOR_SWORD);
    } else if (window == WINDOW_CITY && building_construction_type() == BUILDING_CLEAR_LAND) {
        set_cursor(CURSOR_SHOVEL);
    } else {
        set_cursor(CURSOR_ARROW);
    }
}

static void set_arrow_key(struct key_t *arrow, int value)
{
    int state = KEY_STATE_AXIS;
    if (!value) {
        state = KEY_STATE_UNPRESSED;
    }
    if (value == SCROLL_KEY_PRESSED) {
        state = KEY_STATE_PRESSED;
    }
    if (state != KEY_STATE_AXIS && state != KEY_STATE_UNPRESSED &&
        arrow->state != KEY_STATE_AXIS && arrow->state != KEY_STATE_UNPRESSED) {
        return;
    }
    // Key should retain axis state even if its value is zero
    if (arrow->state != KEY_STATE_AXIS || state != KEY_STATE_UNPRESSED) {
        arrow->state = state;
    }
    arrow->value = value;
    arrow->last_change = time_get_millis();
}

static void scroll_arrow_up(int value)
{
    set_arrow_key(&scroll_data.arrow_key.up, value);
}

static void scroll_arrow_down(int value)
{
    set_arrow_key(&scroll_data.arrow_key.down, value);
}

static void scroll_arrow_left(int value)
{
    set_arrow_key(&scroll_data.arrow_key.left, value);
}

static void scroll_arrow_right(int value)
{
    set_arrow_key(&scroll_data.arrow_key.right, value);
}

void hotkey_install_mapping(struct hotkey_mapping_t *mappings, int num_mappings)
{
    int total_definitions = 2; // Enter and ESC are fixed hotkeys
    int total_arrows = 0;
    for (int i = 0; i < num_mappings; i++) {
        int action = mappings[i].action;
        if (action == HOTKEY_ARROW_UP || action == HOTKEY_ARROW_DOWN ||
            action == HOTKEY_ARROW_LEFT || action == HOTKEY_ARROW_RIGHT) {
            total_arrows++;
        } else {
            total_definitions++;
        }
    }
    if (!total_arrows) {
        return;
    }
    free(hotkey_data.definitions);
    free(hotkey_data.arrows);
    hotkey_data.num_definitions = 0;
    hotkey_data.num_arrows = 0;
    hotkey_data.definitions = malloc(sizeof(struct hotkey_definition_t) * total_definitions);
    hotkey_data.arrows = malloc(sizeof(struct arrow_definition_t) * total_arrows);
    if (!hotkey_data.definitions || !hotkey_data.arrows) {
        free(hotkey_data.definitions);
        free(hotkey_data.arrows);
        return;
    }
    // Fixed keys: Escape and Enter
    hotkey_data.definitions[0].action = &hotkey_data.hotkey_state.enter_pressed;
    hotkey_data.definitions[0].key = KEY_TYPE_ENTER;
    hotkey_data.definitions[0].modifiers = 0;
    hotkey_data.definitions[0].repeatable = 0;
    hotkey_data.definitions[0].value = 1;

    hotkey_data.definitions[1].action = &hotkey_data.hotkey_state.escape_pressed;
    hotkey_data.definitions[1].key = KEY_TYPE_ESCAPE;
    hotkey_data.definitions[1].modifiers = 0;
    hotkey_data.definitions[1].repeatable = 0;
    hotkey_data.definitions[1].value = 1;

    hotkey_data.num_definitions = 2;

    for (int i = 0; i < num_mappings; i++) {
        int action = mappings[i].action;
        if (action == HOTKEY_ARROW_UP || action == HOTKEY_ARROW_DOWN ||
            action == HOTKEY_ARROW_LEFT || action == HOTKEY_ARROW_RIGHT) {
            struct arrow_definition_t *arrow = &hotkey_data.arrows[hotkey_data.num_arrows];
            arrow->key = mappings[i].key;
            switch (mappings[i].action) {
                case HOTKEY_ARROW_UP:
                    arrow->action = scroll_arrow_up;
                    break;
                case HOTKEY_ARROW_DOWN:
                    arrow->action = scroll_arrow_down;
                    break;
                case HOTKEY_ARROW_LEFT:
                    arrow->action = scroll_arrow_left;
                    break;
                case HOTKEY_ARROW_RIGHT:
                    arrow->action = scroll_arrow_right;
                    break;
                default:
                    arrow->action = 0;
                    break;
            }
            if (arrow->action) {
                hotkey_data.num_arrows++;
            }
        } else {
            struct hotkey_definition_t *def = &hotkey_data.definitions[hotkey_data.num_definitions];
            def->key = mappings[i].key;
            def->modifiers = mappings[i].modifiers;
            def->value = 1;
            def->repeatable = 0;
            switch (mappings[i].action) {
                case HOTKEY_TOGGLE_FULLSCREEN:
                    def->action = &hotkey_data.global_hotkey_state.toggle_fullscreen;
                    break;
                case HOTKEY_RESET_WINDOW:
                    def->action = &hotkey_data.global_hotkey_state.reset_window;
                    break;
                case HOTKEY_SAVE_SCREENSHOT:
                    def->action = &hotkey_data.global_hotkey_state.save_screenshot;
                    break;
                case HOTKEY_SAVE_CITY_SCREENSHOT:
                    def->action = &hotkey_data.global_hotkey_state.save_city_screenshot;
                    break;
                case HOTKEY_LOAD_FILE:
                    def->action = &hotkey_data.hotkey_state.load_file;
                    break;
                case HOTKEY_SAVE_FILE:
                    def->action = &hotkey_data.hotkey_state.save_file;
                    break;
                case HOTKEY_DECREASE_GAME_SPEED:
                    def->action = &hotkey_data.hotkey_state.decrease_game_speed;
                    def->repeatable = 1;
                    break;
                case HOTKEY_INCREASE_GAME_SPEED:
                    def->action = &hotkey_data.hotkey_state.increase_game_speed;
                    def->repeatable = 1;
                    break;
                case HOTKEY_TOGGLE_PAUSE:
                    def->action = &hotkey_data.hotkey_state.toggle_pause;
                    break;
                case HOTKEY_ROTATE_MAP_LEFT:
                    def->action = &hotkey_data.hotkey_state.rotate_map_left;
                    break;
                case HOTKEY_ROTATE_MAP_RIGHT:
                    def->action = &hotkey_data.hotkey_state.rotate_map_right;
                    break;
                case HOTKEY_REPLAY_MAP:
                    def->action = &hotkey_data.hotkey_state.replay_map;
                    break;
                case HOTKEY_CYCLE_LEGION:
                    def->action = &hotkey_data.hotkey_state.cycle_legion;
                    break;
                case HOTKEY_RETURN_LEGIONS_TO_FORT:
                    def->action = &hotkey_data.hotkey_state.return_legions_to_fort;
                    break;
                case HOTKEY_SHOW_LAST_ADVISOR:
                    def->action = &hotkey_data.hotkey_state.show_last_advisor;
                    break;
                case HOTKEY_SHOW_EMPIRE_MAP:
                    def->action = &hotkey_data.hotkey_state.show_empire_map;
                    break;
                case HOTKEY_SHOW_MESSAGES:
                    def->action = &hotkey_data.hotkey_state.show_messages;
                    break;
                case HOTKEY_GO_TO_PROBLEM:
                    def->action = &hotkey_data.hotkey_state.go_to_problem;
                    break;
                case HOTKEY_SHOW_OVERLAY_WATER:
                    def->action = &hotkey_data.hotkey_state.show_overlay;
                    def->value = OVERLAY_WATER;
                    break;
                case HOTKEY_SHOW_OVERLAY_FIRE:
                    def->action = &hotkey_data.hotkey_state.show_overlay;
                    def->value = OVERLAY_FIRE;
                    break;
                case HOTKEY_SHOW_OVERLAY_DAMAGE:
                    def->action = &hotkey_data.hotkey_state.show_overlay;
                    def->value = OVERLAY_DAMAGE;
                    break;
                case HOTKEY_SHOW_OVERLAY_CRIME:
                    def->action = &hotkey_data.hotkey_state.show_overlay;
                    def->value = OVERLAY_CRIME;
                    break;
                case HOTKEY_SHOW_OVERLAY_PROBLEMS:
                    def->action = &hotkey_data.hotkey_state.show_overlay;
                    def->value = OVERLAY_PROBLEMS;
                    break;
                case HOTKEY_GO_TO_BOOKMARK_1:
                    def->action = &hotkey_data.hotkey_state.go_to_bookmark;
                    def->value = 1;
                    break;
                case HOTKEY_GO_TO_BOOKMARK_2:
                    def->action = &hotkey_data.hotkey_state.go_to_bookmark;
                    def->value = 2;
                    break;
                case HOTKEY_GO_TO_BOOKMARK_3:
                    def->action = &hotkey_data.hotkey_state.go_to_bookmark;
                    def->value = 3;
                    break;
                case HOTKEY_GO_TO_BOOKMARK_4:
                    def->action = &hotkey_data.hotkey_state.go_to_bookmark;
                    def->value = 4;
                    break;
                case HOTKEY_SET_BOOKMARK_1:
                    def->action = &hotkey_data.hotkey_state.set_bookmark;
                    def->value = 1;
                    break;
                case HOTKEY_SET_BOOKMARK_2:
                    def->action = &hotkey_data.hotkey_state.set_bookmark;
                    def->value = 2;
                    break;
                case HOTKEY_SET_BOOKMARK_3:
                    def->action = &hotkey_data.hotkey_state.set_bookmark;
                    def->value = 3;
                    break;
                case HOTKEY_SET_BOOKMARK_4:
                    def->action = &hotkey_data.hotkey_state.set_bookmark;
                    def->value = 4;
                    break;
                case HOTKEY_EDITOR_TOGGLE_BATTLE_INFO:
                    def->action = &hotkey_data.hotkey_state.toggle_editor_battle_info;
                    break;
                case HOTKEY_CHEAT_MONEY:
                    def->action = &hotkey_data.hotkey_state.cheat_money;
                    def->repeatable = 1;
                    break;
                case HOTKEY_CHEAT_INVASION:
                    def->action = &hotkey_data.hotkey_state.cheat_invasion;
                    break;
                case HOTKEY_CHEAT_VICTORY:
                    def->action = &hotkey_data.hotkey_state.cheat_victory;
                    break;
                case HOTKEY_BUILD_CLONE:
                    def->action = &hotkey_data.hotkey_state.clone_building;
                    break;
                case HOTKEY_CYCLE_BUILDINGS:
                    def->action = &hotkey_data.hotkey_state.cycle_buildings;
                    def->repeatable = 1;
                    break;
                case HOTKEY_CYCLE_BUILDINGS_REVERSE:
                    def->action = &hotkey_data.hotkey_state.cycle_buildings_reverse;
                    def->repeatable = 1;
                    break;
                case HOTKEY_UNDO:
                    def->action = &hotkey_data.hotkey_state.undo;
                    break;
                case HOTKEY_BUILD_VACANT_HOUSE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_HOUSE_VACANT_LOT;
                    break;
                case HOTKEY_BUILD_CLEAR_LAND:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_CLEAR_LAND;
                    break;
                case HOTKEY_BUILD_ROAD:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_ROAD;
                    break;
                case HOTKEY_BUILD_RESERVOIR:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_RESERVOIR;
                    break;
                case HOTKEY_BUILD_AQUEDUCT:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_AQUEDUCT;
                    break;
                case HOTKEY_BUILD_FOUNTAIN:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_FOUNTAIN;
                    break;
                case HOTKEY_BUILD_WELL:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_WELL;
                    break;
                case HOTKEY_BUILD_BARBER:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_BARBER;
                    break;
                case HOTKEY_BUILD_BATHHOUSE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_BATHHOUSE;
                    break;
                case HOTKEY_BUILD_DOCTOR:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_DOCTOR;
                    break;
                case HOTKEY_BUILD_HOSPITAL:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_HOSPITAL;
                    break;
                case HOTKEY_BUILD_SMALL_TEMPLE_CERES:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_SMALL_TEMPLE_CERES;
                    break;
                case HOTKEY_BUILD_LARGE_TEMPLE_CERES:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_LARGE_TEMPLE_CERES;
                    break;
                case HOTKEY_BUILD_ORACLE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_ORACLE;
                    break;
                case HOTKEY_BUILD_SCHOOL:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_SCHOOL;
                    break;
                case HOTKEY_BUILD_ACADEMY:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_ACADEMY;
                    break;
                case HOTKEY_BUILD_LIBRARY:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_LIBRARY;
                    break;
                case HOTKEY_BUILD_MISSION_POST:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_MISSION_POST;
                    break;
                case HOTKEY_BUILD_THEATER:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_THEATER;
                    break;
                case HOTKEY_BUILD_AMPHITHEATER:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_AMPHITHEATER;
                    break;
                case HOTKEY_BUILD_COLOSSEUM:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_COLOSSEUM;
                    break;
                case HOTKEY_BUILD_HIPPODROME:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_HIPPODROME;
                    break;
                case HOTKEY_BUILD_GLADIATOR_SCHOOL:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_GLADIATOR_SCHOOL;
                    break;
                case HOTKEY_BUILD_LION_HOUSE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_LION_HOUSE;
                    break;
                case HOTKEY_BUILD_ACTOR_COLONY:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_ACTOR_COLONY;
                    break;
                case HOTKEY_BUILD_CHARIOT_MAKER:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_CHARIOT_MAKER;
                    break;
                case HOTKEY_BUILD_FORUM:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_FORUM;
                    break;
                case HOTKEY_BUILD_SENATE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_SENATE;
                    break;
                case HOTKEY_BUILD_GOVERNORS_HOUSE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_GOVERNORS_HOUSE;
                    break;
                case HOTKEY_BUILD_GOVERNORS_VILLA:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_GOVERNORS_VILLA;
                    break;
                case HOTKEY_BUILD_GOVERNORS_PALACE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_GOVERNORS_PALACE;
                    break;
                case HOTKEY_BUILD_SMALL_STATUE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_SMALL_STATUE;
                    break;
                case HOTKEY_BUILD_MEDIUM_STATUE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_MEDIUM_STATUE;
                    break;
                case HOTKEY_BUILD_LARGE_STATUE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_LARGE_STATUE;
                    break;
                case HOTKEY_BUILD_GARDENS:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_GARDENS;
                    break;
                case HOTKEY_BUILD_PLAZA:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_PLAZA;
                    break;
                case HOTKEY_BUILD_ENGINEERS_POST:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_ENGINEERS_POST;
                    break;
                case HOTKEY_BUILD_LOW_BRIDGE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_LOW_BRIDGE;
                    break;
                case HOTKEY_BUILD_SHIP_BRIDGE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_SHIP_BRIDGE;
                    break;
                case HOTKEY_BUILD_SHIPYARD:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_SHIPYARD;
                    break;
                case HOTKEY_BUILD_DOCK:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_DOCK;
                    break;
                case HOTKEY_BUILD_WHARF:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_WHARF;
                    break;
                case HOTKEY_BUILD_WALL:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_WALL;
                    break;
                case HOTKEY_BUILD_TOWER:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_TOWER;
                    break;
                case HOTKEY_BUILD_GATEHOUSE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_GATEHOUSE;
                    break;
                case HOTKEY_BUILD_PREFECTURE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_PREFECTURE;
                    break;
                case HOTKEY_BUILD_FORT_LEGIONARIES:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_FORT_LEGIONARIES;
                    break;
                case HOTKEY_BUILD_FORT_JAVELIN:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_FORT_JAVELIN;
                    break;
                case HOTKEY_BUILD_FORT_MOUNTED:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_FORT_MOUNTED;
                    break;
                case HOTKEY_BUILD_MILITARY_ACADEMY:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_MILITARY_ACADEMY;
                    break;
                case HOTKEY_BUILD_BARRACKS:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_BARRACKS;
                    break;
                case HOTKEY_BUILD_WHEAT_FARM:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_WHEAT_FARM;
                    break;
                case HOTKEY_BUILD_VEGETABLE_FARM:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_VEGETABLE_FARM;
                    break;
                case HOTKEY_BUILD_FRUIT_FARM:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_FRUIT_FARM;
                    break;
                case HOTKEY_BUILD_OLIVE_FARM:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_OLIVE_FARM;
                    break;
                case HOTKEY_BUILD_VINES_FARM:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_VINES_FARM;
                    break;
                case HOTKEY_BUILD_PIG_FARM:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_PIG_FARM;
                    break;
                case HOTKEY_BUILD_CLAY_PIT:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_CLAY_PIT;
                    break;
                case HOTKEY_BUILD_MARBLE_QUARRY:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_MARBLE_QUARRY;
                    break;
                case HOTKEY_BUILD_IRON_MINE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_IRON_MINE;
                    break;
                case HOTKEY_BUILD_TIMBER_YARD:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_TIMBER_YARD;
                    break;
                case HOTKEY_BUILD_WINE_WORKSHOP:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_WINE_WORKSHOP;
                    break;
                case HOTKEY_BUILD_OIL_WORKSHOP:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_OIL_WORKSHOP;
                    break;
                case HOTKEY_BUILD_WEAPONS_WORKSHOP:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_WEAPONS_WORKSHOP;
                    break;
                case HOTKEY_BUILD_FURNITURE_WORKSHOP:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_FURNITURE_WORKSHOP;
                    break;
                case HOTKEY_BUILD_POTTERY_WORKSHOP:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_POTTERY_WORKSHOP;
                    break;
                case HOTKEY_BUILD_MARKET:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_MARKET;
                    break;
                case HOTKEY_BUILD_GRANARY:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_GRANARY;
                    break;
                case HOTKEY_BUILD_WAREHOUSE:
                    def->action = &hotkey_data.hotkey_state.building;
                    def->value = BUILDING_WAREHOUSE;
                    break;
                default:
                    def->action = 0;
            }
            if (def->action) {
                hotkey_data.num_definitions++;
            }
        }
    }
}

const struct hotkeys_t *hotkey_state(void)
{
    return &hotkey_data.hotkey_state;
}

void hotkey_reset_state(void)
{
    memset(&hotkey_data.hotkey_state, 0, sizeof(hotkey_data.hotkey_state));
    memset(&hotkey_data.global_hotkey_state, 0, sizeof(hotkey_data.global_hotkey_state));
}

void hotkey_key_pressed(int key, int modifiers, int repeat)
{
    if (window_is(WINDOW_HOTKEY_EDITOR)) {
        window_hotkey_editor_key_pressed(key, modifiers);
        return;
    }
    if (key == KEY_TYPE_NONE) {
        return;
    }
    int found_action = 0;
    for (int i = 0; i < hotkey_data.num_definitions; i++) {
        struct hotkey_definition_t *def = &hotkey_data.definitions[i];
        if (def->key == key && def->modifiers == modifiers && (!repeat || def->repeatable)) {
            *(def->action) = def->value;
            found_action = 1;
        }
    }
    if (found_action) {
        return;
    }
    for (int i = 0; i < hotkey_data.num_arrows; i++) {
        struct arrow_definition_t *arrow = &hotkey_data.arrows[i];
        if (arrow->key == key) {
            arrow->action(1);
        }
    }
}

void hotkey_key_released(int key, int modifiers)
{
    if (window_is(WINDOW_HOTKEY_EDITOR)) {
        window_hotkey_editor_key_released(key, modifiers);
        return;
    }
    if (key == KEY_TYPE_NONE) {
        return;
    }
    for (int i = 0; i < hotkey_data.num_arrows; i++) {
        struct arrow_definition_t *arrow = &hotkey_data.arrows[i];
        if (arrow->key == key) {
            arrow->action(0);
        }
    }
}

void hotkey_handle_global_keys(void)
{
    if (hotkey_data.global_hotkey_state.reset_window) {
        system_resize(1280, 800);
        post_event(USER_EVENT_CENTER_WINDOW);
    }
    if (hotkey_data.global_hotkey_state.toggle_fullscreen) {
        post_event(setting_fullscreen() ? USER_EVENT_WINDOWED : USER_EVENT_FULLSCREEN);
    }
    if (hotkey_data.global_hotkey_state.save_screenshot) {
        graphics_save_screenshot(0);
    }
    if (hotkey_data.global_hotkey_state.save_city_screenshot) {
        graphics_save_screenshot(1);
    }
}

static void set_viewport_to_start(void)
{
    keyboard_data.viewport_start = 0;
    keyboard_data.viewport_end = text_get_max_length_for_width(keyboard_data.text, keyboard_data.length, keyboard_data.font, keyboard_data.box_width, 0);
}

static void set_viewport_to_end(void)
{
    keyboard_data.viewport_end = keyboard_data.length;
    int maxlen = text_get_max_length_for_width(keyboard_data.text, keyboard_data.length, keyboard_data.font, keyboard_data.box_width, 1);
    keyboard_data.viewport_start = keyboard_data.length - maxlen;
}

static void update_viewport(int has_changed)
{
    int is_within_viewport = keyboard_data.cursor_position >= keyboard_data.viewport_start && keyboard_data.cursor_position <= keyboard_data.viewport_end;
    if (!has_changed && is_within_viewport) {
        // no update necessary
    } else if (keyboard_data.cursor_position == 0) {
        set_viewport_to_start();
    } else if (keyboard_data.cursor_position == keyboard_data.length) {
        set_viewport_to_end();
    } else {
        // first check if we can keep the viewport
        int new_start = keyboard_data.viewport_start;
        int new_end = text_get_max_length_for_width(keyboard_data.text, keyboard_data.length - new_start, keyboard_data.font, keyboard_data.box_width, 0);
        if (keyboard_data.cursor_position < new_start && keyboard_data.cursor_position >= new_end && new_start + new_end >= keyboard_data.length) {
            if (keyboard_data.cursor_position <= keyboard_data.viewport_cursor_position) {
                // move toward start
                int maxlen = text_get_max_length_for_width(
                    keyboard_data.text + keyboard_data.cursor_position,
                    keyboard_data.length - keyboard_data.cursor_position,
                    keyboard_data.font, keyboard_data.box_width, 0);
                if (keyboard_data.cursor_position + maxlen < keyboard_data.length) {
                    keyboard_data.viewport_start = keyboard_data.cursor_position;
                    keyboard_data.viewport_end = keyboard_data.cursor_position + maxlen;
                } else {
                    // all remaining text fits: set to end
                    set_viewport_to_end();
                }
            } else {
                // move toward end
                int viewport_length = keyboard_data.cursor_position + 1;
                int maxlen = text_get_max_length_for_width(
                    keyboard_data.text, viewport_length, keyboard_data.font, keyboard_data.box_width, 1);
                if (maxlen < viewport_length) {
                    keyboard_data.viewport_start = viewport_length - maxlen;
                    keyboard_data.viewport_end = viewport_length;
                } else {
                    // all remaining text fits: set to start
                    set_viewport_to_start();
                }
            }
        }
    }
    keyboard_data.viewport_cursor_position = keyboard_data.cursor_position;
}

void keyboard_start_capture(char *text, int max_length, int allow_punctuation, int box_width, int font)
{
    keyboard_data.capture = 1;
    keyboard_data.text = text;
    keyboard_data.length = string_length(text);
    keyboard_data.cursor_position = keyboard_data.length;
    keyboard_data.max_length = max_length;
    keyboard_data.allow_punctuation = allow_punctuation;
    keyboard_data.accepted = 0;
    keyboard_data.box_width = box_width;
    keyboard_data.font = font;
    update_viewport(1);
    SDL_StartTextInput();
}

void keyboard_refresh(void)
{
    keyboard_data.length = string_length(keyboard_data.text);
    keyboard_data.cursor_position = keyboard_data.length;
    update_viewport(1);
}

void keyboard_stop_capture(void)
{
    keyboard_data.capture = 0;
    keyboard_data.text = 0;
    keyboard_data.cursor_position = 0;
    keyboard_data.length = 0;
    keyboard_data.max_length = 0;
    keyboard_data.accepted = 0;
    SDL_StopTextInput();
}

void keyboard_start_capture_numeric(void (*callback)(int))
{
    keyboard_data.capture_numeric = 1;
    keyboard_data.capture_numeric_callback = callback;
    SDL_StartTextInput();
}

void keyboard_stop_capture_numeric(void)
{
    keyboard_data.capture_numeric = 0;
    keyboard_data.capture_numeric_callback = 0;
    SDL_StopTextInput();
}

int keyboard_input_is_accepted(void)
{
    if (keyboard_data.accepted) {
        keyboard_data.accepted = 0;
        return 1;
    } else {
        return 0;
    }
}

int keyboard_cursor_position(void)
{
    return keyboard_data.cursor_position - keyboard_data.viewport_start;
}

int keyboard_offset_start(void)
{
    return keyboard_data.viewport_start;
}

int keyboard_offset_end(void)
{
    return keyboard_data.viewport_end;
}

void keyboard_return(void)
{
    keyboard_data.accepted = 1;
}

static void move_left(char *start, const char *end)
{
    while (start < end) {
        start[0] = start[1];
        start++;
    }
    *start = 0;
}

static void move_right(const char *start, char *end)
{
    end[1] = 0;
    while (end > start) {
        end--;
        end[1] = end[0];
    }
}

static void remove_current_char(void)
{
    move_left(&keyboard_data.text[keyboard_data.cursor_position], &keyboard_data.text[keyboard_data.length]);
    keyboard_data.length -= 1;
}

void keyboard_backspace(void)
{
    if (keyboard_data.capture && keyboard_data.cursor_position > 0) {
        keyboard_data.cursor_position--;
        remove_current_char();
        update_viewport(1);
    }
}

void keyboard_delete(void)
{
    if (keyboard_data.capture && keyboard_data.cursor_position < keyboard_data.length) {
        remove_current_char();
        update_viewport(1);
    }
}

void keyboard_left(void)
{
    if (keyboard_data.capture) {
        if (keyboard_data.cursor_position > 0) {
            keyboard_data.cursor_position--;
            update_viewport(0);
        }
    }
}

void keyboard_right(void)
{
    if (keyboard_data.capture) {
        if (keyboard_data.cursor_position < keyboard_data.length) {
            keyboard_data.cursor_position += 1;
            update_viewport(0);
        }
    }
}

void keyboard_home(void)
{
    if (keyboard_data.capture) {
        keyboard_data.cursor_position = 0;
        update_viewport(0);
    }
}

void keyboard_end(void)
{
    if (keyboard_data.capture) {
        keyboard_data.cursor_position = keyboard_data.length;
        update_viewport(0);
    }
}

void keyboard_text(const char *text)
{
    if (keyboard_data.capture_numeric) {
        char c = text[0];
        if (c >= '0' && c <= '9') {
            keyboard_data.capture_numeric_callback(c - '0');
        }
        return;
    }
    if (!keyboard_data.capture) {
        return;
    }
    int index = 0;
    while (text[index]) {
        char c = text[0];
        int add = 0;
        if (c == ' ' || c == '-') {
            add = 1;
        } else if (c >= '0' && c <= '9') {
            add = 1;
        } else if (c >= 'a' && c <= 'z') {
            add = 1;
        } else if (c >= 'A' && c <= 'Z') {
            add = 1;
        } else if (c == ',' || c == '.' || c == '?' || c == '!' || c == '@' || c == '%' || c == '\'' || c == '/' || c == '_') {
            add = keyboard_data.allow_punctuation;
        }
        if (add) {
            if (keyboard_data.length + 1 < keyboard_data.max_length) {
                move_right(&keyboard_data.text[keyboard_data.cursor_position], &keyboard_data.text[keyboard_data.length]);
                keyboard_data.text[keyboard_data.cursor_position] = text[0];
                keyboard_data.cursor_position++;
                keyboard_data.length += 1;
                update_viewport(1);
            }
        }
        index++;
    }
}

const char *key_combination_name(int key, int modifiers)
{
    static char name[100];
    name[0] = 0;
    for (const struct modifier_name_t *modname = modifier_names; modname->modifier; modname++) {
        if (modifiers & modname->modifier) {
            strcat(name, modname->name);
            strcat(name, " ");
        }
    }
    strcat(name, key_names[key]);
    return name;
}

int key_combination_from_name(const char *name, int *key, int *modifiers)
{
    char editable_name[100] = { 0 };
    string_copy(name, editable_name, 99);

    *key = KEY_TYPE_NONE;
    *modifiers = KEY_MOD_NONE;

    char *token = strtok(editable_name, " ");
    while (token) {
        if (token[0]) {
            int mod = KEY_MOD_NONE;
            for (const struct modifier_name_t *modname = modifier_names; modname->modifier; modname++) {
                if (string_equals(modname->name, token)) {
                    mod = modname->modifier;
                }
            }
            if (mod != KEY_MOD_NONE) {
                *modifiers |= mod;
            } else {
                *key = KEY_TYPE_NONE;
                for (int i = 1; i < KEY_TYPE_MAX_ITEMS; i++) {
                    if (string_equals(key_names[i], token)) {
                        *key = i;
                        break;
                    }
                }
                if (*key == KEY_TYPE_NONE) {
                    return 0;
                }
            }
        }
        token = strtok(0, " ");
    }
    if (*key == KEY_TYPE_NONE) {
        return 0;
    }
    return 1;
}

const struct mouse_t *mouse_get(void)
{
    return &mouse_data;
}

static void clear_mouse_button(struct mouse_button_t *button)
{
    button->is_down = 0;
    button->went_down = 0;
    button->went_up = 0;
    button->double_click = 0;
    button->system_change = SYSTEM_NONE;
}

void mouse_set_position(int x, int y)
{
    if (x != mouse_data.x || y != mouse_data.y) {
        last_click = 0;
    }
    mouse_data.x = x;
    mouse_data.y = y;
    mouse_data.is_inside_window = 1;
}

void mouse_set_left_down(int down)
{
    mouse_data.left.system_change |= down ? SYSTEM_DOWN : SYSTEM_UP;
    mouse_data.is_inside_window = 1;
    if (!down) {
        uint32_t now = time_get_millis();
        int is_double_click = (last_click < now) && ((now - last_click) <= DOUBLE_CLICK_TIME);
        mouse_data.left.system_change |= is_double_click ? SYSTEM_DOUBLE_CLICK : SYSTEM_NONE;
        last_click = now;
    }
}

void mouse_set_right_down(int down)
{
    mouse_data.right.system_change |= down ? SYSTEM_DOWN : SYSTEM_UP;
    mouse_data.is_inside_window = 1;
    last_click = 0;
}

void mouse_set_inside_window(int inside)
{
    mouse_data.is_inside_window = inside;
}

static void update_button_state(struct mouse_button_t *button)
{
    button->went_down = (button->system_change & SYSTEM_DOWN) == SYSTEM_DOWN;
    button->went_up = (button->system_change & SYSTEM_UP) == SYSTEM_UP;
    button->double_click = (button->system_change & SYSTEM_DOUBLE_CLICK) == SYSTEM_DOUBLE_CLICK;
    button->system_change = SYSTEM_NONE;
    button->is_down = (button->is_down || button->went_down) && !button->went_up;
}

void mouse_determine_button_state(void)
{
    update_button_state(&mouse_data.left);
    update_button_state(&mouse_data.right);
}

void mouse_set_scroll(int state)
{
    mouse_data.scrolled = state;
    mouse_data.is_inside_window = 1;
}

void mouse_reset_scroll(void)
{
    mouse_data.scrolled = SCROLL_NONE;
}

void mouse_reset_up_state(void)
{
    mouse_data.left.went_up = 0;
    mouse_data.right.went_up = 0;
}

void mouse_reset_button_state(void)
{
    last_click = 0;
    clear_mouse_button(&mouse_data.left);
    clear_mouse_button(&mouse_data.right);
}

const struct mouse_t *mouse_in_dialog(const struct mouse_t *m)
{
    dialog.left = m->left;
    dialog.right = m->right;
    dialog.scrolled = m->scrolled;
    dialog.is_inside_window = m->is_inside_window;

    dialog.x = m->x - screen_dialog_offset_x();
    dialog.y = m->y - screen_dialog_offset_y();
    return &dialog;
}

static void clear_scroll_speed(void)
{
    speed_clear(&scroll_data.speed.x);
    speed_clear(&scroll_data.speed.y);
    scroll_data.x_align_direction = SPEED_DIRECTION_STOPPED;
    scroll_data.y_align_direction = SPEED_DIRECTION_STOPPED;
}

static int get_arrow_key_value(struct key_t *arrow)
{
    if (arrow->state == KEY_STATE_AXIS) {
        return arrow->value;
    }
    return arrow->state != KEY_STATE_UNPRESSED;
}

int scroll_in_progress(void)
{
    return scroll_data.is_scrolling || scroll_data.drag.active;
}

void scroll_drag_start(void)
{
    if (scroll_data.drag.active || config_get(CONFIG_UI_DISABLE_RIGHT_CLICK_MAP_DRAG)) {
        return;
    }
    scroll_data.drag.active = 1;
    scroll_data.drag.delta.x = 0;
    scroll_data.drag.delta.y = 0;
    SDL_GetRelativeMouseState(0, 0);
    clear_scroll_speed();
}

int scroll_drag_end(void)
{
    if (!scroll_data.drag.active) {
        return 0;
    }

    int has_scrolled = scroll_data.drag.has_started;

    scroll_data.drag.active = 0;
    scroll_data.drag.has_started = 0;

    set_relative_mouse_mode(0);
    scroll_data.x_align_direction = speed_get_current_direction(&scroll_data.speed.x);
    scroll_data.y_align_direction = speed_get_current_direction(&scroll_data.speed.y);
    speed_set_target(&scroll_data.speed.x, 0, SPEED_CHANGE_IMMEDIATE, 1);
    speed_set_target(&scroll_data.speed.y, 0, SPEED_CHANGE_IMMEDIATE, 1);

    return has_scrolled;
}

static int set_arrow_input(struct key_t *arrow, const struct key_t *opposite_arrow, float *modifier)
{
    if (get_arrow_key_value(arrow) && (!opposite_arrow || opposite_arrow->value == 0)) {
        if (arrow->state == KEY_STATE_AXIS) {
            scroll_data.constant_input = 1;
            int value = get_arrow_key_value(arrow);
            if (value == SCROLL_KEY_PRESSED) {
                *modifier = 1.0f;
            } else {
                *modifier = fminf(arrow->value / SCROLL_KEY_MAX_VALUE, 1.0f);
            }
        }
        return 1;
    }
    return 0;
}

int scroll_get_delta(const struct mouse_t *m, struct pixel_view_coordinates_t *delta, int type)
{
    if (scroll_data.drag.active) {
        int delta_x = 0;
        int delta_y = 0;
        SDL_GetRelativeMouseState(&delta_x, &delta_y);
        scroll_data.drag.delta.x += delta_x;
        scroll_data.drag.delta.y += delta_y;
        if ((delta_x != 0 || delta_y != 0)) {
            set_relative_mouse_mode(1);
            // Store tiny movements until we decide that it's enough to move into scroll mode
            if (!scroll_data.drag.has_started) {
                scroll_data.drag.has_started = abs(scroll_data.drag.delta.x) > SCROLL_DRAG_MIN_DELTA
                    || abs(scroll_data.drag.delta.y) > SCROLL_DRAG_MIN_DELTA;
            }
        }
        if (scroll_data.drag.has_started) {
            speed_set_target(&scroll_data.speed.x, scroll_data.drag.delta.x, SPEED_CHANGE_IMMEDIATE, 0);
            speed_set_target(&scroll_data.speed.y, scroll_data.drag.delta.y, SPEED_CHANGE_IMMEDIATE, 0);
            scroll_data.drag.delta.x = 0;
            scroll_data.drag.delta.y = 0;
        }
        scroll_data.is_scrolling = 1;
    } else {
        int direction;
        int is_inside_window = m->is_inside_window;
        int width = screen_width();
        int height = screen_height();
        if (setting_fullscreen() && m->x < width && m->y < height) {
            // For Windows 10, in fullscreen mode, on HiDPI screens, this is needed
            // to get scrolling to work
            is_inside_window = 1;
        }
        if (is_inside_window) {
            int top = 0;
            int bottom = 0;
            int left = 0;
            int right = 0;
            int border = MOUSE_BORDER;
            int x = m->x;
            int y = m->y;
            scroll_data.constant_input = 0;
            scroll_data.speed.modifier_x = 0.0f;
            scroll_data.speed.modifier_y = 0.0f;
            if (scroll_data.limits.active) {
                border = TOUCH_BORDER;
                width = scroll_data.limits.width;
                height = scroll_data.limits.height;
                x -= scroll_data.limits.x;
                y -= scroll_data.limits.y;
                scroll_data.constant_input = 1;
            }
            // mouse near map edge
            // NOTE: using <= width/height (instead of <) to compensate for rounding
            // errors caused by scaling the display. SDL adds a 1px border to either
            // the right or the bottom when the aspect ratio does not match exactly.
            if (((!config_get(CONFIG_UI_DISABLE_MOUSE_EDGE_SCROLLING)) || scroll_data.limits.active) &&
                (x >= 0 && x <= width && y >= 0 && y <= height)) {
                if (x < border) {
                    left = 1;
                    scroll_data.speed.modifier_x = 1 - x / (float) border;
                } else if (x >= width - border) {
                    right = 1;
                    scroll_data.speed.modifier_x = 1 - (width - x) / (float) border;
                }
                if (y < border) {
                    top = 1;
                    scroll_data.speed.modifier_y = 1 - y / (float) border;
                } else if (y >= height - border) {
                    bottom = 1;
                    scroll_data.speed.modifier_y = 1 - (height - y) / (float) border;
                }
            }
            // keyboard/joystick arrow keys
            left |= set_arrow_input(&scroll_data.arrow_key.left, 0, &scroll_data.speed.modifier_x);
            right |= set_arrow_input(&scroll_data.arrow_key.right, &scroll_data.arrow_key.left, &scroll_data.speed.modifier_x);
            top |= set_arrow_input(&scroll_data.arrow_key.up, 0, &scroll_data.speed.modifier_y);
            bottom |= set_arrow_input(&scroll_data.arrow_key.down, &scroll_data.arrow_key.up, &scroll_data.speed.modifier_y);

            if (scroll_data.constant_input) {
                if (!scroll_data.speed.modifier_x) {
                    scroll_data.speed.modifier_x = scroll_data.speed.modifier_y;
                }
                if (!scroll_data.speed.modifier_y) {
                    scroll_data.speed.modifier_y = scroll_data.speed.modifier_x;
                }
            }
            // two sides
            if (left && top) {
                direction = DIR_7_TOP_LEFT;
            } else if (left && bottom) {
                direction = DIR_5_BOTTOM_LEFT;
            } else if (right && top) {
                direction = DIR_1_TOP_RIGHT;
            } else if (right && bottom) {
                direction = DIR_3_BOTTOM_RIGHT;
            } else if (left) { // one side
                direction = DIR_6_LEFT;
            } else if (right) {
                direction = DIR_2_RIGHT;
            } else if (top) {
                direction = DIR_0_TOP;
            } else if (bottom) {
                direction = DIR_4_BOTTOM;
            } else { // none of them
                direction = DIR_8_NONE;
            }
        } else {
            direction = DIR_8_NONE;
        }
        if (direction == DIR_8_NONE) {
            uint32_t time = SPEED_CHANGE_IMMEDIATE;
            speed_set_target(&scroll_data.speed.x, 0, time, 1);
            speed_set_target(&scroll_data.speed.y, 0, time, 1);
            scroll_data.is_scrolling = 0;
        } else {
            int dir_x = DIRECTION_X[direction];
            int dir_y = DIRECTION_Y[direction];
            int y_fraction = type == SCROLL_TYPE_CITY ? 2 : 1;
            int max_speed = SCROLL_STEP[type][calc_bound((100 - setting_scroll_speed()) / 10, 0, 10)];
            int max_speed_x = max_speed * dir_x;
            int max_speed_y = (max_speed / y_fraction) * dir_y;
            if (!scroll_data.constant_input) {
                if (speed_get_current_direction(&scroll_data.speed.x) * dir_x < 0) {
                    speed_invert(&scroll_data.speed.x);
                } else if (scroll_data.speed.x.desired_speed != max_speed_x) {
                    speed_set_target(&scroll_data.speed.x, max_speed_x, SPEED_CHANGE_IMMEDIATE, 1);
                }
                if (speed_get_current_direction(&scroll_data.speed.y) * dir_y < 0) {
                    speed_invert(&scroll_data.speed.y);
                } else if (scroll_data.speed.y.desired_speed != max_speed_y) {
                    speed_set_target(&scroll_data.speed.y, max_speed_y, SPEED_CHANGE_IMMEDIATE, 1);
                }
            } else {
                speed_set_target(&scroll_data.speed.x, (int) (max_speed_x * scroll_data.speed.modifier_x), SPEED_CHANGE_IMMEDIATE, 1);
                speed_set_target(&scroll_data.speed.y, (int) (max_speed_y * scroll_data.speed.modifier_y), SPEED_CHANGE_IMMEDIATE, 1);
            }
            scroll_data.is_scrolling = 1;
        }
    }
    delta->x = speed_get_delta(&scroll_data.speed.x);
    delta->y = speed_get_delta(&scroll_data.speed.y);
    return delta->x != 0 || delta->y != 0;
}

void scroll_stop(void)
{
    clear_scroll_speed();
    set_relative_mouse_mode(0);
    scroll_data.is_scrolling = 0;
    scroll_data.constant_input = 0;
    scroll_data.drag.active = 0;
    scroll_data.limits.active = 0;
}
