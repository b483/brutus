#ifndef BUILDING_TYPE_H
#define BUILDING_TYPE_H

#define MAX_HOUSE_TYPES 20

typedef enum {
    BUILDING_NONE = 0,
    BUILDING_HOUSE_VACANT_LOT = 1,
    BUILDING_HOUSE_SMALL_TENT = 2,
    BUILDING_HOUSE_LARGE_TENT = 3,
    BUILDING_HOUSE_SMALL_SHACK = 4,
    BUILDING_HOUSE_LARGE_SHACK = 5,
    BUILDING_HOUSE_SMALL_HOVEL = 6,
    BUILDING_HOUSE_LARGE_HOVEL = 7,
    BUILDING_HOUSE_SMALL_CASA = 8,
    BUILDING_HOUSE_LARGE_CASA = 9,
    BUILDING_HOUSE_SMALL_INSULA = 10,
    BUILDING_HOUSE_MEDIUM_INSULA = 11,
    BUILDING_HOUSE_LARGE_INSULA = 12,
    BUILDING_HOUSE_GRAND_INSULA = 13,
    BUILDING_HOUSE_SMALL_VILLA = 14,
    BUILDING_HOUSE_MEDIUM_VILLA = 15,
    BUILDING_HOUSE_LARGE_VILLA = 16,
    BUILDING_HOUSE_GRAND_VILLA = 17,
    BUILDING_HOUSE_SMALL_PALACE = 18,
    BUILDING_HOUSE_MEDIUM_PALACE = 19,
    BUILDING_HOUSE_LARGE_PALACE = 20,
    BUILDING_HOUSE_LUXURY_PALACE = 21,
    BUILDING_CLEAR_LAND = 22,
    BUILDING_ROAD = 23,
    BUILDING_RESERVOIR = 24,
    BUILDING_AQUEDUCT = 25,
    BUILDING_FOUNTAIN = 26,
    BUILDING_WELL = 27,
    BUILDING_BARBER = 28,
    BUILDING_BATHHOUSE = 29,
    BUILDING_DOCTOR = 30,
    BUILDING_HOSPITAL = 31,
    BUILDING_MENU_SMALL_TEMPLES = 32,
    BUILDING_SMALL_TEMPLE_CERES = 33,
    BUILDING_SMALL_TEMPLE_NEPTUNE = 34,
    BUILDING_SMALL_TEMPLE_MERCURY = 35,
    BUILDING_SMALL_TEMPLE_MARS = 36,
    BUILDING_SMALL_TEMPLE_VENUS = 37,
    BUILDING_MENU_LARGE_TEMPLES = 38,
    BUILDING_LARGE_TEMPLE_CERES = 39,
    BUILDING_LARGE_TEMPLE_NEPTUNE = 40,
    BUILDING_LARGE_TEMPLE_MERCURY = 41,
    BUILDING_LARGE_TEMPLE_MARS = 42,
    BUILDING_LARGE_TEMPLE_VENUS = 43,
    BUILDING_ORACLE = 44,
    BUILDING_SCHOOL = 45,
    BUILDING_ACADEMY = 46,
    BUILDING_LIBRARY = 47,
    BUILDING_MISSION_POST = 48,
    BUILDING_THEATER = 49,
    BUILDING_AMPHITHEATER = 50,
    BUILDING_COLOSSEUM = 51,
    BUILDING_HIPPODROME = 52,
    BUILDING_GLADIATOR_SCHOOL = 53,
    BUILDING_LION_HOUSE = 54,
    BUILDING_ACTOR_COLONY = 55,
    BUILDING_CHARIOT_MAKER = 56,
    BUILDING_FORUM = 57,
    BUILDING_SENATE = 58,
    BUILDING_GOVERNORS_HOUSE = 59,
    BUILDING_GOVERNORS_VILLA = 60,
    BUILDING_GOVERNORS_PALACE = 61,
    BUILDING_SMALL_STATUE = 62,
    BUILDING_MEDIUM_STATUE = 63,
    BUILDING_LARGE_STATUE = 64,
    BUILDING_TRIUMPHAL_ARCH = 65,
    BUILDING_GARDENS = 66,
    BUILDING_PLAZA = 67,
    BUILDING_ENGINEERS_POST = 68,
    BUILDING_LOW_BRIDGE = 69,
    BUILDING_SHIP_BRIDGE = 70,
    BUILDING_SHIPYARD = 71,
    BUILDING_DOCK = 72,
    BUILDING_WHARF = 73,
    BUILDING_WALL = 74,
    BUILDING_TOWER = 75,
    BUILDING_GATEHOUSE = 76,
    BUILDING_PREFECTURE = 77,
    BUILDING_FORT = 78,
    BUILDING_FORT_LEGIONARIES = 79,
    BUILDING_FORT_JAVELIN = 80,
    BUILDING_FORT_MOUNTED = 81,
    BUILDING_MILITARY_ACADEMY = 82,
    BUILDING_BARRACKS = 83,
    BUILDING_MENU_FARMS = 84,
    BUILDING_WHEAT_FARM = 85,
    BUILDING_VEGETABLE_FARM = 86,
    BUILDING_FRUIT_FARM = 87,
    BUILDING_OLIVE_FARM = 88,
    BUILDING_VINES_FARM = 89,
    BUILDING_PIG_FARM = 90,
    BUILDING_MENU_RAW_MATERIALS = 91,
    BUILDING_CLAY_PIT = 92,
    BUILDING_MARBLE_QUARRY = 93,
    BUILDING_IRON_MINE = 94,
    BUILDING_TIMBER_YARD = 95,
    BUILDING_MENU_WORKSHOPS = 96,
    BUILDING_WINE_WORKSHOP = 97,
    BUILDING_OIL_WORKSHOP = 98,
    BUILDING_WEAPONS_WORKSHOP = 99,
    BUILDING_FURNITURE_WORKSHOP = 100,
    BUILDING_POTTERY_WORKSHOP = 101,
    BUILDING_MARKET = 102,
    BUILDING_GRANARY = 103,
    BUILDING_WAREHOUSE = 104,
    BUILDING_WAREHOUSE_SPACE = 105,
    BUILDING_NATIVE_HUT = 106,
    BUILDING_NATIVE_MEETING = 107,
    BUILDING_NATIVE_CROPS = 108,
    BUILDING_FORT_GROUND = 109,
    BUILDING_BURNING_RUIN = 110,
    // helper constants
    BUILDING_TYPE_MAX = 111
} building_type;

/**
 * House levels
 */
typedef enum {
    HOUSE_SMALL_TENT = 0,
    HOUSE_LARGE_TENT = 1,
    HOUSE_SMALL_SHACK = 2,
    HOUSE_LARGE_SHACK = 3,
    HOUSE_SMALL_HOVEL = 4,
    HOUSE_LARGE_HOVEL = 5,
    HOUSE_SMALL_CASA = 6,
    HOUSE_LARGE_CASA = 7,
    HOUSE_SMALL_INSULA = 8,
    HOUSE_MEDIUM_INSULA = 9,
    HOUSE_LARGE_INSULA = 10,
    HOUSE_GRAND_INSULA = 11,
    HOUSE_SMALL_VILLA = 12,
    HOUSE_MEDIUM_VILLA = 13,
    HOUSE_LARGE_VILLA = 14,
    HOUSE_GRAND_VILLA = 15,
    HOUSE_SMALL_PALACE = 16,
    HOUSE_MEDIUM_PALACE = 17,
    HOUSE_LARGE_PALACE = 18,
    HOUSE_LUXURY_PALACE = 19,
} house_level;

enum {
    BUILDING_STATE_UNUSED = 0,
    BUILDING_STATE_IN_USE = 1,
    BUILDING_STATE_UNDO = 2,
    BUILDING_STATE_CREATED = 3,
    BUILDING_STATE_RUBBLE = 4,
    BUILDING_STATE_DELETED_BY_GAME = 5, // used for earthquakes, fires, house mergers
    BUILDING_STATE_DELETED_BY_PLAYER = 6
};

#endif // BUILDING_TYPE_H
