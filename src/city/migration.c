#include "migration.h"

#include "building/building.h"
#include "city/data.h"
#include "city/message.h"
#include "city/population.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/figure.h"
#include "map/building_tiles.h"
#include "map/image.h"
#include "map/terrain.h"
#include "scenario/scenario.h"

static void update_status(void)
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

    int population_cap = 200000;
    if (city_data.population.population >= population_cap) {
        city_data.migration.percentage = 0;
        return;
    }
    // war scares immigrants away
    if (city_figures_total_invading_enemies() > 3 &&
        city_data.migration.percentage > 0) {
        city_data.migration.percentage = 0;
        return;
    }
    if (city_data.migration.percentage > 0) {
        // immigration
        if (city_data.migration.emigration_duration) {
            city_data.migration.emigration_duration--;
        } else {
            city_data.migration.immigration_amount_per_batch =
                calc_adjust_with_percentage(12, city_data.migration.percentage);
            city_data.migration.immigration_duration = 2;
        }
    } else if (city_data.migration.percentage < 0) {
        // emigration
        if (city_data.migration.immigration_duration) {
            city_data.migration.immigration_duration--;
        } else if (city_data.population.population > 100) {
            city_data.migration.emigration_amount_per_batch =
                calc_adjust_with_percentage(12, -city_data.migration.percentage);
            city_data.migration.emigration_duration = 2;
        }
    }
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
        if (b->distance_from_entry > 0 && b->house_population_room >= 8 && !b->immigrant_figure_id) {
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
        if (b->distance_from_entry > 0 && b->house_population_room > 0 && !b->immigrant_figure_id) {
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
    b->distance_from_entry = 0;
    map_building_tiles_add(b->id, b->x, b->y, 1, image_id, TERRAIN_BUILDING);
}

static void figure_create_emigrant(struct building_t *house, int num_people)
{
    city_population_remove(num_people);
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

static void create_migrants(void)
{
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

void city_migration_update(void)
{
    update_status();
    create_migrants();
}

void city_migration_determine_no_immigration_cause(void)
{
    switch (city_data.sentiment.low_mood_cause) {
        case LOW_MOOD_CAUSE_NO_FOOD:
            city_data.migration.no_immigration_cause = 2;
            break;
        case LOW_MOOD_CAUSE_NO_JOBS:
            city_data.migration.no_immigration_cause = 1;
            break;
        case LOW_MOOD_CAUSE_HIGH_TAXES:
            city_data.migration.no_immigration_cause = 3;
            break;
        case LOW_MOOD_CAUSE_LOW_WAGES:
            city_data.migration.no_immigration_cause = 0;
            break;
        case LOW_MOOD_CAUSE_MANY_TENTS:
            city_data.migration.no_immigration_cause = 4;
            break;
        default:
            city_data.migration.no_immigration_cause = 5;
            break;
    }
}

int city_migration_no_immigration_cause(void)
{
    return city_data.migration.no_immigration_cause;
}

int city_migration_no_room_for_immigrants(void)
{
    return city_data.migration.refused_immigrants_today || city_data.population.room_in_houses <= 0;
}

int city_migration_percentage(void)
{
    return city_data.migration.percentage;
}

int city_migration_newcomers(void)
{
    return city_data.migration.newcomers;
}

void city_migration_reset_newcomers(void)
{
    city_data.migration.newcomers = 0;
}
