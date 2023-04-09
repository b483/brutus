#ifndef SCENARIO_DATA_H
#define SCENARIO_DATA_H

#include "map/point.h"

#define MAX_PLAYER_NAME 24
#define MAX_SCENARIO_NAME 65
#define MAX_BRIEF_DESCRIPTION 32
#define MAX_BRIEFING 2500
#define MAX_ALLOWED_BUILDINGS 78
#define MAX_REQUESTS 40
#define MAX_EDITOR_CUSTOM_MESSAGES 45
#define MAX_EARTHQUAKE_POINTS 8
#define MAX_EARTHQUAKES 10
#define MAX_EARTHQUAKE_BRANCHES 4
#define MAX_INVASION_POINTS 8
#define MAX_INVASIONS 40
#define ENEMY_TYPE_MAX_COUNT 20
#define MAX_PRICE_CHANGES 40
#define MAX_DEMAND_CHANGES 40
#define MAX_CUSTOM_MESSAGE_TITLE 30
#define MAX_CUSTOM_MESSAGE_TEXT 1000
#define MAX_CUSTOM_MESSAGE_VIDEO_TEXT 24
#define MAX_HERD_POINTS 8
#define MAX_FISH_POINTS 8


enum {
    CLIMATE_NORTHERN = 0,
    CLIMATE_CENTRAL = 1,
    CLIMATE_DESERT = 2,
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
    uint8_t enabled;
    uint32_t goal;
};

struct time_limit_criteria_t {
    uint8_t enabled;
    uint16_t years;
};

struct request_t {
    uint16_t year;
    uint16_t amount;
    uint16_t years_deadline;
    uint16_t months_to_comply;
    uint8_t month;
    uint8_t resource;
    uint8_t favor;
    uint8_t state;
    uint8_t visible;
    uint8_t can_comply_dialog_shown;
};

struct editor_custom_messages_t {
    unsigned char title[MAX_CUSTOM_MESSAGE_TITLE];
    unsigned char text[MAX_CUSTOM_MESSAGE_TEXT];
    unsigned char video_file[MAX_CUSTOM_MESSAGE_VIDEO_TEXT];
    uint8_t month;
    uint8_t urgent;
    uint8_t enabled;
    uint16_t year;
};

struct earthquake_t {
    uint8_t state;
    uint8_t severity;
    uint8_t month;
    uint16_t year;
    uint16_t duration;
    uint16_t max_duration;
    uint8_t delay;
    uint8_t max_delay;
    uint8_t point;
    map_point branch_coordinates[MAX_EARTHQUAKE_BRANCHES];
};

struct invasion_t {
    uint16_t year_offset;
    uint16_t amount;
    uint8_t month;
    uint8_t type;
    uint8_t enemy_type;
    uint8_t from;
    uint8_t target_type;
};

struct price_change_t {
    uint16_t year;
    uint8_t month;
    uint8_t resource;
    uint8_t is_rise;
    uint8_t amount;
};

struct demand_change_t {
    uint16_t year;
    uint8_t month;
    uint8_t resource;
    uint8_t trade_city_id;
    uint8_t is_rise;
};

struct scenario_t {
    struct {
        uint16_t width;
        uint16_t height;
        uint16_t grid_start;
        uint16_t grid_border_size;
    } map;
    struct {
        uint16_t expansion_year;
        uint8_t id;
        uint8_t is_expanded;
        uint8_t distant_battle_roman_travel_months;
        uint8_t distant_battle_enemy_travel_months;
    } empire;
    unsigned char scenario_name[MAX_SCENARIO_NAME];
    unsigned char brief_description[MAX_BRIEF_DESCRIPTION];
    uint8_t brief_description_image_id;
    unsigned char briefing[MAX_BRIEFING];
    uint8_t climate;
    uint8_t player_rank;
    int16_t start_year;
    uint8_t initial_favor;
    uint32_t initial_funds;
    uint32_t rescue_loan;
    uint32_t initial_personal_savings;
    uint8_t rome_supplies_wheat;
    uint8_t flotsam_enabled;
    struct win_criteria_t population_win_criteria;
    struct win_criteria_t culture_win_criteria;
    struct win_criteria_t prosperity_win_criteria;
    struct win_criteria_t peace_win_criteria;
    struct win_criteria_t favor_win_criteria;
    struct time_limit_criteria_t time_limit_win_criteria;
    struct time_limit_criteria_t survival_time_win_criteria;
    struct {
        uint8_t state;
        uint8_t month;
        uint16_t year;
    } gladiator_revolt;
    struct {
        uint8_t sea_trade_problem;
        uint8_t land_trade_problem;
        uint8_t raise_wages;
        uint8_t lower_wages;
        uint8_t contaminated_water;
    } random_events;
    uint8_t allowed_buildings[MAX_ALLOWED_BUILDINGS];
    struct request_t requests[MAX_REQUESTS];
    struct editor_custom_messages_t editor_custom_messages[MAX_EDITOR_CUSTOM_MESSAGES];
    struct earthquake_t earthquakes[MAX_EARTHQUAKES];
    struct invasion_t invasions[MAX_INVASIONS];
    uint8_t invasion_upcoming;
    struct price_change_t price_changes[MAX_PRICE_CHANGES];
    struct demand_change_t demand_changes[MAX_DEMAND_CHANGES];
    map_point earthquake_points[MAX_EARTHQUAKE_POINTS];
    map_point invasion_points[MAX_INVASION_POINTS];
    map_point entry_point;
    map_point exit_point;
    map_point river_entry_point;
    map_point river_exit_point;
    map_point herd_points[MAX_HERD_POINTS];
    map_point fishing_points[MAX_FISH_POINTS];
    struct {
        uint32_t hut;
        uint32_t meeting;
        uint32_t crops;
        uint32_t vacant_lots;
    } native_images;
    uint8_t is_saved;
};

extern struct scenario_t scenario;

struct scenario_settings_t {
    unsigned char player_name[MAX_PLAYER_NAME];
};

extern struct scenario_settings_t scenario_settings;

#endif // SCENARIO_DATA_H
