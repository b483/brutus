#ifndef SCENARIO_DATA_H
#define SCENARIO_DATA_H

#include "map/point.h"

#include <stdint.h>

#define MAX_REQUESTS 40
#define MAX_EDITOR_CUSTOM_MESSAGES 45
#define MAX_INVASIONS 40
#define ENEMY_TYPE_MAX_COUNT 20
#define MAX_PRICE_CHANGES 40
#define MAX_DEMAND_CHANGES 40

#define MAX_HERD_POINTS 8
#define MAX_FISH_POINTS 8
#define MAX_INVASION_POINTS 8

#define MAX_ALLOWED_BUILDINGS 78

#define MAX_PLAYER_NAME 24
#define MAX_SCENARIO_NAME 65
#define MAX_BRIEF_DESCRIPTION 32
#define MAX_BRIEFING 2500
#define MAX_CUSTOM_MESSAGE_TITLE 30
#define MAX_CUSTOM_MESSAGE_TEXT 1000
#define MAX_CUSTOM_MESSAGE_VIDEO_TEXT 24

enum {
    CLIMATE_CENTRAL = 0,
    CLIMATE_NORTHERN = 1,
    CLIMATE_DESERT = 2
};

enum {
    ALLOWED_BUILDING_HOUSE_VACANT_LOT = 0,
    ALLOWED_BUILDING_CLEAR_TERRAIN = 1,
    ALLOWED_BUILDING_ROAD = 2,
    ALLOWED_BUILDING_RESERVOIR = 3,
    ALLOWED_BUILDING_AQUEDUCT = 4,
    ALLOWED_BUILDING_FOUNTAIN = 5,
    ALLOWED_BUILDING_WELL = 6,
    ALLOWED_BUILDING_BARBER = 7,
    ALLOWED_BUILDING_BATHHOUSE = 8,
    ALLOWED_BUILDING_DOCTOR = 9,
    ALLOWED_BUILDING_HOSPITAL = 10,
    ALLOWED_BUILDING_SMALL_TEMPLE_CERES = 11,
    ALLOWED_BUILDING_SMALL_TEMPLE_NEPTUNE = 12,
    ALLOWED_BUILDING_SMALL_TEMPLE_MERCURY = 13,
    ALLOWED_BUILDING_SMALL_TEMPLE_MARS = 14,
    ALLOWED_BUILDING_SMALL_TEMPLE_VENUS = 15,
    ALLOWED_BUILDING_LARGE_TEMPLE_CERES = 16,
    ALLOWED_BUILDING_LARGE_TEMPLE_NEPTUNE = 17,
    ALLOWED_BUILDING_LARGE_TEMPLE_MERCURY = 18,
    ALLOWED_BUILDING_LARGE_TEMPLE_MARS = 19,
    ALLOWED_BUILDING_LARGE_TEMPLE_VENUS = 20,
    ALLOWED_BUILDING_ORACLE = 21,
    ALLOWED_BUILDING_SCHOOL = 22,
    ALLOWED_BUILDING_ACADEMY = 23,
    ALLOWED_BUILDING_LIBRARY = 24,
    ALLOWED_BUILDING_MISSION_POST = 25,
    ALLOWED_BUILDING_THEATER = 26,
    ALLOWED_BUILDING_AMPHITHEATER = 27,
    ALLOWED_BUILDING_COLOSSEUM = 28,
    ALLOWED_BUILDING_HIPPODROME = 29,
    ALLOWED_BUILDING_GLADIATOR_SCHOOL = 30,
    ALLOWED_BUILDING_LION_HOUSE = 31,
    ALLOWED_BUILDING_ACTOR_COLONY = 32,
    ALLOWED_BUILDING_CHARIOT_MAKER = 33,
    ALLOWED_BUILDING_FORUM = 34,
    ALLOWED_BUILDING_SENATE_UPGRADED = 35,
    ALLOWED_BUILDING_GOVERNORS_HOUSE = 36,
    ALLOWED_BUILDING_GOVERNORS_VILLA = 37,
    ALLOWED_BUILDING_GOVERNORS_PALACE = 38,
    ALLOWED_BUILDING_SMALL_STATUE = 39,
    ALLOWED_BUILDING_MEDIUM_STATUE = 40,
    ALLOWED_BUILDING_LARGE_STATUE = 41,
    ALLOWED_BUILDING_TRIUMPHAL_ARCH = 42,
    ALLOWED_BUILDING_GARDENS = 43,
    ALLOWED_BUILDING_PLAZA = 44,
    ALLOWED_BUILDING_ENGINEERS_POST = 45,
    ALLOWED_BUILDING_LOW_BRIDGE = 46,
    ALLOWED_BUILDING_SHIP_BRIDGE = 47,
    ALLOWED_BUILDING_SHIPYARD = 48,
    ALLOWED_BUILDING_WHARF = 49,
    ALLOWED_BUILDING_DOCK = 50,
    ALLOWED_BUILDING_WALL = 51,
    ALLOWED_BUILDING_TOWER = 52,
    ALLOWED_BUILDING_GATEHOUSE = 53,
    ALLOWED_BUILDING_PREFECTURE = 54,
    ALLOWED_BUILDING_FORT_LEGIONARIES = 55,
    ALLOWED_BUILDING_FORT_JAVELIN = 56,
    ALLOWED_BUILDING_FORT_MOUNTED = 57,
    ALLOWED_BUILDING_MILITARY_ACADEMY = 58,
    ALLOWED_BUILDING_BARRACKS = 59,
    ALLOWED_BUILDING_WHEAT_FARM = 60,
    ALLOWED_BUILDING_VEGETABLE_FARM = 61,
    ALLOWED_BUILDING_FRUIT_FARM = 62,
    ALLOWED_BUILDING_OLIVE_FARM = 63,
    ALLOWED_BUILDING_VINES_FARM = 64,
    ALLOWED_BUILDING_PIG_FARM = 65,
    ALLOWED_BUILDING_CLAY_PIT = 66,
    ALLOWED_BUILDING_MARBLE_QUARRY = 67,
    ALLOWED_BUILDING_IRON_MINE = 68,
    ALLOWED_BUILDING_TIMBER_YARD = 69,
    ALLOWED_BUILDING_WINE_WORKSHOP = 70,
    ALLOWED_BUILDING_OIL_WORKSHOP = 71,
    ALLOWED_BUILDING_WEAPONS_WORKSHOP = 72,
    ALLOWED_BUILDING_FURNITURE_WORKSHOP = 73,
    ALLOWED_BUILDING_POTTERY_WORKSHOP = 74,
    ALLOWED_BUILDING_MARKET = 75,
    ALLOWED_BUILDING_GRANARY = 76,
    ALLOWED_BUILDING_WAREHOUSE = 77,
};

enum {
    EARTHQUAKE_NONE = 0,
    EARTHQUAKE_SMALL = 1,
    EARTHQUAKE_MEDIUM = 2,
    EARTHQUAKE_LARGE = 3
};

enum {
    EVENT_DISABLED = 0,
    EVENT_NOT_STARTED = 1,
    EVENT_IN_PROGRESS = 2,
    EVENT_FINISHED = 3
};

enum {
    INVASION_TYPE_LOCAL_UPRISING = 1,
    INVASION_TYPE_ENEMY_ARMY = 2,
    INVASION_TYPE_CAESAR = 3,
    INVASION_TYPE_DISTANT_BATTLE = 4,
};

struct win_criteria_t {
    int8_t enabled;
    int32_t goal;
};

struct request_t {
    int16_t year;
    int16_t amount;
    int16_t years_deadline;
    int16_t months_to_comply;
    int8_t month;
    int8_t resource;
    int8_t favor;
    int8_t state;
    int8_t visible;
    int8_t can_comply_dialog_shown;
};

struct editor_custom_messages_t {
    unsigned char title[MAX_CUSTOM_MESSAGE_TITLE];
    unsigned char text[MAX_CUSTOM_MESSAGE_TEXT];
    unsigned char video_file[MAX_CUSTOM_MESSAGE_VIDEO_TEXT];
    int8_t month;
    int8_t urgent;
    int8_t enabled;
    int16_t year;
};

struct invasion_t {
    int16_t year_offset;
    int16_t amount;
    int8_t month;
    int8_t type;
    int8_t enemy_type;
    int8_t from;
    int8_t target_type;
};

struct price_change_t {
    int16_t year;
    int8_t month;
    int8_t resource;
    int8_t is_rise;
    int8_t amount;
};

struct demand_change_t {
    int16_t year;
    int8_t month;
    int8_t resource;
    int8_t route_id;
    int8_t is_rise;
};

extern struct scenario_t {
    struct {
        int16_t width;
        int16_t height;
        int16_t grid_start;
        int16_t grid_border_size;
    } map;

    struct {
        int16_t expansion_year;
        int8_t id;
        int8_t is_expanded;
        int8_t distant_battle_roman_travel_months;
        int8_t distant_battle_enemy_travel_months;
    } empire;

    unsigned char scenario_name[MAX_SCENARIO_NAME];
    unsigned char brief_description[MAX_BRIEF_DESCRIPTION];
    int8_t brief_description_image_id;
    unsigned char briefing[MAX_BRIEFING];
    int8_t climate;
    int8_t player_rank;
    int16_t start_year;
    int8_t initial_favor;
    int32_t initial_funds;
    int32_t rescue_loan;
    int32_t initial_personal_savings;
    int8_t rome_supplies_wheat;
    int8_t flotsam_enabled;
    int8_t is_open_play;
    struct win_criteria_t population_win_criteria;
    struct win_criteria_t culture_win_criteria;
    struct win_criteria_t prosperity_win_criteria;
    struct win_criteria_t peace_win_criteria;
    struct win_criteria_t favor_win_criteria;
    struct {
        int8_t enabled;
        int16_t years;
    } time_limit_win_criteria;
    struct {
        int8_t enabled;
        int16_t years;
    } survival_time_win_criteria;
    int8_t allowed_buildings[MAX_ALLOWED_BUILDINGS];
    struct {
        int8_t severity;
        int8_t month;
        int16_t year;
    } earthquake;
    struct {
        int8_t state;
        int8_t month;
        int16_t year;
    } gladiator_revolt;
    struct {
        int8_t sea_trade_problem;
        int8_t land_trade_problem;
        int8_t raise_wages;
        int8_t lower_wages;
        int8_t contaminated_water;
    } random_events;
    struct request_t requests[MAX_REQUESTS];
    struct editor_custom_messages_t editor_custom_messages[MAX_EDITOR_CUSTOM_MESSAGES];
    struct invasion_t invasions[MAX_INVASIONS];
    int8_t invasion_upcoming;
    struct price_change_t price_changes[MAX_PRICE_CHANGES];
    struct demand_change_t demand_changes[MAX_DEMAND_CHANGES];

    map_point earthquake_point;
    map_point invasion_points[MAX_INVASION_POINTS];
    map_point entry_point;
    map_point exit_point;
    map_point river_entry_point;
    map_point river_exit_point;
    map_point herd_points[MAX_HERD_POINTS];
    map_point fishing_points[MAX_FISH_POINTS];

    struct {
        int32_t hut;
        int32_t meeting;
        int32_t crops;
        int32_t vacant_lots;
    } native_images;

    int8_t is_saved;
} scenario;

extern struct scenario_settings {
    unsigned char player_name[MAX_PLAYER_NAME];
} scenario_settings;

#endif // SCENARIO_DATA_H
