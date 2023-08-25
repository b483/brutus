#include "gods.h"

#include "city/culture.h"
#include "city/data.h"
#include "city/health.h"
#include "city/message.h"
#include "city/sentiment.h"
#include "city/trade.h"
#include "core/calc.h"
#include "core/random.h"
#include "figure/formation_legion.h"
#include "figure/route.h"
#include "figuretype/water.h"
#include "game/game.h"
#include "map/map.h"
#include "scenario/scenario.h"
#include "sound/sound.h"

#define TIE 10
#define CURSE_LOADS 16


static void building_bless_farms(void)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->output_resource_id && building_is_farm(b->type)) {
            b->data.industry.progress = MAX_PROGRESS_RAW;
            b->data.industry.curse_days_left = 0;
            b->data.industry.blessing_days_left = 16;
            update_farm_image(b);
        }
    }
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

static void perform_blessing(int god)
{
    switch (god) {
        case GOD_CERES:
            city_message_post(1, MESSAGE_BLESSING_FROM_CERES, 0, 0);
            building_bless_farms();
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
        city_message_disable_sound_for_next_message();
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

static void perform_small_curse(int god)
{
    switch (god) {
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
            cause_invasion_mars(16);
            break;
        case GOD_VENUS:
            city_message_post(1, MESSAGE_VENUS_IS_UPSET, 0, 0);
            city_sentiment_set_max_happiness(50);
            city_sentiment_change_happiness(-5);
            city_data.health.value = calc_bound(city_data.health.value - 10, 0, 100);
            city_sentiment_update();
            break;
    }
}

static int perform_large_curse(int god)
{
    switch (god) {
        case GOD_CERES:
            city_message_post(1, MESSAGE_WRATH_OF_CERES, 0, 0);
            building_curse_farms(1);
            break;
        case GOD_NEPTUNE:
            if (city_data.trade.num_sea_routes <= 0) {
                city_message_post(1, MESSAGE_WRATH_OF_NEPTUNE_NO_SEA_TRADE, 0, 0);
                return 0;
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
            cause_invasion_mars(32);
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
    return 1;
}

static void update_god_moods(void)
{
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
            perform_blessing(god_id);
        } else if (god->wrath_bolts >= 20 && !god->small_curse_done && god->months_since_festival > 3) {
            god->small_curse_done = 1;
            god->wrath_bolts = 0;
            god->happiness += 12;
            perform_small_curse(god_id);
        } else if (god->wrath_bolts >= 50 && god->months_since_festival > 3) {
            god->wrath_bolts = 0;
            god->happiness += 30;
            if (!perform_large_curse(god_id)) {
                return;
            }
        }
    }

    int min_happiness = 100;
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

    // BUG poor Venus never gets points here!
    if (max_god < 4) {
        if (city_data.religion.gods[max_god].target_happiness >= 50) {
            city_data.religion.gods[max_god].target_happiness = 100;
        } else {
            city_data.religion.gods[max_god].target_happiness += 50;
        }
    }
    if (min_god < 4) {
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
        update_god_moods();
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

int city_god_neptune_create_shipwreck_flotsam(void)
{
    if (city_data.religion.neptune_sank_ships) {
        city_data.religion.neptune_sank_ships = 0;
        return 1;
    } else {
        return 0;
    }
}
