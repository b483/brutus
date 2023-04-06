#include "gods.h"

#include "building/count.h"
#include "building/granary.h"
#include "building/industry.h"
#include "city/culture.h"
#include "city/data_private.h"
#include "city/health.h"
#include "city/message.h"
#include "city/sentiment.h"
#include "city/trade.h"
#include "core/calc.h"
#include "core/random.h"
#include "figure/formation_legion.h"
#include "figure/route.h"
#include "figuretype/water.h"
#include "game/settings.h"
#include "game/time.h"
#include "scenario/data.h"
#include "scenario/editor_events.h"

#define TIE 10

static void perform_blessing(god_type god)
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
            building_granary_bless();
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
    int grid_offset = start_invasion(ENEMY_TYPE_BARBARIAN, ENEMY_TYPE_BARBARIAN, enemy_amount, MAX_INVASION_POINTS, FORMATION_ATTACK_FOOD_CHAIN, 23);
    if (grid_offset > 0) {
        city_message_post(1, MESSAGE_LOCAL_UPRISING_MARS, 0, grid_offset);
    }
}

static void perform_small_curse(god_type god)
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
            city_health_change(-10);
            city_sentiment_update();
            break;
    }
}

static int perform_large_curse(god_type god)
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
            struct formation_t *best_legion = 0;
            int best_legion_weight = 0;
            for (int i = 1; i <= MAX_FORMATIONS; i++) {
                if (formations[i].in_use && formations[i].is_legion) {
                    int weight = formations[i].num_figures;
                    if (formations[i].figure_type == FIGURE_FORT_LEGIONARY) {
                        weight *= 2;
                    }
                    if (weight > best_legion_weight) {
                        best_legion_weight = weight;
                        best_legion = &formations[i];
                    }
                }
            }
            if (best_legion) {
                for (int i = 0; i < best_legion->max_figures; i++) {
                    if (best_legion->figures[i]) {
                        struct figure_t *f = &figures[best_legion->figures[i]];
                        map_point nearest_barracks_road_tile = { 0 };
                        set_destination__closest_building_of_type(best_legion->building_id, BUILDING_BARRACKS, &nearest_barracks_road_tile);
                        figure_route_remove(f);
                        if (nearest_barracks_road_tile.x) {
                            f->destination_x = nearest_barracks_road_tile.x;
                            f->destination_y = nearest_barracks_road_tile.y;
                        } else {
                            f->destination_x = city_data.map.exit_point.x;
                            f->destination_y = city_data.map.exit_point.y;
                        }
                        f->action_state = FIGURE_ACTION_SOLDIER_RETURNING_TO_BARRACKS;
                    }
                }
                best_legion->cursed_by_mars = 96;
                formation_calculate_figures();
                city_message_post(1, MESSAGE_WRATH_OF_MARS, 0, 0);
            } else {
                city_message_post(1, MESSAGE_WRATH_OF_MARS_NO_MILITARY, 0, 0);
            }
            cause_invasion_mars(32);
            break;
        case GOD_VENUS:
            city_message_post(1, MESSAGE_WRATH_OF_VENUS, 0, 0);
            city_sentiment_set_max_happiness(40);
            city_sentiment_change_happiness(-10);
            if (city_data.health.value >= 80) {
                city_health_change(-50);
            } else if (city_data.health.value >= 60) {
                city_health_change(-40);
            } else {
                city_health_change(-20);
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
        god_status *god = &city_data.religion.gods[i];
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
        god_status *god = &city_data.religion.gods[god_id];
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
        god_status *god = &city_data.religion.gods[god_id];
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
