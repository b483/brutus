#include "translation/common.h"
#include "translation/translation.h"

static translation_string all_strings[] = {
    {TR_NO_PATCH_TITLE, "Patch 1.0.1.0 not installed"},
    {TR_NO_PATCH_MESSAGE,
        "Your Caesar 3 installation does not have the 1.0.1.0 patch installed. "
        "You can download the patch from:\n"
        URL_PATCHES "\n"
        "Continue at your own risk."},
    {TR_MISSING_FONTS_TITLE, "Missing fonts"},
    {TR_MISSING_FONTS_MESSAGE,
        "Your Caesar 3 installation requires extra font files. "
        "You can download them for your language from:\n"
        URL_PATCHES},
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
    {TR_CONFIG_TITLE, "Julius configuration options"},
    {TR_CONFIG_LANGUAGE_LABEL, "Language:"},
    {TR_CONFIG_LANGUAGE_DEFAULT, "(default)"},
    {TR_CONFIG_DISPLAY_SCALE, "Display scale:"},
    {TR_CONFIG_CURSOR_SCALE, "Cursor scale:"},
    {TR_CONFIG_HEADER_UI_CHANGES, "User interface changes"},
    {TR_CONFIG_HEADER_GAMEPLAY_CHANGES, "Gameplay changes"},
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
    {TR_CONFIG_FIX_IMMIGRATION_BUG, "Fix immigration bug on very hard"},
    {TR_CONFIG_FIX_100_YEAR_GHOSTS, "Fix 100-year-old ghosts"},
    {TR_HOTKEY_TITLE, "Julius hotkey configuration"},
    {TR_HOTKEY_LABEL, "Hotkey"},
    {TR_HOTKEY_ALTERNATIVE_LABEL, "Alternative"},
    {TR_HOTKEY_HEADER_ARROWS, "Arrow keys"},
    {TR_HOTKEY_HEADER_GLOBAL, "Global hotkeys"},
    {TR_HOTKEY_HEADER_CITY, "City hotkeys"},
    {TR_HOTKEY_HEADER_ADVISORS, "Advisors"},
    {TR_HOTKEY_HEADER_OVERLAYS, "Overlays"},
    {TR_HOTKEY_HEADER_BOOKMARKS, "City map bookmarks"},
    {TR_HOTKEY_HEADER_EDITOR, "Editor"},
    {TR_HOTKEY_HEADER_BUILD, "Construction hotkeys"},
    {TR_HOTKEY_ARROW_UP, "Up"},
    {TR_HOTKEY_ARROW_DOWN, "Down"},
    {TR_HOTKEY_ARROW_LEFT, "Left"},
    {TR_HOTKEY_ARROW_RIGHT, "Right"},
    {TR_HOTKEY_TOGGLE_FULLSCREEN, "Toggle fullscreen"},
    {TR_HOTKEY_CENTER_WINDOW, "Center window"},
    {TR_HOTKEY_RESIZE_TO_640, "Resize window to 640x480"},
    {TR_HOTKEY_RESIZE_TO_800, "Resize window to 800x600"},
    {TR_HOTKEY_RESIZE_TO_1024, "Resize window to 1024x768"},
    {TR_HOTKEY_SAVE_SCREENSHOT, "Save screenshot"},
    {TR_HOTKEY_SAVE_CITY_SCREENSHOT, "Save full city screenshot"},
    {TR_HOTKEY_BUILD_CLONE, "Clone building under cursor"},
    {TR_HOTKEY_LOAD_FILE, "Load file"},
    {TR_HOTKEY_SAVE_FILE, "Save file"},
    {TR_HOTKEY_INCREASE_GAME_SPEED, "Increase game speed"},
    {TR_HOTKEY_DECREASE_GAME_SPEED, "Decrease game speed"},
    {TR_HOTKEY_TOGGLE_PAUSE, "Toggle pause"},
    {TR_HOTKEY_CYCLE_LEGION, "Cycle through legions"},
    {TR_HOTKEY_ROTATE_MAP_LEFT, "Rotate map left"},
    {TR_HOTKEY_ROTATE_MAP_RIGHT, "Rotate map right"},
    {TR_HOTKEY_SHOW_ADVISOR_LABOR, "Labor advisor"},
    {TR_HOTKEY_SHOW_ADVISOR_MILITARY, "Military advisor"},
    {TR_HOTKEY_SHOW_ADVISOR_IMPERIAL, "Imperial advisor"},
    {TR_HOTKEY_SHOW_ADVISOR_RATINGS, "Ratings advisor"},
    {TR_HOTKEY_SHOW_ADVISOR_TRADE, "Trade advisor"},
    {TR_HOTKEY_SHOW_ADVISOR_POPULATION, "Population advisor"},
    {TR_HOTKEY_SHOW_ADVISOR_HEALTH, "Health advisor"},
    {TR_HOTKEY_SHOW_ADVISOR_EDUCATION, "Education advisor"},
    {TR_HOTKEY_SHOW_ADVISOR_ENTERTAINMENT, "Entertainment advisor"},
    {TR_HOTKEY_SHOW_ADVISOR_RELIGION, "Religion advisor"},
    {TR_HOTKEY_SHOW_ADVISOR_FINANCIAL, "Financial advisor"},
    {TR_HOTKEY_SHOW_ADVISOR_CHIEF, "Chief advisor"},
    {TR_HOTKEY_TOGGLE_OVERLAY, "Toggle current overlay"},
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
    {TR_ALLOWED_BUILDING_SENATE_UPGRADED, "Senate upgraded"},
    {TR_ALLOWED_BUILDING_GOVERNORS_HOUSE, "Governors house"},
    {TR_ALLOWED_BUILDING_GOVERNORS_VILLA, "Governors villa"},
    {TR_ALLOWED_BUILDING_GOVERNORS_PALACE, "Governors palace"},
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
};

void translation_english(const translation_string **strings, int *num_strings)
{
    *strings = all_strings;
    *num_strings = sizeof(all_strings) / sizeof(translation_string);
}
