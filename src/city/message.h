#ifndef CITY_MESSAGE_H
#define CITY_MESSAGE_H

#include "core/buffer.h"

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

struct city_message_t {
    int sequence;
    int message_type;
    int year;
    int month;
    int param1;
    int param2;
    int is_read;
};

void city_message_init_scenario(void);

void city_message_init_problem_areas(void);

void city_message_disable_sound_for_next_message(void);

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

#endif // CITY_MESSAGE_H
