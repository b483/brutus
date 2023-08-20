#include "culture.h"

#include "building/building.h"
#include "city/constants.h"
#include "city/data.h"
#include "city/entertainment.h"
#include "city/message.h"
#include "city/population.h"
#include "city/sentiment.h"
#include "core/calc.h"

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
} coverage;

int city_culture_coverage_theater(void)
{
    return coverage.theater;
}

int city_culture_coverage_amphitheater(void)
{
    return coverage.amphitheater;
}

int city_culture_coverage_colosseum(void)
{
    return coverage.colosseum;
}

int city_culture_coverage_hippodrome(void)
{
    return coverage.hippodrome;
}

int city_culture_coverage_average_entertainment(void)
{
    return (coverage.hippodrome + coverage.colosseum + coverage.amphitheater + coverage.theater) / 4;
}

int city_culture_coverage_religion(int god)
{
    return coverage.religion[god];
}

int city_culture_coverage_school(void)
{
    return coverage.school;
}

int city_culture_coverage_library(void)
{
    return coverage.library;
}

int city_culture_coverage_academy(void)
{
    return coverage.academy;
}

int city_culture_coverage_hospital(void)
{
    return coverage.hospital;
}

static int top(int input)
{
    return input > 100 ? 100 : input;
}

void city_culture_update_coverage(void)
{
    int population = city_data.population.population;

    // entertainment
    coverage.theater = top(calc_percentage(500 * building_count_active(BUILDING_THEATER), population));
    coverage.amphitheater = top(calc_percentage(800 * building_count_active(BUILDING_AMPHITHEATER), population));
    coverage.colosseum = top(calc_percentage(1500 * building_count_active(BUILDING_COLOSSEUM), population));
    if (building_count_active(BUILDING_HIPPODROME) <= 0) {
        coverage.hippodrome = 0;
    } else {
        coverage.hippodrome = 100;
    }

    // religion
    int oracles = building_count_total(BUILDING_ORACLE);
    coverage.religion[GOD_CERES] = top(calc_percentage(
        500 * oracles +
        750 * building_count_active(BUILDING_SMALL_TEMPLE_CERES) +
        1500 * building_count_active(BUILDING_LARGE_TEMPLE_CERES),
        population));
    coverage.religion[GOD_NEPTUNE] = top(calc_percentage(
        500 * oracles +
        750 * building_count_active(BUILDING_SMALL_TEMPLE_NEPTUNE) +
        1500 * building_count_active(BUILDING_LARGE_TEMPLE_NEPTUNE),
        population));
    coverage.religion[GOD_MERCURY] = top(calc_percentage(
        500 * oracles +
        750 * building_count_active(BUILDING_SMALL_TEMPLE_MERCURY) +
        1500 * building_count_active(BUILDING_LARGE_TEMPLE_MERCURY),
        population));
    coverage.religion[GOD_MARS] = top(calc_percentage(
        500 * oracles +
        750 * building_count_active(BUILDING_SMALL_TEMPLE_MARS) +
        1500 * building_count_active(BUILDING_LARGE_TEMPLE_MARS),
        population));
    coverage.religion[GOD_VENUS] = top(calc_percentage(
        500 * oracles +
        750 * building_count_active(BUILDING_SMALL_TEMPLE_VENUS) +
        1500 * building_count_active(BUILDING_LARGE_TEMPLE_VENUS),
        population));
    coverage.oracle = top(calc_percentage(500 * oracles, population));

    city_data.culture.religion_coverage =
        coverage.religion[GOD_CERES] +
        coverage.religion[GOD_NEPTUNE] +
        coverage.religion[GOD_MERCURY] +
        coverage.religion[GOD_MARS] +
        coverage.religion[GOD_VENUS];
    city_data.culture.religion_coverage /= 5;

    // education
    city_population_calculate_educational_age();

    coverage.school = top(calc_percentage(
        75 * building_count_active(BUILDING_SCHOOL), city_data.population.school_age));
    coverage.library = top(calc_percentage(
        800 * building_count_active(BUILDING_LIBRARY), population));
    coverage.academy = top(calc_percentage(
        100 * building_count_active(BUILDING_ACADEMY), city_data.population.academy_age));

    // health
    coverage.hospital = top(calc_percentage(
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

    city_entertainment_calculate_shows();
}

static void throw_party(void)
{
    if (city_data.festival.first_festival_effect_months <= 0) {
        city_data.festival.first_festival_effect_months = 12;
        switch (city_data.festival.size) {
            case FESTIVAL_SMALL: city_sentiment_change_happiness(7); break;
            case FESTIVAL_LARGE: city_sentiment_change_happiness(9); break;
            case FESTIVAL_GRAND: city_sentiment_change_happiness(12); break;
        }
    } else if (city_data.festival.second_festival_effect_months <= 0) {
        city_data.festival.second_festival_effect_months = 12;
        switch (city_data.festival.size) {
            case FESTIVAL_SMALL: city_sentiment_change_happiness(2); break;
            case FESTIVAL_LARGE: city_sentiment_change_happiness(3); break;
            case FESTIVAL_GRAND: city_sentiment_change_happiness(5); break;
        }
    }
    city_data.festival.months_since_festival = 1;
    city_data.religion.gods[city_data.festival.god].months_since_festival = 0;
    switch (city_data.festival.size) {
        case FESTIVAL_SMALL: city_message_post(1, MESSAGE_SMALL_FESTIVAL, 0, 0); break;
        case FESTIVAL_LARGE: city_message_post(1, MESSAGE_LARGE_FESTIVAL, 0, 0); break;
        case FESTIVAL_GRAND: city_message_post(1, MESSAGE_GRAND_FESTIVAL, 0, 0); break;
    }
    city_data.festival.size = FESTIVAL_NONE;
    city_data.festival.months_to_go = 0;
}

void city_festival_update(void)
{
    city_data.festival.months_since_festival++;
    if (city_data.festival.first_festival_effect_months) {
        --city_data.festival.first_festival_effect_months;
    }
    if (city_data.festival.second_festival_effect_months) {
        --city_data.festival.second_festival_effect_months;
    }
    if (city_data.festival.size) {
        city_data.festival.months_to_go--;
        if (city_data.festival.months_to_go <= 0) {
            throw_party();
        }
    }
}

void city_culture_save_state(struct buffer_t *buf)
{
    buffer_write_i32(buf, coverage.theater);
    buffer_write_i32(buf, coverage.amphitheater);
    buffer_write_i32(buf, coverage.colosseum);
    buffer_write_i32(buf, coverage.hippodrome);
    for (int i = GOD_CERES; i <= GOD_VENUS; i++) {
        buffer_write_i32(buf, coverage.religion[i]);
    }
    buffer_write_i32(buf, coverage.oracle);
    buffer_write_i32(buf, coverage.school);
    buffer_write_i32(buf, coverage.library);
    buffer_write_i32(buf, coverage.academy);
    buffer_write_i32(buf, coverage.hospital);
}

void city_culture_load_state(struct buffer_t *buf)
{
    coverage.theater = buffer_read_i32(buf);
    coverage.amphitheater = buffer_read_i32(buf);
    coverage.colosseum = buffer_read_i32(buf);
    coverage.hippodrome = buffer_read_i32(buf);
    for (int i = GOD_CERES; i <= GOD_VENUS; i++) {
        coverage.religion[i] = buffer_read_i32(buf);
    }
    coverage.oracle = buffer_read_i32(buf);
    coverage.school = buffer_read_i32(buf);
    coverage.library = buffer_read_i32(buf);
    coverage.academy = buffer_read_i32(buf);
    coverage.hospital = buffer_read_i32(buf);
}
