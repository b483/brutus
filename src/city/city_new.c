#include "city_new.h"

#include "city/city_new.h"
#include "core/calc.h"
#include "core/file.h"
#include "core/image.h"
#include "core/lang.h"
#include "core/random.h"
#include "core/string.h"
#include "core/time.h"
#include "empire/empire.h"
#include "figure/formation_enemy.h"
#include "figure/formation_legion.h"
#include "figure/route.h"
#include "figuretype/figuretype.h"
#include "graphics/graphics.h"
#include "scenario/scenario.h"
#include "sound/sound.h"
#include "window/window.h"

#include <string.h>

#define TIE 10
#define CURSE_LOADS 16
#define MARS_INVASION_SMALL 16
#define MARS_INVASION_LARGE 32
#define MAX_TEXT 100
#define TIMEOUT_MS 15000
#define MAX_MESSAGES 1000
#define MAX_QUEUE 20
#define MAX_MESSAGE_CATEGORIES 20
#define LABOR_CATEGORIES_COUNT 9

enum {
    LABOR_CATEGORY_INDUSTRY_COMMERCE = 0,
    LABOR_CATEGORY_FOOD_PRODUCTION = 1,
    LABOR_CATEGORY_ENGINEERING = 2,
    LABOR_CATEGORY_WATER = 3,
    LABOR_CATEGORY_PREFECTURES = 4,
    LABOR_CATEGORY_MILITARY = 5,
    LABOR_CATEGORY_ENTERTAINMENT = 6,
    LABOR_CATEGORY_HEALTH_EDUCATION = 7,
    LABOR_CATEGORY_GOVERNANCE_RELIGION = 8
};

static struct {
    int category;
    int workers;
} DEFAULT_PRIORITY[LABOR_CATEGORIES_COUNT] = {
    {LABOR_CATEGORY_ENGINEERING, 3},
    {LABOR_CATEGORY_WATER, 1},
    {LABOR_CATEGORY_PREFECTURES, 3},
    {LABOR_CATEGORY_MILITARY, 2},
    {LABOR_CATEGORY_FOOD_PRODUCTION, 4},
    {LABOR_CATEGORY_INDUSTRY_COMMERCE, 2},
    {LABOR_CATEGORY_ENTERTAINMENT, 1},
    {LABOR_CATEGORY_HEALTH_EDUCATION, 1},
    {LABOR_CATEGORY_GOVERNANCE_RELIGION, 1},
};

struct city_data_t city_data;

static const int SALARY_FOR_RANK[11] = { 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 };

static struct {
    struct city_message_t messages[MAX_MESSAGES];
    int queue[20];
    int consecutive_message_delay;
    int next_message_sequence;
    int total_messages;
    int current_message_id;
    struct {
        int pop500;
        int pop1000;
        int pop2000;
        int pop3000;
        int pop5000;
        int pop10000;
        int pop15000;
        int pop20000;
        int pop25000;
    } population_shown;
    int message_count[MAX_MESSAGE_CATEGORIES];
    int message_delay[MAX_MESSAGE_CATEGORIES];
    uint32_t last_sound_time[MESSAGE_CAT_RIOT_COLLAPSE + 1];
    int problem_count;
    int problem_index;
    uint32_t problem_last_click_time;
    int scroll_position;
} message_data;


static int should_play_sound = 1;

static struct {
    int state;
    int force_win;
} victory_data;

static struct {
    int screen_width;
    int screen_height;
    int sidebar_collapsed;
    int orientation;
    struct {
        struct pixel_view_coordinates_t tile;
        struct pixel_view_coordinates_t pixel;
    } camera;
    struct {
        int x;
        int y;
        int width_pixels;
        int height_pixels;
        int width_tiles;
        int height_tiles;
    } viewport;
    struct {
        int x_pixels;
        int y_pixels;
    } selected_tile;
} view_data;

static int view_to_grid_offset_lookup[VIEW_X_MAX][VIEW_Y_MAX];

struct warning {
    int in_use;
    uint32_t time;
    char text[MAX_TEXT];
};

static struct warning warnings[MAX_WARNINGS];

static const int SENTIMENT_PER_TAX_RATE[26] = {
    3, 2, 2, 2, 1, 1, 1, 0, 0, -1,
    -2, -2, -3, -3, -3, -5, -5, -5, -5, -6,
    -6, -6, -6, -6, -6, -6
};

static const int BIRTHS_PER_AGE_DECENNIUM[10] = {
    0, 3, 16, 9, 2, 0, 0, 0, 0, 0
};

static const int DEATHS_PER_HEALTH_PER_AGE_DECENNIUM[11][10] = {
    {20, 10, 5, 10, 20, 30, 50, 85, 100, 100},
    {15, 8, 4, 8, 16, 25, 45, 70, 90, 100},
    {10, 6, 2, 6, 12, 20, 30, 55, 80, 90},
    {5, 4, 0, 4, 8, 15, 25, 40, 65, 80},
    {3, 2, 0, 2, 6, 12, 20, 30, 50, 70},
    {2, 0, 0, 0, 4, 8, 15, 25, 40, 60},
    {1, 0, 0, 0, 2, 6, 12, 20, 30, 50},
    {0, 0, 0, 0, 0, 4, 8, 15, 20, 40},
    {0, 0, 0, 0, 0, 2, 6, 10, 15, 30},
    {0, 0, 0, 0, 0, 0, 4, 5, 10, 20},
    {0, 0, 0, 0, 0, 0, 0, 2, 5, 10}
};

static struct {
    int theater;
    int amphitheater;
    int colosseum;
    int hippodrome;
    int hospital;
    int school;
    int academy;
    int library;
    int religion[5];
    int oracle;
} culture_coverage;

int city_culture_coverage_theater(void)
{
    return culture_coverage.theater;
}

int city_culture_coverage_amphitheater(void)
{
    return culture_coverage.amphitheater;
}

int city_culture_coverage_colosseum(void)
{
    return culture_coverage.colosseum;
}

int city_culture_coverage_hippodrome(void)
{
    return culture_coverage.hippodrome;
}

int city_culture_coverage_average_entertainment(void)
{
    return (culture_coverage.hippodrome + culture_coverage.colosseum + culture_coverage.amphitheater + culture_coverage.theater) / 4;
}

int city_culture_coverage_religion(int god)
{
    return culture_coverage.religion[god];
}

int city_culture_coverage_school(void)
{
    return culture_coverage.school;
}

int city_culture_coverage_library(void)
{
    return culture_coverage.library;
}

int city_culture_coverage_academy(void)
{
    return culture_coverage.academy;
}

int city_culture_coverage_hospital(void)
{
    return culture_coverage.hospital;
}

static int cap_input_at_100(int input)
{
    return input > 100 ? 100 : input;
}

static int get_people_aged_between(int min, int max)
{
    int pop = 0;
    for (int i = min; i < max; i++) {
        pop += city_data.population.at_age[i];
    }
    return pop;
}

void city_culture_update_coverage(void)
{
    int population = city_data.population.population;

    // entertainment
    culture_coverage.theater = cap_input_at_100(calc_percentage(500 * building_count_active(BUILDING_THEATER), population));
    culture_coverage.amphitheater = cap_input_at_100(calc_percentage(800 * building_count_active(BUILDING_AMPHITHEATER), population));
    culture_coverage.colosseum = cap_input_at_100(calc_percentage(1500 * building_count_active(BUILDING_COLOSSEUM), population));
    if (building_count_active(BUILDING_HIPPODROME) <= 0) {
        culture_coverage.hippodrome = 0;
    } else {
        culture_coverage.hippodrome = 100;
    }

    // religion
    int oracles = building_count_total(BUILDING_ORACLE);
    culture_coverage.religion[GOD_CERES] = cap_input_at_100(calc_percentage(
        500 * oracles +
        750 * building_count_active(BUILDING_SMALL_TEMPLE_CERES) +
        1500 * building_count_active(BUILDING_LARGE_TEMPLE_CERES),
        population));
    culture_coverage.religion[GOD_NEPTUNE] = cap_input_at_100(calc_percentage(
        500 * oracles +
        750 * building_count_active(BUILDING_SMALL_TEMPLE_NEPTUNE) +
        1500 * building_count_active(BUILDING_LARGE_TEMPLE_NEPTUNE),
        population));
    culture_coverage.religion[GOD_MERCURY] = cap_input_at_100(calc_percentage(
        500 * oracles +
        750 * building_count_active(BUILDING_SMALL_TEMPLE_MERCURY) +
        1500 * building_count_active(BUILDING_LARGE_TEMPLE_MERCURY),
        population));
    culture_coverage.religion[GOD_MARS] = cap_input_at_100(calc_percentage(
        500 * oracles +
        750 * building_count_active(BUILDING_SMALL_TEMPLE_MARS) +
        1500 * building_count_active(BUILDING_LARGE_TEMPLE_MARS),
        population));
    culture_coverage.religion[GOD_VENUS] = cap_input_at_100(calc_percentage(
        500 * oracles +
        750 * building_count_active(BUILDING_SMALL_TEMPLE_VENUS) +
        1500 * building_count_active(BUILDING_LARGE_TEMPLE_VENUS),
        population));
    culture_coverage.oracle = cap_input_at_100(calc_percentage(500 * oracles, population));

    city_data.culture.religion_coverage =
        culture_coverage.religion[GOD_CERES] +
        culture_coverage.religion[GOD_NEPTUNE] +
        culture_coverage.religion[GOD_MERCURY] +
        culture_coverage.religion[GOD_MARS] +
        culture_coverage.religion[GOD_VENUS];
    city_data.culture.religion_coverage /= 5;

    // education
    city_data.population.school_age = get_people_aged_between(0, 14);
    city_data.population.academy_age = get_people_aged_between(14, 21);

    culture_coverage.school = cap_input_at_100(calc_percentage(
        75 * building_count_active(BUILDING_SCHOOL), city_data.population.school_age));
    culture_coverage.library = cap_input_at_100(calc_percentage(
        800 * building_count_active(BUILDING_LIBRARY), population));
    culture_coverage.academy = cap_input_at_100(calc_percentage(
        100 * building_count_active(BUILDING_ACADEMY), city_data.population.academy_age));

    // health
    culture_coverage.hospital = cap_input_at_100(calc_percentage(
        1000 * building_count_active(BUILDING_HOSPITAL), population));
}

void city_culture_calculate(void)
{
    city_data.culture.average_entertainment = 0;
    city_data.culture.average_religion = 0;
    city_data.culture.average_education = 0;
    city_data.culture.average_health = 0;

    int num_houses = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size) {
            num_houses++;
            city_data.culture.average_entertainment += b->data.house.entertainment;
            city_data.culture.average_religion += b->data.house.num_gods;
            city_data.culture.average_education += b->data.house.education;
            city_data.culture.average_health += b->data.house.health;
        }
    }
    if (num_houses) {
        city_data.culture.average_entertainment /= num_houses;
        city_data.culture.average_religion /= num_houses;
        city_data.culture.average_education /= num_houses;
        city_data.culture.average_health /= num_houses;
    }
    // calculate entertainment shows
    city_data.entertainment.theater_shows = 0;
    city_data.entertainment.theater_no_shows_weighted = 0;
    city_data.entertainment.amphitheater_shows = 0;
    city_data.entertainment.amphitheater_no_shows_weighted = 0;
    city_data.entertainment.colosseum_shows = 0;
    city_data.entertainment.colosseum_no_shows_weighted = 0;
    city_data.entertainment.hippodrome_shows = 0;
    city_data.entertainment.hippodrome_no_shows_weighted = 0;
    city_data.entertainment.venue_needing_shows = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        switch (b->type) {
            case BUILDING_THEATER:
                if (b->data.entertainment.days1) {
                    city_data.entertainment.theater_shows++;
                } else {
                    city_data.entertainment.theater_no_shows_weighted++;
                }
                break;
            case BUILDING_AMPHITHEATER:
                if (b->data.entertainment.days1) {
                    city_data.entertainment.amphitheater_shows++;
                } else {
                    city_data.entertainment.amphitheater_no_shows_weighted += 2;
                }
                if (b->data.entertainment.days2) {
                    city_data.entertainment.amphitheater_shows++;
                } else {
                    city_data.entertainment.amphitheater_no_shows_weighted += 2;
                }
                break;
            case BUILDING_COLOSSEUM:
                if (b->data.entertainment.days1) {
                    city_data.entertainment.colosseum_shows++;
                } else {
                    city_data.entertainment.colosseum_no_shows_weighted += 3;
                }
                if (b->data.entertainment.days2) {
                    city_data.entertainment.colosseum_shows++;
                } else {
                    city_data.entertainment.colosseum_no_shows_weighted += 3;
                }
                break;
            case BUILDING_HIPPODROME:
                if (b->data.entertainment.days1) {
                    city_data.entertainment.hippodrome_shows++;
                } else {
                    city_data.entertainment.hippodrome_no_shows_weighted += 100;
                }
                break;
        }
    }
    int worst_shows = 0;
    if (city_data.entertainment.theater_no_shows_weighted > worst_shows) {
        worst_shows = city_data.entertainment.theater_no_shows_weighted;
        city_data.entertainment.venue_needing_shows = 1;
    }
    if (city_data.entertainment.amphitheater_no_shows_weighted > worst_shows) {
        worst_shows = city_data.entertainment.amphitheater_no_shows_weighted;
        city_data.entertainment.venue_needing_shows = 2;
    }
    if (city_data.entertainment.colosseum_no_shows_weighted > worst_shows) {
        worst_shows = city_data.entertainment.colosseum_no_shows_weighted;
        city_data.entertainment.venue_needing_shows = 3;
    }
    if (city_data.entertainment.hippodrome_no_shows_weighted > worst_shows) {
        city_data.entertainment.venue_needing_shows = 4;
    }
}

void city_culture_save_state(struct buffer_t *buf)
{
    buffer_write_i32(buf, culture_coverage.theater);
    buffer_write_i32(buf, culture_coverage.amphitheater);
    buffer_write_i32(buf, culture_coverage.colosseum);
    buffer_write_i32(buf, culture_coverage.hippodrome);
    for (int i = GOD_CERES; i <= GOD_VENUS; i++) {
        buffer_write_i32(buf, culture_coverage.religion[i]);
    }
    buffer_write_i32(buf, culture_coverage.oracle);
    buffer_write_i32(buf, culture_coverage.school);
    buffer_write_i32(buf, culture_coverage.library);
    buffer_write_i32(buf, culture_coverage.academy);
    buffer_write_i32(buf, culture_coverage.hospital);
}

void city_culture_load_state(struct buffer_t *buf)
{
    culture_coverage.theater = buffer_read_i32(buf);
    culture_coverage.amphitheater = buffer_read_i32(buf);
    culture_coverage.colosseum = buffer_read_i32(buf);
    culture_coverage.hippodrome = buffer_read_i32(buf);
    for (int i = GOD_CERES; i <= GOD_VENUS; i++) {
        culture_coverage.religion[i] = buffer_read_i32(buf);
    }
    culture_coverage.oracle = buffer_read_i32(buf);
    culture_coverage.school = buffer_read_i32(buf);
    culture_coverage.library = buffer_read_i32(buf);
    culture_coverage.academy = buffer_read_i32(buf);
    culture_coverage.hospital = buffer_read_i32(buf);
}

void city_data_init(void)
{
    memset(&city_data, 0, sizeof(struct city_data_t));

    city_data.sentiment.value = 60;
    city_data.health.target_value = 50;
    city_data.health.value = 50;
    city_data.labor.wages_rome = 30;
    city_data.labor.wages = 30;
    city_data.finance.tax_percentage = 7;
    city_data.trade.caravan_import_resource = RESOURCE_WHEAT;
    city_data.trade.caravan_backup_import_resource = RESOURCE_WHEAT;
    city_data.population.monthly.next_index = 0;
    city_data.population.monthly.count = 0;
    city_data.festival.months_since_festival = 1;
    city_data.festival.size = FESTIVAL_NONE;
    city_data.emperor.gifts[GIFT_MODEST].cost = 0;
    city_data.emperor.gifts[GIFT_GENEROUS].cost = 0;
    city_data.emperor.gifts[GIFT_LAVISH].cost = 0;

    for (int i = 0; i < MAX_GODS; i++) {
        city_data.religion.gods[i].target_happiness = 50;
        city_data.religion.gods[i].happiness = 50;
        city_data.religion.gods[i].wrath_bolts = 0;
        city_data.religion.gods[i].blessing_done = 0;
        city_data.religion.gods[i].small_curse_done = 0;
        city_data.religion.gods[i].months_since_festival = 0;
    }
    city_data.religion.angry_message_delay = 0;
}

void city_data_init_scenario(void)
{
    city_data.finance.treasury = scenario.initial_funds;
    city_data.finance.last_year.balance = city_data.finance.treasury;
}

void city_data_save_state(struct buffer_t *main, struct buffer_t *graph_order)
{
    buffer_write_i32(main, city_data.finance.tax_percentage);
    buffer_write_i32(main, city_data.finance.treasury);
    buffer_write_i32(main, city_data.sentiment.value);
    buffer_write_i32(main, city_data.health.target_value);
    buffer_write_i32(main, city_data.health.value);
    buffer_write_i32(main, city_data.health.num_hospital_workers);
    buffer_write_u32(main, city_data.population.population);
    buffer_write_i32(main, city_data.population.population_last_year);
    buffer_write_i32(main, city_data.population.school_age);
    buffer_write_i32(main, city_data.population.academy_age);
    buffer_write_i32(main, city_data.population.total_capacity);
    buffer_write_i32(main, city_data.population.room_in_houses);
    for (int i = 0; i < 2400; i++) {
        buffer_write_i32(main, city_data.population.monthly.values[i]);
    }
    buffer_write_i32(main, city_data.population.monthly.next_index);
    buffer_write_i32(main, city_data.population.monthly.count);
    for (int i = 0; i < 100; i++) {
        buffer_write_i16(main, city_data.population.at_age[i]);
    }
    for (int i = 0; i < 20; i++) {
        buffer_write_i32(main, city_data.population.at_level[i]);
    }
    buffer_write_i32(main, city_data.population.yearly_births);
    buffer_write_i32(main, city_data.population.yearly_deaths);
    buffer_write_i32(main, city_data.population.lost_removal);
    buffer_write_i32(main, city_data.migration.immigration_amount_per_batch);
    buffer_write_i32(main, city_data.migration.emigration_amount_per_batch);
    buffer_write_i32(main, city_data.migration.emigration_queue_size);
    buffer_write_i32(main, city_data.migration.immigration_queue_size);
    buffer_write_i32(main, city_data.population.lost_homeless);
    buffer_write_i32(main, city_data.population.last_change);
    buffer_write_i32(main, city_data.population.average_per_year);
    buffer_write_i32(main, city_data.population.total_all_years);
    buffer_write_i32(main, city_data.population.people_in_tents_shacks);
    buffer_write_i32(main, city_data.population.people_in_villas_palaces);
    buffer_write_i32(main, city_data.population.total_years);
    buffer_write_i32(main, city_data.population.yearly_update_requested);
    buffer_write_i32(main, city_data.population.last_used_house_add);
    buffer_write_i32(main, city_data.population.last_used_house_remove);
    buffer_write_i32(main, city_data.migration.immigrated_today);
    buffer_write_i32(main, city_data.migration.emigrated_today);
    buffer_write_i32(main, city_data.migration.refused_immigrants_today);
    buffer_write_i32(main, city_data.migration.percentage);
    buffer_write_i32(main, city_data.migration.immigration_duration);
    buffer_write_i32(main, city_data.migration.emigration_duration);
    buffer_write_i32(main, city_data.migration.newcomers);
    buffer_write_i16(main, city_data.resource.last_used_warehouse);
    buffer_write_u8(main, city_data.building.senate_x);
    buffer_write_u8(main, city_data.building.senate_y);
    buffer_write_i16(main, city_data.building.senate_grid_offset);
    buffer_write_i32(main, city_data.building.senate_building_id);
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        buffer_write_i16(main, city_data.resource.space_in_warehouses[i]);
    }
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        buffer_write_u16(main, city_data.resource.stored_in_warehouses[i]);
    }
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        buffer_write_i16(main, city_data.resource.trade_status[i]);
    }
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        buffer_write_i16(main, city_data.resource.export_over[i]);
    }
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        buffer_write_i16(main, city_data.resource.mothballed[i]);
    }
    for (int i = 0; i < FOOD_TYPES_MAX; i++) {
        buffer_write_i32(main, city_data.resource.granary_food_stored[i]);
    }
    for (int i = 0; i < 6; i++) {
        buffer_write_i32(main, city_data.resource.stored_in_workshops[i]);
    }
    for (int i = 0; i < 6; i++) {
        buffer_write_i32(main, city_data.resource.space_in_workshops[i]);
    }
    buffer_write_i32(main, city_data.resource.granary_total_stored);
    buffer_write_i32(main, city_data.resource.food_types_available);
    buffer_write_i32(main, city_data.resource.food_types_eaten);
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        buffer_write_i32(main, city_data.resource.stockpiled[i]);
    }
    buffer_write_i32(main, city_data.resource.food_supply_months);
    buffer_write_i32(main, city_data.resource.granaries.operating);
    buffer_write_i32(main, city_data.population.percentage_plebs);
    buffer_write_i32(main, city_data.population.working_age);
    buffer_write_i32(main, city_data.labor.workers_available);
    for (int i = 0; i < 10; i++) {
        buffer_write_i32(main, city_data.labor.categories[i].workers_needed);
        buffer_write_i32(main, city_data.labor.categories[i].workers_allocated);
        buffer_write_i32(main, city_data.labor.categories[i].total_houses_covered);
        buffer_write_i32(main, city_data.labor.categories[i].buildings);
        buffer_write_i32(main, city_data.labor.categories[i].priority);
    }
    buffer_write_i32(main, city_data.labor.workers_employed);
    buffer_write_i32(main, city_data.labor.workers_unemployed);
    buffer_write_i32(main, city_data.labor.unemployment_percentage);
    buffer_write_i32(main, city_data.labor.unemployment_percentage_for_senate);
    buffer_write_i32(main, city_data.labor.workers_needed);
    buffer_write_i32(main, city_data.labor.wages);
    buffer_write_i32(main, city_data.labor.wages_rome);
    buffer_write_i32(main, city_data.finance.wages_so_far);
    buffer_write_i32(main, city_data.finance.this_year.expenses.wages);
    buffer_write_i32(main, city_data.finance.last_year.expenses.wages);
    buffer_write_i32(main, city_data.taxes.taxed_plebs);
    buffer_write_i32(main, city_data.taxes.taxed_patricians);
    buffer_write_i32(main, city_data.taxes.untaxed_plebs);
    buffer_write_i32(main, city_data.taxes.untaxed_patricians);
    buffer_write_i32(main, city_data.taxes.percentage_taxed_plebs);
    buffer_write_i32(main, city_data.taxes.percentage_taxed_patricians);
    buffer_write_i32(main, city_data.taxes.percentage_taxed_people);
    buffer_write_i32(main, city_data.taxes.yearly.collected_plebs);
    buffer_write_i32(main, city_data.taxes.yearly.collected_patricians);
    buffer_write_i32(main, city_data.taxes.yearly.uncollected_plebs);
    buffer_write_i32(main, city_data.taxes.yearly.uncollected_patricians);
    buffer_write_i32(main, city_data.finance.this_year.income.taxes);
    buffer_write_i32(main, city_data.finance.last_year.income.taxes);
    buffer_write_i32(main, city_data.taxes.monthly.collected_plebs);
    buffer_write_i32(main, city_data.taxes.monthly.uncollected_plebs);
    buffer_write_i32(main, city_data.taxes.monthly.collected_patricians);
    buffer_write_i32(main, city_data.taxes.monthly.uncollected_patricians);
    buffer_write_i32(main, city_data.finance.this_year.income.exports);
    buffer_write_i32(main, city_data.finance.last_year.income.exports);
    buffer_write_i32(main, city_data.finance.this_year.expenses.imports);
    buffer_write_i32(main, city_data.finance.last_year.expenses.imports);
    buffer_write_i32(main, city_data.finance.interest_so_far);
    buffer_write_i32(main, city_data.finance.last_year.expenses.interest);
    buffer_write_i32(main, city_data.finance.this_year.expenses.interest);
    buffer_write_i32(main, city_data.finance.last_year.expenses.sundries);
    buffer_write_i32(main, city_data.finance.this_year.expenses.sundries);
    buffer_write_i32(main, city_data.finance.last_year.expenses.construction);
    buffer_write_i32(main, city_data.finance.this_year.expenses.construction);
    buffer_write_i32(main, city_data.finance.last_year.expenses.salary);
    buffer_write_i32(main, city_data.finance.this_year.expenses.salary);
    buffer_write_i32(main, city_data.emperor.salary_amount);
    buffer_write_i32(main, city_data.emperor.salary_rank);
    buffer_write_i32(main, city_data.finance.salary_so_far);
    buffer_write_i32(main, city_data.finance.last_year.income.total);
    buffer_write_i32(main, city_data.finance.this_year.income.total);
    buffer_write_i32(main, city_data.finance.last_year.expenses.total);
    buffer_write_i32(main, city_data.finance.this_year.expenses.total);
    buffer_write_i32(main, city_data.finance.last_year.net_in_out);
    buffer_write_i32(main, city_data.finance.this_year.net_in_out);
    buffer_write_i32(main, city_data.finance.last_year.balance);
    buffer_write_i32(main, city_data.finance.this_year.balance);
    buffer_write_i32(main, city_data.trade.caravan_import_resource);
    buffer_write_i32(main, city_data.trade.caravan_backup_import_resource);
    buffer_write_u32(main, city_data.ratings.culture);
    buffer_write_u32(main, city_data.ratings.prosperity);
    buffer_write_u32(main, city_data.ratings.peace);
    buffer_write_u32(main, city_data.ratings.favor);
    buffer_write_i32(main, city_data.ratings.prosperity_treasury_last_year);
    buffer_write_i32(main, city_data.ratings.peace_num_criminals);
    buffer_write_i32(main, city_data.ratings.peace_num_rioters);
    buffer_write_i32(main, city_data.houses.missing.fountain);
    buffer_write_i32(main, city_data.houses.missing.well);
    buffer_write_i32(main, city_data.houses.missing.more_entertainment);
    buffer_write_i32(main, city_data.houses.missing.more_education);
    buffer_write_i32(main, city_data.houses.missing.education);
    buffer_write_i32(main, city_data.houses.requiring.school);
    buffer_write_i32(main, city_data.houses.requiring.library);
    buffer_write_i32(main, city_data.houses.missing.barber);
    buffer_write_i32(main, city_data.houses.missing.bathhouse);
    buffer_write_i32(main, city_data.houses.missing.food);
    buffer_write_i32(main, city_data.building.hippodrome_placed);
    buffer_write_i32(main, city_data.houses.missing.clinic);
    buffer_write_i32(main, city_data.houses.missing.hospital);
    buffer_write_i32(main, city_data.houses.requiring.barber);
    buffer_write_i32(main, city_data.houses.requiring.bathhouse);
    buffer_write_i32(main, city_data.houses.requiring.clinic);
    buffer_write_i32(main, city_data.houses.missing.religion);
    buffer_write_i32(main, city_data.houses.missing.second_religion);
    buffer_write_i32(main, city_data.houses.missing.third_religion);
    buffer_write_i32(main, city_data.houses.requiring.religion);
    buffer_write_i32(main, city_data.entertainment.theater_shows);
    buffer_write_i32(main, city_data.entertainment.theater_no_shows_weighted);
    buffer_write_i32(main, city_data.entertainment.amphitheater_shows);
    buffer_write_i32(main, city_data.entertainment.amphitheater_no_shows_weighted);
    buffer_write_i32(main, city_data.entertainment.colosseum_shows);
    buffer_write_i32(main, city_data.entertainment.colosseum_no_shows_weighted);
    buffer_write_i32(main, city_data.entertainment.hippodrome_shows);
    buffer_write_i32(main, city_data.entertainment.hippodrome_no_shows_weighted);
    buffer_write_i32(main, city_data.entertainment.venue_needing_shows);
    buffer_write_i32(main, city_data.culture.average_entertainment);
    buffer_write_i32(main, city_data.houses.missing.entertainment);
    buffer_write_i32(main, city_data.festival.months_since_festival);
    for (int i = 0; i < MAX_GODS; i++) {
        buffer_write_i8(main, city_data.religion.gods[i].target_happiness);
    }
    for (int i = 0; i < MAX_GODS; i++) {
        buffer_write_i8(main, city_data.religion.gods[i].happiness);
    }
    for (int i = 0; i < MAX_GODS; i++) {
        buffer_write_i8(main, city_data.religion.gods[i].wrath_bolts);
    }
    for (int i = 0; i < MAX_GODS; i++) {
        buffer_write_i8(main, city_data.religion.gods[i].blessing_done);
    }
    for (int i = 0; i < MAX_GODS; i++) {
        buffer_write_i8(main, city_data.religion.gods[i].small_curse_done);
    }
    for (int i = 0; i < MAX_GODS; i++) {
        buffer_write_i32(main, city_data.religion.gods[i].months_since_festival);
    }
    buffer_write_i32(main, city_data.religion.least_happy_god);
    buffer_write_i32(main, city_data.migration.no_immigration_cause);
    buffer_write_i32(main, city_data.sentiment.protesters);
    buffer_write_i32(main, city_data.sentiment.criminals);
    buffer_write_i32(main, city_data.houses.health);
    buffer_write_i32(main, city_data.houses.religion);
    buffer_write_i32(main, city_data.houses.education);
    buffer_write_i32(main, city_data.houses.entertainment);
    buffer_write_i32(main, city_data.figure.rioters);
    buffer_write_i32(main, city_data.ratings.selected);
    buffer_write_i32(main, city_data.ratings.culture_explanation);
    buffer_write_i32(main, city_data.ratings.prosperity_explanation);
    buffer_write_i32(main, city_data.ratings.peace_explanation);
    buffer_write_i32(main, city_data.ratings.favor_explanation);
    buffer_write_i32(main, city_data.emperor.player_rank);
    buffer_write_i32(main, city_data.emperor.personal_savings);
    buffer_write_i32(main, city_data.finance.last_year.income.donated);
    buffer_write_i32(main, city_data.finance.this_year.income.donated);
    buffer_write_i32(main, city_data.emperor.donate_amount);
    for (int i = 0; i < 10; i++) {
        buffer_write_i16(main, city_data.building.working_dock_ids[i]);
    }
    buffer_write_i16(main, city_data.figure.animals);
    buffer_write_i16(main, city_data.trade.num_sea_routes);
    buffer_write_i16(main, city_data.trade.num_land_routes);
    buffer_write_i16(main, city_data.trade.sea_trade_problem_duration);
    buffer_write_i16(main, city_data.trade.land_trade_problem_duration);
    buffer_write_i16(main, city_data.building.working_docks);
    buffer_write_i16(main, city_data.building.senate_placed);
    buffer_write_i16(main, city_data.building.working_wharfs);
    buffer_write_i16(main, city_data.finance.stolen_this_year);
    buffer_write_i16(main, city_data.finance.stolen_last_year);
    buffer_write_i32(main, city_data.trade.docker_import_resource);
    buffer_write_i32(main, city_data.trade.docker_export_resource);
    buffer_write_i32(main, city_data.emperor.debt_state);
    buffer_write_i32(main, city_data.emperor.months_in_debt);
    buffer_write_i32(main, city_data.finance.cheated_money);
    buffer_write_i8(main, city_data.building.barracks_x);
    buffer_write_i8(main, city_data.building.barracks_y);
    buffer_write_i16(main, city_data.building.barracks_grid_offset);
    buffer_write_i32(main, city_data.building.barracks_building_id);
    buffer_write_i32(main, city_data.building.barracks_placed);
    buffer_write_i32(main, city_data.population.lost_troop_request);
    buffer_write_i32(main, city_data.mission.has_won);
    buffer_write_i32(main, city_data.mission.continue_months_left);
    buffer_write_i32(main, city_data.mission.continue_months_chosen);
    buffer_write_i32(main, city_data.finance.wage_rate_paid_this_year);
    buffer_write_i32(main, city_data.finance.this_year.expenses.tribute);
    buffer_write_i32(main, city_data.finance.last_year.expenses.tribute);
    buffer_write_i32(main, city_data.finance.tribute_not_paid_last_year);
    buffer_write_i32(main, city_data.finance.tribute_not_paid_total_years);
    buffer_write_i8(main, city_data.festival.god);
    buffer_write_i8(main, city_data.festival.size);
    buffer_write_i32(main, city_data.festival.cost);
    buffer_write_i32(main, city_data.festival.months_to_go);
    buffer_write_i32(main, city_data.culture.average_religion);
    buffer_write_i32(main, city_data.culture.average_education);
    buffer_write_i32(main, city_data.culture.average_health);
    buffer_write_i32(main, city_data.culture.religion_coverage);
    buffer_write_i32(main, city_data.festival.first_festival_effect_months);
    buffer_write_i32(main, city_data.festival.second_festival_effect_months);
    buffer_write_i32(main, city_data.sentiment.previous_value);
    buffer_write_i32(main, city_data.sentiment.message_delay);
    buffer_write_i32(main, city_data.sentiment.low_mood_cause);
    buffer_write_i32(main, city_data.emperor.months_since_gift);
    buffer_write_i32(main, city_data.emperor.gift_overdose_penalty);
    buffer_write_i32(main, city_data.emperor.gifts[GIFT_MODEST].id);
    buffer_write_i32(main, city_data.emperor.gifts[GIFT_GENEROUS].id);
    buffer_write_i32(main, city_data.emperor.gifts[GIFT_LAVISH].id);
    buffer_write_i32(main, city_data.emperor.gifts[GIFT_MODEST].cost);
    buffer_write_i32(main, city_data.emperor.gifts[GIFT_GENEROUS].cost);
    buffer_write_i32(main, city_data.emperor.gifts[GIFT_LAVISH].cost);
    buffer_write_i32(main, city_data.ratings.favor_salary_penalty);
    buffer_write_i32(main, city_data.ratings.favor_ignored_request_penalty);
    buffer_write_u32(main, city_data.ratings.favor_last_year);
    buffer_write_i32(main, city_data.ratings.favor_change);
    buffer_write_i32(main, city_data.military.native_attack_duration);
    buffer_write_i32(main, city_data.building.mission_post_operational);
    buffer_write_i32(main, city_data.building.main_native_meeting.x);
    buffer_write_i32(main, city_data.building.main_native_meeting.y);
    buffer_write_i32(main, city_data.finance.wage_rate_paid_last_year);
    buffer_write_i32(main, city_data.resource.food_needed_per_month);
    buffer_write_i32(main, city_data.resource.granaries.understaffed);
    buffer_write_i32(main, city_data.resource.granaries.not_operating);
    buffer_write_i32(main, city_data.resource.granaries.not_operating_with_food);
    buffer_write_i32(main, city_data.religion.venus_curse_active);
    buffer_write_i32(main, city_data.religion.neptune_double_trade_active);
    buffer_write_i32(main, city_data.religion.mars_spirit_power);
    buffer_write_i32(main, city_data.religion.angry_message_delay);
    buffer_write_i32(main, city_data.resource.food_consumed_last_month);
    buffer_write_i32(main, city_data.resource.food_produced_last_month);
    buffer_write_i32(main, city_data.resource.food_produced_this_month);
    buffer_write_i32(main, city_data.ratings.peace_riot_cause);
    buffer_write_i32(main, city_data.finance.estimated_tax_income);
    buffer_write_i32(main, city_data.building.shipyard_boats_requested);
    buffer_write_i32(main, city_data.figure.enemies);
    buffer_write_i32(main, city_data.population.people_in_tents);
    buffer_write_i32(main, city_data.population.people_in_large_insula_and_above);
    buffer_write_i32(main, city_data.figure.imperial_soldiers);
    buffer_write_i32(main, city_data.emperor.invasion.duration_day_countdown);
    buffer_write_i32(main, city_data.emperor.invasion.warnings_given);
    buffer_write_i32(main, city_data.emperor.invasion.days_until_invasion);
    buffer_write_i32(main, city_data.emperor.invasion.retreat_message_shown);
    buffer_write_i16(main, city_data.emperor.invasion.from_editor);
    buffer_write_i32(main, city_data.ratings.peace_destroyed_buildings);
    buffer_write_i32(main, city_data.ratings.peace_years_of_peace);
    buffer_write_u8(main, city_data.distant_battle.city);
    buffer_write_u8(main, city_data.distant_battle.enemy_strength);
    buffer_write_u8(main, city_data.distant_battle.roman_strength);
    buffer_write_i8(main, city_data.distant_battle.months_until_battle);
    buffer_write_i8(main, city_data.distant_battle.roman_months_to_travel_back);
    buffer_write_i8(main, city_data.distant_battle.roman_months_to_travel_forth);
    buffer_write_i8(main, city_data.distant_battle.city_foreign_months_left);
    buffer_write_i8(main, city_data.building.triumphal_arches_available);
    buffer_write_i8(main, city_data.distant_battle.total_count);
    buffer_write_i8(main, city_data.distant_battle.won_count);
    buffer_write_i8(main, city_data.distant_battle.enemy_months_traveled);
    buffer_write_i8(main, city_data.distant_battle.roman_months_traveled);
    buffer_write_u8(main, city_data.military.total_legions);
    buffer_write_u8(main, city_data.sound.die_citizen);
    buffer_write_u8(main, city_data.sound.die_soldier);
    buffer_write_i32(main, city_data.building.trade_center_building_id);
    buffer_write_i32(main, city_data.figure.soldiers);
    buffer_write_u8(main, city_data.sound.hit_elephant);
    buffer_write_i32(main, city_data.emperor.invasion.count);
    buffer_write_i32(main, city_data.emperor.invasion.size);
    buffer_write_i32(main, city_data.emperor.invasion.soldiers_killed);
    buffer_write_i32(main, city_data.military.legionary_legions);
    buffer_write_u32(main, city_data.population.highest_ever);
    buffer_write_i32(main, city_data.finance.estimated_wages);
    buffer_write_i32(main, city_data.resource.wine_types_available);
    buffer_write_u32(main, city_data.ratings.prosperity_max);
    for (int i = 0; i < 10; i++) {
        buffer_write_i32(main, city_data.map.largest_road_networks[i].id);
        buffer_write_i32(main, city_data.map.largest_road_networks[i].size);
    }
    buffer_write_i32(main, city_data.houses.missing.second_wine);
    buffer_write_i32(main, city_data.religion.neptune_sank_ships);
    buffer_write_i32(main, city_data.entertainment.hippodrome_has_race);
    buffer_write_i32(main, city_data.entertainment.hippodrome_message_shown);
    buffer_write_i32(main, city_data.entertainment.colosseum_message_shown);
    buffer_write_i32(main, city_data.migration.emigration_message_shown);
    buffer_write_i32(main, city_data.mission.fired_message_shown);
    buffer_write_i32(main, city_data.mission.victory_message_shown);

    buffer_write_i32(graph_order, city_data.population.graph_order);
}

void city_data_load_state(struct buffer_t *main, struct buffer_t *graph_order)
{
    city_data.finance.tax_percentage = buffer_read_i32(main);
    city_data.finance.treasury = buffer_read_i32(main);
    city_data.sentiment.value = buffer_read_i32(main);
    city_data.health.target_value = buffer_read_i32(main);
    city_data.health.value = buffer_read_i32(main);
    city_data.health.num_hospital_workers = buffer_read_i32(main);
    city_data.population.population = buffer_read_u32(main);
    city_data.population.population_last_year = buffer_read_i32(main);
    city_data.population.school_age = buffer_read_i32(main);
    city_data.population.academy_age = buffer_read_i32(main);
    city_data.population.total_capacity = buffer_read_i32(main);
    city_data.population.room_in_houses = buffer_read_i32(main);
    for (int i = 0; i < 2400; i++) {
        city_data.population.monthly.values[i] = buffer_read_i32(main);
    }
    city_data.population.monthly.next_index = buffer_read_i32(main);
    city_data.population.monthly.count = buffer_read_i32(main);
    for (int i = 0; i < 100; i++) {
        city_data.population.at_age[i] = buffer_read_i16(main);
    }
    for (int i = 0; i < 20; i++) {
        city_data.population.at_level[i] = buffer_read_i32(main);
    }
    city_data.population.yearly_births = buffer_read_i32(main);
    city_data.population.yearly_deaths = buffer_read_i32(main);
    city_data.population.lost_removal = buffer_read_i32(main);
    city_data.migration.immigration_amount_per_batch = buffer_read_i32(main);
    city_data.migration.emigration_amount_per_batch = buffer_read_i32(main);
    city_data.migration.emigration_queue_size = buffer_read_i32(main);
    city_data.migration.immigration_queue_size = buffer_read_i32(main);
    city_data.population.lost_homeless = buffer_read_i32(main);
    city_data.population.last_change = buffer_read_i32(main);
    city_data.population.average_per_year = buffer_read_i32(main);
    city_data.population.total_all_years = buffer_read_i32(main);
    city_data.population.people_in_tents_shacks = buffer_read_i32(main);
    city_data.population.people_in_villas_palaces = buffer_read_i32(main);
    city_data.population.total_years = buffer_read_i32(main);
    city_data.population.yearly_update_requested = buffer_read_i32(main);
    city_data.population.last_used_house_add = buffer_read_i32(main);
    city_data.population.last_used_house_remove = buffer_read_i32(main);
    city_data.migration.immigrated_today = buffer_read_i32(main);
    city_data.migration.emigrated_today = buffer_read_i32(main);
    city_data.migration.refused_immigrants_today = buffer_read_i32(main);
    city_data.migration.percentage = buffer_read_i32(main);
    city_data.migration.immigration_duration = buffer_read_i32(main);
    city_data.migration.emigration_duration = buffer_read_i32(main);
    city_data.migration.newcomers = buffer_read_i32(main);
    city_data.resource.last_used_warehouse = buffer_read_i16(main);
    city_data.building.senate_x = buffer_read_u8(main);
    city_data.building.senate_y = buffer_read_u8(main);
    city_data.building.senate_grid_offset = buffer_read_i16(main);
    city_data.building.senate_building_id = buffer_read_i32(main);
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        city_data.resource.space_in_warehouses[i] = buffer_read_i16(main);
    }
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        city_data.resource.stored_in_warehouses[i] = buffer_read_u16(main);
    }
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        city_data.resource.trade_status[i] = buffer_read_i16(main);
    }
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        city_data.resource.export_over[i] = buffer_read_i16(main);
    }
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        city_data.resource.mothballed[i] = buffer_read_i16(main);
    }
    for (int i = 0; i < FOOD_TYPES_MAX; i++) {
        city_data.resource.granary_food_stored[i] = buffer_read_i32(main);
    }
    for (int i = 0; i < 6; i++) {
        city_data.resource.stored_in_workshops[i] = buffer_read_i32(main);
    }
    for (int i = 0; i < 6; i++) {
        city_data.resource.space_in_workshops[i] = buffer_read_i32(main);
    }
    city_data.resource.granary_total_stored = buffer_read_i32(main);
    city_data.resource.food_types_available = buffer_read_i32(main);
    city_data.resource.food_types_eaten = buffer_read_i32(main);
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        city_data.resource.stockpiled[i] = buffer_read_i32(main);
    }
    city_data.resource.food_supply_months = buffer_read_i32(main);
    city_data.resource.granaries.operating = buffer_read_i32(main);
    city_data.population.percentage_plebs = buffer_read_i32(main);
    city_data.population.working_age = buffer_read_i32(main);
    city_data.labor.workers_available = buffer_read_i32(main);
    for (int i = 0; i < 10; i++) {
        city_data.labor.categories[i].workers_needed = buffer_read_i32(main);
        city_data.labor.categories[i].workers_allocated = buffer_read_i32(main);
        city_data.labor.categories[i].total_houses_covered = buffer_read_i32(main);
        city_data.labor.categories[i].buildings = buffer_read_i32(main);
        city_data.labor.categories[i].priority = buffer_read_i32(main);
    }
    city_data.labor.workers_employed = buffer_read_i32(main);
    city_data.labor.workers_unemployed = buffer_read_i32(main);
    city_data.labor.unemployment_percentage = buffer_read_i32(main);
    city_data.labor.unemployment_percentage_for_senate = buffer_read_i32(main);
    city_data.labor.workers_needed = buffer_read_i32(main);
    city_data.labor.wages = buffer_read_i32(main);
    city_data.labor.wages_rome = buffer_read_i32(main);
    city_data.finance.wages_so_far = buffer_read_i32(main);
    city_data.finance.this_year.expenses.wages = buffer_read_i32(main);
    city_data.finance.last_year.expenses.wages = buffer_read_i32(main);
    city_data.taxes.taxed_plebs = buffer_read_i32(main);
    city_data.taxes.taxed_patricians = buffer_read_i32(main);
    city_data.taxes.untaxed_plebs = buffer_read_i32(main);
    city_data.taxes.untaxed_patricians = buffer_read_i32(main);
    city_data.taxes.percentage_taxed_plebs = buffer_read_i32(main);
    city_data.taxes.percentage_taxed_patricians = buffer_read_i32(main);
    city_data.taxes.percentage_taxed_people = buffer_read_i32(main);
    city_data.taxes.yearly.collected_plebs = buffer_read_i32(main);
    city_data.taxes.yearly.collected_patricians = buffer_read_i32(main);
    city_data.taxes.yearly.uncollected_plebs = buffer_read_i32(main);
    city_data.taxes.yearly.uncollected_patricians = buffer_read_i32(main);
    city_data.finance.this_year.income.taxes = buffer_read_i32(main);
    city_data.finance.last_year.income.taxes = buffer_read_i32(main);
    city_data.taxes.monthly.collected_plebs = buffer_read_i32(main);
    city_data.taxes.monthly.uncollected_plebs = buffer_read_i32(main);
    city_data.taxes.monthly.collected_patricians = buffer_read_i32(main);
    city_data.taxes.monthly.uncollected_patricians = buffer_read_i32(main);
    city_data.finance.this_year.income.exports = buffer_read_i32(main);
    city_data.finance.last_year.income.exports = buffer_read_i32(main);
    city_data.finance.this_year.expenses.imports = buffer_read_i32(main);
    city_data.finance.last_year.expenses.imports = buffer_read_i32(main);
    city_data.finance.interest_so_far = buffer_read_i32(main);
    city_data.finance.last_year.expenses.interest = buffer_read_i32(main);
    city_data.finance.this_year.expenses.interest = buffer_read_i32(main);
    city_data.finance.last_year.expenses.sundries = buffer_read_i32(main);
    city_data.finance.this_year.expenses.sundries = buffer_read_i32(main);
    city_data.finance.last_year.expenses.construction = buffer_read_i32(main);
    city_data.finance.this_year.expenses.construction = buffer_read_i32(main);
    city_data.finance.last_year.expenses.salary = buffer_read_i32(main);
    city_data.finance.this_year.expenses.salary = buffer_read_i32(main);
    city_data.emperor.salary_amount = buffer_read_i32(main);
    city_data.emperor.salary_rank = buffer_read_i32(main);
    city_data.finance.salary_so_far = buffer_read_i32(main);
    city_data.finance.last_year.income.total = buffer_read_i32(main);
    city_data.finance.this_year.income.total = buffer_read_i32(main);
    city_data.finance.last_year.expenses.total = buffer_read_i32(main);
    city_data.finance.this_year.expenses.total = buffer_read_i32(main);
    city_data.finance.last_year.net_in_out = buffer_read_i32(main);
    city_data.finance.this_year.net_in_out = buffer_read_i32(main);
    city_data.finance.last_year.balance = buffer_read_i32(main);
    city_data.finance.this_year.balance = buffer_read_i32(main);
    city_data.trade.caravan_import_resource = buffer_read_i32(main);
    city_data.trade.caravan_backup_import_resource = buffer_read_i32(main);
    city_data.ratings.culture = buffer_read_u32(main);
    city_data.ratings.prosperity = buffer_read_u32(main);
    city_data.ratings.peace = buffer_read_u32(main);
    city_data.ratings.favor = buffer_read_u32(main);
    city_data.ratings.prosperity_treasury_last_year = buffer_read_i32(main);
    city_data.ratings.peace_num_criminals = buffer_read_i32(main);
    city_data.ratings.peace_num_rioters = buffer_read_i32(main);
    city_data.houses.missing.fountain = buffer_read_i32(main);
    city_data.houses.missing.well = buffer_read_i32(main);
    city_data.houses.missing.more_entertainment = buffer_read_i32(main);
    city_data.houses.missing.more_education = buffer_read_i32(main);
    city_data.houses.missing.education = buffer_read_i32(main);
    city_data.houses.requiring.school = buffer_read_i32(main);
    city_data.houses.requiring.library = buffer_read_i32(main);
    city_data.houses.missing.barber = buffer_read_i32(main);
    city_data.houses.missing.bathhouse = buffer_read_i32(main);
    city_data.houses.missing.food = buffer_read_i32(main);
    city_data.building.hippodrome_placed = buffer_read_i32(main);
    city_data.houses.missing.clinic = buffer_read_i32(main);
    city_data.houses.missing.hospital = buffer_read_i32(main);
    city_data.houses.requiring.barber = buffer_read_i32(main);
    city_data.houses.requiring.bathhouse = buffer_read_i32(main);
    city_data.houses.requiring.clinic = buffer_read_i32(main);
    city_data.houses.missing.religion = buffer_read_i32(main);
    city_data.houses.missing.second_religion = buffer_read_i32(main);
    city_data.houses.missing.third_religion = buffer_read_i32(main);
    city_data.houses.requiring.religion = buffer_read_i32(main);
    city_data.entertainment.theater_shows = buffer_read_i32(main);
    city_data.entertainment.theater_no_shows_weighted = buffer_read_i32(main);
    city_data.entertainment.amphitheater_shows = buffer_read_i32(main);
    city_data.entertainment.amphitheater_no_shows_weighted = buffer_read_i32(main);
    city_data.entertainment.colosseum_shows = buffer_read_i32(main);
    city_data.entertainment.colosseum_no_shows_weighted = buffer_read_i32(main);
    city_data.entertainment.hippodrome_shows = buffer_read_i32(main);
    city_data.entertainment.hippodrome_no_shows_weighted = buffer_read_i32(main);
    city_data.entertainment.venue_needing_shows = buffer_read_i32(main);
    city_data.culture.average_entertainment = buffer_read_i32(main);
    city_data.houses.missing.entertainment = buffer_read_i32(main);
    city_data.festival.months_since_festival = buffer_read_i32(main);
    for (int i = 0; i < MAX_GODS; i++) {
        city_data.religion.gods[i].target_happiness = buffer_read_i8(main);
    }
    for (int i = 0; i < MAX_GODS; i++) {
        city_data.religion.gods[i].happiness = buffer_read_i8(main);
    }
    for (int i = 0; i < MAX_GODS; i++) {
        city_data.religion.gods[i].wrath_bolts = buffer_read_i8(main);
    }
    for (int i = 0; i < MAX_GODS; i++) {
        city_data.religion.gods[i].blessing_done = buffer_read_i8(main);
    }
    for (int i = 0; i < MAX_GODS; i++) {
        city_data.religion.gods[i].small_curse_done = buffer_read_i8(main);
    }
    for (int i = 0; i < MAX_GODS; i++) {
        city_data.religion.gods[i].months_since_festival = buffer_read_i32(main);
    }
    city_data.religion.least_happy_god = buffer_read_i32(main);
    city_data.migration.no_immigration_cause = buffer_read_i32(main);
    city_data.sentiment.protesters = buffer_read_i32(main);
    city_data.sentiment.criminals = buffer_read_i32(main);
    city_data.houses.health = buffer_read_i32(main);
    city_data.houses.religion = buffer_read_i32(main);
    city_data.houses.education = buffer_read_i32(main);
    city_data.houses.entertainment = buffer_read_i32(main);
    city_data.figure.rioters = buffer_read_i32(main);
    city_data.ratings.selected = buffer_read_i32(main);
    city_data.ratings.culture_explanation = buffer_read_i32(main);
    city_data.ratings.prosperity_explanation = buffer_read_i32(main);
    city_data.ratings.peace_explanation = buffer_read_i32(main);
    city_data.ratings.favor_explanation = buffer_read_i32(main);
    city_data.emperor.player_rank = buffer_read_i32(main);
    city_data.emperor.personal_savings = buffer_read_i32(main);
    city_data.finance.last_year.income.donated = buffer_read_i32(main);
    city_data.finance.this_year.income.donated = buffer_read_i32(main);
    city_data.emperor.donate_amount = buffer_read_i32(main);
    for (int i = 0; i < 10; i++) {
        city_data.building.working_dock_ids[i] = buffer_read_i16(main);
    }
    city_data.figure.animals = buffer_read_i16(main);
    city_data.trade.num_sea_routes = buffer_read_i16(main);
    city_data.trade.num_land_routes = buffer_read_i16(main);
    city_data.trade.sea_trade_problem_duration = buffer_read_i16(main);
    city_data.trade.land_trade_problem_duration = buffer_read_i16(main);
    city_data.building.working_docks = buffer_read_i16(main);
    city_data.building.senate_placed = buffer_read_i16(main);
    city_data.building.working_wharfs = buffer_read_i16(main);
    city_data.finance.stolen_this_year = buffer_read_i16(main);
    city_data.finance.stolen_last_year = buffer_read_i16(main);
    city_data.trade.docker_import_resource = buffer_read_i32(main);
    city_data.trade.docker_export_resource = buffer_read_i32(main);
    city_data.emperor.debt_state = buffer_read_i32(main);
    city_data.emperor.months_in_debt = buffer_read_i32(main);
    city_data.finance.cheated_money = buffer_read_i32(main);
    city_data.building.barracks_x = buffer_read_i8(main);
    city_data.building.barracks_y = buffer_read_i8(main);
    city_data.building.barracks_grid_offset = buffer_read_i16(main);
    city_data.building.barracks_building_id = buffer_read_i32(main);
    city_data.building.barracks_placed = buffer_read_i32(main);
    city_data.population.lost_troop_request = buffer_read_i32(main);
    city_data.mission.has_won = buffer_read_i32(main);
    city_data.mission.continue_months_left = buffer_read_i32(main);
    city_data.mission.continue_months_chosen = buffer_read_i32(main);
    city_data.finance.wage_rate_paid_this_year = buffer_read_i32(main);
    city_data.finance.this_year.expenses.tribute = buffer_read_i32(main);
    city_data.finance.last_year.expenses.tribute = buffer_read_i32(main);
    city_data.finance.tribute_not_paid_last_year = buffer_read_i32(main);
    city_data.finance.tribute_not_paid_total_years = buffer_read_i32(main);
    city_data.festival.god = buffer_read_i8(main);
    city_data.festival.size = buffer_read_i8(main);
    city_data.festival.cost = buffer_read_i32(main);
    city_data.festival.months_to_go = buffer_read_i32(main);
    city_data.culture.average_religion = buffer_read_i32(main);
    city_data.culture.average_education = buffer_read_i32(main);
    city_data.culture.average_health = buffer_read_i32(main);
    city_data.culture.religion_coverage = buffer_read_i32(main);
    city_data.festival.first_festival_effect_months = buffer_read_i32(main);
    city_data.festival.second_festival_effect_months = buffer_read_i32(main);
    city_data.sentiment.previous_value = buffer_read_i32(main);
    city_data.sentiment.message_delay = buffer_read_i32(main);
    city_data.sentiment.low_mood_cause = buffer_read_i32(main);
    city_data.emperor.months_since_gift = buffer_read_i32(main);
    city_data.emperor.gift_overdose_penalty = buffer_read_i32(main);
    city_data.emperor.gifts[GIFT_MODEST].id = buffer_read_i32(main);
    city_data.emperor.gifts[GIFT_GENEROUS].id = buffer_read_i32(main);
    city_data.emperor.gifts[GIFT_LAVISH].id = buffer_read_i32(main);
    city_data.emperor.gifts[GIFT_MODEST].cost = buffer_read_i32(main);
    city_data.emperor.gifts[GIFT_GENEROUS].cost = buffer_read_i32(main);
    city_data.emperor.gifts[GIFT_LAVISH].cost = buffer_read_i32(main);
    city_data.ratings.favor_salary_penalty = buffer_read_i32(main);
    city_data.ratings.favor_ignored_request_penalty = buffer_read_i32(main);
    city_data.ratings.favor_last_year = buffer_read_u32(main);
    city_data.ratings.favor_change = buffer_read_i32(main);
    city_data.military.native_attack_duration = buffer_read_i32(main);
    city_data.building.mission_post_operational = buffer_read_i32(main);
    city_data.building.main_native_meeting.x = buffer_read_i32(main);
    city_data.building.main_native_meeting.y = buffer_read_i32(main);
    city_data.finance.wage_rate_paid_last_year = buffer_read_i32(main);
    city_data.resource.food_needed_per_month = buffer_read_i32(main);
    city_data.resource.granaries.understaffed = buffer_read_i32(main);
    city_data.resource.granaries.not_operating = buffer_read_i32(main);
    city_data.resource.granaries.not_operating_with_food = buffer_read_i32(main);
    city_data.religion.venus_curse_active = buffer_read_i32(main);
    city_data.religion.neptune_double_trade_active = buffer_read_i32(main);
    city_data.religion.mars_spirit_power = buffer_read_i32(main);
    city_data.religion.angry_message_delay = buffer_read_i32(main);
    city_data.resource.food_consumed_last_month = buffer_read_i32(main);
    city_data.resource.food_produced_last_month = buffer_read_i32(main);
    city_data.resource.food_produced_this_month = buffer_read_i32(main);
    city_data.ratings.peace_riot_cause = buffer_read_i32(main);
    city_data.finance.estimated_tax_income = buffer_read_i32(main);
    city_data.building.shipyard_boats_requested = buffer_read_i32(main);
    city_data.figure.enemies = buffer_read_i32(main);
    city_data.population.people_in_tents = buffer_read_i32(main);
    city_data.population.people_in_large_insula_and_above = buffer_read_i32(main);
    city_data.figure.imperial_soldiers = buffer_read_i32(main);
    city_data.emperor.invasion.duration_day_countdown = buffer_read_i32(main);
    city_data.emperor.invasion.warnings_given = buffer_read_i32(main);
    city_data.emperor.invasion.days_until_invasion = buffer_read_i32(main);
    city_data.emperor.invasion.retreat_message_shown = buffer_read_i32(main);
    city_data.emperor.invasion.from_editor = buffer_read_i16(main);
    city_data.ratings.peace_destroyed_buildings = buffer_read_i32(main);
    city_data.ratings.peace_years_of_peace = buffer_read_i32(main);
    city_data.distant_battle.city = buffer_read_u8(main);
    city_data.distant_battle.enemy_strength = buffer_read_u8(main);
    city_data.distant_battle.roman_strength = buffer_read_u8(main);
    city_data.distant_battle.months_until_battle = buffer_read_i8(main);
    city_data.distant_battle.roman_months_to_travel_back = buffer_read_i8(main);
    city_data.distant_battle.roman_months_to_travel_forth = buffer_read_i8(main);
    city_data.distant_battle.city_foreign_months_left = buffer_read_i8(main);
    city_data.building.triumphal_arches_available = buffer_read_i8(main);
    city_data.distant_battle.total_count = buffer_read_i8(main);
    city_data.distant_battle.won_count = buffer_read_i8(main);
    city_data.distant_battle.enemy_months_traveled = buffer_read_i8(main);
    city_data.distant_battle.roman_months_traveled = buffer_read_i8(main);
    city_data.military.total_legions = buffer_read_u8(main);
    city_data.sound.die_citizen = buffer_read_u8(main);
    city_data.sound.die_soldier = buffer_read_u8(main);
    city_data.building.trade_center_building_id = buffer_read_i32(main);
    city_data.figure.soldiers = buffer_read_i32(main);
    city_data.sound.hit_elephant = buffer_read_u8(main);
    city_data.emperor.invasion.count = buffer_read_i32(main);
    city_data.emperor.invasion.size = buffer_read_i32(main);
    city_data.emperor.invasion.soldiers_killed = buffer_read_i32(main);
    city_data.military.legionary_legions = buffer_read_i32(main);
    city_data.population.highest_ever = buffer_read_u32(main);
    city_data.finance.estimated_wages = buffer_read_i32(main);
    city_data.resource.wine_types_available = buffer_read_i32(main);
    city_data.ratings.prosperity_max = buffer_read_u32(main);
    for (int i = 0; i < 10; i++) {
        city_data.map.largest_road_networks[i].id = buffer_read_i32(main);
        city_data.map.largest_road_networks[i].size = buffer_read_i32(main);
    }
    city_data.houses.missing.second_wine = buffer_read_i32(main);
    city_data.religion.neptune_sank_ships = buffer_read_i32(main);
    city_data.entertainment.hippodrome_has_race = buffer_read_i32(main);
    city_data.entertainment.hippodrome_message_shown = buffer_read_i32(main);
    city_data.entertainment.colosseum_message_shown = buffer_read_i32(main);
    city_data.migration.emigration_message_shown = buffer_read_i32(main);
    city_data.mission.fired_message_shown = buffer_read_i32(main);
    city_data.mission.victory_message_shown = buffer_read_i32(main);

    city_data.population.graph_order = buffer_read_i32(graph_order);
}

int city_emperor_salary_for_rank(int rank)
{
    return SALARY_FOR_RANK[rank];
}

void city_emperor_set_salary_rank(int player_rank)
{
    city_data.emperor.salary_rank = player_rank;
    city_data.emperor.salary_amount = SALARY_FOR_RANK[player_rank];
}

int city_finance_can_afford(int cost)
{
    return -cost + city_data.finance.treasury >= -5000;
}

void city_finance_process_export(int price)
{
    city_data.finance.treasury += price;
    city_data.finance.this_year.income.exports += price;
    if (city_data.religion.neptune_double_trade_active) {
        city_data.finance.treasury += price;
        city_data.finance.this_year.income.exports += price;
    }
}

void city_finance_process_donation(int amount)
{
    city_data.finance.treasury += amount;
    city_data.finance.this_year.income.donated += amount;
}

void city_finance_process_misc(int cost)
{
    city_data.finance.treasury -= cost;
    city_data.finance.this_year.expenses.sundries += cost;
}

void city_finance_process_construction(int cost)
{
    city_data.finance.treasury -= cost;
    city_data.finance.this_year.expenses.construction += cost;
}

void city_finance_calculate_totals(void)
{
    struct finance_overview_t *this_year = &city_data.finance.this_year;
    this_year->income.total =
        this_year->income.donated +
        this_year->income.taxes +
        this_year->income.exports;

    this_year->expenses.total =
        this_year->expenses.sundries +
        this_year->expenses.salary +
        this_year->expenses.interest +
        this_year->expenses.construction +
        this_year->expenses.wages +
        this_year->expenses.imports;

    struct finance_overview_t *last_year = &city_data.finance.last_year;
    last_year->net_in_out = last_year->income.total - last_year->expenses.total;
    this_year->net_in_out = this_year->income.total - this_year->expenses.total;
    this_year->balance = last_year->balance + this_year->net_in_out;

    this_year->expenses.tribute = 0;
}

void city_finance_estimate_wages(void)
{
    int monthly_wages = city_data.labor.wages * city_data.labor.workers_employed / 10 / 12;
    city_data.finance.this_year.expenses.wages = city_data.finance.wages_so_far;
    city_data.finance.estimated_wages = (12 - game_time_month()) * monthly_wages + city_data.finance.wages_so_far;
}

void city_finance_estimate_taxes(void)
{
    city_data.taxes.monthly.collected_plebs = 0;
    city_data.taxes.monthly.collected_patricians = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size && b->house_tax_coverage) {
            int is_patrician = b->subtype.house_level >= HOUSE_SMALL_VILLA;
            if (is_patrician) {
                city_data.taxes.monthly.collected_patricians += b->house_population * house_properties[b->subtype.house_level].tax_multiplier;
            } else {
                city_data.taxes.monthly.collected_plebs += b->house_population * house_properties[b->subtype.house_level].tax_multiplier;
            }
        }
    }
    int monthly_patricians = calc_adjust_with_percentage(
        city_data.taxes.monthly.collected_patricians / 2,
        city_data.finance.tax_percentage);
    int monthly_plebs = calc_adjust_with_percentage(
        city_data.taxes.monthly.collected_plebs / 2,
        city_data.finance.tax_percentage);
    int estimated_rest_of_year = (12 - game_time_month()) * (monthly_patricians + monthly_plebs);

    city_data.finance.this_year.income.taxes =
        city_data.taxes.yearly.collected_plebs + city_data.taxes.yearly.collected_patricians;
    city_data.finance.estimated_tax_income = city_data.finance.this_year.income.taxes + estimated_rest_of_year;
}

static int get_amount(struct building_t *granary, int resource)
{
    if (!resource_is_food(resource)) {
        return 0;
    }
    if (granary->type != BUILDING_GRANARY) {
        return 0;
    }
    return granary->data.granary.resource_stored[resource];
}

static void cause_invasion_mars(int enemy_amount)
{
    int grid_offset = start_invasion(ENEMY_TYPE_BARBARIAN, enemy_amount, MAX_INVASION_POINTS, FORMATION_ATTACK_FOOD_CHAIN);
    if (grid_offset > 0) {
        city_message_post(1, MESSAGE_LOCAL_UPRISING_MARS, 0, grid_offset);
    }
}

static void building_curse_farms(int big_curse)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->output_resource_id && building_is_farm(b->type)) {
            b->data.industry.progress = 0;
            b->data.industry.blessing_days_left = 0;
            b->data.industry.curse_days_left = big_curse ? 48 : 4;
            update_farm_image(b);
        }
    }
}

static void building_granary_warehouse_curse(int big)
{
    int max_stored = 0;
    struct building_t *max_building = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        int total_stored = 0;
        if (b->type == BUILDING_WAREHOUSE) {
            for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                total_stored += building_warehouse_get_amount(b, r);
            }
        } else if (b->type == BUILDING_GRANARY) {
            for (int r = RESOURCE_WHEAT; r < FOOD_TYPES_MAX; r++) {
                total_stored += get_amount(b, r);
            }
            total_stored /= UNITS_PER_LOAD;
        } else {
            continue;
        }
        if (total_stored > max_stored) {
            max_stored = total_stored;
            max_building = b;
        }
    }
    if (!max_building) {
        return;
    }
    if (big) {
        should_play_sound = 0;
        city_message_post(0, MESSAGE_FIRE, max_building->type, max_building->grid_offset);
        building_destroy_by_fire(max_building);
        play_sound_effect(SOUND_EFFECT_EXPLOSION);
        map_routing_update_land();
    } else {
        if (max_building->type == BUILDING_WAREHOUSE) {
            int amount = CURSE_LOADS;
            struct building_t *space = max_building;
            for (int i = 0; i < 8 && amount > 0; i++) {
                space = &all_buildings[space->next_part_building_id];
                if (space->id <= 0 || space->loads_stored <= 0) {
                    continue;
                }
                int resource = space->subtype.warehouse_resource_id;
                if (space->loads_stored > amount) {
                    city_resource_remove_from_warehouse(resource, amount);
                    space->loads_stored -= amount;
                    amount = 0;
                } else {
                    city_resource_remove_from_warehouse(resource, space->loads_stored);
                    amount -= space->loads_stored;
                    space->loads_stored = 0;
                    space->subtype.warehouse_resource_id = RESOURCE_NONE;
                }
                building_warehouse_space_set_image(space, resource);
            }
        } else if (max_building->type == BUILDING_GRANARY) {
            int amount = building_granary_remove_resource(max_building, RESOURCE_WHEAT, CURSE_LOADS * UNITS_PER_LOAD);
            amount = building_granary_remove_resource(max_building, RESOURCE_VEGETABLES, amount);
            amount = building_granary_remove_resource(max_building, RESOURCE_FRUIT, amount);
            building_granary_remove_resource(max_building, RESOURCE_MEAT, amount);
        }
    }
}

static void city_sentiment_set_max_happiness(int max)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size) {
            if (b->sentiment.house_happiness > max) {
                b->sentiment.house_happiness = max;
            }
            b->sentiment.house_happiness = calc_bound(b->sentiment.house_happiness, 0, 100);
        }
    }
}

static void figure_sink_all_ships(void)
{
    for (int i = 1; i < MAX_FIGURES; i++) {
        struct figure_t *f = &figures[i];
        if (!figure_is_alive(f)) {
            continue;
        }
        if (f->type == FIGURE_TRADE_SHIP) {
            all_buildings[f->destination_building_id].data.dock.trade_ship_id = 0;
        } else if (f->type == FIGURE_FISHING_BOAT) {
            all_buildings[f->building_id].data.industry.fishing_boat_id = 0;
        } else {
            continue;
        }
        f->building_id = 0;
        f->type = FIGURE_SHIPWRECK;
        f->wait_ticks = 0;
    }
}

void city_gods_calculate_moods(int update_moods)
{
    // base happiness: percentage of houses covered
    for (int i = 0; i < MAX_GODS; i++) {
        city_data.religion.gods[i].target_happiness = city_culture_coverage_religion(i);
    }

    int max_temples = 0;
    int max_god = TIE;
    int min_temples = 100000;
    int min_god = TIE;
    for (int i = 0; i < MAX_GODS; i++) {
        int num_temples = 0;
        switch (i) {
            case GOD_CERES:
                num_temples = building_count_total(BUILDING_SMALL_TEMPLE_CERES)
                    + building_count_total(BUILDING_LARGE_TEMPLE_CERES);
                break;
            case GOD_NEPTUNE:
                num_temples = building_count_total(BUILDING_SMALL_TEMPLE_NEPTUNE)
                    + building_count_total(BUILDING_LARGE_TEMPLE_NEPTUNE);
                break;
            case GOD_MERCURY:
                num_temples = building_count_total(BUILDING_SMALL_TEMPLE_MERCURY)
                    + building_count_total(BUILDING_LARGE_TEMPLE_MERCURY);
                break;
            case GOD_MARS:
                num_temples = building_count_total(BUILDING_SMALL_TEMPLE_MARS)
                    + building_count_total(BUILDING_LARGE_TEMPLE_MARS);
                break;
            case GOD_VENUS:
                num_temples = building_count_total(BUILDING_SMALL_TEMPLE_VENUS)
                    + building_count_total(BUILDING_LARGE_TEMPLE_VENUS);
                break;
        }
        if (num_temples == max_temples) {
            max_god = TIE;
        } else if (num_temples > max_temples) {
            max_temples = num_temples;
            max_god = i;
        }
        if (num_temples == min_temples) {
            min_god = TIE;
        } else if (num_temples < min_temples) {
            min_temples = num_temples;
            min_god = i;
        }
    }
    // happiness factor based on months since festival (max 40)
    for (int i = 0; i < MAX_GODS; i++) {
        int festival_penalty = city_data.religion.gods[i].months_since_festival;
        if (festival_penalty > 40) {
            festival_penalty = 40;
        }
        city_data.religion.gods[i].target_happiness += 12 - festival_penalty;
    }

    if (max_god < MAX_GODS) {
        if (city_data.religion.gods[max_god].target_happiness >= 50) {
            city_data.religion.gods[max_god].target_happiness = 100;
        } else {
            city_data.religion.gods[max_god].target_happiness += 50;
        }
    }
    if (min_god < MAX_GODS) {
        city_data.religion.gods[min_god].target_happiness -= 25;
    }
    int min_happiness;
    if (city_data.population.population < 100) {
        min_happiness = 50;
    } else if (city_data.population.population < 200) {
        min_happiness = 40;
    } else if (city_data.population.population < 300) {
        min_happiness = 30;
    } else if (city_data.population.population < 400) {
        min_happiness = 20;
    } else if (city_data.population.population < 500) {
        min_happiness = 10;
    } else {
        min_happiness = 0;
    }
    for (int i = 0; i < MAX_GODS; i++) {
        city_data.religion.gods[i].target_happiness =
            calc_bound(city_data.religion.gods[i].target_happiness, min_happiness, 100);
    }
    if (update_moods) {
        for (int i = 0; i < MAX_GODS; i++) {
            struct god_status_t *god = &city_data.religion.gods[i];
            if (god->happiness < god->target_happiness) {
                god->happiness++;
            } else if (god->happiness > god->target_happiness) {
                god->happiness--;
            }
            if (god->happiness > 50) {
                god->small_curse_done = 0;
            }
            if (god->happiness < 50) {
                god->blessing_done = 0;
            }
        }
        int god_id = random_byte() & 7;
        if (god_id < MAX_GODS) {
            struct god_status_t *god = &city_data.religion.gods[god_id];
            if (god->happiness >= 50) {
                god->wrath_bolts = 0;
            } else if (god->happiness < 40) {
                if (god->happiness >= 20) {
                    god->wrath_bolts += 1;
                } else if (god->happiness >= 10) {
                    god->wrath_bolts += 2;
                } else {
                    god->wrath_bolts += 5;
                }
            }
            if (god->wrath_bolts > 50) {
                god->wrath_bolts = 50;
            }
        }
        if (game_time_day() != 0) {
            return;
        }
        // handle blessings, curses, etc every month
        for (int i = 0; i < MAX_GODS; i++) {
            city_data.religion.gods[i].months_since_festival++;
        }
        if (god_id >= MAX_GODS) {
            if (city_gods_calculate_least_happy()) {
                god_id = city_data.religion.least_happy_god - 1;
            }
        }
        if (god_id < MAX_GODS) {
            struct god_status_t *god = &city_data.religion.gods[god_id];
            if (god->happiness >= 100 && !god->blessing_done) {
                god->blessing_done = 1;
                switch (god_id) {
                    case GOD_CERES:
                        city_message_post(1, MESSAGE_BLESSING_FROM_CERES, 0, 0);
                        for (int i = 1; i < MAX_BUILDINGS; i++) {
                            struct building_t *b = &all_buildings[i];
                            if (b->state == BUILDING_STATE_IN_USE && b->output_resource_id && building_is_farm(b->type)) {
                                b->data.industry.progress = MAX_PROGRESS_RAW;
                                b->data.industry.curse_days_left = 0;
                                b->data.industry.blessing_days_left = 16;
                                update_farm_image(b);
                            }
                        }
                        break;
                    case GOD_NEPTUNE:
                        city_message_post(1, MESSAGE_BLESSING_FROM_NEPTUNE, 0, 0);
                        city_data.religion.neptune_double_trade_active = 1;
                        break;
                    case GOD_MERCURY:
                        city_message_post(1, MESSAGE_BLESSING_FROM_MERCURY, 0, 0);
                        int min_stored = INFINITE;
                        struct building_t *min_building = 0;
                        for (int i = 1; i < MAX_BUILDINGS; i++) {
                            struct building_t *b = &all_buildings[i];
                            if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_GRANARY) {
                                continue;
                            }
                            int total_stored = 0;
                            for (int r = RESOURCE_WHEAT; r < FOOD_TYPES_MAX; r++) {
                                total_stored += get_amount(b, r);
                            }
                            if (total_stored < min_stored) {
                                min_stored = total_stored;
                                min_building = b;
                            }
                        }
                        if (min_building) {
                            for (int n = 0; n < 6; n++) {
                                building_granary_add_resource(min_building, RESOURCE_WHEAT, 0);
                            }
                            for (int n = 0; n < 6; n++) {
                                building_granary_add_resource(min_building, RESOURCE_VEGETABLES, 0);
                            }
                            for (int n = 0; n < 6; n++) {
                                building_granary_add_resource(min_building, RESOURCE_FRUIT, 0);
                            }
                            for (int n = 0; n < 6; n++) {
                                building_granary_add_resource(min_building, RESOURCE_MEAT, 0);
                            }
                        }
                        break;
                    case GOD_MARS:
                        city_message_post(1, MESSAGE_BLESSING_FROM_MARS, 0, 0);
                        city_data.religion.mars_spirit_power = 10;
                        break;
                    case GOD_VENUS:
                        city_message_post(1, MESSAGE_BLESSING_FROM_VENUS, 0, 0);
                        city_sentiment_change_happiness(25);
                        break;
                }
            } else if (god->wrath_bolts >= 20 && !god->small_curse_done && god->months_since_festival > 3) {
                god->small_curse_done = 1;
                god->wrath_bolts = 0;
                god->happiness += 12;
                switch (god_id) {
                    case GOD_CERES:
                        city_message_post(1, MESSAGE_CERES_IS_UPSET, 0, 0);
                        building_curse_farms(0);
                        break;
                    case GOD_NEPTUNE:
                        city_message_post(1, MESSAGE_NEPTUNE_IS_UPSET, 0, 0);
                        figure_sink_all_ships();
                        city_data.religion.neptune_sank_ships = 1;
                        break;
                    case GOD_MERCURY:
                        city_message_post(1, MESSAGE_MERCURY_IS_UPSET, 0, 0);
                        building_granary_warehouse_curse(0);
                        break;
                    case GOD_MARS:
                        city_message_post(1, MESSAGE_MARS_IS_UPSET, 0, 0);
                        cause_invasion_mars(MARS_INVASION_SMALL);
                        break;
                    case GOD_VENUS:
                        city_message_post(1, MESSAGE_VENUS_IS_UPSET, 0, 0);
                        city_sentiment_set_max_happiness(50);
                        city_sentiment_change_happiness(-5);
                        city_data.health.value = calc_bound(city_data.health.value - 10, 0, 100);
                        city_sentiment_update();
                        break;
                }
            } else if (god->wrath_bolts >= 50 && god->months_since_festival > 3) {
                god->wrath_bolts = 0;
                god->happiness += 30;
                switch (god_id) {
                    case GOD_CERES:
                        city_message_post(1, MESSAGE_WRATH_OF_CERES, 0, 0);
                        building_curse_farms(1);
                        break;
                    case GOD_NEPTUNE:
                        if (city_data.trade.num_sea_routes <= 0) {
                            city_message_post(1, MESSAGE_WRATH_OF_NEPTUNE_NO_SEA_TRADE, 0, 0);
                            return;
                        } else {
                            city_message_post(1, MESSAGE_WRATH_OF_NEPTUNE, 0, 0);
                            figure_sink_all_ships();
                            city_data.religion.neptune_sank_ships = 1;
                            city_data.trade.sea_trade_problem_duration = 80;
                        }
                        break;
                    case GOD_MERCURY:
                        city_message_post(1, MESSAGE_WRATH_OF_MERCURY, 0, 0);
                        building_granary_warehouse_curse(1);
                        break;
                    case GOD_MARS:
                    {
                        struct formation_t *best_legion = 0;
                        int best_legion_weight = 0;
                        for (int i = 0; i < MAX_LEGIONS; i++) {
                            if (legion_formations[i].in_use) {
                                int weight = legion_formations[i].num_figures;
                                if (legion_formations[i].figure_type == FIGURE_FORT_LEGIONARY) {
                                    weight *= 2;
                                }
                                if (weight > best_legion_weight) {
                                    best_legion_weight = weight;
                                    best_legion = &legion_formations[i];
                                }
                            }
                        }
                        if (best_legion) {
                            for (int i = 0; i < best_legion->max_figures; i++) {
                                if (best_legion->figures[i]) {
                                    struct figure_t *f = &figures[best_legion->figures[i]];
                                    struct map_point_t nearest_barracks_road_tile = { 0 };
                                    set_destination__closest_building_of_type(best_legion->building_id, BUILDING_BARRACKS, &nearest_barracks_road_tile);
                                    figure_route_remove(f);
                                    if (nearest_barracks_road_tile.x) {
                                        f->destination_x = nearest_barracks_road_tile.x;
                                        f->destination_y = nearest_barracks_road_tile.y;
                                    } else {
                                        f->destination_x = scenario.exit_point.x;
                                        f->destination_y = scenario.exit_point.y;
                                    }
                                    f->action_state = FIGURE_ACTION_SOLDIER_RETURNING_TO_BARRACKS;
                                }
                            }
                            best_legion->cursed_by_mars = 96;
                            city_message_post(1, MESSAGE_WRATH_OF_MARS, 0, 0);
                        } else {
                            city_message_post(1, MESSAGE_WRATH_OF_MARS_NO_MILITARY, 0, 0);
                        }
                        cause_invasion_mars(MARS_INVASION_LARGE);
                        break;
                    }
                    case GOD_VENUS:
                        city_message_post(1, MESSAGE_WRATH_OF_VENUS, 0, 0);
                        city_sentiment_set_max_happiness(40);
                        city_sentiment_change_happiness(-10);
                        if (city_data.health.value >= 80) {
                            city_data.health.value = calc_bound(city_data.health.value - 50, 0, 100);
                        } else if (city_data.health.value >= 60) {
                            city_data.health.value = calc_bound(city_data.health.value - 40, 0, 100);
                        } else {
                            city_data.health.value = calc_bound(city_data.health.value - 20, 0, 100);
                        }
                        city_data.religion.venus_curse_active = 1;
                        city_sentiment_update();
                        break;
                }
            }
        }
        min_happiness = 100;
        for (int i = 0; i < MAX_GODS; i++) {
            if (city_data.religion.gods[i].happiness < min_happiness) {
                min_happiness = city_data.religion.gods[i].happiness;
            }
        }
        if (city_data.religion.angry_message_delay) {
            city_data.religion.angry_message_delay--;
        } else if (min_happiness < 30) {
            city_data.religion.angry_message_delay = 20;
            if (min_happiness < 10) {
                city_message_post(0, MESSAGE_GODS_WRATHFUL, 0, 0);
            } else {
                city_message_post(0, MESSAGE_GODS_UNHAPPY, 0, 0);
            }
        }
    }
}

int city_gods_calculate_least_happy(void)
{
    int max_god = 0;
    int max_wrath = 0;
    for (int i = 0; i < MAX_GODS; i++) {
        if (city_data.religion.gods[i].wrath_bolts > max_wrath) {
            max_god = i + 1;
            max_wrath = city_data.religion.gods[i].wrath_bolts;
        }
    }
    if (max_god > 0) {
        city_data.religion.least_happy_god = max_god;
        return 1;
    }
    int min_happiness = 40;
    for (int i = 0; i < MAX_GODS; i++) {
        if (city_data.religion.gods[i].happiness < min_happiness) {
            max_god = i + 1;
            min_happiness = city_data.religion.gods[i].happiness;
        }
    }
    city_data.religion.least_happy_god = max_god;
    return max_god > 0;
}

void city_health_update(void)
{
    if (city_data.population.population < 200) {
        city_data.health.value = 50;
        city_data.health.target_value = 50;
        return;
    }
    int total_population = 0;
    int healthy_population = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || !b->house_size || !b->house_population) {
            continue;
        }
        total_population += b->house_population;
        if (b->subtype.house_level <= HOUSE_LARGE_TENT) {
            if (b->data.house.clinic) {
                healthy_population += b->house_population;
            } else {
                healthy_population += b->house_population / 4;
            }
        } else if (b->data.house.clinic) {
            if (b->house_days_without_food == 0) {
                healthy_population += b->house_population;
            } else {
                healthy_population += b->house_population / 4;
            }
        } else if (b->house_days_without_food == 0) {
            healthy_population += b->house_population / 4;
        }
    }
    city_data.health.target_value = calc_percentage(healthy_population, total_population);
    if (city_data.health.value < city_data.health.target_value) {
        city_data.health.value += 2;
        if (city_data.health.value > city_data.health.target_value) {
            city_data.health.value = city_data.health.target_value;
        }
    } else if (city_data.health.value > city_data.health.target_value) {
        city_data.health.value -= 2;
        if (city_data.health.value < city_data.health.target_value) {
            city_data.health.value = city_data.health.target_value;
        }
    }
    city_data.health.value = calc_bound(city_data.health.value, 0, 100);
    if (city_data.health.value >= 40) {
        return;
    }
    int chance_value = random_byte() & 0x3f;
    if (city_data.religion.venus_curse_active) {
        // force plague
        chance_value = 0;
        city_data.religion.venus_curse_active = 0;
    }
    if (chance_value > 40 - city_data.health.value) {
        return;
    }
    int sick_people = calc_adjust_with_percentage(total_population, 7 + (random_byte() & 3));
    if (sick_people <= 0) {
        return;
    }
    city_data.health.value = calc_bound(city_data.health.value + 10, 0, 100);
    int people_to_kill = sick_people - city_data.health.num_hospital_workers;
    if (people_to_kill <= 0) {
        city_message_post(1, MESSAGE_HEALTH_ILLNESS, 0, 0);
        return;
    }
    if (city_data.health.num_hospital_workers > 0) {
        city_message_post(1, MESSAGE_HEALTH_DISEASE, 0, 0);
    } else {
        city_message_post(1, MESSAGE_HEALTH_PESTILENCE, 0, 0);
    }
    // kill people who don't have access to a doctor
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size && b->house_population) {
            if (!b->data.house.clinic) {
                people_to_kill -= b->house_population;
                destroy_on_fire(b, 1);
                if (people_to_kill <= 0) {
                    return;
                }
            }
        }
    }
    // kill people in tents
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size && b->house_population) {
            if (b->subtype.house_level <= HOUSE_LARGE_TENT) {
                people_to_kill -= b->house_population;
                destroy_on_fire(b, 1);
                if (people_to_kill <= 0) {
                    return;
                }
            }
        }
    }
    // kill anyone
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size && b->house_population) {
            people_to_kill -= b->house_population;
            destroy_on_fire(b, 1);
            if (people_to_kill <= 0) {
                return;
            }
        }
    }
}

static int get_people_in_age_decennium(int decennium)
{
    int pop = 0;
    for (int i = 0; i < 10; i++) {
        pop += city_data.population.at_age[10 * decennium + i];
    }
    return pop;
}

void city_labor_calculate_workers(int num_plebs, int num_patricians)
{
    city_data.population.percentage_plebs = calc_percentage(num_plebs, num_plebs + num_patricians);
    city_data.population.working_age = calc_adjust_with_percentage(get_people_in_age_decennium(2) + get_people_in_age_decennium(3) + get_people_in_age_decennium(4), 60);
    city_data.labor.workers_available = calc_adjust_with_percentage(city_data.population.working_age, city_data.population.percentage_plebs);
}

static int should_have_workers(struct building_t *b, int check_access)
{
    if (b->labor_category < 0) {
        return 0;
    }

    if (b->labor_category == LABOR_CATEGORY_ENTERTAINMENT) {
        if (b->type == BUILDING_HIPPODROME && b->prev_part_building_id) {
            return 0;
        }
    } else if (b->labor_category == LABOR_CATEGORY_FOOD_PRODUCTION || b->labor_category == LABOR_CATEGORY_INDUSTRY_COMMERCE) {
        if (b->type >= BUILDING_WHEAT_FARM && b->type <= BUILDING_WEAPONS_WORKSHOP && city_data.resource.mothballed[b->output_resource_id]) {
            return 0;
        }
    }
    // engineering and water are always covered
    if (b->labor_category == LABOR_CATEGORY_ENGINEERING || b->labor_category == LABOR_CATEGORY_WATER) {
        return 1;
    }
    if (check_access) {
        return b->houses_covered > 0 ? 1 : 0;
    }
    return 1;
}

static void allocate_workers_to_categories(void)
{
    int workers_needed = 0;
    for (int i = 0; i < LABOR_CATEGORIES_COUNT; i++) {
        city_data.labor.categories[i].workers_allocated = 0;
        workers_needed += city_data.labor.categories[i].workers_needed;
    }
    city_data.labor.workers_needed = 0;
    if (workers_needed <= city_data.labor.workers_available) {
        for (int i = 0; i < LABOR_CATEGORIES_COUNT; i++) {
            city_data.labor.categories[i].workers_allocated = city_data.labor.categories[i].workers_needed;
        }
        city_data.labor.workers_employed = workers_needed;
    } else {
        // not enough workers
        int available = city_data.labor.workers_available;
        // distribute by user-defined priority
        for (int p = 1; p <= 9 && available > 0; p++) {
            for (int c = 0; c < 9; c++) {
                if (p == city_data.labor.categories[c].priority) {
                    int to_allocate = city_data.labor.categories[c].workers_needed;
                    if (to_allocate > available) {
                        to_allocate = available;
                    }
                    city_data.labor.categories[c].workers_allocated = to_allocate;
                    available -= to_allocate;
                    break;
                }
            }
        }
        // (sort of) round-robin distribution over unprioritized categories:
        int guard = 0;
        do {
            guard++;
            if (guard >= city_data.labor.workers_available) {
                break;
            }
            for (int p = 0; p < 9; p++) {
                int cat = DEFAULT_PRIORITY[p].category;
                if (!city_data.labor.categories[cat].priority) {
                    int needed = city_data.labor.categories[cat].workers_needed
                        - city_data.labor.categories[cat].workers_allocated;
                    if (needed > 0) {
                        int to_allocate = DEFAULT_PRIORITY[p].workers;
                        if (to_allocate > available) {
                            to_allocate = available;
                        }
                        if (to_allocate > needed) {
                            to_allocate = needed;
                        }
                        city_data.labor.categories[cat].workers_allocated += to_allocate;
                        available -= to_allocate;
                        if (available <= 0) {
                            break;
                        }
                    }
                }
            }
        } while (available > 0);

        city_data.labor.workers_employed = city_data.labor.workers_available;
        for (int i = 0; i < 9; i++) {
            city_data.labor.workers_needed +=
                city_data.labor.categories[i].workers_needed - city_data.labor.categories[i].workers_allocated;
        }
    }
    city_data.labor.workers_unemployed = city_data.labor.workers_available - city_data.labor.workers_employed;
    city_data.labor.unemployment_percentage =
        calc_percentage(city_data.labor.workers_unemployed, city_data.labor.workers_available);
}

static void allocate_workers_to_buildings(void)
{
    // set building worker weight
    int water_per_10k_per_building = calc_percentage(100, city_data.labor.categories[LABOR_CATEGORY_WATER].buildings);
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        int cat = building_properties[b->type].labor_category;
        if (cat == LABOR_CATEGORY_WATER) {
            b->percentage_houses_covered = water_per_10k_per_building;
        } else if (cat >= 0) {
            b->percentage_houses_covered = 0;
            if (b->houses_covered) {
                b->percentage_houses_covered =
                    calc_percentage(100 * b->houses_covered, city_data.labor.categories[cat].total_houses_covered);
            }
        }
    }
    // allocate workers to water
    static int start_building_id = 1;
    struct labor_category_data_t *water_cat = &city_data.labor.categories[LABOR_CATEGORY_WATER];

    int percentage_not_filled = 100 - calc_percentage(water_cat->workers_allocated, water_cat->workers_needed);

    int buildings_to_skip = calc_adjust_with_percentage(water_cat->buildings, percentage_not_filled);

    int workers_per_building;
    if (buildings_to_skip == water_cat->buildings) {
        workers_per_building = 1;
    } else {
        workers_per_building = water_cat->workers_allocated / (water_cat->buildings - buildings_to_skip);
    }
    int building_id = start_building_id;
    start_building_id = 0;
    for (int guard = 1; guard < MAX_BUILDINGS; guard++, building_id++) {
        if (building_id >= MAX_BUILDINGS) {
            building_id = 1;
        }
        struct building_t *b = &all_buildings[building_id];
        if (b->state != BUILDING_STATE_IN_USE || building_properties[b->type].labor_category != LABOR_CATEGORY_WATER) {
            continue;
        }
        b->num_workers = 0;
        if (b->percentage_houses_covered > 0) {
            if (percentage_not_filled > 0) {
                if (buildings_to_skip) {
                    --buildings_to_skip;
                } else if (start_building_id) {
                    b->num_workers = workers_per_building;
                } else {
                    start_building_id = building_id;
                    b->num_workers = workers_per_building;
                }
            } else {
                b->num_workers = building_properties[b->type].n_laborers;
            }
        }
    }
    if (!start_building_id) {
        // no buildings assigned or full employment
        start_building_id = 1;
    }
    // allocate workers to non-water buildings
    int category_workers_needed[LABOR_CATEGORIES_COUNT];
    int category_workers_allocated[LABOR_CATEGORIES_COUNT];
    for (int i = 0; i < LABOR_CATEGORIES_COUNT; i++) {
        category_workers_allocated[i] = 0;
        category_workers_needed[i] =
            city_data.labor.categories[i].workers_allocated < city_data.labor.categories[i].workers_needed
            ? 1 : 0;
    }
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        int cat = building_properties[b->type].labor_category;
        if (cat == LABOR_CATEGORY_WATER || cat < 0) {
            // water is handled by allocate workers to water(void)
            continue;
        }
        b->num_workers = 0;
        if (!should_have_workers(b, 0)) {
            continue;
        }
        if (b->percentage_houses_covered > 0) {
            if (category_workers_needed[cat]) {
                int num_workers = calc_adjust_with_percentage(
                    city_data.labor.categories[cat].workers_allocated,
                    b->percentage_houses_covered) / 100;
                if (num_workers > building_properties[b->type].n_laborers) {
                    num_workers = building_properties[b->type].n_laborers;
                }
                b->num_workers = num_workers;
                category_workers_allocated[cat] += num_workers;
            } else {
                b->num_workers = building_properties[b->type].n_laborers;
            }
        }
    }
    for (int i = 0; i < LABOR_CATEGORIES_COUNT; i++) {
        if (category_workers_needed[i]) {
            // watch out: category_workers_needed is now reset to 'unallocated workers available'
            if (category_workers_allocated[i] >= city_data.labor.categories[i].workers_allocated) {
                category_workers_needed[i] = 0;
                category_workers_allocated[i] = 0;
            } else {
                category_workers_needed[i] =
                    city_data.labor.categories[i].workers_allocated - category_workers_allocated[i];
            }
        }
    }
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        int cat = building_properties[b->type].labor_category;
        if (cat < 0 || cat == LABOR_CATEGORY_WATER || cat == LABOR_CATEGORY_MILITARY) {
            continue;
        }
        if (!should_have_workers(b, 0)) {
            continue;
        }
        if (b->percentage_houses_covered > 0 && category_workers_needed[cat]) {
            if (b->num_workers < building_properties[b->type].n_laborers) {
                int needed = building_properties[b->type].n_laborers - b->num_workers;
                if (needed > category_workers_needed[cat]) {
                    b->num_workers += category_workers_needed[cat];
                    category_workers_needed[cat] = 0;
                } else {
                    b->num_workers += needed;
                    category_workers_needed[cat] -= needed;
                }
            }
        }
    }
}

void city_labor_allocate_workers(void)
{
    allocate_workers_to_categories();
    allocate_workers_to_buildings();
}

void city_labor_update(void)
{
    // calculate workers needed per category
    for (int cat = 0; cat < LABOR_CATEGORIES_COUNT; cat++) {
        city_data.labor.categories[cat].buildings = 0;
        city_data.labor.categories[cat].total_houses_covered = 0;
        city_data.labor.categories[cat].workers_allocated = 0;
        city_data.labor.categories[cat].workers_needed = 0;
    }
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        b->labor_category = building_properties[b->type].labor_category;
        if (!should_have_workers(b, 1)) {
            continue;
        }
        city_data.labor.categories[b->labor_category].workers_needed += building_properties[b->type].n_laborers;
        city_data.labor.categories[b->labor_category].total_houses_covered += b->houses_covered;
        city_data.labor.categories[b->labor_category].buildings++;
    }
    // check employment
    int orig_needed = city_data.labor.workers_needed;
    allocate_workers_to_categories();
    // senate unemployment display is delayed when unemployment is rising
    if (city_data.labor.unemployment_percentage < city_data.labor.unemployment_percentage_for_senate) {
        city_data.labor.unemployment_percentage_for_senate = city_data.labor.unemployment_percentage;
    } else if (city_data.labor.unemployment_percentage < city_data.labor.unemployment_percentage_for_senate + 5) {
        city_data.labor.unemployment_percentage_for_senate = city_data.labor.unemployment_percentage;
    } else {
        city_data.labor.unemployment_percentage_for_senate += 5;
    }
    if (city_data.labor.unemployment_percentage_for_senate > 100) {
        city_data.labor.unemployment_percentage_for_senate = 100;
    }
    // workers needed message
    if (!orig_needed && city_data.labor.workers_needed > 0) {
        if (game_time_year() >= scenario.start_year) {
            city_message_post_with_message_delay(MESSAGE_CAT_WORKERS_NEEDED, 0, MESSAGE_WORKERS_NEEDED, 6);
        }
    }

    allocate_workers_to_buildings();
}

void city_labor_set_priority(int category, int new_priority)
{
    int old_priority = city_data.labor.categories[category].priority;
    if (old_priority == new_priority) {
        return;
    }
    int shift;
    int from_prio;
    int to_prio;
    if (!old_priority && new_priority) {
        // shift all bigger than 'new_priority' by one down (+1)
        shift = 1;
        from_prio = new_priority;
        to_prio = 9;
    } else if (old_priority && !new_priority) {
        // shift all bigger than 'old_priority' by one up (-1)
        shift = -1;
        from_prio = old_priority;
        to_prio = 9;
    } else if (new_priority < old_priority) {
        // shift all between new and old by one down (+1)
        shift = 1;
        from_prio = new_priority;
        to_prio = old_priority;
    } else {
        // shift all between old and new by one up (-1)
        shift = -1;
        from_prio = old_priority;
        to_prio = new_priority;
    }
    city_data.labor.categories[category].priority = new_priority;
    for (int i = 0; i < 9; i++) {
        if (i == category) {
            continue;
        }
        int current_priority = city_data.labor.categories[i].priority;
        if (from_prio <= current_priority && current_priority <= to_prio) {
            city_data.labor.categories[i].priority += shift;
        }
    }
    city_labor_allocate_workers();
}

void city_message_init_scenario(void)
{
    for (int i = 0; i < MAX_MESSAGES; i++) {
        message_data.messages[i].message_type = 0;
    }
    for (int i = 0; i < MAX_QUEUE; i++) {
        message_data.queue[i] = 0;
    }
    message_data.consecutive_message_delay = 0;

    message_data.next_message_sequence = 0;
    message_data.total_messages = 0;
    message_data.current_message_id = -1;

    for (int i = 0; i < MAX_MESSAGE_CATEGORIES; i++) {
        message_data.message_count[i] = 0;
        message_data.message_delay[i] = 0;
    }
    // population
    message_data.population_shown.pop500 = 0;
    message_data.population_shown.pop1000 = 0;
    message_data.population_shown.pop2000 = 0;
    message_data.population_shown.pop3000 = 0;
    message_data.population_shown.pop5000 = 0;
    message_data.population_shown.pop10000 = 0;
    message_data.population_shown.pop15000 = 0;
    message_data.population_shown.pop20000 = 0;
    message_data.population_shown.pop25000 = 0;

    for (int i = 0; i <= MESSAGE_CAT_RIOT_COLLAPSE; i++) {
        message_data.last_sound_time[i] = 0;
    }

    city_message_init_problem_areas();
}

void city_message_init_problem_areas(void)
{
    message_data.problem_count = 0;
    message_data.problem_index = 0;
    message_data.problem_last_click_time = time_get_millis();
}

static void play_sound(int text_id)
{
    if (lang_get_message(text_id)->urgent == 1) {
        play_sound_effect(SOUND_EFFECT_FANFARE_URGENT);
    } else {
        play_sound_effect(SOUND_EFFECT_FANFARE);
    }
}

static void show_message_popup(int message_id)
{
    message_data.consecutive_message_delay = 5;
    message_data.messages[message_id].is_read = 1;
    int text_id = city_message_get_text_id(message_data.messages[message_id].message_type);
    const struct lang_message_t *msg = lang_get_message(text_id);
    if (!msg->video.text || !file_exists(0, msg->video.text)) { // does not have video
        play_sound(text_id);
    }
    window_message_dialog_show_city_message(text_id,
        message_data.messages[message_id].year, message_data.messages[message_id].month, message_data.messages[message_id].param1, message_data.messages[message_id].param2,
        city_message_get_advisor(message_data.messages[message_id].message_type), 1);
}

void city_message_apply_sound_interval(int category)
{
    uint32_t now = time_get_millis();
    if (now - message_data.last_sound_time[category] <= 15000) {
        should_play_sound = 0;
    } else {
        message_data.last_sound_time[category] = now;
    }
}

void city_message_post(int use_popup, int message_type, int param1, int param2)
{
    for (int i = 0; i < MAX_MESSAGES; i++) {
        if (!message_data.messages[i].message_type) {
            message_data.current_message_id = i;
            break;
        }
    }
    if (message_data.current_message_id < 0) {
        return;
    }
    message_data.total_messages++;

    message_data.messages[message_data.current_message_id].message_type = message_type;
    message_data.messages[message_data.current_message_id].is_read = 0;
    message_data.messages[message_data.current_message_id].year = game_time_year();
    message_data.messages[message_data.current_message_id].month = game_time_month();
    message_data.messages[message_data.current_message_id].param1 = param1;
    message_data.messages[message_data.current_message_id].param2 = param2;
    message_data.messages[message_data.current_message_id].sequence = message_data.next_message_sequence++;

    int text_id = city_message_get_text_id(message_type);
    int lang_msg_type = lang_get_message(text_id)->message_type;
    if (lang_msg_type == MESSAGE_TYPE_DISASTER || lang_msg_type == MESSAGE_TYPE_INVASION) {
        message_data.problem_count = 1;
        window_invalidate();
    }
    if (use_popup && window_is(WINDOW_CITY)) {
        show_message_popup(message_data.current_message_id);
    } else if (use_popup) {
        // add to queue to be processed when player returns to city
        for (int i = 0; i < MAX_QUEUE; i++) {
            if (!message_data.queue[i]) {
                message_data.queue[i] = message_data.messages[message_data.current_message_id].sequence;
                break;
            }
        }
    } else if (should_play_sound) {
        play_sound(text_id);
    }
    should_play_sound = 1;
}

void city_message_post_with_popup_delay(int category, int message_type, int param1, short param2)
{
    int use_popup = 0;
    if (message_data.message_delay[category] <= 0) {
        use_popup = 1;
        message_data.message_delay[category] = 12;
    }
    city_message_post(use_popup, message_type, param1, param2);
    message_data.message_count[category]++;
}

void city_message_post_with_message_delay(int category, int use_popup, int message_type, int delay)
{
    if (category == MESSAGE_CAT_FISHING_BLOCKED || category == MESSAGE_CAT_NO_WORKING_DOCK) {
        // bug in the original game: delays for 'fishing blocked' and 'no working dock'
        // are stored in message_count with manual countdown
        if (message_data.message_count[category] > 0) {
            message_data.message_count[category]--;
        } else {
            message_data.message_count[category] = delay;
            city_message_post(use_popup, message_type, 0, 0);
        }
    } else {
        if (message_data.message_delay[category] <= 0) {
            message_data.message_delay[category] = delay;
            city_message_post(use_popup, message_type, 0, 0);
        }
    }
}

void city_message_process_queue(void)
{
    if (message_data.consecutive_message_delay > 0) {
        message_data.consecutive_message_delay--;
        return;
    }
    int sequence = 0;
    for (int i = 0; i < MAX_QUEUE; i++) {
        if (message_data.queue[i]) {
            sequence = message_data.queue[i];
            message_data.queue[i] = 0;
            break;
        }
    }
    if (sequence == 0) {
        return;
    }
    int message_id = -1;
    for (int i = 0; i < 999; i++) {
        if (!message_data.messages[i].message_type) {
            return;
        }
        if (message_data.messages[i].sequence == sequence) {
            message_id = i;
            break;
        }
    }
    if (message_id >= 0) {
        show_message_popup(message_id);
    }
}

void city_message_sort_and_compact(void)
{
    for (int i = 0; i < MAX_MESSAGES; i++) {
        for (int a = 0; a < MAX_MESSAGES - 1; a++) {
            int swap = 0;
            if (message_data.messages[a].message_type) {
                if (message_data.messages[a].sequence < message_data.messages[a + 1].sequence) {
                    if (message_data.messages[a + 1].message_type) {
                        swap = 1;
                    }
                }
            } else if (message_data.messages[a + 1].message_type) {
                swap = 1;
            }
            if (swap) {
                struct city_message_t tmp_message = message_data.messages[a];
                message_data.messages[a] = message_data.messages[a + 1];
                message_data.messages[a + 1] = tmp_message;
            }
        }
    }
    message_data.total_messages = 0;
    for (int i = 0; i < MAX_MESSAGES; i++) {
        if (message_data.messages[i].message_type) {
            message_data.total_messages++;
        }
    }
}

int city_message_get_text_id(int message_type)
{
    if (message_type > 50) {
        return message_type + 199;
    } else {
        return message_type + 99;
    }
}

int city_message_get_advisor(int message_type)
{
    switch (message_type) {
        case MESSAGE_LOCAL_UPRISING:
        case MESSAGE_BARBARIAN_ATTACK:
        case MESSAGE_CAESAR_ARMY_ATTACK:
        case MESSAGE_CAESAR_ARMY_CONTINUE:
        case MESSAGE_CAESAR_ARMY_RETREAT:
        case MESSAGE_ENEMY_ARMY_ATTACK:
        case MESSAGE_DISTANT_BATTLE:
        case MESSAGE_ENEMIES_CLOSING:
        case MESSAGE_ENEMIES_AT_THE_DOOR:
        case MESSAGE_TROOPS_RETURN_VICTORIOUS:
        case MESSAGE_TROOPS_RETURN_FAILED:
        case MESSAGE_DISTANT_BATTLE_LOST_NO_TROOPS:
        case MESSAGE_DISTANT_BATTLE_LOST_TOO_LATE:
        case MESSAGE_DISTANT_BATTLE_LOST_TOO_WEAK:
        case MESSAGE_DISTANT_BATTLE_CITY_RETAKEN:
        case MESSAGE_SPIRIT_OF_MARS:
        case MESSAGE_GLADIATOR_REVOLT:
        case MESSAGE_GLADIATOR_REVOLT_FINISHED:
            return MESSAGE_ADVISOR_MILITARY;
        case MESSAGE_CAESAR_REQUESTS_GOODS:
        case MESSAGE_CAESAR_REQUESTS_MONEY:
        case MESSAGE_CAESAR_REQUESTS_ARMY:
        case MESSAGE_REQUEST_REMINDER:
        case MESSAGE_REQUEST_RECEIVED:
        case MESSAGE_REQUEST_REFUSED:
        case MESSAGE_REQUEST_REFUSED_OVERDUE:
        case MESSAGE_REQUEST_RECEIVED_LATE:
        case MESSAGE_REQUEST_CAN_COMPLY:
        case MESSAGE_CAESAR_WRATH:
        case MESSAGE_CAESAR_RESPECT_1:
        case MESSAGE_CAESAR_RESPECT_2:
        case MESSAGE_CAESAR_RESPECT_3:
            return MESSAGE_ADVISOR_IMPERIAL;
        case MESSAGE_UNEMPLOYMENT:
        case MESSAGE_WORKERS_NEEDED:
        case MESSAGE_ROME_LOWERS_WAGES:
        case MESSAGE_ROME_RAISES_WAGES:
            return MESSAGE_ADVISOR_LABOR;
        case MESSAGE_NOT_ENOUGH_FOOD:
        case MESSAGE_FOOD_NOT_DELIVERED:
        case MESSAGE_POPULATION_500:
        case MESSAGE_POPULATION_1000:
        case MESSAGE_POPULATION_2000:
        case MESSAGE_POPULATION_3000:
        case MESSAGE_POPULATION_5000:
        case MESSAGE_POPULATION_10000:
        case MESSAGE_POPULATION_15000:
        case MESSAGE_POPULATION_20000:
        case MESSAGE_POPULATION_25000:
            return MESSAGE_ADVISOR_POPULATION;
        case MESSAGE_HEALTH_ILLNESS:
        case MESSAGE_HEALTH_DISEASE:
        case MESSAGE_HEALTH_PESTILENCE:
        case MESSAGE_CONTAMINATED_WATER:
            return MESSAGE_ADVISOR_HEALTH;
        case MESSAGE_GODS_UNHAPPY:
        case MESSAGE_GODS_WRATHFUL:
        case MESSAGE_GRAND_FESTIVAL:
        case MESSAGE_LARGE_FESTIVAL:
        case MESSAGE_SMALL_FESTIVAL:
        case MESSAGE_BLESSING_FROM_CERES:
        case MESSAGE_BLESSING_FROM_MARS:
        case MESSAGE_BLESSING_FROM_MERCURY:
        case MESSAGE_BLESSING_FROM_NEPTUNE:
        case MESSAGE_BLESSING_FROM_VENUS:
        case MESSAGE_CERES_IS_UPSET:
        case MESSAGE_MARS_IS_UPSET:
        case MESSAGE_MERCURY_IS_UPSET:
        case MESSAGE_NEPTUNE_IS_UPSET:
        case MESSAGE_VENUS_IS_UPSET:
        case MESSAGE_WRATH_OF_CERES:
        case MESSAGE_WRATH_OF_MARS:
        case MESSAGE_WRATH_OF_MARS_NO_MILITARY:
        case MESSAGE_WRATH_OF_MERCURY:
        case MESSAGE_WRATH_OF_NEPTUNE:
        case MESSAGE_WRATH_OF_NEPTUNE_NO_SEA_TRADE:
        case MESSAGE_WRATH_OF_VENUS:
        case MESSAGE_LOCAL_UPRISING_MARS:
            return MESSAGE_ADVISOR_RELIGION;
        case MESSAGE_INCREASED_TRADING:
        case MESSAGE_DECREASED_TRADING:
        case MESSAGE_PRICE_DECREASED:
        case MESSAGE_PRICE_INCREASED:
        case MESSAGE_TRADE_STOPPED:
        case MESSAGE_LAND_TRADE_DISRUPTED_LANDSLIDES:
        case MESSAGE_LAND_TRADE_DISRUPTED_SANDSTORMS:
        case MESSAGE_SEA_TRADE_DISRUPTED:
            return MESSAGE_ADVISOR_TRADE;
        default:
            return MESSAGE_ADVISOR_NONE;
    }
}

void city_message_reset_category_count(int category)
{
    message_data.message_count[category] = 0;
}

void city_message_increase_category_count(int category)
{
    message_data.message_count[category]++;
}

int city_message_get_category_count(int category)
{
    return message_data.message_count[category];
}

void city_message_decrease_delays(void)
{
    for (int i = 0; i < MAX_MESSAGE_CATEGORIES; i++) {
        if (message_data.message_delay[i] > 0) {
            message_data.message_delay[i]--;
        }
    }
}

int city_message_mark_population_shown(int population)
{
    int *field;
    switch (population) {
        case 500: field = &message_data.population_shown.pop500; break;
        case 1000: field = &message_data.population_shown.pop1000; break;
        case 2000: field = &message_data.population_shown.pop2000; break;
        case 3000: field = &message_data.population_shown.pop3000; break;
        case 5000: field = &message_data.population_shown.pop5000; break;
        case 10000: field = &message_data.population_shown.pop10000; break;
        case 15000: field = &message_data.population_shown.pop15000; break;
        case 20000: field = &message_data.population_shown.pop20000; break;
        case 25000: field = &message_data.population_shown.pop25000; break;
        default: return 0;
    }
    if (!*field) {
        *field = 1;
        return 1;
    }
    return 0;
}

struct city_message_t *city_message_get(int message_id)
{
    return &message_data.messages[message_id];
}

int city_message_set_current(int message_id)
{
    return message_data.current_message_id = message_id;
}

void city_message_mark_read(int message_id)
{
    message_data.messages[message_id].is_read = 1;
}

void city_message_delete(int message_id)
{
    message_data.messages[message_id].message_type = 0;
    city_message_sort_and_compact();
}

int city_message_count(void)
{
    return message_data.total_messages;
}

int city_message_problem_area_count(void)
{
    return message_data.problem_count;
}

int city_message_next_problem_area_grid_offset(void)
{
    uint32_t now = time_get_millis();
    if (now - message_data.problem_last_click_time > 3000) {
        message_data.problem_index = 0;
    }
    message_data.problem_last_click_time = now;
    city_message_sort_and_compact();
    message_data.problem_count = 0;
    for (int i = 0; i < 999; i++) {
        struct city_message_t *msg = &message_data.messages[i];
        if (msg->message_type && msg->year >= game_time_year() - 1) {
            const struct lang_message_t *lang_msg = lang_get_message(city_message_get_text_id(msg->message_type));
            int lang_msg_type = lang_msg->message_type;
            if (lang_msg_type == MESSAGE_TYPE_DISASTER || lang_msg_type == MESSAGE_TYPE_INVASION) {
                message_data.problem_count++;
            }
        }
    }
    if (message_data.problem_count <= 0) {
        message_data.problem_index = 0;
        return 0;
    }
    if (message_data.problem_index >= message_data.problem_count) {
        message_data.problem_index = 0;
    }
    int index = 0;
    int current_year = game_time_year();
    for (int i = 0; i < 999; i++) {
        struct city_message_t *msg = &message_data.messages[i];
        if (msg->message_type && msg->year >= current_year - 1) {
            int text_id = city_message_get_text_id(msg->message_type);
            int lang_msg_type = lang_get_message(text_id)->message_type;
            if (lang_msg_type == MESSAGE_TYPE_DISASTER || lang_msg_type == MESSAGE_TYPE_INVASION) {
                index++;
                if (message_data.problem_index < index) {
                    message_data.problem_index++;
                    return msg->param2;
                }
            }
        }
    }
    return 0;
}

void city_message_clear_scroll(void)
{
    message_data.scroll_position = 0;
}

int city_message_scroll_position(void)
{
    return message_data.scroll_position;
}

void city_message_set_scroll_position(int scroll_position)
{
    message_data.scroll_position = scroll_position;
}

void city_message_save_state(struct buffer_t *messages, struct buffer_t *extra, struct buffer_t *counts, struct buffer_t *delays, struct buffer_t *population)
{
    for (int i = 0; i < MAX_MESSAGES; i++) {
        struct city_message_t *msg = &message_data.messages[i];
        buffer_write_i32(messages, msg->param1);
        buffer_write_i16(messages, msg->year);
        buffer_write_i16(messages, msg->param2);
        buffer_write_i16(messages, msg->message_type);
        buffer_write_i16(messages, msg->sequence);
        buffer_write_u8(messages, msg->is_read);
        buffer_write_u8(messages, msg->month);
    }

    buffer_write_i32(extra, message_data.next_message_sequence);
    buffer_write_i32(extra, message_data.total_messages);
    buffer_write_i32(extra, message_data.current_message_id);

    for (int i = 0; i < MAX_MESSAGE_CATEGORIES; i++) {
        buffer_write_i32(counts, message_data.message_count[i]);
        buffer_write_i32(delays, message_data.message_delay[i]);
    }
    // population
    buffer_write_u8(population, message_data.population_shown.pop500);
    buffer_write_u8(population, message_data.population_shown.pop1000);
    buffer_write_u8(population, message_data.population_shown.pop2000);
    buffer_write_u8(population, message_data.population_shown.pop3000);
    buffer_write_u8(population, message_data.population_shown.pop5000);
    buffer_write_u8(population, message_data.population_shown.pop10000);
    buffer_write_u8(population, message_data.population_shown.pop15000);
    buffer_write_u8(population, message_data.population_shown.pop20000);
    buffer_write_u8(population, message_data.population_shown.pop25000);
}

void city_message_load_state(struct buffer_t *messages, struct buffer_t *extra, struct buffer_t *counts, struct buffer_t *delays, struct buffer_t *population)
{
    for (int i = 0; i < MAX_MESSAGES; i++) {
        struct city_message_t *msg = &message_data.messages[i];
        msg->param1 = buffer_read_i32(messages);
        msg->year = buffer_read_i16(messages);
        msg->param2 = buffer_read_i16(messages);
        msg->message_type = buffer_read_i16(messages);
        msg->sequence = buffer_read_i16(messages);
        msg->is_read = buffer_read_u8(messages);
        msg->month = buffer_read_u8(messages);
    }

    message_data.next_message_sequence = buffer_read_i32(extra);
    message_data.total_messages = buffer_read_i32(extra);
    message_data.current_message_id = buffer_read_i32(extra);

    for (int i = 0; i < MAX_MESSAGE_CATEGORIES; i++) {
        message_data.message_count[i] = buffer_read_i32(counts);
        message_data.message_delay[i] = buffer_read_i32(delays);
    }
    // population
    message_data.population_shown.pop500 = buffer_read_u8(population);
    message_data.population_shown.pop1000 = buffer_read_u8(population);
    message_data.population_shown.pop2000 = buffer_read_u8(population);
    message_data.population_shown.pop3000 = buffer_read_u8(population);
    message_data.population_shown.pop5000 = buffer_read_u8(population);
    message_data.population_shown.pop10000 = buffer_read_u8(population);
    message_data.population_shown.pop15000 = buffer_read_u8(population);
    message_data.population_shown.pop20000 = buffer_read_u8(population);
    message_data.population_shown.pop25000 = buffer_read_u8(population);
}

static void figure_create_immigrant(struct building_t *house, int num_people)
{
    struct figure_t *f = figure_create(FIGURE_IMMIGRANT, scenario.entry_point.x, scenario.entry_point.y, DIR_0_TOP);
    f->action_state = FIGURE_ACTION_IMMIGRANT_CREATED;
    f->is_targetable = 1;
    f->terrain_usage = TERRAIN_USAGE_ANY;
    f->immigrant_building_id = house->id;
    f->wait_ticks = 10 + (house->house_figure_generation_delay & 0x7f);
    f->migrant_num_people = num_people;
    house->immigrant_figure_id = f->id;
}

static void create_immigrants(int num_people)
{
    int total_houses = building_list_large_size();
    const int *houses = building_list_large_items();
    int to_immigrate = num_people;
    // clean up any dead immigrants
    for (int i = 0; i < total_houses; i++) {
        struct building_t *b = &all_buildings[houses[i]];
        if (b->immigrant_figure_id && !figure_is_alive(&figures[b->immigrant_figure_id])) {
            b->immigrant_figure_id = 0;
        }
    }
    // houses with plenty of room
    for (int i = 0; i < total_houses && to_immigrate > 0; i++) {
        struct building_t *b = &all_buildings[houses[i]];
        if (b->house_population_room >= 8 && !b->immigrant_figure_id) {
            if (to_immigrate <= 4) {
                figure_create_immigrant(b, to_immigrate);
                to_immigrate = 0;
            } else {
                figure_create_immigrant(b, 4);
                to_immigrate -= 4;
            }
        }
    }
    // houses with less room
    for (int i = 0; i < total_houses && to_immigrate > 0; i++) {
        struct building_t *b = &all_buildings[houses[i]];
        if (b->house_population_room > 0 && !b->immigrant_figure_id) {
            if (to_immigrate <= b->house_population_room) {
                figure_create_immigrant(b, to_immigrate);
                to_immigrate = 0;
            } else {
                figure_create_immigrant(b, b->house_population_room);
                to_immigrate -= b->house_population_room;
            }
        }
    }
    int immigrated = num_people - to_immigrate;
    city_data.migration.immigrated_today += immigrated;
    city_data.migration.newcomers += city_data.migration.immigrated_today;
    if (immigrated == 0) {
        city_data.migration.refused_immigrants_today += num_people;
    }
}

static void create_vacant_lot(int x, int y, int image_id)
{
    struct building_t *b = building_create(BUILDING_HOUSE_VACANT_LOT, x, y);
    b->house_population = 0;
    map_building_tiles_add(b->id, b->x, b->y, 1, image_id, TERRAIN_BUILDING);
}

static void remove_from_census(int num_people)
{
    int index = 0;
    int empty_buckets = 0;
    // remove people randomly up to age 63
    while (num_people > 0 && empty_buckets < 100) {
        int age = random_from_pool(index++) & 0x3f;
        if (city_data.population.at_age[age] <= 0) {
            empty_buckets++;
        } else {
            city_data.population.at_age[age]--;
            num_people--;
            empty_buckets = 0;
        }
    }
    // if random didn't work: remove from age 10 and up
    empty_buckets = 0;
    int age = 10;
    while (num_people > 0 && empty_buckets < 100) {
        if (city_data.population.at_age[age] <= 0) {
            empty_buckets++;
        } else {
            city_data.population.at_age[age]--;
            num_people--;
            empty_buckets = 0;
        }
        age++;
        if (age >= 100) {
            age = 0;
        }
    }
}

static void recalculate_population(void)
{
    city_data.population.population = 0;
    for (int i = 0; i < 100; i++) {
        city_data.population.population += city_data.population.at_age[i];
    }
    if (city_data.population.population > city_data.population.highest_ever) {
        city_data.population.highest_ever = city_data.population.population;
    }
}

static void figure_create_emigrant(struct building_t *house, int num_people)
{
    city_data.population.last_change = -num_people;
    remove_from_census(num_people);
    recalculate_population();
    if (num_people < house->house_population) {
        house->house_population -= num_people;
    } else {
        house->house_population = 0;
        house->type = BUILDING_HOUSE_VACANT_LOT;
        house->subtype.house_level = house->type - BUILDING_HOUSE_SMALL_TENT;
        int image_id = image_group(GROUP_BUILDING_HOUSE_VACANT_LOT);
        if (house->house_is_merged) {
            map_building_tiles_remove(house->id, house->x, house->y);
            house->house_is_merged = 0;
            house->size = house->house_size = 1;
            map_building_tiles_add(house->id, house->x, house->y, 1, image_id, TERRAIN_BUILDING);
            create_vacant_lot(house->x + 1, house->y, image_id);
            create_vacant_lot(house->x, house->y + 1, image_id);
            create_vacant_lot(house->x + 1, house->y + 1, image_id);
        } else {
            map_image_set(house->grid_offset, image_id);
        }
    }
    struct figure_t *f = figure_create(FIGURE_EMIGRANT, house->x, house->y, DIR_0_TOP);
    f->action_state = FIGURE_ACTION_EMIGRANT_CREATED;
    f->is_targetable = 1;
    f->terrain_usage = TERRAIN_USAGE_ANY;
    f->wait_ticks = 0;
    f->migrant_num_people = num_people;
}

static void create_emigrants(int num_people)
{
    int total_houses = building_list_large_size();
    const int *houses = building_list_large_items();
    int to_emigrate = num_people;
    for (int level = HOUSE_SMALL_TENT; level < HOUSE_LARGE_INSULA && to_emigrate > 0; level++) {
        for (int i = 0; i < total_houses && to_emigrate > 0; i++) {
            struct building_t *b = &all_buildings[houses[i]];
            if (b->house_population > 0 && b->subtype.house_level == level) {
                int current_people;
                if (b->house_population >= 4) {
                    current_people = 4;
                } else {
                    current_people = b->house_population;
                }
                if (to_emigrate <= current_people) {
                    figure_create_emigrant(b, to_emigrate);
                    to_emigrate = 0;
                } else {
                    figure_create_emigrant(b, current_people);
                    to_emigrate -= current_people;
                }
            }
        }
    }
    city_data.migration.emigrated_today += (num_people - to_emigrate);
}

void city_migration_update(void)
{
    if (city_data.sentiment.value > 70) {
        city_data.migration.percentage = 100;
    } else if (city_data.sentiment.value > 60) {
        city_data.migration.percentage = 75;
    } else if (city_data.sentiment.value >= 50) {
        city_data.migration.percentage = 50;
    } else if (city_data.sentiment.value > 40) {
        city_data.migration.percentage = 0;
    } else if (city_data.sentiment.value > 30) {
        city_data.migration.percentage = -10;
    } else if (city_data.sentiment.value > 20) {
        city_data.migration.percentage = -25;
    } else {
        city_data.migration.percentage = -50;
    }
    city_data.migration.immigration_amount_per_batch = 0;
    city_data.migration.emigration_amount_per_batch = 0;
    unsigned int population_cap = 200000;
    if (city_data.population.population >= population_cap) {
        city_data.migration.percentage = 0;
    } else if (city_figures_total_invading_enemies() > 3 && city_data.migration.percentage > 0) { // war scares immigrants away
        city_data.migration.percentage = 0;
    } else {
        if (city_data.migration.percentage > 0) {
            // immigration
            if (city_data.migration.emigration_duration) {
                city_data.migration.emigration_duration--;
            } else {
                city_data.migration.immigration_amount_per_batch = calc_adjust_with_percentage(12, city_data.migration.percentage);
                city_data.migration.immigration_duration = 2;
            }
        } else if (city_data.migration.percentage < 0) {
            // emigration
            if (city_data.migration.immigration_duration) {
                city_data.migration.immigration_duration--;
            } else if (city_data.population.population > 100) {
                city_data.migration.emigration_amount_per_batch = calc_adjust_with_percentage(12, -city_data.migration.percentage);
                city_data.migration.emigration_duration = 2;
            }
        }
    }
    city_data.migration.immigrated_today = 0;
    city_data.migration.emigrated_today = 0;
    city_data.migration.refused_immigrants_today = 0;
    if (city_data.migration.immigration_amount_per_batch > 0) {
        if (city_data.migration.immigration_amount_per_batch >= 4) {
            create_immigrants(city_data.migration.immigration_amount_per_batch);
        } else if (city_data.migration.immigration_amount_per_batch
            + city_data.migration.immigration_queue_size >= 4) {
            create_immigrants(city_data.migration.immigration_amount_per_batch
                + city_data.migration.immigration_queue_size);
            city_data.migration.immigration_queue_size = 0;
        } else {
            // queue them for next round
            city_data.migration.immigration_queue_size += city_data.migration.immigration_amount_per_batch;
        }
    }
    if (city_data.migration.emigration_amount_per_batch > 0) {
        if (city_data.migration.emigration_amount_per_batch >= 4) {
            create_emigrants(city_data.migration.emigration_amount_per_batch);
        } else if (city_data.migration.emigration_amount_per_batch + city_data.migration.emigration_queue_size >= 4) {
            create_emigrants(city_data.migration.emigration_amount_per_batch
                + city_data.migration.emigration_queue_size);
            city_data.migration.emigration_queue_size = 0;
            if (!city_data.migration.emigration_message_shown) {
                city_data.migration.emigration_message_shown = 1;
                city_message_post(1, MESSAGE_EMIGRATION, 0, 0);
            }
        } else {
            // queue them for next round
            city_data.migration.emigration_queue_size += city_data.migration.emigration_amount_per_batch;
        }
    }
    city_data.migration.immigration_amount_per_batch = 0;
    city_data.migration.emigration_amount_per_batch = 0;
}

int city_migration_no_room_for_immigrants(void)
{
    return city_data.migration.refused_immigrants_today || city_data.population.room_in_houses <= 0;
}

void city_military_determine_distant_battle_city(void)
{
    for (int i = 0; i < MAX_EMPIRE_OBJECTS; i++) {
        if (empire_objects[i].in_use && empire_objects[i].city_type == EMPIRE_CITY_VULNERABLE_ROMAN) {
            city_data.distant_battle.city = i;
        }
    }
}

int city_military_distant_battle_roman_army_is_traveling(void)
{
    return city_data.distant_battle.roman_months_to_travel_forth > 0 ||
        city_data.distant_battle.roman_months_to_travel_back > 0;
}

static void set_city_foreign(void)
{
    if (city_data.distant_battle.city) {
        empire_objects[city_data.distant_battle.city].city_type = EMPIRE_CITY_DISTANT_FOREIGN;
    }
    city_data.distant_battle.city_foreign_months_left = 24;
}

void city_military_process_distant_battle(void)
{
    if (city_data.distant_battle.months_until_battle > 0) {
        --city_data.distant_battle.months_until_battle;
        if (city_data.distant_battle.months_until_battle > 0) {
            if (city_data.distant_battle.months_until_battle < scenario.empire.distant_battle_enemy_travel_months) {
                city_data.distant_battle.enemy_months_traveled =
                    scenario.empire.distant_battle_enemy_travel_months - city_data.distant_battle.months_until_battle + 1;
            } else {
                city_data.distant_battle.enemy_months_traveled = 1;
            }
            if (city_data.distant_battle.roman_months_to_travel_forth >= 1) {
                if (scenario.empire.distant_battle_roman_travel_months - city_data.distant_battle.roman_months_traveled >
                    scenario.empire.distant_battle_enemy_travel_months - city_data.distant_battle.enemy_months_traveled) {
                    city_data.distant_battle.roman_months_to_travel_forth -= 2;
                } else {
                    city_data.distant_battle.roman_months_to_travel_forth--;
                }
                if (city_data.distant_battle.roman_months_to_travel_forth <= 1) {
                    city_data.distant_battle.roman_months_to_travel_forth = 1;
                }
                city_data.distant_battle.roman_months_traveled =
                    scenario.empire.distant_battle_roman_travel_months - city_data.distant_battle.roman_months_to_travel_forth + 1;
                if (city_data.distant_battle.roman_months_traveled < 1) {
                    city_data.distant_battle.roman_months_traveled = 1;
                }
                if (city_data.distant_battle.roman_months_traveled > scenario.empire.distant_battle_roman_travel_months) {
                    city_data.distant_battle.roman_months_traveled = scenario.empire.distant_battle_roman_travel_months;
                }
            }
        } else {
            if (city_data.distant_battle.roman_months_to_travel_forth <= 0) {
                city_message_post(1, MESSAGE_DISTANT_BATTLE_LOST_NO_TROOPS, 0, 0);
                city_data.ratings.favor = calc_bound(city_data.ratings.favor - 50, 0, 100);
                set_city_foreign();
            } else if (city_data.distant_battle.roman_months_to_travel_forth > 2) {
                city_message_post(1, MESSAGE_DISTANT_BATTLE_LOST_TOO_LATE, 0, 0);
                city_data.ratings.favor = calc_bound(city_data.ratings.favor - 25, 0, 100);
                set_city_foreign();
                city_data.distant_battle.roman_months_to_travel_back = city_data.distant_battle.roman_months_traveled;
            } else {
                int player_won;
                int pct_loss;
                if (city_data.distant_battle.roman_strength < city_data.distant_battle.enemy_strength) {
                    player_won = 0;
                    pct_loss = 100;
                } else {
                    player_won = 1;
                    int pct_advantage = calc_percentage(
                        city_data.distant_battle.roman_strength - city_data.distant_battle.enemy_strength,
                        city_data.distant_battle.roman_strength);
                    if (pct_advantage < 10) {
                        pct_loss = 70;
                    } else if (pct_advantage < 25) {
                        pct_loss = 50;
                    } else if (pct_advantage < 50) {
                        pct_loss = 25;
                    } else if (pct_advantage < 75) {
                        pct_loss = 15;
                    } else if (pct_advantage < 100) {
                        pct_loss = 10;
                    } else if (pct_advantage < 150) {
                        pct_loss = 5;
                    } else {
                        pct_loss = 0;
                    }
                }
                // apply legion losses
                for (int i = 0; i < MAX_LEGIONS; i++) {
                    if (legion_formations[i].in_use && legion_formations[i].in_distant_battle) {
                        struct formation_t *m = &legion_formations[i];
                        m->morale = calc_bound(m->morale - 75, 0, m->max_morale);
                        int soldiers_total = 0;
                        for (int fig = 0; fig < m->num_figures; fig++) {
                            if (m->figures[fig] > 0) {
                                struct figure_t *f = &figures[m->figures[fig]];
                                if (figure_is_alive(f)) {
                                    soldiers_total++;
                                }
                            }
                        }
                        int soldiers_to_kill = calc_adjust_with_percentage(soldiers_total, pct_loss);
                        if (soldiers_to_kill >= soldiers_total) {
                            m->in_distant_battle = 0;
                        }
                        for (int fig = 0; fig < m->num_figures; fig++) {
                            if (m->figures[fig] > 0) {
                                struct figure_t *f = &figures[m->figures[fig]];
                                if (figure_is_alive(f)) {
                                    if (soldiers_to_kill) {
                                        soldiers_to_kill--;
                                        figure_delete(f);
                                    }
                                }
                            }
                        }
                    }
                }
                if (player_won) {
                    if (scenario.allowed_buildings[BUILDING_TRIUMPHAL_ARCH]) {
                        city_message_post(1, MESSAGE_DISTANT_BATTLE_WON, 0, 0);
                        city_data.building.triumphal_arches_available++;
                        build_menus[MENU_ADMINISTRATION].is_enabled = 1; // in case every other item in the menu was disabled
                        build_menus[MENU_ADMINISTRATION].menu_items[10].building_id = BUILDING_TRIUMPHAL_ARCH;
                    } else {
                        city_message_post(1, MESSAGE_DISTANT_BATTLE_WON_TRIUMPHAL_ARCH_DISABLED, 0, 0);
                    }
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor + 25, 0, 100);
                    city_data.distant_battle.won_count++;
                    city_data.distant_battle.city_foreign_months_left = 0;
                    city_data.distant_battle.roman_months_to_travel_back = city_data.distant_battle.roman_months_traveled;
                } else {
                    city_message_post(1, MESSAGE_DISTANT_BATTLE_LOST_TOO_WEAK, 0, 0);
                    city_data.ratings.favor = calc_bound(city_data.ratings.favor - 10, 0, 100);
                    set_city_foreign();
                    city_data.distant_battle.roman_months_traveled = 0;
                }
            }
            city_data.distant_battle.months_until_battle = 0;
            city_data.distant_battle.enemy_months_traveled = 0;
            city_data.distant_battle.roman_months_to_travel_forth = 0;
        }
    } else {
        if (city_data.distant_battle.roman_months_to_travel_back > 0) {
            city_data.distant_battle.roman_months_to_travel_back--;
            city_data.distant_battle.roman_months_traveled = city_data.distant_battle.roman_months_to_travel_back;
            if (city_data.distant_battle.roman_months_to_travel_back <= 0) {
                if (city_data.distant_battle.city_foreign_months_left) {
                    // soldiers return - not in time
                    city_message_post(1, MESSAGE_TROOPS_RETURN_FAILED, 0, map_grid_offset(scenario.exit_point.x, scenario.exit_point.y));
                } else {
                    // victorious
                    city_message_post(1, MESSAGE_TROOPS_RETURN_VICTORIOUS, 0, map_grid_offset(scenario.exit_point.x, scenario.exit_point.y));
                }
                city_data.distant_battle.roman_months_traveled = 0;
                // return soldiers
                for (int i = 0; i < MAX_LEGIONS; i++) {
                    if (legion_formations[i].in_use && legion_formations[i].in_distant_battle) {
                        legion_formations[i].in_distant_battle = 0;
                        for (int fig = 0; fig < legion_formations[i].num_figures; fig++) {
                            if (legion_formations[i].figures[fig] > 0) {
                                struct figure_t *f = &figures[legion_formations[i].figures[fig]];
                                if (figure_is_alive(f)) {
                                    f->action_state = FIGURE_ACTION_SOLDIER_RETURNING_FROM_DISTANT_BATTLE;
                                }
                            }
                        }
                    }
                }
            }
        } else if (city_data.distant_battle.city_foreign_months_left > 0) {
            city_data.distant_battle.city_foreign_months_left--;
            if (city_data.distant_battle.city_foreign_months_left <= 0) {
                city_message_post(1, MESSAGE_DISTANT_BATTLE_CITY_RETAKEN, 0, 0);
                if (city_data.distant_battle.city) {
                    empire_objects[city_data.distant_battle.city].city_type = EMPIRE_CITY_VULNERABLE_ROMAN;
                }
            }
        }
    }
}

static void add_to_census(int num_people)
{
    int odd = 0;
    int index = 0;
    for (int i = 0; i < num_people; i++, odd = 1 - odd) {
        int age = random_from_pool(index++) & 0x3f; // 63
        if (age > 50) {
            age -= 30;
        } else if (age < 10 && odd) {
            age += 20;
        }
        city_data.population.at_age[age]++;
    }
}

void city_population_add(int num_people)
{
    city_data.population.last_change = num_people;
    add_to_census(num_people);
    recalculate_population();
}

void city_population_add_homeless(int num_people)
{
    city_data.population.lost_homeless -= num_people;
    add_to_census(num_people);
    recalculate_population();
}

void city_population_remove_homeless(int num_people)
{
    city_data.population.lost_homeless += num_people;
    remove_from_census(num_people);
    recalculate_population();
}

void city_population_remove_home_removed(int num_people)
{
    city_data.population.lost_removal += num_people;
    remove_from_census(num_people);
    recalculate_population();
}

static int house_population_remove_from_city(int num_people)
{
    int removed = 0;
    int building_id = city_data.population.last_used_house_remove;
    for (int i = 1; i < 4 * MAX_BUILDINGS && removed < num_people; i++) {
        if (++building_id >= MAX_BUILDINGS) {
            building_id = 1;
        }
        struct building_t *b = &all_buildings[building_id];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size) {
            city_data.population.last_used_house_remove = building_id;
            if (b->house_population > 0) {
                ++removed;
                --b->house_population;
            }
        }
    }
    return removed;
}

void city_population_remove_for_troop_request(int num_people)
{
    int removed = house_population_remove_from_city(num_people);
    remove_from_census(removed);
    city_data.population.lost_troop_request += num_people;
    recalculate_population();
}

static int calculate_people_per_house_type(void)
{
    city_data.population.people_in_tents_shacks = 0;
    city_data.population.people_in_villas_palaces = 0;
    city_data.population.people_in_tents = 0;
    city_data.population.people_in_large_insula_and_above = 0;
    int total = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_UNUSED ||
            b->state == BUILDING_STATE_UNDO ||
            b->state == BUILDING_STATE_DELETED_BY_GAME ||
            b->state == BUILDING_STATE_DELETED_BY_PLAYER) {
            continue;
        }
        if (b->house_size) {
            int pop = b->house_population;
            total += pop;
            if (b->subtype.house_level <= HOUSE_LARGE_TENT) {
                city_data.population.people_in_tents += pop;
            }
            if (b->subtype.house_level <= HOUSE_LARGE_SHACK) {
                city_data.population.people_in_tents_shacks += pop;
            }
            if (b->subtype.house_level >= HOUSE_LARGE_INSULA) {
                city_data.population.people_in_large_insula_and_above += pop;
            }
            if (b->subtype.house_level >= HOUSE_SMALL_VILLA) {
                city_data.population.people_in_villas_palaces += pop;
            }
        }
    }
    return total;
}

void city_population_request_yearly_update(void)
{
    city_data.population.yearly_update_requested = 1;
    calculate_people_per_house_type();
}

void city_population_yearly_update(void)
{
    if (city_data.population.yearly_update_requested) {
        int aged100 = city_data.population.at_age[99];
        for (int age = 99; age > 0; age--) {
            city_data.population.at_age[age] = city_data.population.at_age[age - 1];
        }
        city_data.population.at_age[0] = 0;
        city_data.population.yearly_deaths = 0;
        for (int decennium = 9; decennium >= 0; decennium--) {
            int people = get_people_in_age_decennium(decennium);
            int death_percentage = DEATHS_PER_HEALTH_PER_AGE_DECENNIUM[city_data.health.value / 10][decennium];
            int deaths = calc_adjust_with_percentage(people, death_percentage);
            int removed = house_population_remove_from_city(deaths + aged100);
            int empty_buckets = 0;
            int age = 0;
            while (deaths > 0 && empty_buckets < 10) {
                if (city_data.population.at_age[10 * decennium + age] <= 0) {
                    empty_buckets++;
                } else {
                    city_data.population.at_age[10 * decennium + age]--;
                    deaths--;
                    empty_buckets = 0;
                }
                age++;
                if (age >= 10) {
                    age = 0;
                }
            }
            city_data.population.yearly_deaths += removed;
            aged100 = 0;
        }
        city_data.population.yearly_births = 0;
        for (int decennium = 9; decennium >= 0; decennium--) {
            int people = get_people_in_age_decennium(decennium);
            int births = calc_adjust_with_percentage(people, BIRTHS_PER_AGE_DECENNIUM[decennium]);
            int added = 0;
            int building_id = city_data.population.last_used_house_add;
            for (int i = 1; i < MAX_BUILDINGS && added < births; i++) {
                if (++building_id >= MAX_BUILDINGS) {
                    building_id = 1;
                }
                struct building_t *b = &all_buildings[building_id];
                if (b->state == BUILDING_STATE_IN_USE && b->house_size && b->house_population > 0) {
                    city_data.population.last_used_house_add = building_id;
                    int max_people = house_properties[b->subtype.house_level].max_people;
                    if (b->house_is_merged) {
                        max_people *= 4;
                    }
                    if (b->house_population < max_people) {
                        ++added;
                        ++b->house_population;
                        b->house_population_room = max_people - b->house_population;
                    }
                }
            }
            city_data.population.at_age[0] += added;
            city_data.population.yearly_births += added;
        }
        city_data.population.yearly_update_requested = 0;
        city_data.population.population_last_year = city_data.population.population;
        recalculate_population();
        city_data.population.lost_removal = 0;
        city_data.population.total_all_years += city_data.population.population;
        city_data.population.total_years++;
        city_data.population.average_per_year = city_data.population.total_all_years / city_data.population.total_years;
    }
}

static void update_culture_explanation(void)
{
    int min_percentage = 100;
    int reason = 1;
    if (city_data.culture.religion_coverage < min_percentage) {
        min_percentage = city_data.culture.religion_coverage;
        reason = 4;
    }
    int pct_theater = city_culture_coverage_theater();
    if (pct_theater < min_percentage) {
        min_percentage = pct_theater;
        reason = 5;
    }
    int pct_library = city_culture_coverage_library();
    if (pct_library < min_percentage) {
        min_percentage = pct_library;
        reason = 2;
    }
    int pct_school = city_culture_coverage_school();
    if (pct_school < min_percentage) {
        min_percentage = pct_school;
        reason = 1;
    }
    int pct_academy = city_culture_coverage_academy();
    if (pct_academy < min_percentage) {
        reason = 3;
    }
    city_data.ratings.culture_explanation = reason;
}

static int has_made_money(void)
{
    return city_data.finance.last_year.expenses.construction + city_data.finance.treasury >
        city_data.ratings.prosperity_treasury_last_year;
}

static void update_prosperity_explanation(void)
{
    int change = 0;
    int profit = 0;
    // unemployment: -1 for too high, +1 for low
    if (city_data.labor.unemployment_percentage < 5) {
        change += 1;
    } else if (city_data.labor.unemployment_percentage >= 15) {
        change -= 1;
    }
    // losing/earning money: -1 for losing, +5 for profit
    if (has_made_money()) {
        change += 5;
        profit = 1;
    } else {
        change -= 1;
    }
    // food types: +1 for multiple foods
    if (city_data.resource.food_types_eaten >= 2) {
        change += 1;
    }
    // wages: +1 for wages 2+ above Rome, -1 for wages below Rome
    int avg_wage = city_data.finance.wage_rate_paid_last_year / 12;
    if (avg_wage >= city_data.labor.wages_rome + 2) {
        change += 1;
    } else if (avg_wage < city_data.labor.wages_rome) {
        change -= 1;
    }
    // high percentage poor: -1, high percentage rich: +1
    int pct_tents = calc_percentage(city_data.population.people_in_tents_shacks, city_data.population.population);
    if (pct_tents > 30) {
        change -= 1;
    }
    if (calc_percentage(city_data.population.people_in_villas_palaces, city_data.population.population) > 10) {
        change += 1;
    }
    // tribute not paid: -1
    if (city_data.finance.tribute_not_paid_last_year) {
        change -= 1;
    }
    // working hippodrome: +1
    if (city_data.entertainment.hippodrome_shows > 0) {
        change += 1;
    }

    int reason;
    if (city_data.ratings.prosperity <= 0 && game_time_year() == scenario.start_year) {
        reason = 0;
    } else if (city_data.ratings.prosperity >= city_data.ratings.prosperity_max) {
        reason = 1;
    } else if (change > 0) {
        reason = 2;
    } else if (!profit) {
        reason = 3;
    } else if (city_data.labor.unemployment_percentage >= 15) {
        reason = 4;
    } else if (avg_wage < city_data.labor.wages_rome) {
        reason = 5;
    } else if (pct_tents > 30) {
        reason = 6;
    } else if (city_data.finance.tribute_not_paid_last_year) {
        reason = 7;
    } else {
        reason = 9;
    }
    // 8 = for bailout
    city_data.ratings.prosperity_explanation = reason;
}

static void update_peace_explanation(void)
{
    int reason;
    if (city_data.figure.imperial_soldiers) {
        reason = 8; // FIXED: 7+8 interchanged
    } else if (city_data.figure.enemies) {
        reason = 7;
    } else if (city_data.figure.rioters) {
        reason = 6;
    } else {
        if (city_data.ratings.peace < 10) {
            reason = 0;
        } else if (city_data.ratings.peace < 30) {
            reason = 1;
        } else if (city_data.ratings.peace < 60) {
            reason = 2;
        } else if (city_data.ratings.peace < 90) {
            reason = 3;
        } else if (city_data.ratings.peace < 100) {
            reason = 4;
        } else { // >= 100
            reason = 5;
        }
    }
    city_data.ratings.peace_explanation = reason;
}

void city_ratings_update_favor_explanation(void)
{
    city_data.ratings.favor_salary_penalty = 0;
    int salary_delta = city_data.emperor.salary_rank - city_data.emperor.player_rank;
    if (city_data.emperor.player_rank != 0) {
        if (salary_delta > 0) {
            city_data.ratings.favor_salary_penalty = salary_delta + 1;
        }
    } else if (salary_delta > 0) {
        city_data.ratings.favor_salary_penalty = salary_delta;
    }

    if (city_data.ratings.favor_salary_penalty >= 8) {
        city_data.ratings.favor_explanation = 1;
    } else if (city_data.finance.tribute_not_paid_total_years >= 3) {
        city_data.ratings.favor_explanation = 2;
    } else if (city_data.ratings.favor_ignored_request_penalty >= 5) {
        city_data.ratings.favor_explanation = 3;
    } else if (city_data.ratings.favor_salary_penalty >= 5) {
        city_data.ratings.favor_explanation = 4;
    } else if (city_data.finance.tribute_not_paid_total_years >= 2) {
        city_data.ratings.favor_explanation = 5;
    } else if (city_data.ratings.favor_ignored_request_penalty >= 3) {
        city_data.ratings.favor_explanation = 6;
    } else if (city_data.ratings.favor_salary_penalty >= 3) {
        city_data.ratings.favor_explanation = 7;
    } else if (city_data.finance.tribute_not_paid_last_year) {
        city_data.ratings.favor_explanation = 8;
    } else if (city_data.ratings.favor_salary_penalty >= 2) {
        city_data.ratings.favor_explanation = 9;
    } else if (city_data.ratings.favor_salary_penalty) {
        city_data.ratings.favor_explanation = 11;
    } else if (city_data.ratings.favor_change == 2) { // rising
        city_data.ratings.favor_explanation = 12;
    } else if (city_data.ratings.favor_change == 1) { // the same
        city_data.ratings.favor_explanation = 13;
    } else {
        city_data.ratings.favor_explanation = 0;
    }
}

void city_ratings_update_explanations(void)
{
    update_culture_explanation();
    update_prosperity_explanation();
    update_peace_explanation();
    city_ratings_update_favor_explanation();
}

void city_ratings_update(int is_yearly_update)
{
    // update culture rating
    city_data.ratings.culture = 0;
    city_data.ratings.culture_explanation = 0;
    if (city_data.population.population) {
        int pct_theater = city_culture_coverage_theater();
        if (pct_theater >= 100) {
            city_data.ratings.culture += 25;
        } else if (pct_theater > 85) {
            city_data.ratings.culture += 18;
        } else if (pct_theater > 70) {
            city_data.ratings.culture += 12;
        } else if (pct_theater > 50) {
            city_data.ratings.culture += 8;
        } else if (pct_theater > 30) {
            city_data.ratings.culture += 3;
        }
        int pct_religion = city_data.culture.religion_coverage;
        if (pct_religion >= 100) {
            city_data.ratings.culture += 30;
        } else if (pct_religion > 85) {
            city_data.ratings.culture += 22;
        } else if (pct_religion > 70) {
            city_data.ratings.culture += 14;
        } else if (pct_religion > 50) {
            city_data.ratings.culture += 9;
        } else if (pct_religion > 30) {
            city_data.ratings.culture += 3;
        }
        int pct_school = city_culture_coverage_school();
        if (pct_school >= 100) {
            city_data.ratings.culture += 15;
        } else if (pct_school > 85) {
            city_data.ratings.culture += 10;
        } else if (pct_school > 70) {
            city_data.ratings.culture += 6;
        } else if (pct_school > 50) {
            city_data.ratings.culture += 4;
        } else if (pct_school > 30) {
            city_data.ratings.culture += 1;
        }
        int pct_academy = city_culture_coverage_academy();
        if (pct_academy >= 100) {
            city_data.ratings.culture += 10;
        } else if (pct_academy > 85) {
            city_data.ratings.culture += 7;
        } else if (pct_academy > 70) {
            city_data.ratings.culture += 4;
        } else if (pct_academy > 50) {
            city_data.ratings.culture += 2;
        } else if (pct_academy > 30) {
            city_data.ratings.culture += 1;
        }
        int pct_library = city_culture_coverage_library();
        if (pct_library >= 100) {
            city_data.ratings.culture += 20;
        } else if (pct_library > 85) {
            city_data.ratings.culture += 14;
        } else if (pct_library > 70) {
            city_data.ratings.culture += 8;
        } else if (pct_library > 50) {
            city_data.ratings.culture += 4;
        } else if (pct_library > 30) {
            city_data.ratings.culture += 2;
        }
        city_data.ratings.culture = calc_bound(city_data.ratings.culture, 0, 100);
        update_culture_explanation();
    }
    // update favor rating
    city_data.emperor.months_since_gift++;
    if (city_data.emperor.months_since_gift >= 12) {
        city_data.emperor.gift_overdose_penalty = 0;
    }
    // update prosperity
    int points = 0;
    int houses = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state && b->house_size) {
            points += house_properties[b->subtype.house_level].prosperity;
            houses++;
        }
    }
    if (houses > 0) {
        city_data.ratings.prosperity_max = points / houses;
    } else {
        city_data.ratings.prosperity_max = 0;
    }
    if (is_yearly_update) {
        // update favor
        city_data.ratings.favor_salary_penalty = 0;
        city_data.ratings.favor_ignored_request_penalty = 0;
        city_data.ratings.favor -= 2;
        // tribute penalty
        if (city_data.finance.tribute_not_paid_last_year) {
            if (city_data.finance.tribute_not_paid_total_years <= 1) {
                city_data.ratings.favor -= 3;
            } else if (city_data.finance.tribute_not_paid_total_years <= 2) {
                city_data.ratings.favor -= 5;
            } else {
                city_data.ratings.favor -= 8;
            }
        }
        // salary
        int salary_delta = city_data.emperor.salary_rank - city_data.emperor.player_rank;
        if (city_data.emperor.player_rank != 0) {
            if (salary_delta > 0) {
                // salary too high
                city_data.ratings.favor -= salary_delta;
                city_data.ratings.favor_salary_penalty = salary_delta + 1;
            } else if (salary_delta < 0) {
                // salary lower than rank
                city_data.ratings.favor += 1;
            }
        } else if (salary_delta > 0) {
            city_data.ratings.favor -= salary_delta;
            city_data.ratings.favor_salary_penalty = salary_delta;
        }

        if (city_data.ratings.favor < city_data.ratings.favor_last_year) {
            city_data.ratings.favor_change = 0;
        } else if (city_data.ratings.favor == city_data.ratings.favor_last_year) {
            city_data.ratings.favor_change = 1;
        } else {
            city_data.ratings.favor_change = 2;
        }
        city_data.ratings.favor_last_year = city_data.ratings.favor;
        // update prosperity rating
        int change = 0;
        // unemployment: -1 for too high, +1 for low
        if (city_data.labor.unemployment_percentage < 5) {
            change += 1;
        } else if (city_data.labor.unemployment_percentage >= 15) {
            change -= 1;
        }
        // losing/earning money: -1 for losing, +5 for profit
        if (has_made_money()) {
            change += 5;
        } else {
            change -= 1;
        }
        city_data.ratings.prosperity_treasury_last_year = city_data.finance.treasury;
        // food types: +1 for multiple foods
        if (city_data.resource.food_types_eaten >= 2) {
            change += 1;
        }
        // wages: +1 for wages 2+ above Rome, -1 for wages below Rome
        int avg_wage = city_data.finance.wage_rate_paid_last_year / 12;
        if (avg_wage >= city_data.labor.wages_rome + 2) {
            change += 1;
        } else if (avg_wage < city_data.labor.wages_rome) {
            change -= 1;
        }
        // high percentage poor: -1, high percentage rich: +1
        if (calc_percentage(city_data.population.people_in_tents_shacks, city_data.population.population) > 30) {
            change -= 1;
        }
        if (calc_percentage(city_data.population.people_in_villas_palaces, city_data.population.population) > 10) {
            change += 1;
        }
        // tribute not paid: -1
        if (city_data.finance.tribute_not_paid_last_year) {
            change -= 1;
        }
        // working hippodrome: +1
        if (city_data.entertainment.hippodrome_shows > 0) {
            change += 1;
        }
        city_data.ratings.prosperity += change;
        if (city_data.ratings.prosperity > city_data.ratings.prosperity_max) {
            city_data.ratings.prosperity = city_data.ratings.prosperity_max;
        }
        city_data.ratings.prosperity = calc_bound(city_data.ratings.prosperity, 0, 100);
        update_prosperity_explanation();
        // update peace rating
        change = 0;
        if (city_data.ratings.peace_years_of_peace < 2) {
            change += 2;
        } else {
            change += 5;
        }
        if (city_data.ratings.peace_num_criminals) {
            change -= 1;
        }
        if (city_data.ratings.peace_num_rioters) {
            change -= 5;
        }
        if (city_data.ratings.peace_destroyed_buildings) {
            change -= city_data.ratings.peace_destroyed_buildings;
        }
        if (city_data.ratings.peace_num_rioters || city_data.ratings.peace_destroyed_buildings) {
            city_data.ratings.peace_years_of_peace = 0;
        } else {
            city_data.ratings.peace_years_of_peace += 1;
        }
        city_data.ratings.peace_num_criminals = 0;
        city_data.ratings.peace_num_rioters = 0;
        city_data.ratings.peace_destroyed_buildings = 0;
        city_data.ratings.peace = calc_bound(city_data.ratings.peace + change, 0, 100);
        update_peace_explanation();
    }
    city_data.ratings.favor = calc_bound(city_data.ratings.favor, 0, 100);
    city_ratings_update_favor_explanation();
}

void city_sentiment_change_happiness(int amount)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size) {
            b->sentiment.house_happiness = calc_bound(b->sentiment.house_happiness + amount, 0, 100);
        }
    }
}

void city_sentiment_update(void)
{
    unsigned int people_in_houses = calculate_people_per_house_type();
    if (people_in_houses < city_data.population.population) {
        remove_from_census(city_data.population.population - people_in_houses);
    }
    int sentiment_contribution_taxes = SENTIMENT_PER_TAX_RATE[city_data.finance.tax_percentage];
    int sentiment_contribution_wages = 0;
    int wage_diff = city_data.labor.wages - city_data.labor.wages_rome;
    if (wage_diff < 0) {
        sentiment_contribution_wages = wage_diff / 2;
        if (!sentiment_contribution_wages) {
            sentiment_contribution_wages = -1;
        }
    } else if (wage_diff > 7) {
        sentiment_contribution_wages = 4;
    } else if (wage_diff > 4) {
        sentiment_contribution_wages = 3;
    } else if (wage_diff > 1) {
        sentiment_contribution_wages = 2;
    } else if (wage_diff > 0) {
        sentiment_contribution_wages = 1;
    }
    int sentiment_contribution_employment = 1;
    if (city_data.labor.unemployment_percentage > 25) {
        sentiment_contribution_employment = -3;
    } else if (city_data.labor.unemployment_percentage > 17) {
        sentiment_contribution_employment = -2;
    } else if (city_data.labor.unemployment_percentage > 10) {
        sentiment_contribution_employment = -1;
    } else if (city_data.labor.unemployment_percentage > 4) {
        sentiment_contribution_employment = 0;
    }
    int sentiment_penalty_tents;
    int pct_tents = calc_percentage(city_data.population.people_in_tents, city_data.population.population);
    if (city_data.population.people_in_villas_palaces > 0) {
        if (pct_tents >= 57) {
            sentiment_penalty_tents = 0;
        } else if (pct_tents >= 40) {
            sentiment_penalty_tents = -3;
        } else if (pct_tents >= 26) {
            sentiment_penalty_tents = -4;
        } else if (pct_tents >= 10) {
            sentiment_penalty_tents = -5;
        } else {
            sentiment_penalty_tents = -6;
        }
    } else if (city_data.population.people_in_large_insula_and_above > 0) {
        if (pct_tents >= 57) {
            sentiment_penalty_tents = 0;
        } else if (pct_tents >= 40) {
            sentiment_penalty_tents = -2;
        } else if (pct_tents >= 26) {
            sentiment_penalty_tents = -3;
        } else if (pct_tents >= 10) {
            sentiment_penalty_tents = -4;
        } else {
            sentiment_penalty_tents = -5;
        }
    } else {
        if (pct_tents >= 40) {
            sentiment_penalty_tents = 0;
        } else if (pct_tents >= 26) {
            sentiment_penalty_tents = -1;
        } else if (pct_tents >= 10) {
            sentiment_penalty_tents = -2;
        } else {
            sentiment_penalty_tents = -3;
        }
    }
    int houses_calculated = 0;
    int houses_needing_food = 0;
    int total_sentiment_contribution_food = 0;
    int total_sentiment_penalty_tents = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || !b->house_size) {
            continue;
        }
        if (!b->house_population) {
            b->sentiment.house_happiness = 50;
            continue;
        }
        if (city_data.population.population < 300) {
            // small town has no complaints
            sentiment_contribution_employment = 0;
            sentiment_contribution_taxes = 0;
            sentiment_contribution_wages = 0;

            b->sentiment.house_happiness = 50;
            if (city_data.population.population < 200) {
                b->sentiment.house_happiness += 10;
            }
            continue;
        }
        // population >= 300
        houses_calculated++;
        int sentiment_contribution_food = 0;
        int sentiment_contribution_tents = 0;
        if (!house_properties[b->subtype.house_level].food_types) {
            // tents
            b->house_days_without_food = 0;
            sentiment_contribution_tents = sentiment_penalty_tents;
            total_sentiment_penalty_tents += sentiment_penalty_tents;
        } else {
            // shack+
            houses_needing_food++;
            if (b->data.house.num_foods >= 2) {
                sentiment_contribution_food = 2;
                total_sentiment_contribution_food += 2;
                b->house_days_without_food = 0;
            } else if (b->data.house.num_foods >= 1) {
                sentiment_contribution_food = 1;
                total_sentiment_contribution_food += 1;
                b->house_days_without_food = 0;
            } else {
                // needs food but has no food
                if (b->house_days_without_food < 3) {
                    b->house_days_without_food++;
                }
                sentiment_contribution_food = -b->house_days_without_food;
                total_sentiment_contribution_food -= b->house_days_without_food;
            }
        }
        b->sentiment.house_happiness += sentiment_contribution_taxes;
        b->sentiment.house_happiness += sentiment_contribution_wages;
        b->sentiment.house_happiness += sentiment_contribution_employment;
        b->sentiment.house_happiness += sentiment_contribution_food;
        b->sentiment.house_happiness += sentiment_contribution_tents;
        b->sentiment.house_happiness = calc_bound(b->sentiment.house_happiness, 0, 100);
    }

    int sentiment_contribution_food = 0;
    int sentiment_contribution_tents = 0;
    if (houses_needing_food) {
        sentiment_contribution_food = total_sentiment_contribution_food / houses_needing_food;
    }
    if (houses_calculated) {
        sentiment_contribution_tents = total_sentiment_penalty_tents / houses_calculated;
    }

    int total_sentiment = 0;
    int total_houses = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size && b->house_population) {
            total_houses++;
            total_sentiment += b->sentiment.house_happiness;
        }
    }
    if (total_houses) {
        city_data.sentiment.value = total_sentiment / total_houses;
    } else {
        city_data.sentiment.value = 60;
    }
    if (city_data.sentiment.message_delay) {
        city_data.sentiment.message_delay--;
    }
    if (city_data.sentiment.value < 48 && city_data.sentiment.value < city_data.sentiment.previous_value) {
        if (city_data.sentiment.message_delay <= 0) {
            city_data.sentiment.message_delay = 3;
            if (city_data.sentiment.value < 35) {
                city_message_post(0, MESSAGE_PEOPLE_ANGRY, 0, 0);
            } else if (city_data.sentiment.value < 40) {
                city_message_post(0, MESSAGE_PEOPLE_UNHAPPY, 0, 0);
            } else {
                city_message_post(0, MESSAGE_PEOPLE_DISGRUNTLED, 0, 0);
            }
        }
    }

    int worst_sentiment = 0;
    city_data.sentiment.low_mood_cause = LOW_MOOD_CAUSE_NONE;
    if (sentiment_contribution_food < worst_sentiment) {
        worst_sentiment = sentiment_contribution_food;
        city_data.sentiment.low_mood_cause = LOW_MOOD_CAUSE_NO_FOOD;
    }
    if (sentiment_contribution_employment < worst_sentiment) {
        worst_sentiment = sentiment_contribution_employment;
        city_data.sentiment.low_mood_cause = LOW_MOOD_CAUSE_NO_JOBS;
    }
    if (sentiment_contribution_taxes < worst_sentiment) {
        worst_sentiment = sentiment_contribution_taxes;
        city_data.sentiment.low_mood_cause = LOW_MOOD_CAUSE_HIGH_TAXES;
    }
    if (sentiment_contribution_wages < worst_sentiment) {
        worst_sentiment = sentiment_contribution_wages;
        city_data.sentiment.low_mood_cause = LOW_MOOD_CAUSE_LOW_WAGES;
    }
    if (sentiment_contribution_tents < worst_sentiment) {
        city_data.sentiment.low_mood_cause = LOW_MOOD_CAUSE_MANY_TENTS;
    }
    city_data.sentiment.previous_value = city_data.sentiment.value;
}

void city_trade_update(void)
{
    city_data.trade.num_sea_routes = 0;
    city_data.trade.num_land_routes = 0;
    // Wine types
    city_data.resource.wine_types_available = building_count_industry_total(RESOURCE_WINE) > 0 ? 1 : 0;
    if (city_data.resource.trade_status[RESOURCE_WINE] == TRADE_STATUS_IMPORT) {
        for (int i = 0; i < MAX_EMPIRE_OBJECTS; i++) {
            if (empire_objects[i].in_use
                && empire_objects[i].trade_route_open
                && empire_objects[i].resource_sell_limit[RESOURCE_WINE]) {
                city_data.resource.wine_types_available++;
            }
        }
    }
    // Update trade problems
    if (city_data.trade.land_trade_problem_duration > 0) {
        city_data.trade.land_trade_problem_duration--;
    } else {
        city_data.trade.land_trade_problem_duration = 0;
    }
    if (city_data.trade.sea_trade_problem_duration > 0) {
        city_data.trade.sea_trade_problem_duration--;
    } else {
        city_data.trade.sea_trade_problem_duration = 0;
    }

    for (int i = 1; i < MAX_EMPIRE_OBJECTS; i++) {
        if (!empire_objects[i].in_use || !empire_objects[i].trade_route_open) {
            continue;
        }
        if (empire_objects[i].is_sea_trade) {
            if (!city_data.building.working_docks) {
                // delay of 384 = 1 year
                city_message_post_with_message_delay(MESSAGE_CAT_NO_WORKING_DOCK, 1, MESSAGE_NO_WORKING_DOCK, 384);
                continue;
            }
            if (scenario.river_entry_point.x == -1 || scenario.river_entry_point.y == -1) {
                continue;
            }
            city_data.trade.num_sea_routes++;
        } else {
            city_data.trade.num_land_routes++;
        }
        // generate trader
        struct empire_object_t *city = &empire_objects[i];
        int max_traders = 0;
        int num_resources = 0;
        for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
            if (city->resource_buy_limit[r]) {
                ++num_resources;
                switch (city->resource_buy_limit[r]) {
                    case 15: max_traders += 1; break;
                    case 25: max_traders += 2; break;
                    case 40: max_traders += 3; break;
                }
            } else if (city->resource_sell_limit[r]) {
                ++num_resources;
                switch (city->resource_sell_limit[r]) {
                    case 15: max_traders += 1; break;
                    case 25: max_traders += 2; break;
                    case 40: max_traders += 3; break;
                }
            }
        }
        if (num_resources > 1) {
            if (max_traders % num_resources) {
                max_traders = max_traders / num_resources + 1;
            } else {
                max_traders = max_traders / num_resources;
            }
        }
        if (max_traders <= 0) {
            continue;
        }
        int index;
        if (max_traders == 1) {
            if (!city->trader_figure_ids[0]) {
                index = 0;
            } else {
                continue;
            }
        } else if (max_traders == 2) {
            if (!city->trader_figure_ids[0]) {
                index = 0;
            } else if (!city->trader_figure_ids[1]) {
                index = 1;
            } else {
                continue;
            }
        } else { // 3
            if (!city->trader_figure_ids[0]) {
                index = 0;
            } else if (!city->trader_figure_ids[1]) {
                index = 1;
            } else if (!city->trader_figure_ids[2]) {
                index = 2;
            } else {
                continue;
            }
        }
        if (city->trader_entry_delay > 0) {
            city->trader_entry_delay--;
            continue;
        }
        city->trader_entry_delay = city->is_sea_trade ? 30 : 4;
        if (city->is_sea_trade) {
            // generate ship
            if (city_data.building.working_docks && (scenario.river_entry_point.x != -1 && scenario.river_entry_point.y != -1) && !city_data.trade.sea_trade_problem_duration) {
                struct figure_t *ship = figure_create(FIGURE_TRADE_SHIP, scenario.river_entry_point.x, scenario.river_entry_point.y, DIR_0_TOP);
                ship->empire_city_id = city->id;
                ship->action_state = FIGURE_ACTION_TRADE_SHIP_CREATED;
                ship->wait_ticks = 10;
                city->trader_figure_ids[index] = ship->id;
                break;
            }
        } else {
            // generate caravan and donkeys
            if (!city_data.trade.land_trade_problem_duration) {
                // caravan head
                struct figure_t *caravan = figure_create(FIGURE_TRADE_CARAVAN, scenario.entry_point.x, scenario.entry_point.y, DIR_0_TOP);
                caravan->is_targetable = 1;
                caravan->empire_city_id = city->id;
                caravan->action_state = FIGURE_ACTION_TRADE_CARAVAN_CREATED;
                caravan->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
                caravan->wait_ticks = 10;
                // donkey 1
                struct figure_t *donkey1 = figure_create(FIGURE_TRADE_CARAVAN_DONKEY, scenario.entry_point.x, scenario.entry_point.y, DIR_0_TOP);
                donkey1->is_targetable = 1;
                donkey1->action_state = FIGURE_ACTION_TRADE_CARAVAN_CREATED;
                donkey1->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
                donkey1->leading_figure_id = caravan->id;
                // donkey 2
                struct figure_t *donkey2 = figure_create(FIGURE_TRADE_CARAVAN_DONKEY, scenario.entry_point.x, scenario.entry_point.y, DIR_0_TOP);
                donkey2->is_targetable = 1;
                donkey2->action_state = FIGURE_ACTION_TRADE_CARAVAN_CREATED;
                donkey2->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
                donkey2->leading_figure_id = donkey1->id;
                city->trader_figure_ids[index] = caravan->id;
                break;
            }
        }
    }
}

int city_trade_next_caravan_import_resource(void)
{
    city_data.trade.caravan_import_resource++;
    if (city_data.trade.caravan_import_resource >= RESOURCE_TYPES_MAX) {
        city_data.trade.caravan_import_resource = RESOURCE_WHEAT;
    }
    return city_data.trade.caravan_import_resource;
}

int city_trade_next_caravan_backup_import_resource(void)
{
    city_data.trade.caravan_backup_import_resource++;
    if (city_data.trade.caravan_backup_import_resource >= RESOURCE_TYPES_MAX) {
        city_data.trade.caravan_backup_import_resource = RESOURCE_WHEAT;
    }
    return city_data.trade.caravan_backup_import_resource;
}

int city_trade_next_docker_import_resource(void)
{
    city_data.trade.docker_import_resource++;
    if (city_data.trade.docker_import_resource >= RESOURCE_TYPES_MAX) {
        city_data.trade.docker_import_resource = RESOURCE_WHEAT;
    }
    return city_data.trade.docker_import_resource;
}

int city_trade_next_docker_export_resource(void)
{
    city_data.trade.docker_export_resource++;
    if (city_data.trade.docker_export_resource >= RESOURCE_TYPES_MAX) {
        city_data.trade.docker_export_resource = RESOURCE_WHEAT;
    }
    return city_data.trade.docker_export_resource;
}

void city_victory_reset(void)
{
    victory_data.state = VICTORY_STATE_NONE;
    victory_data.force_win = 0;
}

void city_victory_force_win(void)
{
    victory_data.force_win = 1;
}

int city_victory_state(void)
{
    return victory_data.state;
}

void city_victory_check(void)
{
    victory_data.state = VICTORY_STATE_NONE;
    if ((scenario.time_limit_win_criteria.enabled && (game_time_year() >= scenario.start_year + scenario.time_limit_win_criteria.years))
    || (city_figures_total_invading_enemies() > 2 + city_data.figure.soldiers && city_data.population.population < city_data.population.highest_ever / 4)
    || (city_figures_total_invading_enemies() > 0 && !city_data.population.population)
    ) {
        victory_data.state = VICTORY_STATE_LOST;
    } else if (scenario.survival_time_win_criteria.enabled && (game_time_year() >= scenario.start_year + scenario.survival_time_win_criteria.years)) {
        victory_data.state = VICTORY_STATE_WON;
    } else {
        if (scenario.culture_win_criteria.enabled || scenario.prosperity_win_criteria.enabled
        || scenario.peace_win_criteria.enabled || scenario.favor_win_criteria.enabled || scenario.population_win_criteria.enabled) {
            int all_criteria_fulfilled = 1;
            if ((scenario.culture_win_criteria.enabled && city_data.ratings.culture < scenario.culture_win_criteria.goal)
            || (scenario.prosperity_win_criteria.enabled && city_data.ratings.prosperity < scenario.prosperity_win_criteria.goal)
            || (scenario.peace_win_criteria.enabled && city_data.ratings.peace < scenario.peace_win_criteria.goal)
            || (scenario.favor_win_criteria.enabled && city_data.ratings.favor < scenario.favor_win_criteria.goal)
            || (scenario.population_win_criteria.enabled && city_data.population.population < scenario.population_win_criteria.goal)
            ) {
                all_criteria_fulfilled = 0;
            }
            if (all_criteria_fulfilled) {
                victory_data.state = VICTORY_STATE_WON;
            }
        }
    }
    if (city_data.mission.has_won) {
        victory_data.state = city_data.mission.continue_months_left <= 0 ? VICTORY_STATE_WON : VICTORY_STATE_NONE;
    }
    if (victory_data.force_win) {
        victory_data.state = VICTORY_STATE_WON;
    }
    if (victory_data.state != VICTORY_STATE_NONE) {
        building_construction_clear_type();
        if (victory_data.state == VICTORY_STATE_LOST) {
            if (city_data.mission.fired_message_shown) {
                window_mission_end_show_fired();
            } else {
                city_data.mission.fired_message_shown = 1;
                city_message_post(1, MESSAGE_FIRED, 0, 0);
            }
            victory_data.force_win = 0;
        } else if (victory_data.state == VICTORY_STATE_WON) {
            stop_music();
            if (city_data.mission.victory_message_shown) {
                window_mission_end_show_won();
                victory_data.force_win = 0;
            } else {
                city_data.mission.victory_message_shown = 1;
                window_victory_dialog_show();
            }
        }
    }
}

void city_victory_update_months_to_govern(void)
{
    if (city_data.mission.has_won) {
        city_data.mission.continue_months_left--;
    }
}

void city_victory_continue_governing(int months)
{
    city_data.mission.has_won = 1;
    city_data.mission.continue_months_left += months;
    city_data.mission.continue_months_chosen = months;
    city_data.emperor.salary_rank = 0;
    city_data.emperor.salary_amount = 0;
    city_data.finance.this_year.expenses.salary = city_data.finance.salary_so_far;
}

void city_victory_stop_governing(void)
{
    city_data.mission.has_won = 0;
    city_data.mission.continue_months_left = 0;
    city_data.mission.continue_months_chosen = 0;
}

static void check_camera_boundaries(void)
{
    int x_min = (VIEW_X_MAX - map_data.width) / 2;
    int y_min = (VIEW_Y_MAX - 2 * map_data.height) / 2;
    if (view_data.camera.tile.x < x_min - 1) {
        view_data.camera.tile.x = x_min - 1;
        view_data.camera.pixel.x = 0;
    }
    if (view_data.camera.tile.x >= VIEW_X_MAX - x_min - view_data.viewport.width_tiles) {
        view_data.camera.tile.x = VIEW_X_MAX - x_min - view_data.viewport.width_tiles;
        view_data.camera.pixel.x = 0;
    }
    if (view_data.camera.tile.y < y_min - 2) {
        view_data.camera.tile.y = y_min - 1;
        view_data.camera.pixel.y = 0;
    }
    if (view_data.camera.tile.y >= ((VIEW_Y_MAX - y_min - view_data.viewport.height_tiles) & ~1)) {
        view_data.camera.tile.y = VIEW_Y_MAX - y_min - view_data.viewport.height_tiles;
        view_data.camera.pixel.y = 0;
    }
    view_data.camera.tile.y &= ~1;
}

static void calculate_lookup(void)
{
    for (int y = 0; y < VIEW_Y_MAX; y++) {
        for (int x = 0; x < VIEW_X_MAX; x++) {
            view_to_grid_offset_lookup[x][y] = -1;
        }
    }
    int y_view_start;
    int y_view_skip;
    int y_view_step;
    int x_view_start;
    int x_view_skip;
    int x_view_step;
    switch (view_data.orientation) {
        default:
        case DIR_0_TOP:
            x_view_start = VIEW_X_MAX - 1;
            x_view_skip = -1;
            x_view_step = 1;
            y_view_start = 1;
            y_view_skip = 1;
            y_view_step = 1;
            break;
        case DIR_2_RIGHT:
            x_view_start = 3;
            x_view_skip = 1;
            x_view_step = 1;
            y_view_start = VIEW_X_MAX - 3;
            y_view_skip = 1;
            y_view_step = -1;
            break;
        case DIR_4_BOTTOM:
            x_view_start = VIEW_X_MAX - 1;
            x_view_skip = 1;
            x_view_step = -1;
            y_view_start = VIEW_Y_MAX - 2;
            y_view_skip = -1;
            y_view_step = -1;
            break;
        case DIR_6_LEFT:
            x_view_start = VIEW_Y_MAX;
            x_view_skip = -1;
            x_view_step = -1;
            y_view_start = VIEW_X_MAX - 3;
            y_view_skip = -1;
            y_view_step = 1;
            break;
    }

    for (int y = 0; y < GRID_SIZE; y++) {
        int x_view = x_view_start;
        int y_view = y_view_start;
        for (int x = 0; x < GRID_SIZE; x++) {
            int grid_offset = x + GRID_SIZE * y;
            if (map_image_at(grid_offset) < 6) {
                view_to_grid_offset_lookup[x_view / 2][y_view] = -1;
            } else {
                view_to_grid_offset_lookup[x_view / 2][y_view] = grid_offset;
            }
            x_view += x_view_step;
            y_view += y_view_step;
        }
        x_view_start += x_view_skip;
        y_view_start += y_view_skip;
    }
}

void city_view_init(void)
{
    calculate_lookup();
    check_camera_boundaries();
    widget_minimap_invalidate();
}

int city_view_orientation(void)
{
    return view_data.orientation;
}

void city_view_reset_orientation(void)
{
    view_data.orientation = 0;
    calculate_lookup();
}

void city_view_get_camera(int *x, int *y)
{
    *x = view_data.camera.tile.x;
    *y = view_data.camera.tile.y;
}

void city_view_get_pixel_offset(int *x, int *y)
{
    *x = view_data.camera.pixel.x;
    *y = view_data.camera.pixel.y;
}

void city_view_get_camera_in_pixels(int *x, int *y)
{
    *x = view_data.camera.tile.x * TILE_WIDTH_PIXELS + view_data.camera.pixel.x;
    *y = view_data.camera.tile.y * HALF_TILE_HEIGHT_PIXELS + view_data.camera.pixel.y;
}

void city_view_set_camera(int x, int y)
{
    view_data.camera.tile.x = x;
    view_data.camera.tile.y = y;
    check_camera_boundaries();
}

void city_view_set_camera_from_pixel_position(int x, int y)
{
    x = x < 0 ? 0 : x;
    y = y < 0 ? 0 : y;

    view_data.camera.tile.x = x / TILE_WIDTH_PIXELS;
    view_data.camera.tile.y = y / HALF_TILE_HEIGHT_PIXELS;
    view_data.camera.pixel.x = x % TILE_WIDTH_PIXELS;
    view_data.camera.pixel.y = y % TILE_HEIGHT_PIXELS;
    check_camera_boundaries();
}

void city_view_scroll(int x, int y)
{
    view_data.camera.pixel.x += x;
    view_data.camera.pixel.y += y;
    while (view_data.camera.pixel.x < 0) {
        view_data.camera.tile.x--;
        view_data.camera.pixel.x += TILE_WIDTH_PIXELS;
    }
    while (view_data.camera.pixel.y < 0) {
        view_data.camera.tile.y -= 2;
        view_data.camera.pixel.y += TILE_HEIGHT_PIXELS;
    }
    while (view_data.camera.pixel.x >= TILE_WIDTH_PIXELS) {
        view_data.camera.tile.x++;
        view_data.camera.pixel.x -= TILE_WIDTH_PIXELS;
    }
    while (view_data.camera.pixel.y >= TILE_HEIGHT_PIXELS) {
        view_data.camera.tile.y += 2;
        view_data.camera.pixel.y -= TILE_HEIGHT_PIXELS;
    }
    check_camera_boundaries();
}

void city_view_grid_offset_to_xy_view(int grid_offset, int *x_view, int *y_view)
{
    *x_view = *y_view = 0;
    for (int y = 0; y < VIEW_Y_MAX; y++) {
        for (int x = 0; x < VIEW_X_MAX; x++) {
            if (view_to_grid_offset_lookup[x][y] == grid_offset) {
                *x_view = x;
                *y_view = y;
                return;
            }
        }
    }
}

void city_view_get_selected_tile_pixels(int *x_pixels, int *y_pixels)
{
    *x_pixels = view_data.selected_tile.x_pixels;
    *y_pixels = view_data.selected_tile.y_pixels;
}

int city_view_pixels_to_view_tile(int x_pixels, int y_pixels, struct pixel_view_coordinates_t *tile)
{
    if (x_pixels < view_data.viewport.x ||
            x_pixels >= view_data.viewport.x + view_data.viewport.width_pixels ||
            y_pixels < view_data.viewport.y ||
            y_pixels >= view_data.viewport.y + view_data.viewport.height_pixels) {
        return 0;
    }

    x_pixels += view_data.camera.pixel.x;
    y_pixels += view_data.camera.pixel.y;
    int odd = ((x_pixels - view_data.viewport.x) / HALF_TILE_WIDTH_PIXELS +
        (y_pixels - view_data.viewport.y) / HALF_TILE_HEIGHT_PIXELS) & 1;
    int x_is_odd = ((x_pixels - view_data.viewport.x) / HALF_TILE_WIDTH_PIXELS) & 1;
    int y_is_odd = ((y_pixels - view_data.viewport.y) / HALF_TILE_HEIGHT_PIXELS) & 1;
    int x_mod = ((x_pixels - view_data.viewport.x) % HALF_TILE_WIDTH_PIXELS) / 2;
    int y_mod = (y_pixels - view_data.viewport.y) % HALF_TILE_HEIGHT_PIXELS;
    int x_view_offset = (x_pixels - view_data.viewport.x) / TILE_WIDTH_PIXELS;
    int y_view_offset = (y_pixels - view_data.viewport.y) / HALF_TILE_HEIGHT_PIXELS;
    if (odd) {
        if (x_mod + y_mod >= HALF_TILE_HEIGHT_PIXELS - 1) {
            y_view_offset++;
            if (x_is_odd && !y_is_odd) {
                x_view_offset++;
            }
        }
    } else {
        if (y_mod > x_mod) {
            y_view_offset++;
        } else if (x_is_odd && y_is_odd) {
            x_view_offset++;
        }
    }
    tile->x = view_data.camera.tile.x + x_view_offset;
    tile->y = view_data.camera.tile.y + y_view_offset;
    return 1;
}

void city_view_set_selected_view_tile(const struct pixel_view_coordinates_t *tile)
{
    int x_view_offset = tile->x - view_data.camera.tile.x;
    int y_view_offset = tile->y - view_data.camera.tile.y;
    view_data.selected_tile.x_pixels = view_data.viewport.x + TILE_WIDTH_PIXELS * x_view_offset - view_data.camera.pixel.x;
    if (y_view_offset & 1) {
        view_data.selected_tile.x_pixels -= HALF_TILE_WIDTH_PIXELS;
    }
    view_data.selected_tile.y_pixels = view_data.viewport.y + HALF_TILE_HEIGHT_PIXELS * y_view_offset
        - HALF_TILE_HEIGHT_PIXELS - view_data.camera.pixel.y;
}

int city_view_tile_to_grid_offset(const struct pixel_view_coordinates_t *tile)
{
    int grid_offset = view_to_grid_offset_lookup[tile->x][tile->y];
    return grid_offset < 0 ? 0 : grid_offset;
}

void city_view_go_to_grid_offset(int grid_offset)
{
    int x, y;
    city_view_grid_offset_to_xy_view(grid_offset, &x, &y);
    view_data.camera.tile.x = x - view_data.viewport.width_tiles / 2;
    view_data.camera.tile.y = y - view_data.viewport.height_tiles / 2;
    view_data.camera.tile.y &= ~1;
    check_camera_boundaries();
}

static int get_center_grid_offset(void)
{
    int x_center = view_data.camera.tile.x + view_data.viewport.width_tiles / 2;
    int y_center = view_data.camera.tile.y + view_data.viewport.height_tiles / 2;
    return view_to_grid_offset_lookup[x_center][y_center];
}

void city_view_rotate_left(void)
{
    int center_grid_offset = get_center_grid_offset();

    view_data.orientation += 2;
    if (view_data.orientation > 6) {
        view_data.orientation = DIR_0_TOP;
    }
    calculate_lookup();
    if (center_grid_offset >= 0) {
        int x, y;
        city_view_grid_offset_to_xy_view(center_grid_offset, &x, &y);
        view_data.camera.tile.x = x - view_data.viewport.width_tiles / 2;
        view_data.camera.tile.y = y - view_data.viewport.height_tiles / 2;
    }
    check_camera_boundaries();
}

void city_view_rotate_right(void)
{
    int center_grid_offset = get_center_grid_offset();

    view_data.orientation -= 2;
    if (view_data.orientation < 0) {
        view_data.orientation = DIR_6_LEFT;
    }
    calculate_lookup();
    if (center_grid_offset >= 0) {
        int x, y;
        city_view_grid_offset_to_xy_view(center_grid_offset, &x, &y);
        view_data.camera.tile.x = x - view_data.viewport.width_tiles / 2;
        view_data.camera.tile.y = y - view_data.viewport.height_tiles / 2;
    }
    check_camera_boundaries();
}

static void set_viewport(int x_offset, int y_offset, int width, int height)
{
    view_data.viewport.x = x_offset;
    view_data.viewport.y = y_offset;
    view_data.viewport.width_pixels = width - 2;
    view_data.viewport.height_pixels = height;
    view_data.viewport.width_tiles = width / TILE_WIDTH_PIXELS;
    view_data.viewport.height_tiles = height / HALF_TILE_HEIGHT_PIXELS;
}

void city_view_set_viewport(int screen_width, int screen_height)
{
    view_data.screen_width = screen_width;
    view_data.screen_height = screen_height;
    if (view_data.sidebar_collapsed) {
        set_viewport(0, TOP_MENU_HEIGHT, view_data.screen_width - 40, view_data.screen_height - TOP_MENU_HEIGHT);
    } else {
        set_viewport(0, TOP_MENU_HEIGHT, view_data.screen_width - 160, view_data.screen_height - TOP_MENU_HEIGHT);
    }
    check_camera_boundaries();
}

void city_view_get_viewport(int *x, int *y, int *width, int *height)
{
    *x = view_data.viewport.x;
    *y = view_data.viewport.y;
    *width = view_data.viewport.width_pixels;
    *height = view_data.viewport.height_pixels;
}

void city_view_get_viewport_size_tiles(int *width, int *height)
{
    *width = view_data.viewport.width_tiles;
    *height = view_data.viewport.height_tiles;
}

int city_view_is_sidebar_collapsed(void)
{
    return view_data.sidebar_collapsed;
}

void city_view_start_sidebar_toggle(void)
{
    set_viewport(0, TOP_MENU_HEIGHT, view_data.screen_width - 40, view_data.screen_height - TOP_MENU_HEIGHT);
    check_camera_boundaries();
}

void city_view_toggle_sidebar(void)
{
    if (view_data.sidebar_collapsed) {
        view_data.sidebar_collapsed = 0;
        set_viewport(0, TOP_MENU_HEIGHT, view_data.screen_width - 160, view_data.screen_height - TOP_MENU_HEIGHT);
    } else {
        view_data.sidebar_collapsed = 1;
        set_viewport(0, TOP_MENU_HEIGHT, view_data.screen_width - 40, view_data.screen_height - TOP_MENU_HEIGHT);
    }
    check_camera_boundaries();
}

void city_view_save_state(struct buffer_t *orientation, struct buffer_t *camera)
{
    buffer_write_i32(orientation, view_data.orientation);

    buffer_write_i32(camera, view_data.camera.tile.x);
    buffer_write_i32(camera, view_data.camera.tile.y);
}

void city_view_load_state(struct buffer_t *orientation, struct buffer_t *camera)
{
    view_data.orientation = buffer_read_i32(orientation);
    city_view_load_scenario_state(camera);

    if (view_data.orientation >= 0 && view_data.orientation <= 6) {
        // ensure even number
        view_data.orientation = 2 * (view_data.orientation / 2);
    } else {
        view_data.orientation = 0;
    }
}

void city_view_save_scenario_state(struct buffer_t *camera)
{
    buffer_write_i32(camera, view_data.camera.tile.x);
    buffer_write_i32(camera, view_data.camera.tile.y);
}

void city_view_load_scenario_state(struct buffer_t *camera)
{
    view_data.camera.tile.x = buffer_read_i32(camera);
    view_data.camera.tile.y = buffer_read_i32(camera);
}

void city_view_foreach_map_tile(map_callback *callback)
{
    int odd = 0;
    int y_view = view_data.camera.tile.y - 8;
    int y_graphic = view_data.viewport.y - 9 * HALF_TILE_HEIGHT_PIXELS - view_data.camera.pixel.y;
    for (int y = 0; y < view_data.viewport.height_tiles + 21; y++) {
        if (y_view >= 0 && y_view < VIEW_Y_MAX) {
            int x_graphic = -(4 * TILE_WIDTH_PIXELS) - view_data.camera.pixel.x;
            if (odd) {
                x_graphic += view_data.viewport.x - HALF_TILE_WIDTH_PIXELS;
            } else {
                x_graphic += view_data.viewport.x;
            }
            int x_view = view_data.camera.tile.x - 4;
            for (int x = 0; x < view_data.viewport.width_tiles + 7; x++) {
                if (x_view >= 0 && x_view < VIEW_X_MAX) {
                    int grid_offset = view_to_grid_offset_lookup[x_view][y_view];
                    callback(x_graphic, y_graphic, grid_offset);
                }
                x_graphic += TILE_WIDTH_PIXELS;
                x_view++;
            }
        }
        odd = 1 - odd;
        y_graphic += HALF_TILE_HEIGHT_PIXELS;
        y_view++;
    }
}

void city_view_foreach_valid_map_tile(map_callback *callback1, map_callback *callback2, map_callback *callback3)
{
    int odd = 0;
    int y_view = view_data.camera.tile.y - 8;
    int y_graphic = view_data.viewport.y - 9 * HALF_TILE_HEIGHT_PIXELS - view_data.camera.pixel.y;
    int x_graphic, x_view;
    for (int y = 0; y < view_data.viewport.height_tiles + 21; y++) {
        if (y_view >= 0 && y_view < VIEW_Y_MAX) {
            if (callback1) {
                x_graphic = -(4 * TILE_WIDTH_PIXELS) - view_data.camera.pixel.x;
                if (odd) {
                    x_graphic += view_data.viewport.x - HALF_TILE_WIDTH_PIXELS;
                } else {
                    x_graphic += view_data.viewport.x;
                }
                x_view = view_data.camera.tile.x - 4;
                for (int x = 0; x < view_data.viewport.width_tiles + 7; x++) {
                    if (x_view >= 0 && x_view < VIEW_X_MAX) {
                        int grid_offset = view_to_grid_offset_lookup[x_view][y_view];
                        if (grid_offset >= 0) {
                            callback1(x_graphic, y_graphic, grid_offset);
                        }
                    }
                    x_graphic += TILE_WIDTH_PIXELS;
                    x_view++;
                }
            }
            if (callback2) {
                x_graphic = -(4 * TILE_WIDTH_PIXELS) - view_data.camera.pixel.x;
                if (odd) {
                    x_graphic += view_data.viewport.x - HALF_TILE_WIDTH_PIXELS;
                } else {
                    x_graphic += view_data.viewport.x;
                }
                x_view = view_data.camera.tile.x - 4;
                for (int x = 0; x < view_data.viewport.width_tiles + 7; x++) {
                    if (x_view >= 0 && x_view < VIEW_X_MAX) {
                        int grid_offset = view_to_grid_offset_lookup[x_view][y_view];
                        if (grid_offset >= 0) {
                            callback2(x_graphic, y_graphic, grid_offset);
                        }
                    }
                    x_graphic += TILE_WIDTH_PIXELS;
                    x_view++;
                }
            }
            if (callback3) {
                x_graphic = -(4 * TILE_WIDTH_PIXELS) - view_data.camera.pixel.x;
                if (odd) {
                    x_graphic += view_data.viewport.x - HALF_TILE_WIDTH_PIXELS;
                } else {
                    x_graphic += view_data.viewport.x;
                }
                x_view = view_data.camera.tile.x - 4;
                for (int x = 0; x < view_data.viewport.width_tiles + 7; x++) {
                    if (x_view >= 0 && x_view < VIEW_X_MAX) {
                        int grid_offset = view_to_grid_offset_lookup[x_view][y_view];
                        if (grid_offset >= 0) {
                            callback3(x_graphic, y_graphic, grid_offset);
                        }
                    }
                    x_graphic += TILE_WIDTH_PIXELS;
                    x_view++;
                }
            }
        }
        odd = 1 - odd;
        y_graphic += HALF_TILE_HEIGHT_PIXELS;
        y_view++;
    }
}

void city_view_foreach_minimap_tile(int x_offset, int y_offset, int absolute_x, int absolute_y, int width_tiles, int height_tiles, map_callback *callback)
{
    int odd = 0;
    int y_abs = absolute_y - 4;
    int y_view = y_offset - 4;
    for (int y_rel = -4; y_rel < height_tiles + 4; y_rel++, y_abs++, y_view++) {
        int x_view;
        if (odd) {
            x_view = x_offset - 9;
            odd = 0;
        } else {
            x_view = x_offset - 8;
            odd = 1;
        }
        int x_abs = absolute_x - 4;
        for (int x_rel = -4; x_rel < width_tiles; x_rel++, x_abs++, x_view += 2) {
            if (x_abs >= 0 && x_abs < VIEW_X_MAX && y_abs >= 0 && y_abs < VIEW_Y_MAX) {
                callback(x_view, y_view, view_to_grid_offset_lookup[x_abs][y_abs]);
            }
        }
    }
}

void city_warning_show(int type)
{
    const char *text;
    if (type == WARNING_ORIENTATION) {
        text = lang_get_string(17, city_view_orientation());
    } else {
        text = lang_get_string(19, type - 2);
    }
    city_warning_show_custom(text);
}

void city_warning_show_custom(const char *text)
{
    if (!setting_warnings()) {
        return;
    }
    struct warning *w = 0;
    for (int i = 0; i < MAX_WARNINGS; i++) {
        if (!warnings[i].in_use) {
            w = &warnings[i];
            break;
        }
    }
    if (w) {
        w->in_use = 1;
        w->time = time_get_millis();
        string_copy(text, w->text, MAX_TEXT);
    }
}

int city_has_warnings(void)
{
    for (int i = 0; i < MAX_WARNINGS; i++) {
        if (warnings[i].in_use) {
            return 1;
        }
    }
    return 0;
}

const char *city_warning_get(int id)
{
    if (warnings[id].in_use) {
        return warnings[id].text;
    }
    return 0;
}

void city_warning_clear_all(void)
{
    for (int i = 0; i < MAX_WARNINGS; i++) {
        warnings[i].in_use = 0;
    }
}

void city_warning_clear_outdated(void)
{
    for (int i = 0; i < MAX_WARNINGS; i++) {
        if (warnings[i].in_use && time_get_millis() - warnings[i].time > TIMEOUT_MS) {
            warnings[i].in_use = 0;
        }
    }
}
