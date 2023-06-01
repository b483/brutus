#ifndef SCENARIO_DATA_H
#define SCENARIO_DATA_H

#include "building/building.h"
#include "map/point.h"

#include <stdint.h>

#define MAX_PLAYER_NAME 24
#define MAX_SCENARIO_NAME 65
#define MAX_BRIEF_DESCRIPTION 32
#define MAX_BRIEFING 2500
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
    char title[MAX_CUSTOM_MESSAGE_TITLE];
    char text[MAX_CUSTOM_MESSAGE_TEXT];
    char video_file[MAX_CUSTOM_MESSAGE_VIDEO_TEXT];
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
    struct map_point_t branch_coordinates[MAX_EARTHQUAKE_BRANCHES];
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
    char scenario_name[MAX_SCENARIO_NAME];
    char brief_description[MAX_BRIEF_DESCRIPTION];
    uint8_t brief_description_image_id;
    char briefing[MAX_BRIEFING];
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
    uint8_t allowed_buildings[BUILDING_TYPE_MAX];
    struct request_t requests[MAX_REQUESTS];
    struct editor_custom_messages_t editor_custom_messages[MAX_EDITOR_CUSTOM_MESSAGES];
    struct earthquake_t earthquakes[MAX_EARTHQUAKES];
    struct invasion_t invasions[MAX_INVASIONS];
    uint8_t invasion_upcoming;
    struct price_change_t price_changes[MAX_PRICE_CHANGES];
    struct demand_change_t demand_changes[MAX_DEMAND_CHANGES];
    struct map_point_t earthquake_points[MAX_EARTHQUAKE_POINTS];
    struct map_point_t invasion_points[MAX_INVASION_POINTS];
    struct map_point_t entry_point;
    struct map_point_t exit_point;
    struct map_point_t river_entry_point;
    struct map_point_t river_exit_point;
    struct map_point_t herd_points[MAX_HERD_POINTS];
    struct map_point_t fishing_points[MAX_FISH_POINTS];
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
    char player_name[MAX_PLAYER_NAME];
};

extern struct scenario_settings_t scenario_settings;

#endif // SCENARIO_DATA_H
