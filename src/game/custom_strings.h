#include <stdint.h>

typedef enum {
    TR_NO_PATCH_TITLE,
    TR_NO_PATCH_MESSAGE,
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
    TR_CONFIG_SHOW_INTRO_VIDEO,
    TR_CONFIG_SIDEBAR_INFO,
    TR_CONFIG_DISABLE_MOUSE_EDGE_SCROLLING,
    TR_CONFIG_DISABLE_RIGHT_CLICK_MAP_DRAG,
    TR_CONFIG_VISUAL_FEEDBACK_ON_DELETE,
    TR_CONFIG_ALLOW_CYCLING_TEMPLES,
    TR_CONFIG_HIGHLIGHT_LEGIONS,
    TR_CONFIG_SHOW_MILITARY_SIDEBAR,
    TR_HOTKEY_TITLE,
    TR_HOTKEY_LABEL,
    TR_HOTKEY_ALTERNATIVE_LABEL,
    TR_HOTKEY_HEADER_ARROWS,
    TR_HOTKEY_HEADER_GLOBAL,
    TR_HOTKEY_HEADER_CITY,
    TR_HOTKEY_HEADER_OVERLAYS,
    TR_HOTKEY_HEADER_BOOKMARKS,
    TR_HOTKEY_HEADER_EDITOR,
    TR_HOTKEY_HEADER_CHEATS,
    TR_HOTKEY_HEADER_BUILD,
    TR_HOTKEY_ARROW_UP,
    TR_HOTKEY_ARROW_DOWN,
    TR_HOTKEY_ARROW_LEFT,
    TR_HOTKEY_ARROW_RIGHT,
    TR_HOTKEY_TOGGLE_FULLSCREEN,
    TR_HOTKEY_RESET_WINDOW,
    TR_HOTKEY_SAVE_SCREENSHOT,
    TR_HOTKEY_SAVE_CITY_SCREENSHOT,
    TR_HOTKEY_CYCLE_BUILDINGS,
    TR_HOTKEY_CYCLE_BUILDINGS_REVERSE,
    TR_HOTKEY_BUILD_CLONE,
    TR_HOTKEY_LOAD_FILE,
    TR_HOTKEY_SAVE_FILE,
    TR_HOTKEY_DECREASE_GAME_SPEED,
    TR_HOTKEY_INCREASE_GAME_SPEED,
    TR_HOTKEY_TOGGLE_PAUSE,
    TR_HOTKEY_ROTATE_MAP_LEFT,
    TR_HOTKEY_ROTATE_MAP_RIGHT,
    TR_HOTKEY_REPLAY_MAP,
    TR_HOTKEY_CYCLE_LEGION,
    TR_HOTKEY_RETURN_LEGIONS_TO_FORT,
    TR_HOTKEY_SHOW_LAST_ADVISOR,
    TR_HOTKEY_SHOW_EMPIRE_MAP,
    TR_HOTKEY_SHOW_MESSAGES,
    TR_HOTKEY_GO_TO_PROBLEM,
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
    TR_HOTKEY_CHEAT_MONEY,
    TR_HOTKEY_CHEAT_INVASION,
    TR_HOTKEY_CHEAT_VICTORY,
    TR_HOTKEY_EDIT_TITLE,
    TR_HOTKEY_DUPLICATE_TITLE,
    TR_HOTKEY_DUPLICATE_MESSAGE,
    TR_WARNING_SCREENSHOT_SAVED,
    TR_ALLOWED_BUILDING_HOUSE_VACANT_LOT,
    TR_ALLOWED_BUILDING_CLEAR_TERRAIN,
    TR_ALLOWED_BUILDING_ROAD,
    TR_ALLOWED_BUILDING_RESERVOIR,
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
    TR_EDITOR_SCENARIO_BRIEFING,
    TR_EDITOR_SCENARIO_RICH_TEXT_HINT,
    TR_EDITOR_SCENARIO_BRIEFING_RESET,
    TR_EDITOR_INITIAL_FAVOR,
    TR_EDITOR_INITIAL_PERSONAL_SAVINGS,
    TR_EDITOR_TOOL_SUBMENU_RAISE_LAND,
    TR_EDITOR_TOOL_SUBMENU_LOWER_LAND,
    TR_EDITOR_TOOL_SUBMENU_ACCESS_RAMP,
    TR_EDITOR_TOOL_SUBMENU_TINY_BRUSH,
    TR_EDITOR_TOOL_SUBMENU_SMALL_BRUSH,
    TR_EDITOR_TOOL_SUBMENU_MEDIUM_BRUSH,
    TR_EDITOR_TOOL_SUBMENU_BIG_BRUSH,
    TR_EDITOR_TOOL_SUBMENU_BIGGEST_BRUSH,
    TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_1,
    TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_2,
    TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_3,
    TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_4,
    TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_5,
    TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_6,
    TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_7,
    TR_EDITOR_TOOL_SUBMENU_INVASION_POINT_8,
    TR_EDITOR_TOOL_SUBMENU_ENTRY_POINT,
    TR_EDITOR_TOOL_SUBMENU_EXIT_POINT,
    TR_EDITOR_TOOL_SUBMENU_RIVER_ENTRY,
    TR_EDITOR_TOOL_SUBMENU_RIVER_EXIT,
    TR_EDITOR_TOOL_SUBMENU_NATIVE_HUT,
    TR_EDITOR_TOOL_SUBMENU_NATIVE_CENTER,
    TR_EDITOR_TOOL_SUBMENU_NATIVE_FIELD,
    TR_EDITOR_TOOL_SUBMENU_VACANT_LOT,
    TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_1,
    TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_2,
    TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_3,
    TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_4,
    TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_5,
    TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_6,
    TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_7,
    TR_EDITOR_TOOL_SUBMENU_FISHING_POINT_8,
    TR_EDITOR_TOOL_SUBMENU_HERD_POINT_1,
    TR_EDITOR_TOOL_SUBMENU_HERD_POINT_2,
    TR_EDITOR_TOOL_SUBMENU_HERD_POINT_3,
    TR_EDITOR_TOOL_SUBMENU_HERD_POINT_4,
    TR_EDITOR_TOOL_SUBMENU_HERD_POINT_5,
    TR_EDITOR_TOOL_SUBMENU_HERD_POINT_6,
    TR_EDITOR_TOOL_SUBMENU_HERD_POINT_7,
    TR_EDITOR_TOOL_SUBMENU_HERD_POINT_8,
    TR_EDITOR_OFFSET_YEAR,
    TR_EDITOR_MONTH,
    TR_EDITOR_INVALID_YEAR_MONTH,
    TR_EDITOR_AMOUNT,
    TR_EDITOR_INVASION_TYPE,
    TR_EDITOR_INVASION_FROM,
    TR_EDITOR_INVASION_TARGET_TYPE,
    TR_EDITOR_INVASION_TYPE_NO_INVADERS,
    TR_EDITOR_INVASION_TYPE_LOCAL_RAIDERS,
    TR_EDITOR_INVASION_TYPE_ENEMY_ARMY,
    TR_EDITOR_INVASION_TYPE_CAESAR,
    TR_EDITOR_INVASION_TYPE_DISTANT_BATTLE,
    TR_EDITOR_ENEMY_TYPE_BARBARIANS,
    TR_EDITOR_ENEMY_TYPE_CARTHAGINIANS,
    TR_EDITOR_ENEMY_TYPE_BRITONS,
    TR_EDITOR_ENEMY_TYPE_CELTS,
    TR_EDITOR_ENEMY_TYPE_PICTS,
    TR_EDITOR_ENEMY_TYPE_EGYPTIANS,
    TR_EDITOR_ENEMY_TYPE_ETRUSCANS,
    TR_EDITOR_ENEMY_TYPE_SAMNITES,
    TR_EDITOR_ENEMY_TYPE_GAULS,
    TR_EDITOR_ENEMY_TYPE_HELVETII,
    TR_EDITOR_ENEMY_TYPE_GOTHS,
    TR_EDITOR_ENEMY_TYPE_HUNS,
    TR_EDITOR_ENEMY_TYPE_VISIGOTHS,
    TR_EDITOR_ENEMY_TYPE_GRAECI,
    TR_EDITOR_ENEMY_TYPE_MACEDONIANS,
    TR_EDITOR_ENEMY_TYPE_NUMIDIANS,
    TR_EDITOR_ENEMY_TYPE_PERGAMUM,
    TR_EDITOR_ENEMY_TYPE_IBERIANS,
    TR_EDITOR_ENEMY_TYPE_JUDAEANS,
    TR_EDITOR_ENEMY_TYPE_SELEUCIDS,
    TR_EDITOR_INVASION_SCHEDULED,
    TR_EDITOR_REQUEST_FROM_EMPEROR,
    TR_EDITOR_RESOURCE,
    TR_EDITOR_YEARS_DEADLINE,
    TR_EDITOR_FAVOR_GRANTED,
    TR_EDITOR_REQUEST_SCHEDULED,
    TR_EDITOR_CUSTOM_MESSAGE_ATTRIBUTES,
    TR_EDITOR_CUSTOM_MESSAGE_URGENT,
    TR_EDITOR_CUSTOM_MESSAGE_ENABLED,
    TR_EDITOR_CUSTOM_MESSAGE_VIDEO_FILE,
    TR_EDITOR_CUSTOM_MESSAGE_VIDEO_FILE_HINT,
    TR_EDITOR_CUSTOM_MESSAGE_RESET,
    TR_EDITOR_CUSTOM_MESSAGE_TITLE,
    TR_EDITOR_CUSTOM_MESSAGE_RESET_TITLE,
    TR_EDITOR_CUSTOM_MESSAGE_TEXT,
    TR_EDITOR_CUSTOM_MESSAGE_RESET_TEXT,
    TR_EDITOR_NO_CUSTOM_MESSAGE,
    TR_EDITOR_CUSTOM_MESSAGE_SCHEDULED,
    TR_EDITOR_CUSTOM_MESSAGE_HEADER,
    TR_EDITOR_NO_PRICE_CHANGE,
    TR_EDITOR_PRICE_CHANGE_SCHEDULED,
    TR_EDITOR_NO_DEMAND_CHANGE,
    TR_EDITOR_DEMAND_CHANGE_SCHEDULED,
    TR_BRIEFING_FIRED_AFTER,
    TR_BRIEFING_SURVIVE_FOR,
    TR_TOO_MANY_FILES,
    TR_GAME_PAUSED,
    TR_ENEMY_TYPE_BARBARIAN,
    TR_ENEMY_TYPE_CARTHAGINIAN,
    TR_ENEMY_TYPE_BRITON,
    TR_ENEMY_TYPE_CELT,
    TR_ENEMY_TYPE_PICT,
    TR_ENEMY_TYPE_EGYPTIAN,
    TR_ENEMY_TYPE_ETRUSCAN,
    TR_ENEMY_TYPE_SAMNITE,
    TR_ENEMY_TYPE_GAUL,
    TR_ENEMY_TYPE_HELVETIUS,
    TR_ENEMY_TYPE_GOTH,
    TR_ENEMY_TYPE_HUN,
    TR_ENEMY_TYPE_VISIGOTH,
    TR_ENEMY_TYPE_GRAECUS,
    TR_ENEMY_TYPE_MACEDONIAN,
    TR_ENEMY_TYPE_NUMIDIAN,
    TR_ENEMY_TYPE_PERGAMUM,
    TR_ENEMY_TYPE_IBERIAN,
    TR_ENEMY_TYPE_JUDAEAN,
    TR_ENEMY_TYPE_SELEUCID,
    TR_ENEMY_TYPE_LEGION,
    TR_FIGURE_DESC_NOBODY,
    TR_FIGURE_DESC_IMMIGRANT,
    TR_FIGURE_DESC_EMIGRANT,
    TR_FIGURE_DESC_HOMELESS,
    TR_FIGURE_DESC_PATRICIAN,
    TR_FIGURE_DESC_CART_PUSHER,
    TR_FIGURE_DESC_CITIZEN,
    TR_FIGURE_DESC_BARBER,
    TR_FIGURE_DESC_BATHS_WORKER,
    TR_FIGURE_DESC_DOCTOR,
    TR_FIGURE_DESC_SURGEON,
    TR_FIGURE_DESC_PRIEST,
    TR_FIGURE_DESC_SCHOOL_CHILD,
    TR_FIGURE_DESC_TEACHER,
    TR_FIGURE_DESC_LIBRARIAN,
    TR_FIGURE_DESC_MISSIONARY,
    TR_FIGURE_DESC_ACTOR,
    TR_FIGURE_DESC_GLADIATOR,
    TR_FIGURE_DESC_LION_TAMER,
    TR_FIGURE_DESC_CHARIOTEER,
    TR_FIGURE_HIPPODROME_HORSE,
    TR_FIGURE_DESC_TAX_COLLECTOR,
    TR_FIGURE_DESC_ENGINEER,
    TR_FIGURE_DESC_FISHING_BOAT,
    TR_FIGURE_DESC_SEAGULLS,
    TR_FIGURE_DESC_SHIPWRECK,
    TR_FIGURE_DESC_DOCKER,
    TR_FIGURE_DESC_FLOTSAM,
    TR_FIGURE_DESC_BALLISTA,
    TR_FIGURE_DESC_BOLT,
    TR_FIGURE_DESC_SENTRY,
    TR_FIGURE_DESC_JAVELIN,
    TR_FIGURE_DESC_PREFECT,
    TR_FIGURE_DESC_STANDARD_BEARER,
    TR_FIGURE_DESC_JAVELIN_THROWER,
    TR_FIGURE_DESC_MOUNTED_AUXILIARY,
    TR_FIGURE_DESC_LEGIONARY,
    TR_FIGURE_DESC_MARKET_BUYER,
    TR_FIGURE_DESC_MARKET_TRADER,
    TR_FIGURE_DESC_DELIVERY_BOY,
    TR_FIGURE_DESC_WAREHOUSEMAN,
    TR_FIGURE_DESC_PROTESTOR,
    TR_FIGURE_DESC_CRIMINAL,
    TR_FIGURE_DESC_RIOTER,
    TR_FIGURE_DESC_CARAVAN_OF_MERCHANTS_FROM,
    TR_FIGURE_DESC_MERCHANT_DONKEY_FROM,
    TR_FIGURE_DESC_TRADE_SHIP_FROM,
    TR_FIGURE_DESC_INDIGENOUS_NATIVE,
    TR_FIGURE_DESC_NATIVE_TRADER,
    TR_FIGURE_DESC_WOLF,
    TR_FIGURE_DESC_SHEEP,
    TR_FIGURE_DESC_ZEBRA,
    TR_FIGURE_DESC_ENEMY,
    TR_FIGURE_DESC_ARROW,
    TR_FIGURE_DESC_MAP_FLAG,
    TR_FIGURE_DESC_EXPLOSION,
    TR_CLIMATE_NORTHERN,
    TR_CLIMATE_CENTRAL,
    TR_CLIMATE_DESERT,
    CUSTOM_STRINGS_MAX_KEY
} custom_string_key;

typedef struct {
    custom_string_key key;
    const char *string;
} custom_string;

void custom_strings_load(void);

uint8_t *get_custom_string(custom_string_key key);