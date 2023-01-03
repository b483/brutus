#include "city/buildings.h"
#include "core/config.h"
#include "menu.h"
#include "scenario/editor_events.h"

#define BUILD_MENU_ITEM_MAX 30

static const building_type MENU_BUILDING_TYPE[BUILD_MENU_MAX][BUILD_MENU_ITEM_MAX] = {
    {BUILDING_HOUSE_VACANT_LOT, 0},
    {BUILDING_CLEAR_LAND, 0},
    {BUILDING_ROAD, 0},
    {BUILDING_RESERVOIR, BUILDING_AQUEDUCT, BUILDING_FOUNTAIN, BUILDING_WELL, 0},
    {BUILDING_BARBER, BUILDING_BATHHOUSE, BUILDING_DOCTOR, BUILDING_HOSPITAL, 0},
    {BUILDING_MENU_SMALL_TEMPLES, BUILDING_MENU_LARGE_TEMPLES, BUILDING_ORACLE, 0},
    {BUILDING_SCHOOL, BUILDING_ACADEMY, BUILDING_LIBRARY, BUILDING_MISSION_POST, 0},
    {BUILDING_THEATER, BUILDING_AMPHITHEATER, BUILDING_COLOSSEUM, BUILDING_HIPPODROME,
        BUILDING_GLADIATOR_SCHOOL, BUILDING_LION_HOUSE, BUILDING_ACTOR_COLONY, BUILDING_CHARIOT_MAKER, 0},
    {BUILDING_FORUM, BUILDING_SENATE_UPGRADED,
        BUILDING_GOVERNORS_HOUSE, BUILDING_GOVERNORS_VILLA, BUILDING_GOVERNORS_PALACE,
        BUILDING_SMALL_STATUE, BUILDING_MEDIUM_STATUE, BUILDING_LARGE_STATUE, BUILDING_TRIUMPHAL_ARCH, 0},
    {BUILDING_GARDENS, BUILDING_PLAZA, BUILDING_ENGINEERS_POST, BUILDING_LOW_BRIDGE, BUILDING_SHIP_BRIDGE,
        BUILDING_SHIPYARD, BUILDING_DOCK, BUILDING_WHARF, 0},
    {BUILDING_WALL, BUILDING_TOWER, BUILDING_GATEHOUSE, BUILDING_PREFECTURE,
        BUILDING_FORT, BUILDING_MILITARY_ACADEMY, BUILDING_BARRACKS, 0},
    {BUILDING_MENU_FARMS, BUILDING_MENU_RAW_MATERIALS, BUILDING_MENU_WORKSHOPS,
        BUILDING_MARKET, BUILDING_GRANARY, BUILDING_WAREHOUSE, 0},
    {BUILDING_WHEAT_FARM, BUILDING_VEGETABLE_FARM, BUILDING_FRUIT_FARM,
        BUILDING_OLIVE_FARM, BUILDING_VINES_FARM, BUILDING_PIG_FARM, 0},
    {BUILDING_CLAY_PIT, BUILDING_MARBLE_QUARRY, BUILDING_IRON_MINE, BUILDING_TIMBER_YARD, 0},
    {BUILDING_WINE_WORKSHOP, BUILDING_OIL_WORKSHOP, BUILDING_WEAPONS_WORKSHOP,
        BUILDING_FURNITURE_WORKSHOP, BUILDING_POTTERY_WORKSHOP, 0},
    {BUILDING_MENU_SMALL_TEMPLES, BUILDING_SMALL_TEMPLE_CERES, BUILDING_SMALL_TEMPLE_NEPTUNE,
        BUILDING_SMALL_TEMPLE_MERCURY, BUILDING_SMALL_TEMPLE_MARS, BUILDING_SMALL_TEMPLE_VENUS, 0},
    {BUILDING_MENU_LARGE_TEMPLES, BUILDING_LARGE_TEMPLE_CERES, BUILDING_LARGE_TEMPLE_NEPTUNE,
        BUILDING_LARGE_TEMPLE_MERCURY, BUILDING_LARGE_TEMPLE_MARS, BUILDING_LARGE_TEMPLE_VENUS, 0},
    {BUILDING_FORT_LEGIONARIES, BUILDING_FORT_JAVELIN, BUILDING_FORT_MOUNTED, 0},
};
static int menu_enabled[BUILD_MENU_MAX][BUILD_MENU_ITEM_MAX];

static int changed = 1;

static int get_building_menu_item_for_type(building_type type);


void building_menu_disable_all(void)
{
    for (int sub = 0; sub < BUILD_MENU_MAX; sub++) {
        for (int item = 0; item < BUILD_MENU_ITEM_MAX; item++) {
            menu_enabled[sub][item] = 0;
        }
    }
}

static void enable_cycling_temples_if_configured(building_type type)
{
    int sub = (type == BUILDING_MENU_SMALL_TEMPLES) ? BUILD_MENU_SMALL_TEMPLES : BUILD_MENU_LARGE_TEMPLES;
    menu_enabled[sub][0] = config_get(CONFIG_UI_ALLOW_CYCLING_TEMPLES);
}


static void enable_if_allowed(int *enabled, building_type menu_building_type, building_type type)
{
    if (menu_building_type == type && scenario_building_allowed(type)) {
        *enabled = 1;
    }
}

/**
 * Only submenus with 1 or more enabled items are shown as buttons
 */
static void enable_building_group_menu_items(void)
{
    if (building_menu_is_enabled(BUILDING_SMALL_TEMPLE_CERES)
        || building_menu_is_enabled(BUILDING_SMALL_TEMPLE_NEPTUNE)
        || building_menu_is_enabled(BUILDING_SMALL_TEMPLE_MERCURY)
        || building_menu_is_enabled(BUILDING_SMALL_TEMPLE_MARS)
        || building_menu_is_enabled(BUILDING_SMALL_TEMPLE_VENUS)
    ) {
        menu_enabled[get_building_menu_for_type(BUILDING_MENU_SMALL_TEMPLES)][get_building_menu_item_for_type(BUILDING_MENU_SMALL_TEMPLES)] = 1;
        enable_cycling_temples_if_configured(BUILDING_MENU_SMALL_TEMPLES);
    }

    if (building_menu_is_enabled(BUILDING_LARGE_TEMPLE_CERES)
        || building_menu_is_enabled(BUILDING_LARGE_TEMPLE_NEPTUNE)
        || building_menu_is_enabled(BUILDING_LARGE_TEMPLE_MERCURY)
        || building_menu_is_enabled(BUILDING_LARGE_TEMPLE_MARS)
        || building_menu_is_enabled(BUILDING_LARGE_TEMPLE_VENUS)
    ) {
        menu_enabled[get_building_menu_for_type(BUILDING_MENU_LARGE_TEMPLES)][get_building_menu_item_for_type(BUILDING_MENU_LARGE_TEMPLES)] = 1;
        enable_cycling_temples_if_configured(BUILDING_MENU_LARGE_TEMPLES);
    }

    if (building_menu_is_enabled(BUILDING_FORT_LEGIONARIES)
        || building_menu_is_enabled(BUILDING_FORT_JAVELIN)
        || building_menu_is_enabled(BUILDING_FORT_MOUNTED)
    ) {
        menu_enabled[get_building_menu_for_type(BUILDING_FORT)][get_building_menu_item_for_type(BUILDING_FORT)] = 1;
    }

    if (building_menu_is_enabled(BUILDING_WHEAT_FARM)
        || building_menu_is_enabled(BUILDING_VEGETABLE_FARM)
        || building_menu_is_enabled(BUILDING_FRUIT_FARM)
        || building_menu_is_enabled(BUILDING_OLIVE_FARM)
        || building_menu_is_enabled(BUILDING_VINES_FARM)
        || building_menu_is_enabled(BUILDING_PIG_FARM)
    ) {
        menu_enabled[get_building_menu_for_type(BUILDING_MENU_FARMS)][get_building_menu_item_for_type(BUILDING_MENU_FARMS)] = 1;
    }

    if (building_menu_is_enabled(BUILDING_CLAY_PIT)
        || building_menu_is_enabled(BUILDING_MARBLE_QUARRY)
        || building_menu_is_enabled(BUILDING_IRON_MINE)
        || building_menu_is_enabled(BUILDING_TIMBER_YARD)
    ) {
        menu_enabled[get_building_menu_for_type(BUILDING_MENU_RAW_MATERIALS)][get_building_menu_item_for_type(BUILDING_MENU_RAW_MATERIALS)] = 1;
    }

    if (building_menu_is_enabled(BUILDING_WINE_WORKSHOP)
       || building_menu_is_enabled(BUILDING_OIL_WORKSHOP)
       || building_menu_is_enabled(BUILDING_WEAPONS_WORKSHOP)
       || building_menu_is_enabled(BUILDING_FURNITURE_WORKSHOP)
       || building_menu_is_enabled(BUILDING_POTTERY_WORKSHOP)
    ) {
        menu_enabled[get_building_menu_for_type(BUILDING_MENU_WORKSHOPS)][get_building_menu_item_for_type(BUILDING_MENU_WORKSHOPS)] = 1;
    }
}

static void enable_normal(int *enabled, building_type type)
{
    enable_if_allowed(enabled, type, BUILDING_HOUSE_VACANT_LOT);
    // clear land option always enabled to allow deleting buildings
    if (type == BUILDING_CLEAR_LAND) {
        *enabled = 1;
    }
    enable_if_allowed(enabled, type, BUILDING_ROAD);
    enable_if_allowed(enabled, type, BUILDING_RESERVOIR);
    enable_if_allowed(enabled, type, BUILDING_AQUEDUCT);
    enable_if_allowed(enabled, type, BUILDING_FOUNTAIN);
    enable_if_allowed(enabled, type, BUILDING_WELL);
    enable_if_allowed(enabled, type, BUILDING_BARBER);
    enable_if_allowed(enabled, type, BUILDING_BATHHOUSE);
    enable_if_allowed(enabled, type, BUILDING_DOCTOR);
    enable_if_allowed(enabled, type, BUILDING_HOSPITAL);
    enable_if_allowed(enabled, type, BUILDING_SMALL_TEMPLE_CERES);
    enable_if_allowed(enabled, type, BUILDING_SMALL_TEMPLE_NEPTUNE);
    enable_if_allowed(enabled, type, BUILDING_SMALL_TEMPLE_MERCURY);
    enable_if_allowed(enabled, type, BUILDING_SMALL_TEMPLE_MARS);
    enable_if_allowed(enabled, type, BUILDING_SMALL_TEMPLE_VENUS);
    enable_if_allowed(enabled, type, BUILDING_LARGE_TEMPLE_CERES);
    enable_if_allowed(enabled, type, BUILDING_LARGE_TEMPLE_NEPTUNE);
    enable_if_allowed(enabled, type, BUILDING_LARGE_TEMPLE_MERCURY);
    enable_if_allowed(enabled, type, BUILDING_LARGE_TEMPLE_MARS);
    enable_if_allowed(enabled, type, BUILDING_LARGE_TEMPLE_VENUS);
    enable_if_allowed(enabled, type, BUILDING_ORACLE);
    enable_if_allowed(enabled, type, BUILDING_SCHOOL);
    enable_if_allowed(enabled, type, BUILDING_ACADEMY);
    enable_if_allowed(enabled, type, BUILDING_LIBRARY);
    enable_if_allowed(enabled, type, BUILDING_MISSION_POST);
    enable_if_allowed(enabled, type, BUILDING_THEATER);
    enable_if_allowed(enabled, type, BUILDING_AMPHITHEATER);
    enable_if_allowed(enabled, type, BUILDING_COLOSSEUM);
    enable_if_allowed(enabled, type, BUILDING_HIPPODROME);
    enable_if_allowed(enabled, type, BUILDING_GLADIATOR_SCHOOL);
    enable_if_allowed(enabled, type, BUILDING_LION_HOUSE);
    enable_if_allowed(enabled, type, BUILDING_ACTOR_COLONY);
    enable_if_allowed(enabled, type, BUILDING_CHARIOT_MAKER);
    enable_if_allowed(enabled, type, BUILDING_FORUM);
    enable_if_allowed(enabled, type, BUILDING_SENATE_UPGRADED);
    enable_if_allowed(enabled, type, BUILDING_GOVERNORS_HOUSE);
    enable_if_allowed(enabled, type, BUILDING_GOVERNORS_VILLA);
    enable_if_allowed(enabled, type, BUILDING_GOVERNORS_PALACE);
    enable_if_allowed(enabled, type, BUILDING_SMALL_STATUE);
    enable_if_allowed(enabled, type, BUILDING_MEDIUM_STATUE);
    enable_if_allowed(enabled, type, BUILDING_LARGE_STATUE);
    if (type == BUILDING_TRIUMPHAL_ARCH) {
        if (city_buildings_triumphal_arch_available()) {
            enable_if_allowed(enabled, type, BUILDING_TRIUMPHAL_ARCH);
        }
    }
    enable_if_allowed(enabled, type, BUILDING_GARDENS);
    enable_if_allowed(enabled, type, BUILDING_PLAZA);
    enable_if_allowed(enabled, type, BUILDING_ENGINEERS_POST);
    enable_if_allowed(enabled, type, BUILDING_LOW_BRIDGE);
    enable_if_allowed(enabled, type, BUILDING_SHIP_BRIDGE);
    enable_if_allowed(enabled, type, BUILDING_SHIPYARD);
    enable_if_allowed(enabled, type, BUILDING_WHARF);
    enable_if_allowed(enabled, type, BUILDING_DOCK);
    enable_if_allowed(enabled, type, BUILDING_WALL);
    enable_if_allowed(enabled, type, BUILDING_TOWER);
    enable_if_allowed(enabled, type, BUILDING_GATEHOUSE);
    enable_if_allowed(enabled, type, BUILDING_PREFECTURE);
    enable_if_allowed(enabled, type, BUILDING_FORT_LEGIONARIES);
    enable_if_allowed(enabled, type, BUILDING_FORT_JAVELIN);
    enable_if_allowed(enabled, type, BUILDING_FORT_MOUNTED);
    enable_if_allowed(enabled, type, BUILDING_MILITARY_ACADEMY);
    enable_if_allowed(enabled, type, BUILDING_BARRACKS);
    enable_if_allowed(enabled, type, BUILDING_WHEAT_FARM);
    enable_if_allowed(enabled, type, BUILDING_VEGETABLE_FARM);
    enable_if_allowed(enabled, type, BUILDING_FRUIT_FARM);
    enable_if_allowed(enabled, type, BUILDING_OLIVE_FARM);
    enable_if_allowed(enabled, type, BUILDING_VINES_FARM);
    enable_if_allowed(enabled, type, BUILDING_PIG_FARM);
    enable_if_allowed(enabled, type, BUILDING_CLAY_PIT);
    enable_if_allowed(enabled, type, BUILDING_MARBLE_QUARRY);
    enable_if_allowed(enabled, type, BUILDING_IRON_MINE);
    enable_if_allowed(enabled, type, BUILDING_TIMBER_YARD);
    enable_if_allowed(enabled, type, BUILDING_WINE_WORKSHOP);
    enable_if_allowed(enabled, type, BUILDING_OIL_WORKSHOP);
    enable_if_allowed(enabled, type, BUILDING_WEAPONS_WORKSHOP);
    enable_if_allowed(enabled, type, BUILDING_FURNITURE_WORKSHOP);
    enable_if_allowed(enabled, type, BUILDING_POTTERY_WORKSHOP);
    enable_if_allowed(enabled, type, BUILDING_MARKET);
    enable_if_allowed(enabled, type, BUILDING_GRANARY);
    enable_if_allowed(enabled, type, BUILDING_WAREHOUSE);
}

void building_menu_update(void)
{
    building_menu_disable_all();
    for (int sub = 0; sub < BUILD_MENU_MAX; sub++) {
        for (int item = 0; item < BUILD_MENU_ITEM_MAX; item++) {
            int building_type = MENU_BUILDING_TYPE[sub][item];
            int *menu_item = &menu_enabled[sub][item];
            enable_normal(menu_item, building_type);
        }
    }
    enable_building_group_menu_items();
    changed = 1;
}

int building_menu_count_items(int submenu)
{
    int count = 0;
    for (int item = 0; item < BUILD_MENU_ITEM_MAX; item++) {
        if (menu_enabled[submenu][item] && MENU_BUILDING_TYPE[submenu][item] > 0) {
            count++;
        }
    }
    return count;
}

int building_menu_next_index(int submenu, int current_index)
{
    for (int i = current_index + 1; i < BUILD_MENU_ITEM_MAX; i++) {
        if (MENU_BUILDING_TYPE[submenu][i] <= 0) {
            return 0;
        }
        if (menu_enabled[submenu][i]) {
            return i;
        }
    }
    return 0;
}

building_type building_menu_type(int submenu, int item)
{
    return MENU_BUILDING_TYPE[submenu][item];
}

static int get_building_menu_item_for_type(building_type type)
{
    for (int sub = 0; sub < BUILD_MENU_MAX; sub++) {
        for (int item = 0; item < BUILD_MENU_ITEM_MAX && MENU_BUILDING_TYPE[sub][item]; item++) {
            if (MENU_BUILDING_TYPE[sub][item] == type) {
                return item;
            }
        }
    }
    return -1;
}

build_menu_group get_building_menu_for_type(building_type type)
{
    for (int sub = 0; sub < BUILD_MENU_MAX; sub++) {
        for (int item = 0; item < BUILD_MENU_ITEM_MAX && MENU_BUILDING_TYPE[sub][item]; item++) {
            if (MENU_BUILDING_TYPE[sub][item] == type) {
                return sub;
            }
        }
    }
    return -1;
}

int building_menu_is_enabled(building_type type)
{
    for (int sub = 0; sub < BUILD_MENU_MAX; sub++) {
        for (int item = 0; item < BUILD_MENU_ITEM_MAX && MENU_BUILDING_TYPE[sub][item]; item++) {
            if (MENU_BUILDING_TYPE[sub][item] == type) {
                return menu_enabled[sub][item];
            }
        }
    }
    return 0;
}

int building_menu_has_changed(void)
{
    if (changed) {
        changed = 0;
        return 1;
    }
    return 0;
}
