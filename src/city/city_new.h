#ifndef CITY_CITY_NEW_H
#define CITY_CITY_NEW_H

#include "building/building.h"
#include "city/resource.h"

#define MAX_GODS 5

// TODO get rid of these
#define VIEW_X_MAX 165
#define VIEW_Y_MAX 325

#define TILE_WIDTH_PIXELS 60
#define TILE_HEIGHT_PIXELS 30
#define HALF_TILE_WIDTH_PIXELS 30
#define HALF_TILE_HEIGHT_PIXELS 15

#define MAX_WARNINGS 5

enum {
    ADVISOR_NONE = 0,
    ADVISOR_LABOR = 1,
    ADVISOR_MILITARY = 2,
    ADVISOR_IMPERIAL = 3,
    ADVISOR_RATINGS = 4,
    ADVISOR_TRADE = 5,
    ADVISOR_POPULATION = 6,
    ADVISOR_HEALTH = 7,
    ADVISOR_EDUCATION = 8,
    ADVISOR_ENTERTAINMENT = 9,
    ADVISOR_RELIGION = 10,
    ADVISOR_FINANCIAL = 11,
    ADVISOR_CHIEF = 12
};

enum {
    LOW_MOOD_CAUSE_NONE = 0,
    LOW_MOOD_CAUSE_NO_FOOD = 1,
    LOW_MOOD_CAUSE_NO_JOBS = 2,
    LOW_MOOD_CAUSE_HIGH_TAXES = 3,
    LOW_MOOD_CAUSE_LOW_WAGES = 4,
    LOW_MOOD_CAUSE_MANY_TENTS = 5,
};

enum {
    NO_IMMIGRATION_LOW_WAGES = 0,
    NO_IMMIGRATION_NO_JOBS = 1,
    NO_IMMIGRATION_NO_FOOD = 2,
    NO_IMMIGRATION_HIGH_TAXES = 3,
    NO_IMMIGRATION_MANY_TENTS = 4,
    NO_IMMIGRATION_LOW_MOOD = 5
};

enum {
    FESTIVAL_NONE = 0,
    FESTIVAL_SMALL = 1,
    FESTIVAL_LARGE = 2,
    FESTIVAL_GRAND = 3
};

enum {
    GOD_CERES = 0,
    GOD_NEPTUNE = 1,
    GOD_MERCURY = 2,
    GOD_MARS = 3,
    GOD_VENUS = 4
};

enum {
    TRADE_STATUS_NONE = 0,
    TRADE_STATUS_IMPORT = 1,
    TRADE_STATUS_EXPORT = 2
};

enum {
    GIFT_MODEST = 0,
    GIFT_GENEROUS = 1,
    GIFT_LAVISH = 2
};

struct emperor_gift_t {
    int id;
    int cost;
};

struct god_status_t {
    int8_t happiness;
    int8_t target_happiness;
    int8_t wrath_bolts;
    int8_t blessing_done;
    int8_t small_curse_done;
    int32_t months_since_festival;
};

struct finance_overview_t {
    struct {
        int taxes;
        int exports;
        int donated;
        int total;
    } income;
    struct {
        int imports;
        int wages;
        int construction;
        int interest;
        int salary;
        int sundries;
        int tribute;
        int total;
    } expenses;
    int net_in_out;
    int balance;
};

struct house_demands_t {
    struct {
        int well;
        int fountain;
        int entertainment;
        int more_entertainment;
        int education;
        int more_education;
        int religion;
        int second_religion;
        int third_religion;
        int barber;
        int bathhouse;
        int clinic;
        int hospital;
        int food;
        int second_wine;
    } missing;
    struct {
        int school;
        int library;
        int barber;
        int bathhouse;
        int clinic;
        int religion;
    } requiring;
    int health;
    int religion;
    int education;
    int entertainment;
};

struct labor_category_data_t {
    int workers_needed;
    int workers_allocated;
    int buildings;
    int priority;
    int total_houses_covered;
};

enum {
    MESSAGE_CAT_RIOT = 0,
    MESSAGE_CAT_FIRE = 1,
    MESSAGE_CAT_COLLAPSE = 2,
    MESSAGE_CAT_RIOT_COLLAPSE = 3,
    MESSAGE_CAT_BLOCKED_DOCK = 4,
    MESSAGE_CAT_WORKERS_NEEDED = 8,
    MESSAGE_CAT_NO_WORKING_DOCK = 10,
    MESSAGE_CAT_FISHING_BLOCKED = 11,
};

enum {
    MESSAGE_ADVISOR_NONE = 0,
    MESSAGE_ADVISOR_LABOR = 1,
    MESSAGE_ADVISOR_TRADE = 2,
    MESSAGE_ADVISOR_POPULATION = 3,
    MESSAGE_ADVISOR_IMPERIAL = 4,
    MESSAGE_ADVISOR_MILITARY = 5,
    MESSAGE_ADVISOR_HEALTH = 6,
    MESSAGE_ADVISOR_RELIGION = 7,
};

enum {
    MESSAGE_POPULATION_500 = 2,
    MESSAGE_POPULATION_1000 = 3,
    MESSAGE_POPULATION_2000 = 4,
    MESSAGE_POPULATION_3000 = 5,
    MESSAGE_POPULATION_5000 = 6,
    MESSAGE_POPULATION_10000 = 7,
    MESSAGE_POPULATION_15000 = 8,
    MESSAGE_POPULATION_20000 = 9,
    MESSAGE_POPULATION_25000 = 10,
    MESSAGE_RIOT = 11,
    MESSAGE_FIRE = 12,
    MESSAGE_COLLAPSED_BUILDING = 13,
    MESSAGE_DESTROYED_BUILDING = 14,
    MESSAGE_NAVIGATION_IMPOSSIBLE = 15,
    MESSAGE_CITY_IN_DEBT = 16,
    MESSAGE_CITY_IN_DEBT_AGAIN = 17,
    MESSAGE_CITY_STILL_IN_DEBT = 18,
    MESSAGE_CAESAR_WRATH = 19,
    MESSAGE_CAESAR_ARMY_CONTINUE = 20,
    MESSAGE_CAESAR_ARMY_RETREAT = 21,
    MESSAGE_LOCAL_UPRISING = 22,
    MESSAGE_BARBARIAN_ATTACK = 23,
    MESSAGE_CAESAR_ARMY_ATTACK = 24,
    MESSAGE_DISTANT_BATTLE = 25,
    MESSAGE_ENEMIES_CLOSING = 26,
    MESSAGE_ENEMIES_AT_THE_DOOR = 27,
    MESSAGE_CAESAR_REQUESTS_GOODS = 28,
    MESSAGE_CAESAR_REQUESTS_MONEY = 29,
    MESSAGE_CAESAR_REQUESTS_ARMY = 30,
    MESSAGE_REQUEST_REMINDER = 31,
    MESSAGE_REQUEST_RECEIVED = 32,
    MESSAGE_REQUEST_REFUSED = 33,
    MESSAGE_REQUEST_REFUSED_OVERDUE = 34,
    MESSAGE_REQUEST_RECEIVED_LATE = 35,
    MESSAGE_UNEMPLOYMENT = 36,
    MESSAGE_WORKERS_NEEDED = 37,
    MESSAGE_SMALL_FESTIVAL = 38,
    MESSAGE_LARGE_FESTIVAL = 39,
    MESSAGE_GRAND_FESTIVAL = 40,
    MESSAGE_WRATH_OF_CERES = 41,
    MESSAGE_WRATH_OF_NEPTUNE_NO_SEA_TRADE = 42,
    MESSAGE_WRATH_OF_MERCURY = 43,
    MESSAGE_WRATH_OF_MARS_NO_MILITARY = 44,
    MESSAGE_WRATH_OF_VENUS = 45,
    MESSAGE_PEOPLE_DISGRUNTLED = 46,
    MESSAGE_PEOPLE_UNHAPPY = 47,
    MESSAGE_PEOPLE_ANGRY = 48,
    MESSAGE_NOT_ENOUGH_FOOD = 49,
    MESSAGE_FOOD_NOT_DELIVERED = 50,
    MESSAGE_THEFT = 52,
    MESSAGE_GODS_UNHAPPY = 55,
    MESSAGE_EARTHQUAKE = 62,
    MESSAGE_GLADIATOR_REVOLT = 63,
    MESSAGE_LAND_TRADE_DISRUPTED_SANDSTORMS = 65,
    MESSAGE_SEA_TRADE_DISRUPTED = 66,
    MESSAGE_LAND_TRADE_DISRUPTED_LANDSLIDES = 67,
    MESSAGE_ROME_RAISES_WAGES = 68,
    MESSAGE_ROME_LOWERS_WAGES = 69,
    MESSAGE_CONTAMINATED_WATER = 70,
    MESSAGE_IRON_MINE_COLLAPED = 71,
    MESSAGE_CLAY_PIT_FLOODED = 72,
    MESSAGE_GLADIATOR_REVOLT_FINISHED = 73,
    MESSAGE_INCREASED_TRADING = 74,
    MESSAGE_DECREASED_TRADING = 75,
    MESSAGE_TRADE_STOPPED = 76,
    MESSAGE_PRICE_INCREASED = 78,
    MESSAGE_PRICE_DECREASED = 79,
    MESSAGE_EMPIRE_HAS_EXPANDED = 77,
    MESSAGE_ROAD_TO_ROME_BLOCKED = 80,
    MESSAGE_WRATH_OF_NEPTUNE = 81,
    MESSAGE_WRATH_OF_MARS = 82,
    MESSAGE_DISTANT_BATTLE_LOST_NO_TROOPS = 84,
    MESSAGE_DISTANT_BATTLE_LOST_TOO_LATE = 85,
    MESSAGE_DISTANT_BATTLE_LOST_TOO_WEAK = 86,
    MESSAGE_DISTANT_BATTLE_WON = 87,
    MESSAGE_TROOPS_RETURN_FAILED = 88,
    MESSAGE_TROOPS_RETURN_VICTORIOUS = 89,
    MESSAGE_DISTANT_BATTLE_CITY_RETAKEN = 90,
    MESSAGE_CERES_IS_UPSET = 91,
    MESSAGE_NEPTUNE_IS_UPSET = 92,
    MESSAGE_MERCURY_IS_UPSET = 93,
    MESSAGE_MARS_IS_UPSET = 94,
    MESSAGE_VENUS_IS_UPSET = 95,
    MESSAGE_BLESSING_FROM_CERES = 96,
    MESSAGE_BLESSING_FROM_NEPTUNE = 97,
    MESSAGE_BLESSING_FROM_MERCURY = 98,
    MESSAGE_BLESSING_FROM_MARS = 99,
    MESSAGE_BLESSING_FROM_VENUS = 100,
    MESSAGE_GODS_WRATHFUL = 101,
    MESSAGE_HEALTH_ILLNESS = 102,
    MESSAGE_HEALTH_DISEASE = 103,
    MESSAGE_HEALTH_PESTILENCE = 104,
    MESSAGE_SPIRIT_OF_MARS = 105,
    MESSAGE_CAESAR_RESPECT_1 = 106,
    MESSAGE_CAESAR_RESPECT_2 = 107,
    MESSAGE_CAESAR_RESPECT_3 = 108,
    MESSAGE_WORKING_HIPPODROME = 109,
    MESSAGE_WORKING_COLOSSEUM = 110,
    MESSAGE_EMIGRATION = 111,
    MESSAGE_FIRED = 112,
    MESSAGE_ENEMY_ARMY_ATTACK = 114,
    MESSAGE_REQUEST_CAN_COMPLY = 115,
    MESSAGE_ROAD_TO_ROME_OBSTRUCTED = 116,
    MESSAGE_NO_WORKING_DOCK = 117,
    MESSAGE_FISHING_BOAT_BLOCKED = 118,
    MESSAGE_LOCAL_UPRISING_MARS = 121,
    MESSAGE_DISTANT_BATTLE_WON_TRIUMPHAL_ARCH_DISABLED = 122,
    MESSAGE_EDITOR_1 = 123,
    MESSAGE_EDITOR_2 = 124,
    MESSAGE_EDITOR_3 = 125,
    MESSAGE_EDITOR_4 = 126,
    MESSAGE_EDITOR_5 = 127,
    MESSAGE_EDITOR_6 = 128,
    MESSAGE_EDITOR_7 = 129,
    MESSAGE_EDITOR_8 = 130,
    MESSAGE_EDITOR_9 = 131,
    MESSAGE_EDITOR_10 = 132,
    MESSAGE_EDITOR_11 = 133,
    MESSAGE_EDITOR_12 = 134,
    MESSAGE_EDITOR_13 = 135,
    MESSAGE_EDITOR_14 = 136,
    MESSAGE_EDITOR_15 = 137,
    MESSAGE_EDITOR_16 = 138,
    MESSAGE_EDITOR_17 = 139,
    MESSAGE_EDITOR_18 = 140,
    MESSAGE_EDITOR_19 = 141,
    MESSAGE_EDITOR_20 = 142,
    MESSAGE_EDITOR_21 = 143,
    MESSAGE_EDITOR_22 = 144,
    MESSAGE_EDITOR_23 = 145,
    MESSAGE_EDITOR_24 = 146,
    MESSAGE_EDITOR_25 = 147,
    MESSAGE_EDITOR_26 = 148,
    MESSAGE_EDITOR_27 = 149,
    MESSAGE_EDITOR_28 = 150,
    MESSAGE_EDITOR_29 = 151,
    MESSAGE_EDITOR_30 = 152,
    MESSAGE_EDITOR_31 = 153,
    MESSAGE_EDITOR_32 = 154,
    MESSAGE_EDITOR_33 = 155,
    MESSAGE_EDITOR_34 = 156,
    MESSAGE_EDITOR_35 = 157,
    MESSAGE_EDITOR_36 = 158,
    MESSAGE_EDITOR_37 = 159,
    MESSAGE_EDITOR_38 = 160,
    MESSAGE_EDITOR_39 = 161,
    MESSAGE_EDITOR_40 = 162,
    MESSAGE_EDITOR_41 = 163,
    MESSAGE_EDITOR_42 = 164,
    MESSAGE_EDITOR_43 = 165,
    MESSAGE_EDITOR_44 = 166,
    MESSAGE_EDITOR_45 = 167,
};

enum {
    SELECTED_RATING_NONE = 0,
    SELECTED_RATING_CULTURE = 1,
    SELECTED_RATING_PROSPERITY = 2,
    SELECTED_RATING_PEACE = 3,
    SELECTED_RATING_FAVOR = 4
};

enum {
    VICTORY_STATE_LOST = -1,
    VICTORY_STATE_NONE = 0,
    VICTORY_STATE_WON = 1
};

enum {
    WARNING_ORIENTATION = 1,
    WARNING_CLEAR_LAND_NEEDED = 2,
    WARNING_OUT_OF_MONEY = 3,
    WARNING_ONE_BUILDING_OF_TYPE = 4,
    WARNING_ROAD_ACCESS_NEEDED = 11,
    WARNING_NOT_AVAILABLE = 13,
    WARNING_NOT_AVAILABLE_YET = 14,
    WARNING_MARBLE_NEEDED_LARGE_TEMPLE = 15,
    WARNING_MARBLE_NEEDED_ORACLE = 16,
    WARNING_WORKERS_NEEDED = 17,
    WARNING_MORE_FOOD_NEEDED = 18,
    WARNING_BUILD_MARKET = 19,
    WARNING_MEADOW_NEEDED = 20,
    WARNING_WATER_NEEDED = 21,
    WARNING_ROCK_NEEDED = 22,
    WARNING_TREE_NEEDED = 23,
    WARNING_SHORE_NEEDED = 25,
    WARNING_IRON_NEEDED = 26,
    WARNING_VINES_NEEDED = 27,
    WARNING_OLIVES_NEEDED = 28,
    WARNING_CLAY_NEEDED = 29,
    WARNING_TIMBER_NEEDED = 30,
    WARNING_OPEN_TRADE_TO_IMPORT = 31,
    WARNING_TRADE_IMPORT_RESOURCE = 32,
    WARNING_BUILD_IRON_MINE = 33,
    WARNING_BUILD_VINES_FARM = 34,
    WARNING_BUILD_OLIVE_FARM = 35,
    WARNING_BUILD_CLAY_PIT = 36,
    WARNING_BUILD_TIMBER_YARD = 37,
    WARNING_WATER_PIPE_ACCESS_NEEDED = 38,
    WARNING_PLACE_RESERVOIR_NEXT_TO_WATER = 39,
    WARNING_CONNECT_TO_RESERVOIR = 40,
    WARNING_SENTRIES_NEED_WALL = 41,
    WARNING_BUILD_BARRACKS = 42,
    WARNING_WEAPONS_NEEDED = 43,
    WARNING_BUILD_ACTOR_COLONY = 44,
    WARNING_BUILD_GLADIATOR_SCHOOL = 45,
    WARNING_BUILD_LION_HOUSE = 46,
    WARNING_BUILD_CHARIOT_MAKER = 47,
    WARNING_WALL_NEEDED = 49,
    WARNING_ENEMY_NEARBY = 50,
    WARNING_LEGION_MORALE_TOO_LOW = 51,
    WARNING_MAX_LEGIONS_REACHED = 52,
    WARNING_PEOPLE_ON_BRIDGE = 55,
    WARNING_DOCK_OPEN_WATER_NEEDED = 56,
    WARNING_EDITOR_NEED_MAP_EDGE = 58,
    WARNING_EDITOR_NEED_OPEN_WATER = 59,
    WARNING_EDITOR_CANNOT_PLACE = 60,
    WARNING_HOUSE_TOO_FAR_FROM_ROAD = 62,
    WARNING_CITY_BOXED_IN = 63,
    WARNING_CITY_BOXED_IN_PEOPLE_WILL_PERISH = 64,
    WARNING_DATA_LIMIT_REACHED = 65
};

struct city_message_t {
    int sequence;
    int message_type;
    int year;
    int month;
    int param1;
    int param2;
    int is_read;
};

struct pixel_view_coordinates_t {
    int x;
    int y;
};

extern struct city_data_t {
    struct {
        int16_t senate_placed;
        uint8_t senate_x;
        uint8_t senate_y;
        int16_t senate_grid_offset;
        int32_t senate_building_id;
        int32_t hippodrome_placed;
        int8_t barracks_x;
        int8_t barracks_y;
        int16_t barracks_grid_offset;
        int32_t barracks_building_id;
        int32_t barracks_placed;
        int32_t trade_center_building_id;
        int8_t triumphal_arches_available;
        int16_t working_wharfs;
        int32_t shipyard_boats_requested;
        int16_t working_docks;
        int16_t working_dock_ids[10];
        int32_t mission_post_operational;
        struct map_point_t main_native_meeting;
    } building;
    struct {
        int16_t animals;
        int32_t enemies;
        int32_t imperial_soldiers;
        int32_t rioters;
        int32_t soldiers;
    } figure;
    struct house_demands_t houses;
    struct {
        struct emperor_gift_t gifts[3];
        int32_t months_since_gift;
        int32_t gift_overdose_penalty;

        int32_t debt_state;
        int32_t months_in_debt;

        int32_t player_rank;
        int32_t salary_rank;
        int32_t salary_amount;
        int32_t donate_amount;
        int32_t personal_savings;
        struct {
            int32_t count;
            int32_t size;
            int32_t soldiers_killed;
            int32_t warnings_given;
            int32_t days_until_invasion;
            int32_t duration_day_countdown;
            int32_t retreat_message_shown;
            int16_t from_editor;
        } invasion;
    } emperor;
    struct {
        uint8_t total_legions;
        int32_t legionary_legions;
        int32_t native_attack_duration;
    } military;
    struct {
        uint8_t city;
        int8_t city_foreign_months_left;
        int8_t total_count;
        int8_t won_count;
        uint8_t enemy_strength;
        uint8_t roman_strength;
        int8_t months_until_battle;
        int8_t roman_months_to_travel_forth;
        int8_t roman_months_to_travel_back;
        int8_t enemy_months_traveled;
        int8_t roman_months_traveled;
    } distant_battle;
    struct {
        int32_t treasury;
        int32_t tax_percentage;
        int32_t estimated_tax_income;
        int32_t estimated_wages;
        struct finance_overview_t last_year;
        struct finance_overview_t this_year;
        int32_t interest_so_far;
        int32_t salary_so_far;
        int32_t wages_so_far;
        int16_t stolen_this_year;
        int16_t stolen_last_year;
        int32_t cheated_money;
        int32_t tribute_not_paid_last_year;
        int32_t tribute_not_paid_total_years;
        int32_t wage_rate_paid_this_year;
        int32_t wage_rate_paid_last_year;
    } finance;
    struct {
        int32_t taxed_plebs;
        int32_t taxed_patricians;
        int32_t untaxed_plebs;
        int32_t untaxed_patricians;
        int32_t percentage_taxed_plebs;
        int32_t percentage_taxed_patricians;
        int32_t percentage_taxed_people;
        struct {
            int32_t collected_plebs;
            int32_t collected_patricians;
            int32_t uncollected_plebs;
            int32_t uncollected_patricians;
        } yearly;
        struct {
            int32_t collected_plebs;
            int32_t collected_patricians;
            int32_t uncollected_plebs;
            int32_t uncollected_patricians;
        } monthly;
    } taxes;
    struct {
        uint32_t population;
        int32_t population_last_year;
        int32_t school_age;
        int32_t academy_age;
        int32_t working_age;
        struct {
            int32_t values[2400];
            int32_t next_index;
            int32_t count;
        } monthly;
        int16_t at_age[100];
        int32_t at_level[20];

        int32_t yearly_update_requested;
        int32_t yearly_births;
        int32_t yearly_deaths;
        int32_t lost_removal;
        int32_t lost_homeless;
        int32_t lost_troop_request;
        int32_t last_change;
        int32_t total_all_years;
        int32_t total_years;
        int32_t average_per_year;
        uint32_t highest_ever;
        int32_t total_capacity;
        int32_t room_in_houses;

        int32_t people_in_tents;
        int32_t people_in_tents_shacks;
        int32_t people_in_large_insula_and_above;
        int32_t people_in_villas_palaces;
        int32_t percentage_plebs;

        int32_t last_used_house_add;
        int32_t last_used_house_remove;
        int32_t graph_order;
    } population;
    struct {
        int32_t wages;
        int32_t wages_rome;
        int32_t workers_available;
        int32_t workers_employed;
        int32_t workers_unemployed;
        int32_t workers_needed;
        int32_t unemployment_percentage;
        int32_t unemployment_percentage_for_senate;
        struct labor_category_data_t categories[10];
    } labor;
    struct {
        int32_t immigration_duration;
        int32_t emigration_duration;
        int32_t immigration_amount_per_batch;
        int32_t emigration_amount_per_batch;
        int32_t immigration_queue_size;
        int32_t emigration_queue_size;
        int32_t immigrated_today;
        int32_t emigrated_today;
        int32_t refused_immigrants_today;
        int32_t no_immigration_cause;
        int32_t percentage;
        int32_t newcomers;
        int32_t emigration_message_shown;
    } migration;
    struct {
        int32_t value;
        int32_t previous_value;
        int32_t message_delay;
        int32_t low_mood_cause;
        int32_t protesters;
        int32_t criminals; // muggers+rioters
    } sentiment;
    struct {
        int32_t num_hospital_workers;
        int32_t target_value;
        int32_t value;
    } health;
    struct {
        uint32_t culture;
        uint32_t prosperity;
        uint32_t peace;
        uint32_t favor;
        struct {
            int32_t library;
        } culture_points;
        int32_t prosperity_treasury_last_year;
        uint32_t prosperity_max;
        int32_t peace_destroyed_buildings;
        int32_t peace_years_of_peace;
        int32_t peace_num_criminals;
        int32_t peace_num_rioters;
        int32_t peace_riot_cause;
        int32_t favor_salary_penalty;
        int32_t favor_ignored_request_penalty;
        uint32_t favor_last_year;
        int32_t favor_change; // 0 = dropping, 1 = stalling, 2 = rising

        int32_t selected;
        int32_t culture_explanation;
        int32_t prosperity_explanation;
        int32_t peace_explanation;
        int32_t favor_explanation;
    } ratings;
    struct {
        int32_t average_entertainment;
        int32_t average_religion;
        int32_t average_education;
        int32_t average_health;
        int32_t religion_coverage;
    } culture;
    struct {
        struct god_status_t gods[5];
        int32_t least_happy_god;
        int32_t angry_message_delay;
        int32_t venus_curse_active;
        int32_t neptune_double_trade_active;
        int32_t neptune_sank_ships;
        int32_t mars_spirit_power;
    } religion;
    struct {
        int32_t theater_shows;
        int32_t theater_no_shows_weighted;
        int32_t amphitheater_shows;
        int32_t amphitheater_no_shows_weighted;
        int32_t colosseum_shows;
        int32_t colosseum_no_shows_weighted;
        int32_t hippodrome_shows;
        int32_t hippodrome_no_shows_weighted;
        int32_t venue_needing_shows;
        int32_t hippodrome_has_race;
        int32_t hippodrome_message_shown;
        int32_t colosseum_message_shown;
    } entertainment;
    struct {
        int8_t god;
        int8_t size;
        int32_t cost;
        int32_t months_to_go;
        int32_t months_since_festival;
        int32_t first_festival_effect_months;
        int32_t second_festival_effect_months;
    } festival;
    struct {
        int16_t space_in_warehouses[RESOURCE_TYPES_MAX];
        uint16_t stored_in_warehouses[RESOURCE_TYPES_MAX];
        int32_t space_in_workshops[6];
        int32_t stored_in_workshops[6];
        int16_t trade_status[RESOURCE_TYPES_MAX];
        int16_t export_over[RESOURCE_TYPES_MAX];
        int32_t stockpiled[RESOURCE_TYPES_MAX];
        int16_t mothballed[RESOURCE_TYPES_MAX];
        int32_t wine_types_available;
        int32_t food_types_available;
        int32_t food_types_eaten;
        int32_t granary_food_stored[FOOD_TYPES_MAX];
        int32_t granary_total_stored;
        int32_t food_supply_months;
        int32_t food_needed_per_month;
        int32_t food_consumed_last_month;
        int32_t food_produced_last_month;
        int32_t food_produced_this_month;
        struct {
            int operating;
            int not_operating;
            int not_operating_with_food;
            int understaffed;
        } granaries;
        int16_t last_used_warehouse;
    } resource;
    struct {
        uint8_t hit_elephant;
        uint8_t die_citizen;
        uint8_t die_soldier;
    } sound;
    struct {
        int16_t num_land_routes;
        int16_t num_sea_routes;
        int16_t land_trade_problem_duration;
        int16_t sea_trade_problem_duration;
        int32_t caravan_import_resource;
        int32_t caravan_backup_import_resource;
        int32_t docker_import_resource;
        int32_t docker_export_resource;
    } trade;
    struct {
        struct {
            int32_t id;
            int32_t size;
        } largest_road_networks[10];
    } map;
    struct {
        int32_t has_won;
        int32_t continue_months_left;
        int32_t continue_months_chosen;
        int32_t fired_message_shown;
        int32_t victory_message_shown;
    } mission;
} city_data;

void city_culture_update_coverage(void);

int city_culture_coverage_theater(void);
int city_culture_coverage_amphitheater(void);
int city_culture_coverage_colosseum(void);
int city_culture_coverage_hippodrome(void);
int city_culture_coverage_average_entertainment(void);

int city_culture_coverage_religion(int god);

int city_culture_coverage_school(void);
int city_culture_coverage_library(void);
int city_culture_coverage_academy(void);

int city_culture_coverage_hospital(void);

void city_culture_calculate(void);

void city_culture_save_state(struct buffer_t *buf);

void city_culture_load_state(struct buffer_t *buf);

void city_data_init(void);

void city_data_init_scenario(void);

void city_data_save_state(struct buffer_t *main, struct buffer_t *graph_order);

void city_data_load_state(struct buffer_t *main, struct buffer_t *graph_order);

int city_emperor_salary_for_rank(int rank);

void city_emperor_set_salary_rank(int player_rank);

int city_finance_can_afford(int building_cost);

void city_finance_process_export(int price);

void city_finance_process_donation(int amount);

void city_finance_process_misc(int cost);

void city_finance_process_construction(int cost);

void city_finance_calculate_totals(void);

void city_finance_estimate_wages(void);

void city_finance_estimate_taxes(void);

void city_gods_calculate_moods(int update_moods);

int city_gods_calculate_least_happy(void);

void city_health_update(void);

void city_labor_calculate_workers(int num_plebs, int num_patricians);

void city_labor_allocate_workers(void);

void city_labor_update(void);

void city_labor_set_priority(int category, int new_priority);

void city_message_init_scenario(void);

void city_message_init_problem_areas(void);

void city_message_apply_sound_interval(int category);

void city_message_post(int use_popup, int message_type, int param1, int param2);

void city_message_post_with_popup_delay(int category, int message_type, int param1, short param2);

void city_message_post_with_message_delay(int category, int use_popup, int message_type, int delay);

void city_message_process_queue(void);

void city_message_sort_and_compact(void);

int city_message_get_text_id(int message_type);

int city_message_get_advisor(int message_type);

void city_message_reset_category_count(int category);

void city_message_increase_category_count(int category);

int city_message_get_category_count(int category);

void city_message_decrease_delays(void);

int city_message_mark_population_shown(int population);

struct city_message_t *city_message_get(int message_id);

int city_message_set_current(int message_id);

void city_message_mark_read(int message_id);

void city_message_delete(int message_id);

int city_message_count(void);

int city_message_problem_area_count(void);

int city_message_next_problem_area_grid_offset(void);

void city_message_clear_scroll(void);

int city_message_scroll_position(void);

void city_message_set_scroll_position(int scroll_position);

void city_message_save_state(struct buffer_t *messages, struct buffer_t *extra, struct buffer_t *counts, struct buffer_t *delays, struct buffer_t *population);

void city_message_load_state(struct buffer_t *messages, struct buffer_t *extra, struct buffer_t *counts, struct buffer_t *delays, struct buffer_t *population);

void city_migration_update(void);

int city_migration_no_room_for_immigrants(void);

void city_military_determine_distant_battle_city(void);

int city_military_distant_battle_roman_army_is_traveling(void);

void city_military_process_distant_battle(void);

/**
 * Add people to the city.
 * @param num_people Number of people to add
 */
void city_population_add(int num_people);

/**
 * Add people returning after becoming homeless.
 * @param num_people Number of people to add
 */
void city_population_add_homeless(int num_people);

void city_population_remove_homeless(int num_people);

void city_population_remove_home_removed(int num_people);

void city_population_remove_for_troop_request(int num_people);

void city_population_request_yearly_update(void);

void city_population_yearly_update(void);

void city_ratings_update_favor_explanation(void);

void city_ratings_update_explanations(void);

void city_ratings_update(int is_yearly_update);

void city_sentiment_change_happiness(int amount);

void city_sentiment_update(void);

void city_trade_update(void);

int city_trade_next_caravan_import_resource(void);
int city_trade_next_caravan_backup_import_resource(void);

int city_trade_next_docker_import_resource(void);
int city_trade_next_docker_export_resource(void);

void city_victory_reset(void);

void city_victory_force_win(void);

int city_victory_state(void);

void city_victory_check(void);

void city_victory_update_months_to_govern(void);

void city_victory_continue_governing(int months);

void city_victory_stop_governing(void);

typedef void (map_callback)(int x, int y, int grid_offset);

void city_view_init(void);

int city_view_orientation(void);

void city_view_reset_orientation(void);

void city_view_get_camera(int *x, int *y);
void city_view_get_pixel_offset(int *x, int *y);
void city_view_get_camera_in_pixels(int *x, int *y);

void city_view_set_camera(int x, int y);

void city_view_set_camera_from_pixel_position(int x, int y);

void city_view_scroll(int x, int y);

void city_view_grid_offset_to_xy_view(int grid_offset, int *x_view, int *y_view);

void city_view_get_selected_tile_pixels(int *x_pixels, int *y_pixels);

int city_view_pixels_to_view_tile(int x_pixels, int y_pixels, struct pixel_view_coordinates_t *tile);

void city_view_set_selected_view_tile(const struct pixel_view_coordinates_t *tile);

int city_view_tile_to_grid_offset(const struct pixel_view_coordinates_t *tile);

void city_view_go_to_grid_offset(int grid_offset);

void city_view_rotate_left(void);

void city_view_rotate_right(void);

void city_view_set_viewport(int screen_width, int screen_height);

void city_view_get_viewport(int *x, int *y, int *width, int *height);
void city_view_get_viewport_size_tiles(int *width, int *height);

int city_view_is_sidebar_collapsed(void);

void city_view_start_sidebar_toggle(void);

void city_view_toggle_sidebar(void);

void city_view_save_state(struct buffer_t *orientation, struct buffer_t *camera);

void city_view_load_state(struct buffer_t *orientation, struct buffer_t *camera);

void city_view_save_scenario_state(struct buffer_t *camera);

void city_view_load_scenario_state(struct buffer_t *camera);

void city_view_foreach_map_tile(map_callback *callback);

void city_view_foreach_valid_map_tile(map_callback *callback1, map_callback *callback2, map_callback *callback3);

void city_view_foreach_minimap_tile(
    int x_offset, int y_offset, int absolute_x, int absolute_y,
    int width_tiles, int height_tiles, map_callback *callback);

void city_warning_show(int type);
void city_warning_show_custom(const char *text);

int city_has_warnings(void);

const char *city_warning_get(int id);

void city_warning_clear_all(void);
void city_warning_clear_outdated(void);

#endif