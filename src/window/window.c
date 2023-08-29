#include "window.h"

#include "city/city_new.h"
#include "core/calc.h"
#include "core/config.h"
#include "core/file.h"
#include "core/hotkey_config.h"
#include "core/image.h"
#include "core/lang.h"
#include "core/speed.h"
#include "core/string.h"
#include "core/time.h"
#include "editor/editor.h"
#include "empire/empire.h"
#include "empire/object.h"
#include "figure/formation_legion.h"
#include "figuretype/editor.h"
#include "game/game.h"
#include "graphics/graphics.h"
#include "platform/brutus.h"
#include "scenario/scenario.h"
#include "sound/sound.h"

#include "SDL_mixer.h"

#include <string.h>

#define CHIEF_ADVISOR_HEIGHT 23
#define X_OFFSET 232
#define MAX_ITEMS_PER_LIST 20
#define GROUP 5
#define PROCEED_GROUP 43
#define PROCEED_TEXT 5
#define MENU_X_OFFSET_OVERLAY_MENU 170
#define SUBMENU_X_OFFSET 348
#define MENU_Y_OFFSET_OVERLAY_MENU 72
#define MENU_ITEM_HEIGHT 24
#define MENU_CLICK_MARGIN 20
#define MAX_BUTTONS_OVERLAY_MENU 8
#define MAX_MESSAGES 10
#define MAX_HISTORY 200
#define MAX_EDITOR_FILES 9
#define MAX_BUTTONS_MAIN_MENU 5
#define NUM_INTRO_VIDEOS 3
#define DISPLAY_TIME_MILLIS 1000
#define NUM_BOTTOM_BUTTONS_HOTKEY_EDITOR_WINDOW 2
#define HOTKEY_HEADER -1
#define TR_NONE -1
#define GROUP_BUILDINGS 28
#define NUM_VISIBLE_OPTIONS 14
#define NUM_BOTTOM_BUTTONS_HOTKEY_CONFIG_WINDOW 3
#define HOTKEY_X_OFFSET_1 270
#define HOTKEY_X_OFFSET_2 420
#define HOTKEY_BTN_WIDTH 150
#define HOTKEY_BTN_HEIGHT 22
#define NUM_FILES_IN_VIEW 12
#define MAX_FILE_WINDOW_TEXT_WIDTH (18 * BLOCK_SIZE)
#define MAX_WIDTH 2032
#define MAX_HEIGHT 1136
#define MAX_WIDGETS 13
#define NUM_VISIBLE_ITEMS 16
#define NUM_BOTTOM_BUTTONS_WINDOW_CONFIG 4
#define ITEM_Y_OFFSET 60
#define ITEM_HEIGHT 24
#define CHECKBOX_CHECK_SIZE 20
#define CHECKBOX_HEIGHT 20
#define CHECKBOX_WIDTH 560
#define CHECKBOX_TEXT_WIDTH CHECKBOX_WIDTH - CHECKBOX_CHECK_SIZE - 15
#define NUMERICAL_RANGE_X 20
#define NUMERICAL_SLIDER_X 50
#define NUMERICAL_SLIDER_PADDING 2
#define NUMERICAL_DOT_SIZE 20
#define MAX_SCENARIOS 15
#define MENU_X_OFFSET_BUILD_MENU 258
#define MENU_Y_OFFSET_BUILD_MENU 110
#define MENU_ITEM_HEIGHT 24
#define MENU_ITEM_WIDTH 176
#define MENU_CLICK_MARGIN 20
#define MENU_NONE -1
#define TRADE_ADVISOR_HEIGHT 27
#define RATINGS_ADVISOR_HEIGHT 27
#define POPULATION_ADVISOR_HEIGHT 27
#define MILITARY_ADVISOR_HEIGHT 26
#define IMPERIAL_ADVISOR_HEIGHT 27
#define MAX_REQUESTS_SHOWN 5
#define HEALTH_ADVISOR_HEIGHT 18
#define FINANCIAL_ADVISOR_HEIGHT 26
#define ENTERTAINMENT_ADVISOR_HEIGHT 23
#define PEOPLE_OFFSET 330
#define COVERAGE_OFFSET 470
#define COVERAGE_WIDTH 130
#define EDUCATION_ADVISOR_HEIGHT 16
#define MAX_TILES 25
#define SLIDE_SPEED 7
#define SLIDE_ACCELERATION_MILLIS 65
#define SIDEBAR_DECELERATION_OFFSET 125
#define EXTRA_INFO_LINE_SPACE 16
#define EXTRA_INFO_HEIGHT_GAME_SPEED 64
#define EXTRA_INFO_HEIGHT_UNEMPLOYMENT 48
#define EXTRA_INFO_HEIGHT_RATINGS 176
#define EXTRA_INFO_VERTICAL_PADDING 8
#define MINIMAP_Y_OFFSET 59

enum {
    STATUS_NOT_ENOUGH_RESOURCES = -1,
    STATUS_CONFIRM_SEND_LEGIONS = -2,
    STATUS_NO_LEGIONS_SELECTED = -3,
    STATUS_NO_LEGIONS_AVAILABLE = -4,
};

enum {
    MODE_TEXT,
    MODE_GROUP,
};

enum {
    INTERMEZZO_MISSION_BRIEFING = 0,
    INTERMEZZO_FIRED = 1,
    INTERMEZZO_WON = 2,
};

enum {
    TYPE_NONE,
    TYPE_SPACE,
    TYPE_HEADER,
    TYPE_INPUT_BOX,
    TYPE_CHECKBOX,
    TYPE_NUMERICAL_DESC,
    TYPE_NUMERICAL_RANGE
};

enum {
    RANGE_DISPLAY_SCALE,
    RANGE_CURSOR_SCALE
};

enum {
    INFO_NONE = 0,
    INFO_FUNDS = 1,
    INFO_POPULATION = 2,
    INFO_DATE = 3
};

enum {
    FIGURE_COLOR_NONE = 0,
    FIGURE_COLOR_SOLDIER = 1,
    FIGURE_COLOR_SELECTED_SOLDIER = 2,
    FIGURE_COLOR_ENEMY = 3,
    FIGURE_COLOR_WOLF = 4
};

enum {
    REFRESH_NOT_NEEDED = 0,
    REFRESH_FULL = 1,
    REFRESH_CAMERA_MOVED = 2
};

enum {
    COLUMN_TYPE_RISK,
    COLUMN_TYPE_ACCESS
};

enum {
    SIDEBAR_EXTRA_DISPLAY_NONE = 0,
    SIDEBAR_EXTRA_DISPLAY_GAME_SPEED = 1,
    SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT = 2,
    SIDEBAR_EXTRA_DISPLAY_RATINGS = 4,
    SIDEBAR_EXTRA_DISPLAY_ALL = 7
};

static int focus_button_id_entertainment_advisor;
static int arrow_button_focus_financial_advisor;
static int goods_requests_to_draw;
static int selected_request_id;
static int focus_button_id_imperial_advisor;
static int focus_button_id_labor_advisor;
static int arrow_button_focus_labor_advisor;
static int focus_button_id_military_advisor;
static int focus_button_id_population_advisor;
static int focus_button_id_ratings_advisor;
static int focus_button_id_trade_advisor;

static const int ADVISOR_TO_MESSAGE_TEXT[] = {
    MESSAGE_DIALOG_ABOUT,
    MESSAGE_DIALOG_ADVISOR_LABOR,
    MESSAGE_DIALOG_ADVISOR_MILITARY,
    MESSAGE_DIALOG_ADVISOR_IMPERIAL,
    MESSAGE_DIALOG_ADVISOR_RATINGS,
    MESSAGE_DIALOG_ADVISOR_TRADE,
    MESSAGE_DIALOG_ADVISOR_POPULATION,
    MESSAGE_DIALOG_ADVISOR_HEALTH,
    MESSAGE_DIALOG_ADVISOR_EDUCATION,
    MESSAGE_DIALOG_ADVISOR_ENTERTAINMENT,
    MESSAGE_DIALOG_ADVISOR_RELIGION,
    MESSAGE_DIALOG_ADVISOR_FINANCIAL,
    MESSAGE_DIALOG_ADVISOR_CHIEF
};

static struct advisor_window_type_t *current_advisor_window = 0;
static int current_advisor = ADVISOR_NONE;

static int focus_button_id_advisors;
static int advisor_height;

static const int Y_MENU_OFFSETS[] = {
    0, 322, 306, 274, 258, 226, 210, 178, 162, 130, 114,
    82, 66, 34, 18, -30, -46, -62, -78, -78, -94,
    -94, -110, -110,
    0, 0, 0, 0, 0, 0
};

static const int BUILDING_MENU_SUBMENU_ITEM_MAPPING[BUILD_MENU_BUTTONS_COUNT][MAX_ITEMS_PER_BUILD_MENU][MAX_ITEMS_PER_SUBMENU] = {
    { // MENU_VACANT_HOUSE
        {BUILDING_HOUSE_VACANT_LOT},
    },
    { // MENU_CLEAR_LAND
        {BUILDING_CLEAR_LAND},
    },
    { // MENU_ROAD
        {BUILDING_ROAD},
    },
    { // MENU_WATER
        {BUILDING_RESERVOIR},
        {BUILDING_AQUEDUCT},
        {BUILDING_FOUNTAIN},
        {BUILDING_WELL},
    },
    { // MENU_HEALTH
        {BUILDING_DOCTOR},
        {BUILDING_BATHHOUSE},
        {BUILDING_BARBER},
        {BUILDING_HOSPITAL},
    },
    { // MENU_TEMPLES
        {BUILDING_SMALL_TEMPLE_CERES, BUILDING_SMALL_TEMPLE_NEPTUNE, BUILDING_SMALL_TEMPLE_MERCURY, BUILDING_SMALL_TEMPLE_MARS, BUILDING_SMALL_TEMPLE_VENUS},
        {BUILDING_LARGE_TEMPLE_CERES, BUILDING_LARGE_TEMPLE_NEPTUNE, BUILDING_LARGE_TEMPLE_MERCURY, BUILDING_LARGE_TEMPLE_MARS, BUILDING_LARGE_TEMPLE_VENUS},
        {BUILDING_ORACLE},
    },
    { // MENU_EDUCATION
        {BUILDING_SCHOOL},
        {BUILDING_LIBRARY},
        {BUILDING_ACADEMY},
        {BUILDING_MISSION_POST},
    },
    { // MENU_ENTERTAINMENT
        {BUILDING_THEATER},
        {BUILDING_ACTOR_COLONY},
        {BUILDING_AMPHITHEATER},
        {BUILDING_GLADIATOR_SCHOOL},
        {BUILDING_LION_HOUSE},
        {BUILDING_COLOSSEUM},
        {BUILDING_CHARIOT_MAKER},
        {BUILDING_HIPPODROME},
    },
    { // MENU_ADMINISTRATION
        {BUILDING_GARDENS},
        {BUILDING_PLAZA},
        {BUILDING_SMALL_STATUE},
        {BUILDING_MEDIUM_STATUE},
        {BUILDING_LARGE_STATUE},
        {BUILDING_GOVERNORS_HOUSE},
        {BUILDING_GOVERNORS_VILLA},
        {BUILDING_GOVERNORS_PALACE},
        {BUILDING_FORUM},
        {BUILDING_SENATE},
        {BUILDING_TRIUMPHAL_ARCH},
    },
    { // MENU_ENGINEERING
        {BUILDING_ENGINEERS_POST},
        {BUILDING_LOW_BRIDGE},
        {BUILDING_SHIP_BRIDGE},
        {BUILDING_SHIPYARD},
        {BUILDING_WHARF},
        {BUILDING_DOCK},
    },
    { // MENU_SECURITY
        {BUILDING_PREFECTURE},
        {BUILDING_WALL},
        {BUILDING_TOWER},
        {BUILDING_GATEHOUSE},
        {BUILDING_FORT_LEGIONARIES, BUILDING_FORT_JAVELIN, BUILDING_FORT_MOUNTED},
        {BUILDING_BARRACKS},
        {BUILDING_MILITARY_ACADEMY},
    },
    { // MENU_INDUSTRY
        {BUILDING_WHEAT_FARM, BUILDING_VEGETABLE_FARM, BUILDING_FRUIT_FARM, BUILDING_PIG_FARM, BUILDING_OLIVE_FARM, BUILDING_VINES_FARM},
        {BUILDING_CLAY_PIT, BUILDING_TIMBER_YARD, BUILDING_MARBLE_QUARRY, BUILDING_IRON_MINE},
        {BUILDING_OIL_WORKSHOP, BUILDING_WINE_WORKSHOP, BUILDING_POTTERY_WORKSHOP, BUILDING_FURNITURE_WORKSHOP, BUILDING_WEAPONS_WORKSHOP},
        {BUILDING_MARKET},
        {BUILDING_GRANARY},
        {BUILDING_WAREHOUSE},
    }
};

static char *submenu_strings[] = {
"Menu: Small Temples",  // 0
"Menu: Large Temples",  // 1
"Menu: Forts", // 2
"Menu: Farms",  // 3
"Menu: Raw Materials",  // 4
"Menu: Workshops"  // 5
};

struct build_menu_t build_menus[BUILD_MENU_BUTTONS_COUNT];

static struct {
    int selected_menu;
    int selected_submenu;
    int num_items_to_draw;
    int y_offset;
    int focus_button_id;
} build_menu_data = { MENU_NONE, MENU_NONE, 0, 0, 0 };

static struct {
    int focus_button_id;
    int focus_toggle_button;
    int selected_item;
    int show_minimap;
    char selected_scenario_filename[FILE_NAME_MAX];
    char selected_scenario_display[FILE_NAME_MAX];

    const struct dir_listing *scenarios;
} cck_selection_data;

static int current_selected_legion_index = 0;

struct numerical_range_widget_t {
    int width_blocks;
    int min;
    int max;
    int step;
    int *value;
};

struct config_widget_t {
    int type;
    int subtype;
    const char *(*get_display_text)(void);
    int enabled;
};

static struct numerical_range_widget_t scale_ranges[] = {
    {30, 50, 500, 5, 0},
    {30, 100, 200, 50, 0}
};

static struct {
    struct config_widget_t *widgets[MAX_WIDGETS];
    int num_widgets;
    int focus_button;
    int bottom_focus_button;
    struct {
        int original_value;
        int new_value;
        int (*change_action)(int key);
    } config_values[CONFIG_MAX_ENTRIES];
    struct {
        char original_value[CONFIG_STRING_VALUE_MAX];
        char new_value[CONFIG_STRING_VALUE_MAX];
        int (*change_action)(int key);
    } config_string_values[CONFIG_STRING_MAX_ENTRIES];
    int active_numerical_range;
} window_config_data;

static struct input_box_t player_name_input = { 125, 50, 20, 2, FONT_NORMAL_WHITE, 0, window_config_data.config_string_values[CONFIG_STRING_PLAYER_NAME].new_value, MAX_PLAYER_NAME_LENGTH };

static char *config_bottom_button_strings[] = {
    "Configure hotkeys", // 0
    "Reset defaults", // 1
    "Cancel", // 2
    "OK", // 3
};

static char *config_widget_strings[] = {
    "Player name:",
    "Display scale:",
    "Cursor scale:",
    "User interface changes",
    "Extra information in the control panel",
    "Play intro videos",
    "Disable map scrolling on window edge",
    "Disable right click to drag the map",
    "Improve visual feedback when clearing land",
    "Highlight legion on cursor hover",
};

static struct {
    int focus_button_id;
    void (*close_callback)(void);
} display_options_data;

static struct {
    int focus_button_id;
    int focus_arrow_button_id;
} donate_to_city_data;

static struct {
    struct empire_object_t *selected_object;
    int selected_button;
    int x_min, x_max, y_min, y_max;
    int x_draw_offset, y_draw_offset;
    int focus_button_id;
    int focus_resource;
} empire_window_data;

static const uint32_t NOT_EXIST_MESSAGE_TIMEOUT = 500;

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
} file_dialog_data;

static struct input_box_t file_name_input = { 144, 80, 20, 2, FONT_NORMAL_WHITE, 0, file_dialog_data.typed_name, FILE_NAME_MAX };

static struct file_type_data_t saved_game_data = { "sav", {0} };
static struct file_type_data_t scenario_data = { "map", {0} };

static char *too_many_files_string = "Too many files. Showing 128.";

static int focus_button_id_gift_to_emperor;

static int focus_id_gods_button;
static int focus_id_festival_size_button;
static int focus_help_button_id;

struct hotkey_widget_t {
    int action;
};

static struct hotkey_widget_t hotkey_widgets[] = {
    {HOTKEY_HEADER},
    {HOTKEY_ARROW_UP},
    {HOTKEY_ARROW_DOWN},
    {HOTKEY_ARROW_LEFT},
    {HOTKEY_ARROW_RIGHT},
    {HOTKEY_HEADER},
    {HOTKEY_TOGGLE_FULLSCREEN},
    {HOTKEY_RESET_WINDOW},
    {HOTKEY_SAVE_SCREENSHOT},
    {HOTKEY_SAVE_CITY_SCREENSHOT},
    {HOTKEY_LOAD_FILE},
    {HOTKEY_SAVE_FILE},
    {HOTKEY_HEADER},
    {HOTKEY_DECREASE_GAME_SPEED},
    {HOTKEY_INCREASE_GAME_SPEED},
    {HOTKEY_TOGGLE_PAUSE},
    {HOTKEY_ROTATE_MAP_LEFT},
    {HOTKEY_ROTATE_MAP_RIGHT},
    {HOTKEY_REPLAY_MAP},
    {HOTKEY_CYCLE_LEGION},
    {HOTKEY_RETURN_LEGIONS_TO_FORT},
    {HOTKEY_SHOW_LAST_ADVISOR},
    {HOTKEY_SHOW_EMPIRE_MAP},
    {HOTKEY_SHOW_MESSAGES},
    {HOTKEY_GO_TO_PROBLEM},
    {HOTKEY_HEADER},
    {HOTKEY_SHOW_OVERLAY_WATER},
    {HOTKEY_SHOW_OVERLAY_FIRE},
    {HOTKEY_SHOW_OVERLAY_DAMAGE},
    {HOTKEY_SHOW_OVERLAY_CRIME},
    {HOTKEY_SHOW_OVERLAY_PROBLEMS},
    {HOTKEY_HEADER},
    {HOTKEY_GO_TO_BOOKMARK_1},
    {HOTKEY_GO_TO_BOOKMARK_2},
    {HOTKEY_GO_TO_BOOKMARK_3},
    {HOTKEY_GO_TO_BOOKMARK_4},
    {HOTKEY_SET_BOOKMARK_1},
    {HOTKEY_SET_BOOKMARK_2},
    {HOTKEY_SET_BOOKMARK_3},
    {HOTKEY_SET_BOOKMARK_4},
    {HOTKEY_HEADER},
    {HOTKEY_EDITOR_TOGGLE_BATTLE_INFO},
    {HOTKEY_HEADER},
    {HOTKEY_CHEAT_MONEY},
    {HOTKEY_CHEAT_INVASION},
    {HOTKEY_CHEAT_VICTORY},
    {HOTKEY_HEADER},
    {HOTKEY_BUILD_CLONE},
    {HOTKEY_CYCLE_BUILDINGS},
    {HOTKEY_CYCLE_BUILDINGS_REVERSE},
    {HOTKEY_UNDO},
    {HOTKEY_BUILD_VACANT_HOUSE},
    {HOTKEY_BUILD_CLEAR_LAND},
    {HOTKEY_BUILD_ROAD},
    {HOTKEY_BUILD_RESERVOIR},
    {HOTKEY_BUILD_AQUEDUCT},
    {HOTKEY_BUILD_FOUNTAIN},
    {HOTKEY_BUILD_WELL},
    {HOTKEY_BUILD_BARBER},
    {HOTKEY_BUILD_BATHHOUSE},
    {HOTKEY_BUILD_DOCTOR},
    {HOTKEY_BUILD_HOSPITAL},
    {HOTKEY_BUILD_SMALL_TEMPLE_CERES},
    {HOTKEY_BUILD_SMALL_TEMPLE_NEPTUNE},
    {HOTKEY_BUILD_SMALL_TEMPLE_MERCURY},
    {HOTKEY_BUILD_SMALL_TEMPLE_MARS},
    {HOTKEY_BUILD_SMALL_TEMPLE_VENUS},
    {HOTKEY_BUILD_LARGE_TEMPLE_CERES},
    {HOTKEY_BUILD_LARGE_TEMPLE_NEPTUNE},
    {HOTKEY_BUILD_LARGE_TEMPLE_MERCURY},
    {HOTKEY_BUILD_LARGE_TEMPLE_MARS},
    {HOTKEY_BUILD_LARGE_TEMPLE_VENUS},
    {HOTKEY_BUILD_ORACLE},
    {HOTKEY_BUILD_SCHOOL},
    {HOTKEY_BUILD_ACADEMY},
    {HOTKEY_BUILD_LIBRARY},
    {HOTKEY_BUILD_MISSION_POST},
    {HOTKEY_BUILD_THEATER},
    {HOTKEY_BUILD_AMPHITHEATER},
    {HOTKEY_BUILD_COLOSSEUM},
    {HOTKEY_BUILD_HIPPODROME},
    {HOTKEY_BUILD_GLADIATOR_SCHOOL},
    {HOTKEY_BUILD_LION_HOUSE},
    {HOTKEY_BUILD_ACTOR_COLONY},
    {HOTKEY_BUILD_CHARIOT_MAKER},
    {HOTKEY_BUILD_FORUM},
    {HOTKEY_BUILD_SENATE},
    {HOTKEY_BUILD_GOVERNORS_HOUSE},
    {HOTKEY_BUILD_GOVERNORS_VILLA},
    {HOTKEY_BUILD_GOVERNORS_PALACE},
    {HOTKEY_BUILD_SMALL_STATUE},
    {HOTKEY_BUILD_MEDIUM_STATUE},
    {HOTKEY_BUILD_LARGE_STATUE},
    {HOTKEY_BUILD_TRIUMPHAL_ARCH},
    {HOTKEY_BUILD_GARDENS},
    {HOTKEY_BUILD_PLAZA},
    {HOTKEY_BUILD_ENGINEERS_POST},
    {HOTKEY_BUILD_LOW_BRIDGE},
    {HOTKEY_BUILD_SHIP_BRIDGE},
    {HOTKEY_BUILD_SHIPYARD},
    {HOTKEY_BUILD_DOCK},
    {HOTKEY_BUILD_WHARF},
    {HOTKEY_BUILD_WALL},
    {HOTKEY_BUILD_TOWER},
    {HOTKEY_BUILD_GATEHOUSE},
    {HOTKEY_BUILD_PREFECTURE},
    {HOTKEY_BUILD_FORT_LEGIONARIES},
    {HOTKEY_BUILD_FORT_JAVELIN},
    {HOTKEY_BUILD_FORT_MOUNTED},
    {HOTKEY_BUILD_MILITARY_ACADEMY},
    {HOTKEY_BUILD_BARRACKS},
    {HOTKEY_BUILD_WHEAT_FARM},
    {HOTKEY_BUILD_VEGETABLE_FARM},
    {HOTKEY_BUILD_FRUIT_FARM},
    {HOTKEY_BUILD_OLIVE_FARM},
    {HOTKEY_BUILD_VINES_FARM},
    {HOTKEY_BUILD_PIG_FARM},
    {HOTKEY_BUILD_CLAY_PIT},
    {HOTKEY_BUILD_MARBLE_QUARRY},
    {HOTKEY_BUILD_IRON_MINE},
    {HOTKEY_BUILD_TIMBER_YARD},
    {HOTKEY_BUILD_WINE_WORKSHOP},
    {HOTKEY_BUILD_OIL_WORKSHOP},
    {HOTKEY_BUILD_WEAPONS_WORKSHOP},
    {HOTKEY_BUILD_FURNITURE_WORKSHOP},
    {HOTKEY_BUILD_POTTERY_WORKSHOP},
    {HOTKEY_BUILD_MARKET},
    {HOTKEY_BUILD_GRANARY},
    {HOTKEY_BUILD_WAREHOUSE},
};

static const char *key_display_names[KEY_TYPE_MAX_ITEMS] = {
    "", "A", "B", "C", "D", "E", "F", "G", "H", "I",
    "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S",
    "T", "U", "V", "W", "X", "Y", "Z", "1", "2", "3",
    "4", "5", "6", "7", "8", "9", "0", "-", "=", "Enter",
    "Esc", "Backspace", "Tab", "Space", "Left bracket", "Right bracket", "Backslash", ";", "'", "Backtick",
    ",", ".", "/", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
    "F8", "F9", "F10", "F11", "F12", "Insert", "Delete", "Home", "End", "PageUp",
    "PageDown", "Right", "Left", "Down", "Up",
    "Keypad 1", "Keypad 2", "Keypad 3", "Keypad 4", "Keypad 5",
    "Keypad 6", "Keypad 7", "Keypad 8", "Keypad 9", "Keypad 0",
    "Keypad .", "Keypad +", "Keypad -", "Keypad *", "Keypad /", "NonUS"
};

static struct {
    int focus_button;
    int bottom_focus_button;
    struct hotkey_mapping_t mappings[HOTKEY_MAX_ITEMS][2];
} hotkey_config_window_data;

static char *hotkey_strings[] = {
    "Brutus hotkey configuration", // 0
    "Hotkey", // 1
    "Alternative", // 2
    "Reset defaults", // 3
    "Cancel", // 4
    "OK", // 5
};

static char *hotkey_widget_strings[] = {
    "Arrow keys", // 0
    "Up", // 1
    "Down", // 2
    "Left", // 3
    "Right", // 4
    "Global hotkeys", // 5
    "Toggle fullscreen", // 6
    "Reset window", // 7
    "Save screenshot", // 8
    "Save full city screenshot", // 9
    "Load file", // 10
    "Save file", // 11
    "City hotkeys", // 12
    "Decrease game speed", // 13
    "Increase game speed", // 14
    "Toggle pause", // 15
    "Rotate map left", // 16
    "Rotate map right", // 17
    "Replay map", // 18
    "Cycle through legions", // 19
    "Return legions to fort", // 20
    "Show last advisor", // 21
    "Show empire map", // 22
    "Show messages", // 23
    "Go to problem", // 24
    "Overlays", // 25
    "Show water overlay", // 26
    "Show fire overlay", // 27
    "Damage overlay", // 28
    "Crime overlay", // 29
    "Problems overlay", // 30
    "City map bookmarks", // 31
    "Go to bookmark 1", // 32
    "Go to bookmark 2", // 33
    "Go to bookmark 3", // 34
    "Go to bookmark 4", // 35
    "Set bookmark 1", // 36
    "Set bookmark 2", // 37
    "Set bookmark 3", // 38
    "Set bookmark 4", // 39
    "Editor", // 40
    "Toggle battle info", // 41
    "Cheats", // 42
    "Cheat: money", // 43
    "Cheat: invasion", // 44
    "Cheat: victory", // 45
    "Construction hotkeys", // 46
    "Clone building under cursor", // 47
    "Cycle through buildings", // 48
    "Cycle back through buildings", // 49
    "Undo last building", // 50
    "Housing", // 51
};

static struct {
    int action;
    int index;
    int key;
    int modifiers;
    void (*callback)(int, int, int, int);
    int focus_button;
} hotkey_editor_window_data;

static char *hotkey_editor_bottom_button_strings[] = {
    "Press new hotkey", // 0
    "Cancel", // 1
    "OK", // 2
};

static const char SOUND_FILE_LOSE[] = "wavs/lose_game.wav";
static const char SOUND_FILE_WIN[] = "wavs/actors_great1.wav";

static struct {
    int type;
    void (*callback)(void);
    uint32_t start_time;
} intermezzo_data;

static int current_video;
static int started;
static const char *intro_videos[NUM_INTRO_VIDEOS] = { "smk/logo.smk", "smk/intro.smk", "smk/credits.smk" };

static struct {
    int category;
    int max_items;
    int focus_id_button_set_priority;
    int focus_id_button_remove_priority;
} labor_priority_data;

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

static int focus_button_id_main_menu;

static struct {
    struct {
        int text_id;
        int scroll_position;
    } history[200];
    int num_history;
    int text_id;
    void (*background_callback)(void);
    int show_video;
    int x;
    int y;
    int x_text;
    int y_text;
    int text_height_blocks;
    int text_width_blocks;
    int focus_button_id;
} message_dialog_data;

static struct {
    int year;
    int month;
    int param1;
    int param2;
    int message_advisor;
    int use_popup;
} player_message;

static struct {
    int width_blocks;
    int height_blocks;
    int x_text;
    int y_text;
    int text_width_blocks;
    int text_height_blocks;
    int focus_button_id;
} message_list_data;

static struct {
    int x;
    int y;
    int max_digits;
    int max_value;
    void (*callback)(int);

    int num_digits;
    int value;
    int focus_button_id;
} numeric_input_data;

static const int MENU_ID_TO_OVERLAY[MAX_BUTTONS_OVERLAY_MENU] = { OVERLAY_NONE, OVERLAY_WATER, 1, 3, 5, 6, 7, OVERLAY_RELIGION };
static const int MENU_ID_TO_SUBMENU_ID[MAX_BUTTONS_OVERLAY_MENU] = { 0, 0, 1, 2, 3, 4, 5, 0 };

static const int SUBMENU_ID_TO_OVERLAY[6][MAX_BUTTONS_OVERLAY_MENU] = {
    {0},
    {OVERLAY_FIRE, OVERLAY_DAMAGE, OVERLAY_CRIME, OVERLAY_NATIVE, OVERLAY_PROBLEMS, 0},
    {OVERLAY_ENTERTAINMENT, OVERLAY_THEATER, OVERLAY_AMPHITHEATER, OVERLAY_COLOSSEUM, OVERLAY_HIPPODROME, 0},
    {OVERLAY_EDUCATION, OVERLAY_SCHOOL, OVERLAY_LIBRARY, OVERLAY_ACADEMY, 0},
    {OVERLAY_BARBER, OVERLAY_BATHHOUSE, OVERLAY_CLINIC, OVERLAY_HOSPITAL, 0},
    {OVERLAY_TAX_INCOME, OVERLAY_FOOD_STOCKS, OVERLAY_DESIRABILITY, 0},
};

static struct {
    int selected_menu;
    int selected_submenu;
    int num_submenu_items;
    uint32_t submenu_focus_time;
    int menu_focus_button_id;
    int submenu_focus_button_id;
    int keep_submenu_open;
} overlay_menu_data;

static struct {
    const char *title;
    const char *message;
    const char *extra;
} plain_message_dialog_data;

static struct {
    int type;
    int custom_text_group;
    int custom_text_id;
    int ok_clicked;
    void (*close_func)(void);
    int has_buttons;
} popup_dialog_data;

static struct {
    int resource;
    int focus_button_id;
} resource_settings_data;

static struct {
    int x;
    int y;
    int mode;
    int group;
    char **items;
    int num_items;
    void (*callback)(int);
    int focus_button_id;
} select_list_data;

static struct {
    int focus_button_id;
    int from_editor;
    struct set_sound_t original_effects;
    struct set_sound_t original_music;
    struct set_sound_t original_speech;
    struct set_sound_t original_city;
} sound_options_data;

static int focus_button_id_set_salary;

static struct {
    int focus_button_id;
    int from_editor;
    int original_game_speed;
    int original_scroll_speed;
} speed_options_data;

static int focus_button_id_victory_dialog = 0;

static struct {
    int width;
    int height;
    void (*callback)(void);
} victory_video_data;

static const int INDEX_OPTIONS = 1;
static const int INDEX_HELP = 2;

static struct {
    int offset_funds;
    int offset_population;
    int offset_date;

    int open_sub_menu;
    int focus_menu_id;
    int focus_sub_menu_id;
} top_menu_data;

static struct {
    int population;
    int treasury;
    int month;
} drawn;

static struct {
    int absolute_x;
    int absolute_y;
    int width_tiles;
    int height_tiles;
    int x_offset;
    int y_offset;
} scenario_minimap_data;

struct tile_color_t {
    color_t left;
    color_t right;
};

struct tile_color_set_t {
    struct tile_color_t  water[4];
    struct tile_color_t  tree[4];
    struct tile_color_t  rock[4];
    struct tile_color_t  meadow[4];
    struct tile_color_t  grass[8];
    struct tile_color_t  road;
};

// Since the minimap tiles are only 25 color sets per climate, we just hardcode them.
// This "hack" is necessary to avoid reloading the climate graphics when selecting
// a scenario with another climate in the CCK selection screen, which is expensive.
static const struct tile_color_set_t MINIMAP_COLOR_SETS[3] = {
    // central
    {
        .water = {{0x394a7b, 0x31427b}, {0x394a7b, 0x314273}, {0x313973, 0x314273}, {0x31427b, 0x394a7b}},
        .tree = {{0x6b8431, 0x102108}, {0x103908, 0x737b29}, {0x103108, 0x526b21}, {0x737b31, 0x084a10}},
        .rock = {{0x948484, 0x635a4a}, {0xa59c94, 0xb5ada5}, {0xb5ada5, 0x8c8484}, {0x635a4a, 0xa59c94}},
        .meadow = {{0xd6bd63, 0x9c8c39}, {0x948c39, 0xd6bd63}, {0xd6bd63, 0x9c9439}, {0x848431, 0xada54a}},
        .grass = {
            {0x6b8c31, 0x6b7b29}, {0x738431, 0x6b7b29}, {0x6b7329, 0x7b8c39}, {0x527b29, 0x6b7321},
            {0x6b8431, 0x737b31}, {0x6b7b31, 0x737b29}, {0x636b18, 0x526b21}, {0x737b31, 0x737b29}
        },
        .road = {0x736b63, 0x4a3121}
    },
    // northern
    {
        .water = {{0x394a7b, 0x31427b}, {0x394a7b, 0x314273}, {0x313973, 0x314273}, {0x31427b, 0x394a7b}},
        .tree = {{0x527b31, 0x082108}, {0x083908, 0x5a7329}, {0x082908, 0x316b21}, {0x527b29, 0x084a21}},
        .rock = {{0x8c8484, 0x5a5252}, {0x9c9c94, 0xa5a5a5}, {0xa5a5a5, 0x848484}, {0x5a5252, 0x9c9c94}},
        .meadow = {{0x427318, 0x8c9442}, {0xb5ad4a, 0x738c39}, {0x8c8c39, 0x6b7b29}, {0x527331, 0x5a8442}},
        .grass = {
            {0x4a8431, 0x4a7329}, {0x527b29, 0x4a7329}, {0x526b29, 0x5a8439}, {0x397321, 0x4a6b21},
            {0x527b31, 0x5a7331}, {0x4a7329, 0x5a7329}, {0x4a6b18, 0x316b21}, {0x527b29, 0x527329}
        },
        .road = {0x736b63, 0x4a3121}
    },
    // desert
    {
        .water = {{0x4a84c6, 0x4a7bc6}, {0x4a84c6, 0x4a7bc6}, {0x4a84c6, 0x5284c6}, {0x4a7bbd, 0x4a7bc6}},
        .tree = {{0xa59c7b, 0x6b7b18}, {0x214210, 0xada573}, {0x526b21, 0xcec6a5}, {0xa59c7b, 0x316321}},
        .rock = {{0xa59494, 0x736352}, {0xa59c94, 0xb5ada5}, {0xb5ada5, 0x8c847b}, {0x736352, 0xbdada5}},
        .meadow = {{0x739c31, 0x9cbd52}, {0x7bb529, 0x63ad21}, {0x9cbd52, 0x8c944a}, {0x7ba539, 0x739c31}},
        .grass = {
            {0xbdbd9c, 0xb5b594}, {0xc6bda5, 0xbdbda5}, {0xbdbd9c, 0xc6c6ad}, {0xd6cead, 0xc6bd9c},
            {0xa59c7b, 0xbdb594}, {0xcecead, 0xb5ad94}, {0xc6c6a5, 0xdedebd}, {0xcecead, 0xd6d6b5}
        },
        .road = {0x6b5a52, 0x4a4239}
    }
};

static const color_t ENEMY_COLOR_BY_CLIMATE[] = {
    COLOR_MINIMAP_ENEMY_CENTRAL,
    COLOR_MINIMAP_ENEMY_NORTHERN,
    COLOR_MINIMAP_ENEMY_DESERT
};

static struct {
    int absolute_x;
    int absolute_y;
    int width_tiles;
    int height_tiles;
    int x_offset;
    int y_offset;
    int width;
    int height;
    color_t enemy_color;
    color_t *cache;
    struct {
        int x;
        int y;
        int grid_offset;
    } mouse;
    int refresh_requested;
    int camera_x;
    int camera_y;
} minimap_data;

static int draw_cursor_input_box;

static struct {
    struct map_tile_t current_tile;
    struct map_tile_t selected_tile;
    int new_start_grid_offset;
    int capture_input;
} widget_city_data;

static struct {
    uint32_t last_water_animation_time;
    int advance_water_animation;
    int image_id_water_first;
    int image_id_water_last;
    int selected_figure_id;
    int highlighted_formation;
    struct pixel_coordinate_t *selected_figure_coord;
} draw_context;

static struct city_overlay_t *overlay = 0;

static const int ADJACENT_OFFSETS[2][4][7] = {
    {
        {OFFSET(-1, 0), OFFSET(-1, -1), OFFSET(-1, -2), OFFSET(0, -2), OFFSET(1, -2)},
        {OFFSET(0, -1), OFFSET(1, -1), OFFSET(2, -1), OFFSET(2, 0), OFFSET(2, 1)},
        {OFFSET(1, 0), OFFSET(1, 1), OFFSET(1, 2), OFFSET(0, 2), OFFSET(-1, 2)},
        {OFFSET(0, 1), OFFSET(-1, 1), OFFSET(-2, 1), OFFSET(-2, 0), OFFSET(-2, -1)}
    },
    {
        {OFFSET(-1, 0), OFFSET(-1, -1), OFFSET(-1, -2), OFFSET(-1, -3), OFFSET(0, -3),  OFFSET(1, -3), OFFSET(2, -3)},
        {OFFSET(0, -1), OFFSET(1, -1), OFFSET(2, -1), OFFSET(3, -1), OFFSET(3, 0),  OFFSET(3, 1), OFFSET(3, 2)},
        {OFFSET(1, 0), OFFSET(1, 1), OFFSET(1, 2), OFFSET(1, 3), OFFSET(0, 3),  OFFSET(-1, 3), OFFSET(-2, 3)},
        {OFFSET(0, 1), OFFSET(-1, 1), OFFSET(-2, 1), OFFSET(-3, 1), OFFSET(-3, 0),  OFFSET(-3, -1), OFFSET(-3, -2)}
    }
};

static const int X_VIEW_OFFSETS[MAX_TILES] = {
    0,
    -30, 30, 0,
    -60, 60, -30, 30, 0,
    -90, 90, -60, 60, -30, 30, 0,
    -120, 120, -90, 90, -60, 60, -30, 30, 0
};

static const int Y_VIEW_OFFSETS[MAX_TILES] = {
    0,
    15, 15, 30,
    30, 30, 45, 45, 60,
    45, 45, 60, 60, 75, 75, 90,
    60, 60, 75, 75, 90, 90, 105, 105, 120
};

static const int TILE_GRID_OFFSETS[4][MAX_TILES] = {
    {OFFSET(0,0),
    OFFSET(0,1), OFFSET(1,0), OFFSET(1,1),
    OFFSET(0,2), OFFSET(2,0), OFFSET(1,2), OFFSET(2,1), OFFSET(2,2),
    OFFSET(0,3), OFFSET(3,0), OFFSET(1,3), OFFSET(3,1), OFFSET(2,3), OFFSET(3,2), OFFSET(3,3),
    OFFSET(0,4), OFFSET(4,0), OFFSET(1,4), OFFSET(4,1), OFFSET(2,4), OFFSET(4,2),
        OFFSET(3,4), OFFSET(4,3), OFFSET(4,4)},
    {OFFSET(0,0),
    OFFSET(-1,0), OFFSET(0,1), OFFSET(-1,1),
    OFFSET(-2,0), OFFSET(0,2), OFFSET(-2,1), OFFSET(-1,2), OFFSET(-2,2),
    OFFSET(-3,0), OFFSET(0,3), OFFSET(-3,1), OFFSET(-1,3), OFFSET(-3,2), OFFSET(-2,3), OFFSET(-3,3),
    OFFSET(-4,0), OFFSET(0,4), OFFSET(-4,1), OFFSET(-1,4), OFFSET(-4,2), OFFSET(-2,4),
        OFFSET(-4,3), OFFSET(-3,4), OFFSET(-4,4)},
    {OFFSET(0,0),
    OFFSET(0,-1), OFFSET(-1,0), OFFSET(-1,-1),
    OFFSET(0,-2), OFFSET(-2,0), OFFSET(-1,-2), OFFSET(-2,-1), OFFSET(-2,-2),
    OFFSET(0,-3), OFFSET(-3,0), OFFSET(-1,-3), OFFSET(-3,-1), OFFSET(-2,-3), OFFSET(-3,-2), OFFSET(-3,-3),
    OFFSET(0,-4), OFFSET(-4,0), OFFSET(-1,-4), OFFSET(-4,-1), OFFSET(-2,-4), OFFSET(-4,-2),
        OFFSET(-3,-4), OFFSET(-4,-3), OFFSET(-4,-4)},
    {OFFSET(0,0),
    OFFSET(1,0), OFFSET(0,-1), OFFSET(1,-1),
    OFFSET(2,0), OFFSET(0,-2), OFFSET(2,-1), OFFSET(1,-2), OFFSET(2,-2),
    OFFSET(3,0), OFFSET(0,-3), OFFSET(3,-1), OFFSET(1,-3), OFFSET(3,-2), OFFSET(2,-3), OFFSET(3,-3),
    OFFSET(4,0), OFFSET(0,-4), OFFSET(4,-1), OFFSET(1,-4), OFFSET(4,-2), OFFSET(2,-4),
        OFFSET(4,-3), OFFSET(3,-4), OFFSET(4,-4)},
};

static const int FORT_GROUND_GRID_OFFSETS[4] = { OFFSET(3,-1), OFFSET(4,-1), OFFSET(4,0), OFFSET(3,0) };
static const int FORT_GROUND_X_VIEW_OFFSETS[4] = { 120, 90, -120, -90 };
static const int FORT_GROUND_Y_VIEW_OFFSETS[4] = { 30, -75, -60, 45 };

static const int HIPPODROME_X_VIEW_OFFSETS[4] = { 150, 150, -150, -150 };
static const int HIPPODROME_Y_VIEW_OFFSETS[4] = { 75, -75, -75, 75 };

static struct {
    int position;
    struct speed_type_t slide_speed;
    int direction;
    back_sidebar_draw_function back_sidebar_draw;
    front_sidebar_draw_function front_sidebar_draw;
    slide_finished_function finished_callback;
} widget_slide_data;

struct objective_t {
    int value;
    int target;
};

static struct {
    int x_offset;
    int y_offset;
    int width;
    int height;
    int is_collapsed;
    int info_to_display;
    int game_speed;
    int unemployment_percentage;
    int unemployment_amount;
    struct objective_t culture;
    struct objective_t prosperity;
    struct objective_t peace;
    struct objective_t favor;
    struct objective_t population;
} extra_widget_data;

static void window_config_show(void);

static void draw_title_advisors(int y, int text_id)
{
    image_draw(image_group(GROUP_BULLET), 32, y + 1);
    lang_text_draw(61, text_id, 52, y, FONT_NORMAL_WHITE);
}

static int draw_background_chief_advisor(void)
{
    int width;
    outer_panel_draw(0, 0, 40, CHIEF_ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 11, 10, 10);
    lang_text_draw(61, 0, 60, 12, FONT_LARGE_BLACK);
    inner_panel_draw(24, 60, 37, 16);
    // workers
    draw_title_advisors(66, 1);
    if (city_data.labor.unemployment_percentage > 0) {
        width = lang_text_draw(61, 12, X_OFFSET, 66, FONT_NORMAL_RED);
        width += text_draw_percentage(city_data.labor.unemployment_percentage, X_OFFSET + width, 66, FONT_NORMAL_RED);
        text_draw_number(city_data.labor.workers_unemployed - city_data.labor.workers_needed, '(', ")",
            X_OFFSET + width, 66, FONT_NORMAL_RED);
    } else if (city_data.labor.workers_needed > 0) {
        width = lang_text_draw(61, 13, X_OFFSET, 66, FONT_NORMAL_RED);
        lang_text_draw_amount(8, 12, city_data.labor.workers_needed, X_OFFSET + width, 66, FONT_NORMAL_RED);
    } else {
        lang_text_draw(61, 14, X_OFFSET, 66, FONT_NORMAL_GREEN);
    }
    // finance
    draw_title_advisors(86, 2);
    if (city_data.finance.treasury > city_data.finance.last_year.balance) {
        width = lang_text_draw(61, 15, X_OFFSET, 86, FONT_NORMAL_GREEN);
        text_draw_money(city_data.finance.treasury - city_data.finance.last_year.balance, X_OFFSET + width, 86, FONT_NORMAL_GREEN);
    } else if (city_data.finance.treasury < city_data.finance.last_year.balance) {
        width = lang_text_draw(61, 16, X_OFFSET, 86, FONT_NORMAL_RED);
        text_draw_money(city_data.finance.last_year.balance - city_data.finance.treasury, X_OFFSET + width, 86, FONT_NORMAL_RED);
    } else {
        lang_text_draw(61, 17, X_OFFSET, 86, FONT_NORMAL_GREEN);
    }
    // migration
    draw_title_advisors(106, 3);
    if (city_figures_total_invading_enemies() > 3) {
        lang_text_draw(61, 79, X_OFFSET, 106, FONT_NORMAL_GREEN);
    } else if (city_data.migration.newcomers >= 5) {
        lang_text_draw(61, 25, X_OFFSET, 106, FONT_NORMAL_GREEN);
    } else if (city_migration_no_room_for_immigrants()) {
        lang_text_draw(61, 18, X_OFFSET, 106, FONT_NORMAL_RED);
    } else if (city_data.migration.percentage >= 80) {
        lang_text_draw(61, 25, X_OFFSET, 106, FONT_NORMAL_GREEN);
    } else {
        int text_id;
        switch (city_data.migration.no_immigration_cause) {
            case NO_IMMIGRATION_LOW_WAGES: text_id = 19; break;
            case NO_IMMIGRATION_NO_JOBS: text_id = 20; break;
            case NO_IMMIGRATION_NO_FOOD: text_id = 21; break;
            case NO_IMMIGRATION_HIGH_TAXES: text_id = 22; break;
            case NO_IMMIGRATION_MANY_TENTS: text_id = 70; break;
            case NO_IMMIGRATION_LOW_MOOD: text_id = 71; break;
            default: text_id = 0; break;
        }
        if (text_id) {
            lang_text_draw(61, text_id, X_OFFSET, 106, FONT_NORMAL_GREEN);
        }
    }
    // food stocks
    draw_title_advisors(126, 4);
    if (scenario.rome_supplies_wheat) {
        lang_text_draw(61, 26, X_OFFSET, 126, FONT_NORMAL_GREEN);
    } else if (city_data.resource.food_supply_months > 0) {
        width = lang_text_draw(61, 28, X_OFFSET, 126, FONT_NORMAL_GREEN);
        lang_text_draw_amount(8, 4, city_data.resource.food_supply_months, X_OFFSET + width, 126, FONT_NORMAL_GREEN);
    } else {
        lang_text_draw(61, 27, X_OFFSET, 126, FONT_NORMAL_RED);
    }
    // food consumption
    draw_title_advisors(146, 62);
    if (scenario.rome_supplies_wheat) {
        lang_text_draw(61, 26, X_OFFSET, 146, FONT_NORMAL_GREEN);
    } else {
        int pct = calc_percentage(city_data.resource.food_produced_last_month, city_data.resource.food_consumed_last_month);
        if (pct > 150) {
            lang_text_draw(61, 63, X_OFFSET, 146, FONT_NORMAL_GREEN);
        } else if (pct > 105) {
            lang_text_draw(61, 64, X_OFFSET, 146, FONT_NORMAL_GREEN);
        } else if (pct > 95) {
            lang_text_draw(61, 65, X_OFFSET, 146, FONT_NORMAL_GREEN);
        } else if (pct > 75) {
            lang_text_draw(61, 66, X_OFFSET, 146, FONT_NORMAL_RED);
        } else if (pct > 30) {
            lang_text_draw(61, 67, X_OFFSET, 146, FONT_NORMAL_RED);
        } else if (pct > 0) {
            lang_text_draw(61, 68, X_OFFSET, 146, FONT_NORMAL_RED);
        } else {
            lang_text_draw(61, 69, X_OFFSET, 146, FONT_NORMAL_RED);
        }
    }
    // military
    draw_title_advisors(166, 5);
    if (city_data.figure.imperial_soldiers) {
        lang_text_draw(61, 76, X_OFFSET, 166, FONT_NORMAL_RED);
    } else if (city_data.figure.enemies) {
        lang_text_draw(61, 75, X_OFFSET, 166, FONT_NORMAL_RED);
    } else if (scenario.invasion_upcoming) {
        lang_text_draw(61, 74, X_OFFSET, 166, FONT_NORMAL_RED);
    } else if (city_military_distant_battle_roman_army_is_traveling()) {
        lang_text_draw(61, 78, X_OFFSET, 166, FONT_NORMAL_GREEN);
    } else if (city_data.distant_battle.months_until_battle) {
        lang_text_draw(61, 77, X_OFFSET, 166, FONT_NORMAL_RED);
    } else if (city_data.figure.soldiers) {
        lang_text_draw(61, 73, X_OFFSET, 166, FONT_NORMAL_GREEN);
    } else {
        lang_text_draw(61, 72, X_OFFSET, 166, FONT_NORMAL_GREEN);
    }
    // crime
    draw_title_advisors(186, 6);
    if (city_data.figure.rioters) {
        lang_text_draw(61, 33, X_OFFSET, 186, FONT_NORMAL_RED);
    } else if (city_data.sentiment.criminals > 10) {
        lang_text_draw(61, 32, X_OFFSET, 186, FONT_NORMAL_RED);
    } else if (city_data.sentiment.criminals) {
        lang_text_draw(61, 31, X_OFFSET, 186, FONT_NORMAL_RED);
    } else if (city_data.sentiment.protesters) {
        lang_text_draw(61, 30, X_OFFSET, 186, FONT_NORMAL_RED);
    } else {
        lang_text_draw(61, 29, X_OFFSET, 186, FONT_NORMAL_GREEN);
    }
    // health
    draw_title_advisors(206, 7);
    if (city_data.health.value >= 40) {
        lang_text_draw(56, city_data.health.value / 10 + 27, X_OFFSET, 206, FONT_NORMAL_GREEN);
    } else {
        lang_text_draw(56, city_data.health.value / 10 + 27, X_OFFSET, 206, FONT_NORMAL_RED);
    }
    // education
    struct house_demands_t *demands = &city_data.houses;
    draw_title_advisors(226, 8);
    if (demands->education == 1) {
        lang_text_draw(61, 39, X_OFFSET, 226, FONT_NORMAL_RED);
    } else if (demands->education == 2) {
        lang_text_draw(61, 40, X_OFFSET, 226, FONT_NORMAL_RED);
    } else if (demands->education == 3) {
        lang_text_draw(61, 41, X_OFFSET, 226, FONT_NORMAL_RED);
    } else {
        lang_text_draw(61, 42, X_OFFSET, 226, FONT_NORMAL_GREEN);
    }
    // religion
    draw_title_advisors(246, 9);
    if (demands->religion == 1) {
        lang_text_draw(61, 46, X_OFFSET, 246, FONT_NORMAL_RED);
    } else if (demands->religion == 2) {
        lang_text_draw(61, 47, X_OFFSET, 246, FONT_NORMAL_RED);
    } else if (demands->religion == 3) {
        lang_text_draw(61, 48, X_OFFSET, 246, FONT_NORMAL_RED);
    } else {
        lang_text_draw(61, 49, X_OFFSET, 246, FONT_NORMAL_GREEN);
    }
    // entertainment
    draw_title_advisors(266, 10);
    if (demands->entertainment == 1) {
        lang_text_draw(61, 43, X_OFFSET, 266, FONT_NORMAL_RED);
    } else if (demands->entertainment == 2) {
        lang_text_draw(61, 44, X_OFFSET, 266, FONT_NORMAL_RED);
    } else {
        lang_text_draw(61, 45, X_OFFSET, 266, FONT_NORMAL_GREEN);
    }
    // sentiment
    draw_title_advisors(286, 11);
    int sentiment = city_data.sentiment.value;
    if (sentiment <= 0) {
        lang_text_draw(61, 50, X_OFFSET, 286, FONT_NORMAL_RED);
    } else if (sentiment >= 100) {
        lang_text_draw(61, 61, X_OFFSET, 286, FONT_NORMAL_GREEN);
    } else {
        lang_text_draw(61, sentiment / 10 + 51, X_OFFSET, 286, FONT_NORMAL_GREEN);
    }

    return CHIEF_ADVISOR_HEIGHT;
}

static struct advisor_window_type_t *window_advisor_chief(void)
{
    static struct advisor_window_type_t window = {
        draw_background_chief_advisor,
        0,
        0,
    };
    return &window;
}

static int draw_background_education_advisor(void)
{
    outer_panel_draw(0, 0, 40, EDUCATION_ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 7, 10, 10);
    lang_text_draw(57, 0, 60, 12, FONT_LARGE_BLACK);
    // x population, y school age, z academy age
    int width = text_draw_number(city_data.population.population, '@', " ", 60, 50, FONT_NORMAL_BLACK);
    width += lang_text_draw(57, 1, 60 + width, 50, FONT_NORMAL_BLACK);
    width += text_draw_number(city_data.population.school_age, '@', " ", 60 + width, 50, FONT_NORMAL_BLACK);
    width += lang_text_draw(57, 2, 60 + width, 50, FONT_NORMAL_BLACK);
    width += text_draw_number(city_data.population.academy_age, '@', " ", 60 + width, 50, FONT_NORMAL_BLACK);
    lang_text_draw(57, 3, 60 + width, 50, FONT_NORMAL_BLACK);
    // table headers
    lang_text_draw(57, 4, 180, 86, FONT_SMALL_PLAIN);
    lang_text_draw(57, 5, 290, 86, FONT_SMALL_PLAIN);
    lang_text_draw(57, 6, 478, 86, FONT_SMALL_PLAIN);
    inner_panel_draw(32, 100, 36, 4);
    // schools
    lang_text_draw_amount(8, 18, building_count_total(BUILDING_SCHOOL), 40, 105, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_active(BUILDING_SCHOOL), 150, 105, 100, FONT_NORMAL_WHITE);
    width = text_draw_number(75 * building_count_active(BUILDING_SCHOOL), '@', " ", 280, 105, FONT_NORMAL_WHITE);
    lang_text_draw(57, 7, 280 + width, 105, FONT_NORMAL_WHITE);
    int pct_school = city_culture_coverage_school();
    if (pct_school == 0) {
        lang_text_draw_centered(57, 10, 420, 105, 200, FONT_NORMAL_WHITE);
    } else if (pct_school < 100) {
        lang_text_draw_centered(57, pct_school / 10 + 11, 420, 105, 200, FONT_NORMAL_WHITE);
    } else {
        lang_text_draw_centered(57, 21, 420, 105, 200, FONT_NORMAL_WHITE);
    }
    // academies
    lang_text_draw_amount(8, 20, building_count_total(BUILDING_ACADEMY), 40, 125, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_active(BUILDING_ACADEMY), 150, 125, 100, FONT_NORMAL_WHITE);
    width = text_draw_number(100 * building_count_active(BUILDING_ACADEMY), '@', " ", 280, 125, FONT_NORMAL_WHITE);
    lang_text_draw(57, 8, 280 + width, 125, FONT_NORMAL_WHITE);
    int pct_academy = city_culture_coverage_academy();
    if (pct_academy == 0) {
        lang_text_draw_centered(57, 10, 420, 125, 200, FONT_NORMAL_WHITE);
    } else if (pct_academy < 100) {
        lang_text_draw_centered(57, pct_academy / 10 + 11, 420, 125, 200, FONT_NORMAL_WHITE);
    } else {
        lang_text_draw_centered(57, 21, 420, 125, 200, FONT_NORMAL_WHITE);
    }
    // libraries
    lang_text_draw_amount(8, 22, building_count_total(BUILDING_LIBRARY), 40, 145, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_active(BUILDING_LIBRARY), 150, 145, 100, FONT_NORMAL_WHITE);
    width = text_draw_number(800 * building_count_active(BUILDING_LIBRARY), '@', " ", 280, 145, FONT_NORMAL_WHITE);
    lang_text_draw(57, 9, 280 + width, 145, FONT_NORMAL_WHITE);
    int pct_library = city_culture_coverage_library();
    if (pct_library == 0) {
        lang_text_draw_centered(57, 10, 420, 145, 200, FONT_NORMAL_WHITE);
    } else if (pct_library < 100) {
        lang_text_draw_centered(57, pct_library / 10 + 11, 420, 145, 200, FONT_NORMAL_WHITE);
    } else {
        lang_text_draw_centered(57, 21, 420, 145, 200, FONT_NORMAL_WHITE);
    }
    int education_advice;
    const struct house_demands_t *demands = &city_data.houses;
    if (demands->education == 1) {
        education_advice = demands->requiring.school ? 1 : 0;
    } else if (demands->education == 2) {
        education_advice = demands->requiring.library ? 3 : 2;
    } else if (demands->education == 3) {
        education_advice = 4;
    } else {
        int coverage_school = city_culture_coverage_school();
        int coverage_academy = city_culture_coverage_academy();
        int coverage_library = city_culture_coverage_library();
        if (!demands->requiring.school) {
            education_advice = 5; // no demands yet
        } else if (!demands->requiring.library) {
            if (coverage_school >= 100 && coverage_academy >= 100) {
                education_advice = 6; // education is perfect
            } else if (coverage_school <= coverage_academy) {
                education_advice = 7; // build more schools
            } else {
                education_advice = 8; // build more academies
            }
        } else {
            // all education needed
            if (coverage_school >= 100 && coverage_academy >= 100 && coverage_library >= 100) {
                education_advice = 6;
            } else if (coverage_school <= coverage_academy && coverage_school <= coverage_library) {
                education_advice = 7; // build more schools
            } else if (coverage_academy <= coverage_school && coverage_academy <= coverage_library) {
                education_advice = 8; // build more academies
            } else if (coverage_library <= coverage_school && coverage_library <= coverage_academy) {
                education_advice = 9; // build more libraries
            } else {
                education_advice = 6; // unlikely event that all coverages are equal
            }
        }
    }
    lang_text_draw_multiline(57, 22 + education_advice, 60, 180, 512, FONT_NORMAL_BLACK);
    return EDUCATION_ADVISOR_HEIGHT;
}

static struct advisor_window_type_t *window_advisor_education(void)
{
    static struct advisor_window_type_t window = {
        draw_background_education_advisor,
        0,
        0
    };
    return &window;
}

static void window_advisors_draw_dialog_background(void)
{
    image_draw_fullscreen_background(image_group(GROUP_ADVISOR_BACKGROUND));
    graphics_in_dialog();
    image_draw(image_group(GROUP_PANEL_WINDOWS) + 13, 0, 432);

    for (int i = 0; i < 13; i++) {
        int selected_offset = 0;
        if (current_advisor && i == current_advisor - 1) {
            selected_offset = 13;
        }
        image_draw(image_group(GROUP_ADVISOR_ICONS) + i + selected_offset, 48 * i + 12, 441);
    }
    graphics_reset_dialog();
}

static void button_help_resource_settings_festival(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_message_dialog_show(MESSAGE_DIALOG_ADVISOR_ENTERTAINMENT, 0);
}

static struct image_button_t button_help_hold_festival[] = {
    {68, 257, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help_resource_settings_festival, button_none, 0, 0, 1, 0, 0, 0},
};

static void draw_foreground_hold_festival(void)
{
    graphics_in_dialog();
    outer_panel_draw(52, 12, 34, 18);
    // Hold festival to [selected god]
    lang_text_draw_centered(58, 25 + city_data.festival.god, 52, 28, 544, FONT_LARGE_BLACK);
    for (int god = 0; god < MAX_GODS; god++) {
        if (god == city_data.festival.god) {
            image_draw(image_group(GROUP_PANEL_WINDOWS) + god + 21, 91 * god + 101, 76);
        } else {
            image_draw(image_group(GROUP_PANEL_WINDOWS) + god + 16, 91 * god + 101, 76);
        }
    }
    // Small festival
    button_border_draw(100, 183, 448, 26, focus_id_festival_size_button == 1);
    int width = lang_text_draw(58, 31, 116, 191, FONT_NORMAL_BLACK);
    lang_text_draw_amount(8, 0, city_data.population.population / 20 + 10, 116 + width, 191, FONT_NORMAL_BLACK);
    // Large festival
    button_border_draw(100, 213, 448, 26, focus_id_festival_size_button == 2);
    width = lang_text_draw(58, 32, 116, 221, FONT_NORMAL_BLACK);
    lang_text_draw_amount(8, 0, city_data.population.population / 10 + 20, 116 + width, 221, FONT_NORMAL_BLACK);
    // Grand festival
    button_border_draw(100, 243, 448, 26, focus_id_festival_size_button == 3);
    width = lang_text_draw(58, 33, 116, 251, FONT_NORMAL_BLACK);
    width += lang_text_draw_amount(8, 0, city_data.population.population / 5 + 40, 116 + width, 251, FONT_NORMAL_BLACK);
    width += lang_text_draw_amount(8, 10, city_data.population.population / 500 + 1, 126 + width, 251, FONT_NORMAL_BLACK);
    image_draw(resource_images[RESOURCE_WINE].icon_img_id, 126 + width, 246);
    // greying out of buttons
    if (!city_finance_can_afford(city_data.festival.cost)) {
        graphics_shade_rect(102, (city_data.festival.size * 30 - 30) + 185, 444, 22, 0);
    } else if (city_data.resource.stored_in_warehouses[RESOURCE_WINE] < city_data.population.population / 500 + 1) {
        graphics_shade_rect(102, 245, 444, 22, 0);
    }
    image_buttons_draw(0, 0, button_help_hold_festival, 1);
    graphics_reset_dialog();
}

static void button_select_god(int god, __attribute__((unused)) int param2)
{
    city_data.festival.god = god;
    window_invalidate();
}

static struct generic_button_t buttons_select_god[] = {
    {101, 76, 81, 91, button_select_god, button_none, 0, 0},
    {192, 76, 81, 91, button_select_god, button_none, 1, 0},
    {283, 76, 81, 91, button_select_god, button_none, 2, 0},
    {374, 76, 81, 91, button_select_god, button_none, 3, 0},
    {465, 76, 81, 91, button_select_god, button_none, 4, 0},
};

static void button_throw_festival(int size, int cost)
{
    if (!city_finance_can_afford(cost) || (size == FESTIVAL_GRAND && city_data.resource.stored_in_warehouses[RESOURCE_WINE] < city_data.population.population / 500 + 1)) {
        city_data.festival.size = FESTIVAL_NONE;
        city_data.festival.cost = 0;
        return;
    } else {
        city_data.festival.size = size;
        city_data.festival.cost = cost;
        city_data.festival.months_to_go = city_data.festival.size + 1;
        city_finance_process_misc(city_data.festival.cost);
        if (city_data.festival.size == FESTIVAL_GRAND) {
            building_warehouses_remove_resource(RESOURCE_WINE, city_data.population.population / 500 + 1);
        }
        window_advisors_show(ADVISOR_ENTERTAINMENT);
    }
}

static struct generic_button_t buttons_festival_size[] = {
    {100, 183, 448, 26, button_throw_festival, button_none, 1, 0},
    {100, 213, 448, 26, button_throw_festival, button_none, 2, 0},
    {100, 243, 448, 26, button_throw_festival, button_none, 3, 0},
};

static void handle_input_hold_festival(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_advisors_show(ADVISOR_ENTERTAINMENT);
        return;
    }
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_select_god, sizeof(buttons_select_god) / sizeof(struct generic_button_t), &focus_id_gods_button)) {
        return;
    }
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_festival_size, sizeof(buttons_festival_size) / sizeof(struct generic_button_t), &focus_id_festival_size_button)) {
        return;
    }
    if (image_buttons_handle_mouse(m_dialog, 0, 0, button_help_hold_festival, 1, &focus_help_button_id)) {
        return;
    }
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 52 || m_dialog->y < 12 || m_dialog->x > 596 || m_dialog->y > 300)) {
        window_go_back();
        return;
    }
}

static int draw_background_entertainment_advisor(void)
{
    city_gods_calculate_moods(0);
    city_culture_calculate();
    outer_panel_draw(0, 0, 40, ENTERTAINMENT_ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 8, 10, 10);
    lang_text_draw(58, 0, 60, 12, FONT_LARGE_BLACK);
    lang_text_draw(58, 1, 180, 46, FONT_SMALL_PLAIN);
    lang_text_draw(58, 2, 260, 46, FONT_SMALL_PLAIN);
    lang_text_draw(58, 3, PEOPLE_OFFSET + 10, 46, FONT_SMALL_PLAIN);
    lang_text_draw_centered(58, 4, COVERAGE_OFFSET, 46, COVERAGE_WIDTH, FONT_SMALL_PLAIN);
    inner_panel_draw(32, 60, 36, 5);
    // theaters
    lang_text_draw_amount(8, 34, building_count_total(BUILDING_THEATER), 40, 64, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_active(BUILDING_THEATER), 150, 64, 100, FONT_NORMAL_WHITE);
    text_draw_number_centered(city_data.entertainment.theater_shows, 230, 64, 100, FONT_NORMAL_WHITE);
    int width = text_draw_number(500 * building_count_active(BUILDING_THEATER), '_', " ",
        PEOPLE_OFFSET, 64, FONT_NORMAL_WHITE);
    lang_text_draw(58, 5, PEOPLE_OFFSET + width, 64, FONT_NORMAL_WHITE);
    int pct_theater = city_culture_coverage_theater();
    if (pct_theater == 0) {
        lang_text_draw_centered(57, 10, COVERAGE_OFFSET, 64, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
    } else if (pct_theater < 100) {
        lang_text_draw_centered(57, 11 + pct_theater / 10, COVERAGE_OFFSET, 64, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
        //lang_text_draw_centered(57, 17, COVERAGE_OFFSET, 64, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
    } else {
        lang_text_draw_centered(57, 21, COVERAGE_OFFSET, 64, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
    }
    // amphitheaters
    lang_text_draw_amount(8, 36, building_count_total(BUILDING_AMPHITHEATER), 40, 84, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_active(BUILDING_AMPHITHEATER), 150, 84, 100, FONT_NORMAL_WHITE);
    text_draw_number_centered(city_data.entertainment.amphitheater_shows, 230, 84, 100, FONT_NORMAL_WHITE);
    width = text_draw_number(800 * building_count_active(BUILDING_AMPHITHEATER), '@', " ",
        PEOPLE_OFFSET, 84, FONT_NORMAL_WHITE);
    lang_text_draw(58, 5, PEOPLE_OFFSET + width, 84, FONT_NORMAL_WHITE);
    int pct_amphitheater = city_culture_coverage_amphitheater();
    if (pct_amphitheater == 0) {
        lang_text_draw_centered(57, 10, COVERAGE_OFFSET, 84, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
    } else if (pct_amphitheater < 100) {
        lang_text_draw_centered(57, 11 + pct_amphitheater / 10,
            COVERAGE_OFFSET, 84, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
    } else {
        lang_text_draw_centered(57, 21, COVERAGE_OFFSET, 84, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
    }
    // colosseums
    lang_text_draw_amount(8, 38, building_count_total(BUILDING_COLOSSEUM), 40, 104, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_active(BUILDING_COLOSSEUM), 150, 104, 100, FONT_NORMAL_WHITE);
    text_draw_number_centered(city_data.entertainment.colosseum_shows, 230, 104, 100, FONT_NORMAL_WHITE);
    width = text_draw_number(1500 * building_count_active(BUILDING_COLOSSEUM), '@', " ",
        PEOPLE_OFFSET, 104, FONT_NORMAL_WHITE);
    lang_text_draw(58, 5, PEOPLE_OFFSET + width, 104, FONT_NORMAL_WHITE);
    int pct_colosseum = city_culture_coverage_colosseum();
    if (pct_colosseum == 0) {
        lang_text_draw_centered(57, 10, COVERAGE_OFFSET, 104, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
    } else if (pct_colosseum < 100) {
        lang_text_draw_centered(57, 11 + pct_colosseum / 10, COVERAGE_OFFSET, 104, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
    } else {
        lang_text_draw_centered(57, 21, COVERAGE_OFFSET, 104, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
    }
    // hippodromes
    lang_text_draw_amount(8, 40, building_count_total(BUILDING_HIPPODROME), 40, 123, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_active(BUILDING_HIPPODROME), 150, 123, 100, FONT_NORMAL_WHITE);
    text_draw_number_centered(city_data.entertainment.hippodrome_shows, 230, 123, 100, FONT_NORMAL_WHITE);
    lang_text_draw_centered(58, 6, PEOPLE_OFFSET + 10, 123, 100, FONT_NORMAL_WHITE);
    if (city_culture_coverage_hippodrome() == 0) {
        lang_text_draw_centered(57, 10, COVERAGE_OFFSET, 123, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
    } else {
        lang_text_draw_centered(57, 21, COVERAGE_OFFSET, 123, COVERAGE_WIDTH, FONT_NORMAL_WHITE);
    }
    int entertainment_advice = 1;
    struct house_demands_t *demands = &city_data.houses;
    if (demands->missing.entertainment > demands->missing.more_entertainment) {
        entertainment_advice = 3;
    } else if (!demands->missing.more_entertainment) {
        entertainment_advice = city_data.culture.average_entertainment ? 1 : 0;
    } else if (city_data.entertainment.venue_needing_shows) {
        entertainment_advice = 3 + city_data.entertainment.venue_needing_shows;
    }
    lang_text_draw_multiline(58, 7 + entertainment_advice, 60, 148, 512, FONT_NORMAL_BLACK);
    inner_panel_draw(48, 252, 34, 6);
    image_draw(image_group(GROUP_PANEL_WINDOWS) + 15, 460, 255);
    lang_text_draw(58, 17, 52, 224, FONT_LARGE_BLACK);
    width = lang_text_draw_amount(8, 4, city_data.festival.months_since_festival, 112, 260, FONT_NORMAL_WHITE);
    lang_text_draw(58, 15, 112 + width, 260, FONT_NORMAL_WHITE);
    if (city_data.festival.size) {
        lang_text_draw_centered(58, 34, 102, 284, 300, FONT_NORMAL_WHITE);
    } else {
        lang_text_draw_centered(58, 16, 102, 284, 300, FONT_NORMAL_WHITE);
    }
    int festival_advice = 6;
    if (city_data.festival.months_since_festival <= 1) {
        festival_advice = 0;
    } else if (city_data.festival.months_since_festival <= 6) {
        festival_advice = 1;
    } else if (city_data.festival.months_since_festival <= 12) {
        festival_advice = 2;
    } else if (city_data.festival.months_since_festival <= 18) {
        festival_advice = 3;
    } else if (city_data.festival.months_since_festival <= 24) {
        festival_advice = 4;
    } else if (city_data.festival.months_since_festival <= 30) {
        festival_advice = 5;
    }
    lang_text_draw_multiline(58, 18 + festival_advice, 56, 305, 400, FONT_NORMAL_WHITE);
    return ENTERTAINMENT_ADVISOR_HEIGHT;
}

static void draw_foreground_entertainment_advisor(void)
{
    if (!city_data.festival.size) {
        button_border_draw(102, 280, 300, 20, focus_button_id_entertainment_advisor == 1);
    }
}

static void button_hold_festival(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (!city_data.festival.size) {
        struct window_type_t window = {
            WINDOW_HOLD_FESTIVAL,
            window_advisors_draw_dialog_background,
            draw_foreground_hold_festival,
            handle_input_hold_festival,
        };
        buttons_festival_size[0].parameter2 = city_data.population.population / 20 + 10;
        buttons_festival_size[1].parameter2 = city_data.population.population / 10 + 20;
        buttons_festival_size[2].parameter2 = city_data.population.population / 5 + 40;
        window_show(&window);
    }
}

static struct generic_button_t hold_festival_button[] = {
    {102, 280, 300, 20, button_hold_festival, button_none, 0, 0},
};

static int handle_mouse_entertainment_advisor(const struct mouse_t *m)
{
    return generic_buttons_handle_mouse(m, 0, 0, hold_festival_button, 1, &focus_button_id_entertainment_advisor);
}

static struct advisor_window_type_t *window_advisor_entertainment(void)
{
    static struct advisor_window_type_t window = {
        draw_background_entertainment_advisor,
        draw_foreground_entertainment_advisor,
        handle_mouse_entertainment_advisor,
    };
    focus_button_id_entertainment_advisor = 0;
    return &window;
}

static void button_change_taxes(int value, __attribute__((unused)) int param2)
{
    city_data.finance.tax_percentage = calc_bound(city_data.finance.tax_percentage + value, 0, 25);
    city_finance_estimate_taxes();
    city_finance_calculate_totals();
    window_invalidate();
}

static struct arrow_button_t arrow_buttons_taxes[] = {
    {180, 75, 17, 24, button_change_taxes, -1, 0, 0, 0},
    {204, 75, 15, 24, button_change_taxes, 1, 0, 0, 0}
};

static void draw_row(int group, int number, int y, int value_last_year, int value_this_year)
{
    lang_text_draw(group, number, 80, y, FONT_NORMAL_BLACK);
    text_draw_number(value_last_year, '@', " ", 290, y, FONT_NORMAL_BLACK);
    text_draw_number(value_this_year, '@', " ", 430, y, FONT_NORMAL_BLACK);
}

static int draw_background_financial_advisor(void)
{
    outer_panel_draw(0, 0, 40, FINANCIAL_ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 10, 10, 10);
    lang_text_draw(60, 0, 60, 12, FONT_LARGE_BLACK);
    inner_panel_draw(64, 48, 34, 5);
    int width;
    if (city_data.finance.treasury < 0) {
        width = lang_text_draw(60, 3, 70, 58, FONT_NORMAL_RED);
        lang_text_draw_amount(8, 0, -city_data.finance.treasury, 72 + width, 58, FONT_NORMAL_RED);
    } else {
        width = lang_text_draw(60, 2, 70, 58, FONT_NORMAL_WHITE);
        lang_text_draw_amount(8, 0, city_data.finance.treasury, 72 + width, 58, FONT_NORMAL_WHITE);
    }
    // tax percentage and estimated income
    lang_text_draw(60, 1, 70, 81, FONT_NORMAL_WHITE);
    width = text_draw_percentage(city_data.finance.tax_percentage, 240, 81, FONT_NORMAL_WHITE);
    width += lang_text_draw(60, 4, 240 + width, 81, FONT_NORMAL_WHITE);
    lang_text_draw_amount(8, 0, city_data.finance.estimated_tax_income, 240 + width, 81, FONT_NORMAL_WHITE);
    // percentage taxpayers
    width = text_draw_percentage(city_data.taxes.percentage_taxed_people, 70, 103, FONT_NORMAL_WHITE);
    lang_text_draw(60, 5, 70 + width, 103, FONT_NORMAL_WHITE);
    // table headers
    lang_text_draw(60, 6, 270, 133, FONT_NORMAL_BLACK);
    lang_text_draw(60, 7, 400, 133, FONT_NORMAL_BLACK);
    // income
    draw_row(60, 8, 155, city_data.finance.last_year.income.taxes, city_data.finance.this_year.income.taxes);
    draw_row(60, 9, 170, city_data.finance.last_year.income.exports, city_data.finance.this_year.income.exports);
    draw_row(60, 20, 185, city_data.finance.last_year.income.donated, city_data.finance.this_year.income.donated);
    graphics_draw_horizontal_line(280, 350, 198, COLOR_BLACK);
    graphics_draw_horizontal_line(420, 490, 198, COLOR_BLACK);
    draw_row(60, 10, 203, city_data.finance.last_year.income.total, city_data.finance.this_year.income.total);
    // expenses
    draw_row(60, 11, 227, city_data.finance.last_year.expenses.imports, city_data.finance.this_year.expenses.imports);
    draw_row(60, 12, 242, city_data.finance.last_year.expenses.wages, city_data.finance.this_year.expenses.wages);
    draw_row(60, 13, 257, city_data.finance.last_year.expenses.construction, city_data.finance.this_year.expenses.construction);
    // interest (with percentage)
    width = lang_text_draw(60, 14, 80, 272, FONT_NORMAL_BLACK);
    text_draw_percentage(10, 80 + width, 272, FONT_NORMAL_BLACK);
    text_draw_number(city_data.finance.last_year.expenses.interest, '@', " ", 290, 272, FONT_NORMAL_BLACK);
    text_draw_number(city_data.finance.last_year.expenses.interest, '@', " ", 430, 272, FONT_NORMAL_BLACK);
    draw_row(60, 15, 287, city_data.finance.last_year.expenses.salary, city_data.finance.this_year.expenses.salary);
    draw_row(60, 16, 302, city_data.finance.last_year.expenses.sundries, city_data.finance.this_year.expenses.sundries);
    draw_row(60, 21, 317, city_data.finance.last_year.expenses.tribute, city_data.finance.this_year.expenses.tribute);
    graphics_draw_horizontal_line(280, 350, 330, COLOR_BLACK);
    graphics_draw_horizontal_line(420, 490, 330, COLOR_BLACK);
    draw_row(60, 17, 335, city_data.finance.last_year.expenses.total, city_data.finance.this_year.expenses.total);
    draw_row(60, 18, 358, city_data.finance.last_year.net_in_out, city_data.finance.this_year.net_in_out);
    draw_row(60, 19, 381, city_data.finance.last_year.balance, city_data.finance.this_year.balance);
    return FINANCIAL_ADVISOR_HEIGHT;
}

static void draw_foreground_financial_advisor(void)
{
    arrow_buttons_draw(0, 0, arrow_buttons_taxes, 2);
}

static int handle_mouse_financial_advisor(const struct mouse_t *m)
{
    return arrow_buttons_handle_mouse(m, 0, 0, arrow_buttons_taxes, 2, &arrow_button_focus_financial_advisor);
}

static struct advisor_window_type_t *window_advisor_financial(void)
{
    static struct advisor_window_type_t window = {
        draw_background_financial_advisor,
        draw_foreground_financial_advisor,
        handle_mouse_financial_advisor,
    };
    return &window;
}

static int draw_background_health_advisor(void)
{
    outer_panel_draw(0, 0, 40, HEALTH_ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 6, 10, 10);
    lang_text_draw(56, 0, 60, 12, FONT_LARGE_BLACK);
    if (city_data.population.population >= 200) {
        lang_text_draw_multiline(56, city_data.health.value / 10 + 16, 60, 46, 512, FONT_NORMAL_BLACK);
    } else {
        lang_text_draw_multiline(56, 15, 60, 46, 512, FONT_NORMAL_BLACK);
    }
    lang_text_draw(56, 3, 180, 94, FONT_SMALL_PLAIN);
    lang_text_draw(56, 4, 290, 94, FONT_SMALL_PLAIN);
    lang_text_draw_centered(56, 5, 440, 94, 160, FONT_SMALL_PLAIN);
    inner_panel_draw(32, 108, 36, 5);
    // bathhouses
    lang_text_draw_amount(8, 24, building_count_total(BUILDING_BATHHOUSE), 40, 112, FONT_NORMAL_GREEN);
    text_draw_number_centered(building_count_active(BUILDING_BATHHOUSE), 150, 112, 100, FONT_NORMAL_GREEN);
    lang_text_draw_centered(56, 2, 290, 112, 120, FONT_NORMAL_GREEN);
    lang_text_draw_centered(56, 2, 440, 112, 160, FONT_NORMAL_GREEN);
    // barbers
    lang_text_draw_amount(8, 26, building_count_total(BUILDING_BARBER), 40, 132, FONT_NORMAL_GREEN);
    text_draw_number_centered(building_count_active(BUILDING_BARBER), 150, 132, 100, FONT_NORMAL_GREEN);
    lang_text_draw_centered(56, 2, 290, 132, 120, FONT_NORMAL_GREEN);
    lang_text_draw_centered(56, 2, 440, 132, 160, FONT_NORMAL_GREEN);
    // clinics
    lang_text_draw_amount(8, 28, building_count_total(BUILDING_DOCTOR), 40, 152, FONT_NORMAL_GREEN);
    text_draw_number_centered(building_count_active(BUILDING_DOCTOR), 150, 152, 100, FONT_NORMAL_GREEN);
    lang_text_draw_centered(56, 2, 290, 152, 120, FONT_NORMAL_GREEN);
    lang_text_draw_centered(56, 2, 440, 152, 160, FONT_NORMAL_GREEN);
    // hospitals
    lang_text_draw_amount(8, 30, building_count_total(BUILDING_HOSPITAL), 40, 172, FONT_NORMAL_GREEN);
    text_draw_number_centered(building_count_active(BUILDING_HOSPITAL), 150, 172, 100, FONT_NORMAL_GREEN);
    int width = text_draw_number(1000 * building_count_active(BUILDING_HOSPITAL), '@', " ", 280, 172, FONT_NORMAL_GREEN);
    lang_text_draw(56, 6, 280 + width, 172, FONT_NORMAL_GREEN);
    int pct_hospital = city_culture_coverage_hospital();
    if (pct_hospital == 0) {
        lang_text_draw_centered(57, 10, 440, 172, 160, FONT_NORMAL_GREEN);
    } else if (pct_hospital < 100) {
        lang_text_draw_centered(57, pct_hospital / 10 + 11, 440, 172, 160, FONT_NORMAL_GREEN);
    } else {
        lang_text_draw_centered(57, 21, 440, 172, 160, FONT_NORMAL_GREEN);
    }
    int health_advice = 7;
    struct house_demands_t *demands = &city_data.houses;
    switch (demands->health) {
        case 1:
            health_advice = demands->requiring.bathhouse ? 1 : 0;
            break;
        case 2:
            health_advice = demands->requiring.barber ? 3 : 2;
            break;
        case 3:
            health_advice = demands->requiring.clinic ? 5 : 4;
            break;
        case 4:
            health_advice = 6;
            break;
        default:
            break;
    }
    lang_text_draw_multiline(56, 7 + health_advice, 60, 194, 512, FONT_NORMAL_BLACK);
    return HEALTH_ADVISOR_HEIGHT;
}

static struct advisor_window_type_t *window_advisor_health(void)
{
    static struct advisor_window_type_t window = {
        draw_background_health_advisor,
        0,
        0,
    };
    return &window;
}

static void arrow_button_amount(int value, __attribute__((unused)) int param2)
{
    city_data.emperor.donate_amount = calc_bound(city_data.emperor.donate_amount + value, 0, city_data.emperor.personal_savings);
    window_invalidate();
}

static struct arrow_button_t arrow_buttons_donate_to_city[] = {
    {455, 230, 17, 24, arrow_button_amount, -10, 0, 0, 0},
    {479, 230, 15, 24, arrow_button_amount, 10, 0, 0, 0},
};

static void draw_foreground_donate_to_city(void)
{
    graphics_in_dialog();
    outer_panel_draw(108, 172, 27, 8);
    // Coin image
    image_draw(COIN_IMAGE_ID, 124, 188);
    // Give money to the city
    lang_text_draw_centered(52, 16, 108, 188, 432, FONT_LARGE_BLACK);
    inner_panel_draw(124, 220, 25, 4);
    // 0
    button_border_draw(144, 230, 64, 20, donate_to_city_data.focus_button_id == 1);
    text_draw_number_centered(0, 142, 235, 64, FONT_NORMAL_WHITE);
    // 500
    button_border_draw(144, 257, 64, 20, donate_to_city_data.focus_button_id == 2);
    text_draw_number_centered(500, 142, 262, 64, FONT_NORMAL_WHITE);
    // 2000
    button_border_draw(224, 230, 64, 20, donate_to_city_data.focus_button_id == 3);
    text_draw_number_centered(2000, 222, 235, 64, FONT_NORMAL_WHITE);
    // 5000
    button_border_draw(224, 257, 64, 20, donate_to_city_data.focus_button_id == 4);
    text_draw_number_centered(5000, 222, 262, 64, FONT_NORMAL_WHITE);
    // All
    button_border_draw(304, 257, 64, 20, donate_to_city_data.focus_button_id == 5);
    lang_text_draw_centered(52, 19, 304, 262, 64, FONT_NORMAL_WHITE);
    // Donation is
    lang_text_draw(52, 17, 304, 235, FONT_NORMAL_WHITE);
    text_draw_number(city_data.emperor.donate_amount, '@', " ", 394, 235, FONT_NORMAL_GREEN);
    arrow_buttons_draw(0, 0, arrow_buttons_donate_to_city, sizeof(arrow_buttons_donate_to_city) / sizeof(struct arrow_button_t));
    // Give money
    button_border_draw(384, 257, 120, 20, donate_to_city_data.focus_button_id == 6);
    lang_text_draw_centered(52, 18, 384, 262, 120, FONT_NORMAL_GREEN);
    graphics_reset_dialog();
}

static void button_set_amount(int amount_id, __attribute__((unused)) int param2)
{
    int amount;
    switch (amount_id) {
        case 0: amount = 0; break;
        case 1: amount = 500; break;
        case 2: amount = 2000; break;
        case 3: amount = 5000; break;
        case 4: amount = 1000000; break;
        default: return;
    }
    city_data.emperor.donate_amount = calc_bound(amount, 0, city_data.emperor.personal_savings);
    window_invalidate();
}

static void button_donate(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    city_finance_process_donation(city_data.emperor.donate_amount);
    city_data.emperor.personal_savings -= city_data.emperor.donate_amount;
    city_finance_calculate_totals();
    window_advisors_show(ADVISOR_IMPERIAL);
}

static struct generic_button_t buttons_donate_to_city[] = {
    {144, 230, 64, 20, button_set_amount, button_none, 0, 0},
    {144, 257, 64, 20, button_set_amount, button_none, 1, 0},
    {224, 230, 64, 20, button_set_amount, button_none, 2, 0},
    {224, 257, 64, 20, button_set_amount, button_none, 3, 0},
    {304, 257, 64, 20, button_set_amount, button_none, 4, 0},
    {384, 257, 120, 20, button_donate, button_none, 0, 0},
};

static void handle_input_donate_to_city(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }
    donate_to_city_data.focus_arrow_button_id = 0;
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_donate_to_city, sizeof(buttons_donate_to_city) / sizeof(struct generic_button_t), &donate_to_city_data.focus_button_id)) {
        return;
    }
    if (arrow_buttons_handle_mouse(m_dialog, 0, 0, arrow_buttons_donate_to_city, sizeof(arrow_buttons_donate_to_city) / sizeof(struct arrow_button_t), &donate_to_city_data.focus_arrow_button_id)) {
        return;
    }
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 108 || m_dialog->y < 172 || m_dialog->x > 540 || m_dialog->y > 300)) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }
}

static void button_donate_to_city(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_DONATE_TO_CITY,
        window_advisors_draw_dialog_background,
        draw_foreground_donate_to_city,
        handle_input_donate_to_city,
    };
    city_data.emperor.donate_amount = 0;
    window_show(&window);
}

static void draw_foreground_set_salary(void)
{
    graphics_in_dialog();
    outer_panel_draw(164, 32, 20, 23);
    // Coin image
    image_draw(COIN_IMAGE_ID, 180, 48);
    // Set salary level
    lang_text_draw_centered(52, 15, 164, 48, 320, FONT_LARGE_BLACK);
    inner_panel_draw(180, 80, 18, 15);
    for (int rank = 0; rank < 11; rank++) {
        int font = focus_button_id_set_salary == rank + 1 ? FONT_NORMAL_RED : FONT_NORMAL_WHITE;
        lang_text_draw(52, rank + 4, 196, 96 + 20 * rank, font);
        text_draw_money(city_emperor_salary_for_rank(rank), 385, 96 + 20 * rank, font);
    }
    if (!city_data.mission.has_won) {
        if (city_data.emperor.salary_rank <= city_data.emperor.player_rank) {
            lang_text_draw_multiline(52, 76, 185, 336, 288, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw_multiline(52, 71, 185, 336, 288, FONT_NORMAL_BLACK);
        }
    } else {
        graphics_shade_rect(180, 80, 288, 240, 0);
        lang_text_draw_multiline(52, 77, 185, 336, 288, FONT_NORMAL_BLACK);
    }
    graphics_reset_dialog();
}

static void button_set_salary_set_salary(int rank, __attribute__((unused)) int param2)
{
    if (!city_data.mission.has_won) {
        city_emperor_set_salary_rank(rank);
        city_data.finance.this_year.expenses.salary = city_data.finance.salary_so_far;
        city_ratings_update_favor_explanation();
        window_advisors_show(ADVISOR_IMPERIAL);
    }
}

static struct generic_button_t buttons_set_salary[] = {
    {196, 96, 250, 15, button_set_salary_set_salary, button_none, 0, 0},
    {196, 116, 250, 15, button_set_salary_set_salary, button_none, 1, 0},
    {196, 136, 250, 15, button_set_salary_set_salary, button_none, 2, 0},
    {196, 156, 250, 15, button_set_salary_set_salary, button_none, 3, 0},
    {196, 176, 250, 15, button_set_salary_set_salary, button_none, 4, 0},
    {196, 196, 250, 15, button_set_salary_set_salary, button_none, 5, 0},
    {196, 216, 250, 15, button_set_salary_set_salary, button_none, 6, 0},
    {196, 236, 250, 15, button_set_salary_set_salary, button_none, 7, 0},
    {196, 256, 250, 15, button_set_salary_set_salary, button_none, 8, 0},
    {196, 276, 250, 15, button_set_salary_set_salary, button_none, 9, 0},
    {196, 296, 250, 15, button_set_salary_set_salary, button_none, 10, 0},
};

static void handle_input_set_salary(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_set_salary, sizeof(buttons_set_salary) / sizeof(struct generic_button_t), &focus_button_id_set_salary)) {
        return;
    }
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 164 || m_dialog->y < 32 || m_dialog->x > 484 || m_dialog->y > 400)) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }
}

static void button_set_salary_imperial_advisor(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_SET_SALARY,
        window_advisors_draw_dialog_background,
        draw_foreground_set_salary,
        handle_input_set_salary,
    };
    window_show(&window);
}

static void draw_foreground_gift_to_emperor(void)
{
    graphics_in_dialog();
    outer_panel_draw(84, 108, 30, 12);
    // Coin image
    image_draw(COIN_IMAGE_ID, 100, 124);
    // Send the Emperor a gift
    lang_text_draw_centered(52, 69, 84, 124, 480, FONT_LARGE_BLACK);
    inner_panel_draw(100, 165, 28, 5);
    // Modest gift
    if (city_data.emperor.gifts[GIFT_MODEST].cost <= city_data.emperor.personal_savings) {
        lang_text_draw(52, 63, 120, 180, FONT_NORMAL_WHITE);
        int font = focus_button_id_gift_to_emperor == 1 ? FONT_NORMAL_RED : FONT_NORMAL_WHITE;
        lang_text_draw(52, 51 + city_data.emperor.gifts[GIFT_MODEST].id, 210, 180, font);
        text_draw_money(city_data.emperor.gifts[GIFT_MODEST].cost, 460, 180, font);
    } else {
        lang_text_draw_multiline(52, 70, 110, 180, 448, FONT_NORMAL_WHITE);
    }
    // Generous gift
    if (city_data.emperor.gifts[GIFT_GENEROUS].cost <= city_data.emperor.personal_savings) {
        lang_text_draw(52, 64, 120, 200, FONT_NORMAL_WHITE);
        int font = focus_button_id_gift_to_emperor == 2 ? FONT_NORMAL_RED : FONT_NORMAL_WHITE;
        lang_text_draw(52, 55 + city_data.emperor.gifts[GIFT_GENEROUS].id, 210, 200, font);
        text_draw_money(city_data.emperor.gifts[GIFT_GENEROUS].cost, 460, 200, font);
    }
    // Lavish gift
    if (city_data.emperor.gifts[GIFT_LAVISH].cost <= city_data.emperor.personal_savings) {
        lang_text_draw(52, 65, 120, 220, FONT_NORMAL_WHITE);
        int font = focus_button_id_gift_to_emperor == 3 ? FONT_NORMAL_RED : FONT_NORMAL_WHITE;
        lang_text_draw(52, 59 + city_data.emperor.gifts[GIFT_LAVISH].id, 210, 220, font);
        text_draw_money(city_data.emperor.gifts[GIFT_LAVISH].cost, 460, 220, font);
    }
    // Time since last gift
    int width = lang_text_draw(52, 50, 200, 260, FONT_NORMAL_BLACK);
    lang_text_draw_amount(8, 4, city_data.emperor.months_since_gift, width + 200, 260, FONT_NORMAL_BLACK);
    graphics_reset_dialog();
}

static void button_send_gift(int gift_size, __attribute__((unused)) int param2)
{
    if (city_data.emperor.gifts[gift_size].cost <= city_data.emperor.personal_savings) {
        int cost = city_data.emperor.gifts[gift_size].cost;
        if (cost <= city_data.emperor.personal_savings) {
            if (city_data.emperor.gift_overdose_penalty <= 0) {
                city_data.emperor.gift_overdose_penalty = 1;
                if (gift_size == GIFT_MODEST) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 3, 0, 100);
                } else if (gift_size == GIFT_GENEROUS) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 5, 0, 100);
                } else if (gift_size == GIFT_LAVISH) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 10, 0, 100);
                }
            } else if (city_data.emperor.gift_overdose_penalty == 1) {
                city_data.emperor.gift_overdose_penalty = 2;
                if (gift_size == GIFT_MODEST) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 1, 0, 100);
                } else if (gift_size == GIFT_GENEROUS) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 3, 0, 100);
                } else if (gift_size == GIFT_LAVISH) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 5, 0, 100);
                }
            } else if (city_data.emperor.gift_overdose_penalty == 2) {
                city_data.emperor.gift_overdose_penalty = 3;
                if (gift_size == GIFT_MODEST) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 0, 0, 100);
                } else if (gift_size == GIFT_GENEROUS) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 0, 0, 100);
                } else if (gift_size == GIFT_LAVISH) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 3, 0, 100);
                }
            } else if (city_data.emperor.gift_overdose_penalty == 3) {
                city_data.emperor.gift_overdose_penalty = 4;
                if (gift_size == GIFT_MODEST) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 0, 0, 100);
                } else if (gift_size == GIFT_GENEROUS) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 0, 0, 100);
                } else if (gift_size == GIFT_LAVISH) {
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 1, 0, 100);
                }
            }
            city_data.emperor.months_since_gift = 0;
            // rotate gift type
            city_data.emperor.gifts[gift_size].id++;
            if (city_data.emperor.gifts[gift_size].id >= 4) {
                city_data.emperor.gifts[gift_size].id = 0;
            }
            city_data.emperor.personal_savings -= cost;
        }
        window_advisors_show(ADVISOR_IMPERIAL);
    }
}

static struct generic_button_t buttons_gift_to_emperor[] = {
    {210, 180, 325, 15, button_send_gift, button_none, 0, 0},
    {210, 200, 325, 15, button_send_gift, button_none, 1, 0},
    {210, 220, 325, 15, button_send_gift, button_none, 2, 0},
};

static void handle_input_gift_to_emperor(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_gift_to_emperor, sizeof(buttons_gift_to_emperor) / sizeof(struct generic_button_t), &focus_button_id_gift_to_emperor)) {
        return;
    }
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 84 || m_dialog->y < 108 || m_dialog->x > 564 || m_dialog->y > 300)) {
        window_advisors_show(ADVISOR_IMPERIAL);
        return;
    }
}

static void button_gift_to_emperor(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_GIFT_TO_EMPEROR,
        window_advisors_draw_dialog_background,
        draw_foreground_gift_to_emperor,
        handle_input_gift_to_emperor,
    };
    // calculate gift costs
    city_data.emperor.gifts[GIFT_MODEST].cost = city_data.emperor.personal_savings / 8 + 20;
    city_data.emperor.gifts[GIFT_GENEROUS].cost = city_data.emperor.personal_savings / 4 + 50;
    city_data.emperor.gifts[GIFT_LAVISH].cost = city_data.emperor.personal_savings / 2 + 100;
    window_show(&window);
}

static void confirm_nothing(void)
{}

static void confirm_send_troops(void)
{
    int legions_sent = 0;
    int roman_strength = 0;
    for (int i = 0; i < MAX_LEGIONS; i++) {
        if (legion_formations[i].in_use && legion_formations[i].empire_service && legion_formations[i].num_figures) {
            struct formation_t *m = &legion_formations[i];
            m->in_distant_battle = 1;
            for (int fig = 0; fig < m->num_figures; fig++) {
                if (m->figures[fig] > 0) {
                    struct figure_t *f = &figures[m->figures[fig]];
                    if (figure_is_alive(f)) {
                        f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_DISTANT_BATTLE;
                    }
                }
            }
            int strength_factor;
            if (m->has_military_training) {
                strength_factor = m->figure_type == FIGURE_FORT_LEGIONARY ? 3 : 2;
            } else {
                strength_factor = m->figure_type == FIGURE_FORT_LEGIONARY ? 2 : 1;
            }
            roman_strength += strength_factor * m->num_figures;
            legions_sent++;
        }
    }
    if (legions_sent > 0) {
        city_data.distant_battle.roman_months_to_travel_forth = scenario.empire.distant_battle_roman_travel_months;
        city_data.distant_battle.roman_strength = roman_strength;
    }
    window_empire_show();
}

static void on_scroll_imperial_advisor(void)
{
    window_invalidate();
}

static struct scrollbar_type_t scrollbar_imperial_advisor = { 591, 90, 222, on_scroll_imperial_advisor, 0, 0, 0, 0, 0, 0 };

static void confirm_send_goods(void)
{
    dispatch_imperial_request(selected_request_id);
    goods_requests_to_draw--;
    scrollbar_update_max(&scrollbar_imperial_advisor, scrollbar_imperial_advisor.max_scroll_position - 1);
}

static int get_request_status(int index)
{
    int num_requests = 0;
    if (city_data.distant_battle.months_until_battle && !city_data.distant_battle.roman_months_to_travel_forth) {
        num_requests = 1;
        if (index == 0) {
            int legions_selected_emp_service = 0;
            for (int i = 0; i < MAX_LEGIONS; i++) {
                if (legion_formations[i].in_use && legion_formations[i].empire_service && legion_formations[i].num_figures) {
                    legions_selected_emp_service = 1;
                    break;
                }
            }
            if (!city_data.military.total_legions) {
                return STATUS_NO_LEGIONS_AVAILABLE;
            } else if (!legions_selected_emp_service) {
                return STATUS_NO_LEGIONS_SELECTED;
            } else {
                return STATUS_CONFIRM_SEND_LEGIONS;
            }
        }
    }
    int index_offset = index - num_requests;
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (scenario.requests[i].resource && scenario.requests[i].visible &&
            scenario.requests[i].state <= 1) {
            if (index_offset == 0) {
                if (scenario.requests[i].resource == RESOURCE_DENARII) {
                    if (city_data.finance.treasury <= scenario.requests[i].amount) {
                        return STATUS_NOT_ENOUGH_RESOURCES;
                    }
                } else {
                    if (city_data.resource.stored_in_warehouses[scenario.requests[i].resource] < scenario.requests[i].amount) {
                        return STATUS_NOT_ENOUGH_RESOURCES;
                    }
                }
                return i + 1;
            }
            index_offset--;
        }
    }
    return 0;
}

static void button_request(int index, __attribute__((unused)) int param2)
{
    int status = get_request_status(scrollbar_imperial_advisor.scroll_position + index);
    if (status) {
        switch (status) {
            case STATUS_NO_LEGIONS_AVAILABLE:
                window_popup_dialog_show(POPUP_DIALOG_NO_LEGIONS_AVAILABLE, confirm_nothing, 0);
                break;
            case STATUS_NO_LEGIONS_SELECTED:
                window_popup_dialog_show(POPUP_DIALOG_NO_LEGIONS_SELECTED, confirm_nothing, 0);
                break;
            case STATUS_CONFIRM_SEND_LEGIONS:
                window_popup_dialog_show(POPUP_DIALOG_SEND_TROOPS, confirm_send_troops, 2);
                break;
            case STATUS_NOT_ENOUGH_RESOURCES:
                window_popup_dialog_show(POPUP_DIALOG_NOT_ENOUGH_GOODS, confirm_nothing, 0);
                break;
            default:
                selected_request_id = status - 1;
                window_popup_dialog_show(POPUP_DIALOG_SEND_GOODS, confirm_send_goods, 2);
                break;
        }
    }
}

static struct generic_button_t imperial_buttons[] = {
    {38, 96, 550, 40, button_request, button_none, 0, 0},
    {38, 138, 550, 40, button_request, button_none, 1, 0},
    {38, 180, 550, 40, button_request, button_none, 2, 0},
    {38, 222, 550, 40, button_request, button_none, 3, 0},
    {38, 264, 550, 40, button_request, button_none, 4, 0},
    {312, 341, 250, 20, button_gift_to_emperor, button_none, 0, 0},
    {312, 367, 250, 20, button_donate_to_city, button_none, 0, 0},
    {62, 393, 500, 20, button_set_salary_imperial_advisor, button_none, 0, 0},
};

static int draw_background_imperial_advisor(void)
{
    outer_panel_draw(0, 0, 40, IMPERIAL_ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 2, 10, 10);
    text_draw(scenario_settings.player_name, 60, 12, FONT_LARGE_BLACK, 0);
    int width = lang_text_draw(52, 0, 60, 44, FONT_NORMAL_BLACK);
    text_draw_number(city_data.ratings.favor, '@', " ", 60 + width, 44, FONT_NORMAL_BLACK);
    lang_text_draw_multiline(52, city_data.ratings.favor / 5 + 22, 60, 60, 544, FONT_NORMAL_BLACK);
    inner_panel_draw(32, 90, 35, 14);


    int request_index = 0;
    // draw distant battle army request
    if (city_data.distant_battle.months_until_battle && !city_data.distant_battle.roman_months_to_travel_forth) {
        // can send to distant battle
        button_border_draw(38, 96, 550, 40, 0);
        image_draw(resource_images[RESOURCE_WEAPONS].icon_img_id, 50, 106);
        width = lang_text_draw(52, 72, 80, 102, FONT_NORMAL_WHITE);
        lang_text_draw(21, empire_objects[city_data.distant_battle.city].city_name_id, 80 + width, 102, FONT_NORMAL_WHITE);
        int strength_text_id;
        if (city_data.distant_battle.enemy_strength < 46) {
            strength_text_id = 73;
        } else if (city_data.distant_battle.enemy_strength < 89) {
            strength_text_id = 74;
        } else {
            strength_text_id = 75;
        }
        width = lang_text_draw(52, strength_text_id, 80, 120, FONT_NORMAL_WHITE);
        lang_text_draw_amount(8, 4, city_data.distant_battle.months_until_battle, 80 + width, 120, FONT_NORMAL_WHITE);
        request_index = 1;
    }

    if (goods_requests_to_draw) {
        // scroll max position depends on max goods drawn (MAX_REQUESTS_SHOWN, or MAX_REQUESTS_SHOWN - 1 when also drawing distant battle)
        scrollbar_update_max(&scrollbar_imperial_advisor, goods_requests_to_draw + request_index - MAX_REQUESTS_SHOWN);
        // "min" without imported libraries
        int scroll_adjusted_index;
        if (goods_requests_to_draw + request_index <= MAX_REQUESTS_SHOWN) {
            scroll_adjusted_index = 0;
        } else {
            scroll_adjusted_index = scrollbar_imperial_advisor.scroll_position < goods_requests_to_draw + request_index - MAX_REQUESTS_SHOWN ? scrollbar_imperial_advisor.scroll_position : goods_requests_to_draw + request_index - MAX_REQUESTS_SHOWN;
        }
        for (int i = 0; i < MAX_REQUESTS; i++) {
            if (request_index >= MAX_REQUESTS_SHOWN) {
                break;
            }
            if (scenario.requests[i].resource && scenario.requests[i].visible && i >= scroll_adjusted_index) {
                button_border_draw(38, 96 + 42 * request_index, 550, 40, 0);
                text_draw_number(scenario.requests[i].amount, '@', " ", 40, 102 + 42 * request_index, FONT_NORMAL_WHITE);
                image_draw(resource_images[scenario.requests[i].resource].icon_img_id + resource_image_offset(scenario.requests[i].resource, RESOURCE_IMAGE_ICON), 110, 100 + 42 * request_index);
                lang_text_draw(23, scenario.requests[i].resource, 150, 102 + 42 * request_index, FONT_NORMAL_WHITE);

                width = lang_text_draw_amount(8, 4, scenario.requests[i].months_to_comply, 310, 102 + 42 * request_index, FONT_NORMAL_WHITE);
                lang_text_draw(12, 2, 310 + width, 102 + 42 * request_index, FONT_NORMAL_WHITE);

                if (scenario.requests[i].resource == RESOURCE_DENARII) {
                    // request for money
                    width = text_draw_number(city_data.finance.treasury, '@', " ", 40, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                    width += lang_text_draw(52, 44, 40 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                    if (city_data.finance.treasury < scenario.requests[i].amount) {
                        lang_text_draw(52, 48, 105 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                    } else {
                        lang_text_draw(52, 47, 80 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                    }
                } else {
                    // normal goods request
                    int amount_stored = city_data.resource.stored_in_warehouses[scenario.requests[i].resource];
                    width = text_draw_number(amount_stored, '@', " ", 40, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                    width += lang_text_draw(52, 43, 40 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                    if (amount_stored < scenario.requests[i].amount) {
                        lang_text_draw(52, 48, 165 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                    } else {
                        lang_text_draw(52, 47, 127 + width, 120 + 42 * request_index, FONT_NORMAL_WHITE);
                    }
                }
                request_index++;
            }
        }
        if (!request_index) {
            lang_text_draw_multiline(52, 21, 64, 160, 512, FONT_NORMAL_WHITE);
        }
    }
    return IMPERIAL_ADVISOR_HEIGHT;
}

static void draw_foreground_imperial_advisor(void)
{
    inner_panel_draw(56, 324, 32, 6);
    // Request buttons
    if (get_request_status(0)) {
        button_border_draw(38, 96, 550, 40, focus_button_id_imperial_advisor == 1);
    }
    if (get_request_status(1)) {
        button_border_draw(38, 138, 550, 40, focus_button_id_imperial_advisor == 2);
    }
    if (get_request_status(2)) {
        button_border_draw(38, 180, 550, 40, focus_button_id_imperial_advisor == 3);
    }
    if (get_request_status(3)) {
        button_border_draw(38, 222, 550, 40, focus_button_id_imperial_advisor == 4);
    }
    if (get_request_status(4)) {
        button_border_draw(38, 264, 550, 40, focus_button_id_imperial_advisor == 5);
    }
    lang_text_draw(32, city_data.emperor.player_rank, 64, 338, FONT_LARGE_BROWN);
    int width = lang_text_draw(52, 1, 64, 372, FONT_NORMAL_WHITE);
    text_draw_money(city_data.emperor.personal_savings, 72 + width, 372, FONT_NORMAL_WHITE);
    // Send a gift
    button_border_draw(312, 341, 250, 20, focus_button_id_imperial_advisor == 6);
    lang_text_draw_centered(52, 49, 312, 346, 250, FONT_NORMAL_WHITE);
    // Give to city
    button_border_draw(312, 367, 250, 20, focus_button_id_imperial_advisor == 7);
    lang_text_draw_centered(52, 2, 312, 372, 250, FONT_NORMAL_WHITE);
    // Set salary
    button_border_draw(62, 393, 500, 20, focus_button_id_imperial_advisor == 8);
    width = lang_text_draw(52, city_data.emperor.salary_rank + 4, 112, 398, FONT_NORMAL_WHITE);
    width += text_draw_number(city_data.emperor.salary_amount, '@', " ", 112 + width, 398, FONT_NORMAL_WHITE);
    lang_text_draw(52, 3, 112 + width, 398, FONT_NORMAL_WHITE);
    scrollbar_draw(&scrollbar_imperial_advisor);
}

static int handle_mouse_imperial_advisor(const struct mouse_t *m)
{
    if (scrollbar_handle_mouse(&scrollbar_imperial_advisor, m)) {
        return 1;
    }
    if (generic_buttons_handle_mouse(m, 0, 0, imperial_buttons, sizeof(imperial_buttons) / sizeof(struct generic_button_t), &focus_button_id_imperial_advisor)) {
        return 1;
    }
    return 0;
}

static struct advisor_window_type_t *window_advisor_imperial(void)
{
    static struct advisor_window_type_t window = {
        draw_background_imperial_advisor,
        draw_foreground_imperial_advisor,
        handle_mouse_imperial_advisor,
    };
    goods_requests_to_draw = 0;
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (scenario.requests[i].resource && scenario.requests[i].visible) {
            goods_requests_to_draw++;
        }
    }
    scrollbar_init(&scrollbar_imperial_advisor, 0, goods_requests_to_draw - MAX_REQUESTS_SHOWN);
    return &window;
}

static void button_set_priority(int new_priority, __attribute__((unused)) int param2)
{
    city_labor_set_priority(labor_priority_data.category, new_priority);
    window_go_back();
}

static struct generic_button_t priority_buttons[] = {
    {176, 170, 27, 27, button_set_priority, button_none, 1, 0},
    {208, 170, 27, 27, button_set_priority, button_none, 2, 0},
    {240, 170, 27, 27, button_set_priority, button_none, 3, 0},
    {272, 170, 27, 27, button_set_priority, button_none, 4, 0},
    {304, 170, 27, 27, button_set_priority, button_none, 5, 0},
    {336, 170, 27, 27, button_set_priority, button_none, 6, 0},
    {368, 170, 27, 27, button_set_priority, button_none, 7, 0},
    {400, 170, 27, 27, button_set_priority, button_none, 8, 0},
    {432, 170, 27, 27, button_set_priority, button_none, 9, 0},
};

static void button_remove_priority(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    city_labor_set_priority(labor_priority_data.category, 0);
    window_go_back();
}

static struct generic_button_t remove_priority_button[] = {
    {220, 206, 200, 25, button_remove_priority, button_none, 0, 0}
};

static void draw_foreground_labor_priority(void)
{
    graphics_in_dialog();
    outer_panel_draw(160, 122, 20, 8);
    // Priority level
    lang_text_draw_centered(50, 25, 160, 138, 320, FONT_LARGE_BLACK);
    for (int i = 1; i < 10; i++) {
        button_border_draw(144 + 32 * i, 170, 27, 27, labor_priority_data.focus_id_button_set_priority == i);
        lang_text_draw_centered(50, 26 + i, 145 + 32 * i, 172, 27, FONT_LARGE_BLACK);
        if (i > labor_priority_data.max_items) {
            graphics_shade_rect(145 + 32 * i, 171, 25, 25, 1);
        }
    }
    // No priority
    button_border_draw(220, 206, 200, 25, labor_priority_data.focus_id_button_remove_priority);
    lang_text_draw_centered(50, 26, 220, 212, 200, FONT_NORMAL_BLACK);
    graphics_reset_dialog();
}

static void handle_input_labor_priority(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_go_back();
        return;
    }
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0,
        priority_buttons, labor_priority_data.max_items, &labor_priority_data.focus_id_button_set_priority)) {
        return;
    }
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, remove_priority_button, 1, &labor_priority_data.focus_id_button_remove_priority)) {
        return;
    }
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 160 || m_dialog->y < 122 || m_dialog->x > 480 || m_dialog->y > 250)) {
        window_go_back();
        return;
    }
}

static void button_priority(int category, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_LABOR_PRIORITY,
        window_draw_underlying_window,
        draw_foreground_labor_priority,
        handle_input_labor_priority,
    };
    labor_priority_data.category = category;
    labor_priority_data.max_items = 0;
    for (int i = 0; i < 9; i++) {
        if (city_data.labor.categories[i].priority > 0) {
            labor_priority_data.max_items++;
        }
    }
    if (labor_priority_data.max_items < 9 && !city_data.labor.categories[category].priority) {
        // allow space for new priority
        labor_priority_data.max_items++;
    }
    window_show(&window);
}

static struct generic_button_t category_buttons[] = {
    {40, 77, 560, 22, button_priority, button_none, 0, 0},
    {40, 102, 560, 22, button_priority, button_none, 1, 0},
    {40, 127, 560, 22, button_priority, button_none, 2, 0},
    {40, 152, 560, 22, button_priority, button_none, 3, 0},
    {40, 177, 560, 22, button_priority, button_none, 4, 0},
    {40, 202, 560, 22, button_priority, button_none, 5, 0},
    {40, 227, 560, 22, button_priority, button_none, 6, 0},
    {40, 252, 560, 22, button_priority, button_none, 7, 0},
    {40, 277, 560, 22, button_priority, button_none, 8, 0},
};

static void arrow_button_wages(int value, __attribute__((unused)) int param2)
{
    city_data.labor.wages = calc_bound(city_data.labor.wages + value, 0, 100);
    city_finance_estimate_wages();
    city_finance_calculate_totals();
    window_invalidate();
}

static struct arrow_button_t wage_arrow_buttons[] = {
    {158, 354, 17, 24, arrow_button_wages, -1, 0, 0, 0},
    {182, 354, 15, 24, arrow_button_wages, 1, 0, 0, 0}
};

static int draw_background_labor_advisor(void)
{
    outer_panel_draw(0, 0, 40, 26);
    // Labor advisor icon
    image_draw(image_group(GROUP_ADVISOR_ICONS), 10, 10);
    // Labor Allocation
    lang_text_draw(50, 0, 60, 12, FONT_LARGE_BLACK);
    // Priority/Sector/Need/Have
    lang_text_draw(50, 21, 60, 56, FONT_SMALL_PLAIN);
    lang_text_draw(50, 22, 170, 56, FONT_SMALL_PLAIN);
    lang_text_draw(50, 23, 400, 56, FONT_SMALL_PLAIN);
    lang_text_draw(50, 24, 500, 56, FONT_SMALL_PLAIN);
    inner_panel_draw(32, 70, 36, 15);
    // Employed workforce
    int width = text_draw_number(city_data.labor.workers_employed, 0, 0, 32, 320, FONT_NORMAL_BLACK);
    lang_text_draw(50, 12, 32 + width, 320, FONT_NORMAL_BLACK);
    // Unemployed workforce
    width = text_draw_number(city_data.labor.workers_unemployed, 0, 0, 320, 320, FONT_NORMAL_BLACK);
    width += lang_text_draw(50, 13, 320 + width, 320, FONT_NORMAL_BLACK);
    text_draw_number(city_data.labor.unemployment_percentage, 0, "%)", 314 + width, 320, FONT_NORMAL_BLACK);
    // Wages panel
    inner_panel_draw(64, 350, 32, 2);
    lang_text_draw(50, 14, 80, 359, FONT_NORMAL_WHITE);
    text_draw_number(city_data.labor.wages, 0, 0, 222, 359, FONT_NORMAL_WHITE);
    lang_text_draw(50, 15, 254, 359, FONT_NORMAL_WHITE);
    lang_text_draw(50, 18, 330, 359, FONT_NORMAL_WHITE);
    text_draw_number(city_data.labor.wages_rome, 0, ")", 430, 359, FONT_NORMAL_WHITE);
    // Estimated annual bill
    lang_text_draw(50, 19, 64, 388, FONT_NORMAL_BLACK);
    text_draw_money(city_data.finance.estimated_wages, 255, 388, FONT_NORMAL_BLACK);
    // outer panel draw height
    return 26;
}

static void draw_foreground_labor_advisor(void)
{
    // Industry stats
    for (int i = 0; i < 9; i++) {
        button_border_draw(40, 77 + 25 * i, 560, 22, i == focus_button_id_labor_advisor - 1);
        struct labor_category_data_t *labor_category = &city_data.labor.categories[i];
        if (labor_category->priority) {
            image_draw(image_group(GROUP_LABOR_PRIORITY_LOCK), 70, 80 + 25 * i);
            text_draw_number(labor_category->priority, 0, 0, 90, 82 + 25 * i, FONT_NORMAL_WHITE);
        }
        lang_text_draw(50, i + 1, 170, 82 + 25 * i, FONT_NORMAL_WHITE);
        text_draw_number(labor_category->workers_needed, 0, 0, 410, 82 + 25 * i, FONT_NORMAL_WHITE);
        if (labor_category->workers_needed > labor_category->workers_allocated) {
            text_draw_number(labor_category->workers_allocated, 0, 0, 510, 82 + 25 * i, FONT_NORMAL_RED);
        } else {
            text_draw_number(labor_category->workers_allocated, 0, 0, 510, 82 + 25 * i, FONT_NORMAL_WHITE);
        }
    }

    arrow_buttons_draw(0, 0, wage_arrow_buttons, 2);
}

static int handle_mouse_labor_advisor(const struct mouse_t *m)
{
    if (generic_buttons_handle_mouse(m, 0, 0, category_buttons, sizeof(category_buttons) / sizeof(struct generic_button_t), &focus_button_id_labor_advisor)) {
        return 1;
    }
    if (arrow_buttons_handle_mouse(m, 0, 0, wage_arrow_buttons, 2, &arrow_button_focus_labor_advisor)) {
        return 1;
    }
    return 0;
}

static struct advisor_window_type_t *window_advisor_labor(void)
{
    static struct advisor_window_type_t window = {
        draw_background_labor_advisor,
        draw_foreground_labor_advisor,
        handle_mouse_labor_advisor,
    };
    return &window;
}

static int get_legion_formation_by_index_rank(int legion_index)
{
    int index = 0;
    for (int i = 0; i < MAX_LEGIONS; i++) {
        if (legion_formations[i].in_use) {
            if (index == legion_index) {
                return i;
            }
            index++;
        }
    }
    return -1;
}

static void button_go_to_legion(int legion_id, __attribute__((unused)) int param2)
{
    struct formation_t *m = &legion_formations[get_legion_formation_by_index_rank(legion_id)];
    city_view_go_to_grid_offset(map_grid_offset(m->standard_x, m->standard_y));
    window_city_show();
}

static void button_return_to_fort(int legion_id, __attribute__((unused)) int param2)
{
    struct formation_t *m = &legion_formations[get_legion_formation_by_index_rank(legion_id)];
    if (!m->in_distant_battle && !m->is_at_rest) {
        return_legion_formation_home(m);
        window_invalidate();
    }
}

static void button_empire_service(int legion_id, __attribute__((unused)) int param2)
{
    struct formation_t *m = &legion_formations[get_legion_formation_by_index_rank(legion_id)];
    if (!m->in_distant_battle) {
        m->empire_service = m->empire_service ? 0 : 1;
        window_invalidate();
    }
}

static struct generic_button_t fort_buttons[] = {
    {400, 83, 30, 30, button_go_to_legion, button_none, 0, 0},
    {480, 83, 30, 30, button_return_to_fort, button_none, 0, 0},
    {560, 83, 30, 30, button_empire_service, button_none, 0, 0},
    {400, 127, 30, 30, button_go_to_legion, button_none, 1, 0},
    {480, 127, 30, 30, button_return_to_fort, button_none, 1, 0},
    {560, 127, 30, 30, button_empire_service, button_none, 1, 0},
    {400, 171, 30, 30, button_go_to_legion, button_none, 2, 0},
    {480, 171, 30, 30, button_return_to_fort, button_none, 2, 0},
    {560, 171, 30, 30, button_empire_service, button_none, 2, 0},
    {400, 215, 30, 30, button_go_to_legion, button_none, 3, 0},
    {480, 215, 30, 30, button_return_to_fort, button_none, 3, 0},
    {560, 215, 30, 30, button_empire_service, button_none, 3, 0},
    {400, 259, 30, 30, button_go_to_legion, button_none, 4, 0},
    {480, 259, 30, 30, button_return_to_fort, button_none, 4, 0},
    {560, 259, 30, 30, button_empire_service, button_none, 4, 0},
    {400, 303, 30, 30, button_go_to_legion, button_none, 5, 0},
    {480, 303, 30, 30, button_return_to_fort, button_none, 5, 0},
    {560, 303, 30, 30, button_empire_service, button_none, 5, 0},
};

static int draw_background_military_advisor(void)
{
    outer_panel_draw(0, 0, 40, MILITARY_ADVISOR_HEIGHT);
    // Military advisor icon
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 1, 10, 10);
    // Morale
    lang_text_draw(138, 36, 290, 58, FONT_SMALL_PLAIN);
    // Legion status
    lang_text_draw(51, 0, 60, 12, FONT_LARGE_BLACK);
    // Button headers
    lang_text_draw(51, 1, 390, 43, FONT_SMALL_PLAIN);
    lang_text_draw(51, 2, 390, 58, FONT_SMALL_PLAIN);
    lang_text_draw(51, 3, 470, 43, FONT_SMALL_PLAIN);
    lang_text_draw(51, 4, 470, 58, FONT_SMALL_PLAIN);
    lang_text_draw(51, 5, 550, 43, FONT_SMALL_PLAIN);
    lang_text_draw(51, 6, 550, 58, FONT_SMALL_PLAIN);

    int enemy_text_id;
    if (city_data.figure.enemies) {
        enemy_text_id = 10;
    } else if (city_data.figure.imperial_soldiers) {
        enemy_text_id = 11;
    } else if (scenario.invasion_upcoming) {
        enemy_text_id = 9;
    } else {
        enemy_text_id = 8;
    }
    int distant_battle_text_id;
    if (city_data.distant_battle.roman_months_to_travel_back) {
        distant_battle_text_id = 15;
    } else if (city_data.distant_battle.roman_months_to_travel_forth) {
        distant_battle_text_id = 14;
    } else if (city_data.distant_battle.months_until_battle) {
        distant_battle_text_id = 13;
    } else {
        distant_battle_text_id = 12;
    }

    inner_panel_draw(32, 70, 36, 17);
    if (city_data.military.total_legions) {
        int draw_index = 0;
        for (int i = 0; i < MAX_LEGIONS; i++) {
            struct formation_t *m = &legion_formations[i];
            if (m->in_use) {
                button_border_draw(38, 33 + 44 * (draw_index + 1), 560, 40, 0);
                image_draw(image_group(GROUP_FIGURE_FORT_STANDARD_ICONS) + m->id, 48, 38 + 44 * (draw_index + 1));
                lang_text_draw(138, m->id, 100, 39 + 44 * (draw_index + 1), FONT_NORMAL_WHITE);
                int width = text_draw_number(m->num_figures, ' ', "", 100, 56 + 44 * (draw_index + 1), FONT_NORMAL_GREEN);
                switch (m->figure_type) {
                    case FIGURE_FORT_LEGIONARY:
                        lang_text_draw(138, 33, 100 + width, 56 + 44 * (draw_index + 1), FONT_NORMAL_GREEN);
                        break;
                    case FIGURE_FORT_MOUNTED:
                        lang_text_draw(138, 34, 100 + width, 56 + 44 * (draw_index + 1), FONT_NORMAL_GREEN);
                        break;
                    case FIGURE_FORT_JAVELIN:
                        lang_text_draw(138, 35, 100 + width, 56 + 44 * (draw_index + 1), FONT_NORMAL_GREEN);
                        break;
                }
                lang_text_draw_centered(138, 37 + m->morale / 5, 240, 47 + 44 * (draw_index + 1), 150, FONT_NORMAL_GREEN);

                int image_id = image_group(GROUP_FORT_ICONS);
                button_border_draw(400, 39 + 44 * (draw_index + 1), 30, 30, 0);
                image_draw(image_id, 403, 42 + 44 * (draw_index + 1));

                button_border_draw(480, 39 + 44 * (draw_index + 1), 30, 30, 0);
                if (m->is_at_rest || m->in_distant_battle) {
                    image_draw(image_id + 2, 483, 42 + 44 * (draw_index + 1));
                } else {
                    image_draw(image_id + 1, 483, 42 + 44 * (draw_index + 1));
                }

                button_border_draw(560, 39 + 44 * (draw_index + 1), 30, 30, 0);
                if (m->empire_service) {
                    image_draw(image_id + 3, 563, 42 + 44 * (draw_index + 1));
                } else {
                    image_draw(image_id + 4, 563, 42 + 44 * (draw_index + 1));
                }
                draw_index++;
            }
        }
        // x soldiers in y legions
        image_draw(image_group(GROUP_BULLET), 60, 349);
        int total_soldiers = 0;
        for (int i = 0; i < MAX_LEGIONS; i++) {
            if (legion_formations[i].in_use) {
                total_soldiers += legion_formations[i].num_figures;
            }
        }
        int width = lang_text_draw_amount(8, 46, total_soldiers, 80, 348, FONT_NORMAL_BLACK);
        width += lang_text_draw(51, 7, 80 + width, 348, FONT_NORMAL_BLACK);
        lang_text_draw_amount(8, 48, city_data.military.total_legions, 80 + width, 348, FONT_NORMAL_BLACK);
        // Enemy threat status
        image_draw(image_group(GROUP_BULLET), 60, 369);
        lang_text_draw(51, enemy_text_id, 80, 368, FONT_NORMAL_BLACK);
        // Distant battle status
        image_draw(image_group(GROUP_BULLET), 60, 389);
        lang_text_draw(51, distant_battle_text_id, 80, 388, FONT_NORMAL_BLACK);
    } else {
        // You have no legions to command
        lang_text_draw_multiline(51, 16, 64, 200, 496, FONT_NORMAL_GREEN);
        // Enemy threat status
        image_draw(image_group(GROUP_BULLET), 60, 359);
        lang_text_draw(51, enemy_text_id, 80, 358, FONT_NORMAL_BLACK);
        // Distant battle status
        image_draw(image_group(GROUP_BULLET), 60, 379);
        lang_text_draw(51, distant_battle_text_id, 80, 378, FONT_NORMAL_BLACK);
    }

    return MILITARY_ADVISOR_HEIGHT;
}

static void draw_foreground_military_advisor(void)
{
    for (int i = 0; i < city_data.military.total_legions; i++) {
        button_border_draw(400, 83 + 44 * i, 30, 30, focus_button_id_military_advisor == 3 * i + 1);
        button_border_draw(480, 83 + 44 * i, 30, 30, focus_button_id_military_advisor == 3 * i + 2);
        button_border_draw(560, 83 + 44 * i, 30, 30, focus_button_id_military_advisor == 3 * i + 3);
    }
}

static int handle_mouse_military_advisor(const struct mouse_t *m)
{
    return generic_buttons_handle_mouse(m, 0, 0, fort_buttons, 3 * city_data.military.total_legions, &focus_button_id_military_advisor);
}

static struct advisor_window_type_t *window_advisor_military(void)
{
    static struct advisor_window_type_t window = {
        draw_background_military_advisor,
        draw_foreground_military_advisor,
        handle_mouse_military_advisor,
    };
    return &window;
}

static void button_graph(int param1, __attribute__((unused)) int param2)
{
    int new_order;
    switch (city_data.population.graph_order) {
        default:
        case 0:
            new_order = param1 ? 5 : 2;
            break;
        case 1:
            new_order = param1 ? 3 : 4;
            break;
        case 2:
            new_order = param1 ? 4 : 0;
            break;
        case 3:
            new_order = param1 ? 1 : 5;
            break;
        case 4:
            new_order = param1 ? 2 : 1;
            break;
        case 5:
            new_order = param1 ? 0 : 3;
            break;
    }
    city_data.population.graph_order = new_order;
    window_invalidate();
}

static struct generic_button_t graph_buttons[] = {
    {509,  61, 104, 55, button_graph, button_none, 0, 0},
    {509, 161, 104, 55, button_graph, button_none, 1, 0},
};

static void get_y_axis(int max_value, int *y_max, int *y_shift)
{
    int max = 1;
    int shift = -1;
    int value = max_value > 0 ? (max_value - 1) / 100 : 0;
    while (value) {
        max <<= 1;
        shift++;
        value >>= 1;
    }
    *y_max = max * 100;
    *y_shift = shift;
}

static int city_population_at_month(int max_months, int month)
{
    int start_offset = 0;
    if (city_data.population.monthly.count > max_months) {
        start_offset = city_data.population.monthly.count + 2400 - max_months;
    }
    int index = (start_offset + month) % 2400;
    return city_data.population.monthly.values[index];
}

static void draw_history_graph(int full_size, int x, int y)
{
    int max_months;
    if (city_data.population.monthly.count <= 20) {
        max_months = 20;
    } else if (city_data.population.monthly.count <= 40) {
        max_months = 40;
    } else if (city_data.population.monthly.count <= 100) {
        max_months = 100;
    } else if (city_data.population.monthly.count <= 200) {
        max_months = 200;
    } else {
        max_months = 400;
    }
    if (!full_size) {
        if (max_months <= 40) {
            max_months = 20;
        } else {
            max_months = 100;
        }
    }
    // determine max value
    int max_value = 0;
    for (int m = 0; m < max_months; m++) {
        int value = city_population_at_month(max_months, m);
        if (value > max_value) {
            max_value = value;
        }
    }
    int y_max, y_shift;
    get_y_axis(max_value, &y_max, &y_shift);
    if (full_size) {
        // y axis
        text_draw_number_centered(y_max, x - 66, y - 3, 60, FONT_SMALL_PLAIN);
        text_draw_number_centered(y_max / 2, x - 66, y + 96, 60, FONT_SMALL_PLAIN);
        text_draw_number_centered(0, x - 66, y + 196, 60, FONT_SMALL_PLAIN);
        // x axis
        int start_month, start_year, end_month, end_year;
        if (city_data.population.monthly.count > max_months) {
            end_month = game_time_month() - 1;
            end_year = game_time_year();
            if (end_month < 0) {
                end_year -= 1;
            }
            start_month = 11 - (max_months % 12);
            start_year = end_year - max_months / 12;
        } else {
            start_month = 0;
            start_year = scenario.start_year;
            end_month = (max_months + start_month) % 12;
            end_year = (max_months + start_month) / 12 + start_year;
        }

        int width = lang_text_draw(25, start_month, x - 20, y + 210, FONT_SMALL_PLAIN);
        lang_text_draw_year(start_year, x + width - 20, y + 210, FONT_SMALL_PLAIN);

        width = lang_text_draw(25, end_month, x + 380, y + 210, FONT_SMALL_PLAIN);
        lang_text_draw_year(end_year, x + width + 380, y + 210, FONT_SMALL_PLAIN);
    }

    if (full_size) {
        graphics_set_clip_rectangle(0, 0, 640, y + 200);
        for (int m = 0; m < max_months; m++) {
            int pop = city_population_at_month(max_months, m);
            int val;
            if (y_shift == -1) {
                val = 2 * pop;
            } else {
                val = pop >> y_shift;
            }
            if (val > 0) {
                switch (max_months) {
                    case 20:
                        image_draw(image_group(GROUP_POPULATION_GRAPH_BAR), x + 20 * m, y + 200 - val);
                        break;
                    case 40:
                        image_draw(image_group(GROUP_POPULATION_GRAPH_BAR) + 1, x + 10 * m, y + 200 - val);
                        break;
                    case 100:
                        image_draw(image_group(GROUP_POPULATION_GRAPH_BAR) + 2, x + 4 * m, y + 200 - val);
                        break;
                    case 200:
                        image_draw(image_group(GROUP_POPULATION_GRAPH_BAR) + 3, x + 2 * m, y + 200 - val);
                        break;
                    default:
                        graphics_draw_vertical_line(x + m, y + 200 - val, y + 199, COLOR_RED);
                        break;
                }
            }
        }
        graphics_reset_clip_rectangle();
    } else {
        y_shift += 2;
        for (int m = 0; m < max_months; m++) {
            int val = city_population_at_month(max_months, m) >> y_shift;
            if (val > 0) {
                if (max_months == 20) {
                    graphics_fill_rect(x + m, y + 50 - val, 4, val + 1, COLOR_RED);
                } else {
                    graphics_draw_vertical_line(x + m, y + 50 - val, y + 50, COLOR_RED);
                }
            }
        }
    }
}

static void draw_census_graph(int full_size, int x, int y)
{
    int max_value = 0;
    for (int i = 0; i < 100; i++) {
        if (city_data.population.at_age[i] > max_value) {
            max_value = city_data.population.at_age[i];
        }
    }
    int y_max, y_shift;
    get_y_axis(max_value, &y_max, &y_shift);
    if (full_size) {
        // y axis
        text_draw_number_centered(y_max, x - 66, y - 3, 60, FONT_SMALL_PLAIN);
        text_draw_number_centered(y_max / 2, x - 66, y + 96, 60, FONT_SMALL_PLAIN);
        text_draw_number_centered(0, x - 66, y + 196, 60, FONT_SMALL_PLAIN);
        // x axis
        for (int i = 0; i <= 10; i++) {
            text_draw_number_centered(i * 10, x + 40 * i - 22, y + 210, 40, FONT_SMALL_PLAIN);
        }
    }

    if (full_size) {
        graphics_set_clip_rectangle(0, 0, 640, y + 200);
        for (int i = 0; i < 100; i++) {
            int val;
            if (y_shift == -1) {
                val = 2 * city_data.population.at_age[i];
            } else {
                val = city_data.population.at_age[i] >> y_shift;
            }
            if (val > 0) {
                image_draw(image_group(GROUP_POPULATION_GRAPH_BAR) + 2, x + 4 * i, y + 200 - val);
            }
        }
        graphics_reset_clip_rectangle();
    } else {
        y_shift += 2;
        for (int i = 0; i < 100; i++) {
            int val = city_data.population.at_age[i] >> y_shift;
            if (val > 0) {
                graphics_draw_vertical_line(x + i, y + 50 - val, y + 50, COLOR_RED);
            }
        }
    }
}

static void draw_society_graph(int full_size, int x, int y)
{
    int max_value = 0;
    for (int i = 0; i < 20; i++) {
        if (city_data.population.at_level[i] > max_value) {
            max_value = city_data.population.at_level[i];
        }
    }
    int y_max, y_shift;
    get_y_axis(max_value, &y_max, &y_shift);
    if (full_size) {
        // y axis
        text_draw_number_centered(y_max, x - 66, y - 3, 60, FONT_SMALL_PLAIN);
        text_draw_number_centered(y_max / 2, x - 66, y + 96, 60, FONT_SMALL_PLAIN);
        text_draw_number_centered(0, x - 66, y + 196, 60, FONT_SMALL_PLAIN);
        // x axis
        lang_text_draw_centered(55, 9, x - 80, y + 210, 200, FONT_SMALL_PLAIN);
        lang_text_draw_centered(55, 10, x + 280, y + 210, 200, FONT_SMALL_PLAIN);
    }

    if (full_size) {
        graphics_set_clip_rectangle(0, 0, 640, y + 200);
        for (int i = 0; i < 20; i++) {
            int val;
            if (y_shift == -1) {
                val = 2 * city_data.population.at_level[i];
            } else {
                val = city_data.population.at_level[i] >> y_shift;
            }
            if (val > 0) {
                image_draw(image_group(GROUP_POPULATION_GRAPH_BAR), x + 20 * i, y + 200 - val);
            }
        }
        graphics_reset_clip_rectangle();
    } else {
        y_shift += 2;
        for (int i = 0; i < 20; i++) {
            int val = city_data.population.at_level[i] >> y_shift;
            if (val > 0) {
                graphics_fill_rect(x + 5 * i, y + 50 - val, 4, val + 1, COLOR_RED);
            }
        }
    }
}

static int draw_background_population_advisor(void)
{
    outer_panel_draw(0, 0, 40, POPULATION_ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 5, 10, 10);

    // Title: depends on big graph shown
    if (city_data.population.graph_order < 2) {
        lang_text_draw(55, 0, 60, 12, FONT_LARGE_BLACK);
    } else if (city_data.population.graph_order < 4) {
        lang_text_draw(55, 1, 60, 12, FONT_LARGE_BLACK);
    } else {
        lang_text_draw(55, 2, 60, 12, FONT_LARGE_BLACK);
    }
    image_draw(image_group(GROUP_PANEL_WINDOWS) + 14, 62, 60);
    int big_text, top_text, bot_text;
    void (*big_graph)(int, int, int);
    void (*top_graph)(int, int, int);
    void (*bot_graph)(int, int, int);
    switch (city_data.population.graph_order) {
        default:
        case 0:
            big_text = 6;
            top_text = 4;
            bot_text = 5;
            big_graph = draw_history_graph;
            top_graph = draw_census_graph;
            bot_graph = draw_society_graph;
            break;
        case 1:
            big_text = 6;
            top_text = 5;
            bot_text = 4;
            big_graph = draw_history_graph;
            top_graph = draw_society_graph;
            bot_graph = draw_census_graph;
            break;
        case 2:
            big_text = 7;
            top_text = 3;
            bot_text = 5;
            big_graph = draw_census_graph;
            top_graph = draw_history_graph;
            bot_graph = draw_society_graph;
            break;
        case 3:
            big_text = 7;
            top_text = 5;
            bot_text = 3;
            big_graph = draw_census_graph;
            top_graph = draw_society_graph;
            bot_graph = draw_history_graph;
            break;
        case 4:
            big_text = 8;
            top_text = 3;
            bot_text = 4;
            big_graph = draw_society_graph;
            top_graph = draw_history_graph;
            bot_graph = draw_census_graph;
            break;
        case 5:
            big_text = 8;
            top_text = 4;
            bot_text = 3;
            big_graph = draw_society_graph;
            top_graph = draw_census_graph;
            bot_graph = draw_history_graph;
            break;
    }
    lang_text_draw_centered(55, big_text, 60, 295, 400, FONT_NORMAL_BLACK);
    lang_text_draw_centered(55, top_text, 504, 120, 100, FONT_NORMAL_BLACK);
    lang_text_draw_centered(55, bot_text, 504, 220, 100, FONT_NORMAL_BLACK);
    big_graph(1, 70, 64);
    top_graph(0, 511, 63);
    bot_graph(0, 511, 163);
    // food/migration info panel
    inner_panel_draw(48, 336, 34, 5);
    int image_id = image_group(GROUP_BULLET);
    int width;
    image_draw(image_id, 56, 344);
    image_draw(image_id, 56, 362);
    image_draw(image_id, 56, 380);
    image_draw(image_id, 56, 398);
    // food stores
    if (scenario.rome_supplies_wheat) {
        lang_text_draw(55, 11, 75, 342, FONT_NORMAL_WHITE);
    } else {
        width = lang_text_draw_amount(8, 6, city_data.resource.granaries.operating, 75, 342, FONT_NORMAL_WHITE);
        if (city_data.resource.food_supply_months > 0) {
            width += lang_text_draw(55, 12, 75 + width, 342, FONT_NORMAL_WHITE);
            lang_text_draw_amount(8, 4, city_data.resource.food_supply_months, 75 + width, 342, FONT_NORMAL_WHITE);
        } else if (city_data.resource.granary_total_stored > city_data.resource.food_needed_per_month / 2) {
            lang_text_draw(55, 13, 75 + width, 342, FONT_NORMAL_WHITE);
        } else if (city_data.resource.granary_total_stored > 0) {
            lang_text_draw(55, 15, 75 + width, 342, FONT_NORMAL_WHITE);
        } else {
            lang_text_draw(55, 14, 75 + width, 342, FONT_NORMAL_WHITE);
        }
    }
    // food types eaten
    width = lang_text_draw(55, 16, 75, 360, FONT_NORMAL_WHITE);
    text_draw_number(city_data.resource.food_types_available, '@', " ", 75 + width, 360, FONT_NORMAL_WHITE);
    // immigration
    if (city_data.migration.newcomers >= 5) {
        lang_text_draw(55, 24, 75, 378, FONT_NORMAL_WHITE);
        width = text_draw_number(city_data.migration.newcomers, '@', " ", 75, 396, FONT_NORMAL_WHITE);
        lang_text_draw(55, 17, 75 + width, 396, FONT_NORMAL_WHITE);
    } else if (city_migration_no_room_for_immigrants()) {
        lang_text_draw(55, 24, 75, 378, FONT_NORMAL_WHITE);
        lang_text_draw(55, 19, 75, 396, FONT_NORMAL_WHITE);
    } else if (city_data.migration.percentage < 80) {
        lang_text_draw(55, 25, 75, 378, FONT_NORMAL_WHITE);
        int text_id;
        switch (city_data.migration.no_immigration_cause) {
            case NO_IMMIGRATION_LOW_WAGES: text_id = 20; break;
            case NO_IMMIGRATION_NO_JOBS: text_id = 21; break;
            case NO_IMMIGRATION_NO_FOOD: text_id = 22; break;
            case NO_IMMIGRATION_HIGH_TAXES: text_id = 23; break;
            case NO_IMMIGRATION_MANY_TENTS: text_id = 31; break;
            case NO_IMMIGRATION_LOW_MOOD: text_id = 32; break;
            default: text_id = 0; break;
        }
        if (text_id) {
            lang_text_draw(55, text_id, 75, 396, FONT_NORMAL_WHITE);
        }
    } else {
        lang_text_draw(55, 24, 75, 378, FONT_NORMAL_WHITE);
        width = text_draw_number(city_data.migration.newcomers, '@', " ", 75, 396, FONT_NORMAL_WHITE);
        if (city_data.migration.newcomers == 1) {
            lang_text_draw(55, 18, 75 + width, 396, FONT_NORMAL_WHITE);
        } else {
            lang_text_draw(55, 17, 75 + width, 396, FONT_NORMAL_WHITE);
        }
    }
    return POPULATION_ADVISOR_HEIGHT;
}

static void draw_foreground_population_advisor(void)
{
    if (focus_button_id_population_advisor == 0) {
        button_border_draw(507, 60, 106, 57, 0);
        button_border_draw(507, 160, 106, 57, 0);
    } else if (focus_button_id_population_advisor == 1) {
        button_border_draw(507, 60, 106, 57, 1);
        button_border_draw(507, 160, 106, 57, 0);
    } else if (focus_button_id_population_advisor == 2) {
        button_border_draw(507, 60, 106, 57, 0);
        button_border_draw(507, 160, 106, 57, 1);
    }
}

static int handle_mouse_population_advisor(const struct mouse_t *m)
{
    return generic_buttons_handle_mouse(m, 0, 0, graph_buttons, 2, &focus_button_id_population_advisor);
}

static struct advisor_window_type_t *window_advisor_population(void)
{
    static struct advisor_window_type_t window = {
        draw_background_population_advisor,
        draw_foreground_population_advisor,
        handle_mouse_population_advisor,
    };
    return &window;
}

static void button_rating(int rating, __attribute__((unused)) int param2)
{
    city_data.ratings.selected = rating;
    window_invalidate();
}

static struct generic_button_t rating_buttons[] = {
    { 80, 286, 110, 66, button_rating, button_none, SELECTED_RATING_CULTURE, 0},
    {200, 286, 110, 66, button_rating, button_none, SELECTED_RATING_PROSPERITY, 0},
    {320, 286, 110, 66, button_rating, button_none, SELECTED_RATING_PEACE, 0},
    {440, 286, 110, 66, button_rating, button_none, SELECTED_RATING_FAVOR, 0},
};

static void draw_rating_column(int x_offset, int y_offset, int value, int has_reached)
{
    int image_base = image_group(GROUP_RATINGS_COLUMN);
    int y = y_offset - image_get(image_base)->height;
    image_draw(image_base, x_offset, y);
    for (int i = 0; i < 2 * value; i++) {
        image_draw(image_base + 1, x_offset + 11, --y);
    }
    if (value >= 30 && has_reached) {
        image_draw(image_base + 2, x_offset - 6, y);
    }
}

static int city_rating_selected_explanation(void)
{
    switch (city_data.ratings.selected) {
        case SELECTED_RATING_CULTURE:
            return city_data.ratings.culture_explanation;
        case SELECTED_RATING_PROSPERITY:
            return city_data.ratings.prosperity_explanation;
        case SELECTED_RATING_PEACE:
            return city_data.ratings.peace_explanation;
        case SELECTED_RATING_FAVOR:
            return city_data.ratings.favor_explanation;
        default:
            return 0;
    }
}

static int draw_background_ratings_advisor(void)
{
    outer_panel_draw(0, 0, 40, RATINGS_ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 3, 10, 10);
    int width = lang_text_draw(53, 0, 60, 12, FONT_LARGE_BLACK);
    if (scenario.population_win_criteria.enabled) {
        width += lang_text_draw(53, 6, 80 + width, 17, FONT_NORMAL_BLACK);
        text_draw_number(scenario.population_win_criteria.goal, '@', ")", 80 + width, 17, FONT_NORMAL_BLACK);
    } else {
        lang_text_draw(53, 7, 80 + width, 17, FONT_NORMAL_BLACK);
    }
    image_draw(image_group(GROUP_RATINGS_BACKGROUND), 60, 48);
    // culture
    button_border_draw(80, 286, 110, 66, focus_button_id_ratings_advisor == SELECTED_RATING_CULTURE);
    lang_text_draw_centered(53, 1, 80, 294, 110, FONT_NORMAL_BLACK);
    text_draw_number_centered(city_data.ratings.culture, 80, 309, 100, FONT_LARGE_BLACK);
    width = text_draw_number(scenario.culture_win_criteria.enabled ? scenario.culture_win_criteria.goal : 0,
            '@', " ", 85, 334, FONT_NORMAL_BLACK);
    lang_text_draw(53, 5, 85 + width, 334, FONT_NORMAL_BLACK);
    int has_reached = !scenario.culture_win_criteria.enabled || city_data.ratings.culture >= scenario.culture_win_criteria.goal;
    draw_rating_column(110, 274, city_data.ratings.culture, has_reached);
    // prosperity
    button_border_draw(200, 286, 110, 66, focus_button_id_ratings_advisor == SELECTED_RATING_PROSPERITY);
    lang_text_draw_centered(53, 2, 200, 294, 110, FONT_NORMAL_BLACK);
    text_draw_number_centered(city_data.ratings.prosperity, 200, 309, 100, FONT_LARGE_BLACK);
    width = text_draw_number(scenario.prosperity_win_criteria.enabled ? scenario.prosperity_win_criteria.goal : 0,
            '@', " ", 205, 334, FONT_NORMAL_BLACK);
    lang_text_draw(53, 5, 205 + width, 334, FONT_NORMAL_BLACK);
    has_reached = !scenario.prosperity_win_criteria.enabled || city_data.ratings.prosperity >= scenario.prosperity_win_criteria.goal;
    draw_rating_column(230, 274, city_data.ratings.prosperity, has_reached);
    // peace
    button_border_draw(320, 286, 110, 66, focus_button_id_ratings_advisor == SELECTED_RATING_PEACE);
    lang_text_draw_centered(53, 3, 320, 294, 110, FONT_NORMAL_BLACK);
    text_draw_number_centered(city_data.ratings.peace, 320, 309, 100, FONT_LARGE_BLACK);
    width = text_draw_number(scenario.peace_win_criteria.enabled ? scenario.peace_win_criteria.goal : 0,
            '@', " ", 325, 334, FONT_NORMAL_BLACK);
    lang_text_draw(53, 5, 325 + width, 334, FONT_NORMAL_BLACK);
    has_reached = !scenario.peace_win_criteria.enabled || city_data.ratings.peace >= scenario.peace_win_criteria.goal;
    draw_rating_column(350, 274, city_data.ratings.peace, has_reached);
    // favor
    button_border_draw(440, 286, 110, 66, focus_button_id_ratings_advisor == SELECTED_RATING_FAVOR);
    lang_text_draw_centered(53, 4, 440, 294, 110, FONT_NORMAL_BLACK);
    text_draw_number_centered(city_data.ratings.favor, 440, 309, 100, FONT_LARGE_BLACK);
    width = text_draw_number(scenario.favor_win_criteria.enabled ? scenario.favor_win_criteria.goal : 0,
            '@', " ", 445, 334, FONT_NORMAL_BLACK);
    lang_text_draw(53, 5, 445 + width, 334, FONT_NORMAL_BLACK);
    has_reached = !scenario.favor_win_criteria.enabled || city_data.ratings.favor >= scenario.favor_win_criteria.goal;
    draw_rating_column(470, 274, city_data.ratings.favor, has_reached);
    // bottom info box
    inner_panel_draw(64, 356, 32, 4);
    switch (city_data.ratings.selected) {
        case SELECTED_RATING_CULTURE:
            lang_text_draw(53, 1, 72, 359, FONT_NORMAL_WHITE);
            if (city_data.ratings.culture <= 90) {
                lang_text_draw_multiline(53, 9 + city_rating_selected_explanation(),
                    72, 374, 496, FONT_NORMAL_WHITE);
            } else {
                lang_text_draw_multiline(53, 50, 72, 374, 496, FONT_NORMAL_WHITE);
            }
            break;
        case SELECTED_RATING_PROSPERITY:
            lang_text_draw(53, 2, 72, 359, FONT_NORMAL_WHITE);
            if (city_data.ratings.prosperity <= 90) {
                lang_text_draw_multiline(53, 16 + city_rating_selected_explanation(),
                    72, 374, 496, FONT_NORMAL_WHITE);
            } else {
                lang_text_draw_multiline(53, 51, 72, 374, 496, FONT_NORMAL_WHITE);
            }
            break;
        case SELECTED_RATING_PEACE:
            lang_text_draw(53, 3, 72, 359, FONT_NORMAL_WHITE);
            if (city_data.ratings.peace <= 90) {
                lang_text_draw_multiline(53, 41 + city_rating_selected_explanation(),
                    72, 374, 496, FONT_NORMAL_WHITE);
            } else {
                lang_text_draw_multiline(53, 52, 72, 374, 496, FONT_NORMAL_WHITE);
            }
            break;
        case SELECTED_RATING_FAVOR:
            lang_text_draw(53, 4, 72, 359, FONT_NORMAL_WHITE);
            if (city_data.ratings.favor <= 90) {
                lang_text_draw_multiline(53, 27 + city_rating_selected_explanation(),
                    72, 374, 496, FONT_NORMAL_WHITE);
            } else {
                lang_text_draw_multiline(53, 53, 72, 374, 496, FONT_NORMAL_WHITE);
            }
            break;
        default:
            lang_text_draw_centered(53, 8, 72, 380, 496, FONT_NORMAL_WHITE);
            break;
    }

    return RATINGS_ADVISOR_HEIGHT;
}

static void draw_foreground_ratings_advisor(void)
{
    button_border_draw(80, 286, 110, 66, focus_button_id_ratings_advisor == SELECTED_RATING_CULTURE);
    button_border_draw(200, 286, 110, 66, focus_button_id_ratings_advisor == SELECTED_RATING_PROSPERITY);
    button_border_draw(320, 286, 110, 66, focus_button_id_ratings_advisor == SELECTED_RATING_PEACE);
    button_border_draw(440, 286, 110, 66, focus_button_id_ratings_advisor == SELECTED_RATING_FAVOR);
}

static int handle_mouse_ratings_advisor(const struct mouse_t *m)
{
    return generic_buttons_handle_mouse(m, 0, 0, rating_buttons, 4, &focus_button_id_ratings_advisor);
}

static struct advisor_window_type_t *window_advisor_ratings(void)
{
    static struct advisor_window_type_t window = {
        draw_background_ratings_advisor,
        draw_foreground_ratings_advisor,
        handle_mouse_ratings_advisor,
    };
    return &window;
}

static void draw_god_row(int god, int y_offset, int small_temple, int large_temple)
{
    lang_text_draw(59, 11 + god, 40, y_offset, FONT_NORMAL_WHITE);
    lang_text_draw(59, 16 + god, 120, y_offset + 1, FONT_SMALL_PLAIN);
    text_draw_number_centered(building_count_total(small_temple), 230, y_offset, 50, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_total(large_temple), 290, y_offset, 50, FONT_NORMAL_WHITE);
    text_draw_number_centered(city_data.religion.gods[god].months_since_festival, 360, y_offset, 50, FONT_NORMAL_WHITE);
    int width = lang_text_draw(59, 32 + city_data.religion.gods[god].happiness / 10, 460, y_offset, FONT_NORMAL_WHITE);
    int bolts = city_data.religion.gods[god].wrath_bolts;
    for (int i = 0; i < bolts / 10; i++) {
        image_draw(image_group(GROUP_GOD_BOLT), 10 * i + width + 460, y_offset - 4);
    }
}

static int draw_background_religion_advisor(void)
{
    int height_blocks;
    height_blocks = 17;
    outer_panel_draw(0, 0, 40, height_blocks);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 9, 10, 10);
    lang_text_draw(59, 0, 60, 12, FONT_LARGE_BLACK);
    // table header
    lang_text_draw(59, 5, 270, 32, FONT_SMALL_PLAIN);
    lang_text_draw(59, 1, 240, 46, FONT_SMALL_PLAIN);
    lang_text_draw(59, 2, 300, 46, FONT_SMALL_PLAIN);
    lang_text_draw(59, 3, 450, 46, FONT_SMALL_PLAIN);
    lang_text_draw(59, 6, 370, 18, FONT_SMALL_PLAIN);
    lang_text_draw(59, 9, 370, 32, FONT_SMALL_PLAIN);
    lang_text_draw(59, 7, 370, 46, FONT_SMALL_PLAIN);
    inner_panel_draw(32, 60, 36, 8);
    // god rows
    draw_god_row(GOD_CERES, 66, BUILDING_SMALL_TEMPLE_CERES, BUILDING_LARGE_TEMPLE_CERES);
    draw_god_row(GOD_NEPTUNE, 86, BUILDING_SMALL_TEMPLE_NEPTUNE, BUILDING_LARGE_TEMPLE_NEPTUNE);
    draw_god_row(GOD_MERCURY, 106, BUILDING_SMALL_TEMPLE_MERCURY, BUILDING_LARGE_TEMPLE_MERCURY);
    draw_god_row(GOD_MARS, 126, BUILDING_SMALL_TEMPLE_MARS, BUILDING_LARGE_TEMPLE_MARS);
    draw_god_row(GOD_VENUS, 146, BUILDING_SMALL_TEMPLE_VENUS, BUILDING_LARGE_TEMPLE_VENUS);
    // oracles
    lang_text_draw(59, 8, 40, 166, FONT_NORMAL_WHITE);
    text_draw_number_centered(building_count_total(BUILDING_ORACLE), 230, 166, 50, FONT_NORMAL_WHITE);
    city_gods_calculate_least_happy();
    struct house_demands_t *demands = &city_data.houses;
    int religion_advice = 5;
    if (city_data.religion.least_happy_god - 1 >= 0 && city_data.religion.gods[city_data.religion.least_happy_god - 1].wrath_bolts > 4) {
        religion_advice = 6 + city_data.religion.least_happy_god - 1;
    } else if (demands->religion == 1) {
        religion_advice = demands->requiring.religion ? 1 : 0;
    } else if (demands->religion == 2) {
        religion_advice = 2;
    } else if (demands->religion == 3) {
        religion_advice = 3;
    } else if (!demands->requiring.religion) {
        religion_advice = 4;
    } else if (city_data.religion.least_happy_god - 1 >= 0) {
        religion_advice = 6 + city_data.religion.least_happy_god - 1;
    }
    lang_text_draw_multiline(59, 21 + religion_advice, 60, 196, 512, FONT_NORMAL_BLACK);
    return height_blocks;
}

static struct advisor_window_type_t *window_advisor_religion(void)
{
    static struct advisor_window_type_t window = {
        draw_background_religion_advisor,
        0,
        0,
    };
    return &window;
}

static void button_help_resource_settings(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_message_dialog_show(MESSAGE_DIALOG_INDUSTRY, 0);
}

static void button_export_amount_adjust(int value, __attribute__((unused)) int param2)
{
    city_data.resource.export_over[resource_settings_data.resource] = calc_bound(city_data.resource.export_over[resource_settings_data.resource] + value, 0, 100);
}

static struct arrow_button_t resource_arrow_buttons[] = {
    {310, 207, 17, 24, button_export_amount_adjust, -1, 0, 0, 0},
    {334, 207, 15, 24, button_export_amount_adjust, 1, 0, 0, 0}
};

static struct image_button_t help_button_resource_settings[] = {
    {64, 297, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help_resource_settings, button_none, 0, 0, 1, 0, 0, 0},
};

static void button_toggle_trade(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    city_resource_cycle_trade_status(resource_settings_data.resource);
}

static void button_toggle_industry(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (building_count_industry_total(resource_settings_data.resource) > 0) {
        city_data.resource.mothballed[resource_settings_data.resource] = city_data.resource.mothballed[resource_settings_data.resource] ? 0 : 1;
    }
}

static void button_toggle_stockpile(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    city_resource_toggle_stockpiled(resource_settings_data.resource);
}

static struct generic_button_t resource_generic_buttons[] = {
    {104, 204, 432, 30, button_toggle_trade, button_none, 0, 0},
    {104, 240, 432, 30, button_toggle_industry, button_none, 0, 0},
    {104, 276, 432, 50, button_toggle_stockpile, button_none, 0, 0},
};

static void draw_foreground_resource_settings(void)
{
    graphics_in_dialog();
    outer_panel_draw(48, 100, 34, 15);
    // Resource image
    image_draw(resource_images[resource_settings_data.resource].icon_img_id + resource_image_offset(resource_settings_data.resource, RESOURCE_IMAGE_ICON), 64, 116);
    // Resource name
    text_draw_centered(resource_strings[resource_settings_data.resource], 48, 116, 544, FONT_LARGE_BLACK, COLOR_BLACK);
    int total_buildings = building_count_industry_total(resource_settings_data.resource);
    int active_buildings = building_count_industry_active(resource_settings_data.resource);
    if (total_buildings <= 0) {
        // No industries in the city
        lang_text_draw(54, 7, 109, 164, FONT_NORMAL_BLACK);
    } else if (city_data.resource.mothballed[resource_settings_data.resource]) {
        // [count of] mothballed industry/ies in the city
        int width = text_draw_number(total_buildings, 0, " ", 109, 164, FONT_NORMAL_BLACK);
        if (total_buildings == 1) {
            lang_text_draw(54, 10, 109 + width, 164, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw(54, 11, 109 + width, 164, FONT_NORMAL_BLACK);
        }
    } else {
        if (total_buildings == active_buildings) {
            // [count of] working industry/ies in the city
            int width = text_draw_number(total_buildings, 0, " ", 109, 164, FONT_NORMAL_BLACK);
            if (total_buildings == 1) {
                lang_text_draw(54, 8, 109 + width, 164, FONT_NORMAL_BLACK);
            } else {
                lang_text_draw(54, 9, 109 + width, 164, FONT_NORMAL_BLACK);
            }
        } else if (total_buildings > active_buildings) {
            // [count of] working [count of] idle industry/ies in the city
            int idle_buildings_count = total_buildings - active_buildings;
            int width = text_draw_number(active_buildings, 0, " ", 109, 164, FONT_NORMAL_BLACK);
            width += lang_text_draw(54, 12, 109 + width, 164, FONT_NORMAL_BLACK);
            width += text_draw_number(idle_buildings_count, 0, " ", 109 + width, 164, FONT_NORMAL_BLACK);
            if (idle_buildings_count == 1) {
                lang_text_draw(54, 14, 109 + width, 164, FONT_NORMAL_BLACK);
            } else {
                lang_text_draw(54, 13, 109 + width, 164, FONT_NORMAL_BLACK);
            }
        }
    }
    // Units stored in the city's warehouses
    int width = lang_text_draw_amount(8, 10, city_data.resource.stored_in_warehouses[resource_settings_data.resource], 104, 180, FONT_NORMAL_BLACK);
    lang_text_draw(54, 15, 104 + width, 180, FONT_NORMAL_BLACK);
    // Import/Export
    int trade_flags = TRADE_STATUS_NONE;
    int trade_status = city_data.resource.trade_status[resource_settings_data.resource];
    if (resource_import_trade_route_open(resource_settings_data.resource)) {
        trade_flags |= TRADE_STATUS_IMPORT;
    }
    if (resource_export_trade_route_open(resource_settings_data.resource)) {
        trade_flags |= TRADE_STATUS_EXPORT;
    }
    if (!trade_flags) {
        // There are no trade routes open for these goods
        lang_text_draw(54, 24, 109, 212, FONT_NORMAL_BLACK);
    } else {
        button_border_draw(104, 204, 432, 30, resource_settings_data.focus_button_id == 1);
        switch (trade_status) {
            case TRADE_STATUS_NONE:
                // Not trading
                lang_text_draw_centered(54, 18, 104, 212, 432, FONT_NORMAL_BLACK);
                break;
            case TRADE_STATUS_IMPORT:
                // Importing goods
                lang_text_draw_centered(54, 19, 104, 212, 432, FONT_NORMAL_BLACK);
                break;
            case TRADE_STATUS_EXPORT:
                // Export goods over
                lang_text_draw(54, 20, 152, 212, FONT_NORMAL_BLACK);
                break;
        }
    }
    if (trade_status == TRADE_STATUS_EXPORT) {
        arrow_buttons_draw(0, 0, resource_arrow_buttons, 2);
        lang_text_draw_amount(8, 10, city_data.resource.export_over[resource_settings_data.resource], 365, 212, FONT_NORMAL_BLACK);
    }
    if (building_count_industry_total(resource_settings_data.resource) > 0) {
        button_border_draw(104, 240, 432, 30, resource_settings_data.focus_button_id == 2);
        if (city_data.resource.mothballed[resource_settings_data.resource]) {
            // Industry is OFF
            lang_text_draw_centered(54, 17, 104, 248, 432, FONT_NORMAL_BLACK);
        } else {
            // Industry is ON
            lang_text_draw_centered(54, 16, 104, 248, 432, FONT_NORMAL_BLACK);
        }
    }
    button_border_draw(104, 276, 432, 50, resource_settings_data.focus_button_id == 3);
    if (city_data.resource.stockpiled[resource_settings_data.resource]) {
        // Stockpiling resource
        lang_text_draw_centered(54, 26, 104, 284, 432, FONT_NORMAL_BLACK);
        // Click here to turn off stockpiling
        lang_text_draw_centered(54, 27, 104, 304, 432, FONT_NORMAL_BLACK);
    } else {
        // Using and trading this resource
        lang_text_draw_centered(54, 28, 104, 284, 432, FONT_NORMAL_BLACK);
        // Click here to stockpile it
        lang_text_draw_centered(54, 29, 104, 304, 432, FONT_NORMAL_BLACK);
    }
    image_buttons_draw(0, 0, help_button_resource_settings, 1);
    graphics_reset_dialog();
}

static void handle_input_resource_settings(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_go_back();
        return;
    }
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (city_data.resource.trade_status[resource_settings_data.resource] == TRADE_STATUS_EXPORT) {
        int button = 0;
        arrow_buttons_handle_mouse(m_dialog, 0, 0, resource_arrow_buttons, 2, &button);
        if (button) {
            return;
        }
    }
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, resource_generic_buttons, sizeof(resource_generic_buttons) / sizeof(struct generic_button_t), &resource_settings_data.focus_button_id)) {
        return;
    }
    if (image_buttons_handle_mouse(m_dialog, 0, 0, help_button_resource_settings, 1, 0)) {
        return;
    }
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 48 || m_dialog->y < 100 || m_dialog->x > 592 || m_dialog->y > 340)) {
        window_go_back();
        return;
    }
}

static void window_resource_settings_show(int resource)
{
    struct window_type_t window = {
        WINDOW_RESOURCE_SETTINGS,
        window_draw_underlying_window,
        draw_foreground_resource_settings,
        handle_input_resource_settings,
    };
    resource_settings_data.resource = resource;
    window_show(&window);
}

static void draw_foreground_trade_prices(void)
{
    graphics_in_dialog();
    graphics_shade_rect(33, 53, 574, 334, 0);
    outer_panel_draw(16, 140, 38, 10);
    // Coin image
    image_draw(COIN_IMAGE_ID, 32, 156);
    // Prices set by Rome
    lang_text_draw_centered(54, 21, 26, 156, 608, FONT_LARGE_BLACK);
    // Buyers pay
    lang_text_draw(54, 22, 32, 233, FONT_NORMAL_BLACK);
    // Sellers receive
    lang_text_draw(54, 23, 32, 257, FONT_NORMAL_BLACK);
    for (int i = RESOURCE_WHEAT; i < RESOURCE_TYPES_MAX; i++) {
        image_draw(resource_images[i].icon_img_id + resource_image_offset(i, RESOURCE_IMAGE_ICON), 126 + 30 * i, 201);
        text_draw_number_centered(trade_prices[i].buy, 120 + 30 * i, 234, 30, FONT_SMALL_PLAIN);
        text_draw_number_centered(trade_prices[i].sell, 120 + 30 * i, 259, 30, FONT_SMALL_PLAIN);
    }
    graphics_reset_dialog();
}

static void handle_input_trade_prices(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_go_back();
        return;
    }

    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 16 || m_dialog->y < 144 || m_dialog->x > 624 || m_dialog->y > 300)) {
        window_go_back();
        return;
    }
}

static void button_prices(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_TRADE_PRICES,
        window_draw_underlying_window,
        draw_foreground_trade_prices,
        handle_input_trade_prices,
    };
    window_show(&window);
}

static void button_empire(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_empire_show();
}

static void button_resource_trade_advisor(int resource_index, __attribute__((unused)) int param2)
{
    window_resource_settings_show(city_resource_get_available()->items[resource_index]);
}

static struct generic_button_t resource_buttons[] = {
    {400, 398, 200, 23, button_prices, button_none, 1, 0},
    {100, 398, 200, 23, button_empire, button_none, 1, 0},
    {80, 56, 480, 20, button_resource_trade_advisor, button_none, 0, 0},
    {80, 78, 480, 20, button_resource_trade_advisor, button_none, 1, 0},
    {80, 100, 480, 20, button_resource_trade_advisor, button_none, 2, 0},
    {80, 122, 480, 20, button_resource_trade_advisor, button_none, 3, 0},
    {80, 144, 480, 20, button_resource_trade_advisor, button_none, 4, 0},
    {80, 166, 480, 20, button_resource_trade_advisor, button_none, 5, 0},
    {80, 188, 480, 20, button_resource_trade_advisor, button_none, 6, 0},
    {80, 210, 480, 20, button_resource_trade_advisor, button_none, 7, 0},
    {80, 232, 480, 20, button_resource_trade_advisor, button_none, 8, 0},
    {80, 254, 480, 20, button_resource_trade_advisor, button_none, 9, 0},
    {80, 276, 480, 20, button_resource_trade_advisor, button_none, 10, 0},
    {80, 298, 480, 20, button_resource_trade_advisor, button_none, 11, 0},
    {80, 320, 480, 20, button_resource_trade_advisor, button_none, 12, 0},
    {80, 342, 480, 20, button_resource_trade_advisor, button_none, 13, 0},
    {80, 364, 480, 20, button_resource_trade_advisor, button_none, 14, 0}
};

static int draw_background_trade_advisor(void)
{
    city_resource_determine_available();

    outer_panel_draw(0, 0, 40, TRADE_ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 4, 10, 10);

    lang_text_draw(54, 0, 60, 12, FONT_LARGE_BLACK);
    int width = lang_text_get_width(54, 1, FONT_NORMAL_BLACK);
    lang_text_draw(54, 1, 600 - width, 38, FONT_NORMAL_BLACK);

    return TRADE_ADVISOR_HEIGHT;
}

static void draw_foreground_trade_advisor(void)
{
    inner_panel_draw(32, 52, 36, 21);
    struct resource_list_t *list = city_resource_get_available();
    for (int i = 0; i < list->size; i++) {
        int y_offset = 22 * i;
        int resource = list->items[i];
        image_draw(resource_images[resource].icon_img_id + resource_image_offset(resource, RESOURCE_IMAGE_ICON), 48, y_offset + 54);
        image_draw(resource_images[resource].icon_img_id + resource_image_offset(resource, RESOURCE_IMAGE_ICON), 568, y_offset + 54);

        if (focus_button_id_trade_advisor - 3 == i) {
            button_border_draw(80, y_offset + 54, 480, 24, 1);
        }
        text_draw(resource_strings[resource], 88, y_offset + 61, FONT_NORMAL_WHITE, COLOR_BLACK);
        text_draw_number_centered(city_data.resource.stored_in_warehouses[resource], 180, y_offset + 61, 60, FONT_NORMAL_WHITE);
        if (city_data.resource.mothballed[resource]) {
            lang_text_draw_centered(18, 5, 240, y_offset + 61, 100, FONT_NORMAL_WHITE);
        }
        if (city_data.resource.stockpiled[resource]) {
            lang_text_draw(54, 3, 340, y_offset + 61, FONT_NORMAL_WHITE);
        } else {
            int trade_status = city_data.resource.trade_status[resource];
            if (trade_status == TRADE_STATUS_IMPORT) {
                lang_text_draw(54, 5, 340, y_offset + 61, FONT_NORMAL_WHITE);
            } else if (trade_status == TRADE_STATUS_EXPORT) {
                int width = lang_text_draw(54, 6, 340, y_offset + 61, FONT_NORMAL_WHITE);
                text_draw_number(city_data.resource.export_over[resource], '@', " ",
                    340 + width, y_offset + 61, FONT_NORMAL_WHITE);
            }
        }
    }

    button_border_draw(398, 396, 200, 24, focus_button_id_trade_advisor == 1);
    lang_text_draw_centered(54, 2, 400, 402, 200, FONT_NORMAL_BLACK);

    button_border_draw(98, 396, 200, 24, focus_button_id_trade_advisor == 2);
    lang_text_draw_centered(54, 30, 100, 402, 200, FONT_NORMAL_BLACK);
}

static int handle_mouse_trade_advisor(const struct mouse_t *m)
{
    int num_resources = city_resource_get_available()->size;
    return generic_buttons_handle_mouse(m, 0, 0, resource_buttons, num_resources + 2, &focus_button_id_trade_advisor);
}

static struct advisor_window_type_t *window_advisor_trade(void)
{
    static struct advisor_window_type_t window = {
        draw_background_trade_advisor,
        draw_foreground_trade_advisor,
        handle_mouse_trade_advisor,
    };
    return &window;
}

struct advisor_window_type_t *(*sub_advisors[])(void) = {
    0,
    window_advisor_labor,
    window_advisor_military,
    window_advisor_imperial,
    window_advisor_ratings,
    window_advisor_trade,
    window_advisor_population,
    window_advisor_health,
    window_advisor_education,
    window_advisor_entertainment,
    window_advisor_religion,
    window_advisor_financial,
    window_advisor_chief
};

static void set_advisor(int advisor)
{
    current_advisor = advisor;
    setting_set_last_advisor(advisor);
    if (sub_advisors[current_advisor]) {
        current_advisor_window = sub_advisors[current_advisor]();
    } else {
        current_advisor_window = 0;
    }
}

static void button_help_advisor(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (current_advisor > 0 && current_advisor < 13) {
        window_message_dialog_show(ADVISOR_TO_MESSAGE_TEXT[current_advisor], 0);
    }
}

static struct image_button_t help_button = {
    11, -7, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help_advisor, button_none, 0, 0, 1, 0, 0, 0
};

static void button_change_advisor(int advisor, __attribute__((unused)) int param2)
{
    if (advisor) {
        set_advisor(advisor);
        window_invalidate();
    } else {
        window_city_show();
    }
}

static struct generic_button_t advisor_buttons[] = {
    {12, 1, 40, 40, button_change_advisor, button_none, ADVISOR_LABOR, 0},
    {60, 1, 40, 40, button_change_advisor, button_none, ADVISOR_MILITARY, 0},
    {108, 1, 40, 40, button_change_advisor, button_none, ADVISOR_IMPERIAL, 0},
    {156, 1, 40, 40, button_change_advisor, button_none, ADVISOR_RATINGS, 0},
    {204, 1, 40, 40, button_change_advisor, button_none, ADVISOR_TRADE, 0},
    {252, 1, 40, 40, button_change_advisor, button_none, ADVISOR_POPULATION, 0},
    {300, 1, 40, 40, button_change_advisor, button_none, ADVISOR_HEALTH, 0},
    {348, 1, 40, 40, button_change_advisor, button_none, ADVISOR_EDUCATION, 0},
    {396, 1, 40, 40, button_change_advisor, button_none, ADVISOR_ENTERTAINMENT, 0},
    {444, 1, 40, 40, button_change_advisor, button_none, ADVISOR_RELIGION, 0},
    {492, 1, 40, 40, button_change_advisor, button_none, ADVISOR_FINANCIAL, 0},
    {540, 1, 40, 40, button_change_advisor, button_none, ADVISOR_CHIEF, 0},
    {588, 1, 40, 40, button_change_advisor, button_none, 0, 0},
};

static void draw_background_advisors(void)
{
    window_advisors_draw_dialog_background();
    graphics_in_dialog();
    advisor_height = current_advisor_window->draw_background();
    graphics_reset_dialog();
}

static void draw_foreground_advisors(void)
{
    graphics_in_dialog();
    image_buttons_draw(0, BLOCK_SIZE * (advisor_height - 2), &help_button, 1);
    graphics_reset_dialog();

    if (current_advisor_window->draw_foreground) {
        graphics_in_dialog();
        current_advisor_window->draw_foreground();
        graphics_reset_dialog();
    }
}

static void handle_input_advisors(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (h->show_last_advisor) {
        window_city_show();
        return;
    }
    if (h->show_empire_map) {
        window_empire_show();
        return;
    }
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 440, advisor_buttons, 13, &focus_button_id_advisors)) {
        return;
    }
    int button_id;
    image_buttons_handle_mouse(m_dialog, 0, BLOCK_SIZE * (advisor_height - 2), &help_button, 1, &button_id);
    if (button_id) {
        focus_button_id_advisors = -1;
    }
    if (current_advisor_window->handle_mouse && current_advisor_window->handle_mouse(m_dialog)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_city_show();
        return;
    }
}

void window_advisors_show(int advisor)
{
    struct window_type_t window = {
        WINDOW_ADVISORS,
        draw_background_advisors,
        draw_foreground_advisors,
        handle_input_advisors,
    };
    if (advisor) {
        set_advisor(advisor);
    } else {
        set_advisor(ADVISOR_CHIEF);
    }
    city_labor_allocate_workers();
    city_finance_estimate_taxes();
    city_finance_estimate_wages();
    city_data.finance.this_year.expenses.interest = city_data.finance.interest_so_far;
    city_data.finance.this_year.expenses.salary = city_data.finance.salary_so_far;
    city_finance_calculate_totals();
    switch (city_data.sentiment.low_mood_cause) {
        case LOW_MOOD_CAUSE_NO_FOOD:
            city_data.migration.no_immigration_cause = 2;
            break;
        case LOW_MOOD_CAUSE_NO_JOBS:
            city_data.migration.no_immigration_cause = 1;
            break;
        case LOW_MOOD_CAUSE_HIGH_TAXES:
            city_data.migration.no_immigration_cause = 3;
            break;
        case LOW_MOOD_CAUSE_LOW_WAGES:
            city_data.migration.no_immigration_cause = 0;
            break;
        case LOW_MOOD_CAUSE_MANY_TENTS:
            city_data.migration.no_immigration_cause = 4;
            break;
        default:
            city_data.migration.no_immigration_cause = 5;
            break;
    }
    // health
    city_data.houses.health = 0;
    int max = 0;
    if (city_data.houses.missing.bathhouse > max) {
        city_data.houses.health = 1;
        max = city_data.houses.missing.bathhouse;
    }
    if (city_data.houses.missing.barber > max) {
        city_data.houses.health = 2;
        max = city_data.houses.missing.barber;
    }
    if (city_data.houses.missing.clinic > max) {
        city_data.houses.health = 3;
        max = city_data.houses.missing.clinic;
    }
    if (city_data.houses.missing.hospital > max) {
        city_data.houses.health = 4;
    }
    // education
    city_data.houses.education = 0;
    if (city_data.houses.missing.more_education > city_data.houses.missing.education) {
        city_data.houses.education = 1; // schools(academies?)
    } else if (city_data.houses.missing.more_education < city_data.houses.missing.education) {
        city_data.houses.education = 2; // libraries
    } else if (city_data.houses.missing.more_education || city_data.houses.missing.education) {
        city_data.houses.education = 3; // more education
    }
    // entertainment
    city_data.houses.entertainment = 0;
    if (city_data.houses.missing.entertainment > city_data.houses.missing.more_entertainment) {
        city_data.houses.entertainment = 1;
    } else if (city_data.houses.missing.more_entertainment) {
        city_data.houses.entertainment = 2;
    }
    // religion
    city_data.houses.religion = 0;
    max = 0;
    if (city_data.houses.missing.religion > max) {
        city_data.houses.religion = 1;
        max = city_data.houses.missing.religion;
    }
    if (city_data.houses.missing.second_religion > max) {
        city_data.houses.religion = 2;
        max = city_data.houses.missing.second_religion;
    }
    if (city_data.houses.missing.third_religion > max) {
        city_data.houses.religion = 3;
    }
    city_culture_update_coverage();
    city_resource_calculate_food_stocks_and_supply_wheat();
    city_ratings_update_explanations();
    window_show(&window);
}

static void select_building_type(int building_type)
{
    building_construction_set_type(building_type);
    build_menu_data.selected_menu = MENU_NONE;
    build_menu_data.selected_submenu = MENU_NONE;
    window_city_show();
}

static void button_submenu_or_building(int param1, __attribute__((unused)) int param2)
{
    int n_skipped_items = 0;
    if (build_menu_data.selected_submenu > MENU_NONE) { // selecting items from a submenu
        for (int k = 0; k < MAX_ITEMS_PER_SUBMENU; k++) { // select next enabled item
            if (build_menus[build_menu_data.selected_menu].menu_items[build_menu_data.selected_submenu].submenu_items[k]) {
                if (k - n_skipped_items >= param1) {
                    select_building_type(build_menus[build_menu_data.selected_menu].menu_items[build_menu_data.selected_submenu].submenu_items[k]);
                    break;
                }
            } else {
                n_skipped_items++;
            }
        }
    } else { // selecting items from a menu
        for (int j = 0; j < MAX_ITEMS_PER_BUILD_MENU; j++) { // select next enabled item
            if (build_menus[build_menu_data.selected_menu].menu_items[j].building_id) { // building or submenu is enabled
                if (j - n_skipped_items >= param1) { // align button index with item
                    if (build_menus[build_menu_data.selected_menu].menu_items[j].building_id == -1) { // submenu item
                        build_menu_data.selected_submenu = j;
                        build_menu_data.num_items_to_draw = 0;
                        for (int k = 0; k < MAX_ITEMS_PER_SUBMENU; k++) {
                            if (build_menus[build_menu_data.selected_menu].menu_items[j].submenu_items[k]) {
                                build_menu_data.num_items_to_draw++;
                            }
                        }
                    } else { // building item
                        select_building_type(build_menus[build_menu_data.selected_menu].menu_items[j].building_id);
                    }
                    break;
                }
            } else {
                n_skipped_items++;
            }
        }
    }
}

static struct generic_button_t build_menu_buttons[MAX_ITEMS_PER_BUILD_MENU] = {
    {0, 0, 256, 20, button_submenu_or_building, button_none, 0, 0},
    {0, 24, 256, 20, button_submenu_or_building, button_none, 1, 0},
    {0, 48, 256, 20, button_submenu_or_building, button_none, 2, 0},
    {0, 72, 256, 20, button_submenu_or_building, button_none, 3, 0},
    {0, 96, 256, 20, button_submenu_or_building, button_none, 4, 0},
    {0, 120, 256, 20, button_submenu_or_building, button_none, 5, 0},
    {0, 144, 256, 20, button_submenu_or_building, button_none, 6, 0},
    {0, 168, 256, 20, button_submenu_or_building, button_none, 7, 0},
    {0, 192, 256, 20, button_submenu_or_building, button_none, 8, 0},
    {0, 216, 256, 20, button_submenu_or_building, button_none, 9, 0},
    {0, 240, 256, 20, button_submenu_or_building, button_none, 10, 0},
};

static void map_submenu_items(int menu_index, int submenu_index, int submenu_string_index)
{
    build_menus[menu_index].menu_items[submenu_index].submenu_string = submenu_strings[submenu_string_index];
    for (int k = 0; k < MAX_ITEMS_PER_SUBMENU; k++) {
        if (scenario.allowed_buildings[BUILDING_MENU_SUBMENU_ITEM_MAPPING[menu_index][submenu_index][k]]) {
            build_menus[menu_index].is_enabled = 1;
            build_menus[menu_index].menu_items[submenu_index].building_id = -1; // submenu is enabled (negative number to avoid conflict with actual building types)
            build_menus[menu_index].menu_items[submenu_index].submenu_items[k] = BUILDING_MENU_SUBMENU_ITEM_MAPPING[menu_index][submenu_index][k];
        }
    }
}

void map_building_menu_items(void)
{
    // reset values so as to not carry over between maps
    for (int i = 0; i < BUILD_MENU_BUTTONS_COUNT; i++) {
        memset(&build_menus[i], 0, sizeof(struct build_menu_t));
    }

    for (int i = 0; i < BUILD_MENU_BUTTONS_COUNT; i++) {
        for (int j = 0; j < MAX_ITEMS_PER_BUILD_MENU; j++) {
            if (i == 5 && j == 0) { // small temples
                map_submenu_items(i, j, 0);
            } else if (i == 5 && j == 1) { // large temples
                map_submenu_items(i, j, 1);
            } else if (i == 10 && j == 4) { // forts
                map_submenu_items(i, j, 2);
            } else if (i == 11 && j == 0) { // farms
                map_submenu_items(i, j, 3);
            } else if (i == 11 && j == 1) { // raw materials
                map_submenu_items(i, j, 4);
            } else if (i == 11 && j == 2) { // workshops
                map_submenu_items(i, j, 5);
            } else {
                if (scenario.allowed_buildings[BUILDING_MENU_SUBMENU_ITEM_MAPPING[i][j][0]] && BUILDING_MENU_SUBMENU_ITEM_MAPPING[i][j][0] != BUILDING_TRIUMPHAL_ARCH) {
                    build_menus[i].is_enabled = 1;
                    build_menus[i].menu_items[j].building_id = BUILDING_MENU_SUBMENU_ITEM_MAPPING[i][j][0];
                }
            }
        }
    }
}

static int get_sidebar_x_offset_build_menu(void)
{
    int view_x, view_y, view_width, view_height;
    city_view_get_viewport(&view_x, &view_y, &view_width, &view_height);
    return view_x + view_width;
}

int window_build_menu_image(void)
{
    int image_base = image_group(GROUP_PANEL_WINDOWS);
    switch (build_menu_data.selected_menu) {
        case MENU_VACANT_HOUSE:
            return image_base;
        case MENU_CLEAR_LAND:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT);
            } else {
                return image_base + 11;
            }
        case MENU_ROAD:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT) + 1;
            } else {
                return image_base + 10;
            }
        case MENU_WATER:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT) + 2;
            } else {
                return image_base + 3;
            }
        case MENU_HEALTH:
            return image_base + 5;
        case MENU_TEMPLES:
            return image_base + 1;
        case MENU_EDUCATION:
            return image_base + 6;
        case MENU_ENTERTAINMENT:
            return image_base + 4;
        case MENU_ADMINISTRATION:
            return image_base + 2;
        case MENU_ENGINEERING:
            return image_base + 7;
        case MENU_SECURITY:
            if (scenario.climate == CLIMATE_DESERT) {
                return image_group(GROUP_PANEL_WINDOWS_DESERT) + 3;
            } else {
                return image_base + 8;
            }
        case MENU_INDUSTRY:
            return image_base + 9;
        default:
            return image_base + 12;
    }
}

static void draw_foreground_build_menu(void)
{
    widget_city_draw();
    int x_offset = get_sidebar_x_offset_build_menu();
    int item_x_align = x_offset - MENU_X_OFFSET_BUILD_MENU - 8;
    int n_skipped_items = 0;
    for (int j = 0; j < build_menu_data.num_items_to_draw; j++) {
        label_draw(item_x_align, build_menu_data.y_offset + MENU_Y_OFFSET_BUILD_MENU + MENU_ITEM_HEIGHT * j, 16, build_menu_data.focus_button_id == j + 1 ? 1 : 2);
        if (build_menu_data.selected_submenu > MENU_NONE) { // drawing items from a submenu
            for (int k = j + n_skipped_items; k < MAX_ITEMS_PER_SUBMENU; k++) { // draw next enabled item
                if (build_menus[build_menu_data.selected_menu].menu_items[build_menu_data.selected_submenu].submenu_items[k]) {
                    text_draw_centered(all_buildings_strings[build_menus[build_menu_data.selected_menu].menu_items[build_menu_data.selected_submenu].submenu_items[k]], item_x_align, build_menu_data.y_offset + MENU_Y_OFFSET_BUILD_MENU + 4 + MENU_ITEM_HEIGHT * j, MENU_ITEM_WIDTH, FONT_NORMAL_GREEN, COLOR_BLACK);
                    text_draw_money(building_properties[build_menus[build_menu_data.selected_menu].menu_items[build_menu_data.selected_submenu].submenu_items[k]].cost, x_offset - 82, build_menu_data.y_offset + MENU_Y_OFFSET_BUILD_MENU + 4 + MENU_ITEM_HEIGHT * j, FONT_NORMAL_GREEN);
                    break;
                } else {
                    n_skipped_items++;
                }
            }
        } else { // drawing items from a menu
            for (int k = j + n_skipped_items; k < MAX_ITEMS_PER_BUILD_MENU; k++) {
                if (build_menus[build_menu_data.selected_menu].menu_items[k].building_id) { // building or submenu is enabled
                    if (build_menus[build_menu_data.selected_menu].menu_items[k].building_id == -1) { // submenu item
                        text_draw_centered(build_menus[build_menu_data.selected_menu].menu_items[k].submenu_string, item_x_align, build_menu_data.y_offset + MENU_Y_OFFSET_BUILD_MENU + 4 + MENU_ITEM_HEIGHT * j, MENU_ITEM_WIDTH, FONT_NORMAL_GREEN, COLOR_BLACK);
                    } else { // building item
                        text_draw_centered(all_buildings_strings[build_menus[build_menu_data.selected_menu].menu_items[k].building_id], item_x_align, build_menu_data.y_offset + MENU_Y_OFFSET_BUILD_MENU + 4 + MENU_ITEM_HEIGHT * j, MENU_ITEM_WIDTH, FONT_NORMAL_GREEN, COLOR_BLACK);
                        text_draw_money(building_properties[build_menus[build_menu_data.selected_menu].menu_items[k].building_id].cost, x_offset - 82, build_menu_data.y_offset + MENU_Y_OFFSET_BUILD_MENU + 4 + MENU_ITEM_HEIGHT * j, FONT_NORMAL_GREEN);
                    }
                    break;
                } else {
                    n_skipped_items++;
                }
            }
        }
    }
}

static void slide_finished(void)
{
    city_view_toggle_sidebar();
    window_city_show();
}

static void draw_sliding_foreground(void)
{
    window_request_refresh();
    widget_slide_data.position += speed_get_delta(&widget_slide_data.slide_speed);
    int is_finished = 0;
    if (widget_slide_data.position >= SIDEBAR_EXPANDED_WIDTH) {
        widget_slide_data.position = SIDEBAR_EXPANDED_WIDTH;
        is_finished = 1;
    }
    int x_offset = sidebar_common_get_x_offset_expanded();
    graphics_set_clip_rectangle(x_offset, TOP_MENU_HEIGHT, SIDEBAR_EXPANDED_WIDTH, screen_height() - TOP_MENU_HEIGHT);

    if (widget_slide_data.direction == SLIDE_DIRECTION_IN) {
        if (widget_slide_data.position > SIDEBAR_DECELERATION_OFFSET) {
            speed_set_target(&widget_slide_data.slide_speed, 1, SLIDE_ACCELERATION_MILLIS, 1);
        }
        x_offset += SIDEBAR_EXPANDED_WIDTH - widget_slide_data.position;
    } else {
        x_offset += widget_slide_data.position;
    }
    widget_slide_data.back_sidebar_draw();
    widget_slide_data.front_sidebar_draw(x_offset);
    graphics_reset_clip_rectangle();
    if (is_finished) {
        widget_slide_data.finished_callback();
    }
}

static void draw_collapsed_background(void);
static void draw_expanded_background(int x_offset);

static void button_collapse_expand(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    city_view_start_sidebar_toggle();
    widget_slide_data.direction = !city_view_is_sidebar_collapsed();
    widget_slide_data.position = 0;
    speed_clear(&widget_slide_data.slide_speed);
    speed_set_target(&widget_slide_data.slide_speed, SLIDE_SPEED, !(city_view_is_sidebar_collapsed() == SLIDE_DIRECTION_OUT) ? SLIDE_ACCELERATION_MILLIS : SPEED_CHANGE_IMMEDIATE, 1);
    widget_slide_data.back_sidebar_draw = draw_collapsed_background;
    widget_slide_data.front_sidebar_draw = draw_expanded_background;
    widget_slide_data.finished_callback = slide_finished;
    play_sound_effect(SOUND_EFFECT_SIDEBAR);
    struct window_type_t window = {
        WINDOW_SLIDING_SIDEBAR,
        widget_city_draw,
        draw_sliding_foreground,
        0,
    };
    window_show(&window);
}

static void button_help_city_widget(__attribute__((unused)) int param1, int param2)
{
    window_build_menu_hide();
    window_message_dialog_show(param2, window_city_draw_all);
}

static void button_overlay_city_widget(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_overlay_menu_show();
}

static struct image_button_t buttons_overlays_collapse_sidebar[] = {
    {127, 5, 31, 20, IB_NORMAL, 90, 0, button_collapse_expand, button_none, 0, 0, 1, 0, 0, 0},
    {4, 3, 117, 31, IB_NORMAL, 93, 0, button_overlay_city_widget, button_help_city_widget, 0, MESSAGE_DIALOG_OVERLAYS, 1, 0, 0, 0}
};

static struct image_button_t button_expand_sidebar[] = {
    {6, 4, 31, 20, IB_NORMAL, 90, 4, button_collapse_expand, button_none, 0, 0, 1, 0, 0, 0}
};

static void button_build_city_widget(int menu, __attribute__((unused)) int param2)
{
    if (menu == MENU_VACANT_HOUSE || menu == MENU_CLEAR_LAND || menu == MENU_ROAD) {
        building_construction_set_type(BUILDING_MENU_SUBMENU_ITEM_MAPPING[menu][0][0]);
    } else {
        window_build_menu_show(menu);
    }
}

static struct image_button_t buttons_build_collapsed[] = {
    {2, 32, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 0, button_build_city_widget, button_none, MENU_VACANT_HOUSE, 0, 1, 0, 0, 0},
    {2, 67, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 8, button_build_city_widget, button_none, MENU_CLEAR_LAND, 0, 1, 0, 0, 0},
    {2, 102, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 12, button_build_city_widget, button_none, MENU_ROAD, 0, 1, 0, 0, 0},
    {2, 137, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 4, button_build_city_widget, button_none, MENU_WATER, 0, 1, 0, 0, 0},
    {2, 172, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 40, button_build_city_widget, button_none, MENU_HEALTH, 0, 1, 0, 0, 0},
    {2, 207, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 28, button_build_city_widget, button_none, MENU_TEMPLES, 0, 1, 0, 0, 0},
    {2, 242, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 24, button_build_city_widget, button_none, MENU_EDUCATION, 0, 1, 0, 0, 0},
    {2, 277, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 20, button_build_city_widget, button_none, MENU_ENTERTAINMENT, 0, 1, 0, 0, 0},
    {2, 312, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 16, button_build_city_widget, button_none, MENU_ADMINISTRATION, 0, 1, 0, 0, 0},
    {2, 347, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 44, button_build_city_widget, button_none, MENU_ENGINEERING, 0, 1, 0, 0, 0},
    {2, 382, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 36, button_build_city_widget, button_none, MENU_SECURITY, 0, 1, 0, 0, 0},
    {2, 417, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 32, button_build_city_widget, button_none, MENU_INDUSTRY, 0, 1, 0, 0, 0},
};

static void button_undo_city_widget(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_build_menu_hide();
    game_undo_perform();
    window_invalidate();
}

static void button_messages_city_widget(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_build_menu_hide();
    window_message_list_show();
}

static void button_go_to_problem(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_build_menu_hide();
    int grid_offset = city_message_next_problem_area_grid_offset();
    if (grid_offset) {
        city_view_go_to_grid_offset(grid_offset);
        window_city_show();
    } else {
        window_invalidate();
    }
}

static struct image_button_t buttons_build_expanded[] = {
    {13, 277, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 0, button_build_city_widget, button_none, MENU_VACANT_HOUSE, 0, 1, 0, 0, 0},
    {63, 277, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 8, button_build_city_widget, button_none, MENU_CLEAR_LAND, 0, 1, 0, 0, 0},
    {113, 277, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 12, button_build_city_widget, button_none, MENU_ROAD, 0, 1, 0, 0, 0},
    {13, 313, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 4, button_build_city_widget, button_none, MENU_WATER, 0, 1, 0, 0, 0},
    {63, 313, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 40, button_build_city_widget, button_none, MENU_HEALTH, 0, 1, 0, 0, 0},
    {113, 313, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 28, button_build_city_widget, button_none, MENU_TEMPLES, 0, 1, 0, 0, 0},
    {13, 349, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 24, button_build_city_widget, button_none, MENU_EDUCATION, 0, 1, 0, 0, 0},
    {63, 349, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 20, button_build_city_widget, button_none, MENU_ENTERTAINMENT, 0, 1, 0, 0, 0},
    {113, 349, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 16, button_build_city_widget, button_none, MENU_ADMINISTRATION, 0, 1, 0, 0, 0},
    {13, 385, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 44, button_build_city_widget, button_none, MENU_ENGINEERING, 0, 1, 0, 0, 0},
    {63, 385, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 36, button_build_city_widget, button_none, MENU_SECURITY, 0, 1, 0, 0, 0},
    {113, 385, 39, 26, IB_BUILD, GROUP_SIDEBAR_BUTTONS, 32, button_build_city_widget, button_none, MENU_INDUSTRY, 0, 1, 0, 0, 0},
    {13, 421, 39, 26, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 48, button_undo_city_widget, button_none, 0, 0, 1, 0, 0, 0},
    {63, 421, 39, 26, IB_NORMAL, GROUP_ARROW_MESSAGE_PROBLEMS, 18, button_messages_city_widget, button_help_city_widget, 0, MESSAGE_DIALOG_MESSAGES, 1, 0, 0, 0},
    {113, 421, 39, 26, IB_BUILD, GROUP_ARROW_MESSAGE_PROBLEMS, 22, button_go_to_problem, button_none, 0, 0, 1, 0, 0, 0},
};

static void handle_input_build_menu(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(m, get_sidebar_x_offset_build_menu() - MENU_X_OFFSET_BUILD_MENU,
        build_menu_data.y_offset + MENU_Y_OFFSET_BUILD_MENU, build_menu_buttons, build_menu_data.num_items_to_draw, &build_menu_data.focus_button_id)) {
        return;
    }
    if (city_view_is_sidebar_collapsed()) {
        image_buttons_handle_mouse(m, screen_width() - SIDEBAR_COLLAPSED_WIDTH, 24, buttons_build_collapsed, sizeof(buttons_build_collapsed) / sizeof(struct image_button_t), 0);
    } else {
        image_buttons_handle_mouse(m, sidebar_common_get_x_offset_expanded(), 24, buttons_build_expanded, sizeof(buttons_build_expanded) / sizeof(struct image_button_t), 0);
    }
    if (m->right.went_up || h->escape_pressed
    || (m->left.went_up && // click outside build menu
        (m->x < get_sidebar_x_offset_build_menu() - MENU_X_OFFSET_BUILD_MENU - MENU_CLICK_MARGIN ||
            m->x > get_sidebar_x_offset_build_menu() + MENU_CLICK_MARGIN ||
            m->y < build_menu_data.y_offset + MENU_Y_OFFSET_BUILD_MENU - MENU_CLICK_MARGIN ||
            m->y > build_menu_data.y_offset + MENU_Y_OFFSET_BUILD_MENU + MENU_CLICK_MARGIN + MENU_ITEM_HEIGHT * build_menu_data.num_items_to_draw))) {
        build_menu_data.selected_menu = MENU_NONE;
        build_menu_data.selected_submenu = MENU_NONE;
        window_city_show();
        return;
    }
}

void window_build_menu_show(int menu)
{
    if (menu == MENU_NONE || menu == build_menu_data.selected_menu) {
        window_build_menu_hide();
        return;
    }
    build_menu_data.selected_menu = menu;
    build_menu_data.selected_submenu = MENU_NONE;
    build_menu_data.num_items_to_draw = 0;
    for (int j = 0; j < MAX_ITEMS_PER_BUILD_MENU; j++) {
        if (build_menus[menu].menu_items[j].building_id) {
            build_menu_data.num_items_to_draw++;
        }
    }
    build_menu_data.y_offset = Y_MENU_OFFSETS[build_menu_data.num_items_to_draw];
    struct window_type_t window = {
        WINDOW_BUILD_MENU,
        window_city_draw_background,
        draw_foreground_build_menu,
        handle_input_build_menu,
    };
    window_show(&window);
}

void window_build_menu_hide(void)
{
    build_menu_data.selected_menu = MENU_NONE;
    window_city_show();
}

static void clear_state_top_menu(void)
{
    top_menu_data.open_sub_menu = 0;
    top_menu_data.focus_menu_id = 0;
    top_menu_data.focus_sub_menu_id = 0;
}

static void top_menu_file_replay_map(__attribute__((unused)) int param)
{
    clear_state_top_menu();
    replay_map();
}

static void top_menu_file_load_game(__attribute__((unused)) int param)
{
    clear_state_top_menu();
    building_construction_clear_type();
    window_go_back();
    window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_LOAD);
}

static void top_menu_file_save_game(__attribute__((unused)) int param)
{
    clear_state_top_menu();
    window_go_back();
    window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_SAVE);
}

static void top_menu_file_delete_game(__attribute__((unused)) int param)
{
    clear_state_top_menu();
    window_go_back();
    window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_DELETE);
}

static void confirm_exit_to_main_menu(void)
{
    building_construction_clear_type();
    game_undo_disable();
    game_state_reset_overlay();
    window_main_menu_show(1);
}

static void top_menu_file_exit_game(__attribute__((unused)) int param)
{
    clear_state_top_menu();
    window_popup_dialog_show(POPUP_DIALOG_QUIT, confirm_exit_to_main_menu, 1);
}

static struct menu_item_t top_menu_file[] = {
    {1, 2, top_menu_file_replay_map, 0, 0},
    {1, 3, top_menu_file_load_game, 0, 0},
    {1, 4, top_menu_file_save_game, 0, 0},
    {1, 6, top_menu_file_delete_game, 0, 0},
    {1, 5, top_menu_file_exit_game, 0, 0},
};

static void top_menu_options_display(__attribute__((unused)) int param)
{
    clear_state_top_menu();
    window_display_options_show(window_city_return);
}

static void top_menu_options_sound(__attribute__((unused)) int param)
{
    clear_state_top_menu();
    window_sound_options_show(0);
}

static void top_menu_options_speed(__attribute__((unused)) int param)
{
    clear_state_top_menu();
    window_speed_options_show(0);
}

static void top_menu_options_autosave(__attribute__((unused)) int param);

static struct menu_item_t top_menu_options[] = {
    {2, 1, top_menu_options_display, 0, 0},
    {2, 2, top_menu_options_sound, 0, 0},
    {2, 3, top_menu_options_speed, 0, 0},
    {19, 51, top_menu_options_autosave, 0, 0},
};

static void top_menu_help_help(__attribute__((unused)) int param)
{
    clear_state_top_menu();
    window_go_back();
    window_message_dialog_show(MESSAGE_DIALOG_HELP, window_city_draw_all);
}

static void top_menu_help_about(__attribute__((unused)) int param)
{
    clear_state_top_menu();
    window_go_back();
    window_message_dialog_show(MESSAGE_DIALOG_ABOUT, window_city_draw_all);
}

static void top_menu_help_warnings(__attribute__((unused)) int param);

static struct menu_item_t top_menu_help[] = {
    {3, 1, top_menu_help_help, 0, 0},
    {3, 5, top_menu_help_warnings, 0, 0},
    {3, 7, top_menu_help_about, 0, 0},
};

static struct menu_bar_item_t top_menu[] = {
    {1, top_menu_file, 5, 0, 0, 0, 0},
    {2, top_menu_options, 4, 0, 0, 0, 0},
    {3, top_menu_help, 3, 0, 0, 0, 0},
};

static void top_menu_help_warnings(__attribute__((unused)) int param)
{
    setting_toggle_warnings();
    menu_update_text(&top_menu[INDEX_HELP], 1, setting_warnings() ? 6 : 5);
}

static void top_menu_options_autosave(__attribute__((unused)) int param)
{
    setting_toggle_monthly_autosave();
    menu_update_text(&top_menu[INDEX_OPTIONS], 3, setting_monthly_autosave() ? 51 : 52);
}

static void widget_top_menu_draw(int force)
{
    if (!force && drawn.treasury == city_data.finance.treasury &&
        drawn.population == city_data.population.population &&
        drawn.month == game_time_month()) {
        return;
    }

    int s_width = screen_width();
    int block_width = 24;
    int image_base = image_group(GROUP_TOP_MENU);
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
    menu_bar_draw(top_menu, sizeof(top_menu) / sizeof(struct menu_bar_item_t), s_width < 1024 ? 338 : 493);

    color_t treasure_color = COLOR_WHITE;
    if (city_data.finance.treasury < 0) {
        treasure_color = COLOR_FONT_RED;
    }
    if (s_width < 800) {
        top_menu_data.offset_funds = 338;
        top_menu_data.offset_population = 453;
        top_menu_data.offset_date = 547;

        int width = lang_text_draw_colored(6, 0, 350, 5, FONT_NORMAL_PLAIN, treasure_color);
        text_draw_number_colored(city_data.finance.treasury, '@', " ", 346 + width, 5, FONT_NORMAL_PLAIN, treasure_color);

        width = lang_text_draw(6, 1, 458, 5, FONT_NORMAL_GREEN);
        text_draw_number(city_data.population.population, '@', " ", 450 + width, 5, FONT_NORMAL_GREEN);

        lang_text_draw_month_year_max_width(game_time_month(), game_time_year(), 540, 5, 100, FONT_NORMAL_GREEN, 0);
    } else if (s_width < 1024) {
        top_menu_data.offset_funds = 338;
        top_menu_data.offset_population = 458;
        top_menu_data.offset_date = 652;

        int width = lang_text_draw_colored(6, 0, 350, 5, FONT_NORMAL_PLAIN, treasure_color);
        text_draw_number_colored(city_data.finance.treasury, '@', " ", 346 + width, 5, FONT_NORMAL_PLAIN, treasure_color);

        width = lang_text_draw_colored(6, 1, 470, 5, FONT_NORMAL_PLAIN, COLOR_WHITE);
        text_draw_number_colored(city_data.population.population, '@', " ", 466 + width, 5, FONT_NORMAL_PLAIN, COLOR_WHITE);

        lang_text_draw_month_year_max_width(game_time_month(), game_time_year(),
            655, 5, 110, FONT_NORMAL_PLAIN, COLOR_FONT_YELLOW);
    } else {
        top_menu_data.offset_funds = 493;
        top_menu_data.offset_population = 637;
        top_menu_data.offset_date = 852;

        int width = lang_text_draw_colored(6, 0, 495, 5, FONT_NORMAL_PLAIN, treasure_color);
        text_draw_number_colored(city_data.finance.treasury, '@', " ", 501 + width, 5, FONT_NORMAL_PLAIN, treasure_color);

        width = lang_text_draw_colored(6, 1, 645, 5, FONT_NORMAL_PLAIN, COLOR_WHITE);
        text_draw_number_colored(city_data.population.population, '@', " ", 651 + width, 5, FONT_NORMAL_PLAIN, COLOR_WHITE);

        lang_text_draw_month_year_max_width(game_time_month(), game_time_year(),
            850, 5, 110, FONT_NORMAL_PLAIN, COLOR_FONT_YELLOW);
    }
    drawn.treasury = city_data.finance.treasury;
    drawn.population = city_data.population.population;
    drawn.month = game_time_month();
}

static void button_advisors_city_widget(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_advisors_show(setting_last_advisor());
}

static void button_empire_city_widget(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_empire_show();
}

static void button_mission_briefing(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_mission_briefing_show();
}

static void button_rotate_north_city_widget(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    game_orientation_rotate_north();
    window_invalidate();
}

static void button_rotate_city_widget(int clockwise, __attribute__((unused)) int param2)
{
    if (clockwise) {
        game_orientation_rotate_right();
    } else {
        game_orientation_rotate_left();
    }
    window_invalidate();
}

static struct image_button_t buttons_top_expanded[] = {
    {7, 155, 71, 23, IB_NORMAL, GROUP_SIDEBAR_ADVISORS_EMPIRE, 0, button_advisors_city_widget, button_none, 0, 0, 1, 0, 0, 0},
    {84, 155, 71, 23, IB_NORMAL, GROUP_SIDEBAR_ADVISORS_EMPIRE, 3, button_empire_city_widget, button_help_city_widget, 0, MESSAGE_DIALOG_EMPIRE_MAP, 1, 0, 0, 0},
    {7, 184, 33, 22, IB_NORMAL, GROUP_SIDEBAR_BRIEFING_ROTATE_BUTTONS, 0, button_mission_briefing, button_none, 0, 0, 1, 0, 0, 0},
    {46, 184, 33, 22, IB_NORMAL, GROUP_SIDEBAR_BRIEFING_ROTATE_BUTTONS, 3, button_rotate_north_city_widget, button_none, 0, 0, 1, 0, 0, 0},
    {84, 184, 33, 22, IB_NORMAL, GROUP_SIDEBAR_BRIEFING_ROTATE_BUTTONS, 6, button_rotate_city_widget, button_none, 0, 0, 1, 0, 0, 0},
    {123, 184, 33, 22, IB_NORMAL, GROUP_SIDEBAR_BRIEFING_ROTATE_BUTTONS, 9, button_rotate_city_widget, button_none, 1, 0, 1, 0, 0, 0},
};

static void draw_buttons_expanded(int x_offset)
{
    buttons_build_expanded[12].enabled = game_can_undo();
    for (int i = 0; i < BUILD_MENU_BUTTONS_COUNT; i++) {
        buttons_build_expanded[i].enabled = build_menus[i].is_enabled;
    }
    image_buttons_draw(x_offset, 24, buttons_overlays_collapse_sidebar, 2);
    image_buttons_draw(x_offset, 24, buttons_build_expanded, sizeof(buttons_build_expanded) / sizeof(struct image_button_t));
    image_buttons_draw(x_offset, 24, buttons_top_expanded, sizeof(buttons_top_expanded) / sizeof(struct image_button_t));
}

static void draw_number_of_messages(int x_offset)
{
    int messages = city_message_count();
    buttons_build_expanded[13].enabled = messages > 0;
    buttons_build_expanded[14].enabled = city_message_problem_area_count();
    if (messages) {
        text_draw_number_centered_colored(messages, x_offset + 74, 452, 32, FONT_SMALL_PLAIN, COLOR_BLACK);
        text_draw_number_centered_colored(messages, x_offset + 73, 453, 32, FONT_SMALL_PLAIN, COLOR_WHITE);
    }
}

static void button_game_speed(int is_down, __attribute__((unused)) int param2)
{
    if (is_down) {
        setting_decrease_game_speed();
    } else {
        setting_increase_game_speed();
    }
}

static struct arrow_button_t arrow_buttons_speed[] = {
    {11, 30, 17, 24, button_game_speed, 1, 0, 0, 0},
    {35, 30, 15, 24, button_game_speed, 0, 0, 0, 0},
};

static int update_extra_info_value(int value, int *field)
{
    if (value == *field) {
        return 0;
    } else {
        *field = value;
        return 1;
    }
}

static int update_extra_info(int is_background)
{
    int changed = 0;
    if (extra_widget_data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        changed |= update_extra_info_value(setting_game_speed(), &extra_widget_data.game_speed);
    }
    if (extra_widget_data.info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
        changed |= update_extra_info_value(city_data.labor.unemployment_percentage, &extra_widget_data.unemployment_percentage);
        changed |= update_extra_info_value(
                       city_data.labor.workers_unemployed - city_data.labor.workers_needed,
                       &extra_widget_data.unemployment_amount
        );
    }
    if (extra_widget_data.info_to_display & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
        if (is_background) {
            extra_widget_data.culture.target = 0;
            extra_widget_data.prosperity.target = 0;
            extra_widget_data.peace.target = 0;
            extra_widget_data.favor.target = 0;
            extra_widget_data.population.target = 0;
            if (scenario.culture_win_criteria.enabled) {
                extra_widget_data.culture.target = scenario.culture_win_criteria.goal;
            }
            if (scenario.prosperity_win_criteria.enabled) {
                extra_widget_data.prosperity.target = scenario.prosperity_win_criteria.goal;
            }
            if (scenario.peace_win_criteria.enabled) {
                extra_widget_data.peace.target = scenario.peace_win_criteria.goal;
            }
            if (scenario.favor_win_criteria.enabled) {
                extra_widget_data.favor.target = scenario.favor_win_criteria.goal;
            }
            if (scenario.population_win_criteria.enabled) {
                extra_widget_data.population.target = scenario.population_win_criteria.goal;
            }
        }
        changed |= update_extra_info_value(city_data.ratings.culture, &extra_widget_data.culture.value);
        changed |= update_extra_info_value(city_data.ratings.prosperity, &extra_widget_data.prosperity.value);
        changed |= update_extra_info_value(city_data.ratings.peace, &extra_widget_data.peace.value);
        changed |= update_extra_info_value(city_data.ratings.favor, &extra_widget_data.favor.value);
        changed |= update_extra_info_value(city_data.population.population, &extra_widget_data.population.value);
    }
    return changed;
}

static int draw_extra_info_objective(int x_offset, int y_offset, int text_group, int text_id, struct objective_t *obj)
{
    lang_text_draw(text_group, text_id, x_offset + 11, y_offset, FONT_NORMAL_WHITE);
    int font = obj->value >= obj->target ? FONT_NORMAL_GREEN : FONT_NORMAL_RED;
    int width = text_draw_number(obj->value, '@', "", x_offset + 11, y_offset + EXTRA_INFO_LINE_SPACE, font);
    text_draw_number(obj->target, '(', ")", x_offset + 11 + width, y_offset + EXTRA_INFO_LINE_SPACE, font);
    return EXTRA_INFO_LINE_SPACE * 2;
}

static void draw_extra_info_panel(void)
{
    graphics_draw_vertical_line(extra_widget_data.x_offset, extra_widget_data.y_offset, extra_widget_data.y_offset + extra_widget_data.height, COLOR_WHITE);
    graphics_draw_vertical_line(extra_widget_data.x_offset + extra_widget_data.width - 1, extra_widget_data.y_offset,
        extra_widget_data.y_offset + extra_widget_data.height, COLOR_SIDEBAR);
    inner_panel_draw(extra_widget_data.x_offset + 1, extra_widget_data.y_offset, extra_widget_data.width / BLOCK_SIZE, extra_widget_data.height / BLOCK_SIZE);
    int y_current_line = extra_widget_data.y_offset + EXTRA_INFO_VERTICAL_PADDING;
    if (extra_widget_data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        y_current_line += EXTRA_INFO_VERTICAL_PADDING;
        lang_text_draw(45, 2, extra_widget_data.x_offset + 10, y_current_line, FONT_NORMAL_WHITE);
        y_current_line += EXTRA_INFO_LINE_SPACE + EXTRA_INFO_VERTICAL_PADDING;
        text_draw_percentage(extra_widget_data.game_speed, extra_widget_data.x_offset + 60, y_current_line - 2, FONT_NORMAL_GREEN);
        y_current_line += EXTRA_INFO_VERTICAL_PADDING * 3;
    }
    if (extra_widget_data.info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
        y_current_line += EXTRA_INFO_VERTICAL_PADDING;
        lang_text_draw(68, 148, extra_widget_data.x_offset + 10, y_current_line, FONT_NORMAL_WHITE);
        y_current_line += EXTRA_INFO_LINE_SPACE;
        int text_width = text_draw_percentage(extra_widget_data.unemployment_percentage, extra_widget_data.x_offset + 10, y_current_line, FONT_NORMAL_GREEN);
        text_draw_number(extra_widget_data.unemployment_amount, '(', ")", extra_widget_data.x_offset + 10 + text_width, y_current_line, FONT_NORMAL_GREEN);
        y_current_line += EXTRA_INFO_VERTICAL_PADDING * 3;
    }
    if (extra_widget_data.info_to_display & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
        y_current_line += EXTRA_INFO_VERTICAL_PADDING;
        y_current_line += draw_extra_info_objective(extra_widget_data.x_offset, y_current_line, 53, 1, &extra_widget_data.culture);
        y_current_line += draw_extra_info_objective(extra_widget_data.x_offset, y_current_line, 53, 2, &extra_widget_data.prosperity);
        y_current_line += draw_extra_info_objective(extra_widget_data.x_offset, y_current_line, 53, 3, &extra_widget_data.peace);
        y_current_line += draw_extra_info_objective(extra_widget_data.x_offset, y_current_line, 53, 4, &extra_widget_data.favor);
        draw_extra_info_objective(extra_widget_data.x_offset, y_current_line, 4, 6, &extra_widget_data.population);
    }
}

static void draw_extra_info_buttons(void)
{
    if (update_extra_info(0)) {
        // Updates displayed speed % after clicking the arrows
        draw_extra_info_panel();
    }
    if (extra_widget_data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        arrow_buttons_draw(extra_widget_data.x_offset, extra_widget_data.y_offset, arrow_buttons_speed, 2);
    }
}

static void draw_sidebar_remainder(int x_offset, int is_collapsed)
{
    int width = SIDEBAR_EXPANDED_WIDTH;
    if (is_collapsed) {
        width = SIDEBAR_COLLAPSED_WIDTH;
    }
    int available_height = screen_height() - TOP_MENU_HEIGHT - SIDEBAR_MAIN_SECTION_HEIGHT;
    extra_widget_data.is_collapsed = is_collapsed;
    extra_widget_data.x_offset = x_offset;
    extra_widget_data.y_offset = SIDEBAR_FILLER_Y_OFFSET;
    extra_widget_data.width = width;
    if (extra_widget_data.is_collapsed || !config_get(CONFIG_UI_SIDEBAR_INFO) || SIDEBAR_EXTRA_DISPLAY_ALL == SIDEBAR_EXTRA_DISPLAY_NONE) {
        extra_widget_data.info_to_display = SIDEBAR_EXTRA_DISPLAY_NONE;
    } else {
        if (available_height >= EXTRA_INFO_HEIGHT_GAME_SPEED) {
            if (SIDEBAR_EXTRA_DISPLAY_ALL & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
                available_height -= EXTRA_INFO_HEIGHT_GAME_SPEED;
                extra_widget_data.info_to_display |= SIDEBAR_EXTRA_DISPLAY_GAME_SPEED;
            }
        }
        if (available_height >= EXTRA_INFO_HEIGHT_UNEMPLOYMENT) {
            if (SIDEBAR_EXTRA_DISPLAY_ALL & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
                available_height -= EXTRA_INFO_HEIGHT_UNEMPLOYMENT;
                extra_widget_data.info_to_display |= SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT;
            }
        }
        if (available_height >= EXTRA_INFO_HEIGHT_RATINGS) {
            if (SIDEBAR_EXTRA_DISPLAY_ALL & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
                extra_widget_data.info_to_display |= SIDEBAR_EXTRA_DISPLAY_RATINGS;
            }
        }
    }
    extra_widget_data.height = 0;
    if (extra_widget_data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED) {
        extra_widget_data.height += EXTRA_INFO_HEIGHT_GAME_SPEED;
    }
    if (extra_widget_data.info_to_display & SIDEBAR_EXTRA_DISPLAY_UNEMPLOYMENT) {
        extra_widget_data.height += EXTRA_INFO_HEIGHT_UNEMPLOYMENT;
    }
    if (extra_widget_data.info_to_display & SIDEBAR_EXTRA_DISPLAY_RATINGS) {
        extra_widget_data.height += EXTRA_INFO_HEIGHT_RATINGS;
    }
    if (extra_widget_data.info_to_display != SIDEBAR_EXTRA_DISPLAY_NONE) {
        update_extra_info(1);
        draw_extra_info_panel();
    }
    draw_extra_info_buttons();
    int relief_y_offset = SIDEBAR_FILLER_Y_OFFSET + extra_widget_data.height;
    sidebar_common_draw_relief(x_offset, relief_y_offset, GROUP_SIDE_PANEL, is_collapsed);
}

static void draw_overlay_text(int x_offset)
{
    if (game_state_overlay()) {
        lang_text_draw_centered(14, game_state_overlay(), x_offset, 32, 117, FONT_NORMAL_GREEN);
    } else {
        lang_text_draw_centered(6, 4, x_offset, 32, 117, FONT_NORMAL_GREEN);
    }
}

static void draw_expanded_background(int x_offset)
{
    image_draw(image_group(GROUP_SIDE_PANEL) + 1, x_offset, 24);
    draw_buttons_expanded(x_offset);
    draw_overlay_text(x_offset + 4);
    draw_number_of_messages(x_offset);
    image_draw(window_build_menu_image(), x_offset + 6, 239);
    widget_minimap_draw(x_offset + 8, MINIMAP_Y_OFFSET, MINIMAP_WIDTH, MINIMAP_HEIGHT, 1);
    draw_sidebar_remainder(x_offset, 0);
}

static void draw_buttons_collapsed(int x_offset)
{
    image_buttons_draw(x_offset, 24, button_expand_sidebar, 1);
    image_buttons_draw(x_offset, 24, buttons_build_collapsed, 12);
}

static void draw_collapsed_background(void)
{
    int x_offset = screen_width() - SIDEBAR_COLLAPSED_WIDTH;
    image_draw(image_group(GROUP_SIDE_PANEL), x_offset, 24);
    draw_buttons_collapsed(x_offset);
    draw_sidebar_remainder(x_offset, 1);
}

void window_city_draw_background(void)
{
    if (city_view_is_sidebar_collapsed()) {
        draw_collapsed_background();
    } else {
        draw_expanded_background(sidebar_common_get_x_offset_expanded());
    }
    widget_top_menu_draw(1);
}

int center_in_city(int element_width_pixels)
{
    int x, y, width, height;
    city_view_get_viewport(&x, &y, &width, &height);
    int margin = (width - element_width_pixels) / 2;
    return x + margin;
}

static void set_city_clip_rectangle(void)
{
    int x, y, width, height;
    city_view_get_viewport(&x, &y, &width, &height);
    graphics_set_clip_rectangle(x, y, width, height);
}

static void draw_foreground_city(void)
{
    widget_top_menu_draw(0);
    widget_city_draw();
    if (city_view_is_sidebar_collapsed()) {
        int x_offset = screen_width() - SIDEBAR_COLLAPSED_WIDTH;
        draw_buttons_collapsed(x_offset);
    } else {
        int x_offset = sidebar_common_get_x_offset_expanded();
        draw_buttons_expanded(x_offset);
        draw_overlay_text(x_offset + 4);
        widget_minimap_draw(x_offset + 8, MINIMAP_Y_OFFSET, MINIMAP_WIDTH, MINIMAP_HEIGHT, 0);
        draw_number_of_messages(x_offset);
    }
    draw_extra_info_buttons();
    if (window_is(WINDOW_CITY) || window_is(WINDOW_CITY_MILITARY)) {
        if (scenario.time_limit_win_criteria.enabled && !city_data.mission.has_won) {
            int remaining_months;
            if (scenario.start_year + scenario.time_limit_win_criteria.years <= game_time_year() + 1) {
                remaining_months = 12 - game_time_month();
            } else {
                remaining_months = 12 - game_time_month() + 12 * (scenario.start_year + scenario.time_limit_win_criteria.years - game_time_year() - 1);
            }
            label_draw(1, 25, 15, 1);
            int width = lang_text_draw(6, 2, 6, 29, FONT_NORMAL_BLACK);
            text_draw_number(remaining_months, ' ', 0, 6 + width, 29, FONT_NORMAL_BLACK);
        } else if (scenario.survival_time_win_criteria.enabled && !city_data.mission.has_won) {
            int remaining_months;
            if (scenario.start_year + scenario.survival_time_win_criteria.years <= game_time_year() + 1) {
                remaining_months = 12 - game_time_month();
            } else {
                remaining_months = 12 - game_time_month() + 12 * (scenario.start_year + scenario.survival_time_win_criteria.years - game_time_year() - 1);
            }
            label_draw(1, 25, 15, 1);
            int width = lang_text_draw(6, 3, 6, 29, FONT_NORMAL_BLACK);
            text_draw_number(remaining_months, ' ', 0, 6 + width, 29, FONT_NORMAL_BLACK);
        }
        if (game_state_is_paused()) {
            int x_offset = center_in_city(448);
            outer_panel_draw(x_offset, 40, 28, 3);
            text_draw_centered("Game paused", x_offset, 58, 448, FONT_NORMAL_BLACK, COLOR_BLACK);
        }
    }
    if (!scroll_in_progress()) {
        int size_x, size_y;
        int cost = building_construction_cost();
        int has_size = building_construction_size(&size_x, &size_y);
        if (cost && has_size) {
            set_city_clip_rectangle();
            int x, y;
            city_view_get_selected_tile_pixels(&x, &y);
            color_t color;
            if (cost <= city_data.finance.treasury) {
                // Color blind friendly
                color = scenario.climate == CLIMATE_DESERT ? COLOR_FONT_ORANGE : COLOR_FONT_ORANGE_LIGHT;
            } else {
                color = COLOR_FONT_RED;
            }
            text_draw_number_colored(cost, '@', " ", x + 58 + 1, y + 1, FONT_NORMAL_PLAIN, COLOR_BLACK);
            text_draw_number_colored(cost, '@', " ", x + 58, y, FONT_NORMAL_PLAIN, color);
            int width = -text_get_width(string_from_ascii("  "), FONT_SMALL_PLAIN);
            width += text_draw_number_colored(size_x, '@', "x", x - 15 + 1, y + 25 + 1, FONT_SMALL_PLAIN, COLOR_BLACK);
            text_draw_number_colored(size_x, '@', "x", x - 15, y + 25, FONT_SMALL_PLAIN, COLOR_FONT_YELLOW);
            text_draw_number_colored(size_y, '@', " ", x - 15 + width + 1, y + 25 + 1, FONT_SMALL_PLAIN, COLOR_BLACK);
            text_draw_number_colored(size_y, '@', " ", x - 15 + width, y + 25, FONT_SMALL_PLAIN, COLOR_FONT_YELLOW);
            graphics_reset_clip_rectangle();
        }
    }
    if (window_is(WINDOW_CITY)) {
        city_message_process_queue();
    }
}

static void set_construction_building_type(int type)
{
    building_construction_cancel();
    building_construction_set_type(type);
    window_request_refresh();
}

static void draw_background_popup_dialog(void)
{
    window_draw_underlying_window();
    graphics_in_dialog();
    if (popup_dialog_data.has_buttons) {
        outer_panel_draw(80, 80, 30, 10);
    } else {
        outer_panel_draw(80, 80, 30, 7);
    }
    if (popup_dialog_data.type >= 0) {
        lang_text_draw_centered(GROUP, popup_dialog_data.type, 80, 100, 480, FONT_LARGE_BLACK);
        if (lang_text_get_width(GROUP, popup_dialog_data.type + 1, FONT_NORMAL_BLACK) >= 420) {
            lang_text_draw_multiline(GROUP, popup_dialog_data.type + 1, 110, 140, 420, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw_centered(GROUP, popup_dialog_data.type + 1, 80, 140, 480, FONT_NORMAL_BLACK);
        }
    } else {
        lang_text_draw_centered(popup_dialog_data.custom_text_group, popup_dialog_data.custom_text_id, 80, 100, 480, FONT_LARGE_BLACK);
        lang_text_draw_centered(PROCEED_GROUP, PROCEED_TEXT, 80, 140, 480, FONT_NORMAL_BLACK);
    }
    graphics_reset_dialog();
}

static void confirm_popup_dialog(void)
{
    window_go_back();
    popup_dialog_data.close_func();
}

static void button_ok_popup_dialog(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    confirm_popup_dialog();
}

static void button_cancel_popup_dialog(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_go_back();
    // prevent getting stuck on top menu window (last active) when declining pop-up to exit scenario/editor
    if (window_is(WINDOW_TOP_MENU) || window_is(WINDOW_EDITOR_TOP_MENU)) {
        window_go_back();
    }
}

static struct image_button_t popup_dialog_buttons[] = {
    {192, 100, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 0, button_ok_popup_dialog, button_none, 1, 0, 1, 0, 0, 0},
    {256, 100, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 4, button_cancel_popup_dialog, button_none, 0, 0, 1, 0, 0, 0},
};

static void draw_foreground_popup_dialog(void)
{
    graphics_in_dialog();
    if (popup_dialog_data.has_buttons) {
        image_buttons_draw(80, 80, popup_dialog_buttons, sizeof(popup_dialog_buttons) / sizeof(struct image_button_t));
    }
    graphics_reset_dialog();
}

static void handle_input_popup_dialog(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (popup_dialog_data.has_buttons && image_buttons_handle_mouse(mouse_in_dialog(m), 80, 80, popup_dialog_buttons, sizeof(popup_dialog_buttons) / sizeof(struct image_button_t), 0)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        button_cancel_popup_dialog(0, 0);
    }
    if (h->enter_pressed) {
        confirm_popup_dialog();
    }
}

static int init_popup_dialog(int type, int custom_text_group, int custom_text_id, void (*close_func)(void), int has_ok_cancel_buttons)
{
    if (window_is(WINDOW_POPUP_DIALOG)) {
        // don't show popup over popup
        return 0;
    }
    popup_dialog_data.type = type;
    popup_dialog_data.custom_text_group = custom_text_group;
    popup_dialog_data.custom_text_id = custom_text_id;
    popup_dialog_data.ok_clicked = 0;
    popup_dialog_data.close_func = close_func;
    popup_dialog_data.has_buttons = has_ok_cancel_buttons;
    return 1;
}

static void replay_map_confirmed(void)
{
    if (game_file_start_scenario(scenario.scenario_name)) {
        window_city_show();
    }
}

void replay_map(void)
{
    building_construction_clear_type();
    if (init_popup_dialog(POPUP_DIALOG_NONE, 1, 2, replay_map_confirmed, 1)) {
        struct window_type_t window = {
            WINDOW_POPUP_DIALOG,
            draw_background_popup_dialog,
            draw_foreground_popup_dialog,
            handle_input_popup_dialog,
        };
        window_show(&window);
    }
}

static int terrain_on_native_overlay(void)
{
    return
        TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_TREE |
        TERRAIN_GARDEN | TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP | TERRAIN_RUBBLE;
}

static int draw_building_as_deleted_1(struct building_t *b)
{
    if (!config_get(CONFIG_UI_VISUAL_FEEDBACK_ON_DELETE)) {
        return 0;
    }
    b = building_main(b);
    return b->id && (b->is_deleted || map_property_is_deleted(b->grid_offset));
}

static int is_drawable_farmhouse(int grid_offset, int map_orientation)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return 0;
    }
    int xy = map_property_multi_tile_xy(grid_offset);
    if (map_orientation == DIR_0_TOP && xy == EDGE_X0Y1) {
        return 1;
    }
    if (map_orientation == DIR_2_RIGHT && xy == EDGE_X0Y0) {
        return 1;
    }
    if (map_orientation == DIR_4_BOTTOM && xy == EDGE_X1Y0) {
        return 1;
    }
    if (map_orientation == DIR_2_RIGHT && xy == EDGE_X1Y1) {
        return 1;
    }
    return 0;
}

static int is_drawable_farm_corner(int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return 0;
    }

    int map_orientation = city_view_orientation();
    int xy = map_property_multi_tile_xy(grid_offset);
    if (map_orientation == DIR_0_TOP && xy == EDGE_X0Y2) {
        return 1;
    } else if (map_orientation == DIR_2_RIGHT && xy == EDGE_X0Y0) {
        return 1;
    } else if (map_orientation == DIR_4_BOTTOM && xy == EDGE_X2Y0) {
        return 1;
    } else if (map_orientation == DIR_6_LEFT && xy == EDGE_X2Y2) {
        return 1;
    }
    return 0;
}

static int is_problem_cartpusher(int figure_id)
{
    if (figure_id) {
        struct figure_t *fig = &figures[figure_id];
        return fig->action_state == FIGURE_ACTION_CARTPUSHER_INITIAL && fig->min_max_seen;
    } else {
        return 0;
    }
}

static void city_with_overlay_draw_building_top(int x, int y, int grid_offset)
{
    struct building_t *b = &all_buildings[map_building_at(grid_offset)];
    if (overlay->type == OVERLAY_PROBLEMS) {
        if (!b->house_size) {
            if (b->type == BUILDING_FOUNTAIN || b->type == BUILDING_BATHHOUSE) {
                if (!b->has_water_access) {
                    b->show_on_problem_overlay = 1;
                }
            } else if (b->type >= BUILDING_WHEAT_FARM && b->type <= BUILDING_CLAY_PIT) {
                if (is_problem_cartpusher(b->figure_id)) {
                    b->show_on_problem_overlay = 1;
                }
            } else if (building_is_workshop(b->type)) {
                if (is_problem_cartpusher(b->figure_id)) {
                    b->show_on_problem_overlay = 1;
                } else if (b->loads_stored <= 0) {
                    b->show_on_problem_overlay = 1;
                }
            }
        }
    }
    if (overlay->show_building(b)) {
        color_t color_mask = draw_building_as_deleted_1(b) ? COLOR_MASK_RED : 0;
        if (building_is_farm(b->type)) {
            if (is_drawable_farmhouse(grid_offset, city_view_orientation())) {
                image_draw_isometric_top_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
            } else if (map_property_is_draw_tile(grid_offset)) {
                image_draw_isometric_top_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
            }
            return;
        } else if (b->type == BUILDING_GRANARY) {
            const struct image_t *img = image_get(map_image_at(grid_offset));
            image_draw(image_group(GROUP_BUILDING_GRANARY) + 1,
                x + img->sprite_offset_x, y + img->sprite_offset_y - 30 - (img->height - 90));
            if (b->data.granary.resource_stored[RESOURCE_NONE] < 2400) {
                image_draw(image_group(GROUP_BUILDING_GRANARY) + 2, x + 33, y - 60);
                if (b->data.granary.resource_stored[RESOURCE_NONE] < 1800) {
                    image_draw(image_group(GROUP_BUILDING_GRANARY) + 3, x + 56, y - 50);
                }
                if (b->data.granary.resource_stored[RESOURCE_NONE] < 1200) {
                    image_draw(image_group(GROUP_BUILDING_GRANARY) + 4, x + 91, y - 50);
                }
                if (b->data.granary.resource_stored[RESOURCE_NONE] < 600) {
                    image_draw(image_group(GROUP_BUILDING_GRANARY) + 5, x + 117, y - 62);
                }
            }
        } else if (b->type == BUILDING_WAREHOUSE) {
            image_draw(image_group(GROUP_BUILDING_WAREHOUSE) + 17, x - 4, y - 42);
        }
        if (!building_is_farm(b->type)) {
            image_draw_isometric_top_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
        }
    } else {
        int column_height = overlay->get_column_height(b);
        if (column_height != NO_COLUMN) {
            int draw = 1;
            if (building_is_farm(b->type)) {
                draw = is_drawable_farm_corner(grid_offset);
            }
            if (draw) {
                int image_id = image_group(GROUP_OVERLAY_COLUMN);
                if (overlay->column_type == COLUMN_TYPE_RISK) {
                    image_id += 9;
                }
                if (column_height > 10) {
                    column_height = 10;
                }
                int capital_height = image_get(image_id)->height;
                // base
                image_draw(image_id + 2, x + 9, y - 8);
                if (column_height) {
                    // column
                    for (int i = 1; i < column_height; i++) {
                        image_draw(image_id + 1, x + 17, y - 8 - 10 * i + 13);
                    }
                    // capital
                    image_draw(image_id, x + 5, y - 8 - capital_height - 10 * (column_height - 1) + 13);
                }
            }
        }
    }
}

static void draw_top_native(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }
    if (map_terrain_is(grid_offset, terrain_on_native_overlay())) {
        if (!map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
            color_t color_mask = 0;
            if (map_property_is_deleted(grid_offset) && map_property_multi_tile_size(grid_offset) == 1) {
                color_mask = COLOR_MASK_RED;
            }
            image_draw_isometric_top_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
        }
    } else if (map_building_at(grid_offset)) {
        city_with_overlay_draw_building_top(x, y, grid_offset);
    }
}

static void city_with_overlay_draw_building_footprint(int x, int y, int grid_offset, int image_offset)
{
    int building_id = map_building_at(grid_offset);
    if (!building_id) {
        return;
    }
    struct building_t *b = &all_buildings[building_id];
    if (overlay->show_building(b)) {
        if (building_is_farm(b->type)) {
            if (is_drawable_farmhouse(grid_offset, city_view_orientation())) {
                image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, 0);
            } else if (map_property_is_draw_tile(grid_offset)) {
                image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, 0);
            }
        } else {
            image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, 0);
        }
    } else {
        int draw = 1;
        if (b->size == 3 && building_is_farm(b->type)) {
            draw = is_drawable_farm_corner(grid_offset);
        }
        if (draw) {
            int image_base = image_group(GROUP_TERRAIN_OVERLAY) + image_offset;
            if (b->house_size) {
                image_base += 4;
            }
            if (b->size == 1) {
                image_draw_isometric_footprint_from_draw_tile(image_base, x, y, 0);
            } else if (b->size == 2) {
                const int x_tile_offset[] = { 30, 0, 60, 30 };
                const int y_tile_offset[] = { -15, 0, 0, 15 };
                for (int i = 0; i < 4; i++) {
                    image_draw_isometric_footprint_from_draw_tile(image_base + i,
                        x + x_tile_offset[i], y + y_tile_offset[i], 0);
                }
            } else if (b->size == 3) {
                const int image_tile_offset[] = { 0, 1, 2, 1, 3, 2, 3, 3, 3 };
                const int x_tile_offset[] = { 60, 30, 90, 0, 60, 120, 30, 90, 60 };
                const int y_tile_offset[] = { -30, -15, -15, 0, 0, 0, 15, 15, 30 };
                for (int i = 0; i < 9; i++) {
                    image_draw_isometric_footprint_from_draw_tile(image_base + image_tile_offset[i],
                        x + x_tile_offset[i], y + y_tile_offset[i], 0);
                }
            } else if (b->size == 4) {
                const int image_tile_offset[] = { 0, 1, 2, 1, 3, 2, 1, 3, 3, 2, 3, 3, 3, 3, 3, 3 };
                const int x_tile_offset[] = {
                    90,
                    60, 120,
                    30, 90, 150,
                    0, 60, 120, 180,
                    30, 90, 150,
                    60, 120,
                    90
                };
                const int y_tile_offset[] = {
                    -45,
                    -30, -30,
                    -15, -15, -15,
                    0, 0, 0, 0,
                    15, 15, 15,
                    30, 30,
                    45
                };
                for (int i = 0; i < 16; i++) {
                    image_draw_isometric_footprint_from_draw_tile(image_base + image_tile_offset[i],
                        x + x_tile_offset[i], y + y_tile_offset[i], 0);
                }
            } else if (b->size == 5) {
                const int image_tile_offset[] = { 0, 1, 2, 1, 3, 2, 1, 3, 3, 2, 1, 3, 3, 3, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
                const int x_tile_offset[] = {
                    120,
                    90, 150,
                    60, 120, 180,
                    30, 90, 150, 210,
                    0, 60, 120, 180, 240,
                    30, 90, 150, 210,
                    60, 120, 180,
                    90, 150,
                    120
                };
                const int y_tile_offset[] = {
                    -60,
                    -45, -45,
                    -30, -30, -30,
                    -15, -15, -15, -15,
                    0, 0, 0, 0, 0,
                    15, 15, 15, 15,
                    30, 30, 30,
                    45, 45,
                    60
                };
                for (int i = 0; i < 25; i++) {
                    image_draw_isometric_footprint_from_draw_tile(image_base + image_tile_offset[i],
                        x + x_tile_offset[i], y + y_tile_offset[i], 0);
                }
            }
        }
    }
}

static void draw_footprint_native(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }
    if (map_terrain_is(grid_offset, terrain_on_native_overlay())) {
        if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
            city_with_overlay_draw_building_footprint(x, y, grid_offset, 0);
        } else {
            image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, 0);
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT | TERRAIN_WALL)) {
        // display grass
        int image_id = image_group(GROUP_TERRAIN_GRASS_1) + (map_random_get(grid_offset) & 7);
        image_draw_isometric_footprint_from_draw_tile(image_id, x, y, 0);
    } else if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        city_with_overlay_draw_building_footprint(x, y, grid_offset, 0);
    } else {
        if (map_property_is_native_land(grid_offset)) {
            image_draw_isometric_footprint_from_draw_tile(image_group(GROUP_TERRAIN_DESIRABILITY) + 1, x, y, 0);
        } else {
            image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, 0);
        }
    }
}

static int show_building_native(const struct building_t *b)
{
    return b->type == BUILDING_NATIVE_HUT || b->type == BUILDING_NATIVE_MEETING || b->type == BUILDING_MISSION_POST;
}

static int show_figure_native(const struct figure_t *f)
{
    return f->type == FIGURE_INDIGENOUS_NATIVE || f->type == FIGURE_MISSIONARY;
}

static int get_column_height_none(__attribute__((unused)) const struct building_t *b)
{
    return NO_COLUMN;
}

static int show_building_fire_crime(const struct building_t *b)
{
    return b->type == BUILDING_PREFECTURE || b->type == BUILDING_BURNING_RUIN;
}

static int show_building_damage(const struct building_t *b)
{
    return b->type == BUILDING_ENGINEERS_POST;
}

static int show_building_problems(const struct building_t *b)
{
    return b->show_on_problem_overlay;
}

static int show_figure_fire(const struct figure_t *f)
{
    return f->type == FIGURE_PREFECT;
}

static int show_figure_damage(const struct figure_t *f)
{
    return f->type == FIGURE_ENGINEER;
}

static int show_figure_crime(const struct figure_t *f)
{
    return f->type == FIGURE_PREFECT || f->type == FIGURE_PROTESTER ||
        f->type == FIGURE_CRIMINAL || f->type == FIGURE_RIOTER;
}

static int show_figure_problems(const struct figure_t *f)
{
    if (f->type == FIGURE_LABOR_SEEKER) {
        return all_buildings[f->building_id].show_on_problem_overlay;
    } else if (f->type == FIGURE_CART_PUSHER) {
        return f->action_state == FIGURE_ACTION_CARTPUSHER_INITIAL || f->min_max_seen;
    } else {
        return 0;
    }
}

static int get_column_height_fire(const struct building_t *b)
{
    return b->fire_risk > 0 ? b->fire_risk / 10 : NO_COLUMN;
}

static int get_column_height_damage(const struct building_t *b)
{
    return b->damage_risk > 0 ? b->damage_risk / 20 : NO_COLUMN;
}

static int get_column_height_crime(const struct building_t *b)
{
    if (b->house_size) {
        int happiness = b->sentiment.house_happiness;
        if (happiness <= 0) {
            return 10;
        } else if (happiness <= 10) {
            return 8;
        } else if (happiness <= 20) {
            return 6;
        } else if (happiness <= 30) {
            return 4;
        } else if (happiness <= 40) {
            return 2;
        } else if (happiness < 50) {
            return 1;
        }
    }
    return NO_COLUMN;
}

static int show_building_religion(const struct building_t *b)
{
    return
        b->type == BUILDING_ORACLE || b->type == BUILDING_SMALL_TEMPLE_CERES ||
        b->type == BUILDING_SMALL_TEMPLE_NEPTUNE || b->type == BUILDING_SMALL_TEMPLE_MERCURY ||
        b->type == BUILDING_SMALL_TEMPLE_MARS || b->type == BUILDING_SMALL_TEMPLE_VENUS ||
        b->type == BUILDING_LARGE_TEMPLE_CERES || b->type == BUILDING_LARGE_TEMPLE_NEPTUNE ||
        b->type == BUILDING_LARGE_TEMPLE_MERCURY || b->type == BUILDING_LARGE_TEMPLE_MARS ||
        b->type == BUILDING_LARGE_TEMPLE_VENUS;
}

static int show_figure_religion(const struct figure_t *f)
{
    return f->type == FIGURE_PRIEST;
}

static int get_column_height_religion(const struct building_t *b)
{
    return b->house_size && b->data.house.num_gods ? b->data.house.num_gods * 17 / 10 : NO_COLUMN;
}

static int show_building_food_stocks(const struct building_t *b)
{
    return b->type == BUILDING_MARKET || b->type == BUILDING_WHARF || b->type == BUILDING_GRANARY;
}

static const struct city_overlay_t *city_overlay_for_religion(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_RELIGION,
        COLUMN_TYPE_ACCESS,
        show_building_religion,
        show_figure_religion,
        get_column_height_religion,
        0,
        0
    };
    return &overlay;
}

static int get_column_height_food_stocks(const struct building_t *b)
{
    if (b->house_size && house_properties[b->subtype.house_level].food_types) {
        int pop = b->house_population;
        int stocks = 0;
        for (int i = INVENTORY_WHEAT; i <= INVENTORY_MEAT; i++) {
            stocks += b->data.house.inventory[i];
        }
        int pct_stocks = calc_percentage(stocks, pop);
        if (pct_stocks <= 0) {
            return 10;
        } else if (pct_stocks < 100) {
            return 5;
        } else if (pct_stocks <= 200) {
            return 1;
        }
    }
    return NO_COLUMN;
}

static int show_figure_food_stocks(const struct figure_t *f)
{
    if (f->type == FIGURE_MARKET_BUYER || f->type == FIGURE_MARKET_TRADER ||
        f->type == FIGURE_DELIVERY_BOY || f->type == FIGURE_FISHING_BOAT) {
        return 1;
    } else if (f->type == FIGURE_CART_PUSHER) {
        return resource_is_food(f->resource_id);
    }
    return 0;
}

static const struct city_overlay_t *city_overlay_for_food_stocks(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_FOOD_STOCKS,
        COLUMN_TYPE_RISK,
        show_building_food_stocks,
        show_figure_food_stocks,
        get_column_height_food_stocks,
        0,
        0
    };
    return &overlay;
}

static int show_building_tax_income(const struct building_t *b)
{
    return b->type == BUILDING_FORUM || b->type == BUILDING_SENATE;
}

static int show_figure_tax_income(const struct figure_t *f)
{
    return f->type == FIGURE_TAX_COLLECTOR;
}

static int get_column_height_tax_income(const struct building_t *b)
{
    if (b->house_size) {
        int pct = calc_adjust_with_percentage(b->tax_income_or_storage / 2, city_data.finance.tax_percentage);
        if (pct > 0) {
            return pct / 25;
        }
    }
    return NO_COLUMN;
}

static const struct city_overlay_t *city_overlay_for_tax_income(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_TAX_INCOME,
        COLUMN_TYPE_ACCESS,
        show_building_tax_income,
        show_figure_tax_income,
        get_column_height_tax_income,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_fire(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_FIRE,
        COLUMN_TYPE_RISK,
        show_building_fire_crime,
        show_figure_fire,
        get_column_height_fire,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_damage(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_DAMAGE,
        COLUMN_TYPE_RISK,
        show_building_damage,
        show_figure_damage,
        get_column_height_damage,
        0,
        0
    };
    return &overlay;
}

static int show_building_entertainment(const struct building_t *b)
{
    return
        b->type == BUILDING_ACTOR_COLONY || b->type == BUILDING_THEATER ||
        b->type == BUILDING_GLADIATOR_SCHOOL || b->type == BUILDING_AMPHITHEATER ||
        b->type == BUILDING_LION_HOUSE || b->type == BUILDING_COLOSSEUM ||
        b->type == BUILDING_CHARIOT_MAKER || b->type == BUILDING_HIPPODROME;
}

static int show_building_theater(const struct building_t *b)
{
    return b->type == BUILDING_ACTOR_COLONY || b->type == BUILDING_THEATER;
}

static int show_building_amphitheater(const struct building_t *b)
{
    return b->type == BUILDING_ACTOR_COLONY
        || b->type == BUILDING_GLADIATOR_SCHOOL
        || b->type == BUILDING_AMPHITHEATER;
}

static int show_building_colosseum(const struct building_t *b)
{
    return b->type == BUILDING_GLADIATOR_SCHOOL || b->type == BUILDING_LION_HOUSE || b->type == BUILDING_COLOSSEUM;
}

static int show_building_hippodrome(const struct building_t *b)
{
    return b->type == BUILDING_CHARIOT_MAKER || b->type == BUILDING_HIPPODROME;
}

static struct building_t *get_entertainment_building(const struct figure_t *f)
{
    if (f->action_state == FIGURE_ACTION_ENTERTAINER_ROAMING ||
        f->action_state == FIGURE_ACTION_ENTERTAINER_RETURNING) {
        return &all_buildings[f->building_id];
    } else {
        return &all_buildings[f->destination_building_id];
    }
}

static int show_figure_entertainment(const struct figure_t *f)
{
    return f->type == FIGURE_ACTOR || f->type == FIGURE_GLADIATOR ||
        f->type == FIGURE_LION_TAMER || f->type == FIGURE_CHARIOTEER;
}

static int show_figure_theater(const struct figure_t *f)
{
    if (f->type == FIGURE_ACTOR) {
        return get_entertainment_building(f)->type == BUILDING_THEATER;
    }
    return 0;
}

static int show_figure_amphitheater(const struct figure_t *f)
{
    if (f->type == FIGURE_ACTOR || f->type == FIGURE_GLADIATOR) {
        return get_entertainment_building(f)->type == BUILDING_AMPHITHEATER;
    }
    return 0;
}

static int show_figure_colosseum(const struct figure_t *f)
{
    if (f->type == FIGURE_GLADIATOR) {
        return get_entertainment_building(f)->type == BUILDING_COLOSSEUM;
    } else if (f->type == FIGURE_LION_TAMER) {
        return 1;
    }
    return 0;
}

static int show_figure_hippodrome(const struct figure_t *f)
{
    return f->type == FIGURE_CHARIOTEER;
}

static int get_column_height_entertainment(const struct building_t *b)
{
    return b->house_size && b->data.house.entertainment ? b->data.house.entertainment / 10 : NO_COLUMN;
}

static int get_column_height_theater(const struct building_t *b)
{
    return b->house_size && b->data.house.theater ? b->data.house.theater / 10 : NO_COLUMN;
}

static int get_column_height_amphitheater(const struct building_t *b)
{
    return b->house_size && b->data.house.amphitheater_actor ? b->data.house.amphitheater_actor / 10 : NO_COLUMN;
}

static int get_column_height_colosseum(const struct building_t *b)
{
    return b->house_size && b->data.house.colosseum_gladiator ? b->data.house.colosseum_gladiator / 10 : NO_COLUMN;
}

static int get_column_height_hippodrome(const struct building_t *b)
{
    return b->house_size && b->data.house.hippodrome ? b->data.house.hippodrome / 10 : NO_COLUMN;
}

static const struct city_overlay_t *city_overlay_for_entertainment(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_ENTERTAINMENT,
        COLUMN_TYPE_ACCESS,
        show_building_entertainment,
        show_figure_entertainment,
        get_column_height_entertainment,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_theater(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_THEATER,
        COLUMN_TYPE_ACCESS,
        show_building_theater,
        show_figure_theater,
        get_column_height_theater,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_amphitheater(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_AMPHITHEATER,
        COLUMN_TYPE_ACCESS,
        show_building_amphitheater,
        show_figure_amphitheater,
        get_column_height_amphitheater,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_colosseum(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_COLOSSEUM,
        COLUMN_TYPE_ACCESS,
        show_building_colosseum,
        show_figure_colosseum,
        get_column_height_colosseum,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_hippodrome(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_HIPPODROME,
        COLUMN_TYPE_ACCESS,
        show_building_hippodrome,
        show_figure_hippodrome,
        get_column_height_hippodrome,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_crime(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_CRIME,
        COLUMN_TYPE_RISK,
        show_building_fire_crime,
        show_figure_crime,
        get_column_height_crime,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_problems(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_PROBLEMS,
        COLUMN_TYPE_RISK,
        show_building_problems,
        show_figure_problems,
        get_column_height_none,
        0,
        0
    };
    return &overlay;
}

static int terrain_on_desirability_overlay(void)
{
    return
        TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER |
        TERRAIN_TREE | TERRAIN_GARDEN | TERRAIN_ROAD |
        TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP | TERRAIN_RUBBLE;
}

static int get_desirability_image_offset(int desirability)
{
    if (desirability < -10) {
        return 0;
    } else if (desirability < -5) {
        return 1;
    } else if (desirability < 0) {
        return 2;
    } else if (desirability == 1) {
        return 3;
    } else if (desirability < 5) {
        return 4;
    } else if (desirability < 10) {
        return 5;
    } else if (desirability < 15) {
        return 6;
    } else if (desirability < 20) {
        return 7;
    } else if (desirability < 25) {
        return 8;
    } else {
        return 9;
    }
}


static int show_building_barber(const struct building_t *b)
{
    return b->type == BUILDING_BARBER;
}

static int show_building_bathhouse(const struct building_t *b)
{
    return b->type == BUILDING_BATHHOUSE;
}

static int show_building_clinic(const struct building_t *b)
{
    return b->type == BUILDING_DOCTOR;
}

static int show_building_hospital(const struct building_t *b)
{
    return b->type == BUILDING_HOSPITAL;
}

static int show_figure_barber(const struct figure_t *f)
{
    return f->type == FIGURE_BARBER;
}

static int show_figure_bathhouse(const struct figure_t *f)
{
    return f->type == FIGURE_BATHHOUSE_WORKER;
}

static int show_figure_clinic(const struct figure_t *f)
{
    return f->type == FIGURE_DOCTOR;
}

static int show_figure_hospital(const struct figure_t *f)
{
    return f->type == FIGURE_SURGEON;
}

static int get_column_height_barber(const struct building_t *b)
{
    return b->house_size && b->data.house.barber ? b->data.house.barber / 10 : NO_COLUMN;
}

static int get_column_height_bathhouse(const struct building_t *b)
{
    return b->house_size && b->data.house.bathhouse ? b->data.house.bathhouse / 10 : NO_COLUMN;
}

static int get_column_height_clinic(const struct building_t *b)
{
    return b->house_size && b->data.house.clinic ? b->data.house.clinic / 10 : NO_COLUMN;
}

static int get_column_height_hospital(const struct building_t *b)
{
    return b->house_size && b->data.house.hospital ? b->data.house.hospital / 10 : NO_COLUMN;
}
static const struct city_overlay_t *city_overlay_for_barber(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_BARBER,
        COLUMN_TYPE_ACCESS,
        show_building_barber,
        show_figure_barber,
        get_column_height_barber,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_bathhouse(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_BATHHOUSE,
        COLUMN_TYPE_ACCESS,
        show_building_bathhouse,
        show_figure_bathhouse,
        get_column_height_bathhouse,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_clinic(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_CLINIC,
        COLUMN_TYPE_ACCESS,
        show_building_clinic,
        show_figure_clinic,
        get_column_height_clinic,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_hospital(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_HOSPITAL,
        COLUMN_TYPE_ACCESS,
        show_building_hospital,
        show_figure_hospital,
        get_column_height_hospital,
        0,
        0
    };
    return &overlay;
}

static int show_building_education(const struct building_t *b)
{
    return b->type == BUILDING_SCHOOL || b->type == BUILDING_LIBRARY || b->type == BUILDING_ACADEMY;
}

static int show_building_school(const struct building_t *b)
{
    return b->type == BUILDING_SCHOOL;
}

static int show_building_library(const struct building_t *b)
{
    return b->type == BUILDING_LIBRARY;
}

static int show_building_academy(const struct building_t *b)
{
    return b->type == BUILDING_ACADEMY;
}

static int show_figure_education(const struct figure_t *f)
{
    return f->type == FIGURE_SCHOOL_CHILD || f->type == FIGURE_LIBRARIAN || f->type == FIGURE_TEACHER;
}

static int show_figure_school(const struct figure_t *f)
{
    return f->type == FIGURE_SCHOOL_CHILD;
}

static int show_figure_library(const struct figure_t *f)
{
    return f->type == FIGURE_LIBRARIAN;
}

static int show_figure_academy(const struct figure_t *f)
{
    return f->type == FIGURE_TEACHER;
}

static int get_column_height_education(const struct building_t *b)
{
    return b->house_size && b->data.house.education ? b->data.house.education * 3 - 1 : NO_COLUMN;
}

static int get_column_height_school(const struct building_t *b)
{
    return b->house_size && b->data.house.school ? b->data.house.school / 10 : NO_COLUMN;
}

static int get_column_height_library(const struct building_t *b)
{
    return b->house_size && b->data.house.library ? b->data.house.library / 10 : NO_COLUMN;
}

static int get_column_height_academy(const struct building_t *b)
{
    return b->house_size && b->data.house.academy ? b->data.house.academy / 10 : NO_COLUMN;
}

static const struct city_overlay_t *city_overlay_for_education(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_EDUCATION,
        COLUMN_TYPE_ACCESS,
        show_building_education,
        show_figure_education,
        get_column_height_education,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_school(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_SCHOOL,
        COLUMN_TYPE_ACCESS,
        show_building_school,
        show_figure_school,
        get_column_height_school,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_library(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_LIBRARY,
        COLUMN_TYPE_ACCESS,
        show_building_library,
        show_figure_library,
        get_column_height_library,
        0,
        0
    };
    return &overlay;
}

static const struct city_overlay_t *city_overlay_for_academy(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_ACADEMY,
        COLUMN_TYPE_ACCESS,
        show_building_academy,
        show_figure_academy,
        get_column_height_academy,
        0,
        0
    };
    return &overlay;
}

static int terrain_on_water_overlay(void)
{
    return
        TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_TREE |
        TERRAIN_GARDEN | TERRAIN_ROAD | TERRAIN_AQUEDUCT | TERRAIN_ELEVATION |
        TERRAIN_ACCESS_RAMP | TERRAIN_RUBBLE;
}

static int show_building_water(const struct building_t *b)
{
    return b->type == BUILDING_WELL || b->type == BUILDING_FOUNTAIN || b->type == BUILDING_RESERVOIR;
}

static void draw_footprint_water(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }
    if (map_terrain_is(grid_offset, terrain_on_water_overlay())) {
        if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
            city_with_overlay_draw_building_footprint(x, y, grid_offset, 0);
        } else {
            image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, 0);
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_WALL)) {
        // display grass
        int image_id = image_group(GROUP_TERRAIN_GRASS_1) + (map_random_get(grid_offset) & 7);
        image_draw_isometric_footprint_from_draw_tile(image_id, x, y, 0);
    } else if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        struct building_t *b = &all_buildings[map_building_at(grid_offset)];
        int terrain = terrain_grid.items[grid_offset];
        if (b->id && (b->has_well_access || (b->house_size && b->has_water_access))) {
            terrain |= TERRAIN_FOUNTAIN_RANGE;
        }
        int image_offset;
        switch (terrain & (TERRAIN_RESERVOIR_RANGE | TERRAIN_FOUNTAIN_RANGE)) {
            case TERRAIN_RESERVOIR_RANGE | TERRAIN_FOUNTAIN_RANGE:
                image_offset = 24;
                break;
            case TERRAIN_RESERVOIR_RANGE:
                image_offset = 8;
                break;
            case TERRAIN_FOUNTAIN_RANGE:
                image_offset = 16;
                break;
            default:
                image_offset = 0;
                break;
        }
        city_with_overlay_draw_building_footprint(x, y, grid_offset, image_offset);
    } else {
        int image_id = image_group(GROUP_TERRAIN_OVERLAY);
        switch (terrain_grid.items[grid_offset] & (TERRAIN_RESERVOIR_RANGE | TERRAIN_FOUNTAIN_RANGE)) {
            case TERRAIN_RESERVOIR_RANGE | TERRAIN_FOUNTAIN_RANGE:
                image_id += 27;
                break;
            case TERRAIN_RESERVOIR_RANGE:
                image_id += 11;
                break;
            case TERRAIN_FOUNTAIN_RANGE:
                image_id += 19;
                break;
            default:
                image_id = map_image_at(grid_offset);
                break;
        }
        image_draw_isometric_footprint_from_draw_tile(image_id, x, y, 0);
    }
}

static void draw_top_water(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }
    if (map_terrain_is(grid_offset, terrain_on_water_overlay())) {
        if (!map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
            color_t color_mask = 0;
            if (map_property_is_deleted(grid_offset) && map_property_multi_tile_size(grid_offset) == 1) {
                color_mask = COLOR_MASK_RED;
            }
            image_draw_isometric_top_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
        }
    } else if (map_building_at(grid_offset)) {
        city_with_overlay_draw_building_top(x, y, grid_offset);
    }
}

static int show_figure_none(__attribute__((unused)) const struct figure_t *f)
{
    return 0;
}

static const struct city_overlay_t *city_overlay_for_water(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_WATER,
        COLUMN_TYPE_ACCESS,
        show_building_water,
        show_figure_none,
        get_column_height_none,
        draw_footprint_water,
        draw_top_water
    };
    return &overlay;
}

static int has_deleted_building(int grid_offset)
{
    if (!config_get(CONFIG_UI_VISUAL_FEEDBACK_ON_DELETE)) {
        return 0;
    }
    struct building_t *b = &all_buildings[map_building_at(grid_offset)];
    b = building_main(b);
    return b->id && (b->is_deleted || map_property_is_deleted(b->grid_offset));
}

static int show_building_desirability(__attribute__((unused)) const struct building_t *b)
{
    return 0;
}

static void draw_footprint_desirability(int x, int y, int grid_offset)
{
    color_t color_mask = map_property_is_deleted(grid_offset) ? COLOR_MASK_RED : 0;
    if (map_terrain_is(grid_offset, terrain_on_desirability_overlay())
        && !map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        // display normal tile
        if (map_property_is_draw_tile(grid_offset)) {
            image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT | TERRAIN_WALL)) {
        // display empty land/grass
        int image_id = image_group(GROUP_TERRAIN_GRASS_1) + (map_random_get(grid_offset) & 7);
        image_draw_isometric_footprint_from_draw_tile(image_id, x, y, color_mask);
    } else if (map_terrain_is(grid_offset, TERRAIN_BUILDING) || map_desirability_get(grid_offset)) {
        if (has_deleted_building(grid_offset)) {
            color_mask = COLOR_MASK_RED;
        }
        int offset = get_desirability_image_offset(map_desirability_get(grid_offset));
        image_draw_isometric_footprint_from_draw_tile(
            image_group(GROUP_TERRAIN_DESIRABILITY) + offset, x, y, color_mask);
    } else {
        image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
    }
}

static void draw_top_desirability(int x, int y, int grid_offset)
{
    color_t color_mask = map_property_is_deleted(grid_offset) ? COLOR_MASK_RED : 0;
    if (map_terrain_is(grid_offset, terrain_on_desirability_overlay())
        && !map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        // display normal tile
        if (map_property_is_draw_tile(grid_offset)) {
            image_draw_isometric_top_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT | TERRAIN_WALL)) {
        // grass, no top needed
    } else if (map_terrain_is(grid_offset, TERRAIN_BUILDING) || map_desirability_get(grid_offset)) {
        if (has_deleted_building(grid_offset)) {
            color_mask = COLOR_MASK_RED;
        }
        int offset = get_desirability_image_offset(map_desirability_get(grid_offset));
        image_draw_isometric_top_from_draw_tile(image_group(GROUP_TERRAIN_DESIRABILITY) + offset, x, y, color_mask);
    } else {
        image_draw_isometric_top_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
    }
}

static const struct city_overlay_t *city_overlay_for_desirability(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_DESIRABILITY,
        COLUMN_TYPE_ACCESS,
        show_building_desirability,
        show_figure_none,
        get_column_height_none,
        draw_footprint_desirability,
        draw_top_desirability
    };
    return &overlay;
}

static int select_city_overlay(void)
{
    if (!overlay || overlay->type != game_state_overlay()) {
        overlay = 0;
        switch (game_state_overlay()) {
            case OVERLAY_FIRE:
                overlay = city_overlay_for_fire();
                break;
            case OVERLAY_CRIME:
                overlay = city_overlay_for_crime();
                break;
            case OVERLAY_DAMAGE:
                overlay = city_overlay_for_damage();
                break;
            case OVERLAY_PROBLEMS:
                overlay = city_overlay_for_problems();
                break;
            case OVERLAY_NATIVE:
                overlay->type = OVERLAY_NATIVE;
                overlay->column_type = COLUMN_TYPE_RISK;
                overlay->show_building = show_building_native;
                overlay->show_figure = show_figure_native;
                overlay->get_column_height = get_column_height_none;
                overlay->draw_custom_footprint = draw_footprint_native;
                overlay->draw_custom_top = draw_top_native;
                break;
            case OVERLAY_ENTERTAINMENT:
                overlay = city_overlay_for_entertainment();
                break;
            case OVERLAY_THEATER:
                overlay = city_overlay_for_theater();
                break;
            case OVERLAY_AMPHITHEATER:
                overlay = city_overlay_for_amphitheater();
                break;
            case OVERLAY_COLOSSEUM:
                overlay = city_overlay_for_colosseum();
                break;
            case OVERLAY_HIPPODROME:
                overlay = city_overlay_for_hippodrome();
                break;
            case OVERLAY_EDUCATION:
                overlay = city_overlay_for_education();
                break;
            case OVERLAY_SCHOOL:
                overlay = city_overlay_for_school();
                break;
            case OVERLAY_LIBRARY:
                overlay = city_overlay_for_library();
                break;
            case OVERLAY_ACADEMY:
                overlay = city_overlay_for_academy();
                break;
            case OVERLAY_BARBER:
                overlay = city_overlay_for_barber();
                break;
            case OVERLAY_BATHHOUSE:
                overlay = city_overlay_for_bathhouse();
                break;
            case OVERLAY_CLINIC:
                overlay = city_overlay_for_clinic();
                break;
            case OVERLAY_HOSPITAL:
                overlay = city_overlay_for_hospital();
                break;
            case OVERLAY_RELIGION:
                overlay = city_overlay_for_religion();
                break;
            case OVERLAY_TAX_INCOME:
                overlay = city_overlay_for_tax_income();
                break;
            case OVERLAY_FOOD_STOCKS:
                overlay = city_overlay_for_food_stocks();
                break;
            case OVERLAY_WATER:
                overlay = city_overlay_for_water();
                break;
            case OVERLAY_DESIRABILITY:
                overlay = city_overlay_for_desirability();
                break;
            default:
                break;
        }
    }
    return overlay != 0;
}

static void handle_hotkeys_city(const struct hotkeys_t *h)
{
    if (h->load_file) {
        window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_LOAD);
    }
    if (h->save_file) {
        window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_SAVE);
    }
    if (h->decrease_game_speed) {
        setting_decrease_game_speed();
    }
    if (h->increase_game_speed) {
        setting_increase_game_speed();
    }
    if (h->toggle_pause) {
        game_state_toggle_paused();
        city_warning_clear_all();
    }
    if (h->rotate_map_left) {
        game_orientation_rotate_left();
        window_invalidate();
    }
    if (h->rotate_map_right) {
        game_orientation_rotate_right();
        window_invalidate();
    }
    if (h->replay_map) {
        replay_map();
    }
    if (h->cycle_legion) {
        if (city_data.military.total_legions) {
            int selectable_legions_count = 0;
            for (int i = 0; i < MAX_LEGIONS; i++) {
                struct formation_t *m = &legion_formations[i];
                if (m->in_use && !m->in_distant_battle && m->num_figures && m->morale > ROUT_MORALE_THRESHOLD) {
                    selectable_legions_count++;
                }
            }
            // handle wrap around and index mismatches caused by fort delete/recreate, formation rout/destruction, allocation to distant battle
            if (current_selected_legion_index >= selectable_legions_count) {
                current_selected_legion_index = 0;
            }
            int next_available_legion_index = 0;
            for (int i = 0; i < MAX_LEGIONS; i++) {
                struct formation_t *m = &legion_formations[i];
                if (m->in_use && !m->in_distant_battle && m->num_figures && m->morale > ROUT_MORALE_THRESHOLD) {
                    if (next_available_legion_index == current_selected_legion_index) {
                        window_city_military_show(m->id);
                        current_selected_legion_index++;
                        return;
                    }
                    next_available_legion_index++;
                }
            }
        }
    }
    if (h->return_legions_to_fort) {
        for (int i = 0; i < MAX_LEGIONS; i++) {
            if (legion_formations[i].in_use && !legion_formations[i].in_distant_battle) {
                return_legion_formation_home(&legion_formations[i]);
            }
        }
    }
    if (h->show_last_advisor) {
        window_advisors_show(setting_last_advisor());
    }
    if (h->show_empire_map) {
        window_empire_show();
    }
    if (h->show_messages) {
        window_message_list_show();
    }
    if (h->go_to_problem) {
        button_go_to_problem(0, 0);
    }
    if (h->clone_building) {
        int type = BUILDING_NONE;
        if (terrain_grid.items[widget_city_data.current_tile.grid_offset] & TERRAIN_BUILDING) {
            int building_id = map_building_at(widget_city_data.current_tile.grid_offset);
            if (building_id) {
                struct building_t *b = building_main(&all_buildings[building_id]);
                int clone_type = b->type;
                if (building_is_house(clone_type)) {
                    type = BUILDING_HOUSE_VACANT_LOT;
                } else {
                    type = clone_type;
                }
            }
        } else if (terrain_grid.items[widget_city_data.current_tile.grid_offset] & TERRAIN_AQUEDUCT) {
            type = BUILDING_AQUEDUCT;
        } else if (terrain_grid.items[widget_city_data.current_tile.grid_offset] & TERRAIN_WALL) {
            type = BUILDING_WALL;
        } else if (terrain_grid.items[widget_city_data.current_tile.grid_offset] & TERRAIN_GARDEN) {
            type = BUILDING_GARDENS;
        } else if (terrain_grid.items[widget_city_data.current_tile.grid_offset] & TERRAIN_ROAD) {
            if (terrain_grid.items[widget_city_data.current_tile.grid_offset] & TERRAIN_WATER) {
                if (map_sprite_bridge_at(widget_city_data.current_tile.grid_offset) > 6) {
                    type = BUILDING_SHIP_BRIDGE;
                } else {
                    type = BUILDING_LOW_BRIDGE;
                }
            } else if (map_property_is_plaza_or_earthquake(widget_city_data.current_tile.grid_offset)) {
                type = BUILDING_PLAZA;
            } else {
                type = BUILDING_ROAD;
            }
        }
        if (type) {
            set_construction_building_type(type);
        }
    }
    if (h->cycle_buildings) {
        int last_building_type_selected = building_construction_type();
        if (last_building_type_selected < BUILDING_RESERVOIR) {
            last_building_type_selected = BUILDING_RESERVOIR;
        } else {
            last_building_type_selected++;
        }
        while (last_building_type_selected <= BUILDING_WAREHOUSE) {
            if (last_building_type_selected == BUILDING_TRIUMPHAL_ARCH) {
                if (!city_data.building.triumphal_arches_available) {
                    last_building_type_selected++;
                }
            }
            if (scenario.allowed_buildings[last_building_type_selected]) {
                set_construction_building_type(last_building_type_selected);
                break;
            }
            last_building_type_selected++;
        }
    }
    if (h->cycle_buildings_reverse) {
        int last_building_type_selected = building_construction_type();
        if (last_building_type_selected < BUILDING_RESERVOIR) {
            last_building_type_selected = BUILDING_WAREHOUSE;
        } else {
            last_building_type_selected--;
        }
        while (last_building_type_selected >= BUILDING_RESERVOIR) {
            if (last_building_type_selected == BUILDING_TRIUMPHAL_ARCH) {
                if (!city_data.building.triumphal_arches_available) {
                    last_building_type_selected--;
                }
            }
            if (scenario.allowed_buildings[last_building_type_selected]) {
                set_construction_building_type(last_building_type_selected);
                break;
            }
            last_building_type_selected--;
        }
    }
    if (h->undo) {
        game_undo_perform();
    }
    if (h->building) {
        if (h->building == BUILDING_CLEAR_LAND
        || (h->building == BUILDING_TRIUMPHAL_ARCH && city_data.building.triumphal_arches_available && scenario.allowed_buildings[h->building])
        || (h->building != BUILDING_TRIUMPHAL_ARCH && scenario.allowed_buildings[h->building])) {
            set_construction_building_type(h->building);
        }
    }
    if (h->show_overlay) {
        if (game_state_overlay() == h->show_overlay) {
            game_state_set_overlay(OVERLAY_NONE);
        } else {
            game_state_set_overlay(h->show_overlay);
        }
        select_city_overlay();
        window_invalidate();
    }
    if (h->go_to_bookmark) {
        if (map_bookmark_go_to(h->go_to_bookmark - 1)) {
            window_invalidate();
        }
    }
    if (h->set_bookmark) {
        map_bookmark_save(h->set_bookmark - 1);
    }
    if (h->cheat_money) {
        if (city_data.finance.treasury < 50000) {
            city_data.finance.treasury += 1000;
            city_data.finance.cheated_money += 1000;
        }
        window_invalidate();
    }
    if (h->cheat_invasion) {
        start_invasion_by_cheat();
    }
    if (h->cheat_victory) {
        city_victory_force_win();
    }
}

void window_city_draw_all(void)
{
    window_city_draw_background();
    draw_foreground_city();
}

static void draw_background_top_menu(void)
{
    window_city_draw_background();
    widget_city_draw();
}

static void draw_foreground_top_menu(void)
{
    if (top_menu_data.open_sub_menu) {
        menu_draw(&top_menu[top_menu_data.open_sub_menu - 1], top_menu_data.focus_sub_menu_id);
    }
}

static void handle_input_top_menu(const struct mouse_t *m, const struct hotkeys_t *h);

static int widget_top_menu_handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (widget_city_data.capture_input) {
        return 0;
    }
    if (top_menu_data.open_sub_menu) {
        if (m->right.went_up || h->escape_pressed) {
            clear_state_top_menu();
            window_go_back();
            return 1;
        }
        int menu_id = menu_bar_handle_mouse(m, top_menu, sizeof(top_menu) / sizeof(struct menu_bar_item_t), &top_menu_data.focus_menu_id);
        if (menu_id && menu_id != top_menu_data.open_sub_menu) {
            window_request_refresh();
            top_menu_data.open_sub_menu = menu_id;
        }
        if (!menu_handle_mouse(m, &top_menu[top_menu_data.open_sub_menu - 1], &top_menu_data.focus_sub_menu_id)) {
            if (m->left.went_up) {
                clear_state_top_menu();
                window_go_back();
                return 1;
            }
        }
        return 0;
    } else {
        int menu_id = menu_bar_handle_mouse(m, top_menu, sizeof(top_menu) / sizeof(struct menu_bar_item_t), &top_menu_data.focus_menu_id);
        if (menu_id && m->left.went_up) {
            top_menu_data.open_sub_menu = menu_id;
            struct window_type_t window = {
                WINDOW_TOP_MENU,
                draw_background_top_menu,
                draw_foreground_top_menu,
                handle_input_top_menu,
            };
            top_menu[INDEX_OPTIONS].items[0].hidden = 0;
            menu_update_text(&top_menu[INDEX_OPTIONS], 3, setting_monthly_autosave() ? 51 : 52);
            menu_update_text(&top_menu[INDEX_HELP], 1, setting_warnings() ? 6 : 5);
            window_show(&window);
            return 1;
        }
        if (m->right.went_up) {
            int type;
            if (m->y < 4 || m->y >= 18) {
                type = INFO_NONE;
            } else if (m->x > top_menu_data.offset_funds && m->x < top_menu_data.offset_funds + 128) {
                type = INFO_FUNDS;
            } else if (m->x > top_menu_data.offset_population && m->x < top_menu_data.offset_population + 128) {
                type = INFO_POPULATION;
            } else if (m->x > top_menu_data.offset_date && m->x < top_menu_data.offset_date + 128) {
                type = INFO_DATE;
            }
            if (type) {
                if (type == INFO_FUNDS) {
                    window_message_dialog_show(MESSAGE_DIALOG_TOP_FUNDS, window_city_draw_all);
                } else if (type == INFO_POPULATION) {
                    window_message_dialog_show(MESSAGE_DIALOG_TOP_POPULATION, window_city_draw_all);
                } else if (type == INFO_DATE) {
                    window_message_dialog_show(MESSAGE_DIALOG_TOP_DATE, window_city_draw_all);
                }
                return 1;
            }
        }
    }
    return 0;
}

static void handle_input_top_menu(const struct mouse_t *m, const struct hotkeys_t *h)
{
    widget_top_menu_handle_input(m, h);
}

static void build_move(const struct map_tile_t *tile)
{
    if (!building_construction_in_progress()) {
        return;
    }
    building_construction_update(tile->x, tile->y, tile->grid_offset);
}

static void handle_input_city(const struct mouse_t *m, const struct hotkeys_t *h)
{
    handle_hotkeys_city(h);
    if (!building_construction_in_progress()) {
        if (widget_top_menu_handle_input(m, h)) {
            return;
        }
        if (widget_city_data.capture_input) {
            return;
        }
        int handled = 0;
        int button_id;
        if (city_view_is_sidebar_collapsed()) {
            int x_offset = screen_width() - SIDEBAR_COLLAPSED_WIDTH;
            handled |= image_buttons_handle_mouse(m, x_offset, 24, button_expand_sidebar, 1, &button_id);
            handled |= image_buttons_handle_mouse(m, x_offset, 24, buttons_build_collapsed, sizeof(buttons_build_collapsed) / sizeof(struct image_button_t), &button_id);
        } else {
            if (widget_minimap_handle_mouse(m)) {
                return;
            }
            int x_offset = sidebar_common_get_x_offset_expanded();
            handled |= image_buttons_handle_mouse(m, x_offset, 24, buttons_overlays_collapse_sidebar, 2, &button_id);
            handled |= image_buttons_handle_mouse(m, x_offset, 24, buttons_build_expanded, sizeof(buttons_build_expanded) / sizeof(struct image_button_t), &button_id);
            handled |= image_buttons_handle_mouse(m, x_offset, 24, buttons_top_expanded, sizeof(buttons_top_expanded) / sizeof(struct image_button_t), &button_id);
            if (!(extra_widget_data.info_to_display & SIDEBAR_EXTRA_DISPLAY_GAME_SPEED)) {
                return;
            }
            arrow_buttons_handle_mouse(m, extra_widget_data.x_offset, extra_widget_data.y_offset, arrow_buttons_speed, 2, 0);
        }
    }
    scroll_map(m);
    if (h->escape_pressed) {
        if (building_construction_type()) {
            building_construction_cancel();
            window_request_refresh();
        } else {
            window_popup_dialog_show(POPUP_DIALOG_QUIT, confirm_exit_to_main_menu, 1);
        }
    }
    struct map_tile_t *tile = &widget_city_data.current_tile;
    update_city_view_coords(m->x, m->y, tile);
    building_construction_reset_draw_as_constructing();
    if (m->left.went_down) {
        // handle legion click
        if (tile->grid_offset) {
            int formation_id = formation_legion_at_grid_offset(tile->grid_offset);
            if (formation_id > -1 && !legion_formations[formation_id].in_distant_battle) {
                window_city_military_show(formation_id);
                return;
            }
        }
        if (!building_construction_in_progress() && building_construction_type() != BUILDING_NONE) {
            if (tile->grid_offset) { // Allow building on paused
                building_construction_start(tile->x, tile->y, tile->grid_offset);
            }
        }
        build_move(tile);
    } else if (m->left.is_down || building_construction_in_progress()) {
        build_move(tile);
    }
    if (m->left.went_up) {
        if (building_construction_in_progress()) {
            if (building_construction_type() != BUILDING_NONE) {
                play_sound_effect(SOUND_EFFECT_BUILD);
            }
            building_construction_place();
            widget_minimap_invalidate();
        }
    }
    int is_cancel_construction_button;
    if (building_construction_type()) {
        int city_x, city_y, width, height;
        city_view_get_viewport(&city_x, &city_y, &width, &height);
        int touch_width = 5 * BLOCK_SIZE;
        int touch_height = 4 * BLOCK_SIZE;
        int x_offset = width - touch_width;
        int y_offset = 24;
        is_cancel_construction_button = m->x >= x_offset && m->x < x_offset + touch_width && m->y >= y_offset && m->y < y_offset + touch_height;
    }
    int x_offset, y_offset, width, height;
    city_view_get_viewport(&x_offset, &y_offset, &width, &height);
    if (m->right.went_down
    && !(m->x < 5 * BLOCK_SIZE && m->y >= 24 && m->y < 24 + 4 * BLOCK_SIZE) // not pause button
    && !is_cancel_construction_button
    && (m->x - x_offset >= 0 && m->x - x_offset < width && m->y - y_offset >= 0 && m->y - y_offset < height) // coords in city
    && !building_construction_type()) {
        scroll_drag_start();
    }
    if (m->right.went_up) {
        if (!building_construction_type()) {
            int has_scrolled = scroll_drag_end();
            int allow_right_click_info = 1;
            if (!window_is(WINDOW_CITY)) {
                allow_right_click_info = 0;
            }
            window_city_show();
            if (!tile->grid_offset) {
                allow_right_click_info = 0;
            }
            if (allow_right_click_info && city_has_warnings()) {
                city_warning_clear_all();
                allow_right_click_info = 0;
            }
            if (!has_scrolled && allow_right_click_info) {
                window_building_info_show(tile->grid_offset);
            }
        } else {
            building_construction_cancel();
            window_request_refresh();
        }
    }
}

void window_city_show(void)
{
    if (selected_legion_formation > -1) {
        selected_legion_formation = -1;
    }
    struct window_type_t window = {
        WINDOW_CITY,
        window_city_draw_background,
        draw_foreground_city,
        handle_input_city,
    };
    window_show(&window);
}

static void handle_input_military(const struct mouse_t *m, const struct hotkeys_t *h)
{
    handle_hotkeys_city(h);
    if (widget_top_menu_handle_input(m, h)) {
        return;
    }
    struct map_tile_t *tile = &widget_city_data.current_tile;
    update_city_view_coords(m->x, m->y, tile);
    if (!city_view_is_sidebar_collapsed() && widget_minimap_handle_mouse(m)) {
        return;
    }
    scroll_map(m);
    if (m->right.went_up || h->escape_pressed) {
        widget_city_data.capture_input = 0;
        city_warning_clear_all();
        window_city_show();
    } else {
        update_city_view_coords(m->x, m->y, tile);
        if (m->left.went_down) {
            if (!tile->grid_offset) {
                return;
            }
            struct formation_t *legion_formation = &legion_formations[selected_legion_formation];
            if (legion_formation->in_distant_battle || legion_formation->cursed_by_mars) {
                return;
            }
            // return legion home upon clicking on own fort/fort ground
            struct building_t *b = &all_buildings[map_building_at(tile->grid_offset)];
            if (b && b->state == BUILDING_STATE_IN_USE && (building_is_fort(b->type) || b->type == BUILDING_FORT_GROUND) && b->formation_id == selected_legion_formation) {
                return_legion_formation_home(legion_formation);
            } else { // move legion if route available
                map_routing_calculate_distances(legion_formation->standard_x, legion_formation->standard_y);
                if (map_routing_distance(tile->grid_offset)
                && !legion_formation->cursed_by_mars
                && legion_formation->morale > ROUT_MORALE_THRESHOLD
                && legion_formation->num_figures) {
                    move_legion_formation_to(legion_formation, tile);
                } else if (legion_formation->morale <= ROUT_MORALE_THRESHOLD) {
                    city_warning_show(WARNING_LEGION_MORALE_TOO_LOW);
                }
            }
            window_city_show();
        }
    }
}

void window_city_military_show(int legion_formation_id)
{
    if (building_construction_type()) {
        building_construction_cancel();
        building_construction_clear_type();
    }
    selected_legion_formation = legion_formation_id;
    struct window_type_t window = {
        WINDOW_CITY_MILITARY,
        window_city_draw_background,
        draw_foreground_city,
        handle_input_military,

    };
    window_show(&window);
}

void window_city_return(void)
{
    if (selected_legion_formation > -1) {
        window_city_military_show(selected_legion_formation);
    } else {
        window_city_show();
    }
}

static void set_hotkey(int action, int index, int key, int modifiers)
{
    // clear conflicting mappings
    for (int i = 0; i < HOTKEY_MAX_ITEMS; i++) {
        for (int j = 0; j < 2; j++) {
            if (hotkey_config_window_data.mappings[i][j].key == key && hotkey_config_window_data.mappings[i][j].modifiers == modifiers) {
                hotkey_config_window_data.mappings[i][j].key = KEY_TYPE_NONE;
                hotkey_config_window_data.mappings[i][j].modifiers = KEY_MOD_NONE;
            }
        }
    }
    // set new mapping
    hotkey_config_window_data.mappings[action][index].key = key;
    hotkey_config_window_data.mappings[action][index].modifiers = modifiers;
}

static void button_close_hotkey_editor_window(int ok, __attribute__((unused)) int param2)
{
    // destroy window before callback call, because there may appear another popup window
    // by design new popup window can't be showed over another popup window
    window_go_back();
    if (ok) {
        hotkey_editor_window_data.callback(hotkey_editor_window_data.action, hotkey_editor_window_data.index, hotkey_editor_window_data.key, hotkey_editor_window_data.modifiers);
    }
}

static struct generic_button_t bottom_buttons_hotkey_editor_window[] = {
    {192, 228, 120, 24, button_close_hotkey_editor_window, button_none, 0, 0},
    {328, 228, 120, 24, button_close_hotkey_editor_window, button_none, 1, 0},
};

static void draw_background_hotkey_editor_window(void)
{
    window_draw_underlying_window();
    graphics_in_dialog();
    outer_panel_draw(128, 128, 24, 9);
    text_draw_centered(hotkey_editor_bottom_button_strings[0], 136, 144, 376, FONT_LARGE_BLACK, 0);
    for (int i = 0; i < NUM_BOTTOM_BUTTONS_HOTKEY_EDITOR_WINDOW; i++) {
        struct generic_button_t *btn = &bottom_buttons_hotkey_editor_window[i];
        text_draw_centered(hotkey_editor_bottom_button_strings[i + 1], btn->x, btn->y + 6, btn->width, FONT_NORMAL_BLACK, 0);
    }
    graphics_reset_dialog();
}

static const char *key_combination_display_name(int key, int modifiers)
{
    static char result[100];
    result[0] = 0;
    if (modifiers & KEY_MOD_CTRL) {
        strcat(result, "Ctrl");
        strcat(result, " ");
    }
    if (modifiers & KEY_MOD_ALT) {
        strcat(result, "Alt");
        strcat(result, " ");
    }
    if (modifiers & KEY_MOD_GUI) {
        strcat(result, "Gui");
        strcat(result, " ");
    }
    if (modifiers & KEY_MOD_SHIFT) {
        strcat(result, "Shift");
        strcat(result, " ");
    }
    // Modifiers are easy, now for key name...
    SDL_Scancode scan_code;
    switch (key) {
        case KEY_TYPE_A: scan_code = SDL_SCANCODE_A; break;
        case KEY_TYPE_B: scan_code = SDL_SCANCODE_B; break;
        case KEY_TYPE_C: scan_code = SDL_SCANCODE_C; break;
        case KEY_TYPE_D: scan_code = SDL_SCANCODE_D; break;
        case KEY_TYPE_E: scan_code = SDL_SCANCODE_E; break;
        case KEY_TYPE_F: scan_code = SDL_SCANCODE_F; break;
        case KEY_TYPE_G: scan_code = SDL_SCANCODE_G; break;
        case KEY_TYPE_H: scan_code = SDL_SCANCODE_H; break;
        case KEY_TYPE_I: scan_code = SDL_SCANCODE_I; break;
        case KEY_TYPE_J: scan_code = SDL_SCANCODE_J; break;
        case KEY_TYPE_K: scan_code = SDL_SCANCODE_K; break;
        case KEY_TYPE_L: scan_code = SDL_SCANCODE_L; break;
        case KEY_TYPE_M: scan_code = SDL_SCANCODE_M; break;
        case KEY_TYPE_N: scan_code = SDL_SCANCODE_N; break;
        case KEY_TYPE_O: scan_code = SDL_SCANCODE_O; break;
        case KEY_TYPE_P: scan_code = SDL_SCANCODE_P; break;
        case KEY_TYPE_Q: scan_code = SDL_SCANCODE_Q; break;
        case KEY_TYPE_R: scan_code = SDL_SCANCODE_R; break;
        case KEY_TYPE_S: scan_code = SDL_SCANCODE_S; break;
        case KEY_TYPE_T: scan_code = SDL_SCANCODE_T; break;
        case KEY_TYPE_U: scan_code = SDL_SCANCODE_U; break;
        case KEY_TYPE_V: scan_code = SDL_SCANCODE_V; break;
        case KEY_TYPE_W: scan_code = SDL_SCANCODE_W; break;
        case KEY_TYPE_X: scan_code = SDL_SCANCODE_X; break;
        case KEY_TYPE_Y: scan_code = SDL_SCANCODE_Y; break;
        case KEY_TYPE_Z: scan_code = SDL_SCANCODE_Z; break;
        case KEY_TYPE_1: scan_code = SDL_SCANCODE_1; break;
        case KEY_TYPE_2: scan_code = SDL_SCANCODE_2; break;
        case KEY_TYPE_3: scan_code = SDL_SCANCODE_3; break;
        case KEY_TYPE_4: scan_code = SDL_SCANCODE_4; break;
        case KEY_TYPE_5: scan_code = SDL_SCANCODE_5; break;
        case KEY_TYPE_6: scan_code = SDL_SCANCODE_6; break;
        case KEY_TYPE_7: scan_code = SDL_SCANCODE_7; break;
        case KEY_TYPE_8: scan_code = SDL_SCANCODE_8; break;
        case KEY_TYPE_9: scan_code = SDL_SCANCODE_9; break;
        case KEY_TYPE_0: scan_code = SDL_SCANCODE_0; break;
        case KEY_TYPE_ENTER: scan_code = SDL_SCANCODE_RETURN; break;
        case KEY_TYPE_ESCAPE: scan_code = SDL_SCANCODE_ESCAPE; break;
        case KEY_TYPE_BACKSPACE: scan_code = SDL_SCANCODE_BACKSPACE; break;
        case KEY_TYPE_TAB: scan_code = SDL_SCANCODE_TAB; break;
        case KEY_TYPE_SPACE: scan_code = SDL_SCANCODE_SPACE; break;
        case KEY_TYPE_MINUS: scan_code = SDL_SCANCODE_MINUS; break;
        case KEY_TYPE_EQUALS: scan_code = SDL_SCANCODE_EQUALS; break;
        case KEY_TYPE_LEFTBRACKET: scan_code = SDL_SCANCODE_LEFTBRACKET; break;
        case KEY_TYPE_RIGHTBRACKET: scan_code = SDL_SCANCODE_RIGHTBRACKET; break;
        case KEY_TYPE_BACKSLASH: scan_code = SDL_SCANCODE_BACKSLASH; break;
        case KEY_TYPE_SEMICOLON: scan_code = SDL_SCANCODE_SEMICOLON; break;
        case KEY_TYPE_APOSTROPHE: scan_code = SDL_SCANCODE_APOSTROPHE; break;
        case KEY_TYPE_GRAVE: scan_code = SDL_SCANCODE_GRAVE; break;
        case KEY_TYPE_COMMA: scan_code = SDL_SCANCODE_COMMA; break;
        case KEY_TYPE_PERIOD: scan_code = SDL_SCANCODE_PERIOD; break;
        case KEY_TYPE_SLASH: scan_code = SDL_SCANCODE_SLASH; break;
        case KEY_TYPE_CAPSLOCK: scan_code = SDL_SCANCODE_CAPSLOCK; break;
        case KEY_TYPE_F1: scan_code = SDL_SCANCODE_F1; break;
        case KEY_TYPE_F2: scan_code = SDL_SCANCODE_F2; break;
        case KEY_TYPE_F3: scan_code = SDL_SCANCODE_F3; break;
        case KEY_TYPE_F4: scan_code = SDL_SCANCODE_F4; break;
        case KEY_TYPE_F5: scan_code = SDL_SCANCODE_F5; break;
        case KEY_TYPE_F6: scan_code = SDL_SCANCODE_F6; break;
        case KEY_TYPE_F7: scan_code = SDL_SCANCODE_F7; break;
        case KEY_TYPE_F8: scan_code = SDL_SCANCODE_F8; break;
        case KEY_TYPE_F9: scan_code = SDL_SCANCODE_F9; break;
        case KEY_TYPE_F10: scan_code = SDL_SCANCODE_F10; break;
        case KEY_TYPE_F11: scan_code = SDL_SCANCODE_F11; break;
        case KEY_TYPE_F12: scan_code = SDL_SCANCODE_F12; break;
        case KEY_TYPE_INSERT: scan_code = SDL_SCANCODE_INSERT; break;
        case KEY_TYPE_HOME: scan_code = SDL_SCANCODE_HOME; break;
        case KEY_TYPE_PAGEUP: scan_code = SDL_SCANCODE_PAGEUP; break;
        case KEY_TYPE_DELETE: scan_code = SDL_SCANCODE_DELETE; break;
        case KEY_TYPE_END: scan_code = SDL_SCANCODE_END; break;
        case KEY_TYPE_PAGEDOWN: scan_code = SDL_SCANCODE_PAGEDOWN; break;
        case KEY_TYPE_RIGHT: scan_code = SDL_SCANCODE_RIGHT; break;
        case KEY_TYPE_LEFT: scan_code = SDL_SCANCODE_LEFT; break;
        case KEY_TYPE_DOWN: scan_code = SDL_SCANCODE_DOWN; break;
        case KEY_TYPE_UP: scan_code = SDL_SCANCODE_UP; break;
        case KEY_TYPE_KP_1: scan_code = SDL_SCANCODE_KP_1; break;
        case KEY_TYPE_KP_2: scan_code = SDL_SCANCODE_KP_2; break;
        case KEY_TYPE_KP_3: scan_code = SDL_SCANCODE_KP_3; break;
        case KEY_TYPE_KP_4: scan_code = SDL_SCANCODE_KP_4; break;
        case KEY_TYPE_KP_5: scan_code = SDL_SCANCODE_KP_5; break;
        case KEY_TYPE_KP_6: scan_code = SDL_SCANCODE_KP_6; break;
        case KEY_TYPE_KP_7: scan_code = SDL_SCANCODE_KP_7; break;
        case KEY_TYPE_KP_8: scan_code = SDL_SCANCODE_KP_8; break;
        case KEY_TYPE_KP_9: scan_code = SDL_SCANCODE_KP_9; break;
        case KEY_TYPE_KP_0: scan_code = SDL_SCANCODE_KP_0; break;
        case KEY_TYPE_KP_PERIOD: scan_code = SDL_SCANCODE_KP_PERIOD; break;
        case KEY_TYPE_KP_PLUS: scan_code = SDL_SCANCODE_KP_PLUS; break;
        case KEY_TYPE_KP_MINUS: scan_code = SDL_SCANCODE_KP_MINUS; break;
        case KEY_TYPE_KP_MULTIPLY: scan_code = SDL_SCANCODE_KP_MULTIPLY; break;
        case KEY_TYPE_KP_DIVIDE: scan_code = SDL_SCANCODE_KP_DIVIDE; break;
        case KEY_TYPE_NON_US: scan_code = SDL_SCANCODE_NONUSBACKSLASH; break;
        default: scan_code = SDL_SCANCODE_UNKNOWN; break;
    }
    const char *key_name = SDL_GetKeyName(SDL_GetKeyFromScancode(scan_code));
    if ((key_name[0] & 0x80) == 0) {
        // Special cases where we know the key is not displayable using the internal font
        switch (key_name[0]) {
            case '[': key_name = "Left bracket"; break;
            case ']': key_name = "Right bracket"; break;
            case '\\': key_name = "Backslash"; break;
            case '`': key_name = "Backtick"; break;
            case '~': key_name = "Tilde"; break;
            case '#': key_name = "Hash"; break;
            case '$': key_name = "Dollar"; break;
            case '&': key_name = "Ampersand"; break;
            case '<': key_name = "Less than"; break;
            case '>': key_name = "Greater than"; break;
            case '@': key_name = "At-sign"; break;
            case '^': key_name = "Caret"; break;
            case '_': key_name = "Underscore"; break;
            case '|': key_name = "Pipe"; break;
            case '{': key_name = "Left curly brace"; break;
            case '}': key_name = "Right curly brace"; break;
            case '\0': key_name = key_display_names[key];
        }
        strcat(result, key_name);
    } else if (font_can_display(key_name)) {
        strcat(result, key_name);
    } else {
        strcat(result, "? (");
        strcat(result, key_display_names[key]);
        strcat(result, ")");
    }
    return result;
}

static void draw_foreground_hotkey_editor_window(void)
{
    graphics_in_dialog();
    inner_panel_draw(192, 184, 16, 2);
    text_draw_centered(key_combination_display_name(hotkey_editor_window_data.key, hotkey_editor_window_data.modifiers), 192, 193, 256, FONT_NORMAL_WHITE, 0);
    for (int i = 0; i < NUM_BOTTOM_BUTTONS_HOTKEY_EDITOR_WINDOW; i++) {
        struct generic_button_t *btn = &bottom_buttons_hotkey_editor_window[i];
        button_border_draw(btn->x, btn->y, btn->width, btn->height, hotkey_editor_window_data.focus_button == i + 1);
    }
    graphics_reset_dialog();
}

static void handle_input_hotkey_editor_window(const struct mouse_t *m, __attribute__((unused)) const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    int handled = 0;
    handled |= generic_buttons_handle_mouse(m_dialog, 0, 0, bottom_buttons_hotkey_editor_window, NUM_BOTTOM_BUTTONS_HOTKEY_EDITOR_WINDOW, &hotkey_editor_window_data.focus_button);
    if (!handled && m->right.went_up) {
        button_close_hotkey_editor_window(0, 0);
    }
}

static void on_scroll_hotkey_config_window(void)
{
    window_invalidate();
}

static struct scrollbar_type_t scrollbar_hotkey_config_window = { 580, 72, 352, on_scroll_hotkey_config_window, 0, 0, 0, 0, 0, 0 };

static void button_hotkey(int row, int is_alternative)
{
    struct hotkey_widget_t *widget = &hotkey_widgets[row + scrollbar_hotkey_config_window.scroll_position];
    if (widget->action == HOTKEY_HEADER) {
        return;
    }
    struct window_type_t window = {
    WINDOW_HOTKEY_EDITOR,
    draw_background_hotkey_editor_window,
    draw_foreground_hotkey_editor_window,
    handle_input_hotkey_editor_window,
    };
    hotkey_editor_window_data.action = widget->action;
    hotkey_editor_window_data.index = is_alternative;
    hotkey_editor_window_data.callback = set_hotkey;
    hotkey_editor_window_data.key = KEY_TYPE_NONE;
    hotkey_editor_window_data.modifiers = KEY_MOD_NONE;
    hotkey_editor_window_data.focus_button = 0;
    window_show(&window);
}

static struct generic_button_t hotkey_buttons[] = {
    {HOTKEY_X_OFFSET_1, 80 + 24 * 0, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 0, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 0, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 0, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 1, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 1, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 1, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 1, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 2, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 2, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 2, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 2, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 3, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 3, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 3, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 3, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 4, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 4, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 4, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 4, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 5, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 5, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 5, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 5, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 6, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 6, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 6, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 6, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 7, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 7, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 7, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 7, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 8, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 8, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 8, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 8, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 9, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 9, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 9, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 9, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 10, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 10, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 10, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 10, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 11, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 11, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 11, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 11, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 12, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 12, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 12, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 12, 1},
    {HOTKEY_X_OFFSET_1, 80 + 24 * 13, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 13, 0},
    {HOTKEY_X_OFFSET_2, 80 + 24 * 13, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT, button_hotkey, button_none, 13, 1},
};

static void button_reset_defaults_hotkey_config_window(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    for (int action = 0; action < HOTKEY_MAX_ITEMS; action++) {
        for (int index = 0; index < 2; index++) {
            hotkey_config_window_data.mappings[action][index] = *hotkey_default_for_action(action, index);
        }
    }
    window_invalidate();
}

static void button_close_hotkey_config_window(int save, __attribute__((unused)) int param2)
{
    if (!save) {
        window_go_back();
        return;
    }
    hotkey_config_clear();
    for (int action = 0; action < HOTKEY_MAX_ITEMS; action++) {
        for (int index = 0; index < 2; index++) {
            if (hotkey_config_window_data.mappings[action][index].key != KEY_TYPE_NONE) {
                hotkey_config_add_mapping(&hotkey_config_window_data.mappings[action][index]);
            }
        }
    }
    hotkey_config_save();
    window_go_back();
}

static struct generic_button_t bottom_buttons_hotkey_config_window[] = {
    {230, 430, 180, 30, button_reset_defaults_hotkey_config_window, button_none, 0, 0},
    {415, 430, 100, 30, button_close_hotkey_config_window, button_none, 0, 0},
    {520, 430, 100, 30, button_close_hotkey_config_window, button_none, 1, 0},
};

static void draw_version_string(void)
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

static void draw_background_hotkey_config_window(void)
{
    graphics_clear_screen();
    image_draw_fullscreen_background(image_group(GROUP_INTERMEZZO_BACKGROUND) + 16);
    draw_version_string();
    graphics_in_dialog();
    outer_panel_draw(0, 0, 40, 30);
    text_draw_centered(hotkey_strings[0], 16, 16, 608, FONT_LARGE_BLACK, 0);
    text_draw_centered(hotkey_strings[1], HOTKEY_X_OFFSET_1, 55, HOTKEY_BTN_WIDTH, FONT_NORMAL_BLACK, 0);
    text_draw_centered(hotkey_strings[2], HOTKEY_X_OFFSET_2, 55, HOTKEY_BTN_WIDTH, FONT_NORMAL_BLACK, 0);
    inner_panel_draw(20, 72, 35, 22);
    int y_base = 80;
    for (int i = 0; i < NUM_VISIBLE_OPTIONS; i++) {
        int current_pos = i + scrollbar_hotkey_config_window.scroll_position;
        struct hotkey_widget_t *widget = &hotkey_widgets[current_pos];
        int text_offset = y_base + 6 + 24 * i;
        if (current_pos == 0 || current_pos == 5 || current_pos == 12 || current_pos == 25
        || current_pos == 31 || current_pos == 40 || current_pos == 42 || current_pos == 46) { // headers
            text_draw(hotkey_widget_strings[current_pos], 32, text_offset, FONT_NORMAL_WHITE, 0);
        } else {
            if (current_pos <= 51) { // number of entries in hotkey_widget_strings
                text_draw(hotkey_widget_strings[current_pos], 32, text_offset, FONT_NORMAL_GREEN, 0);
            } else {
                int building_index = align_bulding_type_index_to_strings(current_pos - 50);
                text_draw(all_buildings_strings[building_index], 32, text_offset, FONT_NORMAL_GREEN, 0); // reuse building strings
            }
            const struct hotkey_mapping_t *mapping1 = &hotkey_config_window_data.mappings[widget->action][0];
            if (mapping1->key) {
                const char *keyname = key_combination_display_name(mapping1->key, mapping1->modifiers);
                graphics_set_clip_rectangle(HOTKEY_X_OFFSET_1, text_offset, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT);
                text_draw_centered(keyname, HOTKEY_X_OFFSET_1 + 3, text_offset, HOTKEY_BTN_WIDTH - 6, FONT_NORMAL_WHITE, 0);
                graphics_reset_clip_rectangle();
            }
            const struct hotkey_mapping_t *mapping2 = &hotkey_config_window_data.mappings[widget->action][1];
            if (mapping2->key) {
                graphics_set_clip_rectangle(HOTKEY_X_OFFSET_2, text_offset, HOTKEY_BTN_WIDTH, HOTKEY_BTN_HEIGHT);
                const char *keyname = key_combination_display_name(mapping2->key, mapping2->modifiers);
                text_draw_centered(keyname, HOTKEY_X_OFFSET_2 + 3, text_offset, HOTKEY_BTN_WIDTH - 6, FONT_NORMAL_WHITE, 0);
                graphics_reset_clip_rectangle();
            }
        }
    }
    for (int i = 0; i < NUM_BOTTOM_BUTTONS_HOTKEY_CONFIG_WINDOW; i++) {
        text_draw_centered(hotkey_strings[i + 3], bottom_buttons_hotkey_config_window[i].x, bottom_buttons_hotkey_config_window[i].y + 9, bottom_buttons_hotkey_config_window[i].width, FONT_NORMAL_BLACK, 0);
    }
    graphics_reset_dialog();
}

static void draw_foreground_hotkey_config_window(void)
{
    graphics_in_dialog();
    scrollbar_draw(&scrollbar_hotkey_config_window);
    for (int i = 0; i < NUM_VISIBLE_OPTIONS; i++) {
        struct hotkey_widget_t *widget = &hotkey_widgets[i + scrollbar_hotkey_config_window.scroll_position];
        if (widget->action != HOTKEY_HEADER) {
            struct generic_button_t *btn = &hotkey_buttons[2 * i];
            button_border_draw(btn->x, btn->y, btn->width, btn->height, hotkey_config_window_data.focus_button == 1 + 2 * i);
            btn++;
            button_border_draw(btn->x, btn->y, btn->width, btn->height, hotkey_config_window_data.focus_button == 2 + 2 * i);
        }
    }
    for (int i = 0; i < NUM_BOTTOM_BUTTONS_HOTKEY_CONFIG_WINDOW; i++) {
        button_border_draw(bottom_buttons_hotkey_config_window[i].x, bottom_buttons_hotkey_config_window[i].y,
            bottom_buttons_hotkey_config_window[i].width, bottom_buttons_hotkey_config_window[i].height,
            hotkey_config_window_data.bottom_focus_button == i + 1);
    }
    graphics_reset_dialog();
}

static void handle_input_hotkey_config_window(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (scrollbar_handle_mouse(&scrollbar_hotkey_config_window, m_dialog)) {
        return;
    }
    int handled = 0;
    handled |= generic_buttons_handle_mouse(m_dialog, 0, 0,
        hotkey_buttons, NUM_VISIBLE_OPTIONS * 2, &hotkey_config_window_data.focus_button);
    handled |= generic_buttons_handle_mouse(m_dialog, 0, 0,
        bottom_buttons_hotkey_config_window, NUM_BOTTOM_BUTTONS_HOTKEY_CONFIG_WINDOW, &hotkey_config_window_data.bottom_focus_button);
    if (!handled && (m->right.went_up || h->escape_pressed)) {
        window_config_show();
    }
}

static void button_hotkeys(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    struct window_type_t window = {
        WINDOW_HOTKEY_CONFIG,
        draw_background_hotkey_config_window,
        draw_foreground_hotkey_config_window,
        handle_input_hotkey_config_window,
    };
    scrollbar_init(&scrollbar_hotkey_config_window, 0, sizeof(hotkey_widgets) / sizeof(struct hotkey_widget_t) - NUM_VISIBLE_OPTIONS);
    for (int i = 0; i < HOTKEY_MAX_ITEMS; i++) {
        struct hotkey_mapping_t empty = { KEY_TYPE_NONE, KEY_MOD_NONE, i };
        const struct hotkey_mapping_t *mapping = hotkey_for_action(i, 0);
        hotkey_config_window_data.mappings[i][0] = mapping ? *mapping : empty;
        mapping = hotkey_for_action(i, 1);
        hotkey_config_window_data.mappings[i][1] = mapping ? *mapping : empty;
    }
    window_show(&window);
}

static void button_reset_defaults_window_config(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    for (int i = 0; i < CONFIG_MAX_ENTRIES; ++i) {
        window_config_data.config_values[i].new_value = config_get_default_value(i);
    }
    for (int i = 0; i < CONFIG_STRING_MAX_ENTRIES; ++i) {
        string_copy(config_get_default_string_value(i), window_config_data.config_string_values[i].new_value, CONFIG_STRING_VALUE_MAX - 1);
    }
    window_invalidate();
}

static void button_close_window_config(int save, __attribute__((unused)) int param2)
{
    if (!save) {
        for (int i = 0; i < CONFIG_MAX_ENTRIES; i++) {
            window_config_data.config_values[i].new_value = window_config_data.config_values[i].original_value;
        }
        for (int i = 0; i < CONFIG_STRING_MAX_ENTRIES; i++) {
            // memcpy required to fix warning on Switch build
            memcpy(window_config_data.config_string_values[i].new_value,
                window_config_data.config_string_values[i].original_value, CONFIG_STRING_VALUE_MAX - 1);
        }
        window_main_menu_show(0);
        return;
    }
    int apply_changed_configs = 1;
    for (int i = 0; i < CONFIG_MAX_ENTRIES; ++i) {
        if (window_config_data.config_values[i].original_value != window_config_data.config_values[i].new_value) {
            if (!window_config_data.config_values[i].change_action(i)) {
                apply_changed_configs = 0;
                break;
            }
        }
    }
    if (apply_changed_configs) {
        for (int i = 0; i < CONFIG_STRING_MAX_ENTRIES; ++i) {
            if (!string_equals(window_config_data.config_string_values[i].original_value, window_config_data.config_string_values[i].new_value)) {
                if (!window_config_data.config_string_values[i].change_action(i)) {
                    apply_changed_configs = 0;
                    break;
                }
            }
        }
    }
    if (apply_changed_configs) {
        input_box_stop(&player_name_input);
        scenario_settings_set_player_name(window_config_data.config_string_values[CONFIG_STRING_PLAYER_NAME].new_value);
    }
    config_save();
    window_main_menu_show(0);
}

static struct generic_button_t bottom_buttons_window_config[NUM_BOTTOM_BUTTONS_WINDOW_CONFIG] = {
    {20, 480, 180, 30, button_hotkeys, button_none, 0, 0},
    {230, 480, 180, 30, button_reset_defaults_window_config, button_none, 0, 0},
    {415, 480, 100, 30, button_close_window_config, button_none, 0, 0},
    {520, 480, 100, 30, button_close_window_config, button_none, 1, 0},
};

static void on_scroll_window_config(void)
{
    window_invalidate();
}

static struct scrollbar_type_t scrollbar_window_config = { 580, ITEM_Y_OFFSET, ITEM_HEIGHT * NUM_VISIBLE_ITEMS, on_scroll_window_config, 4, 0, 0, 0, 0, 0 };

static void draw_background_window_config(void)
{
    int max_scale = get_max_display_scale();
    scale_ranges[RANGE_DISPLAY_SCALE].max = max_scale;
    if (*scale_ranges[RANGE_DISPLAY_SCALE].value > max_scale) {
        *scale_ranges[RANGE_DISPLAY_SCALE].value = max_scale;
    }
    graphics_clear_screen();
    image_draw_fullscreen_background(image_group(GROUP_INTERMEZZO_BACKGROUND) + 16);
    draw_version_string();
    graphics_in_dialog();
    outer_panel_draw(0, 0, 40, 33);
    text_draw_centered("Brutus configuration options", 16, 16, 608, FONT_LARGE_BLACK, 0);
    int drawn = 0;
    for (int i = 0; i < NUM_VISIBLE_ITEMS && i < window_config_data.num_widgets; i++) {
        struct config_widget_t *w = window_config_data.widgets[i + scrollbar_window_config.scroll_position];
        int y = ITEM_Y_OFFSET + ITEM_HEIGHT * i;
        if (w->type == TYPE_HEADER) {
            text_draw(config_widget_strings[drawn + scrollbar_window_config.scroll_position], 20, y, FONT_NORMAL_BLACK, 0);
            drawn++;
        } else if (w->type == TYPE_CHECKBOX) {
            if (window_config_data.config_values[w->subtype].new_value) {
                text_draw(string_from_ascii("x"), 20 + 6, y + 3, FONT_NORMAL_BLACK, 0);
            }
            text_draw_ellipsized(config_widget_strings[drawn + scrollbar_window_config.scroll_position], 50, y + 5, CHECKBOX_TEXT_WIDTH, FONT_NORMAL_BLACK, 0);
            drawn++;
        } else if (w->type == TYPE_INPUT_BOX) {
            text_draw(config_widget_strings[drawn + scrollbar_window_config.scroll_position], 20, y + 6, FONT_NORMAL_BLACK, 0);
            drawn++;
        } else if (w->type == TYPE_NUMERICAL_RANGE) {
            struct numerical_range_widget_t *ww = &scale_ranges[w->subtype];
            text_draw(w->get_display_text(), NUMERICAL_RANGE_X, y + 6, FONT_NORMAL_BLACK, 0);
            inner_panel_draw(NUMERICAL_RANGE_X + NUMERICAL_SLIDER_X, y + 4, ww->width_blocks, 1);
            int width = ww->width_blocks * BLOCK_SIZE - NUMERICAL_SLIDER_PADDING * 2 - NUMERICAL_DOT_SIZE;
            int scroll_position = (*ww->value - ww->min) * width / (ww->max - ww->min);
            image_draw(image_group(GROUP_PANEL_BUTTON) + 37, NUMERICAL_RANGE_X + NUMERICAL_SLIDER_X + NUMERICAL_SLIDER_PADDING + scroll_position, y + 2);
        } else if (w->type == TYPE_NUMERICAL_DESC) {
            text_draw(config_widget_strings[drawn + scrollbar_window_config.scroll_position], 20, y + 10, FONT_NORMAL_BLACK, 0);
            drawn++;
        }
    }

    for (int i = 0; i < NUM_BOTTOM_BUTTONS_WINDOW_CONFIG; i++) {
        text_draw_centered(config_bottom_button_strings[i], bottom_buttons_window_config[i].x, bottom_buttons_window_config[i].y + 9, bottom_buttons_window_config[i].width, FONT_NORMAL_BLACK, 0);
    }

    graphics_reset_dialog();
}

static void draw_foreground_window_config(void)
{
    graphics_in_dialog();

    for (int i = 0; i < NUM_VISIBLE_ITEMS && i < window_config_data.num_widgets; i++) {
        struct config_widget_t *w = window_config_data.widgets[i + scrollbar_window_config.scroll_position];
        int y = ITEM_Y_OFFSET + ITEM_HEIGHT * i;
        if (w->type == TYPE_CHECKBOX) {
            button_border_draw(20, y, CHECKBOX_CHECK_SIZE, CHECKBOX_CHECK_SIZE, window_config_data.focus_button == i + 1);
        } else if (w->type == TYPE_INPUT_BOX) {
            input_box_draw(&player_name_input);
        }
    }

    for (int i = 0; i < NUM_BOTTOM_BUTTONS_WINDOW_CONFIG; i++) {
        button_border_draw(bottom_buttons_window_config[i].x, bottom_buttons_window_config[i].y,
            bottom_buttons_window_config[i].width, bottom_buttons_window_config[i].height, window_config_data.bottom_focus_button == i + 1);
    }

    if (window_config_data.num_widgets > NUM_VISIBLE_ITEMS) {
        inner_panel_draw(scrollbar_window_config.x + 4, scrollbar_window_config.y + 28, 2, scrollbar_window_config.height / BLOCK_SIZE - 3);
        scrollbar_draw(&scrollbar_window_config);
    }

    graphics_reset_dialog();
}

static int input_box_is_mouse_inside_input(const struct mouse_t *m, const struct input_box_t *box)
{
    return m->x >= box->x && m->x < box->x + box->width_blocks * BLOCK_SIZE &&
        m->y >= box->y && m->y < box->y + box->height_blocks * BLOCK_SIZE;
}

static int numerical_range_handle_mouse(const struct mouse_t *m, int x, int y, int numerical_range_id)
{
    struct numerical_range_widget_t *w = &scale_ranges[numerical_range_id - 1];
    int is_numerical_range = 0;
    if (x + NUMERICAL_SLIDER_X <= m->x && x + w->width_blocks * BLOCK_SIZE + NUMERICAL_SLIDER_X >= m->x && y <= m->y && y + 16 > m->y) {
        is_numerical_range = 1;
    }
    if (window_config_data.active_numerical_range) {
        if (window_config_data.active_numerical_range != numerical_range_id) {
            return 0;
        }
        if (!m->left.is_down) {
            window_config_data.active_numerical_range = 0;
            return 0;
        }
    } else if (!m->left.went_down
    || !is_numerical_range) {
        return 0;
    }
    int slider_width = w->width_blocks * BLOCK_SIZE - NUMERICAL_SLIDER_PADDING * 2 - NUMERICAL_DOT_SIZE;
    int pixels_per_pct = slider_width / (w->max - w->min);
    int dot_position = m->x - x - NUMERICAL_SLIDER_X - NUMERICAL_DOT_SIZE / 2 + pixels_per_pct / 2;

    int exact_value = calc_bound(w->min + dot_position * (w->max - w->min) / slider_width, w->min, w->max);
    int left_step_value = (exact_value / w->step) * w->step;
    int right_step_value = calc_bound(left_step_value + w->step, w->min, w->max);
    int closest_step_value = (exact_value - left_step_value) < (right_step_value - exact_value) ?
        left_step_value : right_step_value;
    if (closest_step_value != *w->value) {
        *w->value = closest_step_value;
        window_request_refresh();
    }
    window_config_data.active_numerical_range = numerical_range_id;
    return 1;
}

static void handle_input_window_config(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (window_config_data.active_numerical_range) {
        numerical_range_handle_mouse(m_dialog, NUMERICAL_RANGE_X, 0, window_config_data.active_numerical_range);
        return;
    }
    if (scrollbar_handle_mouse(&scrollbar_window_config, m_dialog)) {
        return;
    }

    if (m->left.went_up && input_box_is_mouse_inside_input(m_dialog, &player_name_input)) {
        input_box_start(&player_name_input);
    }
    if (m->left.went_up && !input_box_is_mouse_inside_input(m_dialog, &player_name_input)) {
        input_box_stop(&player_name_input);
        scenario_settings_set_player_name(window_config_data.config_string_values[CONFIG_STRING_PLAYER_NAME].new_value);
    }

    int handled = 0;
    window_config_data.focus_button = 0;

    for (int i = 0; i < NUM_VISIBLE_ITEMS && i < window_config_data.num_widgets; i++) {
        struct config_widget_t *w = window_config_data.widgets[i + scrollbar_window_config.scroll_position];
        int y = ITEM_Y_OFFSET + ITEM_HEIGHT * i;
        if (w->type == TYPE_CHECKBOX) {
            int focus = 0;
            if (20 <= m_dialog->x && 20 + CHECKBOX_WIDTH > m_dialog->x && y <= m_dialog->y && y + CHECKBOX_HEIGHT > m_dialog->y) {
                focus = 1;
                if (m_dialog->left.went_up) {
                    window_config_data.config_values[w->subtype].new_value = 1 - window_config_data.config_values[w->subtype].new_value;
                    window_invalidate();
                    handled |= 1;
                } else {
                    handled |= 0;
                }
            }
            if (focus) {
                window_config_data.focus_button = i + 1;
            }
        } else if (w->type == TYPE_NUMERICAL_RANGE) {
            handled |= numerical_range_handle_mouse(m_dialog, NUMERICAL_RANGE_X, y, w->subtype + 1);
        }
    }

    handled |= generic_buttons_handle_mouse(m_dialog, 0, 0,
        bottom_buttons_window_config, NUM_BOTTOM_BUTTONS_WINDOW_CONFIG, &window_config_data.bottom_focus_button);

    if (!handled && (m->right.went_up || h->escape_pressed)) {
        window_main_menu_show(0);
    }
}

static int config_change_string_basic(int key)
{
    config_set_string(key, window_config_data.config_string_values[key].new_value);
    string_copy(window_config_data.config_string_values[key].new_value, window_config_data.config_string_values[key].original_value, CONFIG_STRING_VALUE_MAX - 1);
    return 1;
}

static int config_change_basic(int key)
{
    config_set(key, window_config_data.config_values[key].new_value);
    window_config_data.config_values[key].original_value = window_config_data.config_values[key].new_value;
    return 1;
}

static int config_change_display_scale(int key)
{
    window_config_data.config_values[key].new_value = scale_display(window_config_data.config_values[key].new_value);
    config_change_basic(key);
    return 1;
}

static int config_change_cursor_scale(int key)
{
    config_change_basic(key);
    init_cursors(window_config_data.config_values[key].new_value);
    return 1;
}

static char *percentage_string(char *string, int percentage)
{
    int offset = string_from_int(string, percentage, 0);
    string[offset] = '%';
    string[offset + 1] = 0;
    return string;
}

static const char *display_text_display_scale(void)
{
    static char value[10];
    return percentage_string(value, window_config_data.config_values[CONFIG_SCREEN_DISPLAY_SCALE].new_value);
}

static const char *display_text_cursor_scale(void)
{
    static char value[10];
    return percentage_string(value, window_config_data.config_values[CONFIG_SCREEN_CURSOR_SCALE].new_value);
}

static struct config_widget_t all_widgets[MAX_WIDGETS] = {
    {TYPE_INPUT_BOX, 0, 0, 0},
    {TYPE_NUMERICAL_DESC, RANGE_DISPLAY_SCALE, 0, 0},
    {TYPE_NUMERICAL_RANGE, RANGE_DISPLAY_SCALE, display_text_display_scale, 0},
    {TYPE_NUMERICAL_DESC, RANGE_CURSOR_SCALE, 0, 0},
    {TYPE_NUMERICAL_RANGE, RANGE_CURSOR_SCALE, display_text_cursor_scale, 0},
    {TYPE_SPACE, 0, 0, 0},
    {TYPE_HEADER, 0, 0, 0},
    {TYPE_CHECKBOX, CONFIG_UI_SIDEBAR_INFO, 0, 0},
    {TYPE_CHECKBOX, CONFIG_UI_SHOW_INTRO_VIDEO, 0, 0},
    {TYPE_CHECKBOX, CONFIG_UI_DISABLE_MOUSE_EDGE_SCROLLING, 0, 0},
    {TYPE_CHECKBOX, CONFIG_UI_DISABLE_RIGHT_CLICK_MAP_DRAG, 0, 0},
    {TYPE_CHECKBOX, CONFIG_UI_VISUAL_FEEDBACK_ON_DELETE, 0, 0},
    {TYPE_CHECKBOX, CONFIG_UI_HIGHLIGHT_LEGIONS, 0, 0},
};

static void window_config_show(void)
{
    struct window_type_t window = {
        WINDOW_CONFIG,
        draw_background_window_config,
        draw_foreground_window_config,
        handle_input_window_config,
    };
    if (!window_config_data.config_values[0].change_action) {
        for (int i = 0; i < CONFIG_MAX_ENTRIES; ++i) {
            window_config_data.config_values[i].change_action = config_change_basic;
        }
        for (int i = 0; i < CONFIG_STRING_MAX_ENTRIES; ++i) {
            window_config_data.config_string_values[i].change_action = config_change_string_basic;
        }
        window_config_data.config_values[CONFIG_SCREEN_DISPLAY_SCALE].change_action = config_change_display_scale;
        window_config_data.config_values[CONFIG_SCREEN_CURSOR_SCALE].change_action = config_change_cursor_scale;
        scale_ranges[RANGE_DISPLAY_SCALE].value = &window_config_data.config_values[CONFIG_SCREEN_DISPLAY_SCALE].new_value;
        scale_ranges[RANGE_CURSOR_SCALE].value = &window_config_data.config_values[CONFIG_SCREEN_CURSOR_SCALE].new_value;
    }
    for (int i = 0; i < CONFIG_MAX_ENTRIES; i++) {
        window_config_data.config_values[i].original_value = config_get(i);
        window_config_data.config_values[i].new_value = config_get(i);
    }
    for (int i = 0; i < CONFIG_STRING_MAX_ENTRIES; i++) {
        const char *value = config_get_string(i);
        string_copy(value, window_config_data.config_string_values[i].original_value, CONFIG_STRING_VALUE_MAX - 1);
        string_copy(value, window_config_data.config_string_values[i].new_value, CONFIG_STRING_VALUE_MAX - 1);
    }
    for (int i = 0; i < MAX_WIDGETS; i++) {
        if (all_widgets[i].type) {
            all_widgets[i].enabled = 1;
        }
    }
    window_config_data.num_widgets = 0;
    for (int i = 0; i < MAX_WIDGETS; i++) {
        if (all_widgets[i].enabled) {
            window_config_data.widgets[window_config_data.num_widgets++] = &all_widgets[i];
        }
    }
    scrollbar_init(&scrollbar_window_config, 0, window_config_data.num_widgets - NUM_VISIBLE_ITEMS);
    window_show(&window);
}

static void button_fullscreen(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    post_event(setting_fullscreen() ? USER_EVENT_WINDOWED : USER_EVENT_FULLSCREEN);
}

static void button_reset_window(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    system_resize(1280, 800);
    post_event(USER_EVENT_CENTER_WINDOW);
}

static struct generic_button_t display_top_menu_buttons[] = {
    {128, 136, 224, 20, button_fullscreen, button_none, 1, 0},
    {128, 160, 224, 20, button_reset_window, button_none, 0, 0},
};

static void draw_foreground_display_options(void)
{
    graphics_in_dialog();

    outer_panel_draw(96, 80, 18, 8);

    label_draw(128, 136, 14, display_options_data.focus_button_id == 1 ? 1 : 2);
    label_draw(128, 160, 14, display_options_data.focus_button_id == 2 ? 1 : 2);

    // Display Options
    lang_text_draw_centered(42, 0, 128, 94, 224, FONT_LARGE_BLACK);

    // Full screen/Windowed screen
    lang_text_draw_centered(42, setting_fullscreen() ? 2 : 1, 128, 140, 224, FONT_NORMAL_GREEN);

    // Reset resolution
    text_draw_centered("Reset window", 128, 164, 224, FONT_NORMAL_GREEN, COLOR_BLACK);

    graphics_reset_dialog();
}

static void handle_input_display_options(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, display_top_menu_buttons, sizeof(display_top_menu_buttons) / (sizeof(struct generic_button_t)), &display_options_data.focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        display_options_data.close_callback();
    }
}

void window_display_options_show(void (*close_callback)(void))
{
    struct window_type_t window = {
        WINDOW_DISPLAY_OPTIONS,
        window_draw_underlying_window,
        draw_foreground_display_options,
        handle_input_display_options,
    };
    display_options_data.focus_button_id = 0;
    display_options_data.close_callback = close_callback;
    window_show(&window);
}

static void button_advisor_trade_opened(int advisor, __attribute__((unused)) int param2)
{
    window_advisors_show(advisor);
}

static void button_close_trade_opened(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_empire_show();
}

static struct image_button_t image_buttons_trade_opened[] = {
    {92, 248, 28, 28, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 12, button_advisor_trade_opened, button_none, ADVISOR_TRADE, 0, 1, 0, 0, 0},
    {522, 252, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_close_trade_opened, button_none, 0, 0, 1, 0, 0, 0},
};

static void draw_background_trade_opened(void)
{
    graphics_in_dialog();
    outer_panel_draw(80, 64, 30, 14);
    lang_text_draw_centered(142, 0, 80, 80, 480, FONT_LARGE_BLACK);
    if (empire_window_data.selected_object->is_sea_trade) {
        lang_text_draw_multiline(142, 1, 112, 120, 416, FONT_NORMAL_BLACK);
        lang_text_draw_multiline(142, 3, 112, 184, 416, FONT_NORMAL_BLACK);
    } else {
        lang_text_draw_multiline(142, 1, 112, 152, 416, FONT_NORMAL_BLACK);
    }
    lang_text_draw(142, 2, 128, 256, FONT_NORMAL_BLACK);
    graphics_reset_dialog();
}

static void draw_foreground_trade_opened(void)
{
    graphics_in_dialog();
    image_buttons_draw(0, 0, image_buttons_trade_opened, 2);
    graphics_reset_dialog();
}

static void handle_input_trade_opened(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (image_buttons_handle_mouse(mouse_in_dialog(m), 0, 0, image_buttons_trade_opened, 2, 0)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        window_empire_show();
    }
}

static void confirmed_open_trade(void)
{
    city_finance_process_construction(empire_window_data.selected_object->trade_route_cost);
    empire_window_data.selected_object->trade_route_open = 1;
    struct window_type_t window = {
    WINDOW_TRADE_OPENED,
    draw_background_trade_opened,
    draw_foreground_trade_opened,
    handle_input_trade_opened,
    };
    window_show(&window);
}

static void button_help_empire(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_message_dialog_show(MESSAGE_DIALOG_EMPIRE_MAP, 0);
}

static struct image_button_t image_button_help_empire_window[] = {
    {0, 0, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help_empire, button_none, 0, 0, 1, 0, 0, 0}
};

static void button_return_to_city(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_city_show();
}

static struct image_button_t image_button_return_to_city[] = {
    {0, 0, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_return_to_city, button_none, 0, 0, 1, 0, 0, 0}
};

static struct image_button_t image_button_advisor[] = {
    {-4, 0, 24, 24, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 12, button_advisor_trade_opened, button_none, ADVISOR_TRADE, 0, 1, 0, 0, 0}
};

static void button_show_resource_window(int resource, __attribute__((unused)) int param2)
{
    window_resource_settings_show(resource);
}

static struct generic_button_t generic_button_trade_resource[] = {
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_WHEAT, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_VEGETABLES , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_FRUIT , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_OLIVES , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_VINES , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_MEAT, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_WINE , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_OIL , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_IRON , 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_TIMBER, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_CLAY, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_MARBLE, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_WEAPONS, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_FURNITURE, 0},
    {0, 0, 101, 27, button_show_resource_window, button_none, RESOURCE_POTTERY, 0}
};

static void button_open_trade(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_popup_dialog_show(POPUP_DIALOG_OPEN_TRADE, confirmed_open_trade, 2);
}

static struct generic_button_t generic_button_open_trade[] = {
    {30, 56, 440, 26, button_open_trade, button_none, 0, 0}
};

static void draw_trade_resource(int resource, int trade_max, int x_offset, int y_offset)
{
    graphics_draw_inset_rect(x_offset, y_offset, 26, 26);
    int image_id = resource_images[resource].empire_icon_img_id + resource_image_offset(resource, RESOURCE_IMAGE_ICON);
    image_draw(image_id, x_offset + 1, y_offset + 1);
    if (empire_window_data.focus_resource == resource) {
        button_border_draw(x_offset - 2, y_offset - 2, 101 + 4, 30, 1);
    }
    switch (trade_max) {
        case 15:
            image_draw(image_group(GROUP_TRADE_AMOUNT), x_offset + 21, y_offset - 1);
            break;
        case 25:
            image_draw(image_group(GROUP_TRADE_AMOUNT) + 1, x_offset + 17, y_offset - 1);
            break;
        case 40:
            image_draw(image_group(GROUP_TRADE_AMOUNT) + 2, x_offset + 13, y_offset - 1);
            break;
    }
}

static void draw_background_empire_window_data(void)
{
    int s_width = screen_width();
    int s_height = screen_height();
    empire_window_data.x_min = s_width <= MAX_WIDTH ? 0 : (s_width - MAX_WIDTH) / 2;
    empire_window_data.x_max = s_width <= MAX_WIDTH ? s_width : empire_window_data.x_min + MAX_WIDTH;
    empire_window_data.y_min = s_height <= MAX_HEIGHT ? 0 : (s_height - MAX_HEIGHT) / 2;
    empire_window_data.y_max = s_height <= MAX_HEIGHT ? s_height : empire_window_data.y_min + MAX_HEIGHT;

    if (empire_window_data.x_min || empire_window_data.y_min) {
        graphics_clear_screen();
    }
}

static void draw_foreground_empire_window_data(void)
{
    graphics_set_clip_rectangle(empire_window_data.x_min + 16, empire_window_data.y_min + 16,
        empire_window_data.x_max - empire_window_data.x_min - 32, empire_window_data.y_max - empire_window_data.y_min - 136);
    empire_set_viewport(empire_window_data.x_max - empire_window_data.x_min - 32, empire_window_data.y_max - empire_window_data.y_min - 136);
    empire_window_data.x_draw_offset = empire_window_data.x_min + 16;
    empire_window_data.y_draw_offset = empire_window_data.y_min + 16;
    empire_adjust_scroll(&empire_window_data.x_draw_offset, &empire_window_data.y_draw_offset);
    image_draw(image_group(GROUP_EMPIRE_MAP), empire_window_data.x_draw_offset, empire_window_data.y_draw_offset);
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use) {
            // don't draw trade routes that aren't open
            if (empire_objects[i].type == EMPIRE_OBJECT_LAND_TRADE_ROUTE || empire_objects[i].type == EMPIRE_OBJECT_SEA_TRADE_ROUTE) {
                struct empire_object_t *trade_city = get_trade_city_by_trade_route(empire_objects[i].trade_route_id);
                if (!trade_city->trade_route_open) {
                    continue;
                }
            }
            if (empire_objects[i].type == EMPIRE_OBJECT_BATTLE_ICON) {
                for (int j = 0; j < MAX_INVASIONS; j++) {
                    int battle_icon_year_abs = game_time_year() + empire_objects[i].invasion_years;
                    int invasion_year_abs = scenario.start_year + scenario.invasions[j].year_offset;
                    // check that invasion is yet to come
                    if (scenario.invasions[j].type == INVASION_TYPE_ENEMY_ARMY
                    && (game_time_year() < invasion_year_abs
                        || (game_time_year() == invasion_year_abs && game_time_month() < scenario.invasions[j].month))) {
                        // draw up to 3 battle icons per invasion, 1 per year
                        if (empire_objects[i].invasion_path_id == (j % 3) + 1
                        && (battle_icon_year_abs > invasion_year_abs
                            || (battle_icon_year_abs == invasion_year_abs && game_time_month() >= scenario.invasions[j].month))
                        ) {
                            image_draw(empire_objects[i].image_id, empire_window_data.x_draw_offset + empire_objects[i].x, empire_window_data.y_draw_offset + empire_objects[i].y);
                        }
                    }

                }
                continue;
            }
            if (empire_objects[i].type == EMPIRE_OBJECT_ENEMY_ARMY) {
                if (!city_data.distant_battle.months_until_battle) {
                    continue;
                }
                if (city_data.distant_battle.enemy_months_traveled != empire_objects[i].distant_battle_travel_months) {
                    continue;
                }
            }
            if (empire_objects[i].type == EMPIRE_OBJECT_ROMAN_ARMY) {
                if (!city_military_distant_battle_roman_army_is_traveling()) {
                    continue;
                }
                if (city_data.distant_battle.roman_months_traveled != empire_objects[i].distant_battle_travel_months) {
                    continue;
                }
            }
            int x, y, image_id;
            if (scenario.empire.is_expanded) {
                x = empire_objects[i].expanded.x;
                y = empire_objects[i].expanded.y;
                image_id = empire_objects[i].expanded.image_id;
            } else {
                x = empire_objects[i].x;
                y = empire_objects[i].y;
                image_id = empire_objects[i].image_id;
            }
            if (empire_objects[i].city_type == EMPIRE_CITY_FUTURE_TRADE) {
                // Fix case where future trade city (as specified in the editor) is drawn as a trade city before expansion
                image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
            }
            image_draw(image_id, empire_window_data.x_draw_offset + x, empire_window_data.y_draw_offset + y);
            const struct image_t *img = image_get(image_id);
            if (img->animation_speed_id) {
                image_draw(image_id + empire_object_update_animation(&empire_objects[i], image_id),
                    empire_window_data.x_draw_offset + x + img->sprite_offset_x,
                    empire_window_data.y_draw_offset + y + img->sprite_offset_y);
            }
        }
    }
    graphics_reset_clip_rectangle();
    int image_base = image_group(GROUP_EMPIRE_PANELS);
    // bottom panel background
    graphics_set_clip_rectangle(empire_window_data.x_min, empire_window_data.y_min, empire_window_data.x_max - empire_window_data.x_min, empire_window_data.y_max - empire_window_data.y_min);
    for (int x = empire_window_data.x_min; x < empire_window_data.x_max; x += 70) {
        image_draw(image_base + 3, x, empire_window_data.y_max - 120);
        image_draw(image_base + 3, x, empire_window_data.y_max - 80);
        image_draw(image_base + 3, x, empire_window_data.y_max - 40);
    }
    // horizontal bar borders
    for (int x = empire_window_data.x_min; x < empire_window_data.x_max; x += 86) {
        image_draw(image_base + 1, x, empire_window_data.y_min);
        image_draw(image_base + 1, x, empire_window_data.y_max - 120);
        image_draw(image_base + 1, x, empire_window_data.y_max - 16);
    }
    // vertical bar borders
    for (int y = empire_window_data.y_min + 16; y < empire_window_data.y_max; y += 86) {
        image_draw(image_base, empire_window_data.x_min, y);
        image_draw(image_base, empire_window_data.x_max - 16, y);
    }
    // crossbars
    image_draw(image_base + 2, empire_window_data.x_min, empire_window_data.y_min);
    image_draw(image_base + 2, empire_window_data.x_min, empire_window_data.y_max - 120);
    image_draw(image_base + 2, empire_window_data.x_min, empire_window_data.y_max - 16);
    image_draw(image_base + 2, empire_window_data.x_max - 16, empire_window_data.y_min);
    image_draw(image_base + 2, empire_window_data.x_max - 16, empire_window_data.y_max - 120);
    image_draw(image_base + 2, empire_window_data.x_max - 16, empire_window_data.y_max - 16);
    graphics_reset_clip_rectangle();
    image_buttons_draw(empire_window_data.x_min + 20, empire_window_data.y_max - 44, image_button_help_empire_window, 1);
    image_buttons_draw(empire_window_data.x_max - 44, empire_window_data.y_max - 44, image_button_return_to_city, 1);
    image_buttons_draw(empire_window_data.x_max - 44, empire_window_data.y_max - 100, image_button_advisor, 1);
    if (empire_window_data.selected_object) {
        if (empire_window_data.selected_object->city_type == EMPIRE_CITY_TRADE && !empire_window_data.selected_object->trade_route_open) {
            button_border_draw((empire_window_data.x_min + empire_window_data.x_max - 500) / 2 + 30, empire_window_data.y_max - 49, 440,
                26, empire_window_data.selected_button);
        }
    }
    if (empire_window_data.selected_object) {
        switch (empire_window_data.selected_object->type) {
            case EMPIRE_OBJECT_CITY:
                image_draw(image_base + 6, empire_window_data.x_min + 2, empire_window_data.y_max - 199);
                image_draw(image_base + 7, empire_window_data.x_max - 84, empire_window_data.y_max - 199);
                image_draw(image_base + 8, (empire_window_data.x_min + empire_window_data.x_max - 332) / 2, empire_window_data.y_max - 181);
                lang_text_draw_centered(21, empire_window_data.selected_object->city_name_id,
                    (empire_window_data.x_min + empire_window_data.x_max - 332) / 2 + 64, empire_window_data.y_max - 118, 268, FONT_LARGE_BLACK);
                int x_offset = (empire_window_data.x_min + empire_window_data.x_max - 240) / 2;
                int y_offset = empire_window_data.y_max - 66;
                switch (empire_window_data.selected_object->city_type) {
                    case EMPIRE_CITY_FUTURE_TRADE:
                    case EMPIRE_CITY_DISTANT_ROMAN:
                        lang_text_draw_centered(47, 12, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
                        break;
                    case EMPIRE_CITY_VULNERABLE_ROMAN:
                        if (city_data.distant_battle.city_foreign_months_left <= 0) {
                            lang_text_draw_centered(47, 12, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
                        } else {
                            lang_text_draw_centered(47, 13, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
                        }
                        break;
                    case EMPIRE_CITY_DISTANT_FOREIGN:
                    case EMPIRE_CITY_FUTURE_ROMAN:
                        lang_text_draw_centered(47, 0, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
                        break;
                    case EMPIRE_CITY_OURS:
                        lang_text_draw_centered(47, 1, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
                        break;
                    case EMPIRE_CITY_TRADE:
                        x_offset = (empire_window_data.x_min + empire_window_data.x_max - 500) / 2;
                        y_offset = empire_window_data.y_max - 113;
                        if (empire_window_data.selected_object->trade_route_open) {
                            // city sells
                            lang_text_draw(47, 10, x_offset, y_offset + 40, FONT_NORMAL_GREEN);
                            int index = 0;
                            for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                                if (!empire_window_data.selected_object->resource_sell_limit[r]) {
                                    continue;
                                }
                                int trade_max = empire_window_data.selected_object->resource_sell_limit[r];
                                draw_trade_resource(r, trade_max, x_offset + 104 * index + 76, y_offset + 31);
                                int trade_now = empire_window_data.selected_object->resource_sold[r];
                                if (trade_now > trade_max) {
                                    trade_max = trade_now;
                                }
                                int text_width = text_draw_number(trade_now, '@', "",
                                    x_offset + 104 * index + 106, y_offset + 40, FONT_NORMAL_GREEN);
                                text_width += lang_text_draw(47, 11,
                                    x_offset + 104 * index + 104 + text_width, y_offset + 40, FONT_NORMAL_GREEN);
                                text_draw_number(trade_max, '@', "",
                                    x_offset + 104 * index + 94 + text_width, y_offset + 40, FONT_NORMAL_GREEN);
                                index++;
                            }
                            // city buys
                            lang_text_draw(47, 9, x_offset, y_offset + 71, FONT_NORMAL_GREEN);
                            index = 0;
                            for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                                if (!empire_window_data.selected_object->resource_buy_limit[r]) {
                                    continue;
                                }
                                int trade_max = empire_window_data.selected_object->resource_buy_limit[r];
                                draw_trade_resource(r, trade_max, x_offset + 104 * index + 76, y_offset + 62);
                                int trade_now = empire_window_data.selected_object->resource_bought[r];
                                if (trade_now > trade_max) {
                                    trade_max = trade_now;
                                }
                                int text_width = text_draw_number(trade_now, '@', "",
                                                                 x_offset + 104 * index + 106, y_offset + 71, FONT_NORMAL_GREEN);
                                text_width += lang_text_draw(47, 11,
                                                            x_offset + 104 * index + 104 + text_width, y_offset + 71, FONT_NORMAL_GREEN);
                                text_draw_number(trade_max, '@', "",
                                                 x_offset + 104 * index + 94 + text_width, y_offset + 71, FONT_NORMAL_GREEN);
                                index++;
                            }
                        } else { // trade is closed
                            int index = lang_text_draw(47, 5, x_offset + 35, y_offset + 42, FONT_NORMAL_GREEN);
                            for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                                if (!empire_window_data.selected_object->resource_sell_limit[r]) {
                                    continue;
                                }
                                int trade_max = empire_window_data.selected_object->resource_sell_limit[r];
                                draw_trade_resource(r, trade_max, x_offset + index + 45, y_offset + 33);
                                index += 32;
                            }
                            index += lang_text_draw(47, 4, x_offset + index + 85, y_offset + 42, FONT_NORMAL_GREEN);
                            for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                                if (!empire_window_data.selected_object->resource_buy_limit[r]) {
                                    continue;
                                }
                                int trade_max = empire_window_data.selected_object->resource_buy_limit[r];
                                draw_trade_resource(r, trade_max, x_offset + index + 95, y_offset + 33);
                                index += 32;
                            }
                            index = lang_text_draw_amount(8, 0, empire_window_data.selected_object->trade_route_cost,
                                x_offset + 40, y_offset + 73, FONT_NORMAL_GREEN);
                            lang_text_draw(47, 6, x_offset + index + 40, y_offset + 73, FONT_NORMAL_GREEN);
                            int image_id = image_group(GROUP_EMPIRE_TRADE_ROUTE_TYPE) + 1 - empire_window_data.selected_object->is_sea_trade;
                            image_draw(image_id, x_offset + 430, y_offset + 65 + 2 * empire_window_data.selected_object->is_sea_trade);
                        }
                        break;
                }
                break;
            case EMPIRE_OBJECT_ROMAN_ARMY:
                if (city_military_distant_battle_roman_army_is_traveling()) {
                    if (city_data.distant_battle.roman_months_traveled == empire_window_data.selected_object->distant_battle_travel_months) {
                        x_offset = (empire_window_data.x_min + empire_window_data.x_max - 240) / 2;
                        y_offset = empire_window_data.y_max - 68;
                        int text_id;
                        if (city_data.distant_battle.roman_months_to_travel_forth) {
                            text_id = 15;
                        } else {
                            text_id = 16;
                        }
                        lang_text_draw_multiline(47, text_id, x_offset, y_offset, 240, FONT_NORMAL_GREEN);
                        break;
                    }
                }
                /* fall through */
            case EMPIRE_OBJECT_ENEMY_ARMY:
                if (city_data.distant_battle.months_until_battle) {
                    if (city_data.distant_battle.enemy_months_traveled == empire_window_data.selected_object->distant_battle_travel_months) {
                        lang_text_draw_multiline(47, 14, (empire_window_data.x_min + empire_window_data.x_max - 240) / 2, empire_window_data.y_max - 68, 240, FONT_NORMAL_GREEN);
                        break;
                    }
                }
                /* fall through */
            default:
                lang_text_draw_centered(47, 8, empire_window_data.x_min, empire_window_data.y_max - 65, empire_window_data.x_max - empire_window_data.x_min, FONT_NORMAL_GREEN);
        }
    } else {
        lang_text_draw_centered(47, 8, empire_window_data.x_min, empire_window_data.y_max - 65, empire_window_data.x_max - empire_window_data.x_min, FONT_NORMAL_GREEN);
    }
}

static void handle_input_empire_window_data(const struct mouse_t *m, const struct hotkeys_t *h)
{
    struct pixel_view_coordinates_t position;
    if (scroll_get_delta(m, &position, SCROLL_TYPE_EMPIRE)) {
        empire_scroll_map(position.x, position.y);
    }
    empire_window_data.focus_button_id = 0;
    empire_window_data.focus_resource = 0;
    int button_id;
    image_buttons_handle_mouse(m, empire_window_data.x_min + 20, empire_window_data.y_max - 44, image_button_help_empire_window, 1, &button_id);
    if (button_id) {
        empire_window_data.focus_button_id = 1;
    }
    image_buttons_handle_mouse(m, empire_window_data.x_max - 44, empire_window_data.y_max - 44, image_button_return_to_city, 1, &button_id);
    if (button_id) {
        empire_window_data.focus_button_id = 2;
    }
    image_buttons_handle_mouse(m, empire_window_data.x_max - 44, empire_window_data.y_max - 100, image_button_advisor, 1, &button_id);
    if (button_id) {
        empire_window_data.focus_button_id = 3;
    }
    button_id = 0;

    if (h->show_last_advisor) {
        window_advisors_show(setting_last_advisor());
        return;
    }

    if (h->show_empire_map) {
        window_city_show();
        return;
    }

    if (m->left.went_up
    && !(m->x < empire_window_data.x_min + 16 || m->x >= empire_window_data.x_max - 16 || // is outside map
        m->y < empire_window_data.y_min + 16 || m->y >= empire_window_data.y_max - 120)) {
        empire_window_data.selected_object = empire_select_object(m->x - empire_window_data.x_min - 16, m->y - empire_window_data.y_min - 16);
    }
    if (empire_window_data.selected_object) {
        window_invalidate();
        if (empire_window_data.selected_object->city_type == EMPIRE_CITY_TRADE) {
            if (empire_window_data.selected_object->trade_route_open) {
                int x_offset = (empire_window_data.x_min + empire_window_data.x_max - 500) / 2;
                int y_offset = empire_window_data.y_max - 113;
                int index_sell = 0;
                int index_buy = 0;

                // we only want to handle resource buttons that the selected city trades
                for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                    if (empire_window_data.selected_object->resource_sell_limit[r]) {
                        generic_buttons_handle_mouse(m, x_offset + 75 + 104 * index_sell, y_offset + 31,
                            generic_button_trade_resource + r - 1, 1, &button_id);
                        index_sell++;
                    } else if (empire_window_data.selected_object->resource_buy_limit[r]) {
                        generic_buttons_handle_mouse(m, x_offset + 75 + 104 * index_buy, y_offset + 62,
                            generic_button_trade_resource + r - 1, 1, &button_id);
                        index_buy++;
                    }

                    if (button_id) {
                        empire_window_data.focus_resource = r;
                        // if we're focusing any button we can skip further checks
                        break;
                    }
                }
            } else {
                generic_buttons_handle_mouse(
                    m, (empire_window_data.x_min + empire_window_data.x_max - 500) / 2, empire_window_data.y_max - 105,
                    generic_button_open_trade, 1, &empire_window_data.selected_button);
            }
        }
        // allow de-selection only for objects that are currently selected/drawn
        if (m->right.went_up || h->escape_pressed) {
            switch (empire_window_data.selected_object->type) {
                case EMPIRE_OBJECT_CITY:
                    empire_window_data.selected_object = 0;
                    window_invalidate();
                    break;
                case EMPIRE_OBJECT_ROMAN_ARMY:
                    if (city_military_distant_battle_roman_army_is_traveling()) {
                        if (city_data.distant_battle.roman_months_traveled == empire_window_data.selected_object->distant_battle_travel_months) {
                            empire_window_data.selected_object = 0;
                            window_invalidate();
                            break;
                        }
                    }
                    /* fall through */
                case EMPIRE_OBJECT_ENEMY_ARMY:
                    if (city_data.distant_battle.months_until_battle) {
                        if (city_data.distant_battle.enemy_months_traveled == empire_window_data.selected_object->distant_battle_travel_months) {
                            empire_window_data.selected_object = 0;
                            window_invalidate();
                            break;
                        }
                    }
                    /* fall through */
                default:
                    window_go_back();
            }
        }
    } else if (m->right.went_up || h->escape_pressed) {
        window_go_back();
        if (window_is(WINDOW_TRADE_OPENED)) {
            window_city_show();
        }
    }
}

void window_empire_show(void)
{
    struct window_type_t window = {
        WINDOW_EMPIRE,
        draw_background_empire_window_data,
        draw_foreground_empire_window_data,
        handle_input_empire_window_data,
    };
    empire_window_data.selected_button = 0;
    empire_window_data.selected_object = 0;
    empire_window_data.focus_button_id = 0;
    window_show(&window);
}

static void on_scroll_file_dialog(void)
{
    file_dialog_data.message_not_exist_start_time = 0;
}

static struct scrollbar_type_t scrollbar_file_dialog = { 464, 120, 206, on_scroll_file_dialog, 0, 0, 0, 0, 0, 0 };

static void button_ok_cancel(int is_ok, __attribute__((unused)) int param2)
{
    if (!is_ok) {
        input_box_stop(&file_name_input);
        window_go_back();
        return;
    }
    const char *filename;
    char selected_name[FILE_NAME_MAX];
    string_copy(file_dialog_data.selected_file, selected_name, FILE_NAME_MAX);
    file_remove_extension(selected_name);
    if (string_equals(selected_name, file_dialog_data.typed_name)) {
        // User has not modified the string after selecting it: use filename
        filename = file_dialog_data.selected_file;
    } else {
        // We should use the typed name
        static char typed_file[FILE_NAME_MAX];
        string_copy(file_dialog_data.typed_name, typed_file, FILE_NAME_MAX);
        file_append_extension(typed_file, file_dialog_data.file_data->extension);
        filename = typed_file;
    }
    if (file_dialog_data.type == FILE_TYPE_SAVED_GAME) {
        if (file_dialog_data.dialog_type != FILE_DIALOG_SAVE && !file_exists(SAVES_DIR_PATH, filename)) {
            file_dialog_data.message_not_exist_start_time = time_get_millis();
            return;
        }
        if (file_dialog_data.dialog_type == FILE_DIALOG_SAVE) {
            input_box_stop(&file_name_input);
            game_file_io_write_saved_game(SAVES_DIR_PATH, filename);
            window_city_show();
        } else if (file_dialog_data.dialog_type == FILE_DIALOG_LOAD) {
            if (game_file_load_saved_game(SAVES_DIR_PATH, filename)) {
                input_box_stop(&file_name_input);
                window_city_show();
            } else {
                file_dialog_data.message_not_exist_start_time = time_get_millis();
                return;
            }
        } else if (file_dialog_data.dialog_type == FILE_DIALOG_DELETE) {
            if (game_file_io_delete_saved_game(filename)) {
                dir_list_files(file_dialog_data.file_data->extension);
                if (scrollbar_file_dialog.scroll_position + NUM_FILES_IN_VIEW >= file_dialog_data.file_list->num_files) {
                    --scrollbar_file_dialog.scroll_position;
                }
                if (scrollbar_file_dialog.scroll_position < 0) {
                    scrollbar_file_dialog.scroll_position = 0;
                }
            }
        }
    } else if (file_dialog_data.type == FILE_TYPE_SCENARIO) {
        if (file_dialog_data.dialog_type != FILE_DIALOG_SAVE && !file_exists(MAPS_DIR_PATH, filename)) {
            file_dialog_data.message_not_exist_start_time = time_get_millis();
            return;
        }
        if (file_dialog_data.dialog_type == FILE_DIALOG_SAVE) {
            input_box_stop(&file_name_input);
            game_file_editor_write_scenario(MAPS_DIR_PATH, filename);
            show_editor_map();
        } else if (file_dialog_data.dialog_type == FILE_DIALOG_LOAD) {
            if (game_file_editor_load_scenario(MAPS_DIR_PATH, filename)) {
                input_box_stop(&file_name_input);
                show_editor_map();
            } else {
                file_dialog_data.message_not_exist_start_time = time_get_millis();
                return;
            }
        }
    }
    play_sound_effect(SOUND_EFFECT_ICON);
    string_copy(filename, file_dialog_data.file_data->last_loaded_file, FILE_NAME_MAX - 1);
}

static struct image_button_t image_buttons_file_dialog[] = {
    {344, 335, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 0, button_ok_cancel, button_none, 1, 0, 1, 0, 0, 0},
    {392, 335, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 4, button_ok_cancel, button_none, 0, 0, 1, 0, 0, 0},
};

static void scroll_to_typed_text(void)
{
    if (file_dialog_data.file_list->num_files <= NUM_FILES_IN_VIEW) {
        // No need to scroll
        return;
    }
    int index;
    int len = string_length((const char *) file_dialog_data.typed_name);
    if (!len) {
        index = -1;
    } else {
        int left = 0;
        int right = file_dialog_data.file_list->num_files;
        while (left < right) {
            int middle = (left + right) / 2;
            if (strncmp(file_dialog_data.file_list->files[middle], (const char *) file_dialog_data.typed_name, len) >= 0) {
                right = middle;
            } else {
                left = middle + 1;
            }
        }
        if (strncmp(file_dialog_data.file_list->files[left], (const char *) file_dialog_data.typed_name, len) == 0) {
            index = left;
        } else {
            index = -1;
        }
    }
    if (index >= 0) {
        scrollbar_reset(&scrollbar_file_dialog, calc_bound(index, 0, file_dialog_data.file_list->num_files - NUM_FILES_IN_VIEW));
    }
}

static void draw_foreground_file_dialog(void)
{
    graphics_in_dialog();
    char file[FILE_NAME_MAX];
    outer_panel_draw(128, 40, 22, 21);
    input_box_draw(&file_name_input);
    inner_panel_draw(144, 120, 20, 13);
    // title
    if (file_dialog_data.message_not_exist_start_time
        && time_get_millis() - file_dialog_data.message_not_exist_start_time < NOT_EXIST_MESSAGE_TIMEOUT) {
        lang_text_draw_centered(43, 2, 160, 50, 304, FONT_LARGE_BLACK);
    } else if (file_dialog_data.dialog_type == FILE_DIALOG_DELETE) {
        lang_text_draw_centered(43, 6, 160, 50, 304, FONT_LARGE_BLACK);
    } else {
        int text_id = file_dialog_data.dialog_type + (file_dialog_data.type == FILE_TYPE_SCENARIO ? 3 : 0);
        lang_text_draw_centered(43, text_id, 160, 50, 304, FONT_LARGE_BLACK);
    }
    lang_text_draw(43, 5, 224, 342, FONT_NORMAL_BLACK);
    for (int i = 0; i < NUM_FILES_IN_VIEW; i++) {
        if (i >= file_dialog_data.file_list->num_files) {
            break;
        }
        int font = FONT_NORMAL_GREEN;
        if (file_dialog_data.focus_button_id == i + 1) {
            font = FONT_NORMAL_WHITE;
        }
        string_copy(file_dialog_data.file_list->files[scrollbar_file_dialog.scroll_position + i], file, FILE_NAME_MAX);
        file_remove_extension(file);
        text_ellipsize(file, font, MAX_FILE_WINDOW_TEXT_WIDTH);
        text_draw(file, 160, 130 + 16 * i, font, 0);
    }
    if (file_dialog_data.file_list->file_overflow) {
        inner_panel_draw(184, 22, 15, 1);
        text_draw_centered(too_many_files_string, 184, 25, 240, FONT_NORMAL_PLAIN, COLOR_RED);
    }
    image_buttons_draw(0, 0, image_buttons_file_dialog, 2);
    scrollbar_draw(&scrollbar_file_dialog);
    graphics_reset_dialog();
}

static void button_select_file(int index, __attribute__((unused)) int param2)
{
    if (index < file_dialog_data.file_list->num_files) {
        string_copy(file_dialog_data.file_list->files[scrollbar_file_dialog.scroll_position + index], file_dialog_data.selected_file, FILE_NAME_MAX - 1);
        string_copy(file_dialog_data.selected_file, file_dialog_data.typed_name, FILE_NAME_MAX);
        file_remove_extension(file_dialog_data.typed_name);
        string_copy(file_dialog_data.typed_name, file_dialog_data.previously_seen_typed_name, FILE_NAME_MAX);
        keyboard_refresh();
        file_dialog_data.message_not_exist_start_time = 0;
    }
    if (file_dialog_data.dialog_type != FILE_DIALOG_DELETE && file_dialog_data.double_click) {
        file_dialog_data.double_click = 0;
        button_ok_cancel(1, 0);
    }
}

static struct generic_button_t file_buttons_file_dialog[] = {
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

static void handle_input_file_dialog(const struct mouse_t *m, const struct hotkeys_t *h)
{
    file_dialog_data.double_click = m->left.double_click;
    if (keyboard_input_is_accepted()) {
        button_ok_cancel(1, 0);
        return;
    }
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, file_buttons_file_dialog, NUM_FILES_IN_VIEW, &file_dialog_data.focus_button_id) ||
        image_buttons_handle_mouse(m_dialog, 0, 0, image_buttons_file_dialog, 2, 0) ||
        scrollbar_handle_mouse(&scrollbar_file_dialog, m_dialog)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        input_box_stop(&file_name_input);
        window_go_back();
    }
    if (string_equals(file_dialog_data.previously_seen_typed_name, file_dialog_data.typed_name)) {
        return;
    } else {
        // Only scroll when adding characters to the typed name
        if (string_length(file_dialog_data.typed_name) > string_length(file_dialog_data.previously_seen_typed_name)) {
            scroll_to_typed_text();
        }
        string_copy(file_dialog_data.typed_name, file_dialog_data.previously_seen_typed_name, FILE_NAME_MAX);
    }
}

void window_file_dialog_show(int type, int dialog_type)
{
    struct window_type_t window = {
        WINDOW_FILE_DIALOG,
        0,
        draw_foreground_file_dialog,
        handle_input_file_dialog,
    };
    file_dialog_data.type = type;
    file_dialog_data.file_data = type == FILE_TYPE_SCENARIO ? &scenario_data : &saved_game_data;
    file_dialog_data.dialog_type = dialog_type;
    file_dialog_data.message_not_exist_start_time = 0;
    file_dialog_data.double_click = 0;
    file_dialog_data.focus_button_id = 0;
    if (string_length(file_dialog_data.file_data->last_loaded_file) > 0) {
        string_copy(file_dialog_data.file_data->last_loaded_file, file_dialog_data.typed_name, FILE_NAME_MAX);
        file_remove_extension(file_dialog_data.typed_name);
    } else if (dialog_type == FILE_DIALOG_SAVE) {
        // Suggest default filename
        string_copy(lang_get_string(9, type == FILE_TYPE_SCENARIO ? 7 : 6), file_dialog_data.typed_name, FILE_NAME_MAX);
    } else {
        // Use empty string
        file_dialog_data.typed_name[0] = 0;
    }
    string_copy(file_dialog_data.typed_name, file_dialog_data.previously_seen_typed_name, FILE_NAME_MAX);
    file_dialog_data.file_list = dir_list_files(file_dialog_data.file_data->extension);
    scrollbar_init(&scrollbar_file_dialog, 0, file_dialog_data.file_list->num_files - NUM_FILES_IN_VIEW);
    scroll_to_typed_text();
    string_copy(file_dialog_data.file_data->last_loaded_file, file_dialog_data.selected_file, FILE_NAME_MAX - 1);
    input_box_start(&file_name_input);
    window_show(&window);
}

void window_hotkey_editor_key_pressed(int key, int modifiers)
{
    if (key == KEY_TYPE_ENTER && modifiers == KEY_MOD_NONE) {
        button_close_hotkey_editor_window(1, 0);
    } else if (key == KEY_TYPE_ESCAPE && modifiers == KEY_MOD_NONE) {
        button_close_hotkey_editor_window(0, 0);
    } else {
        if (key != KEY_TYPE_NONE) {
            hotkey_editor_window_data.key = key;
        }
        hotkey_editor_window_data.modifiers = modifiers;
    }
}

void window_hotkey_editor_key_released(int key, int modifiers)
{
    // update modifiers as long as we don't have a proper keypress
    if (hotkey_editor_window_data.key == KEY_TYPE_NONE && key == KEY_TYPE_NONE) {
        hotkey_editor_window_data.modifiers = modifiers;
    }
}

static void draw_background_intermezzo(void)
{
    graphics_clear_screen();
    int x_offset = (screen_width() - 1024) / 2;
    int y_offset = (screen_height() - 768) / 2;

    int image_base = image_group(GROUP_INTERMEZZO_BACKGROUND);
    if (intermezzo_data.type == INTERMEZZO_MISSION_BRIEFING) {
        image_draw(image_base + 1, x_offset, y_offset);
    } else if (intermezzo_data.type == INTERMEZZO_FIRED) {
        image_draw(image_base, x_offset, y_offset);
    } else if (intermezzo_data.type == INTERMEZZO_WON) {
        image_draw(image_base + 2, x_offset, y_offset);
    }
}

static void handle_input_intermezzo(const struct mouse_t *m, __attribute__((unused)) const struct hotkeys_t *h)
{
    uint32_t current_time = time_get_millis();
    if (m->right.went_up || current_time - intermezzo_data.start_time > (intermezzo_data.type ? DISPLAY_TIME_MILLIS : 300)) {
        intermezzo_data.callback();
    }
}

static void window_intermezzo_show(int type, void (*callback)(void))
{
    struct window_type_t window = {
        WINDOW_INTERMEZZO,
        draw_background_intermezzo,
        0,
        handle_input_intermezzo,
    };
    intermezzo_data.type = type;
    intermezzo_data.callback = callback;
    intermezzo_data.start_time = time_get_millis();
    stop_sound_channel(SOUND_CHANNEL_SPEECH);
    if (intermezzo_data.type == INTERMEZZO_FIRED) {
        stop_music();
        play_speech_file(SOUND_FILE_LOSE);
    } else if (intermezzo_data.type == INTERMEZZO_WON) {
        stop_music();
        play_speech_file(SOUND_FILE_WIN);
    }
    window_show(&window);
}

static void draw_background_logo(void)
{
    graphics_clear_screen();
    graphics_in_dialog();
    image_draw(image_group(GROUP_LOGO), 0, 0);
    lang_text_draw_centered_colored(13, 7, 160, 462, 320, FONT_NORMAL_PLAIN, COLOR_WHITE);
    graphics_reset_dialog();
}

static void handle_input_logo(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->left.went_up || m->right.went_up) {
        window_main_menu_show(0);
        return;
    }
    if (h->escape_pressed) {
        post_event(USER_EVENT_QUIT);
    }
}

static void draw_background_plain_message_dialog(void)
{
    graphics_in_dialog();
    outer_panel_draw(80, 80, 30, 12);
    text_draw_centered(plain_message_dialog_data.title, 80, 100, 480, FONT_LARGE_BLACK, 0);
    text_draw_multiline(plain_message_dialog_data.message, 100, 140, 450, FONT_NORMAL_BLACK, 0);
    graphics_reset_dialog();
}

static void close_plain_message_dialog(void)
{
    window_go_back();
}

static void button_ok_plain_message_dialog(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    close_plain_message_dialog();
}

static struct image_button_t buttons_plain_message_dialog[] = {
    {223, 140, 39, 26, IB_NORMAL, GROUP_OK_CANCEL_SCROLL_BUTTONS, 0, button_ok_plain_message_dialog, button_none, 1, 0, 1, 0, 0, 0},
};

static void draw_foreground_plain_message_dialog(void)
{
    graphics_in_dialog();
    image_buttons_draw(80, 80, buttons_plain_message_dialog, 1);
    graphics_reset_dialog();
}

static void handle_input_plain_message_dialog(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (image_buttons_handle_mouse(mouse_in_dialog(m), 80, 80, buttons_plain_message_dialog, 1, 0)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed || h->enter_pressed) {
        close_plain_message_dialog();
    }
}

static void window_plain_message_dialog_show(char *title, char *message)
{
    if (!window_is(WINDOW_PLAIN_MESSAGE_DIALOG)) { // don't show popup over popup
        plain_message_dialog_data.title = title;
        plain_message_dialog_data.message = message;
        struct window_type_t window = {
        WINDOW_PLAIN_MESSAGE_DIALOG,
        draw_background_plain_message_dialog,
        draw_foreground_plain_message_dialog,
        handle_input_plain_message_dialog,
        };
        window_show(&window);
    }
}

static void handle_input_intro_video(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (!started || m->left.went_up || m->right.went_up || video_is_finished() || h->enter_pressed) {
        video_stop();
        graphics_clear_screen();
        while (current_video < NUM_INTRO_VIDEOS) {
            if (video_start(intro_videos[current_video++])) {
                video_init(0);
                started = 1;
                return;
            }
        }
        play_intro_music();
        window_go_back();
    }
}

void window_logo_show(int show_patch_message)
{
    struct window_type_t window = {
        WINDOW_LOGO,
        draw_background_logo,
        0,
        handle_input_logo,
    };
    play_intro_music();
    window_show(&window);
    if (show_patch_message == MESSAGE_MISSING_PATCH) {
        window_plain_message_dialog_show("Patch 1.0.1.0 not installed", "Your Caesar 3 installation does not have the 1.0.1.0 patch installed.\n\
        You can download the patch from : https://github.com/bvschaik/julius/wiki/Patches.\nContinue at your own risk.");
    }
    if (config_get(CONFIG_UI_SHOW_INTRO_VIDEO)) {
        current_video = 0;
        started = 0;
        window.id = WINDOW_INTRO_VIDEO;
        window.draw_background = graphics_clear_screen;
        window.draw_foreground = video_draw_fullscreen;
        window.handle_input = handle_input_intro_video;
        window_show(&window);
    }
}

static void button_start_scenario(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (game_file_start_scenario(cck_selection_data.selected_scenario_filename)) {
        update_music(1);
        window_mission_briefing_show();
    }
}

static struct image_button_t start_button =
{ 600, 440, 27, 27, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 56, button_start_scenario, button_none, 1, 0, 1, 0, 0, 0 };

static void button_toggle_minimap(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    cck_selection_data.show_minimap = !cck_selection_data.show_minimap;
    window_invalidate();
}

static struct generic_button_t toggle_minimap_button =
{ 570, 87, 39, 28, button_toggle_minimap, button_none, 0, 0 };

static void on_scroll_cck(void)
{
    window_invalidate();
}

static struct scrollbar_type_t scrollbar_cck = { 276, 210, 256, on_scroll_cck, 8, 1, 0, 0, 0, 0 };

static void button_select_item(int index, __attribute__((unused)) int param2)
{
    if (index >= cck_selection_data.scenarios->num_files) {
        return;
    }
    cck_selection_data.selected_item = scrollbar_cck.scroll_position + index;
    string_copy(cck_selection_data.scenarios->files[cck_selection_data.selected_item], cck_selection_data.selected_scenario_filename, FILE_NAME_MAX - 1);
    game_file_load_scenario_data(cck_selection_data.selected_scenario_filename);
    string_copy(cck_selection_data.selected_scenario_filename, cck_selection_data.selected_scenario_display, FILE_NAME_MAX - 1);
    file_remove_extension(cck_selection_data.selected_scenario_display);
    window_invalidate();
}

static struct generic_button_t file_buttons_cck[] = {
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

static void draw_scenario_list(void)
{
    inner_panel_draw(16, 210, 16, 16);
    char file[FILE_NAME_MAX];
    for (int i = 0; i < MAX_SCENARIOS; i++) {
        if (i >= cck_selection_data.scenarios->num_files) {
            break;
        }
        int font = FONT_NORMAL_GREEN;
        if (cck_selection_data.focus_button_id == i + 1) {
            font = FONT_NORMAL_WHITE;
        }
        string_copy(cck_selection_data.scenarios->files[i + scrollbar_cck.scroll_position], file, FILE_NAME_MAX - 1);
        file_remove_extension(file);
        text_ellipsize(file, font, 240);
        text_draw(file, 24, 220 + 16 * i, font, 0);
    }
    if (cck_selection_data.scenarios->file_overflow) {
        text_draw(too_many_files_string, 35, 186, FONT_NORMAL_PLAIN, COLOR_RED);
    }
}

static void draw_scenario_minimap_tile(int x_view, int y_view, int grid_offset)
{
    if (grid_offset < 0) {
        return;
    }

    if (terrain_grid.items[grid_offset] & TERRAIN_BUILDING) {
        // Native huts/fields
        if (map_property_is_draw_tile(grid_offset)) {
            int image_id = image_group(GROUP_MINIMAP_BUILDING);
            switch (map_property_multi_tile_size(grid_offset)) {
                case 1: image_draw(image_id, x_view, y_view); break;
                case 2: image_draw(image_id + 1, x_view, y_view - 1); break;
            }
        }
    } else {
        int rand = map_random_get(grid_offset);
        const struct tile_color_t *color;
        const struct tile_color_set_t *set = &MINIMAP_COLOR_SETS[scenario.climate];
        if (terrain_grid.items[grid_offset] & TERRAIN_WATER) {
            color = &set->water[rand & 3];
        } else if (terrain_grid.items[grid_offset] & (TERRAIN_TREE | TERRAIN_SHRUB)) {
            color = &set->tree[rand & 3];
        } else if (terrain_grid.items[grid_offset] & (TERRAIN_ROCK | TERRAIN_ELEVATION)) {
            color = &set->rock[rand & 3];
        } else if (terrain_grid.items[grid_offset] & TERRAIN_ROAD) {
            color = &set->road;
        } else if (terrain_grid.items[grid_offset] & TERRAIN_MEADOW) {
            color = &set->meadow[rand & 3];
        } else {
            color = &set->grass[rand & 7];
        }
        graphics_draw_vertical_line(x_view, y_view, y_view, color->left);
        graphics_draw_vertical_line(x_view + 1, y_view, y_view, color->right);
    }
}

static void widget_scenario_minimap_draw(int x_offset, int y_offset, int width, int height)
{
    scenario_minimap_data.width_tiles = (width + 2) / 2;
    scenario_minimap_data.height_tiles = height;
    scenario_minimap_data.x_offset = x_offset;
    scenario_minimap_data.y_offset = y_offset;
    scenario_minimap_data.absolute_x = (VIEW_X_MAX - scenario_minimap_data.width_tiles) / 2;
    scenario_minimap_data.absolute_y = (VIEW_Y_MAX - scenario_minimap_data.height_tiles) / 2;
    // ensure even height
    scenario_minimap_data.absolute_y &= ~1;
    graphics_set_clip_rectangle(x_offset, y_offset, width, height);
    city_view_foreach_minimap_tile(
    scenario_minimap_data.x_offset, scenario_minimap_data.y_offset, scenario_minimap_data.absolute_x, scenario_minimap_data.absolute_y,
    scenario_minimap_data.width_tiles, scenario_minimap_data.height_tiles, draw_scenario_minimap_tile);
    graphics_reset_clip_rectangle();
}

static void draw_background_cck(void)
{
    image_draw_fullscreen_background(image_group(GROUP_INTERMEZZO_BACKGROUND) + 16);
    draw_version_string();
    graphics_set_clip_rectangle((screen_width() - 640) / 2, (screen_height() - 480) / 2, 640, 480);
    graphics_in_dialog();
    image_draw(image_group(GROUP_CCK_BACKGROUND), (640 - 1024) / 2, (480 - 768) / 2);
    graphics_reset_clip_rectangle();
    inner_panel_draw(280, 242, 2, 12);
    draw_scenario_list();
    const int scenario_info_x = 335;
    const int scenario_info_width = 280;
    const int scenario_criteria_x = 420;
    button_border_draw(75, 35, 184, 144, 0);
    image_draw(image_group(GROUP_SCENARIO_IMAGE) + scenario.brief_description_image_id, 77, 37);
    text_ellipsize(cck_selection_data.selected_scenario_display, FONT_LARGE_BLACK, scenario_info_width + 10);
    text_draw_centered(cck_selection_data.selected_scenario_display, scenario_info_x, 25, scenario_info_width + 10, FONT_LARGE_BLACK, 0);
    text_draw_centered(scenario.brief_description, scenario_info_x, 60, scenario_info_width, FONT_NORMAL_WHITE, 0);
    lang_text_draw_year(scenario.start_year, scenario_criteria_x, 90, FONT_LARGE_BLACK);

    if (cck_selection_data.show_minimap) {
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
    graphics_reset_dialog();
}

static void draw_foreground_cck(void)
{
    graphics_in_dialog();
    image_buttons_draw(0, 0, &start_button, 1);
    button_border_draw(
        toggle_minimap_button.x, toggle_minimap_button.y,
        toggle_minimap_button.width, toggle_minimap_button.height,
        cck_selection_data.focus_toggle_button);
    scrollbar_draw(&scrollbar_cck);
    draw_scenario_list();
    graphics_reset_dialog();
}

static void handle_input_cck(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (scrollbar_handle_mouse(&scrollbar_cck, m_dialog)) {
        return;
    }
    if (image_buttons_handle_mouse(m_dialog, 0, 0, &start_button, 1, 0)) {
        return;
    }
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, &toggle_minimap_button, 1, &cck_selection_data.focus_toggle_button)) {
        return;
    }
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, file_buttons_cck, MAX_SCENARIOS, &cck_selection_data.focus_button_id)) {
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

static void button_click_main_menu(int type, __attribute__((unused)) int param2)
{
    if (type == 1) {
        struct window_type_t window = {
            WINDOW_CCK_SELECTION,
            draw_background_cck,
            draw_foreground_cck,
            handle_input_cck,
        };
        cck_selection_data.scenarios = dir_list_files("map");
        cck_selection_data.focus_button_id = 0;
        cck_selection_data.focus_toggle_button = 0;
        cck_selection_data.show_minimap = 0;
        button_select_item(0, 0);
        scrollbar_init(&scrollbar_cck, 0, cck_selection_data.scenarios->num_files - MAX_SCENARIOS);
        window_show(&window);
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

static struct generic_button_t buttons_main_menu[] = {
    {192, 140, 256, 25, button_click_main_menu, button_none, 1, 0},
    {192, 180, 256, 25, button_click_main_menu, button_none, 2, 0},
    {192, 220, 256, 25, button_click_main_menu, button_none, 3, 0},
    {192, 260, 256, 25, button_click_main_menu, button_none, 4, 0},
    {192, 300, 256, 25, button_click_main_menu, button_none, 5, 0},
};

static void draw_background_main_menu(void)
{
    graphics_clear_screen();
    image_draw_fullscreen_background(image_group(GROUP_INTERMEZZO_BACKGROUND) + 16);
    draw_version_string();
}

static void draw_foreground_main_menu(void)
{
    graphics_in_dialog();
    for (int i = 0; i < MAX_BUTTONS_MAIN_MENU; i++) {
        large_label_draw(buttons_main_menu[i].x, buttons_main_menu[i].y, buttons_main_menu[i].width / BLOCK_SIZE, focus_button_id_main_menu == i + 1 ? 1 : 0);
    }
    lang_text_draw_centered(30, 3, 192, 146, 256, FONT_NORMAL_GREEN);
    lang_text_draw_centered(30, 2, 192, 186, 256, FONT_NORMAL_GREEN);
    lang_text_draw_centered(9, 8, 192, 226, 256, FONT_NORMAL_GREEN);
    lang_text_draw_centered(2, 0, 192, 266, 256, FONT_NORMAL_GREEN);
    lang_text_draw_centered(30, 5, 192, 306, 256, FONT_NORMAL_GREEN);
    graphics_reset_dialog();
}

static void handle_input_main_menu(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_main_menu, MAX_BUTTONS_MAIN_MENU, &focus_button_id_main_menu)) {
        return;
    }
    if (h->escape_pressed) {
        post_event(USER_EVENT_QUIT);
    }
    if (h->load_file) {
        window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_LOAD);
    }
}

void window_main_menu_show(int restart_music)
{
    if (restart_music) {
        play_intro_music();
    }
    struct window_type_t window = {
        WINDOW_MAIN_MENU,
        draw_background_main_menu,
        draw_foreground_main_menu,
        handle_input_main_menu,
    };
    window_show(&window);
}

static void button_back_message_dialog(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (message_dialog_data.num_history > 0) {
        message_dialog_data.num_history--;
        message_dialog_data.text_id = message_dialog_data.history[message_dialog_data.num_history].text_id;
        rich_text_reset(message_dialog_data.history[message_dialog_data.num_history].scroll_position);
        window_invalidate();
    }
}

static void cleanup_message_dialog(void)
{
    if (message_dialog_data.show_video) {
        video_stop();
        message_dialog_data.show_video = 0;
    }
    player_message.message_advisor = 0;
}

static void button_close_message_dialog(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    cleanup_message_dialog();
    window_go_back();
    window_invalidate();
}

static void button_help_message_dialog(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    button_close_message_dialog(0, 0);
    window_message_dialog_show(MESSAGE_DIALOG_HELP, message_dialog_data.background_callback);
}

static void button_advisor_message_dialog(int advisor, __attribute__((unused)) int param2)
{
    cleanup_message_dialog();
    window_advisors_show(advisor);
}

static void button_go_to_problem_message_dialog(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    cleanup_message_dialog();
    int grid_offset = player_message.param2;
    if (grid_offset > 0 && grid_offset < 26244) {
        city_view_go_to_grid_offset(grid_offset);
    }
    window_city_show();
}

static struct image_button_t image_button_back = {
    0, 0, 31, 20, IB_NORMAL, GROUP_ARROW_MESSAGE_PROBLEMS, 8, button_back_message_dialog, button_none, 0, 0, 1, 0, 0, 0
};
static struct image_button_t image_button_close_message_dialog = {
    0, 0, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_close_message_dialog, button_none, 0, 0, 1, 0, 0, 0
};
static struct image_button_t image_button_go_to_problem = {
    0, 0, 27, 27, IB_NORMAL, GROUP_SIDEBAR_BUTTONS, 52, button_go_to_problem_message_dialog, button_none, 1, 0, 1, 0, 0, 0
};
static struct image_button_t image_button_help_message_dialog = {
    0, 0, 18, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help_message_dialog, button_none, 1, 0, 1, 0, 0, 0
};
static struct image_button_t image_button_labor = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 0, button_advisor_message_dialog, button_none, ADVISOR_LABOR, 0, 1, 0, 0, 0
};
static struct image_button_t image_button_trade = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 12, button_advisor_message_dialog, button_none, ADVISOR_TRADE, 0, 1, 0, 0, 0
};
static struct image_button_t image_button_population = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 15, button_advisor_message_dialog, button_none, ADVISOR_POPULATION, 0, 1, 0, 0, 0
};
static struct image_button_t image_button_imperial = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 6, button_advisor_message_dialog, button_none, ADVISOR_IMPERIAL, 0, 1, 0, 0, 0
};
static struct image_button_t image_button_military = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 3, button_advisor_message_dialog, button_none, ADVISOR_MILITARY, 0, 1, 0, 0, 0
};
static struct image_button_t image_button_health = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 18, button_advisor_message_dialog, button_none, ADVISOR_HEALTH, 0, 1, 0, 0, 0
};
static struct image_button_t image_button_religion = {
    0, 0, 27, 27, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 27, button_advisor_message_dialog, button_none, ADVISOR_RELIGION, 0, 1, 0, 0, 0
};

static int resource_image(int resource)
{
    return resource_images[resource].icon_img_id + resource_image_offset(resource, RESOURCE_IMAGE_ICON);
}

static int is_problem_message(const struct lang_message_t *msg)
{
    return msg->type == TYPE_MESSAGE &&
        (msg->message_type == MESSAGE_TYPE_DISASTER || msg->message_type == MESSAGE_TYPE_INVASION);
}

static struct image_button_t *get_advisor_button(void)
{
    switch (player_message.message_advisor) {
        case MESSAGE_ADVISOR_LABOR:
            return &image_button_labor;
        case MESSAGE_ADVISOR_TRADE:
            return &image_button_trade;
        case MESSAGE_ADVISOR_POPULATION:
            return &image_button_population;
        case MESSAGE_ADVISOR_IMPERIAL:
            return &image_button_imperial;
        case MESSAGE_ADVISOR_MILITARY:
            return &image_button_military;
        case MESSAGE_ADVISOR_HEALTH:
            return &image_button_health;
        case MESSAGE_ADVISOR_RELIGION:
            return &image_button_religion;
        default:
            return &image_button_help_message_dialog;
    }
}

static void draw_foreground_video(void)
{
    video_draw(message_dialog_data.x + 8, message_dialog_data.y + 8);
    image_buttons_draw(message_dialog_data.x + 16, message_dialog_data.y + 408, get_advisor_button(), 1);
    image_buttons_draw(message_dialog_data.x + 372, message_dialog_data.y + 410, &image_button_close_message_dialog, 1);
    const struct lang_message_t *msg = lang_get_message(message_dialog_data.text_id);
    if (is_problem_message(msg)) {
        image_buttons_draw(message_dialog_data.x + 48, message_dialog_data.y + 407, &image_button_go_to_problem, 1);
    }
}

static void draw_background_video(void)
{
    const struct lang_message_t *msg = lang_get_message(message_dialog_data.text_id);
    message_dialog_data.x = 32;
    message_dialog_data.y = 28;

    int small_font = 0;
    int lines_available = 4;
    if (msg->type == TYPE_MESSAGE && msg->message_type == MESSAGE_TYPE_IMPERIAL) {
        lines_available = 3;
    }
    rich_text_set_fonts(FONT_NORMAL_WHITE, FONT_NORMAL_RED, 5);
    rich_text_clear_links();
    int lines_required = rich_text_draw(msg->content.text, 0, 0, 384, lines_available, 1);
    if (lines_required > lines_available) {
        small_font = 1;
        rich_text_set_fonts(FONT_SMALL_PLAIN, FONT_SMALL_PLAIN, 7);
        lines_required = rich_text_draw(msg->content.text, 0, 0, 384, lines_available, 1);
    }

    outer_panel_draw(message_dialog_data.x, message_dialog_data.y, 26, 28);
    graphics_draw_rect(message_dialog_data.x + 7, message_dialog_data.y + 7, 402, 294, COLOR_BLACK);

    int y_base = message_dialog_data.y + 308;
    int inner_height_blocks = 6;
    if (lines_required > lines_available) {
        // create space to cram an extra line into the dialog
        y_base = y_base - 8;
        inner_height_blocks += 1;
    }
    inner_panel_draw(message_dialog_data.x + 8, y_base, 25, inner_height_blocks);
    text_draw_centered(msg->title.text,
        message_dialog_data.x + 8, message_dialog_data.y + 414, 400, FONT_NORMAL_BLACK, 0);

    int width = lang_text_draw(25, player_message.month, message_dialog_data.x + 16, y_base + 4, FONT_NORMAL_WHITE);
    width += lang_text_draw_year(player_message.year, message_dialog_data.x + 18 + width, y_base + 4, FONT_NORMAL_WHITE);

    if (msg->type == TYPE_MESSAGE && msg->message_type == MESSAGE_TYPE_DISASTER &&
        message_dialog_data.text_id == MESSAGE_DIALOG_THEFT) {
        lang_text_draw_amount(8, 0, player_message.param1, message_dialog_data.x + 90 + width, y_base + 4, FONT_NORMAL_WHITE);
    } else {
        width += lang_text_draw(63, 5, message_dialog_data.x + 70 + width, y_base + 4, FONT_NORMAL_WHITE);
        text_draw(scenario_settings.player_name, message_dialog_data.x + 70 + width, y_base + 4, FONT_NORMAL_WHITE, 0);
    }

    message_dialog_data.text_height_blocks = msg->height_blocks - 1 - (32 + message_dialog_data.y_text - message_dialog_data.y) / BLOCK_SIZE;
    message_dialog_data.text_width_blocks = msg->width_blocks - 4;
    if (small_font) {
        // Draw in black and then white to create shadow effect
        rich_text_draw_colored(msg->content.text,
            message_dialog_data.x + 16 + 1, y_base + 24 + 1, 384, message_dialog_data.text_height_blocks - 1, COLOR_BLACK);
        rich_text_draw_colored(msg->content.text,
            message_dialog_data.x + 16, y_base + 24, 384, message_dialog_data.text_height_blocks - 1, COLOR_WHITE);
    } else {
        rich_text_draw(msg->content.text, message_dialog_data.x + 16, y_base + 24, 384, message_dialog_data.text_height_blocks - 1, 0);
    }

    if (msg->type == TYPE_MESSAGE && msg->message_type == MESSAGE_TYPE_IMPERIAL) {
        int y_text = message_dialog_data.y + 384;
        if (lines_required > lines_available) {
            y_text += 8;
        }
        text_draw_number(scenario.requests[player_message.param1].amount, '@', " ", message_dialog_data.x + 8, y_text, FONT_NORMAL_WHITE);
        image_draw(
            resource_images[scenario.requests[player_message.param1].resource].icon_img_id
            + resource_image_offset(scenario.requests[player_message.param1].resource, RESOURCE_IMAGE_ICON),
            message_dialog_data.x + 70, y_text - 5);
        lang_text_draw(23, scenario.requests[player_message.param1].resource, message_dialog_data.x + 100, y_text, FONT_NORMAL_WHITE);
        if (scenario.requests[player_message.param1].state == REQUEST_STATE_NORMAL || scenario.requests[player_message.param1].state == REQUEST_STATE_OVERDUE) {
            width = lang_text_draw_amount(8, 4, scenario.requests[player_message.param1].months_to_comply, message_dialog_data.x + 200, y_text, FONT_NORMAL_WHITE);
            lang_text_draw(12, 2, message_dialog_data.x + 200 + width, y_text, FONT_NORMAL_WHITE);
        }
    }

    draw_foreground_video();
}

static void draw_background_message_dialog(void)
{
    if (message_dialog_data.background_callback) {
        message_dialog_data.background_callback();
    } else {
        window_draw_underlying_window();
    }
    graphics_in_dialog();
    if (message_dialog_data.show_video) {
        draw_background_video();
    } else {
        const struct lang_message_t *msg = lang_get_message(message_dialog_data.text_id);
        message_dialog_data.x = msg->x;
        message_dialog_data.y = msg->y;
        message_dialog_data.x_text = message_dialog_data.x + 16;
        outer_panel_draw(message_dialog_data.x, message_dialog_data.y, msg->width_blocks, msg->height_blocks);
        if (msg->title.text) {
            int image_id;
            if (!msg->image.id) {
                image_id = 0;
            } else if (message_dialog_data.text_id == 0) {
                // message id = 0 ==> "about": fixed image position
                image_id = image_group(GROUP_BIG_PEOPLE);
            } else {
                image_id = image_group(GROUP_MESSAGE_IMAGES) + msg->image.id - 1;
            }
            const struct image_t *img = image_id ? image_get(image_id) : 0;
            // title
            // Center title in the dialog but ensure it does not overlap with the
            // image: if the title is too long, it will start 8px from the image.
            int title_x_offset = img ? img->width + msg->image.x + 8 : 0;
            text_draw_centered(msg->title.text, message_dialog_data.x + title_x_offset, message_dialog_data.y + 14,
                BLOCK_SIZE * msg->width_blocks - 2 * title_x_offset, FONT_LARGE_BLACK, 0);

            message_dialog_data.y_text = message_dialog_data.y + 48;

            // picture
            if (img) {
                int image_x = msg->image.x;
                int image_y = msg->image.y;
                image_draw(image_id, message_dialog_data.x + image_x, message_dialog_data.y + image_y);
                if (message_dialog_data.y + image_y + img->height + 8 > message_dialog_data.y_text) {
                    message_dialog_data.y_text = message_dialog_data.y + image_y + img->height + 8;
                }
            }
        }
        if (msg->subtitle.x && msg->subtitle.text) {
            int width = BLOCK_SIZE * (msg->width_blocks - 1) - msg->subtitle.x;
            int height = text_draw_multiline(msg->subtitle.text,
                message_dialog_data.x + msg->subtitle.x, message_dialog_data.y + msg->subtitle.y, width, FONT_NORMAL_BLACK, 0);
            if (message_dialog_data.y + msg->subtitle.y + height > message_dialog_data.y_text) {
                message_dialog_data.y_text = message_dialog_data.y + msg->subtitle.y + height;
            }
        }
        if (msg->content.text) {
            rich_text_set_fonts(FONT_NORMAL_WHITE, FONT_NORMAL_RED, 5);
            int header_offset = msg->type == TYPE_MANUAL ? 48 : 32;
            message_dialog_data.text_height_blocks = msg->height_blocks - 1 - (header_offset + message_dialog_data.y_text - message_dialog_data.y) / BLOCK_SIZE;
            message_dialog_data.text_width_blocks = rich_text_init(msg->content.text,
                message_dialog_data.x_text, message_dialog_data.y_text, msg->width_blocks - 4, message_dialog_data.text_height_blocks, 1);

            // content!
            inner_panel_draw(message_dialog_data.x_text, message_dialog_data.y_text, message_dialog_data.text_width_blocks, message_dialog_data.text_height_blocks);
            graphics_set_clip_rectangle(message_dialog_data.x_text + 3, message_dialog_data.y_text + 3,
                BLOCK_SIZE * message_dialog_data.text_width_blocks - 6, BLOCK_SIZE * message_dialog_data.text_height_blocks - 6);
            rich_text_clear_links();

            if (msg->type == TYPE_MESSAGE) {
                int width = lang_text_draw(25, player_message.month, message_dialog_data.x_text + 10, message_dialog_data.y_text + 6, FONT_NORMAL_WHITE);
                width += lang_text_draw_year(player_message.year, message_dialog_data.x_text + 12 + width, message_dialog_data.y_text + 6, FONT_NORMAL_WHITE);
                if (msg->message_type == MESSAGE_TYPE_DISASTER && player_message.param1) {
                    if (message_dialog_data.text_id == MESSAGE_DIALOG_THEFT) {
                        // param1 = denarii
                        lang_text_draw_amount(8, 0, player_message.param1, message_dialog_data.x + 240, message_dialog_data.y_text + 6, FONT_NORMAL_WHITE);
                    } else {
                        // param1 = building type
                        lang_text_draw(41, player_message.param1, message_dialog_data.x + 240, message_dialog_data.y_text + 6, FONT_NORMAL_WHITE);
                    }
                } else {
                    width += lang_text_draw(63, 5, message_dialog_data.x_text + width + 60, message_dialog_data.y_text + 6, FONT_NORMAL_WHITE);
                    text_draw(scenario_settings.player_name, message_dialog_data.x_text + width + 60, message_dialog_data.y_text + 6, FONT_NORMAL_WHITE, 0);
                }
                switch (msg->message_type) {
                    case MESSAGE_TYPE_DISASTER:
                    case MESSAGE_TYPE_INVASION:
                        lang_text_draw(12, 1, message_dialog_data.x + 100, message_dialog_data.y_text + 44, FONT_NORMAL_WHITE);
                        rich_text_draw(msg->content.text, message_dialog_data.x_text + 8, message_dialog_data.y_text + 86,
                            BLOCK_SIZE * message_dialog_data.text_width_blocks, message_dialog_data.text_height_blocks - 1, 0);
                        break;
                    case MESSAGE_TYPE_EMIGRATION:
                    {
                        if (city_data.sentiment.low_mood_cause >= 1 && city_data.sentiment.low_mood_cause <= 5) {
                            int max_width = BLOCK_SIZE * (message_dialog_data.text_width_blocks - 1) - 64;
                            lang_text_draw_multiline(12, city_data.sentiment.low_mood_cause + 2, message_dialog_data.x + 64, message_dialog_data.y_text + 44, max_width, FONT_NORMAL_WHITE);
                        }
                        rich_text_draw(msg->content.text,
                            message_dialog_data.x_text + 8, message_dialog_data.y_text + 86, BLOCK_SIZE * (message_dialog_data.text_width_blocks - 1),
                            message_dialog_data.text_height_blocks - 1, 0);
                        break;
                    }
                    case MESSAGE_TYPE_TRADE_CHANGE:
                        image_draw(resource_image(player_message.param2), message_dialog_data.x + 64, message_dialog_data.y_text + 40);
                        lang_text_draw(21, empire_objects[player_message.param1].city_name_id,
                            message_dialog_data.x + 100, message_dialog_data.y_text + 44, FONT_NORMAL_WHITE);
                        rich_text_draw(msg->content.text,
                            message_dialog_data.x_text + 8, message_dialog_data.y_text + 86, BLOCK_SIZE * (message_dialog_data.text_width_blocks - 1),
                            message_dialog_data.text_height_blocks - 1, 0);
                        break;
                    case MESSAGE_TYPE_PRICE_CHANGE:
                        image_draw(resource_image(player_message.param2), message_dialog_data.x + 64, message_dialog_data.y_text + 40);
                        text_draw_money(player_message.param1, message_dialog_data.x + 100, message_dialog_data.y_text + 44, FONT_NORMAL_WHITE);
                        rich_text_draw(msg->content.text,
                            message_dialog_data.x_text + 8, message_dialog_data.y_text + 86, BLOCK_SIZE * (message_dialog_data.text_width_blocks - 1),
                            message_dialog_data.text_height_blocks - 1, 0);
                        break;
                    default:
                    {
                        int lines = rich_text_draw(msg->content.text,
                            message_dialog_data.x_text + 8, message_dialog_data.y_text + 56, BLOCK_SIZE * (message_dialog_data.text_width_blocks - 1),
                            message_dialog_data.text_height_blocks - 1, 0);
                        if (msg->message_type == MESSAGE_TYPE_IMPERIAL) {
                            int y_offset = message_dialog_data.y_text + 86 + lines * 16;
                            text_draw_number(scenario.requests[player_message.param1].amount, '@', " ", message_dialog_data.x_text + 8, y_offset, FONT_NORMAL_WHITE);
                            image_draw(resource_image(scenario.requests[player_message.param1].resource), message_dialog_data.x_text + 70, y_offset - 5);
                            lang_text_draw(23, scenario.requests[player_message.param1].resource,
                                message_dialog_data.x_text + 100, y_offset, FONT_NORMAL_WHITE);
                            if (scenario.requests[player_message.param1].state == REQUEST_STATE_NORMAL || scenario.requests[player_message.param1].state == REQUEST_STATE_OVERDUE) {
                                int comply_time_width = lang_text_draw_amount(8, 4, scenario.requests[player_message.param1].months_to_comply,
                                    message_dialog_data.x_text + 200, y_offset, FONT_NORMAL_WHITE);
                                lang_text_draw(12, 2, message_dialog_data.x_text + 200 + comply_time_width, y_offset, FONT_NORMAL_WHITE);
                            }
                        }
                        break;
                    }
                }
            } else {
                rich_text_draw(msg->content.text,
                    message_dialog_data.x_text + 8, message_dialog_data.y_text + 6, BLOCK_SIZE * (message_dialog_data.text_width_blocks - 1),
                    message_dialog_data.text_height_blocks - 1, 0);
            }
            graphics_reset_clip_rectangle();
        }
        if (msg->type == TYPE_MANUAL && message_dialog_data.num_history > 0) {
            // Back button text
            lang_text_draw(12, 0,
                message_dialog_data.x + 52, message_dialog_data.y + BLOCK_SIZE * msg->height_blocks - 31, FONT_NORMAL_BLACK);
        }
    }
    graphics_reset_dialog();
}

static void draw_foreground_message_dialog(void)
{
    graphics_in_dialog();
    if (message_dialog_data.show_video) {
        draw_foreground_video();
    } else {
        const struct lang_message_t *msg = lang_get_message(message_dialog_data.text_id);
        if (msg->type == TYPE_MANUAL && message_dialog_data.num_history > 0) {
            image_buttons_draw(
                message_dialog_data.x + 16, message_dialog_data.y + BLOCK_SIZE * msg->height_blocks - 36,
                &image_button_back, 1);
        }
        if (msg->type == TYPE_MESSAGE) {
            image_buttons_draw(message_dialog_data.x + 16, message_dialog_data.y + BLOCK_SIZE * msg->height_blocks - 40, get_advisor_button(), 1);
            if (msg->message_type == MESSAGE_TYPE_DISASTER || msg->message_type == MESSAGE_TYPE_INVASION) {
                image_buttons_draw(message_dialog_data.x + 64, message_dialog_data.y_text + 36, &image_button_go_to_problem, 1);
            }
        }
        image_buttons_draw(message_dialog_data.x + BLOCK_SIZE * msg->width_blocks - 38, message_dialog_data.y + BLOCK_SIZE * msg->height_blocks - 36,
            &image_button_close_message_dialog, 1);
        rich_text_draw_scrollbar();
    }
    graphics_reset_dialog();
}

static void handle_input_message_dialog(const struct mouse_t *m, const struct hotkeys_t *h)
{
    message_dialog_data.focus_button_id = 0;
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    const struct lang_message_t *msg = lang_get_message(message_dialog_data.text_id);
    int handled = 0;
    if (message_dialog_data.show_video) {
        if (image_buttons_handle_mouse(m_dialog, message_dialog_data.x + 16, message_dialog_data.y + 408, get_advisor_button(), 1, 0)) {
            handled = 1;
        } else if (image_buttons_handle_mouse(m_dialog, message_dialog_data.x + 372, message_dialog_data.y + 410, &image_button_close_message_dialog, 1, 0)) {
            handled = 1;
        } else if (is_problem_message(msg)) {
            if (image_buttons_handle_mouse(m_dialog, message_dialog_data.x + 48, message_dialog_data.y + 407,
                &image_button_go_to_problem, 1, &message_dialog_data.focus_button_id)) {
                handled = 1;
            }
        }
    } else {
        if (msg->type == TYPE_MANUAL && image_buttons_handle_mouse(
            m_dialog, message_dialog_data.x + 16, message_dialog_data.y + BLOCK_SIZE * msg->height_blocks - 36, &image_button_back, 1, 0)) {
            handled = 1;
        } else if (msg->type == TYPE_MESSAGE) {
            if (image_buttons_handle_mouse(m_dialog, message_dialog_data.x + 16, message_dialog_data.y + BLOCK_SIZE * msg->height_blocks - 40, get_advisor_button(), 1, 0)) {
                handled = 1;
            } else if (msg->message_type == MESSAGE_TYPE_DISASTER || msg->message_type == MESSAGE_TYPE_INVASION) {
                if (image_buttons_handle_mouse(m_dialog, message_dialog_data.x + 64, message_dialog_data.y_text + 36,
                    &image_button_go_to_problem, 1, 0)) {
                    handled = 1;
                }
            }
        } else if (image_buttons_handle_mouse(m_dialog,
            message_dialog_data.x + BLOCK_SIZE * msg->width_blocks - 38,
            message_dialog_data.y + BLOCK_SIZE * msg->height_blocks - 36,
            &image_button_close_message_dialog, 1, 0)) {
            handled = 1;
        } else {
            rich_text_handle_mouse(m_dialog);
            int text_id = rich_text_get_clicked_link(m_dialog);
            if (text_id >= 0) {
                if (message_dialog_data.num_history < MAX_HISTORY - 1) {
                    message_dialog_data.history[message_dialog_data.num_history].text_id = message_dialog_data.text_id;
                    message_dialog_data.history[message_dialog_data.num_history].scroll_position = rich_text_scroll_position();
                    message_dialog_data.num_history++;
                }
                message_dialog_data.text_id = text_id;
                rich_text_reset(0);
                window_invalidate();
                handled = 1;
            }
        }
    }
    if (!handled && (m->right.went_up || h->escape_pressed)) {
        button_close_message_dialog(0, 0);
    }
}

void window_message_dialog_show(int text_id, void (*background_callback)(void))
{
    struct window_type_t window = {
        WINDOW_MESSAGE_DIALOG,
        draw_background_message_dialog,
        draw_foreground_message_dialog,
        handle_input_message_dialog,
    };
    scroll_drag_end();
    for (int i = 0; i < MAX_HISTORY; i++) {
        message_dialog_data.history[i].text_id = 0;
        message_dialog_data.history[i].scroll_position = 0;
    }
    message_dialog_data.num_history = 0;
    rich_text_reset(0);
    message_dialog_data.text_id = text_id;
    message_dialog_data.background_callback = background_callback;
    const struct lang_message_t *msg = lang_get_message(text_id);
    if (player_message.use_popup != 1) {
        message_dialog_data.show_video = 0;
    } else if (msg->video.text && video_start(msg->video.text)) {
        message_dialog_data.show_video = 1;
    } else {
        message_dialog_data.show_video = 0;
    }
    if (message_dialog_data.show_video) {
        video_init(1);
    }
    window_show(&window);
}

void window_message_dialog_show_city_message(int text_id, int year, int month, int param1, int param2, int message_advisor, int use_popup)
{
    player_message.year = year;
    player_message.month = month;
    player_message.param1 = param1;
    player_message.param2 = param2;
    player_message.message_advisor = message_advisor;
    player_message.use_popup = use_popup;
    window_message_dialog_show(text_id, window_city_draw_all);
}

static void button_help_message_list(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_message_dialog_show(MESSAGE_DIALOG_MESSAGES, window_city_draw_all);
}

static void button_close_message_list(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_city_show();
}

static void on_scroll_message_list(void);

static struct scrollbar_type_t scrollbar_message_list = { 432, 112, 208, on_scroll_message_list, 0, 0, 0, 0, 0, 0 };

static void on_scroll_message_list(void)
{
    city_message_set_scroll_position(scrollbar_message_list.scroll_position);
    window_invalidate();
}

static void button_message_message_list(int param1, __attribute__((unused)) int param2)
{
    int id = city_message_set_current(scrollbar_message_list.scroll_position + param1);
    if (id < city_message_count()) {
        struct city_message_t *msg = city_message_get(id);
        city_message_mark_read(id);
        window_message_dialog_show_city_message(
            city_message_get_text_id(msg->message_type),
            msg->year, msg->month, msg->param1, msg->param2,
            city_message_get_advisor(msg->message_type),
            0);
    }
}
static void button_delete_message_list(int id_to_delete, __attribute__((unused)) int param2)
{
    int id = city_message_set_current(scrollbar_message_list.scroll_position + id_to_delete);
    if (id < city_message_count()) {
        city_message_delete(id);
        scrollbar_update_max(&scrollbar_message_list, city_message_count() - MAX_MESSAGES);
        window_invalidate();
    }
}

static struct image_button_t image_button_help = {
    0, 0, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help_message_list, button_none, 0, 0, 1, 0, 0, 0
};
static struct image_button_t image_button_close_message_list = {
    0, 0, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_close_message_list, button_none, 0, 0, 1, 0, 0, 0
};
static struct generic_button_t generic_buttons_messages[] = {
    {0, 0, 412, 18, button_message_message_list, button_delete_message_list, 0, 0},
    {0, 20, 412, 18, button_message_message_list, button_delete_message_list, 1, 0},
    {0, 40, 412, 18, button_message_message_list, button_delete_message_list, 2, 0},
    {0, 60, 412, 18, button_message_message_list, button_delete_message_list, 3, 0},
    {0, 80, 412, 18, button_message_message_list, button_delete_message_list, 4, 0},
    {0, 100, 412, 18, button_message_message_list, button_delete_message_list, 5, 0},
    {0, 120, 412, 18, button_message_message_list, button_delete_message_list, 6, 0},
    {0, 140, 412, 18, button_message_message_list, button_delete_message_list, 7, 0},
    {0, 160, 412, 18, button_message_message_list, button_delete_message_list, 8, 0},
    {0, 180, 412, 18, button_message_message_list, button_delete_message_list, 9, 0},
};

static void draw_background_message_list(void)
{
    window_city_draw_all();
    graphics_in_dialog();
    message_list_data.width_blocks = 30;
    message_list_data.height_blocks = 22;
    message_list_data.x_text = 16;
    message_list_data.y_text = 112;
    message_list_data.text_width_blocks = message_list_data.width_blocks - 4;
    message_list_data.text_height_blocks = message_list_data.height_blocks - 9;
    outer_panel_draw(0, 32, message_list_data.width_blocks, message_list_data.height_blocks);
    lang_text_draw_centered(63, 0, 0, 48, BLOCK_SIZE * message_list_data.width_blocks, FONT_LARGE_BLACK);
    inner_panel_draw(message_list_data.x_text, message_list_data.y_text, message_list_data.text_width_blocks, message_list_data.text_height_blocks);

    if (city_message_count() > 0) {
        lang_text_draw(63, 2, message_list_data.x_text + 42, message_list_data.y_text - 12, FONT_SMALL_PLAIN);
        lang_text_draw(63, 3, message_list_data.x_text + 180, message_list_data.y_text - 12, FONT_SMALL_PLAIN);
        lang_text_draw_multiline(63, 4,
            message_list_data.x_text + 50, message_list_data.y_text + 12 + BLOCK_SIZE * message_list_data.text_height_blocks,
            BLOCK_SIZE * message_list_data.text_width_blocks - 100, FONT_NORMAL_BLACK);
    } else {
        lang_text_draw_multiline(63, 1,
            message_list_data.x_text + 16, message_list_data.y_text + 80,
            BLOCK_SIZE * message_list_data.text_width_blocks - 48, FONT_NORMAL_GREEN);
    }
    graphics_reset_dialog();
}

static void draw_foreground_message_list(void)
{
    graphics_in_dialog();
    image_buttons_draw(16, 32 + BLOCK_SIZE * message_list_data.height_blocks - 42, &image_button_help, 1);
    image_buttons_draw(BLOCK_SIZE * message_list_data.width_blocks - 38, 32 + BLOCK_SIZE * message_list_data.height_blocks - 36,
        &image_button_close_message_list, 1);

    int total_messages = city_message_count();
    if (total_messages > 0) {
        int max = total_messages < MAX_MESSAGES ? total_messages : MAX_MESSAGES;
        int index = scrollbar_message_list.scroll_position;
        for (int i = 0; i < max; i++, index++) {
            const struct city_message_t *msg = city_message_get(index);
            const struct lang_message_t *lang_msg = lang_get_message(city_message_get_text_id(msg->message_type));
            int image_offset = 0;
            if (lang_msg->message_type == MESSAGE_TYPE_DISASTER) {
                image_offset = 2;
            }
            if (msg->is_read) {
                image_draw(image_group(GROUP_ARROW_MESSAGE_PROBLEMS) + 15 + image_offset,
                    message_list_data.x_text + 12, message_list_data.y_text + 6 + 20 * i);
            } else {
                image_draw(image_group(GROUP_ARROW_MESSAGE_PROBLEMS) + 14 + image_offset,
                    message_list_data.x_text + 12, message_list_data.y_text + 6 + 20 * i);
            }
            int font = FONT_NORMAL_WHITE;
            if (message_list_data.focus_button_id == i + 1) {
                font = FONT_NORMAL_RED;
            }
            int width = lang_text_draw(25, msg->month, message_list_data.x_text + 42, message_list_data.y_text + 8 + 20 * i, font);
            lang_text_draw_year(msg->year,
                message_list_data.x_text + 42 + width, message_list_data.y_text + 8 + 20 * i, font);
            text_draw(
                lang_msg->title.text,
                message_list_data.x_text + 180, message_list_data.y_text + 8 + 20 * i, font, 0);
        }
        scrollbar_draw(&scrollbar_message_list);
    }

    graphics_reset_dialog();
}

static void handle_input_message_list(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (h->show_messages) {
        window_city_show();
    }
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    int old_button_id = message_list_data.focus_button_id;
    message_list_data.focus_button_id = 0;

    int button_id;
    int handled = image_buttons_handle_mouse(m_dialog, 16, 32 + BLOCK_SIZE * message_list_data.height_blocks - 42,
        &image_button_help, 1, &button_id);
    if (button_id) {
        message_list_data.focus_button_id = 11;
    }
    handled |= image_buttons_handle_mouse(m_dialog, BLOCK_SIZE * message_list_data.width_blocks - 38,
        32 + BLOCK_SIZE * message_list_data.height_blocks - 36, &image_button_close_message_list, 1, &button_id);
    if (button_id) {
        message_list_data.focus_button_id = 12;
    }
    if (scrollbar_handle_mouse(&scrollbar_message_list, m_dialog)) {
        message_list_data.focus_button_id = 13;
    }
    handled |= generic_buttons_handle_mouse(m_dialog, message_list_data.x_text, message_list_data.y_text + 4,
        generic_buttons_messages, MAX_MESSAGES, &button_id);
    if (!message_list_data.focus_button_id) {
        message_list_data.focus_button_id = button_id;
    }
    if (button_id && old_button_id != button_id) {
        window_invalidate();
    }
    if (!handled && (m->right.went_up || h->escape_pressed)) {
        button_close_message_list(0, 0);
    }
}

void window_message_list_show(void)
{
    struct window_type_t window = {
        WINDOW_MESSAGE_LIST,
        draw_background_message_list,
        draw_foreground_message_list,
        handle_input_message_list,
    };
    city_message_sort_and_compact();
    scrollbar_init(&scrollbar_message_list, city_message_scroll_position(), city_message_count() - MAX_MESSAGES);
    window_show(&window);
}

static void draw_background_mission_briefing(void)
{
    image_draw_fullscreen_background(image_group(GROUP_INTERMEZZO_BACKGROUND) + 16);
    graphics_in_dialog();
    outer_panel_draw(-100, -75, 52, 39);
    // Scenario name
    text_draw(scenario.scenario_name, -84, -59, FONT_LARGE_BLACK, 0);
    // Scenario brief description
    text_draw(scenario.brief_description, -84, -27, FONT_NORMAL_BLACK, 0);
    // Player name
    text_draw(scenario_settings.player_name, -84, 5, FONT_NORMAL_BLACK, 0);
    // Objectives
    inner_panel_draw(-84, 37, 50, 5);
    lang_text_draw(62, 10, -60, 72, FONT_NORMAL_WHITE);
    if (scenario.culture_win_criteria.enabled) {
        label_draw(52, 51, 13, 1);
        int width = lang_text_draw(62, 12, 60, 55, FONT_NORMAL_RED);
        text_draw_number(scenario.culture_win_criteria.goal, 0, 0, width + 60, 55, FONT_NORMAL_RED);
    }
    if (scenario.prosperity_win_criteria.enabled) {
        label_draw(52, 83, 13, 1);
        int width = lang_text_draw(62, 13, 60, 87, FONT_NORMAL_RED);
        text_draw_number(scenario.prosperity_win_criteria.goal, 0, 0, width + 60, 87, FONT_NORMAL_RED);
    }
    if (scenario.peace_win_criteria.enabled) {
        label_draw(268, 51, 13, 1);
        int width = lang_text_draw(62, 14, 276, 55, FONT_NORMAL_RED);
        text_draw_number(scenario.peace_win_criteria.goal, 0, 0, width + 276, 55, FONT_NORMAL_RED);
    }
    if (scenario.favor_win_criteria.enabled) {
        label_draw(268, 83, 13, 1);
        int width = lang_text_draw(62, 15, 276, 87, FONT_NORMAL_RED);
        text_draw_number(scenario.favor_win_criteria.goal, 0, 0, width + 276, 87, FONT_NORMAL_RED);
    }
    if (scenario.population_win_criteria.enabled) {
        label_draw(492, 51, 13, 1);
        int width = lang_text_draw(62, 11, 500, 55, FONT_NORMAL_RED);
        text_draw_number(scenario.population_win_criteria.goal, 0, 0, width + 500, 55, FONT_NORMAL_RED);
    }
    if (scenario.time_limit_win_criteria.enabled) {
        label_draw(492, 83, 13, 2);
        int width = text_draw("Fired after", 500, 87, FONT_NORMAL_RED, COLOR_BLACK);
        text_draw_number(scenario.time_limit_win_criteria.years, 0, " Years", width + 500, 87, FONT_NORMAL_RED);
    } else if (scenario.survival_time_win_criteria.enabled) {
        label_draw(492, 83, 13, 2);
        int width = text_draw("Survive for", 500, 87, FONT_NORMAL_RED, COLOR_BLACK);
        text_draw_number(scenario.survival_time_win_criteria.years, 0, " Years", width + 500, 87, FONT_NORMAL_RED);
    }
    inner_panel_draw(-84, 141, 50, 24);
    // Text body (map description)
    rich_text_set_fonts(FONT_NORMAL_WHITE, FONT_NORMAL_RED, 5);
    rich_text_init(scenario.briefing, -60, 141, 46, 24, 0);
    graphics_set_clip_rectangle(-68, 157, 800, 365);
    rich_text_draw(scenario.briefing, -68, 157, 800, 384, 0);
    graphics_reset_clip_rectangle();
    rich_text_draw_scrollbar();
    graphics_reset_dialog();
}

static void handle_input_mission_briefing(const struct mouse_t *m, const struct hotkeys_t *h)
{
    rich_text_handle_mouse(mouse_in_dialog(m));
    if (m->right.went_up || h->escape_pressed || h->enter_pressed) {
        rich_text_reset(0);
        stop_sound_channel(SOUND_CHANNEL_SPEECH);
        update_music(1);
        window_city_show();
        return;
    }
}

void window_mission_briefing_show(void)
{
    struct window_type_t window = {
        WINDOW_MISSION_BRIEFING,
        draw_background_mission_briefing,
        0,
        handle_input_mission_briefing,
    };
    rich_text_reset(0);
    window_show(&window);
}

static void draw_background_mission_end(void)
{
    graphics_in_dialog();
    if (city_victory_state() == VICTORY_STATE_WON) {
        outer_panel_draw(48, 128, 34, 17);
        lang_text_draw_centered(62, 0, 48, 144, 544, FONT_LARGE_BLACK);
        inner_panel_draw(64, 184, 32, 7);
        lang_text_draw_multiline(147, 20, 80, 192, 488, FONT_NORMAL_WHITE);
        int left_offset = 68;
        int right_offset = 315;
        int width = lang_text_draw(148, 0, left_offset, 308, FONT_NORMAL_BLACK);
        text_draw_number(city_data.ratings.culture, '@', " ", left_offset + width, 308, FONT_NORMAL_BLACK);
        width = lang_text_draw(148, 1, right_offset, 308, FONT_NORMAL_BLACK);
        text_draw_number(city_data.ratings.prosperity, '@', " ", right_offset + width, 308, FONT_NORMAL_BLACK);
        width = lang_text_draw(148, 2, left_offset, 328, FONT_NORMAL_BLACK);
        text_draw_number(city_data.ratings.peace, '@', " ", left_offset + width, 328, FONT_NORMAL_BLACK);
        width = lang_text_draw(148, 3, right_offset, 328, FONT_NORMAL_BLACK);
        text_draw_number(city_data.ratings.favor, '@', " ", right_offset + width, 328, FONT_NORMAL_BLACK);
        width = lang_text_draw(148, 4, left_offset, 348, FONT_NORMAL_BLACK);
        text_draw_number(city_data.population.population, '@', " ", left_offset + width, 348, FONT_NORMAL_BLACK);
        width = lang_text_draw(148, 5, right_offset, 348, FONT_NORMAL_BLACK);
        text_draw_number(city_data.finance.treasury, '@', " ", right_offset + width, 348, FONT_NORMAL_BLACK);
    } else {
        outer_panel_draw(48, 16, 34, 13);
        lang_text_draw_centered(62, 1, 48, 32, 544, FONT_LARGE_BLACK);
        lang_text_draw_multiline(62, 16, 72, 75, 496, FONT_NORMAL_BLACK);
    }
    graphics_reset_dialog();
}

static void handle_input_mission_end(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        stop_music();
        stop_sound_channel(SOUND_CHANNEL_SPEECH);
        city_victory_stop_governing();
        game_undo_disable();
        game_state_reset_overlay();
        window_main_menu_show(1);
    }
}

static void show_end_dialog(void)
{
    struct window_type_t window = {
        WINDOW_MISSION_END,
        draw_background_mission_end,
        0,
        handle_input_mission_end,
    };
    window_show(&window);
}

static void show_intermezzo(void)
{
    window_intermezzo_show(INTERMEZZO_WON, show_end_dialog);
}

static void handle_input_victory_video(const struct mouse_t *m, __attribute__((unused)) const struct hotkeys_t *h)
{
    if (m->left.went_up || m->right.went_up || video_is_finished()) {
        video_stop();
        victory_video_data.callback();
    }
}

static void window_victory_video_show(const char *filename, int width, int height, void (*callback)(void))
{
    if (video_start(filename)) {
        victory_video_data.width = width;
        victory_video_data.height = height;
        victory_video_data.callback = callback;
        video_init(0);
        struct window_type_t window = {
            WINDOW_VICTORY_VIDEO,
            graphics_clear_screen,
            video_draw_fullscreen,
            handle_input_victory_video,
        };
        window_show(&window);
    } else {
        callback();
    }
}

void window_mission_end_show_won(void)
{
    mouse_reset_up_state();
    if (setting_victory_video()) {
        window_victory_video_show("smk/victory_balcony.smk", 400, 292, show_intermezzo);
    } else {
        window_victory_video_show("smk/victory_senate.smk", 400, 292, show_intermezzo);
    }
}

void window_mission_end_show_fired(void)
{
    window_intermezzo_show(INTERMEZZO_FIRED, show_end_dialog);
}

static void input_number_numeric_input(int number)
{
    if (numeric_input_data.num_digits < numeric_input_data.max_digits) {
        numeric_input_data.value = numeric_input_data.value * 10 + number;
        numeric_input_data.num_digits++;
        play_sound_effect(SOUND_EFFECT_BUILD);
    }
}

static void button_number_numeric_input(int number, __attribute__((unused)) int param2)
{
    input_number_numeric_input(number);
}

static void close_numeric_input(void)
{
    keyboard_stop_capture_numeric();
    window_go_back();
}

static void button_cancel_numeric_input(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    close_numeric_input();
}

static void input_accept_numeric_input(void)
{
    close_numeric_input();
    if (numeric_input_data.value > numeric_input_data.max_value) {
        numeric_input_data.value = numeric_input_data.max_value;
    }
    numeric_input_data.callback(numeric_input_data.value);
}

static void button_accept_numeric_input(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    input_accept_numeric_input();
}

static struct generic_button_t buttons_numeric_input[] = {
    {21, 51, 25, 25, button_number_numeric_input, button_none, 1, 0},
    {51, 51, 25, 25, button_number_numeric_input, button_none, 2, 0},
    {81, 51, 25, 25, button_number_numeric_input, button_none, 3, 0},
    {21, 81, 25, 25, button_number_numeric_input, button_none, 4, 0},
    {51, 81, 25, 25, button_number_numeric_input, button_none, 5, 0},
    {81, 81, 25, 25, button_number_numeric_input, button_none, 6, 0},
    {21, 111, 25, 25, button_number_numeric_input, button_none, 7, 0},
    {51, 111, 25, 25, button_number_numeric_input, button_none, 8, 0},
    {81, 111, 25, 25, button_number_numeric_input, button_none, 9, 0},
    {21, 141, 25, 25, button_number_numeric_input, button_none, 0, 0},
    {51, 141, 55, 25, button_accept_numeric_input, button_none, 1, 0},
    {21, 171, 85, 25, button_cancel_numeric_input, button_none, 1, 0}
};

static void draw_number_button(int x, int y, int number, int is_selected)
{
    color_t color = is_selected ? COLOR_FONT_BLUE : COLOR_BLACK;
    graphics_draw_rect(x, y, 25, 25, color);
    char number_string[2];
    number_string[0] = '0' + number;
    number_string[1] = 0;
    text_draw_centered(number_string, x, y, 25, FONT_LARGE_PLAIN, color);
}

static void draw_foreground_numeric_input(void)
{
    outer_panel_draw(numeric_input_data.x, numeric_input_data.y, 8, 14);
    graphics_fill_rect(numeric_input_data.x + 16, numeric_input_data.y + 16, 96, 30, COLOR_BLACK);
    if (numeric_input_data.num_digits > 0) {
        text_draw_number_centered_colored(numeric_input_data.value, numeric_input_data.x + 16, numeric_input_data.y + 19, 92, FONT_LARGE_PLAIN, COLOR_FONT_RED);
    }
    draw_number_button(numeric_input_data.x + 21, numeric_input_data.y + 51, 1, numeric_input_data.focus_button_id == 1);
    draw_number_button(numeric_input_data.x + 51, numeric_input_data.y + 51, 2, numeric_input_data.focus_button_id == 2);
    draw_number_button(numeric_input_data.x + 81, numeric_input_data.y + 51, 3, numeric_input_data.focus_button_id == 3);
    draw_number_button(numeric_input_data.x + 21, numeric_input_data.y + 81, 4, numeric_input_data.focus_button_id == 4);
    draw_number_button(numeric_input_data.x + 51, numeric_input_data.y + 81, 5, numeric_input_data.focus_button_id == 5);
    draw_number_button(numeric_input_data.x + 81, numeric_input_data.y + 81, 6, numeric_input_data.focus_button_id == 6);
    draw_number_button(numeric_input_data.x + 21, numeric_input_data.y + 111, 7, numeric_input_data.focus_button_id == 7);
    draw_number_button(numeric_input_data.x + 51, numeric_input_data.y + 111, 8, numeric_input_data.focus_button_id == 8);
    draw_number_button(numeric_input_data.x + 81, numeric_input_data.y + 111, 9, numeric_input_data.focus_button_id == 9);
    draw_number_button(numeric_input_data.x + 21, numeric_input_data.y + 141, 0, numeric_input_data.focus_button_id == 10);
    graphics_draw_rect(numeric_input_data.x + 51, numeric_input_data.y + 141, 55, 25, numeric_input_data.focus_button_id == 11 ? COLOR_FONT_BLUE : COLOR_BLACK);
    lang_text_draw_centered_colored(44, 16, numeric_input_data.x + 51, numeric_input_data.y + 147, 55, FONT_NORMAL_PLAIN,
            numeric_input_data.focus_button_id == 11 ? COLOR_FONT_BLUE : COLOR_BLACK);

    graphics_draw_rect(numeric_input_data.x + 21, numeric_input_data.y + 171, 85, 25, numeric_input_data.focus_button_id == 12 ? COLOR_FONT_BLUE : COLOR_BLACK);
    lang_text_draw_centered_colored(44, 17, numeric_input_data.x + 21, numeric_input_data.y + 177, 85, FONT_NORMAL_PLAIN,
            numeric_input_data.focus_button_id == 12 ? COLOR_FONT_BLUE : COLOR_BLACK);
}

static void handle_input_numeric_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (generic_buttons_handle_mouse(m, numeric_input_data.x, numeric_input_data.y, buttons_numeric_input, 12, &numeric_input_data.focus_button_id)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        close_numeric_input();
    }
    if (h->enter_pressed) {
        input_accept_numeric_input();
    }
}

void window_numeric_input_show(int x, int y, int max_digits, int max_value, void (*callback)(int))
{
    struct window_type_t window = {
        WINDOW_NUMERIC_INPUT,
        window_draw_underlying_window,
        draw_foreground_numeric_input,
        handle_input_numeric_input,
    };
    numeric_input_data.x = x;
    numeric_input_data.y = y;
    numeric_input_data.max_digits = max_digits;
    numeric_input_data.max_value = max_value;
    numeric_input_data.callback = callback;
    numeric_input_data.num_digits = 0;
    numeric_input_data.value = 0;
    numeric_input_data.focus_button_id = 0;
    keyboard_start_capture_numeric(input_number_numeric_input);
    window_show(&window);
}

static void close_submenu(void)
{
    overlay_menu_data.keep_submenu_open = 0;
    overlay_menu_data.selected_menu = 0;
    overlay_menu_data.selected_submenu = 0;
    overlay_menu_data.num_submenu_items = 0;
}

static void open_submenu(int index, int keep_open)
{
    overlay_menu_data.keep_submenu_open = keep_open;
    overlay_menu_data.selected_menu = index;
    overlay_menu_data.selected_submenu = MENU_ID_TO_SUBMENU_ID[index];
    overlay_menu_data.num_submenu_items = 0;
    for (int i = 0; i < 8 && SUBMENU_ID_TO_OVERLAY[overlay_menu_data.selected_submenu][i] > 0; i++) {
        overlay_menu_data.num_submenu_items++;
    }
}

static void button_menu_item_overlay_menu(int index, __attribute__((unused)) int param2)
{
    if (MENU_ID_TO_SUBMENU_ID[index] == 0) {
        game_state_set_overlay(MENU_ID_TO_OVERLAY[index]);
        close_submenu();
        window_city_show();
    } else {
        if (overlay_menu_data.keep_submenu_open && overlay_menu_data.selected_submenu == MENU_ID_TO_SUBMENU_ID[index]) {
            close_submenu();
        } else {
            open_submenu(index, 1);
        }
    }
}

static void button_submenu_item_overlay_menu(int index, int param2); static void button_submenu_item_overlay_menu(int index, __attribute__((unused)) int param2)
{
    int overlay = SUBMENU_ID_TO_OVERLAY[overlay_menu_data.selected_submenu][index];
    if (overlay) {
        game_state_set_overlay(overlay);
    }
    close_submenu();
    window_city_show();
}

static struct generic_button_t menu_buttons[] = {
    {0, 0, 160, 24, button_menu_item_overlay_menu, button_none, 0, 0},
    {0, 24, 160, 24, button_menu_item_overlay_menu, button_none, 1, 0},
    {0, 48, 160, 24, button_menu_item_overlay_menu, button_none, 2, 0},
    {0, 72, 160, 24, button_menu_item_overlay_menu, button_none, 3, 0},
    {0, 96, 160, 24, button_menu_item_overlay_menu, button_none, 4, 0},
    {0, 120, 160, 24, button_menu_item_overlay_menu, button_none, 5, 0},
    {0, 144, 160, 24, button_menu_item_overlay_menu, button_none, 6, 0},
    {0, 168, 160, 24, button_menu_item_overlay_menu, button_none, 7, 0},
    {0, 192, 160, 24, button_menu_item_overlay_menu, button_none, 8, 0},
    {0, 216, 160, 24, button_menu_item_overlay_menu, button_none, 9, 0},
};

static struct generic_button_t submenu_buttons[] = {
    {0, 0, 160, 24, button_submenu_item_overlay_menu, button_none, 0, 0},
    {0, 24, 160, 24, button_submenu_item_overlay_menu, button_none, 1, 0},
    {0, 48, 160, 24, button_submenu_item_overlay_menu, button_none, 2, 0},
    {0, 72, 160, 24, button_submenu_item_overlay_menu, button_none, 3, 0},
    {0, 96, 160, 24, button_submenu_item_overlay_menu, button_none, 4, 0},
    {0, 120, 160, 24, button_submenu_item_overlay_menu, button_none, 5, 0},
    {0, 144, 160, 24, button_submenu_item_overlay_menu, button_none, 6, 0},
    {0, 168, 160, 24, button_submenu_item_overlay_menu, button_none, 7, 0},
    {0, 192, 160, 24, button_submenu_item_overlay_menu, button_none, 8, 0},
    {0, 216, 160, 24, button_submenu_item_overlay_menu, button_none, 9, 0},
};

static int get_sidebar_x_offset_overlay_menu(void)
{
    int view_x, view_y, view_width, view_height;
    city_view_get_viewport(&view_x, &view_y, &view_width, &view_height);
    return view_x + view_width;
}

static void draw_foreground_overlay_menu(void)
{
    widget_city_draw();
    int x_offset = get_sidebar_x_offset_overlay_menu();
    for (int i = 0; i < 8; i++) {
        label_draw(x_offset - 170, 74 + 24 * i, 10, overlay_menu_data.menu_focus_button_id == i + 1 ? 1 : 2);
        lang_text_draw_centered(14, MENU_ID_TO_OVERLAY[i], x_offset - 170, 77 + 24 * i, 160, FONT_NORMAL_GREEN);
    }
    if (overlay_menu_data.selected_submenu > 0) {
        image_draw(image_group(GROUP_BULLET), x_offset - 185, 80 + 24 * overlay_menu_data.selected_menu);
        for (int i = 0; i < overlay_menu_data.num_submenu_items; i++) {
            label_draw(x_offset - 348, 74 + 24 * (i + overlay_menu_data.selected_menu),
                10, overlay_menu_data.submenu_focus_button_id == i + 1 ? 1 : 2);
            lang_text_draw_centered(14, SUBMENU_ID_TO_OVERLAY[overlay_menu_data.selected_submenu][i],
                x_offset - 348, 77 + 24 * (i + overlay_menu_data.selected_menu), 160, FONT_NORMAL_GREEN);
        }
    }
}

static void handle_input_overlay_menu(const struct mouse_t *m, const struct hotkeys_t *h)
{
    int x_offset = get_sidebar_x_offset_overlay_menu();
    int handled = 0;
    handled |= generic_buttons_handle_mouse(m, x_offset - MENU_X_OFFSET_OVERLAY_MENU, MENU_Y_OFFSET_OVERLAY_MENU,
        menu_buttons, MAX_BUTTONS_OVERLAY_MENU, &overlay_menu_data.menu_focus_button_id);

    if (!overlay_menu_data.keep_submenu_open) {
        if (overlay_menu_data.menu_focus_button_id || overlay_menu_data.submenu_focus_button_id) {
            overlay_menu_data.submenu_focus_time = time_get_millis();
            if (overlay_menu_data.menu_focus_button_id) {
                open_submenu(overlay_menu_data.menu_focus_button_id - 1, 0);
            }
        } else if (time_get_millis() - overlay_menu_data.submenu_focus_time > 500) {
            close_submenu();
        }
    }
    if (overlay_menu_data.selected_submenu) {
        handled |= generic_buttons_handle_mouse(
            m, x_offset - SUBMENU_X_OFFSET, MENU_Y_OFFSET_OVERLAY_MENU + MENU_ITEM_HEIGHT * overlay_menu_data.selected_menu,
            submenu_buttons, overlay_menu_data.num_submenu_items, &overlay_menu_data.submenu_focus_button_id);
    }
    if (!handled && (m->right.went_up || h->escape_pressed)) {
        if (overlay_menu_data.keep_submenu_open) {
            close_submenu();
        } else {
            window_city_show();
        }
        return;
    }
    if (!handled
    && (m->left.went_up && // click outside menu
        (m->x < x_offset - MENU_CLICK_MARGIN - (overlay_menu_data.selected_submenu ? SUBMENU_X_OFFSET : MENU_X_OFFSET_OVERLAY_MENU) ||
            m->x > x_offset + MENU_CLICK_MARGIN ||
            m->y < MENU_Y_OFFSET_OVERLAY_MENU - MENU_CLICK_MARGIN ||
            m->y > MENU_Y_OFFSET_OVERLAY_MENU + MENU_CLICK_MARGIN + MENU_ITEM_HEIGHT * MAX_BUTTONS_OVERLAY_MENU))) {
        close_submenu();
        window_city_show();
    }
}

void window_overlay_menu_show(void)
{
    struct window_type_t window = {
        WINDOW_OVERLAY_MENU,
        window_city_draw_background,
        draw_foreground_overlay_menu,
        handle_input_overlay_menu,
    };
    overlay_menu_data.selected_submenu = 0;
    overlay_menu_data.num_submenu_items = 0;
    window_show(&window);
}

void window_popup_dialog_show(int type, void (*close_func)(void), int has_ok_cancel_buttons)
{
    if (init_popup_dialog(type, 0, 0, close_func, has_ok_cancel_buttons)) {
        struct window_type_t window = {
            WINDOW_POPUP_DIALOG,
            draw_background_popup_dialog,
            draw_foreground_popup_dialog,
            handle_input_popup_dialog,
        };
        window_show(&window);
    }
}

static int items_in_first_list(void)
{
    return select_list_data.num_items / 2 + select_list_data.num_items % 2;
}

static void select_item_select_list(int id, int list_id)
{
    window_go_back();
    if (list_id == 0) {
        select_list_data.callback(id);
    } else {
        select_list_data.callback(id + items_in_first_list());
    }
}

static struct generic_button_t buttons_list1[MAX_ITEMS_PER_LIST] = {
    {5, 8, 190, 18, select_item_select_list, button_none, 0, 0},
    {5, 28, 190, 18, select_item_select_list, button_none, 1, 0},
    {5, 48, 190, 18, select_item_select_list, button_none, 2, 0},
    {5, 68, 190, 18, select_item_select_list, button_none, 3, 0},
    {5, 88, 190, 18, select_item_select_list, button_none, 4, 0},
    {5, 108, 190, 18, select_item_select_list, button_none, 5, 0},
    {5, 128, 190, 18, select_item_select_list, button_none, 6, 0},
    {5, 148, 190, 18, select_item_select_list, button_none, 7, 0},
    {5, 168, 190, 18, select_item_select_list, button_none, 8, 0},
    {5, 188, 190, 18, select_item_select_list, button_none, 9, 0},
    {5, 208, 190, 18, select_item_select_list, button_none, 10, 0},
    {5, 228, 190, 18, select_item_select_list, button_none, 11, 0},
    {5, 248, 190, 18, select_item_select_list, button_none, 12, 0},
    {5, 268, 190, 18, select_item_select_list, button_none, 13, 0},
    {5, 288, 190, 18, select_item_select_list, button_none, 14, 0},
    {5, 308, 190, 18, select_item_select_list, button_none, 15, 0},
    {5, 328, 190, 18, select_item_select_list, button_none, 16, 0},
    {5, 348, 190, 18, select_item_select_list, button_none, 17, 0},
    {5, 368, 190, 18, select_item_select_list, button_none, 18, 0},
    {5, 388, 190, 18, select_item_select_list, button_none, 19, 0},
};

static struct generic_button_t buttons_list2[MAX_ITEMS_PER_LIST] = {
    {205, 8, 190, 18, select_item_select_list, button_none, 0, 1},
    {205, 28, 190, 18, select_item_select_list, button_none, 1, 1},
    {205, 48, 190, 18, select_item_select_list, button_none, 2, 1},
    {205, 68, 190, 18, select_item_select_list, button_none, 3, 1},
    {205, 88, 190, 18, select_item_select_list, button_none, 4, 1},
    {205, 108, 190, 18, select_item_select_list, button_none, 5, 1},
    {205, 128, 190, 18, select_item_select_list, button_none, 6, 1},
    {205, 148, 190, 18, select_item_select_list, button_none, 7, 1},
    {205, 168, 190, 18, select_item_select_list, button_none, 8, 1},
    {205, 188, 190, 18, select_item_select_list, button_none, 9, 1},
    {205, 208, 190, 18, select_item_select_list, button_none, 10, 1},
    {205, 228, 190, 18, select_item_select_list, button_none, 11, 1},
    {205, 248, 190, 18, select_item_select_list, button_none, 12, 1},
    {205, 268, 190, 18, select_item_select_list, button_none, 13, 1},
    {205, 288, 190, 18, select_item_select_list, button_none, 14, 1},
    {205, 308, 190, 18, select_item_select_list, button_none, 15, 1},
    {205, 328, 190, 18, select_item_select_list, button_none, 16, 1},
    {205, 348, 190, 18, select_item_select_list, button_none, 17, 1},
    {205, 368, 190, 18, select_item_select_list, button_none, 18, 1},
    {205, 388, 190, 18, select_item_select_list, button_none, 19, 1},
};

static void draw_item(int item_id, int x, int y, int selected)
{
    color_t color = selected ? COLOR_FONT_BLUE : COLOR_BLACK;
    if (select_list_data.mode == MODE_GROUP) {
        lang_text_draw_centered_colored(select_list_data.group, item_id, select_list_data.x + x, select_list_data.y + y, 190, FONT_NORMAL_PLAIN, color);
    } else {
        text_draw_centered(select_list_data.items[item_id], select_list_data.x + x, select_list_data.y + y, 190, FONT_NORMAL_PLAIN, color);
    }
}

static void draw_foreground_select_list(void)
{
    if (select_list_data.num_items > MAX_ITEMS_PER_LIST) {
        int max_first = items_in_first_list();
        outer_panel_draw(select_list_data.x, select_list_data.y, 26, (20 * max_first + 24) / BLOCK_SIZE);
        for (int i = 0; i < max_first; i++) {
            draw_item(i, 5, 11 + 20 * i, i + 1 == select_list_data.focus_button_id);
        }
        for (int i = 0; i < select_list_data.num_items - max_first; i++) {
            draw_item(i + max_first, 205, 11 + 20 * i, MAX_ITEMS_PER_LIST + i + 1 == select_list_data.focus_button_id);
        }
    } else {
        outer_panel_draw(select_list_data.x, select_list_data.y, 13, (20 * select_list_data.num_items + 24) / BLOCK_SIZE);
        for (int i = 0; i < select_list_data.num_items; i++) {
            draw_item(i, 5, 11 + 20 * i, i + 1 == select_list_data.focus_button_id);
        }
    }
}

static void handle_input_select_list(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (select_list_data.num_items > MAX_ITEMS_PER_LIST) {
        int items_first = items_in_first_list();
        if (generic_buttons_handle_mouse(m, select_list_data.x, select_list_data.y, buttons_list1, items_first, &select_list_data.focus_button_id)) {
            return;
        }
        int second_id = 0;
        generic_buttons_handle_mouse(m, select_list_data.x, select_list_data.y, buttons_list2, select_list_data.num_items - items_first, &second_id);
        if (second_id > 0) {
            select_list_data.focus_button_id = second_id + MAX_ITEMS_PER_LIST;
        }
    } else {
        if (generic_buttons_handle_mouse(m, select_list_data.x, select_list_data.y, buttons_list1, select_list_data.num_items, &select_list_data.focus_button_id)) {
            return;
        }
    }
    if (m->right.went_up || h->escape_pressed) {
        window_go_back();
    }
}

void window_select_list_show(int x, int y, int group, int num_items, void (*callback)(int))
{
    struct window_type_t window = {
        WINDOW_SELECT_LIST,
        window_draw_underlying_window,
        draw_foreground_select_list,
        handle_input_select_list,
    };
    select_list_data.x = x;
    select_list_data.y = y;
    select_list_data.mode = MODE_GROUP;
    select_list_data.group = group;
    select_list_data.num_items = num_items;
    select_list_data.callback = callback;
    window_show(&window);
}

void window_select_list_show_text(int x, int y, char **items, int num_items, void (*callback)(int))
{
    struct window_type_t window = {
        WINDOW_SELECT_LIST,
        window_draw_underlying_window,
        draw_foreground_select_list,
        handle_input_select_list,
    };
    select_list_data.x = x;
    select_list_data.y = y;
    select_list_data.mode = MODE_TEXT;
    select_list_data.items = items;
    select_list_data.num_items = num_items;
    select_list_data.callback = callback;
    window_show(&window);
}

static void button_toggle_sound_options(int type, __attribute__((unused)) int param2)
{
    setting_toggle_sound_enabled(type);
    if (type == SOUND_MUSIC) {
        if (get_sound(SOUND_MUSIC)->enabled) {
            update_music(1);
        } else {
            stop_music();
        }
    } else if (type == SOUND_SPEECH) {
        if (!get_sound(SOUND_SPEECH)->enabled) {
            stop_sound_channel(SOUND_CHANNEL_SPEECH);
        }
    }
}

static struct generic_button_t buttons_sound_options[] = {
    {64, 162, 224, 20, button_toggle_sound_options, button_none, SOUND_MUSIC, 0},
    {64, 192, 224, 20, button_toggle_sound_options, button_none, SOUND_SPEECH, 0},
    {64, 222, 224, 20, button_toggle_sound_options, button_none, SOUND_EFFECTS, 0},
    {64, 252, 224, 20, button_toggle_sound_options, button_none, SOUND_CITY, 0},
};

static void update_volume(int type, int is_decrease)
{
    if (is_decrease) {
        setting_decrease_sound_volume(type);
    } else {
        setting_increase_sound_volume(type);
    }
}

static void arrow_button_music(int is_down, __attribute__((unused)) int param2)
{
    update_volume(SOUND_MUSIC, is_down);
    Mix_VolumeMusic(percentage_to_volume(get_sound(SOUND_MUSIC)->volume));
}

static void arrow_button_speech(int is_down, __attribute__((unused)) int param2)
{
    update_volume(SOUND_SPEECH, is_down);
    set_channel_volume(SOUND_CHANNEL_SPEECH, get_sound(SOUND_SPEECH)->volume);
}

static void arrow_button_effects(int is_down, __attribute__((unused)) int param2)
{
    update_volume(SOUND_EFFECTS, is_down);
    set_sound_effect_volume(get_sound(SOUND_EFFECTS)->volume);
}

static void arrow_button_city(int is_down, __attribute__((unused)) int param2)
{
    update_volume(SOUND_CITY, is_down);
    set_city_sounds_volume(get_sound(SOUND_CITY)->volume);
}

static struct arrow_button_t arrow_buttons_sound_options[] = {
    {112, 100, 17, 24, arrow_button_music, 1, 0, 0, 0},
    {136, 100, 15, 24, arrow_button_music, 0, 0, 0, 0},
    {112, 130, 17, 24, arrow_button_speech, 1, 0, 0, 0},
    {136, 130, 15, 24, arrow_button_speech, 0, 0, 0, 0},
    {112, 160, 17, 24, arrow_button_effects, 1, 0, 0, 0},
    {136, 160, 15, 24, arrow_button_effects, 0, 0, 0, 0},
    {112, 190, 17, 24, arrow_button_city, 1, 0, 0, 0},
    {136, 190, 15, 24, arrow_button_city, 0, 0, 0, 0},
};

static void draw_foreground_sound_options(void)
{
    graphics_in_dialog();

    outer_panel_draw(48, 80, 24, 15);

    // on/off labels
    label_draw(64, 162, 14, sound_options_data.focus_button_id == 1 ? 1 : 2);
    label_draw(64, 192, 14, sound_options_data.focus_button_id == 2 ? 1 : 2);
    label_draw(64, 222, 14, sound_options_data.focus_button_id == 3 ? 1 : 2);
    label_draw(64, 252, 14, sound_options_data.focus_button_id == 4 ? 1 : 2);

    // title
    lang_text_draw_centered(46, 0, 96, 92, 288, FONT_LARGE_BLACK);

    lang_text_draw(46, 10, 112, 142, FONT_SMALL_PLAIN);
    lang_text_draw(46, 11, 336, 142, FONT_SMALL_PLAIN);

    struct set_sound_t *music = get_sound(SOUND_MUSIC);
    lang_text_draw_centered(46, music->enabled ? 2 : 1, 64, 166, 224, FONT_NORMAL_GREEN);
    text_draw_percentage(music->volume, 374, 166, FONT_NORMAL_PLAIN);

    struct set_sound_t *speech = get_sound(SOUND_SPEECH);
    lang_text_draw_centered(46, speech->enabled ? 4 : 3, 64, 196, 224, FONT_NORMAL_GREEN);
    text_draw_percentage(speech->volume, 374, 196, FONT_NORMAL_PLAIN);

    struct set_sound_t *effects = get_sound(SOUND_EFFECTS);
    lang_text_draw_centered(46, effects->enabled ? 6 : 5, 64, 226, 224, FONT_NORMAL_GREEN);
    text_draw_percentage(effects->volume, 374, 226, FONT_NORMAL_PLAIN);

    struct set_sound_t *city = get_sound(SOUND_CITY);
    lang_text_draw_centered(46, city->enabled ? 8 : 7, 64, 256, 224, FONT_NORMAL_GREEN);
    text_draw_percentage(city->volume, 374, 256, FONT_NORMAL_PLAIN);

    arrow_buttons_draw(208, 60, arrow_buttons_sound_options, sizeof(arrow_buttons_sound_options) / sizeof(struct arrow_button_t));

    graphics_reset_dialog();
}

static void handle_input_sound_options(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_sound_options, 6, &sound_options_data.focus_button_id) ||
        arrow_buttons_handle_mouse(m_dialog, 208, 60, arrow_buttons_sound_options, sizeof(arrow_buttons_sound_options) / sizeof(struct arrow_button_t), 0)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        if (sound_options_data.from_editor) {
            show_editor_map();
        } else {
            window_city_return();
        }
    }
}

void window_sound_options_show(int from_editor)
{
    struct window_type_t window = {
        WINDOW_SOUND_OPTIONS,
        window_draw_underlying_window,
        draw_foreground_sound_options,
        handle_input_sound_options,
    };
    sound_options_data.focus_button_id = 0;
    sound_options_data.from_editor = from_editor;
    sound_options_data.original_effects = *get_sound(SOUND_EFFECTS);
    sound_options_data.original_music = *get_sound(SOUND_MUSIC);
    sound_options_data.original_speech = *get_sound(SOUND_SPEECH);
    sound_options_data.original_city = *get_sound(SOUND_CITY);
    window_show(&window);
}

static void arrow_button_game(int is_down, __attribute__((unused)) int param2)
{
    if (is_down) {
        setting_decrease_game_speed();
    } else {
        setting_increase_game_speed();
    }
}

static void arrow_button_scroll(int is_down, __attribute__((unused)) int param2)
{
    if (is_down) {
        setting_decrease_scroll_speed();
    } else {
        setting_increase_scroll_speed();
    }
}

static struct arrow_button_t arrow_buttons_speed_options[] = {
    {112, 100, 17, 24, arrow_button_game, 1, 0, 0, 0},
    {136, 100, 15, 24, arrow_button_game, 0, 0, 0, 0},
    {112, 136, 17, 24, arrow_button_scroll, 1, 0, 0, 0},
    {136, 136, 15, 24, arrow_button_scroll, 0, 0, 0, 0},
};

static void draw_foreground_speed_options(void)
{
    graphics_in_dialog();

    outer_panel_draw(80, 80, 20, 10);

    // title
    lang_text_draw_centered(45, 0, 96, 92, 288, FONT_LARGE_BLACK);
    // game speed
    lang_text_draw(45, 2, 112, 146, FONT_NORMAL_PLAIN);
    text_draw_percentage(setting_game_speed(), 328, 146, FONT_NORMAL_PLAIN);
    // scroll speed
    lang_text_draw(45, 3, 112, 182, FONT_NORMAL_PLAIN);
    text_draw_percentage(setting_scroll_speed(), 328, 182, FONT_NORMAL_PLAIN);

    arrow_buttons_draw(160, 40, arrow_buttons_speed_options, sizeof(arrow_buttons_speed_options) / sizeof(struct arrow_button_t));
    graphics_reset_dialog();
}

static void handle_input_speed_options(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (arrow_buttons_handle_mouse(m_dialog, 160, 40, arrow_buttons_speed_options, sizeof(arrow_buttons_speed_options) / sizeof(struct arrow_button_t), 0)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        if (speed_options_data.from_editor) {
            show_editor_map();
        } else {
            window_city_return();
        }
    }
}

void window_speed_options_show(int from_editor)
{
    struct window_type_t window = {
        WINDOW_SPEED_OPTIONS,
        window_draw_underlying_window,
        draw_foreground_speed_options,
        handle_input_speed_options,
    };
    speed_options_data.focus_button_id = 0;
    speed_options_data.from_editor = from_editor;
    speed_options_data.original_game_speed = setting_game_speed();
    speed_options_data.original_scroll_speed = setting_scroll_speed();
    window_show(&window);
}

static void button_continue_governing(int months, __attribute__((unused)) int param2)
{
    city_victory_continue_governing(months);
    window_city_show();
    city_victory_reset();
    update_music(1);
}

static void button_accept_victory_dialog(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_city_show();
}

static struct generic_button_t victory_buttons[] = {
    {32, 112, 480, 20, button_accept_victory_dialog, button_none, 0, 0},
    {32, 144, 480, 20, button_continue_governing, button_none, 24, 0},
    {32, 176, 480, 20, button_continue_governing, button_none, 60, 0},
};

static void draw_background_victory_dialog(void)
{
    graphics_in_dialog();

    outer_panel_draw(48, 128, 34, 15);
    lang_text_draw_centered(62, 0, 48, 144, 544, FONT_LARGE_BLACK);
    text_draw_centered(scenario_settings.player_name, 48, 194, 544, FONT_LARGE_BLACK, 0);
    graphics_reset_dialog();
}

static void draw_foreground_victory_dialog(void)
{
    graphics_in_dialog();

    // Accept promotion
    large_label_draw(80, 240, 30, focus_button_id_victory_dialog == 1);
    lang_text_draw_centered(62, 3, 80, 246, 480, FONT_NORMAL_GREEN);
    // Continue for 2 years
    large_label_draw(80, 272, 30, focus_button_id_victory_dialog == 2);
    lang_text_draw_centered(62, 4, 80, 278, 480, FONT_NORMAL_GREEN);
    // Continue for 5 years
    large_label_draw(80, 304, 30, focus_button_id_victory_dialog == 3);
    lang_text_draw_centered(62, 5, 80, 310, 480, FONT_NORMAL_GREEN);

    graphics_reset_dialog();
}

static void handle_input_victory_dialog(const struct mouse_t *m, __attribute__((unused)) const struct hotkeys_t *h)
{
    generic_buttons_handle_mouse(mouse_in_dialog(m), 48, 128, victory_buttons, 3, &focus_button_id_victory_dialog);
}

void window_victory_dialog_show(void)
{
    struct window_type_t window = {
        WINDOW_VICTORY_DIALOG,
        draw_background_victory_dialog,
        draw_foreground_victory_dialog,
        handle_input_victory_dialog,
    };
    window_show(&window);
}

int sidebar_common_get_x_offset_expanded(void)
{
    return screen_width() - SIDEBAR_EXPANDED_WIDTH;
}

void sidebar_common_draw_relief(int x_offset, int y_offset, int image, int is_collapsed)
{
    // relief images below panel
    int image_base = image_group(image);
    int image_offset = image == GROUP_SIDE_PANEL ? 2 : 1;
    int y_max = screen_height();
    while (y_offset < y_max) {
        if (y_max - y_offset <= 120) {
            image_draw(image_base + image_offset + is_collapsed, x_offset, y_offset);
            y_offset += 120;
        } else {
            image_draw(image_base + image_offset + image_offset + is_collapsed, x_offset, y_offset);
            y_offset += 285;
        }
    }
}

static void city_draw_bridge_tile(int x, int y, int bridge_sprite_id, color_t color_mask)
{
    int image_id = image_group(GROUP_BUILDING_BRIDGE);
    switch (bridge_sprite_id) {
        case 1:
            image_draw_masked(image_id + 5, x, y - 20, color_mask);
            break;
        case 2:
            image_draw_masked(image_id, x - 1, y - 8, color_mask);
            break;
        case 3:
            image_draw_masked(image_id + 3, x, y - 8, color_mask);
            break;
        case 4:
            image_draw_masked(image_id + 2, x + 7, y - 20, color_mask);
            break;
        case 5:
            image_draw_masked(image_id + 4, x, y - 21, color_mask);
            break;
        case 6:
            image_draw_masked(image_id + 1, x + 5, y - 21, color_mask);
            break;
        case 7:
            image_draw_masked(image_id + 11, x - 3, y - 50, color_mask);
            break;
        case 8:
            image_draw_masked(image_id + 6, x - 1, y - 12, color_mask);
            break;
        case 9:
            image_draw_masked(image_id + 9, x - 30, y - 12, color_mask);
            break;
        case 10:
            image_draw_masked(image_id + 8, x - 23, y - 53, color_mask);
            break;
        case 11:
            image_draw_masked(image_id + 10, x, y - 37, color_mask);
            break;
        case 12:
            image_draw_masked(image_id + 7, x + 7, y - 38, color_mask);
            break;
            // Note: no nr 13
        case 14:
            image_draw_masked(image_id + 13, x, y - 38, color_mask);
            break;
        case 15:
            image_draw_masked(image_id + 12, x + 7, y - 38, color_mask);
            break;
    }
}

static void city_draw_bridge(int x, int y, int grid_offset)
{
    if (!map_terrain_is(grid_offset, TERRAIN_WATER)) {
        map_sprite_clear_tile(grid_offset);
        return;
    }
    if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        return;
    }
    color_t color_mask = 0;
    if (map_property_is_deleted(grid_offset)) {
        color_mask = COLOR_MASK_RED;
    }
    city_draw_bridge_tile(x, y, map_sprite_bridge_at(grid_offset), color_mask);
}

static int building_preview_blocked(int grid_offset, int num_tiles, int *blocked_tiles, int type)
{
    int orientation_index = city_view_orientation() / 2;
    int blocked = 0;
    for (int i = 0; i < num_tiles; i++) {
        int tile_offset = grid_offset + TILE_GRID_OFFSETS[orientation_index][i];
        int forbidden_terrain = terrain_grid.items[tile_offset] & TERRAIN_NOT_CLEAR;
        if (type == BUILDING_GATEHOUSE || type == BUILDING_TRIUMPHAL_ARCH) {
            forbidden_terrain &= ~TERRAIN_ROAD;
        }
        if (type == BUILDING_TOWER) {
            forbidden_terrain &= ~TERRAIN_WALL;
        }
        if (forbidden_terrain || map_has_figure_at(tile_offset)) {
            blocked_tiles[i] = 1;
            blocked = 1;
        } else {
            blocked_tiles[i] = 0;
        }
    }
    return blocked;
}

static void draw_blocked_building_preview(int x, int y, int num_tiles, int *blocked_tiles, int completely_blocked)
{
    for (int i = 0; i < num_tiles; i++) {
        int x_offset = x + X_VIEW_OFFSETS[i];
        int y_offset = y + Y_VIEW_OFFSETS[i];
        if (completely_blocked || blocked_tiles[i]) {
            image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x_offset, y_offset, COLOR_MASK_RED);
        } else {
            image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x_offset, y_offset, COLOR_MASK_GREEN);
        }
    }
}

static void draw_building(int image_id, int x, int y)
{
    image_draw_isometric_footprint(image_id, x, y, COLOR_MASK_GREEN);
    image_draw_isometric_top(image_id, x, y, COLOR_MASK_GREEN);
}

static void draw_water_range_preview(int x, int y, int radius)
{
    int image_id = image_group(GROUP_TERRAIN_FLAT_TILE);
    for (int i = 1; i <= radius; i++) {
        for (int j = 0; j <= radius; j++) {
            image_draw_blend_alpha(image_id, x + HALF_TILE_WIDTH_PIXELS * (i + j), y - HALF_TILE_HEIGHT_PIXELS * (i - j), COLOR_MASK_BLUE);
            image_draw_blend_alpha(image_id, x + HALF_TILE_WIDTH_PIXELS * (i - j), y + HALF_TILE_HEIGHT_PIXELS * (i + j), COLOR_MASK_BLUE);
            image_draw_blend_alpha(image_id, x - HALF_TILE_WIDTH_PIXELS * (i + j), y + HALF_TILE_HEIGHT_PIXELS * (i - j), COLOR_MASK_BLUE);
            image_draw_blend_alpha(image_id, x - HALF_TILE_WIDTH_PIXELS * (i - j), y - HALF_TILE_HEIGHT_PIXELS * (i + j), COLOR_MASK_BLUE);
        }
    }
}

static int city_building_ghost_mark_deleting(const struct map_tile_t *tile)
{
    if (!config_get(CONFIG_UI_VISUAL_FEEDBACK_ON_DELETE)) {
        return 0;
    }
    int construction_type = building_construction_type();
    if (!tile->grid_offset || building_construction_draw_as_constructing() ||
        scroll_in_progress() || construction_type != BUILDING_CLEAR_LAND) {
        return (construction_type == BUILDING_CLEAR_LAND);
    }
    if (!building_construction_in_progress()) {
        map_property_clear_constructing_and_deleted();
    }
    map_building_tiles_mark_deleting(tile->grid_offset);
    return 1;
}

static int is_road_tile_for_aqueduct(int grid_offset, int gate_orientation)
{
    int is_road = map_terrain_is(grid_offset, TERRAIN_ROAD) ? 1 : 0;
    if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        struct building_t *b = &all_buildings[map_building_at(grid_offset)];
        if (b->type == BUILDING_GATEHOUSE) {
            if (b->subtype.orientation == gate_orientation) {
                is_road = 1;
            }
        } else if (b->type == BUILDING_GRANARY) {
            if (terrain_land_citizen.items[grid_offset] == CITIZEN_0_ROAD) {
                is_road = 1;
            }
        }
    }
    return is_road;
}

static void city_building_ghost_draw(const struct map_tile_t *tile)
{
    if (!tile->grid_offset || scroll_in_progress()) {
        return;
    }

    int type = building_construction_type();
    if (building_construction_draw_as_constructing() || type == BUILDING_NONE || type == BUILDING_CLEAR_LAND) {
        return;
    }

    int x, y;
    city_view_get_selected_tile_pixels(&x, &y);

    struct building_properties_t *building_props = &building_properties[type];
    int building_size = type == BUILDING_WAREHOUSE ? 3 : building_props->size;
    int num_tiles = building_size * building_size;
    int blocked_tiles[num_tiles];
    int orientation_index = city_view_orientation() / 2;

    if (!city_finance_can_afford(building_properties[type].cost)) {
        draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
        if (type == BUILDING_HIPPODROME) {
            int blocked_tiles2[MAX_TILES];
            int blocked_tiles3[MAX_TILES];
            draw_blocked_building_preview(x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles2, 1);
            draw_blocked_building_preview(x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles3, 1);
        } else if (building_is_fort(type)) {
            int num_fort_ground_tiles = building_properties[BUILDING_FORT_GROUND].size * building_properties[BUILDING_FORT_GROUND].size;
            int blocked_tiles_ground[MAX_TILES];
            draw_blocked_building_preview(x + FORT_GROUND_X_VIEW_OFFSETS[orientation_index], y + FORT_GROUND_Y_VIEW_OFFSETS[orientation_index], num_fort_ground_tiles, blocked_tiles_ground, 1);
        }
        return;
    }

    int x_dir_offset = 0;
    int y_dir_offset = 0;
    switch (city_view_orientation()) {
        case DIR_0_TOP:
            x_dir_offset = tile->x;
            y_dir_offset = tile->y;
            break;
        case DIR_2_RIGHT:
            x_dir_offset = tile->x - building_size + 1;
            y_dir_offset = tile->y;
            break;
        case DIR_4_BOTTOM:
            x_dir_offset = tile->x - building_size + 1;
            y_dir_offset = tile->y - building_size + 1;
            break;
        case DIR_6_LEFT:
            x_dir_offset = tile->x;
            y_dir_offset = tile->y - building_size + 1;
            break;
        default:
            x_dir_offset = 0;
            y_dir_offset = 0;
    }
    if (!check_building_terrain_requirements(x_dir_offset, y_dir_offset, 0)) {
        draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
        return;
    }
    int image_id = image_group(building_props->image_group) + building_props->image_offset;
    if (type == BUILDING_GATEHOUSE) {
        int orientation = map_orientation_for_gatehouse(tile->x, tile->y);
        int image_offset;
        if (orientation == 2) {
            image_offset = 1;
        } else if (orientation == 1) {
            image_offset = 0;
        } else {
            image_offset = building_construction_road_orientation() == 2 ? 1 : 0;
        }
        int map_orientation = city_view_orientation();
        if (map_orientation == DIR_6_LEFT || map_orientation == DIR_2_RIGHT) {
            image_offset = 1 - image_offset;
        }
        image_id += image_offset;
    } else if (type == BUILDING_TRIUMPHAL_ARCH) {
        int orientation = map_orientation_for_triumphal_arch(tile->x, tile->y);
        int image_offset;
        if (orientation == 2) {
            image_offset = 2;
        } else if (orientation == 1) {
            image_offset = 0;
        } else {
            image_offset = building_construction_road_orientation() == 2 ? 2 : 0;
        }
        int map_orientation = city_view_orientation();
        if (map_orientation == DIR_6_LEFT || map_orientation == DIR_2_RIGHT) {
            image_offset = 2 - image_offset;
        }
        image_id += image_offset;
    }
    int dir_absolute;
    int dir_relative;
    switch (type) {
        case BUILDING_HOUSE_VACANT_LOT:
            if (map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            draw_building(image_group(GROUP_BUILDING_HOUSE_VACANT_LOT), x, y);
            return;
        case BUILDING_ROAD:
            if (map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            if (map_terrain_is(tile->grid_offset, TERRAIN_AQUEDUCT)) {
                if (map_can_place_road_under_aqueduct(tile->grid_offset)) {
                    draw_building(image_group(GROUP_BUILDING_AQUEDUCT) + map_get_aqueduct_with_road_image(tile->grid_offset), x, y);
                } else {
                    image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                }
            } else {
                if (!map_terrain_has_adjacent_x_with_type(tile->grid_offset, TERRAIN_ROAD) &&
                    map_terrain_has_adjacent_y_with_type(tile->grid_offset, TERRAIN_ROAD)) {
                    image_id++;
                }
                draw_building(image_id, x, y);
            }
            return;
        case BUILDING_RESERVOIR:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            // y + 30 to center mouse, 11 instead of 10 range to compensate for building size
            draw_water_range_preview(x, y + 30, 11);
            draw_building(image_id, x, y);
            if (map_terrain_exists_tile_in_area_with_type(tile->x - 1, tile->y - 1, 5, TERRAIN_WATER)) {
                const struct image_t *img = image_get(image_id);
                int x_water = x - 58 + img->sprite_offset_x - 2;
                int y_water = y + img->sprite_offset_y - (img->height - 90);
                image_draw_masked(image_id + 1, x_water, y_water, COLOR_MASK_GREEN);
            }
            return;
        case BUILDING_AQUEDUCT:
            if (map_terrain_is(tile->grid_offset, TERRAIN_ROAD)) {
                int map_is_straight_road_for_aqueduct = 0;
                int road_tiles_x =
                    is_road_tile_for_aqueduct(tile->grid_offset + map_grid_delta(1, 0), 2) +
                    is_road_tile_for_aqueduct(tile->grid_offset + map_grid_delta(-1, 0), 2);
                int road_tiles_y =
                    is_road_tile_for_aqueduct(tile->grid_offset + map_grid_delta(0, -1), 1) +
                    is_road_tile_for_aqueduct(tile->grid_offset + map_grid_delta(0, 1), 1);
                if ((road_tiles_x == 2 && road_tiles_y == 0)
                || (road_tiles_y == 2 && road_tiles_x == 0)) {
                    map_is_straight_road_for_aqueduct = 1;
                }
                if (!map_is_straight_road_for_aqueduct || map_property_is_plaza_or_earthquake(tile->grid_offset)) {
                    image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                    return;
                }
            } else if (map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            const struct terrain_image_t *terrain_img = map_image_context_get_aqueduct(tile->grid_offset, 1);
            if (map_terrain_is(tile->grid_offset, TERRAIN_ROAD)) {
                int group_offset = terrain_img->group_offset;
                if (!terrain_img->aqueduct_offset) {
                    if (map_terrain_is(tile->grid_offset + map_grid_delta(0, -1), TERRAIN_ROAD)) {
                        group_offset = 3;
                    } else {
                        group_offset = 2;
                    }
                }
                if (map_tiles_is_paved_road(tile->grid_offset)) {
                    image_id += group_offset + 13;
                } else {
                    image_id += group_offset + 21;
                }
            } else {
                image_id += terrain_img->group_offset + 15;
            }
            draw_building(image_id, x, y);
            return;
        case BUILDING_FOUNTAIN:
            if (map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            draw_water_range_preview(x, y, scenario.climate == CLIMATE_DESERT ? 3 : 4);
            draw_building(image_id, x, y);
            if (map_terrain_is(tile->grid_offset, TERRAIN_RESERVOIR_RANGE)) {
                const struct image_t *img = image_get(image_id);
                image_draw_masked(image_id + 1, x + img->sprite_offset_x, y + img->sprite_offset_y, COLOR_MASK_GREEN);
            }
            return;
        case BUILDING_WELL:
            if (map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            draw_water_range_preview(x, y, 2);
            draw_building(image_id, x, y);
            return;
        case BUILDING_BATHHOUSE:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            for (int i = 0; i < num_tiles; i++) {
                if (map_terrain_is(tile->grid_offset + TILE_GRID_OFFSETS[orientation_index][i], TERRAIN_RESERVOIR_RANGE)) {
                    const struct image_t *img = image_get(image_id);
                    image_draw_masked(image_id - 1, x + img->sprite_offset_x - 7, y + img->sprite_offset_y + 6, COLOR_MASK_GREEN);
                    break;
                }
            }
            return;
        case BUILDING_HIPPODROME:
        {
            int blocked_tiles2[25];
            int blocked_tiles3[25];
            if (city_data.building.hippodrome_placed) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                draw_blocked_building_preview(x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles2, 1);
                draw_blocked_building_preview(x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles3, 1);
                return;
            }
            int blocked1 = building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type);
            int blocked2 = building_preview_blocked(tile->grid_offset + map_grid_delta(5, 0), num_tiles, blocked_tiles2, type);
            int blocked3 = building_preview_blocked(tile->grid_offset + map_grid_delta(10, 0), num_tiles, blocked_tiles3, type);
            if (blocked1 || blocked2 || blocked3) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                draw_blocked_building_preview(x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles2, 0);
                draw_blocked_building_preview(x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles3, 0);
                return;
            }
            if (orientation_index == 0) {
                image_id = image_group(GROUP_BUILDING_HIPPODROME_2);
                // part 1, 2, 3
                draw_building(image_id, x, y);
                draw_building(image_id + 2, x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id + 4, x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
            } else if (orientation_index == 1) {
                image_id = image_group(GROUP_BUILDING_HIPPODROME_1);
                // part 3, 2, 1
                draw_building(image_id, x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id + 2, x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id + 4, x, y);
            } else if (orientation_index == 2) {
                image_id = image_group(GROUP_BUILDING_HIPPODROME_2);
                // part 1, 2, 3
                draw_building(image_id + 4, x, y);
                draw_building(image_id + 2, x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id, x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
            } else if (orientation_index == 3) {
                image_id = image_group(GROUP_BUILDING_HIPPODROME_1);
                // part 3, 2, 1
                draw_building(image_id + 4, x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id + 2, x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id, x, y);
            }
            return;
        }
        case BUILDING_SENATE:
            if (city_data.building.senate_placed) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                return;
            }
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            return;
        case BUILDING_TRIUMPHAL_ARCH:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            const struct image_t *img = image_get(image_id + 1);
            if (image_id == image_group(GROUP_BUILDING_TRIUMPHAL_ARCH)) {
                image_draw_masked(image_id + 1, x + img->sprite_offset_x + 4, y + img->sprite_offset_y - 51, COLOR_MASK_GREEN);
            } else {
                image_draw_masked(image_id + 1, x + img->sprite_offset_x - 33, y + img->sprite_offset_y - 56, COLOR_MASK_GREEN);
            }
            return;
        case BUILDING_PLAZA:
            if (!map_terrain_is(tile->grid_offset, TERRAIN_ROAD)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            draw_building(image_id, x, y);
            return;
        case BUILDING_LOW_BRIDGE:
        case BUILDING_SHIP_BRIDGE:
            int length, direction;
            int end_grid_offset = map_bridge_calculate_length_direction(tile->x, tile->y, &length, &direction);
            int dir = direction - city_view_orientation();
            if (dir < 0) {
                dir += 8;
            }
            int blocked = 0;
            if (type == BUILDING_SHIP_BRIDGE && length < 5) {
                blocked = 1;
            } else if (!end_grid_offset) {
                blocked = 1;
            }
            int x_delta, y_delta;
            switch (dir) {
                case DIR_0_TOP:
                    x_delta = 29;
                    y_delta = -15;
                    break;
                case DIR_2_RIGHT:
                    x_delta = 29;
                    y_delta = 15;
                    break;
                case DIR_4_BOTTOM:
                    x_delta = -29;
                    y_delta = 15;
                    break;
                case DIR_6_LEFT:
                    x_delta = -29;
                    y_delta = -15;
                    break;
                default:
                    return;
            }
            if (blocked) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, length > 0 ? COLOR_MASK_GREEN : COLOR_MASK_RED);
                if (length > 1) {
                    image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x + x_delta * (length - 1), y + y_delta * (length - 1), COLOR_MASK_RED);
                }
                building_construction_set_cost(0);
            } else {
                if (dir == DIR_0_TOP || dir == DIR_6_LEFT) {
                    for (int i = length - 1; i >= 0; i--) {
                        int sprite_id = map_bridge_get_sprite_id(i, length, dir, type == BUILDING_SHIP_BRIDGE);
                        city_draw_bridge_tile(x + x_delta * i, y + y_delta * i, sprite_id, COLOR_MASK_GREEN);
                    }
                } else {
                    for (int i = 0; i < length; i++) {
                        int sprite_id = map_bridge_get_sprite_id(i, length, dir, type == BUILDING_SHIP_BRIDGE);
                        city_draw_bridge_tile(x + x_delta * i, y + y_delta * i, sprite_id, COLOR_MASK_GREEN);
                    }
                }
                building_construction_set_cost(building_properties[type].cost * length);
            }
            break;
        case BUILDING_SHIPYARD:
        case BUILDING_WHARF:
            if (map_water_determine_orientation_size2(tile->x, tile->y, 1, &dir_absolute, &dir_relative)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                return;
            }
            image_id = image_group(building_props->image_group) + building_props->image_offset + dir_relative;
            draw_building(image_id, x, y);
            return;
        case BUILDING_DOCK:
            if (map_water_determine_orientation_size3(tile->x, tile->y, 1, &dir_absolute, &dir_relative)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                return;
            }
            switch (dir_relative) {
                case 0: image_id = image_group(GROUP_BUILDING_DOCK_1); break;
                case 1: image_id = image_group(GROUP_BUILDING_DOCK_2); break;
                case 2: image_id = image_group(GROUP_BUILDING_DOCK_3); break;
                default:image_id = image_group(GROUP_BUILDING_DOCK_4); break;
            }
            draw_building(image_id, x, y);
            return;
        case BUILDING_GATEHOUSE:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            // update road required based on timer
            building_construction_update_road_orientation();
            draw_building(image_id, x, y);
            return;
        case BUILDING_FORT_LEGIONARIES:
        case BUILDING_FORT_JAVELIN:
        case BUILDING_FORT_MOUNTED:
        {
            int num_fort_ground_tiles = building_properties[BUILDING_FORT_GROUND].size * building_properties[BUILDING_FORT_GROUND].size;
            int blocked_tiles_ground[MAX_TILES];
            if (city_data.military.total_legions >= MAX_LEGIONS) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                draw_blocked_building_preview(x + FORT_GROUND_X_VIEW_OFFSETS[orientation_index], y + FORT_GROUND_Y_VIEW_OFFSETS[orientation_index], num_fort_ground_tiles, blocked_tiles_ground, 1);
                return;
            }
            int blocked1_f = building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type);
            int blocked2_f = building_preview_blocked(tile->grid_offset + FORT_GROUND_GRID_OFFSETS[orientation_index], num_fort_ground_tiles, blocked_tiles_ground, type);
            if (blocked1_f || blocked2_f) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                draw_blocked_building_preview(x + FORT_GROUND_X_VIEW_OFFSETS[orientation_index], y + FORT_GROUND_Y_VIEW_OFFSETS[orientation_index], num_fort_ground_tiles, blocked_tiles_ground, 0);
                return;
            }
            if (orientation_index == 0 || orientation_index == 3) {
                draw_building(image_id, x, y);
                draw_building(image_id + 1, x + FORT_GROUND_X_VIEW_OFFSETS[orientation_index], y + FORT_GROUND_Y_VIEW_OFFSETS[orientation_index]);
            } else {
                draw_building(image_id + 1, x + FORT_GROUND_X_VIEW_OFFSETS[orientation_index], y + FORT_GROUND_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id, x, y);
            }
            return;
        }
        case BUILDING_BARRACKS:
            if (building_count_total(BUILDING_BARRACKS)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                return;
            }
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            return;
        case BUILDING_WHEAT_FARM:
        case BUILDING_VEGETABLE_FARM:
        case BUILDING_FRUIT_FARM:
        case BUILDING_OLIVE_FARM:
        case BUILDING_VINES_FARM:
        case BUILDING_PIG_FARM:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            // fields
            for (int i = 4; i < 9; i++) {
                image_draw_isometric_footprint(image_id + 1, x + X_VIEW_OFFSETS[i], y + Y_VIEW_OFFSETS[i], COLOR_MASK_GREEN);
            }
            return;
        case BUILDING_GRANARY:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            image_draw_isometric_footprint(image_id, x, y, COLOR_MASK_GREEN);
            const struct image_t *img_granary = image_get(image_id + 1);
            image_draw_masked(image_id + 1, x + img_granary->sprite_offset_x - 32, y + img_granary->sprite_offset_y - 64, COLOR_MASK_GREEN);
            return;
        case BUILDING_WAREHOUSE:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            image_draw_masked(image_group(GROUP_BUILDING_WAREHOUSE) + 17, x - 4, y - 42, COLOR_MASK_GREEN);
            for (int i = 1; i < 9; i++) {
                draw_building(EMPTY_WAREHOUSE_IMG_ID, x + X_VIEW_OFFSETS[i], y + Y_VIEW_OFFSETS[i]);
            }
            return;
        default:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            return;
    }
}

static void draw_figure_with_cart(const struct figure_t *f, int x, int y)
{
    if (f->y_offset_cart >= 0) {
        image_draw(f->image_id, x, y);
        image_draw(f->cart_image_id, x + f->x_offset_cart, y + f->y_offset_cart);
    } else {
        image_draw(f->cart_image_id, x + f->x_offset_cart, y + f->y_offset_cart);
        image_draw(f->image_id, x, y);
    }
}

static void adjust_pixel_offset(struct figure_t *f, int *pixel_x, int *pixel_y)
{
    // determining x/y offset on tile
    int x_offset = 0;
    int y_offset = 0;
    if (f->use_cross_country) {
        int dir = city_view_orientation();
        if (dir == DIR_0_TOP || dir == DIR_4_BOTTOM) {
            int base_pixel_x = 2 * f->cross_country_x % 15 - 2 * f->cross_country_y % 15;
            int base_pixel_y = f->cross_country_x % 15 + f->cross_country_y % 15;
            x_offset = dir == DIR_0_TOP ? base_pixel_x : -base_pixel_x;
            y_offset = dir == DIR_0_TOP ? base_pixel_y : -base_pixel_y;
        } else {
            int base_pixel_x = 2 * f->cross_country_x % 15 + 2 * f->cross_country_y % 15;
            int base_pixel_y = f->cross_country_x % 15 - f->cross_country_y % 15;
            x_offset = dir == DIR_2_RIGHT ? base_pixel_x : -base_pixel_x;
            y_offset = dir == DIR_6_LEFT ? base_pixel_y : -base_pixel_y;
        }
        y_offset -= f->missile_offset;
    } else {
        int direction = figure_image_normalize_direction(f->direction);

        if (f->progress_on_tile >= 15) {
            x_offset = 0;
        } else {
            switch (direction) {
                case DIR_0_TOP:
                case DIR_2_RIGHT:
                    x_offset = 2 * f->progress_on_tile - 28;
                    break;
                case DIR_1_TOP_RIGHT:
                    x_offset = 4 * f->progress_on_tile - 56;
                    break;
                case DIR_4_BOTTOM:
                case DIR_6_LEFT:
                    x_offset = 28 - 2 * f->progress_on_tile;
                    break;
                case DIR_5_BOTTOM_LEFT:
                    x_offset = 56 - 4 * f->progress_on_tile;
                    break;
                default:
                    x_offset = 0;
                    break;
            }
        }
        if (f->progress_on_tile >= 15) {
            y_offset = 0;
        } else {
            switch (direction) {
                case DIR_0_TOP:
                case DIR_6_LEFT:
                    y_offset = 14 - f->progress_on_tile;
                    break;
                case DIR_2_RIGHT:
                case DIR_4_BOTTOM:
                    y_offset = f->progress_on_tile - 14;
                    break;
                case DIR_3_BOTTOM_RIGHT:
                    y_offset = 2 * f->progress_on_tile - 28;
                    break;
                case DIR_7_TOP_LEFT:
                    y_offset = 28 - 2 * f->progress_on_tile;
                    break;
                default:
                    y_offset = 0;
                    break;
            }
        }
        y_offset -= f->current_height;
    }

    x_offset += 29;
    y_offset += 15;

    struct image_t *img = (figure_properties[f->type].is_native_unit || (figure_properties[f->type].is_enemy_unit && f->type != FIGURE_ENEMY_GLADIATOR)) ? image_get_enemy(f) : image_get(f->image_id);
    *pixel_x += x_offset - img->sprite_offset_x;
    *pixel_y += y_offset - img->sprite_offset_y;
}

static void draw_figure_city(struct figure_t *f, int x, int y, int highlight)
{
    if (f->cart_image_id) {
        switch (f->type) {
            case FIGURE_CART_PUSHER:
            case FIGURE_WAREHOUSEMAN:
            case FIGURE_LION_TAMER:
            case FIGURE_DOCKER:
            case FIGURE_NATIVE_TRADER:
            case FIGURE_IMMIGRANT:
            case FIGURE_EMIGRANT:
                draw_figure_with_cart(f, x, y);
                break;
            case FIGURE_HIPPODROME_HORSES:
                int val = f->wait_ticks_missile;
                switch (city_view_orientation()) {
                    case DIR_0_TOP:
                        x += 10;
                        if (val <= 10) {
                            y -= 2;
                        } else if (val <= 11) {
                            y -= 10;
                        } else if (val <= 12) {
                            y -= 18;
                        } else if (val <= 13) {
                            y -= 16;
                        } else if (val <= 20) {
                            y -= 14;
                        } else if (val <= 21) {
                            y -= 10;
                        } else {
                            y -= 2;
                        }
                        break;
                    case DIR_2_RIGHT:
                        x -= 10;
                        if (val <= 9) {
                            y -= 12;
                        } else if (val <= 10) {
                            y += 4;
                        } else if (val <= 11) {
                            x -= 5;
                            y += 2;
                        } else if (val <= 13) {
                            x -= 5;
                        } else if (val <= 20) {
                            y -= 2;
                        } else if (val <= 21) {
                            y -= 6;
                        } else {
                            y -= 12;
                        }
                        /* fall through */
                    case DIR_4_BOTTOM:
                        x += 20;
                        if (val <= 9) {
                            y += 4;
                        } else if (val <= 10) {
                            x += 10;
                            y += 4;
                        } else if (val <= 11) {
                            x += 10;
                            y -= 4;
                        } else if (val <= 13) {
                            y -= 6;
                        } else if (val <= 20) {
                            y -= 12;
                        } else if (val <= 21) {
                            y -= 10;
                        } else {
                            y -= 2;
                        }
                        break;
                    case DIR_6_LEFT:
                        x -= 10;
                        if (val <= 9) {
                            y -= 12;
                        } else if (val <= 10) {
                            y += 4;
                        } else if (val <= 11) {
                            y += 2;
                        } else if (val <= 13) {
                            // no change
                        } else if (val <= 20) {
                            y -= 2;
                        } else if (val <= 21) {
                            y -= 6;
                        } else {
                            y -= 12;
                        }
                        break;
                }
                draw_figure_with_cart(f, x, y);
                break;
            case FIGURE_FORT_STANDARD:
                if (!legion_formations[f->formation_id].in_distant_battle) {
                    // base
                    image_draw(f->image_id, x, y);
                    // flag
                    int flag_height = image_get(f->cart_image_id)->height;
                    image_draw(f->cart_image_id, x, y - flag_height);
                    // top icon
                    int icon_image_id = image_group(GROUP_FIGURE_FORT_STANDARD_ICONS) + legion_formations[f->formation_id].id;
                    image_draw(icon_image_id, x, y - image_get(icon_image_id)->height - flag_height);
                }
                break;
            case FIGURE_MAP_FLAG:
                // base
                image_draw(f->image_id, x, y);
                // flag
                image_draw(f->cart_image_id, x, y - image_get(f->cart_image_id)->height);
                // flag number
                int number = 0;
                int id = f->resource_id;
                if (id >= MAP_FLAG_EARTHQUAKE_MIN && id <= MAP_FLAG_EARTHQUAKE_MAX) {
                    number = id - MAP_FLAG_EARTHQUAKE_MIN + 1;
                } else if (id >= MAP_FLAG_INVASION_MIN && id <= MAP_FLAG_INVASION_MAX) {
                    number = id - MAP_FLAG_INVASION_MIN + 1;
                } else if (id >= MAP_FLAG_FISHING_MIN && id <= MAP_FLAG_FISHING_MAX) {
                    number = id - MAP_FLAG_FISHING_MIN + 1;
                } else if (id >= MAP_FLAG_HERD_MIN && id <= MAP_FLAG_HERD_MAX) {
                    number = id - MAP_FLAG_HERD_MIN + 1;
                }
                if (number > 0) {
                    text_draw_number_colored(number, '@', " ", x + 6, y + 7, FONT_NORMAL_PLAIN, COLOR_WHITE);
                }
                break;
            default:
                image_draw(f->image_id, x, y);
                break;
        }
    } else {
        if (figure_properties[f->type].is_native_unit || figure_properties[f->type].is_enemy_unit) {
            image_draw_enemy(f, x, y);
        } else {
            image_draw(f->image_id, x, y);
            if (highlight) {
                image_draw_blend_alpha(f->image_id, x, y, COLOR_MASK_LEGION_HIGHLIGHT);
            }
        }
    }
}

static int is_multi_tile_terrain(int grid_offset)
{
    return !map_building_at(grid_offset) && map_property_multi_tile_size(grid_offset) > 1;
}

static void draw_footprint_with_overlay(int x, int y, int grid_offset)
{
    building_construction_record_view_position(x, y, grid_offset);
    if (grid_offset < 0) {
        // Outside map: draw black tile
        image_draw_isometric_footprint_from_draw_tile(image_group(GROUP_TERRAIN_BLACK), x, y, 0);
    } else if (overlay->draw_custom_footprint) {
        overlay->draw_custom_footprint(x, y, grid_offset);
    } else if (map_property_is_draw_tile(grid_offset)) {
        if (terrain_grid.items[grid_offset] & (TERRAIN_AQUEDUCT | TERRAIN_WALL)) {
            // display grass
            int image_id = image_group(GROUP_TERRAIN_GRASS_1) + (map_random_get(grid_offset) & 7);
            image_draw_isometric_footprint_from_draw_tile(image_id, x, y, 0);
        } else if ((terrain_grid.items[grid_offset] & TERRAIN_ROAD) && !(terrain_grid.items[grid_offset] & TERRAIN_BUILDING)) {
            image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, 0);
        } else if (terrain_grid.items[grid_offset] & TERRAIN_BUILDING) {
            city_with_overlay_draw_building_footprint(x, y, grid_offset, 0);
        } else {
            image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, 0);
        }
    }
}

static void draw_top_with_overlay(int x, int y, int grid_offset)
{
    if (overlay->draw_custom_top) {
        overlay->draw_custom_top(x, y, grid_offset);
    } else if (map_property_is_draw_tile(grid_offset)) {
        if (!map_terrain_is(grid_offset, TERRAIN_WALL | TERRAIN_AQUEDUCT | TERRAIN_ROAD)) {
            if (map_terrain_is(grid_offset, TERRAIN_BUILDING) && map_building_at(grid_offset)) {
                city_with_overlay_draw_building_top(x, y, grid_offset);
            } else if (!map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
                color_t color_mask = 0;
                if (map_property_is_deleted(grid_offset) && !is_multi_tile_terrain(grid_offset)) {
                    color_mask = COLOR_MASK_RED;
                }
                // terrain
                image_draw_isometric_top_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
            }
        }
    }
}

static void draw_animation_with_overlay(int x, int y, int grid_offset)
{
    int draw = 0;
    if (map_building_at(grid_offset)) {
        int btype = all_buildings[map_building_at(grid_offset)].type;
        switch (overlay->type) {
            case OVERLAY_FIRE:
            case OVERLAY_CRIME:
                if (btype == BUILDING_PREFECTURE || btype == BUILDING_BURNING_RUIN) {
                    draw = 1;
                }
                break;
            case OVERLAY_DAMAGE:
                if (btype == BUILDING_ENGINEERS_POST) {
                    draw = 1;
                }
                break;
            case OVERLAY_WATER:
                if (btype == BUILDING_RESERVOIR || btype == BUILDING_FOUNTAIN) {
                    draw = 1;
                }
                break;
            case OVERLAY_FOOD_STOCKS:
                if (btype == BUILDING_MARKET || btype == BUILDING_GRANARY) {
                    draw = 1;
                }
                break;
        }
    }

    int image_id = map_image_at(grid_offset);
    const struct image_t *img = image_get(image_id);
    if (img->num_animation_sprites && draw) {
        if (map_property_is_draw_tile(grid_offset)) {
            struct building_t *b = &all_buildings[map_building_at(grid_offset)];
            int color_mask = draw_building_as_deleted_1(b) ? COLOR_MASK_RED : 0;
            if (b->type == BUILDING_GRANARY) {
                image_draw_masked(image_group(GROUP_BUILDING_GRANARY) + 1,
                                  x + img->sprite_offset_x,
                                  y + 60 + img->sprite_offset_y - img->height,
                                  color_mask);
                if (b->data.granary.resource_stored[RESOURCE_NONE] < 2400) {
                    image_draw_masked(image_group(GROUP_BUILDING_GRANARY) + 2, x + 33, y - 60, color_mask);
                }
                if (b->data.granary.resource_stored[RESOURCE_NONE] < 1800) {
                    image_draw_masked(image_group(GROUP_BUILDING_GRANARY) + 3, x + 56, y - 50, color_mask);
                }
                if (b->data.granary.resource_stored[RESOURCE_NONE] < 1200) {
                    image_draw_masked(image_group(GROUP_BUILDING_GRANARY) + 4, x + 91, y - 50, color_mask);
                }
                if (b->data.granary.resource_stored[RESOURCE_NONE] < 600) {
                    image_draw_masked(image_group(GROUP_BUILDING_GRANARY) + 5, x + 117, y - 62, color_mask);
                }
            } else {
                int animation_offset = building_animation_offset(b, image_id, grid_offset);
                if (animation_offset > 0) {
                    if (animation_offset > img->num_animation_sprites) {
                        animation_offset = img->num_animation_sprites;
                    }
                    int ydiff = 0;
                    switch (map_property_multi_tile_size(grid_offset)) {
                        case 1: ydiff = 30; break;
                        case 2: ydiff = 45; break;
                        case 3: ydiff = 60; break;
                        case 4: ydiff = 75; break;
                        case 5: ydiff = 90; break;
                    }
                    image_draw_masked(image_id + animation_offset,
                                      x + img->sprite_offset_x,
                                      y + ydiff + img->sprite_offset_y - img->height,
                                      color_mask);
                }
            }
        }
    } else if (map_is_bridge(grid_offset)) {
        city_draw_bridge(x, y, grid_offset);
    }
}

void city_draw_figure(struct figure_t *f, int x, int y, int highlight)
{
    adjust_pixel_offset(f, &x, &y);
    draw_figure_city(f, x, y, highlight);
}

static void draw_figures_with_overlay(int x, int y, int grid_offset)
{
    int figure_id = map_figure_at(grid_offset);
    while (figure_id) {
        struct figure_t *f = &figures[figure_id];
        if (!f->is_invisible && overlay->show_figure(f)) {
            city_draw_figure(f, x, y, 0);
        }
        figure_id = f->next_figure_id_on_same_tile;
    }
}

static void draw_elevated_figures_with_overlay(int x, int y, int grid_offset)
{
    int figure_id = map_figure_at(grid_offset);
    while (figure_id > 0) {
        struct figure_t *f = &figures[figure_id];
        if (((f->use_cross_country && !f->is_invisible) || f->height_adjusted_ticks) && overlay->show_figure(f)) {
            city_draw_figure(f, x, y, 0);
        }
        figure_id = f->next_figure_id_on_same_tile;
    }
}

static int should_draw_top_before_deletion_with_overlay(int grid_offset)
{
    int has_adjacent_deletion = 0;
    int size = map_property_multi_tile_size(grid_offset);
    int total_adjacent_offsets = size * 2 + 1;
    const int *adjacent_offset = ADJACENT_OFFSETS[size - 1][city_view_orientation() / 2];
    for (int i = 0; i < total_adjacent_offsets; ++i) {
        if (map_property_is_deleted(grid_offset + adjacent_offset[i]) ||
            draw_building_as_deleted_1(&all_buildings[map_building_at(grid_offset + adjacent_offset[i])])) {
            has_adjacent_deletion = 1;
            break;
        }
    }
    return is_multi_tile_terrain(grid_offset) && has_adjacent_deletion;
}

static void deletion_draw_terrain_top_with_overlay(int x, int y, int grid_offset)
{
    if (map_property_is_draw_tile(grid_offset) && should_draw_top_before_deletion_with_overlay(grid_offset)) {
        draw_top_with_overlay(x, y, grid_offset);
    }
}

static void deletion_draw_animations(int x, int y, int grid_offset)
{
    if (map_property_is_deleted(grid_offset) || draw_building_as_deleted_1(&all_buildings[map_building_at(grid_offset)])) {
        image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
    }
    if (map_property_is_draw_tile(grid_offset) && !should_draw_top_before_deletion_with_overlay(grid_offset)) {
        draw_top_with_overlay(x, y, grid_offset);
    }
    draw_animation_with_overlay(x, y, grid_offset);
}

static int draw_building_as_deleted_2(struct building_t *b)
{
    b = building_main(b);
    return (b->id && (b->is_deleted || map_property_is_deleted(b->grid_offset)));
}

static void draw_footprint_without_overlay(int x, int y, int grid_offset)
{
    building_construction_record_view_position(x, y, grid_offset);
    if (grid_offset < 0) {
        // Outside map: draw black tile
        image_draw_isometric_footprint_from_draw_tile(image_group(GROUP_TERRAIN_BLACK), x, y, 0);
    } else if (map_property_is_draw_tile(grid_offset)) {
        // Valid grid_offset and leftmost tile -> draw
        int building_id = map_building_at(grid_offset);
        color_t color_mask = 0;
        if (building_id || map_terrain_is(grid_offset, TERRAIN_GARDEN) || map_terrain_is(grid_offset, TERRAIN_AQUEDUCT)) {
            int view_x, view_y, view_width, view_height;
            city_view_get_viewport(&view_x, &view_y, &view_width, &view_height);
            if (building_id) {
                struct building_t *b = &all_buildings[building_id];
                if (draw_building_as_deleted_2(b)) {
                    color_mask = COLOR_MASK_RED;
                }
                if (x < view_x + 100) {
                    city_sounds__mark_building_view(b->type, b->num_workers, SOUND_DIRECTION_LEFT);
                } else if (x > view_x + view_width - 100) {
                    city_sounds__mark_building_view(b->type, b->num_workers, SOUND_DIRECTION_RIGHT);
                } else {
                    city_sounds__mark_building_view(b->type, b->num_workers, SOUND_DIRECTION_CENTER);
                }
            } else if (map_terrain_is(grid_offset, TERRAIN_GARDEN)) {
                if (x < view_x + 100) {
                    city_sounds__mark_building_view(BUILDING_GARDENS, 0, SOUND_DIRECTION_LEFT);
                } else if (x > view_x + view_width - 100) {
                    city_sounds__mark_building_view(BUILDING_GARDENS, 0, SOUND_DIRECTION_RIGHT);
                } else {
                    city_sounds__mark_building_view(BUILDING_GARDENS, 0, SOUND_DIRECTION_CENTER);
                }
            } else if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT)) {
                if (x < view_x + 100) {
                    city_sounds__mark_building_view(BUILDING_AQUEDUCT, 0, SOUND_DIRECTION_LEFT);
                } else if (x > view_x + view_width - 100) {
                    city_sounds__mark_building_view(BUILDING_AQUEDUCT, 0, SOUND_DIRECTION_RIGHT);
                } else {
                    city_sounds__mark_building_view(BUILDING_AQUEDUCT, 0, SOUND_DIRECTION_CENTER);
                }
            }
        }
        int image_id = map_image_at(grid_offset);
        if (map_property_is_constructing(grid_offset)) {
            image_id = image_group(GROUP_TERRAIN_OVERLAY);
        }
        if (draw_context.advance_water_animation &&
            image_id >= draw_context.image_id_water_first &&
            image_id <= draw_context.image_id_water_last) {
            image_id++;
            if (image_id > draw_context.image_id_water_last) {
                image_id = draw_context.image_id_water_first;
            }
            map_image_set(grid_offset, image_id);
        }
        image_draw_isometric_footprint_from_draw_tile(image_id, x, y, color_mask);
    }
}

static void draw_top_without_overlay(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }
    struct building_t *b = &all_buildings[map_building_at(grid_offset)];
    int image_id = map_image_at(grid_offset);
    color_t color_mask = 0;
    if (draw_building_as_deleted_2(b) || (map_property_is_deleted(grid_offset) && !is_multi_tile_terrain(grid_offset))) {
        color_mask = COLOR_MASK_RED;
    }
    image_draw_isometric_top_from_draw_tile(image_id, x, y, color_mask);
    // specific buildings
    if (b->type == BUILDING_SENATE) {
        // rating flags
        image_id = image_group(GROUP_BUILDING_SENATE);
        image_draw_masked(image_id + 1, x + 138, y + 44 - city_data.ratings.culture / 2, color_mask);
        image_draw_masked(image_id + 2, x + 168, y + 36 - city_data.ratings.prosperity / 2, color_mask);
        image_draw_masked(image_id + 3, x + 198, y + 27 - city_data.ratings.peace / 2, color_mask);
        image_draw_masked(image_id + 4, x + 228, y + 19 - city_data.ratings.favor / 2, color_mask);
        // unemployed
        image_id = image_group(GROUP_FIGURE_HOMELESS);
        int unemployment_pct = city_data.labor.unemployment_percentage_for_senate;
        if (unemployment_pct > 0) {
            image_draw_masked(image_id + 108, x + 80, y, color_mask);
        }
        if (unemployment_pct > 5) {
            image_draw_masked(image_id + 104, x + 230, y - 30, color_mask);
        }
        if (unemployment_pct > 10) {
            image_draw_masked(image_id + 107, x + 100, y + 20, color_mask);
        }
        if (unemployment_pct > 15) {
            image_draw_masked(image_id + 106, x + 235, y - 10, color_mask);
        }
        if (unemployment_pct > 20) {
            image_draw_masked(image_id + 106, x + 66, y + 20, color_mask);
        }
    }
    if (b->type == BUILDING_AMPHITHEATER && b->num_workers > 0) {
        image_draw_masked(image_group(GROUP_BUILDING_AMPHITHEATER_SHOW), x + 36, y - 47, color_mask);
    }
    if (b->type == BUILDING_THEATER && b->num_workers > 0) {
        image_draw_masked(image_group(GROUP_BUILDING_THEATER_SHOW), x + 34, y - 22, color_mask);
    }
    if (b->type == BUILDING_COLOSSEUM && b->num_workers > 0) {
        image_draw_masked(image_group(GROUP_BUILDING_COLOSSEUM_SHOW), x + 70, y - 90, color_mask);
    }
    if (b->type == BUILDING_HIPPODROME && building_main(b)->num_workers > 0 && city_data.entertainment.hippodrome_has_race) {
        int subtype = b->subtype.orientation;
        int orientation = city_view_orientation();
        if ((subtype == 0 || subtype == 3) && city_data.population.population > 2000) {
            // first building part
            switch (orientation) {
                case DIR_0_TOP:
                    image_draw_masked(image_group(GROUP_BUILDING_HIPPODROME_2) + 6, x + 147, y - 72, color_mask);
                    break;
                case DIR_2_RIGHT:
                    image_draw_masked(image_group(GROUP_BUILDING_HIPPODROME_1) + 8, x + 58, y - 79, color_mask);
                    break;
                case DIR_4_BOTTOM:
                    image_draw_masked(image_group(GROUP_BUILDING_HIPPODROME_2) + 8, x + 119, y - 80, color_mask);
                    break;
                case DIR_6_LEFT:
                    image_draw_masked(image_group(GROUP_BUILDING_HIPPODROME_1) + 6, x, y - 72, color_mask);
            }
        } else if ((subtype == 1 || subtype == 4) && city_data.population.population > 100) {
            // middle building part
            switch (orientation) {
                case DIR_0_TOP:
                case DIR_4_BOTTOM:
                    image_draw_masked(image_group(GROUP_BUILDING_HIPPODROME_2) + 7, x + 122, y - 79, color_mask);
                    break;
                case DIR_2_RIGHT:
                case DIR_6_LEFT:
                    image_draw_masked(image_group(GROUP_BUILDING_HIPPODROME_1) + 7, x, y - 80, color_mask);
            }
        } else if ((subtype == 2 || subtype == 5) && city_data.population.population > 1000) {
            // last building part
            switch (orientation) {
                case DIR_0_TOP:
                    image_draw_masked(image_group(GROUP_BUILDING_HIPPODROME_2) + 8, x + 119, y - 80, color_mask);
                    break;
                case DIR_2_RIGHT:
                    image_draw_masked(image_group(GROUP_BUILDING_HIPPODROME_1) + 6, x, y - 72, color_mask);
                    break;
                case DIR_4_BOTTOM:
                    image_draw_masked(image_group(GROUP_BUILDING_HIPPODROME_2) + 6, x + 147, y - 72, color_mask);
                    break;
                case DIR_6_LEFT:
                    image_draw_masked(image_group(GROUP_BUILDING_HIPPODROME_1) + 8, x + 58, y - 79, color_mask);
                    break;
            }
        }
    }
    if (b->type == BUILDING_WINE_WORKSHOP) {
        if (b->loads_stored >= 2 || b->data.industry.has_raw_materials) {
            image_draw_masked(image_group(GROUP_BUILDING_WORKSHOP_RAW_MATERIAL), x + 45, y + 23, color_mask);
        }
    }
    if (b->type == BUILDING_OIL_WORKSHOP) {
        if (b->loads_stored >= 2 || b->data.industry.has_raw_materials) {
            image_draw_masked(image_group(GROUP_BUILDING_WORKSHOP_RAW_MATERIAL) + 1, x + 35, y + 15, color_mask);
        }
    }
    if (b->type == BUILDING_WEAPONS_WORKSHOP) {
        if (b->loads_stored >= 2 || b->data.industry.has_raw_materials) {
            image_draw_masked(image_group(GROUP_BUILDING_WORKSHOP_RAW_MATERIAL) + 3, x + 46, y + 24, color_mask);
        }
    }
    if (b->type == BUILDING_FURNITURE_WORKSHOP) {
        if (b->loads_stored >= 2 || b->data.industry.has_raw_materials) {
            image_draw_masked(image_group(GROUP_BUILDING_WORKSHOP_RAW_MATERIAL) + 2, x + 48, y + 19, color_mask);
        }
    }
    if (b->type == BUILDING_POTTERY_WORKSHOP) {
        if (b->loads_stored >= 2 || b->data.industry.has_raw_materials) {
            image_draw_masked(image_group(GROUP_BUILDING_WORKSHOP_RAW_MATERIAL) + 4, x + 47, y + 24, color_mask);
        }
    }
}

static void draw_figures_without_overlay(int x, int y, int grid_offset)
{
    int figure_id = map_figure_at(grid_offset);
    while (figure_id) {
        struct figure_t *f = &figures[figure_id];
        if (figure_id == draw_context.selected_figure_id) {
            if (!f->is_invisible || f->height_adjusted_ticks) {
                adjust_pixel_offset(f, &x, &y);
                draw_figure_city(f, x, y, 0);
                draw_context.selected_figure_coord->x = x;
                draw_context.selected_figure_coord->y = y;
            }
        } else if (!f->is_invisible) {
            if (figure_properties[f->type].is_player_legion_unit && f->formation_id == draw_context.highlighted_formation) {
                city_draw_figure(f, x, y, 1);
            } else {
                city_draw_figure(f, x, y, 0);
            }

        }
        figure_id = f->next_figure_id_on_same_tile;
    }
}

static void draw_animation_without_overlay(int x, int y, int grid_offset)
{
    int image_id = map_image_at(grid_offset);
    const struct image_t *img = image_get(image_id);
    if (img->num_animation_sprites) {
        if (map_property_is_draw_tile(grid_offset)) {
            int building_id = map_building_at(grid_offset);
            struct building_t *b = &all_buildings[building_id];
            int color_mask = 0;
            if (draw_building_as_deleted_2(b) || map_property_is_deleted(grid_offset)) {
                color_mask = COLOR_MASK_RED;
            }
            if (b->type == BUILDING_DOCK) {
                int num_idle = 0;
                for (int i = 0; i < 3; i++) {
                    if (b->data.dock.docker_ids[i]) {
                        struct figure_t *f = &figures[b->data.dock.docker_ids[i]];
                        if (f->action_state == FIGURE_ACTION_DOCKER_IDLING ||
                            f->action_state == FIGURE_ACTION_DOCKER_IMPORT_QUEUE) {
                            num_idle++;
                        }
                    }
                }
                if (num_idle > 0) {
                    int image_dock = map_image_at(b->grid_offset);
                    int image_dockers = image_group(GROUP_BUILDING_DOCK_DOCKERS);
                    if (image_dock == image_group(GROUP_BUILDING_DOCK_1)) {
                        image_dockers += 0;
                    } else if (image_dock == image_group(GROUP_BUILDING_DOCK_2)) {
                        image_dockers += 3;
                    } else if (image_dock == image_group(GROUP_BUILDING_DOCK_3)) {
                        image_dockers += 6;
                    } else {
                        image_dockers += 9;
                    }
                    if (num_idle == 2) {
                        image_dockers += 1;
                    } else if (num_idle == 3) {
                        image_dockers += 2;
                    }
                    const struct image_t *img2 = image_get(image_dockers);
                    image_draw_masked(image_dockers, x + img2->sprite_offset_x, y + img2->sprite_offset_y, color_mask);
                }
            } else if (b->type == BUILDING_WAREHOUSE) {
                image_draw_masked(image_group(GROUP_BUILDING_WAREHOUSE) + 17, x - 4, y - 42, color_mask);
                if (b->id == city_data.building.trade_center_building_id) {
                    image_draw_masked(image_group(GROUP_BUILDING_TRADE_CENTER_FLAG), x + 19, y - 56, color_mask);
                }
            } else if (b->type == BUILDING_GRANARY) {
                image_draw_masked(image_group(GROUP_BUILDING_GRANARY) + 1,
                                  x + img->sprite_offset_x,
                                  y + 60 + img->sprite_offset_y - img->height,
                                  color_mask);
                if (b->data.granary.resource_stored[RESOURCE_NONE] < 2400) {
                    image_draw_masked(image_group(GROUP_BUILDING_GRANARY) + 2, x + 33, y - 60, color_mask);
                }
                if (b->data.granary.resource_stored[RESOURCE_NONE] < 1800) {
                    image_draw_masked(image_group(GROUP_BUILDING_GRANARY) + 3, x + 56, y - 50, color_mask);
                }
                if (b->data.granary.resource_stored[RESOURCE_NONE] < 1200) {
                    image_draw_masked(image_group(GROUP_BUILDING_GRANARY) + 4, x + 91, y - 50, color_mask);
                }
                if (b->data.granary.resource_stored[RESOURCE_NONE] < 600) {
                    image_draw_masked(image_group(GROUP_BUILDING_GRANARY) + 5, x + 117, y - 62, color_mask);
                }
            } else if (b->type == BUILDING_BURNING_RUIN && b->ruin_has_plague) {
                image_draw_masked(image_group(GROUP_PLAGUE_SKULL), x + 18, y - 32, color_mask);
            }
            int animation_offset = building_animation_offset(b, image_id, grid_offset);
            if (b->type != BUILDING_HIPPODROME && animation_offset > 0) {
                if (animation_offset > img->num_animation_sprites) {
                    animation_offset = img->num_animation_sprites;
                }
                if (b->type == BUILDING_GRANARY) {
                    image_draw_masked(image_id + animation_offset + 5, x + 77, y - 49, color_mask);
                } else {
                    int ydiff = 0;
                    switch (map_property_multi_tile_size(grid_offset)) {
                        case 1: ydiff = 30; break;
                        case 2: ydiff = 45; break;
                        case 3: ydiff = 60; break;
                        case 4: ydiff = 75; break;
                        case 5: ydiff = 90; break;
                    }
                    image_draw_masked(image_id + animation_offset,
                                      x + img->sprite_offset_x,
                                      y + ydiff + img->sprite_offset_y - img->height,
                                      color_mask);
                }
            }
        }
    } else if (map_sprite_bridge_at(grid_offset)) {
        city_draw_bridge(x, y, grid_offset);
    } else if (building_is_fort(all_buildings[map_building_at(grid_offset)].type)) {
        if (map_property_is_draw_tile(grid_offset)) {
            struct building_t *fort = &all_buildings[map_building_at(grid_offset)];
            int offset = 0;
            switch (fort->subtype.fort_figure_type) {
                case FIGURE_FORT_LEGIONARY: offset = 4; break;
                case FIGURE_FORT_MOUNTED: offset = 3; break;
                case FIGURE_FORT_JAVELIN: offset = 2; break;
            }
            if (offset) {
                image_draw_masked(image_group(GROUP_BUILDING_FORT) + offset, x + 81, y + 5, draw_building_as_deleted_2(fort) ? COLOR_MASK_RED : 0);
            }
        }
    } else if (all_buildings[map_building_at(grid_offset)].type == BUILDING_GATEHOUSE) {
        int xy = map_property_multi_tile_xy(grid_offset);
        int orientation = city_view_orientation();
        if ((orientation == DIR_0_TOP && xy == EDGE_X1Y1) ||
            (orientation == DIR_2_RIGHT && xy == EDGE_X0Y1) ||
            (orientation == DIR_4_BOTTOM && xy == EDGE_X0Y0) ||
            (orientation == DIR_6_LEFT && xy == EDGE_X1Y0)) {
            struct building_t *gate = &all_buildings[map_building_at(grid_offset)];
            int gatehouse_image_id = image_group(GROUP_BUILDING_GATEHOUSE);
            int color_mask = draw_building_as_deleted_2(gate) ? COLOR_MASK_RED : 0;
            if (gate->subtype.orientation == 1) {
                if (orientation == DIR_0_TOP || orientation == DIR_4_BOTTOM) {
                    image_draw_masked(gatehouse_image_id, x - 22, y - 80, color_mask);
                } else {
                    image_draw_masked(gatehouse_image_id + 1, x - 18, y - 81, color_mask);
                }
            } else if (gate->subtype.orientation == 2) {
                if (orientation == DIR_0_TOP || orientation == DIR_4_BOTTOM) {
                    image_draw_masked(gatehouse_image_id + 1, x - 18, y - 81, color_mask);
                } else {
                    image_draw_masked(gatehouse_image_id, x - 22, y - 80, color_mask);
                }
            }
        }
    }
}

static void draw_elevated_figures_without_overlay(int x, int y, int grid_offset)
{
    int figure_id = map_figure_at(grid_offset);
    while (figure_id > 0) {
        struct figure_t *f = &figures[figure_id];
        if ((f->use_cross_country && !f->is_invisible) || f->height_adjusted_ticks) {
            city_draw_figure(f, x, y, 0);
        }
        figure_id = f->next_figure_id_on_same_tile;
    }
}

static void draw_hippodrome_ornaments(int x, int y, int grid_offset)
{
    int image_id = map_image_at(grid_offset);
    const struct image_t *img = image_get(image_id);
    struct building_t *b = &all_buildings[map_building_at(grid_offset)];
    if (img->num_animation_sprites
        && map_property_is_draw_tile(grid_offset)
        && b->type == BUILDING_HIPPODROME) {
        image_draw_masked(image_id + 1,
            x + img->sprite_offset_x, y + img->sprite_offset_y - img->height + 90,
            draw_building_as_deleted_2(b) ? COLOR_MASK_RED : 0
        );
    }
}

static int should_draw_top_before_deletion_without_overlay(int grid_offset)
{
    int has_adjacent_deletion = 0;
    int size = map_property_multi_tile_size(grid_offset);
    int total_adjacent_offsets = size * 2 + 1;
    const int *adjacent_offset = ADJACENT_OFFSETS[size - 1][city_view_orientation() / 2];
    for (int i = 0; i < total_adjacent_offsets; ++i) {
        if (map_property_is_deleted(grid_offset + adjacent_offset[i]) ||
            draw_building_as_deleted_2(&all_buildings[map_building_at(grid_offset + adjacent_offset[i])])) {
            has_adjacent_deletion = 1;
            break;
        }
    }
    return is_multi_tile_terrain(grid_offset) && has_adjacent_deletion;
}

static void deletion_draw_terrain_top_without_overlay(int x, int y, int grid_offset)
{
    if (map_property_is_draw_tile(grid_offset) && should_draw_top_before_deletion_without_overlay(grid_offset)) {
        draw_top_without_overlay(x, y, grid_offset);
    }
}

static void deletion_draw_figures_animations(int x, int y, int grid_offset)
{
    if (map_property_is_deleted(grid_offset) || draw_building_as_deleted_2(&all_buildings[map_building_at(grid_offset)])) {
        image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
    }
    if (map_property_is_draw_tile(grid_offset) && !should_draw_top_before_deletion_without_overlay(grid_offset)) {
        draw_top_without_overlay(x, y, grid_offset);
    }
    draw_figures_without_overlay(x, y, grid_offset);
    draw_animation_without_overlay(x, y, grid_offset);
}

static void deletion_draw_remaining(int x, int y, int grid_offset)
{
    draw_elevated_figures_without_overlay(x, y, grid_offset);
    draw_hippodrome_ornaments(x, y, grid_offset);
}

void city_without_overlay_draw(int selected_figure_id, struct pixel_coordinate_t *figure_coord, const struct map_tile_t *tile)
{
    int highlighted_formation = -1;
    if (config_get(CONFIG_UI_HIGHLIGHT_LEGIONS)) {
        highlighted_formation = formation_legion_at_grid_offset(tile->grid_offset);
        if (highlighted_formation > -1) {
            if (selected_legion_formation > -1 && highlighted_formation != selected_legion_formation) {
                highlighted_formation = -1;
            }
            if (legion_formations[highlighted_formation].in_distant_battle) {
                highlighted_formation = -1;
            }
        }
    }
    draw_context.advance_water_animation = 0;
    if (!selected_figure_id) {
        uint32_t now = time_get_millis();
        if (now - draw_context.last_water_animation_time > 60) {
            draw_context.last_water_animation_time = now;
            draw_context.advance_water_animation = 1;
        }
    }
    draw_context.image_id_water_first = image_group(GROUP_TERRAIN_WATER);
    draw_context.image_id_water_last = 5 + draw_context.image_id_water_first;
    draw_context.selected_figure_id = selected_figure_id;
    draw_context.selected_figure_coord = figure_coord;
    draw_context.highlighted_formation = highlighted_formation;
    int should_mark_deleting = city_building_ghost_mark_deleting(tile);
    city_view_foreach_map_tile(draw_footprint_without_overlay);
    if (!should_mark_deleting) {
        city_view_foreach_valid_map_tile(
            draw_top_without_overlay,
            draw_figures_without_overlay,
            draw_animation_without_overlay
        );
        if (!selected_figure_id) {
            city_building_ghost_draw(tile);
        }
        city_view_foreach_valid_map_tile(
            draw_elevated_figures_without_overlay,
            draw_hippodrome_ornaments,
            0
        );
    } else {
        city_view_foreach_map_tile(deletion_draw_terrain_top_without_overlay);
        city_view_foreach_map_tile(deletion_draw_figures_animations);
        city_view_foreach_map_tile(deletion_draw_remaining);
    }
}

void widget_city_draw(void)
{
    set_city_clip_rectangle();
    if (game_state_overlay() && select_city_overlay()) {
        int should_mark_deleting = city_building_ghost_mark_deleting(&widget_city_data.current_tile);
        city_view_foreach_map_tile(draw_footprint_with_overlay);
        if (!should_mark_deleting) {
            city_view_foreach_valid_map_tile(
                draw_figures_with_overlay,
                draw_top_with_overlay,
                draw_animation_with_overlay
            );
            city_building_ghost_draw(&widget_city_data.current_tile);
            city_view_foreach_map_tile(draw_elevated_figures_with_overlay);
        } else {
            city_view_foreach_map_tile(draw_figures_with_overlay);
            city_view_foreach_map_tile(deletion_draw_terrain_top_with_overlay);
            city_view_foreach_map_tile(deletion_draw_animations);
            city_view_foreach_map_tile(draw_elevated_figures_with_overlay);
        }
    } else {
        city_without_overlay_draw(0, 0, &widget_city_data.current_tile);
    }
    graphics_reset_clip_rectangle();
}

void widget_city_draw_for_figure(int figure_id, struct pixel_coordinate_t *coord)
{
    set_city_clip_rectangle();
    city_without_overlay_draw(figure_id, coord, &widget_city_data.current_tile);
    graphics_reset_clip_rectangle();
}

void update_city_view_coords(int x, int y, struct map_tile_t *tile)
{
    struct pixel_view_coordinates_t view;
    if (city_view_pixels_to_view_tile(x, y, &view)) {
        tile->grid_offset = city_view_tile_to_grid_offset(&view);
        city_view_set_selected_view_tile(&view);
        tile->x = map_grid_offset_to_x(tile->grid_offset);
        tile->y = map_grid_offset_to_y(tile->grid_offset);
    } else {
        tile->grid_offset = tile->x = tile->y = 0;
    }
}

void scroll_map(const struct mouse_t *m)
{
    struct pixel_view_coordinates_t delta;
    if (scroll_get_delta(m, &delta, SCROLL_TYPE_CITY)) {
        city_view_scroll(delta.x, delta.y);
        decay_city_sounds_views();
    }
}

void input_box_start(struct input_box_t *box)
{
    int text_width = (box->width_blocks - 2) * BLOCK_SIZE;
    keyboard_start_capture(box->text, box->text_length, box->allow_punctuation, text_width, box->font);
    SDL_Rect rect = { box->x, box->y, box->width_blocks * BLOCK_SIZE, box->height_blocks * BLOCK_SIZE };
    SDL_SetTextInputRect(&rect);
    draw_cursor_input_box = 1;
}

void input_box_stop(__attribute__((unused)) struct input_box_t *box)
{
    keyboard_stop_capture();
    SDL_Rect rect = { 0, 0, 0, 0 };
    SDL_SetTextInputRect(&rect);
    draw_cursor_input_box = 0;
}

void input_box_draw(const struct input_box_t *box)
{
    inner_panel_draw(box->x, box->y, box->width_blocks, box->height_blocks);
    int text_x = box->x + 16;
    int text_y = box->y + 10;
    // text_capture_cursor and text_draw_cursor must be before and after text_draw respectively...
    if (draw_cursor_input_box) {
        text_capture_cursor(keyboard_cursor_position(), keyboard_offset_start(), keyboard_offset_end());
    }
    text_draw(box->text, text_x, text_y, box->font, 0);
    if (draw_cursor_input_box) {
        text_draw_cursor(text_x, text_y + 1);
    }
}

void widget_minimap_invalidate(void)
{
    minimap_data.refresh_requested = 1;
}

static void set_bounds_minimap(int x_offset, int y_offset, int width, int height)
{
    minimap_data.width_tiles = width / 2;
    minimap_data.height_tiles = height;
    minimap_data.x_offset = x_offset;
    minimap_data.y_offset = y_offset;
    minimap_data.width = width;
    minimap_data.height = height;
    minimap_data.absolute_x = (VIEW_X_MAX - minimap_data.width_tiles) / 2;
    minimap_data.absolute_y = (VIEW_Y_MAX - minimap_data.height_tiles) / 2;

    city_view_get_camera(&minimap_data.camera_x, &minimap_data.camera_y);
    int view_width_tiles, view_height_tiles;
    city_view_get_viewport_size_tiles(&view_width_tiles, &view_height_tiles);

    if ((map_data.width - minimap_data.width_tiles) / 2 > 0) {
        if (minimap_data.camera_x < minimap_data.absolute_x) {
            minimap_data.absolute_x = minimap_data.camera_x;
        } else if (minimap_data.camera_x > minimap_data.width_tiles + minimap_data.absolute_x - view_width_tiles) {
            minimap_data.absolute_x = view_width_tiles + minimap_data.camera_x - minimap_data.width_tiles;
        }
    }
    if ((2 * map_data.height - minimap_data.height_tiles) / 2 > 0) {
        if (minimap_data.camera_y < minimap_data.absolute_y) {
            minimap_data.absolute_y = minimap_data.camera_y;
        } else if (minimap_data.camera_y > minimap_data.height_tiles + minimap_data.absolute_y - view_height_tiles) {
            minimap_data.absolute_y = view_height_tiles + minimap_data.camera_y - minimap_data.height_tiles;
        }
    }
    // ensure even height
    minimap_data.absolute_y &= ~1;
}

static void draw_minimap_tile(int x_view, int y_view, int grid_offset)
{
    if (grid_offset < 0) {
        image_draw(image_group(GROUP_MINIMAP_BLACK), x_view, y_view);
        return;
    }
    int color_type = FIGURE_COLOR_NONE;
    if (map_figures.items[grid_offset] > 0) {
        int figure_id = map_figures.items[grid_offset];
        while (figure_id) {
            struct figure_t *f = &figures[figure_id];
            if (figure_properties[f->type].is_player_legion_unit) {
                color_type = selected_legion_formation == f->formation_id ? FIGURE_COLOR_SELECTED_SOLDIER : FIGURE_COLOR_SOLDIER;
                break;
            } else if (figure_properties[f->type].is_enemy_unit || figure_properties[f->type].is_caesar_legion_unit || (figure_properties[f->type].is_native_unit && f->action_state == FIGURE_ACTION_NATIVE_ATTACKING)) {
                color_type = FIGURE_COLOR_ENEMY;
                break;
            } else if (f->type == FIGURE_WOLF) {
                color_type = FIGURE_COLOR_WOLF;
                break;
            }
            figure_id = f->next_figure_id_on_same_tile;
        }
    }
    if (color_type) {
        color_t color = COLOR_MINIMAP_WOLF;
        if (color_type == FIGURE_COLOR_SOLDIER) {
            color = COLOR_MINIMAP_SOLDIER;
        } else if (color_type == FIGURE_COLOR_SELECTED_SOLDIER) {
            color = COLOR_MINIMAP_SELECTED_SOLDIER;
        } else if (color_type == FIGURE_COLOR_ENEMY) {
            color = minimap_data.enemy_color;
        }
        graphics_draw_horizontal_line(x_view, x_view + 1, y_view, color);
        return;
    }
    int terrain = terrain_grid.items[grid_offset];
    // exception for fort ground: display as empty land
    if (terrain & TERRAIN_BUILDING) {
        if (all_buildings[map_building_at(grid_offset)].type == BUILDING_FORT_GROUND) {
            terrain = 0;
        }
    }
    if (terrain & TERRAIN_BUILDING) {
        if (map_property_is_draw_tile(grid_offset)) {
            int image_id;
            struct building_t *b = &all_buildings[map_building_at(grid_offset)];
            if (b->house_size) {
                image_id = image_group(GROUP_MINIMAP_HOUSE);
            } else if (b->type == BUILDING_RESERVOIR) {
                image_id = image_group(GROUP_MINIMAP_AQUEDUCT) - 1;
            } else {
                image_id = image_group(GROUP_MINIMAP_BUILDING);
            }
            switch (map_property_multi_tile_size(grid_offset)) {
                case 1: image_draw(image_id, x_view, y_view); break;
                case 2: image_draw(image_id + 1, x_view, y_view - 1); break;
                case 3: image_draw(image_id + 2, x_view, y_view - 2); break;
                case 4: image_draw(image_id + 3, x_view, y_view - 3); break;
                case 5: image_draw(image_id + 4, x_view, y_view - 4); break;
            }
        }
    } else {
        int rand = map_random_get(grid_offset);
        int image_id;
        if (terrain & TERRAIN_ROAD) {
            image_id = image_group(GROUP_MINIMAP_ROAD);
        } else if (terrain & TERRAIN_WATER) {
            image_id = image_group(GROUP_MINIMAP_WATER) + (rand & 3);
        } else if (terrain & (TERRAIN_TREE | TERRAIN_SHRUB)) {
            image_id = image_group(GROUP_MINIMAP_TREE) + (rand & 3);
        } else if (terrain & (TERRAIN_ROCK | TERRAIN_ELEVATION)) {
            image_id = image_group(GROUP_MINIMAP_ROCK) + (rand & 3);
        } else if (terrain & TERRAIN_AQUEDUCT) {
            image_id = image_group(GROUP_MINIMAP_AQUEDUCT);
        } else if (terrain & TERRAIN_WALL) {
            image_id = image_group(GROUP_MINIMAP_WALL);
        } else if (terrain & TERRAIN_MEADOW) {
            image_id = image_group(GROUP_MINIMAP_MEADOW) + (rand & 3);
        } else {
            image_id = image_group(GROUP_MINIMAP_EMPTY_LAND) + (rand & 7);
        }
        image_draw(image_id, x_view, y_view);
    }
}

static void draw_viewport_rectangle(void)
{
    int camera_x, camera_y;
    int camera_pixels_x, camera_pixels_y;
    city_view_get_camera(&camera_x, &camera_y);
    city_view_get_pixel_offset(&camera_pixels_x, &camera_pixels_y);
    int view_width_tiles, view_height_tiles;
    city_view_get_viewport_size_tiles(&view_width_tiles, &view_height_tiles);

    int x_offset = minimap_data.x_offset + 2 * (camera_x - minimap_data.absolute_x) - 2 + camera_pixels_x / 30;
    if (x_offset < minimap_data.x_offset) {
        x_offset = minimap_data.x_offset;
    }
    if (x_offset + 2 * view_width_tiles + 4 > minimap_data.x_offset + minimap_data.width_tiles) {
        x_offset -= 2;
    }
    int y_offset = minimap_data.y_offset + camera_y - minimap_data.absolute_y + 2;
    graphics_draw_rect(x_offset, y_offset,
        view_width_tiles * 2 + 4,
        view_height_tiles - 4,
        COLOR_MINIMAP_VIEWPORT);
}

static void draw_minimap(void)
{
    graphics_set_clip_rectangle(minimap_data.x_offset, minimap_data.y_offset, minimap_data.width, minimap_data.height);
    city_view_foreach_minimap_tile(minimap_data.x_offset, minimap_data.y_offset,
                               minimap_data.absolute_x, minimap_data.absolute_y,
                               minimap_data.width_tiles, minimap_data.height_tiles,
                               draw_minimap_tile);
    graphics_save_to_buffer(minimap_data.x_offset, minimap_data.y_offset, minimap_data.width, minimap_data.height, minimap_data.cache);
    draw_viewport_rectangle();
    graphics_reset_clip_rectangle();
}

static void draw_uncached(int x_offset, int y_offset, int width, int height)
{
    minimap_data.enemy_color = ENEMY_COLOR_BY_CLIMATE[scenario.climate];
    if (width != minimap_data.width || height != minimap_data.height) {
        free(minimap_data.cache);
        minimap_data.cache = (color_t *) malloc(sizeof(color_t) * width * height);
    }
    set_bounds_minimap(x_offset, y_offset, width, height);
    draw_minimap();
}

void widget_minimap_draw(int x_offset, int y_offset, int width, int height, int force)
{
    int refresh_type = REFRESH_NOT_NEEDED;
    if (minimap_data.refresh_requested || force) {
        minimap_data.refresh_requested = 0;
        refresh_type = REFRESH_FULL;
    } else {
        int new_x, new_y;
        city_view_get_camera(&new_x, &new_y);
        if (minimap_data.camera_x != new_x || minimap_data.camera_y != new_y) {
            refresh_type = REFRESH_CAMERA_MOVED;
        }
    }
    if (refresh_type) {
        if (refresh_type == REFRESH_FULL) {
            draw_uncached(x_offset, y_offset, width, height);
        } else {
            if (width != minimap_data.width || height != minimap_data.height || x_offset != minimap_data.x_offset) {
                draw_uncached(x_offset, y_offset, width, height);
            } else {
                int old_absolute_x = minimap_data.absolute_x;
                int old_absolute_y = minimap_data.absolute_y;
                set_bounds_minimap(x_offset, y_offset, width, height);
                if (minimap_data.absolute_x != old_absolute_x || minimap_data.absolute_y != old_absolute_y) {
                    draw_minimap();
                } else {
                    graphics_set_clip_rectangle(x_offset, y_offset, width, height);
                    graphics_draw_from_buffer(x_offset, y_offset, minimap_data.width, minimap_data.height, minimap_data.cache);
                    draw_viewport_rectangle();
                    graphics_reset_clip_rectangle();
                }
            }
        }
        graphics_draw_horizontal_line(x_offset - 1, x_offset - 1 + width, y_offset - 1, COLOR_MINIMAP_DARK);
        graphics_draw_vertical_line(x_offset - 1, y_offset, y_offset + height, COLOR_MINIMAP_DARK);
        graphics_draw_vertical_line(x_offset - 1 + width, y_offset,
            y_offset + height, COLOR_MINIMAP_LIGHT);
    }
}

static void update_mouse_grid_offset(int x_view, int y_view, int grid_offset)
{
    if (minimap_data.mouse.y == y_view && (minimap_data.mouse.x == x_view || minimap_data.mouse.x == x_view + 1)) {
        minimap_data.mouse.grid_offset = grid_offset < 0 ? 0 : grid_offset;
    }
}

int widget_minimap_handle_mouse(const struct mouse_t *m)
{
    if ((m->left.went_down || m->right.went_down)
    && (m->x >= minimap_data.x_offset && m->x < minimap_data.x_offset + minimap_data.width && // in minimap
        m->y >= minimap_data.y_offset && m->y < minimap_data.y_offset + minimap_data.height)) {
        minimap_data.mouse.x = m->x;
        minimap_data.mouse.y = m->y;
        minimap_data.mouse.grid_offset = 0;
        city_view_foreach_minimap_tile(minimap_data.x_offset, minimap_data.y_offset,
                                       minimap_data.absolute_x, minimap_data.absolute_y,
                                       minimap_data.width_tiles, minimap_data.height_tiles,
                                       update_mouse_grid_offset);
        int grid_offset = minimap_data.mouse.grid_offset;
        if (grid_offset > 0) {
            city_view_go_to_grid_offset(grid_offset);
            return 1;
        }
    }
    return 0;
}
