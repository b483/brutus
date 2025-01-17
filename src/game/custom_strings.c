#include "custom_strings.h"

#include "core/encoding.h"
#include "core/log.h"
#include "core/string.h"

#include <string.h>

#define URL_PATCHES "https://github.com/bvschaik/julius/wiki/Patches"
#define URL_EDITOR "https://github.com/bvschaik/julius/wiki/Editor"
#define BUFFER_SIZE 100000

static struct {
    uint8_t *strings[TRANSLATION_MAX_KEY];
    uint8_t buffer[BUFFER_SIZE];
    int buf_index;
} data;

static custom_string all_strings[] = {
    {TR_NO_PATCH_TITLE, "Patch 1.0.1.0 not installed"},
    {TR_NO_PATCH_MESSAGE,
        "Your Caesar 3 installation does not have the 1.0.1.0 patch installed. "
        "You can download the patch from:\n"
        URL_PATCHES "\n"
        "Continue at your own risk."},
    {TR_NO_EDITOR_TITLE, "Editor not installed"},
    {TR_NO_EDITOR_MESSAGE,
        "Your Caesar 3 installation does not contain the editor files. "
        "You can download them from:\n"
        URL_EDITOR},
    {TR_INVALID_LANGUAGE_TITLE, "Invalid language directory"},
    {TR_INVALID_LANGUAGE_MESSAGE,
        "The directory you selected does not contain a valid language pack. "
        "Please check the log for errors."},
    {TR_BUILD_ALL_TEMPLES, "All"},
    {TR_BUTTON_OK, "OK"},
    {TR_BUTTON_CANCEL, "Cancel"},
    {TR_BUTTON_RESET_DEFAULTS, "Reset defaults"},
    {TR_BUTTON_CONFIGURE_HOTKEYS, "Configure hotkeys"},
    {TR_CONFIG_TITLE, "Brutus configuration options"},
    {TR_CONFIG_PLAYER_NAME_LABEL, "Player name:"},
    {TR_CONFIG_LANGUAGE_LABEL, "Language:"},
    {TR_CONFIG_LANGUAGE_DEFAULT, "(default)"},
    {TR_CONFIG_DISPLAY_SCALE, "Display scale:"},
    {TR_CONFIG_CURSOR_SCALE, "Cursor scale:"},
    {TR_CONFIG_HEADER_UI_CHANGES, "User interface changes"},
    {TR_CONFIG_SHOW_INTRO_VIDEO, "Play intro videos"},
    {TR_CONFIG_SIDEBAR_INFO, "Extra information in the control panel"},
    {TR_CONFIG_SMOOTH_SCROLLING, "Enable smooth scrolling"},
    {TR_CONFIG_DISABLE_MOUSE_EDGE_SCROLLING, "Disable map scrolling on window edge"},
    {TR_CONFIG_DISABLE_RIGHT_CLICK_MAP_DRAG, "Disable right click to drag the map"},
    {TR_CONFIG_VISUAL_FEEDBACK_ON_DELETE, "Improve visual feedback when clearing land"},
    {TR_CONFIG_ALLOW_CYCLING_TEMPLES, "Allow building each temple in succession"},
    {TR_CONFIG_SHOW_WATER_STRUCTURE_RANGE, "Show range when building reservoirs, fountains and wells"},
    {TR_CONFIG_SHOW_CONSTRUCTION_SIZE, "Show draggable construction size"},
    {TR_CONFIG_HIGHLIGHT_LEGIONS, "Highlight legion on cursor hover"},
    {TR_CONFIG_SHOW_MILITARY_SIDEBAR, "Enable military sidebar"},
    {TR_HOTKEY_TITLE, "Brutus hotkey configuration"},
    {TR_HOTKEY_LABEL, "Hotkey"},
    {TR_HOTKEY_ALTERNATIVE_LABEL, "Alternative"},
    {TR_HOTKEY_HEADER_ARROWS, "Arrow keys"},
    {TR_HOTKEY_HEADER_GLOBAL, "Global hotkeys"},
    {TR_HOTKEY_HEADER_CITY, "City hotkeys"},
    {TR_HOTKEY_HEADER_OVERLAYS, "Overlays"},
    {TR_HOTKEY_HEADER_BOOKMARKS, "City map bookmarks"},
    {TR_HOTKEY_HEADER_EDITOR, "Editor"},
    {TR_HOTKEY_HEADER_CHEATS, "Cheats"},
    {TR_HOTKEY_HEADER_BUILD, "Construction hotkeys"},
    {TR_HOTKEY_ARROW_UP, "Up"},
    {TR_HOTKEY_ARROW_DOWN, "Down"},
    {TR_HOTKEY_ARROW_LEFT, "Left"},
    {TR_HOTKEY_ARROW_RIGHT, "Right"},
    {TR_HOTKEY_TOGGLE_FULLSCREEN, "Toggle fullscreen"},
    {TR_HOTKEY_RESET_WINDOW, "Reset window"},
    {TR_HOTKEY_SAVE_SCREENSHOT, "Save screenshot"},
    {TR_HOTKEY_SAVE_CITY_SCREENSHOT, "Save full city screenshot"},
    {TR_HOTKEY_BUILD_CLONE, "Clone building under cursor"},
    {TR_HOTKEY_LOAD_FILE, "Load file"},
    {TR_HOTKEY_SAVE_FILE, "Save file"},
    {TR_HOTKEY_DECREASE_GAME_SPEED, "Decrease game speed"},
    {TR_HOTKEY_INCREASE_GAME_SPEED, "Increase game speed"},
    {TR_HOTKEY_TOGGLE_PAUSE, "Toggle pause"},
    {TR_HOTKEY_ROTATE_MAP_LEFT, "Rotate map left"},
    {TR_HOTKEY_ROTATE_MAP_RIGHT, "Rotate map right"},
    {TR_HOTKEY_REPLAY_MAP, "Replay map"},
    {TR_HOTKEY_CYCLE_LEGION, "Cycle through legions"},
    {TR_HOTKEY_RETURN_LEGIONS_TO_FORT, "Return legions to fort"},
    {TR_HOTKEY_SHOW_LAST_ADVISOR, "Show last advisor"},
    {TR_HOTKEY_SHOW_EMPIRE_MAP, "Show empire map"},
    {TR_HOTKEY_SHOW_MESSAGES, "Show messages"},
    {TR_HOTKEY_SHOW_OVERLAY_WATER, "Show water overlay"},
    {TR_HOTKEY_SHOW_OVERLAY_FIRE, "Show fire overlay"},
    {TR_HOTKEY_SHOW_OVERLAY_DAMAGE, "Damage overlay"},
    {TR_HOTKEY_SHOW_OVERLAY_CRIME, "Crime overlay"},
    {TR_HOTKEY_SHOW_OVERLAY_PROBLEMS, "Problems overlay"},
    {TR_HOTKEY_GO_TO_BOOKMARK_1, "Go to bookmark 1"},
    {TR_HOTKEY_GO_TO_BOOKMARK_2, "Go to bookmark 2"},
    {TR_HOTKEY_GO_TO_BOOKMARK_3, "Go to bookmark 3"},
    {TR_HOTKEY_GO_TO_BOOKMARK_4, "Go to bookmark 4"},
    {TR_HOTKEY_SET_BOOKMARK_1, "Set bookmark 1"},
    {TR_HOTKEY_SET_BOOKMARK_2, "Set bookmark 2"},
    {TR_HOTKEY_SET_BOOKMARK_3, "Set bookmark 3"},
    {TR_HOTKEY_SET_BOOKMARK_4, "Set bookmark 4"},
    {TR_HOTKEY_EDITOR_TOGGLE_BATTLE_INFO, "Toggle battle info"},
    {TR_HOTKEY_CHEAT_MONEY, "Cheat: money"},
    {TR_HOTKEY_CHEAT_INVASION, "Cheat: invasion"},
    {TR_HOTKEY_CHEAT_VICTORY, "Cheat: victory"},
    {TR_HOTKEY_EDIT_TITLE, "Press new hotkey"},
    {TR_HOTKEY_DUPLICATE_TITLE, "Hotkey already used"},
    {TR_HOTKEY_DUPLICATE_MESSAGE, "This key combination is already assigned to the following action:"},
    {TR_WARNING_SCREENSHOT_SAVED, "Screenshot saved: "},
    {TR_ALLOWED_BUILDING_HOUSE_VACANT_LOT, "Housing"},
    {TR_ALLOWED_BUILDING_CLEAR_LAND, "Clear land"},
    {TR_ALLOWED_BUILDING_ROAD, "Road"},
    {TR_ALLOWED_BUILDING_DRAGGABLE_RESERVOIR, "Reservoir"},
    {TR_ALLOWED_BUILDING_AQUEDUCT, "Aqueduct"},
    {TR_ALLOWED_BUILDING_FOUNTAIN, "Fountain"},
    {TR_ALLOWED_BUILDING_WELL, "Well"},
    {TR_ALLOWED_BUILDING_BARBER, "Barber"},
    {TR_ALLOWED_BUILDING_BATHHOUSE, "Bathhouse"},
    {TR_ALLOWED_BUILDING_DOCTOR, "Doctor"},
    {TR_ALLOWED_BUILDING_HOSPITAL, "Hospital"},
    {TR_ALLOWED_BUILDING_SMALL_TEMPLE_CERES, "Small temple Ceres"},
    {TR_ALLOWED_BUILDING_SMALL_TEMPLE_NEPTUNE, "Small temple Neptune"},
    {TR_ALLOWED_BUILDING_SMALL_TEMPLE_MERCURY, "Small temple Mercury"},
    {TR_ALLOWED_BUILDING_SMALL_TEMPLE_MARS, "Small temple Mars"},
    {TR_ALLOWED_BUILDING_SMALL_TEMPLE_VENUS, "Small temple Venus"},
    {TR_ALLOWED_BUILDING_LARGE_TEMPLE_CERES, "Large temple Ceres"},
    {TR_ALLOWED_BUILDING_LARGE_TEMPLE_NEPTUNE, "Large temple Neptune"},
    {TR_ALLOWED_BUILDING_LARGE_TEMPLE_MERCURY, "Large temple Mercury"},
    {TR_ALLOWED_BUILDING_LARGE_TEMPLE_MARS, "Large temple Mars"},
    {TR_ALLOWED_BUILDING_LARGE_TEMPLE_VENUS, "Large temple Venus"},
    {TR_ALLOWED_BUILDING_ORACLE, "Oracle"},
    {TR_ALLOWED_BUILDING_SCHOOL, "School"},
    {TR_ALLOWED_BUILDING_ACADEMY, "Academy"},
    {TR_ALLOWED_BUILDING_LIBRARY, "Library"},
    {TR_ALLOWED_BUILDING_MISSION_POST, "Mission post"},
    {TR_ALLOWED_BUILDING_THEATER, "Theater"},
    {TR_ALLOWED_BUILDING_AMPHITHEATER, "Amphitheater"},
    {TR_ALLOWED_BUILDING_COLOSSEUM, "Colosseum"},
    {TR_ALLOWED_BUILDING_HIPPODROME, "Hippodrome"},
    {TR_ALLOWED_BUILDING_GLADIATOR_SCHOOL, "Gladiator school"},
    {TR_ALLOWED_BUILDING_LION_HOUSE, "Lion house"},
    {TR_ALLOWED_BUILDING_ACTOR_COLONY, "Actor colony"},
    {TR_ALLOWED_BUILDING_CHARIOT_MAKER, "Chariot maker"},
    {TR_ALLOWED_BUILDING_FORUM, "Forum"},
    {TR_ALLOWED_BUILDING_SENATE_UPGRADED, "Senate"},
    {TR_ALLOWED_BUILDING_GOVERNORS_HOUSE, "Governor's house"},
    {TR_ALLOWED_BUILDING_GOVERNORS_VILLA, "Governor's villa"},
    {TR_ALLOWED_BUILDING_GOVERNORS_PALACE, "Governor's palace"},
    {TR_ALLOWED_BUILDING_SMALL_STATUE, "Small statue"},
    {TR_ALLOWED_BUILDING_MEDIUM_STATUE, "Medium statue"},
    {TR_ALLOWED_BUILDING_LARGE_STATUE, "Large statue"},
    {TR_ALLOWED_BUILDING_TRIUMPHAL_ARCH, "Triumphal arch"},
    {TR_ALLOWED_BUILDING_GARDENS, "Gardens"},
    {TR_ALLOWED_BUILDING_PLAZA, "Plaza"},
    {TR_ALLOWED_BUILDING_ENGINEERS_POST, "Engineers post"},
    {TR_ALLOWED_BUILDING_LOW_BRIDGE, "Low bridge"},
    {TR_ALLOWED_BUILDING_SHIP_BRIDGE, "Ship bridge"},
    {TR_ALLOWED_BUILDING_SHIPYARD, "Shipyard"},
    {TR_ALLOWED_BUILDING_WHARF, "Wharf"},
    {TR_ALLOWED_BUILDING_DOCK, "Dock"},
    {TR_ALLOWED_BUILDING_WALL, "Wall"},
    {TR_ALLOWED_BUILDING_TOWER, "Tower"},
    {TR_ALLOWED_BUILDING_GATEHOUSE, "Gatehouse"},
    {TR_ALLOWED_BUILDING_PREFECTURE, "Prefecture"},
    {TR_ALLOWED_BUILDING_FORT_LEGIONARIES, "Fort legionaries"},
    {TR_ALLOWED_BUILDING_FORT_JAVELIN, "Fort javelin"},
    {TR_ALLOWED_BUILDING_FORT_MOUNTED, "Fort mounted"},
    {TR_ALLOWED_BUILDING_MILITARY_ACADEMY, "Military academy"},
    {TR_ALLOWED_BUILDING_BARRACKS, "Barracks"},
    {TR_ALLOWED_BUILDING_WHEAT_FARM, "Wheat farm"},
    {TR_ALLOWED_BUILDING_VEGETABLE_FARM, "Vegetable farm"},
    {TR_ALLOWED_BUILDING_FRUIT_FARM, "Fruit farm"},
    {TR_ALLOWED_BUILDING_OLIVE_FARM, "Olive farm"},
    {TR_ALLOWED_BUILDING_VINES_FARM, "Vines farm"},
    {TR_ALLOWED_BUILDING_PIG_FARM, "Pig farm"},
    {TR_ALLOWED_BUILDING_CLAY_PIT, "Clay pit"},
    {TR_ALLOWED_BUILDING_MARBLE_QUARRY, "Marble quarry"},
    {TR_ALLOWED_BUILDING_IRON_MINE, "Iron mine"},
    {TR_ALLOWED_BUILDING_TIMBER_YARD, "Timber yard"},
    {TR_ALLOWED_BUILDING_WINE_WORKSHOP, "Wine workshop"},
    {TR_ALLOWED_BUILDING_OIL_WORKSHOP, "Oil workshop"},
    {TR_ALLOWED_BUILDING_WEAPONS_WORKSHOP, "Weapons workshop"},
    {TR_ALLOWED_BUILDING_FURNITURE_WORKSHOP, "Furniture workshop"},
    {TR_ALLOWED_BUILDING_POTTERY_WORKSHOP, "Pottery workshop"},
    {TR_ALLOWED_BUILDING_MARKET, "Market"},
    {TR_ALLOWED_BUILDING_GRANARY, "Granary"},
    {TR_ALLOWED_BUILDING_WAREHOUSE, "Warehouse"},
    {TR_CITY_MESSAGE_TITLE_DISTANT_BATTLE_WON_TRIUMPHAL_ARCH_DISABLED, "Roman city saved"},
    {TR_CITY_MESSAGE_TEXT_DISTANT_BATTLE_WON_TRIUMPHAL_ARCH_DISABLED, "Your relief force defeated the invading barbarians. Caesar is pleased."},
    {TR_EDITOR_SIDEBAR_BUTTON_GRASS_TOOLTIP, "Grass"},
    {TR_EDITOR_SIDEBAR_BUTTON_SHRUB_TOOLTIP, "Shrub"},
    {TR_EDITOR_SIDEBAR_BUTTON_WATER_TOOLTIP, "Water"},
    {TR_EDITOR_SIDEBAR_BUTTON_RAISE_LAND_TOOLTIP, "Raise land"},
    {TR_EDITOR_SIDEBAR_BUTTON_LOWER_LAND_TOOLTIP, "Lower land"},
    {TR_EDITOR_SIDEBAR_BUTTON_ACCESS_RAMP_TOOLTIP, "Access ramp"},
    {TR_EDITOR_SIDEBAR_BUTTON_TREES_TOOLTIP, "Trees"},
    {TR_EDITOR_SIDEBAR_BUTTON_ROCKS_TOOLTIP, "Rocks"},
    {TR_EDITOR_SIDEBAR_BUTTON_MEADOW_TOOLTIP, "Meadow"},
    {TR_EDITOR_SIDEBAR_BUTTON_ROAD_TOOLTIP, "Road"},
    {TR_EDITOR_SIDEBAR_BUTTON_EARTHQUAKE_POINT_TOOLTIP, "Earthquake"},
    {TR_EDITOR_SIDEBAR_BUTTON_INVASION_POINT_TOOLTIP, "Invasion point"},
    {TR_EDITOR_SIDEBAR_BUTTON_ENTRY_POINT_TOOLTIP, "Entry point"},
    {TR_EDITOR_SIDEBAR_BUTTON_EXIT_POINT_TOOLTIP, "Exit point"},
    {TR_EDITOR_SIDEBAR_BUTTON_RIVER_ENTRY_POINT_TOOLTIP, "River IN"},
    {TR_EDITOR_SIDEBAR_BUTTON_RIVER_EXIT_POINT_TOOLTIP, "River OUT"},
    {TR_EDITOR_SIDEBAR_BUTTON_NATIVE_HUT_TOOLTIP, "Native hut"},
    {TR_EDITOR_SIDEBAR_BUTTON_NATIVE_CENTER_TOOLTIP, "Native center"},
    {TR_EDITOR_SIDEBAR_BUTTON_NATIVE_FIELD_TOOLTIP, "Native field"},
    {TR_EDITOR_SIDEBAR_BUTTON_FISHING_POINT_TOOLTIP, "Fishing waters"},
    {TR_EDITOR_SIDEBAR_BUTTON_HERD_POINT_TOOLTIP, "Herd point"},
    {TR_COST_OPEN_TRADE_ROUTE, "Cost to open trade route: "},
    {TR_TRADE_CITY_TYPE, "A trade city"},
    {TR_FUTURE_TRADE_CITY_TYPE, "A future trade city"},
    {TR_EMPIRE_EXPANSION_YEAR, "Year offset for empire expansion: "},
    {TR_EDITOR_MAP_BRIEFING, "Map briefing"},
    {TR_EDITOR_MAP_BRIEFING_HINT, "Hint: 'atL' for new line, 'atP' for new paragraph (replace 'at' with the symbol - will be hidden)"},
    {TR_EDITOR_MAP_BRIEFING_RESET, "Reset briefing"},
    {TR_EDITOR_INITIAL_FAVOR, "Initial favor"},
    {TR_EDITOR_INITIAL_PERSONAL_SAVINGS, "Initial personal savings"},
    {TR_EDITOR_HERD_POINT_1, "Herd point 1"},
    {TR_EDITOR_HERD_POINT_2, "Herd point 2"},
    {TR_EDITOR_HERD_POINT_3, "Herd point 3"},
    {TR_EDITOR_HERD_POINT_4, "Herd point 4"},
    {TR_EDITOR_HERD_POINT_5, "Herd point 5"},
    {TR_EDITOR_HERD_POINT_6, "Herd point 6"},
    {TR_EDITOR_HERD_POINT_7, "Herd point 7"},
    {TR_EDITOR_HERD_POINT_8, "Herd point 8"},
    {TR_EDITOR_OFFSET_YEAR, "Year offset:"},
    {TR_EDITOR_MONTH, "Month:"},
    {TR_EDITOR_INVALID_YEAR_MONTH, "Jan year 0 invalid"},
    {TR_EDITOR_AMOUNT, "Amount:"},
    {TR_EDITOR_INVASION_TYPE, "Type:"},
    {TR_EDITOR_INVASION_FROM, "From:"},
    {TR_EDITOR_INVASION_ATTACK_TYPE, "Attack type:"},
    {TR_EDITOR_INVASION_TYPE_NO_INVADERS, "No invaders"},
    {TR_EDITOR_INVASION_TYPE_LOCAL_RAIDERS, "Local raiders"},
    {TR_EDITOR_INVASION_TYPE_ENEMY_ARMY, "Enemy army"},
    {TR_EDITOR_INVASION_TYPE_CAESAR, "Caesar's legions"},
    {TR_EDITOR_INVASION_TYPE_DISTANT_BATTLE, "Distant battle"},
    {TR_EDITOR_INVASION_SCHEDULED, "Invasions scheduled"},
    {TR_EDITOR_REQUEST_FROM_EMPEROR, "Request from the Emperor"},
    {TR_EDITOR_RESOURCE, "Resource:"},
    {TR_EDITOR_YEARS_DEADLINE, "Years deadline:"},
    {TR_EDITOR_FAVOR_GRANTED, "Favor granted:"},
    {TR_EDITOR_REQUEST_SCHEDULED, "Requests scheduled"},
    {TR_EDITOR_NO_PRICE_CHANGE, "No price changes"},
    {TR_EDITOR_PRICE_CHANGE_SCHEDULED, "Price changes scheduled"},
    {TR_EDITOR_NO_DEMAND_CHANGE, "No demand changes"},
    {TR_EDITOR_DEMAND_CHANGE_SCHEDULED, "Demand changes scheduled"}
};

static void set_strings(const custom_string *strings, int num_strings)
{
    for (int i = 0; i < num_strings; i++) {
        const custom_string *string = &strings[i];
        if (data.strings[string->key]) {
            continue;
        }
        int length_left = BUFFER_SIZE - data.buf_index;
        encoding_from_utf8(string->string, &data.buffer[data.buf_index], length_left);
        data.strings[string->key] = &data.buffer[data.buf_index];
        data.buf_index += 1 + string_length(&data.buffer[data.buf_index]);
    }
}

void custom_strings_load(void)
{
    const custom_string *strings = all_strings;
    int num_strings = sizeof(all_strings) / sizeof(custom_string);

    memset(data.strings, 0, sizeof(data.strings));
    data.buf_index = 0;
    set_strings(strings, num_strings);
}

uint8_t *get_custom_string(custom_string_key key)
{
    return data.strings[key];
}
