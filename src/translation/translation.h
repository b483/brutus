#ifndef TRANSLATION_TRANSLATION_H
#define TRANSLATION_TRANSLATION_H

#include "core/locale.h"

#include <stdint.h>

typedef enum {
    TR_NO_PATCH_TITLE,
    TR_NO_PATCH_MESSAGE,
    TR_MISSING_FONTS_TITLE,
    TR_MISSING_FONTS_MESSAGE,
    TR_NO_EDITOR_TITLE,
    TR_NO_EDITOR_MESSAGE,
    TR_INVALID_LANGUAGE_TITLE,
    TR_INVALID_LANGUAGE_MESSAGE,
    TR_BUILD_ALL_TEMPLES,
    TR_BUTTON_OK,
    TR_BUTTON_CANCEL,
    TR_BUTTON_RESET_DEFAULTS,
    TR_BUTTON_CONFIGURE_HOTKEYS,
    TR_CONFIG_TITLE,
    TR_CONFIG_PLAYER_NAME_LABEL,
    TR_CONFIG_LANGUAGE_LABEL,
    TR_CONFIG_LANGUAGE_DEFAULT,
    TR_CONFIG_DISPLAY_SCALE,
    TR_CONFIG_CURSOR_SCALE,
    TR_CONFIG_HEADER_UI_CHANGES,
    TR_CONFIG_HEADER_GAMEPLAY_CHANGES,
    TR_CONFIG_SHOW_INTRO_VIDEO,
    TR_CONFIG_SIDEBAR_INFO,
    TR_CONFIG_SMOOTH_SCROLLING,
    TR_CONFIG_DISABLE_MOUSE_EDGE_SCROLLING,
    TR_CONFIG_DISABLE_RIGHT_CLICK_MAP_DRAG,
    TR_CONFIG_VISUAL_FEEDBACK_ON_DELETE,
    TR_CONFIG_ALLOW_CYCLING_TEMPLES,
    TR_CONFIG_SHOW_WATER_STRUCTURE_RANGE,
    TR_CONFIG_SHOW_CONSTRUCTION_SIZE,
    TR_CONFIG_HIGHLIGHT_LEGIONS,
    TR_CONFIG_SHOW_MILITARY_SIDEBAR,
    TR_CONFIG_FIX_IMMIGRATION_BUG,
    TR_CONFIG_FIX_100_YEAR_GHOSTS,
    TR_HOTKEY_TITLE,
    TR_HOTKEY_LABEL,
    TR_HOTKEY_ALTERNATIVE_LABEL,
    TR_HOTKEY_HEADER_ARROWS,
    TR_HOTKEY_HEADER_GLOBAL,
    TR_HOTKEY_HEADER_CITY,
    TR_HOTKEY_HEADER_ADVISORS,
    TR_HOTKEY_HEADER_OVERLAYS,
    TR_HOTKEY_HEADER_BOOKMARKS,
    TR_HOTKEY_HEADER_EDITOR,
    TR_HOTKEY_HEADER_BUILD,
    TR_HOTKEY_ARROW_UP,
    TR_HOTKEY_ARROW_DOWN,
    TR_HOTKEY_ARROW_LEFT,
    TR_HOTKEY_ARROW_RIGHT,
    TR_HOTKEY_TOGGLE_FULLSCREEN,
    TR_HOTKEY_CENTER_WINDOW,
    TR_HOTKEY_RESIZE_TO_640,
    TR_HOTKEY_RESIZE_TO_800,
    TR_HOTKEY_RESIZE_TO_1024,
    TR_HOTKEY_SAVE_SCREENSHOT,
    TR_HOTKEY_SAVE_CITY_SCREENSHOT,
    TR_HOTKEY_BUILD_CLONE,
    TR_HOTKEY_LOAD_FILE,
    TR_HOTKEY_SAVE_FILE,
    TR_HOTKEY_INCREASE_GAME_SPEED,
    TR_HOTKEY_DECREASE_GAME_SPEED,
    TR_HOTKEY_TOGGLE_PAUSE,
    TR_HOTKEY_CYCLE_LEGION,
    TR_HOTKEY_ROTATE_MAP_LEFT,
    TR_HOTKEY_ROTATE_MAP_RIGHT,
    TR_HOTKEY_SHOW_ADVISOR_LABOR,
    TR_HOTKEY_SHOW_ADVISOR_MILITARY,
    TR_HOTKEY_SHOW_ADVISOR_IMPERIAL,
    TR_HOTKEY_SHOW_ADVISOR_RATINGS,
    TR_HOTKEY_SHOW_ADVISOR_TRADE,
    TR_HOTKEY_SHOW_ADVISOR_POPULATION,
    TR_HOTKEY_SHOW_ADVISOR_HEALTH,
    TR_HOTKEY_SHOW_ADVISOR_EDUCATION,
    TR_HOTKEY_SHOW_ADVISOR_ENTERTAINMENT,
    TR_HOTKEY_SHOW_ADVISOR_RELIGION,
    TR_HOTKEY_SHOW_ADVISOR_FINANCIAL,
    TR_HOTKEY_SHOW_ADVISOR_CHIEF,
    TR_HOTKEY_TOGGLE_OVERLAY,
    TR_HOTKEY_SHOW_OVERLAY_WATER,
    TR_HOTKEY_SHOW_OVERLAY_FIRE,
    TR_HOTKEY_SHOW_OVERLAY_DAMAGE,
    TR_HOTKEY_SHOW_OVERLAY_CRIME,
    TR_HOTKEY_SHOW_OVERLAY_PROBLEMS,
    TR_HOTKEY_GO_TO_BOOKMARK_1,
    TR_HOTKEY_GO_TO_BOOKMARK_2,
    TR_HOTKEY_GO_TO_BOOKMARK_3,
    TR_HOTKEY_GO_TO_BOOKMARK_4,
    TR_HOTKEY_SET_BOOKMARK_1,
    TR_HOTKEY_SET_BOOKMARK_2,
    TR_HOTKEY_SET_BOOKMARK_3,
    TR_HOTKEY_SET_BOOKMARK_4,
    TR_HOTKEY_EDITOR_TOGGLE_BATTLE_INFO,
    TR_HOTKEY_EDIT_TITLE,
    TR_HOTKEY_DUPLICATE_TITLE,
    TR_HOTKEY_DUPLICATE_MESSAGE,
    TR_WARNING_SCREENSHOT_SAVED,
    TR_ALLOWED_BUILDING_HOUSE_VACANT_LOT,
    TR_ALLOWED_BUILDING_CLEAR_LAND,
    TR_ALLOWED_BUILDING_ROAD,
    TR_ALLOWED_BUILDING_DRAGGABLE_RESERVOIR,
    TR_ALLOWED_BUILDING_AQUEDUCT,
    TR_ALLOWED_BUILDING_FOUNTAIN,
    TR_ALLOWED_BUILDING_WELL,
    TR_ALLOWED_BUILDING_BARBER,
    TR_ALLOWED_BUILDING_BATHHOUSE,
    TR_ALLOWED_BUILDING_DOCTOR,
    TR_ALLOWED_BUILDING_HOSPITAL,
    TR_ALLOWED_BUILDING_SMALL_TEMPLE_CERES,
    TR_ALLOWED_BUILDING_SMALL_TEMPLE_NEPTUNE,
    TR_ALLOWED_BUILDING_SMALL_TEMPLE_MERCURY,
    TR_ALLOWED_BUILDING_SMALL_TEMPLE_MARS,
    TR_ALLOWED_BUILDING_SMALL_TEMPLE_VENUS,
    TR_ALLOWED_BUILDING_LARGE_TEMPLE_CERES,
    TR_ALLOWED_BUILDING_LARGE_TEMPLE_NEPTUNE,
    TR_ALLOWED_BUILDING_LARGE_TEMPLE_MERCURY,
    TR_ALLOWED_BUILDING_LARGE_TEMPLE_MARS,
    TR_ALLOWED_BUILDING_LARGE_TEMPLE_VENUS,
    TR_ALLOWED_BUILDING_ORACLE,
    TR_ALLOWED_BUILDING_SCHOOL,
    TR_ALLOWED_BUILDING_ACADEMY,
    TR_ALLOWED_BUILDING_LIBRARY,
    TR_ALLOWED_BUILDING_MISSION_POST,
    TR_ALLOWED_BUILDING_THEATER,
    TR_ALLOWED_BUILDING_AMPHITHEATER,
    TR_ALLOWED_BUILDING_COLOSSEUM,
    TR_ALLOWED_BUILDING_HIPPODROME,
    TR_ALLOWED_BUILDING_GLADIATOR_SCHOOL,
    TR_ALLOWED_BUILDING_LION_HOUSE,
    TR_ALLOWED_BUILDING_ACTOR_COLONY,
    TR_ALLOWED_BUILDING_CHARIOT_MAKER,
    TR_ALLOWED_BUILDING_FORUM,
    TR_ALLOWED_BUILDING_SENATE_UPGRADED,
    TR_ALLOWED_BUILDING_GOVERNORS_HOUSE,
    TR_ALLOWED_BUILDING_GOVERNORS_VILLA,
    TR_ALLOWED_BUILDING_GOVERNORS_PALACE,
    TR_ALLOWED_BUILDING_SMALL_STATUE,
    TR_ALLOWED_BUILDING_MEDIUM_STATUE,
    TR_ALLOWED_BUILDING_LARGE_STATUE,
    TR_ALLOWED_BUILDING_TRIUMPHAL_ARCH,
    TR_ALLOWED_BUILDING_GARDENS,
    TR_ALLOWED_BUILDING_PLAZA,
    TR_ALLOWED_BUILDING_ENGINEERS_POST,
    TR_ALLOWED_BUILDING_LOW_BRIDGE,
    TR_ALLOWED_BUILDING_SHIP_BRIDGE,
    TR_ALLOWED_BUILDING_SHIPYARD,
    TR_ALLOWED_BUILDING_WHARF,
    TR_ALLOWED_BUILDING_DOCK,
    TR_ALLOWED_BUILDING_WALL,
    TR_ALLOWED_BUILDING_TOWER,
    TR_ALLOWED_BUILDING_GATEHOUSE,
    TR_ALLOWED_BUILDING_PREFECTURE,
    TR_ALLOWED_BUILDING_FORT_LEGIONARIES,
    TR_ALLOWED_BUILDING_FORT_JAVELIN,
    TR_ALLOWED_BUILDING_FORT_MOUNTED,
    TR_ALLOWED_BUILDING_MILITARY_ACADEMY,
    TR_ALLOWED_BUILDING_BARRACKS,
    TR_ALLOWED_BUILDING_WHEAT_FARM,
    TR_ALLOWED_BUILDING_VEGETABLE_FARM,
    TR_ALLOWED_BUILDING_FRUIT_FARM,
    TR_ALLOWED_BUILDING_OLIVE_FARM,
    TR_ALLOWED_BUILDING_VINES_FARM,
    TR_ALLOWED_BUILDING_PIG_FARM,
    TR_ALLOWED_BUILDING_CLAY_PIT,
    TR_ALLOWED_BUILDING_MARBLE_QUARRY,
    TR_ALLOWED_BUILDING_IRON_MINE,
    TR_ALLOWED_BUILDING_TIMBER_YARD,
    TR_ALLOWED_BUILDING_WINE_WORKSHOP,
    TR_ALLOWED_BUILDING_OIL_WORKSHOP,
    TR_ALLOWED_BUILDING_WEAPONS_WORKSHOP,
    TR_ALLOWED_BUILDING_FURNITURE_WORKSHOP,
    TR_ALLOWED_BUILDING_POTTERY_WORKSHOP,
    TR_ALLOWED_BUILDING_MARKET,
    TR_ALLOWED_BUILDING_GRANARY,
    TR_ALLOWED_BUILDING_WAREHOUSE,
    TR_CITY_MESSAGE_TITLE_DISTANT_BATTLE_WON_TRIUMPHAL_ARCH_DISABLED,
    TR_CITY_MESSAGE_TEXT_DISTANT_BATTLE_WON_TRIUMPHAL_ARCH_DISABLED,
    TR_EDITOR_SIDEBAR_BUTTON_GRASS_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_SHRUB_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_WATER_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_RAISE_LAND_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_LOWER_LAND_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_ACCESS_RAMP_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_TREES_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_ROCKS_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_MEADOW_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_ROAD_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_EARTHQUAKE_POINT_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_INVASION_POINT_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_ENTRY_POINT_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_EXIT_POINT_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_RIVER_ENTRY_POINT_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_RIVER_EXIT_POINT_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_NATIVE_HUT_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_NATIVE_CENTER_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_NATIVE_FIELD_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_FISHING_POINT_TOOLTIP,
    TR_EDITOR_SIDEBAR_BUTTON_HERD_POINT_TOOLTIP,
    TR_COST_OPEN_TRADE_ROUTE,
    TR_TRADE_CITY_TYPE,
    TR_FUTURE_TRADE_CITY_TYPE,
    TR_EMPIRE_EXPANSION_YEAR,
    TR_EDITOR_INITIAL_FAVOR,
    TR_EDITOR_INITIAL_PERSONAL_SAVINGS,
    TRANSLATION_MAX_KEY
} translation_key;

typedef struct {
    translation_key key;
    const char *string;
} translation_string;

void translation_load(language_type language);

uint8_t *translation_for(translation_key key);

void translation_english(const translation_string **strings, int *num_strings);
void translation_french(const translation_string **strings, int *num_strings);
void translation_german(const translation_string **strings, int *num_strings);
void translation_italian(const translation_string **strings, int *num_strings);
void translation_japanese(const translation_string **strings, int *num_strings);
void translation_korean(const translation_string **strings, int *num_strings);
void translation_polish(const translation_string **strings, int *num_strings);
void translation_portuguese(const translation_string **strings, int *num_strings);
void translation_russian(const translation_string **strings, int *num_strings);
void translation_spanish(const translation_string **strings, int *num_strings);
void translation_swedish(const translation_string **strings, int *num_strings);
void translation_simplified_chinese(const translation_string **strings, int *num_strings);
void translation_traditional_chinese(const translation_string **strings, int *num_strings);

#endif // TRANSLATION_TRANSLATION_H
