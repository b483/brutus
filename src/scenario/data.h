#ifndef SCENARIO_DATA_H
#define SCENARIO_DATA_H

#include "map/point.h"
#include "scenario/types.h"

#include <stdint.h>

#define MAX_REQUESTS 20
#define MAX_INVASIONS 20
#define MAX_DEMAND_CHANGES 20
#define MAX_PRICE_CHANGES 20

#define MAX_HERD_POINTS 8
#define MAX_FISH_POINTS 8
#define MAX_INVASION_POINTS 8

#define MAX_ALLOWED_BUILDINGS 78

#define MAX_PLAYER_NAME 24
#define MAX_SCENARIO_NAME 65
#define MAX_BRIEF_DESCRIPTION 32
#define MAX_BRIEFING 2500

enum {
    EVENT_NOT_STARTED = 0,
    EVENT_IN_PROGRESS = 1,
    EVENT_FINISHED = 2
};

enum {
    ALLOWED_BUILDING_HOUSE_VACANT_LOT = 0,
    ALLOWED_BUILDING_CLEAR_LAND = 1,
    ALLOWED_BUILDING_ROAD = 2,
    ALLOWED_BUILDING_DRAGGABLE_RESERVOIR = 3,
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

typedef enum {
    CLIMATE_CENTRAL = 0,
    CLIMATE_NORTHERN = 1,
    CLIMATE_DESERT = 2
} scenario_climate;

struct win_criteria_t {
    int enabled;
    int goal;
};

typedef struct {
    int year;
    int month;
    int amount;
    int resource;
    int years_deadline;
    int favor;
    int state;
    int visible;
    int months_to_comply;
    int can_comply_dialog_shown;
} request_t;

typedef struct {
    int year;
    int month;
    int amount;
    int type;
    int from;
    int attack_type;
} invasion_t;

typedef struct {
    int year;
    int month;
    int resource;
    int is_rise;
    int amount;
} price_change_t;

typedef struct {
    int year;
    int month;
    int resource;
    int route_id;
    int is_rise;
} demand_change_t;

extern struct scenario_t {
    struct {
        int width;
        int height;
        int grid_start;
        int grid_border_size;
    } map;

    struct {
        int id;
        int is_expanded;
        int expansion_year;
        int distant_battle_roman_travel_months;
        int distant_battle_enemy_travel_months;
    } empire;

    uint8_t scenario_name[MAX_SCENARIO_NAME];
    uint8_t brief_description[MAX_BRIEF_DESCRIPTION];
    int brief_description_image_id;
    uint8_t briefing[MAX_BRIEFING];
    int climate;
    int player_rank;
    int start_year;
    int initial_favor;
    int initial_funds;
    int rescue_loan;
    int initial_personal_savings;
    int rome_supplies_wheat;
    int flotsam_enabled;
    int is_open_play;
    struct win_criteria_t population_win_criteria;
    struct win_criteria_t culture_win_criteria;
    struct win_criteria_t prosperity_win_criteria;
    struct win_criteria_t peace_win_criteria;
    struct win_criteria_t favor_win_criteria;
    struct {
        int enabled;
        int years;
    } time_limit_win_criteria;
    struct {
        int enabled;
        int years;
    } survival_time_win_criteria;
    int milestone25_year;
    int milestone50_year;
    int milestone75_year;
    short allowed_buildings[MAX_ALLOWED_BUILDINGS];
    struct {
        int severity;
        int year;
    } earthquake;
    struct {
        int year;
        int enabled;
    } gladiator_revolt;
    struct {
        int year;
        int enabled;
    } emperor_change;
    struct {
        int sea_trade_problem;
        int land_trade_problem;
        int raise_wages;
        int lower_wages;
        int contaminated_water;
    } random_events;
    request_t requests[MAX_REQUESTS];
    int enemy_id;
    invasion_t invasions[MAX_INVASIONS];
    price_change_t price_changes[MAX_DEMAND_CHANGES];
    demand_change_t demand_changes[MAX_DEMAND_CHANGES];

    map_point earthquake_point;
    map_point invasion_points[MAX_INVASION_POINTS];
    map_point entry_point;
    map_point exit_point;
    map_point river_entry_point;
    map_point river_exit_point;
    map_point herd_points[MAX_HERD_POINTS];
    map_point fishing_points[MAX_FISH_POINTS];

    struct {
        int hut;
        int meeting;
        int crops;
        int vacant_lots;
    } native_images;

    int is_saved;
} scenario;

extern struct scenario_settings {
    uint8_t player_name[MAX_PLAYER_NAME];
} scenario_settings;

#endif // SCENARIO_DATA_H
