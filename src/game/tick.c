#include "tick.h"

#include "city/culture.h"
#include "city/data.h"
#include "city/emperor.h"
#include "city/finance.h"
#include "city/gods.h"
#include "city/health.h"
#include "city/labor.h"
#include "city/message.h"
#include "city/migration.h"
#include "city/population.h"
#include "city/ratings.h"
#include "city/resource.h"
#include "city/sentiment.h"
#include "city/trade.h"
#include "city/victory.h"
#include "city/view.h"
#include "city/warning.h"
#include "core/file.h"
#include "core/image.h"
#include "core/random.h"
#include "editor/editor.h"
#include "empire/object.h"
#include "figure/combat.h"
#include "figure/formation_legion.h"
#include "figuretype/animal.h"
#include "figuretype/cartpusher.h"
#include "figuretype/crime.h"
#include "figuretype/docker.h"
#include "figuretype/editor.h"
#include "figuretype/enemy.h"
#include "figuretype/entertainer.h"
#include "figuretype/maintenance.h"
#include "figuretype/market.h"
#include "figuretype/migrant.h"
#include "figuretype/missile.h"
#include "figuretype/service.h"
#include "figuretype/soldier.h"
#include "figuretype/trader.h"
#include "figuretype/wall.h"
#include "figuretype/water.h"
#include "game/file_io.h"
#include "game/settings.h"
#include "game/time.h"
#include "game/undo.h"
#include "map/building.h"
#include "map/building_tiles.h"
#include "map/desirability.h"
#include "map/grid.h"
#include "map/natives.h"
#include "map/random.h"
#include "map/road_access.h"
#include "map/road_network.h"
#include "map/routing.h"
#include "map/routing_terrain.h"
#include "map/terrain.h"
#include "map/tiles.h"
#include "map/water.h"
#include "map/water_supply.h"
#include "scenario/scenario.h"
#include "sound/sound.h"
#include "widget/minimap.h"

#define MAX_DIR 4

enum {
    EVOLVE = 1,
    NONE = 0,
    DEVOLVE = -1
};

static int fire_spread_direction = 0;

static const struct {
    int x;
    int y;
    int offset;
} EXPAND_DIRECTION_DELTA[MAX_DIR] = { {0, 0, 0}, {-1, -1, -GRID_SIZE - 1}, {-1, 0, -1}, {0, -1, -GRID_SIZE} };

static struct {
    int x;
    int y;
    int inventory[INVENTORY_MAX];
    int population;
} merge_data;

static const int HOUSE_TILE_OFFSETS[] = {
    OFFSET(0,0), OFFSET(1,0), OFFSET(0,1), OFFSET(1,1), // 2x2
    OFFSET(2,0), OFFSET(2,1), OFFSET(2,2), OFFSET(1,2), OFFSET(0,2), // 3x3
    OFFSET(3,0), OFFSET(3,1), OFFSET(3,2), OFFSET(3,3), OFFSET(2,3), OFFSET(1,3), OFFSET(0,3) // 4x4
};

static const struct {
    int group;
    int offset;
    int num_types;
} HOUSE_IMAGE[20] = {
    {GROUP_BUILDING_HOUSE_TENT, 0, 2}, {GROUP_BUILDING_HOUSE_TENT, 2, 2},
    {GROUP_BUILDING_HOUSE_SHACK, 0, 2}, {GROUP_BUILDING_HOUSE_SHACK, 2, 2},
    {GROUP_BUILDING_HOUSE_HOVEL, 0, 2}, {GROUP_BUILDING_HOUSE_HOVEL, 2, 2},
    {GROUP_BUILDING_HOUSE_CASA, 0, 2}, {GROUP_BUILDING_HOUSE_CASA, 2, 2},
    {GROUP_BUILDING_HOUSE_INSULA_1, 0, 2}, {GROUP_BUILDING_HOUSE_INSULA_1, 2, 2},
    {GROUP_BUILDING_HOUSE_INSULA_2, 0, 2}, {GROUP_BUILDING_HOUSE_INSULA_2, 2, 2},
    {GROUP_BUILDING_HOUSE_VILLA_1, 0, 2}, {GROUP_BUILDING_HOUSE_VILLA_1, 2, 2},
    {GROUP_BUILDING_HOUSE_VILLA_2, 0, 1}, {GROUP_BUILDING_HOUSE_VILLA_2, 1, 1},
    {GROUP_BUILDING_HOUSE_PALACE_1, 0, 1}, {GROUP_BUILDING_HOUSE_PALACE_1, 1, 1},
    {GROUP_BUILDING_HOUSE_PALACE_2, 0, 1}, {GROUP_BUILDING_HOUSE_PALACE_2, 1, 1},
};

void building_house_change_to(struct building_t *house, int type)
{
    house->type = type;
    house->subtype.house_level = house->type - BUILDING_HOUSE_SMALL_TENT;
    int image_id = image_group(HOUSE_IMAGE[house->subtype.house_level].group);
    if (house->house_is_merged) {
        image_id += 4;
        if (HOUSE_IMAGE[house->subtype.house_level].offset) {
            image_id += 1;
        }
    } else {
        image_id += HOUSE_IMAGE[house->subtype.house_level].offset;
        image_id += map_random_get(house->grid_offset) & (HOUSE_IMAGE[house->subtype.house_level].num_types - 1);
    }
    map_building_tiles_add(house->id, house->x, house->y, house->size, image_id, TERRAIN_BUILDING);
}

static void advance_year(void)
{
    game_undo_disable();
    game_time_advance_year();
    process_empire_expansion();
    city_population_request_yearly_update();
    city_finance_handle_year_change();

    // reset yearly trade amounts
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use && empire_objects[i].trade_route_open) {
            for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                empire_objects[i].resource_bought[r] = 0;
                empire_objects[i].resource_sold[r] = 0;
            }
        }
    }

    fire_spread_direction = random_byte() & 7;
    city_ratings_update(1);
    city_data.religion.neptune_double_trade_active = 0;
}

static void advance_month(void)
{
    city_migration_reset_newcomers();
    city_health_update();
    process_random_event();
    city_finance_handle_month_change();
    city_resource_consume_food();
    city_victory_update_months_to_govern();
    update_legion_morale_monthly();
    city_message_decrease_delays();

    map_tiles_update_all_roads();
    map_tiles_update_all_water();
    map_routing_update_land_citizen();
    city_message_sort_and_compact();

    if (game_time_advance_month()) {
        advance_year();
    } else {
        city_ratings_update(0);
    }
    process_custom_messages();
    process_gladiator_revolt();
    process_imperial_requests();
    process_price_changes();
    process_demand_changes();
    process_invasions();
    process_distant_battles();
    // record monthly population
    city_data.population.monthly.values[city_data.population.monthly.next_index++] = city_data.population.population;
    if (city_data.population.monthly.next_index >= 2400) {
        city_data.population.monthly.next_index = 0;
    }
    ++city_data.population.monthly.count;
    city_festival_update();
    if (setting_monthly_autosave()) {
        game_file_io_write_saved_game(SAVES_DIR_PATH, "autosave.sav");
    }
}

static void advance_day(void)
{
    if (game_time_advance_day()) {
        advance_month();
    }
    if (game_time_day() == 0 || game_time_day() == 8) {
        city_sentiment_update();
    }
}

static void building_dock_update_open_water_access(void)
{
    map_routing_calculate_distances_water_boat(scenario.river_entry_point.x, scenario.river_entry_point.y);
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && !b->house_size && b->type == BUILDING_DOCK) {
            if (map_terrain_is_adjacent_to_open_water(b->x, b->y, 3)) {
                b->has_water_access = 1;
            } else {
                b->has_water_access = 0;
            }
        }
    }
}

static void house_service_decay_houses_covered(void)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_UNUSED && b->type != BUILDING_TOWER) {
            if (b->houses_covered <= 1) {
                b->houses_covered = 0;
            } else {
                b->houses_covered--;
            }
        }
    }
}

static void house_population_evict_overcrowded(void)
{
    int size = building_list_large_size();
    const int *items = building_list_large_items();
    for (int i = 0; i < size; i++) {
        struct building_t *b = &all_buildings[items[i]];
        if (b->house_population_room < 0) {
            int num_people_to_evict = -b->house_population_room;
            figure_create_homeless(b->x, b->y, num_people_to_evict);
            if (num_people_to_evict < b->house_population) {
                b->house_population -= num_people_to_evict;
            } else {
                // house has been removed
                b->state = BUILDING_STATE_UNDO;
            }
        }
    }
}

static void building_maintenance_check_rome_access(void)
{
    map_routing_calculate_distances(scenario.entry_point.x, scenario.entry_point.y);
    int problem_grid_offset = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        if (b->house_size) {
            int x_road, y_road;
            if (!map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                // no road: eject people
                b->distance_from_entry = 0;
                b->house_unreachable_ticks++;
                if (b->house_unreachable_ticks > 4) {
                    if (b->house_population) {
                        figure_create_homeless(b->x, b->y, b->house_population);
                        b->house_population = 0;
                        b->house_unreachable_ticks = 0;
                    }
                    b->state = BUILDING_STATE_UNDO;
                }
            } else if (map_routing_distance(map_grid_offset(x_road, y_road))) {
                // reachable from rome
                b->distance_from_entry = map_routing_distance(map_grid_offset(x_road, y_road));
                b->house_unreachable_ticks = 0;
            } else if (map_closest_reachable_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                b->distance_from_entry = map_routing_distance(map_grid_offset(x_road, y_road));
                b->house_unreachable_ticks = 0;
            } else {
                // no reachable road in radius
                if (!b->house_unreachable_ticks) {
                    problem_grid_offset = b->grid_offset;
                }
                b->house_unreachable_ticks++;
                if (b->house_unreachable_ticks > 8) {
                    b->distance_from_entry = 0;
                    b->house_unreachable_ticks = 0;
                    b->state = BUILDING_STATE_UNDO;
                }
            }
        } else if (b->type == BUILDING_WAREHOUSE) {
            if (!city_data.building.trade_center_building_id) {
                city_data.building.trade_center_building_id = i;
            }
            b->distance_from_entry = 0;
            int x_road, y_road;
            int road_grid_offset = map_road_to_largest_network(b->x, b->y, 3, &x_road, &y_road);
            if (road_grid_offset >= 0) {
                b->road_network_id = map_road_network_get(road_grid_offset);
                b->distance_from_entry = map_routing_distance(road_grid_offset);
                b->road_access_x = x_road;
                b->road_access_y = y_road;
            }
        } else if (b->type == BUILDING_WAREHOUSE_SPACE) {
            b->distance_from_entry = 0;
            struct building_t *main_building = building_main(b);
            b->road_network_id = main_building->road_network_id;
            b->distance_from_entry = main_building->distance_from_entry;
            b->road_access_x = main_building->road_access_x;
            b->road_access_y = main_building->road_access_y;
        } else if (b->type == BUILDING_HIPPODROME) {
            b->distance_from_entry = 0;
            int x_road, y_road;
            int road_grid_offset = map_road_to_largest_network_hippodrome(b->x, b->y, &x_road, &y_road);
            if (road_grid_offset >= 0) {
                b->road_network_id = map_road_network_get(road_grid_offset);
                b->distance_from_entry = map_routing_distance(road_grid_offset);
                b->road_access_x = x_road;
                b->road_access_y = y_road;
            }
        } else { // other building
            b->distance_from_entry = 0;
            int x_road, y_road;
            int road_grid_offset = map_road_to_largest_network(b->x, b->y, b->size, &x_road, &y_road);
            if (road_grid_offset >= 0) {
                b->road_network_id = map_road_network_get(road_grid_offset);
                b->distance_from_entry = map_routing_distance(road_grid_offset);
                b->road_access_x = x_road;
                b->road_access_y = y_road;
            }
        }
    }
    if (!map_routing_distance(map_grid_offset(scenario.exit_point.x, scenario.exit_point.y))) {
        // no route through city
        if (city_data.population.population <= 0) {
            return;
        }
        for (int i = 0; i < 15; i++) {
            map_routing_delete_first_wall_or_aqueduct(scenario.entry_point.x, scenario.entry_point.y);
            map_routing_delete_first_wall_or_aqueduct(scenario.exit_point.x, scenario.exit_point.y);
            map_routing_calculate_distances(scenario.entry_point.x, scenario.entry_point.y);

            map_tiles_update_all_walls();
            map_tiles_update_all_aqueducts(0);
            map_tiles_update_all_empty_land();

            map_routing_update_land();
            map_routing_update_walls();

            if (map_routing_distance(map_grid_offset(scenario.exit_point.x, scenario.exit_point.y))) {
                city_message_post(1, MESSAGE_ROAD_TO_ROME_OBSTRUCTED, 0, 0);
                game_undo_disable();
                return;
            }
        }
        int highest_sequence = 0;
        struct building_t *last_building = 0;
        for (int i = 1; i < MAX_BUILDINGS; i++) {
            struct building_t *b = &all_buildings[i];
            if (b->state == BUILDING_STATE_CREATED || b->state == BUILDING_STATE_IN_USE) {
                if (b->created_sequence > highest_sequence) {
                    highest_sequence = b->created_sequence;
                    last_building = b;
                }
            }
        }
        if (last_building) {
            city_message_post(1, MESSAGE_ROAD_TO_ROME_BLOCKED, 0, last_building->grid_offset);
            game_undo_disable();
            building_destroy_by_collapse(last_building);
            map_routing_update_land();
        }
    } else if (problem_grid_offset) {
        // parts of city disconnected
        city_warning_show(WARNING_CITY_BOXED_IN);
        city_warning_show(WARNING_CITY_BOXED_IN_PEOPLE_WILL_PERISH);
        city_view_go_to_grid_offset(problem_grid_offset);
    }
}

static void building_maintenance_check_fire_collapse(void)
{
    city_sentiment_reset_protesters_criminals();

    int recalculate_terrain = 0;
    int random_global = random_byte() & 7;
    int max_id = building_get_highest_id();
    for (int i = 1; i <= max_id; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->fire_proof) {
            continue;
        }
        if (b->type == BUILDING_HIPPODROME && b->prev_part_building_id) {
            continue;
        }
        int random_building = (i + map_random_get(b->grid_offset)) & 7;
        // damage
        b->damage_risk += random_building == random_global ? 3 : 1;
        if (b->house_size && b->subtype.house_level <= HOUSE_LARGE_TENT) {
            b->damage_risk = 0;
        }
        if (b->damage_risk > 200) {
            city_message_apply_sound_interval(MESSAGE_CAT_COLLAPSE);
            city_message_post_with_popup_delay(MESSAGE_CAT_COLLAPSE, MESSAGE_COLLAPSED_BUILDING, b->type, b->grid_offset);
            game_undo_disable();
            building_destroy_by_collapse(b);
            recalculate_terrain = 1;
            continue;
        }
        // fire
        if (random_building == random_global) {
            if (!b->house_size) {
                b->fire_risk += 5;
            } else if (b->house_population <= 0) {
                b->fire_risk = 0;
            } else if (b->subtype.house_level <= HOUSE_LARGE_SHACK) {
                b->fire_risk += 10;
            } else if (b->subtype.house_level <= HOUSE_GRAND_INSULA) {
                b->fire_risk += 5;
            } else {
                b->fire_risk += 2;
            }
            if (scenario.climate == CLIMATE_NORTHERN) {
                b->fire_risk = 0;
            } else if (scenario.climate == CLIMATE_DESERT) {
                b->fire_risk += 3;
            }
        }
        if (b->fire_risk > 100) {
            city_message_apply_sound_interval(MESSAGE_CAT_FIRE);
            city_message_post_with_popup_delay(MESSAGE_CAT_FIRE, MESSAGE_FIRE, b->type, b->grid_offset);
            building_destroy_by_fire(b);
            play_sound_effect(SOUND_EFFECT_EXPLOSION);
            recalculate_terrain = 1;
        }
    }

    if (recalculate_terrain) {
        map_routing_update_land();
    }
}

static void house_population_update_migration(void)
{
    city_migration_update();

    city_population_yearly_update();
    int num_plebs = 0;
    int num_patricians = 0;
    int total_houses = building_list_large_size();
    const int *houses = building_list_large_items();
    for (int i = 0; i < total_houses; i++) {
        struct building_t *b = &all_buildings[houses[i]];
        if (b->house_population > 0) {
            if (b->subtype.house_level >= HOUSE_SMALL_VILLA) {
                num_patricians += b->house_population;
            } else {
                num_plebs += b->house_population;
            }
        }
    }
    city_labor_calculate_workers(num_plebs, num_patricians);
    // population messages
    if (city_data.population.population >= 500 && city_message_mark_population_shown(500)) {
        city_message_post(1, MESSAGE_POPULATION_500, 0, 0);
    }
    if (city_data.population.population >= 1000 && city_message_mark_population_shown(1000)) {
        city_message_post(1, MESSAGE_POPULATION_1000, 0, 0);
    }
    if (city_data.population.population >= 2000 && city_message_mark_population_shown(2000)) {
        city_message_post(1, MESSAGE_POPULATION_2000, 0, 0);
    }
    if (city_data.population.population >= 3000 && city_message_mark_population_shown(3000)) {
        city_message_post(1, MESSAGE_POPULATION_3000, 0, 0);
    }
    if (city_data.population.population >= 5000 && city_message_mark_population_shown(5000)) {
        city_message_post(1, MESSAGE_POPULATION_5000, 0, 0);
    }
    if (city_data.population.population >= 10000 && city_message_mark_population_shown(10000)) {
        city_message_post(1, MESSAGE_POPULATION_10000, 0, 0);
    }
    if (city_data.population.population >= 15000 && city_message_mark_population_shown(15000)) {
        city_message_post(1, MESSAGE_POPULATION_15000, 0, 0);
    }
    if (city_data.population.population >= 20000 && city_message_mark_population_shown(20000)) {
        city_message_post(1, MESSAGE_POPULATION_20000, 0, 0);
    }
    if (city_data.population.population >= 25000 && city_message_mark_population_shown(25000)) {
        city_message_post(1, MESSAGE_POPULATION_25000, 0, 0);
    }
}

static void house_population_update_room(void)
{
    city_data.population.total_capacity = 0;
    city_data.population.room_in_houses = 0;

    building_list_large_clear(0);
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size) {
            building_list_large_add(i);
        }
    }
    int total_houses = building_list_large_size();
    const int *houses = building_list_large_items();
    for (int i = 0; i < total_houses; i++) {
        struct building_t *b = &all_buildings[houses[i]];
        b->house_population_room = 0;
        if (b->distance_from_entry > 0) {
            int max_pop = house_properties[b->subtype.house_level].max_people;
            if (b->house_is_merged) {
                max_pop *= 4;
            }
            city_data.population.total_capacity += max_pop;
            city_data.population.room_in_houses += max_pop - b->house_population;
            b->house_population_room = max_pop - b->house_population;
            if (b->house_population > b->house_highest_population) {
                b->house_highest_population = b->house_population;
            }
        } else if (b->house_population) {
            // not connected to Rome, mark people for eviction
            b->house_population_room = -b->house_population;
        }
    }
}

static void building_industry_update_production(void)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || !b->output_resource_id) {
            continue;
        }
        b->data.industry.has_raw_materials = 0;
        if (b->houses_covered <= 0 || b->num_workers <= 0) {
            continue;
        }
        if (b->subtype.workshop_type && !b->loads_stored) {
            continue;
        }
        if (b->data.industry.curse_days_left) {
            b->data.industry.curse_days_left--;
        } else {
            if (b->data.industry.blessing_days_left) {
                b->data.industry.blessing_days_left--;
            }
            if (b->type == BUILDING_MARBLE_QUARRY) {
                b->data.industry.progress += b->num_workers / 2;
            } else {
                b->data.industry.progress += b->num_workers;
            }
            if (b->data.industry.blessing_days_left && building_is_farm(b->type)) {
                b->data.industry.progress += b->num_workers;
            }
            int max = b->subtype.workshop_type ? MAX_PROGRESS_WORKSHOP : MAX_PROGRESS_RAW;
            if (b->data.industry.progress > max) {
                b->data.industry.progress = max;
            }
            if (building_is_farm(b->type)) {
                update_farm_image(b);
            }
        }
    }
}

static void decay(unsigned char *value)
{
    if (*value > 0) {
        *value = *value - 1;
    } else {
        *value = 0;
    }
}

static void house_service_decay_culture(void)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || !b->house_size) {
            continue;
        }
        decay(&b->data.house.theater);
        decay(&b->data.house.amphitheater_actor);
        decay(&b->data.house.amphitheater_gladiator);
        decay(&b->data.house.colosseum_gladiator);
        decay(&b->data.house.colosseum_lion);
        decay(&b->data.house.hippodrome);
        decay(&b->data.house.school);
        decay(&b->data.house.library);
        decay(&b->data.house.academy);
        decay(&b->data.house.barber);
        decay(&b->data.house.clinic);
        decay(&b->data.house.bathhouse);
        decay(&b->data.house.hospital);
        decay(&b->data.house.temple_ceres);
        decay(&b->data.house.temple_neptune);
        decay(&b->data.house.temple_mercury);
        decay(&b->data.house.temple_mars);
        decay(&b->data.house.temple_venus);
    }
}

static void house_service_calculate_culture_aggregates(void)
{
    int base_entertainment = city_culture_coverage_average_entertainment() / 5;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || !b->house_size) {
            continue;
        }
        // entertainment
        b->data.house.entertainment = base_entertainment;
        if (b->data.house.theater) {
            b->data.house.entertainment += 10;
        }
        if (b->data.house.amphitheater_actor) {
            if (b->data.house.amphitheater_gladiator) {
                b->data.house.entertainment += 15;
            } else {
                b->data.house.entertainment += 10;
            }
        }
        if (b->data.house.colosseum_gladiator) {
            if (b->data.house.colosseum_lion) {
                b->data.house.entertainment += 25;
            } else {
                b->data.house.entertainment += 15;
            }
        }
        if (b->data.house.hippodrome) {
            b->data.house.entertainment += 30;
        }
        // education
        b->data.house.education = 0;
        // release build mingw doesn't like school || library for some reason
        if (b->data.house.school) {
            b->data.house.education = 1;
        }
        if (b->data.house.library) {
            b->data.house.education = 1;
        }
        if (b->data.house.school && b->data.house.library) {
            b->data.house.education = 2;
            if (b->data.house.academy) {
                b->data.house.education = 3;
            }
        }
        // religion
        b->data.house.num_gods = 0;
        if (b->data.house.temple_ceres) {
            ++b->data.house.num_gods;
        }
        if (b->data.house.temple_neptune) {
            ++b->data.house.num_gods;
        }
        if (b->data.house.temple_mercury) {
            ++b->data.house.num_gods;
        }
        if (b->data.house.temple_mars) {
            ++b->data.house.num_gods;
        }
        if (b->data.house.temple_venus) {
            ++b->data.house.num_gods;
        }
        // health
        b->data.house.health = 0;
        if (b->data.house.clinic) {
            ++b->data.house.health;
        }
        if (b->data.house.hospital) {
            ++b->data.house.health;
        }
    }
}

static int check_evolve_desirability(struct building_t *house)
{
    int level = house->subtype.house_level;
    int evolve_des = house_properties[level].evolve_desirability;
    if (level >= HOUSE_LUXURY_PALACE) {
        evolve_des = 1000;
    }
    int current_des = house->desirability;
    int status;
    if (current_des <= house_properties[level].devolve_desirability) {
        status = DEVOLVE;
    } else if (current_des >= evolve_des) {
        status = EVOLVE;
    } else {
        status = NONE;
    }
    house->data.house.evolve_text_id = status; // BUG? -1 in an unsigned char?
    return status;
}

static int has_required_goods_and_services(struct building_t *house, int for_upgrade, struct house_demands_t *demands)
{
    int level = house->subtype.house_level;
    if (for_upgrade) {
        ++level;
    }
    // water
    if (!house->has_water_access) {
        if (house_properties[level].water >= 2) {
            ++demands->missing.fountain;
            return 0;
        }
        if (house_properties[level].water == 1 && !house->has_well_access) {
            ++demands->missing.well;
            return 0;
        }
    }
    // entertainment
    if (house->data.house.entertainment < house_properties[level].entertainment) {
        if (house->data.house.entertainment) {
            ++demands->missing.more_entertainment;
        } else {
            ++demands->missing.entertainment;
        }
        return 0;
    }
    // education
    if (house->data.house.education < house_properties[level].education) {
        if (house->data.house.education) {
            ++demands->missing.more_education;
        } else {
            ++demands->missing.education;
        }
        return 0;
    }
    if (house_properties[level].education == 2) {
        ++demands->requiring.school;
        ++demands->requiring.library;
    } else if (house_properties[level].education == 1) {
        ++demands->requiring.school;
    }
    // religion
    if (house->data.house.num_gods < house_properties[level].religion) {
        if (house_properties[level].religion == 1) {
            ++demands->missing.religion;
            return 0;
        } else if (house_properties[level].religion == 2) {
            ++demands->missing.second_religion;
            return 0;
        } else if (house_properties[level].religion == 3) {
            ++demands->missing.third_religion;
            return 0;
        }
    } else if (house_properties[level].religion > 0) {
        ++demands->requiring.religion;
    }
    // barber
    if (house->data.house.barber < house_properties[level].barber) {
        ++demands->missing.barber;
        return 0;
    }
    if (house_properties[level].barber == 1) {
        ++demands->requiring.barber;
    }
    // bathhouse
    if (house->data.house.bathhouse < house_properties[level].bathhouse) {
        ++demands->missing.bathhouse;
        return 0;
    }
    if (house_properties[level].bathhouse == 1) {
        ++demands->requiring.bathhouse;
    }
    // health
    if (house->data.house.health < house_properties[level].health) {
        if (house_properties[level].health < 2) {
            ++demands->missing.clinic;
        } else {
            ++demands->missing.hospital;
        }
        return 0;
    }
    if (house_properties[level].health >= 1) {
        ++demands->requiring.clinic;
    }
    // food types
    int foodtypes_available = 0;
    for (int i = INVENTORY_WHEAT; i <= INVENTORY_MEAT; i++) {
        if (house->data.house.inventory[i]) {
            foodtypes_available++;
        }
    }
    if (foodtypes_available < house_properties[level].food_types) {
        ++demands->missing.food;
        return 0;
    }
    // goods
    if (house->data.house.inventory[INVENTORY_POTTERY] < house_properties[level].pottery) {
        return 0;
    }
    if (house->data.house.inventory[INVENTORY_OIL] < house_properties[level].oil) {
        return 0;
    }
    if (house->data.house.inventory[INVENTORY_FURNITURE] < house_properties[level].furniture) {
        return 0;
    }
    int wine_required = house_properties[level].wine;
    if (wine_required && !house->data.house.inventory[INVENTORY_WINE]) {
        return 0;
    }
    if (wine_required > 1 && !city_resource_multiple_wine_available()) {
        ++demands->missing.second_wine;
        return 0;
    }
    return 1;
}

static int check_requirements(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_evolve_desirability(house);
    if (!has_required_goods_and_services(house, 0, demands)) {
        status = DEVOLVE;
    } else if (status == EVOLVE) {
        status = has_required_goods_and_services(house, 1, demands);
    }
    return status;
}

static void prepare_for_merge(int building_id, int num_tiles)
{
    for (int i = 0; i < INVENTORY_MAX; i++) {
        merge_data.inventory[i] = 0;
    }
    merge_data.population = 0;
    int grid_offset = map_grid_offset(merge_data.x, merge_data.y);
    for (int i = 0; i < num_tiles; i++) {
        int house_offset = grid_offset + HOUSE_TILE_OFFSETS[i];
        if (map_terrain_is(house_offset, TERRAIN_BUILDING)) {
            struct building_t *house = &all_buildings[map_building_at(house_offset)];
            if (house->id != building_id && house->house_size) {
                merge_data.population += house->house_population;
                for (int inv = 0; inv < INVENTORY_MAX; inv++) {
                    merge_data.inventory[inv] += house->data.house.inventory[inv];
                    house->house_population = 0;
                    house->state = BUILDING_STATE_DELETED_BY_GAME;
                }
            }
        }
    }
}

static void building_house_merge(struct building_t *house)
{
    if (house->house_is_merged) {
        return;
    }
    if ((map_random_get(house->grid_offset) & 7) >= 5) {
        return;
    }
    int num_house_tiles = 0;
    for (int i = 0; i < 4; i++) {
        int tile_offset = house->grid_offset + HOUSE_TILE_OFFSETS[i];
        if (map_terrain_is(tile_offset, TERRAIN_BUILDING)) {
            struct building_t *other_house = &all_buildings[map_building_at(tile_offset)];
            if (other_house->id == house->id) {
                num_house_tiles++;
            } else if (other_house->state == BUILDING_STATE_IN_USE && other_house->house_size &&
                    other_house->subtype.house_level == house->subtype.house_level &&
                    !other_house->house_is_merged) {
                num_house_tiles++;
            }
        }
    }
    if (num_house_tiles == 4) {
        game_undo_disable();
        merge_data.x = house->x + EXPAND_DIRECTION_DELTA[0].x;
        merge_data.y = house->y + EXPAND_DIRECTION_DELTA[0].y;
        prepare_for_merge(house->id, 4);
        house->size = house->house_size = 2;
        house->house_population += merge_data.population;
        for (int i = 0; i < INVENTORY_MAX; i++) {
            house->data.house.inventory[i] += merge_data.inventory[i];
        }
        int image_id = image_group(HOUSE_IMAGE[house->subtype.house_level].group) + 4;
        if (HOUSE_IMAGE[house->subtype.house_level].offset) {
            image_id += 1;
        }
        map_building_tiles_remove(house->id, house->x, house->y);
        house->x = merge_data.x;
        house->y = merge_data.y;
        house->grid_offset = map_grid_offset(house->x, house->y);
        house->house_is_merged = 1;
        map_building_tiles_add(house->id, house->x, house->y, 2, image_id, TERRAIN_BUILDING);
    }
}

static int evolve_small_tent(struct building_t *house, struct house_demands_t *demands)
{
    if (house->house_population > 0) {
        building_house_merge(house);
        int status = check_requirements(house, demands);
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_TENT);
        }
    }
    return 0;
}

static int has_devolve_delay(struct building_t *house, int status)
{
    if (status == DEVOLVE && house->data.house.devolve_delay < 2) {
        house->data.house.devolve_delay++;
        return 1;
    } else {
        house->data.house.devolve_delay = 0;
        return 0;
    }
}

static int evolve_large_tent(struct building_t *house, struct house_demands_t *demands)
{
    if (house->house_population > 0) {
        building_house_merge(house);
        int status = check_requirements(house, demands);
        if (!has_devolve_delay(house, status)) {
            if (status == EVOLVE) {
                building_house_change_to(house, BUILDING_HOUSE_SMALL_SHACK);
            } else if (status == DEVOLVE) {
                building_house_change_to(house, BUILDING_HOUSE_SMALL_TENT);
            }
        }
    }
    return 0;
}

static int evolve_small_shack(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_SHACK);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_TENT);
        }
    }
    return 0;
}

static int evolve_large_shack(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_HOVEL);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_SHACK);
        }
    }
    return 0;
}

static int evolve_small_hovel(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_HOVEL);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_SHACK);
        }
    }
    return 0;
}

static int evolve_large_hovel(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_CASA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_HOVEL);
        }
    }
    return 0;
}

static int evolve_small_casa(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_CASA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_HOVEL);
        }
    }
    return 0;
}

static int evolve_large_casa(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_INSULA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_CASA);
        }
    }
    return 0;
}

static int evolve_small_insula(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_MEDIUM_INSULA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_CASA);
        }
    }
    return 0;
}

static int building_house_can_expand(struct building_t *house, int num_tiles)
{
    // merge with other houses
    for (int dir = 0; dir < MAX_DIR; dir++) {
        int base_offset = EXPAND_DIRECTION_DELTA[dir].offset + house->grid_offset;
        int ok_tiles = 0;
        for (int i = 0; i < num_tiles; i++) {
            int tile_offset = base_offset + HOUSE_TILE_OFFSETS[i];
            if (map_terrain_is(tile_offset, TERRAIN_BUILDING)) {
                struct building_t *other_house = &all_buildings[map_building_at(tile_offset)];
                if (other_house->id == house->id) {
                    ok_tiles++;
                } else if (other_house->state == BUILDING_STATE_IN_USE && other_house->house_size) {
                    if (other_house->subtype.house_level <= house->subtype.house_level) {
                        ok_tiles++;
                    }
                }
            }
        }
        if (ok_tiles == num_tiles) {
            merge_data.x = house->x + EXPAND_DIRECTION_DELTA[dir].x;
            merge_data.y = house->y + EXPAND_DIRECTION_DELTA[dir].y;
            return 1;
        }
    }
    // merge with houses and empty terrain
    for (int dir = 0; dir < MAX_DIR; dir++) {
        int base_offset = EXPAND_DIRECTION_DELTA[dir].offset + house->grid_offset;
        int ok_tiles = 0;
        for (int i = 0; i < num_tiles; i++) {
            int tile_offset = base_offset + HOUSE_TILE_OFFSETS[i];
            if (!map_terrain_is(tile_offset, TERRAIN_NOT_CLEAR)) {
                ok_tiles++;
            } else if (map_terrain_is(tile_offset, TERRAIN_BUILDING)) {
                struct building_t *other_house = &all_buildings[map_building_at(tile_offset)];
                if (other_house->id == house->id) {
                    ok_tiles++;
                } else if (other_house->state == BUILDING_STATE_IN_USE && other_house->house_size) {
                    if (other_house->subtype.house_level <= house->subtype.house_level) {
                        ok_tiles++;
                    }
                }
            }
        }
        if (ok_tiles == num_tiles) {
            merge_data.x = house->x + EXPAND_DIRECTION_DELTA[dir].x;
            merge_data.y = house->y + EXPAND_DIRECTION_DELTA[dir].y;
            return 1;
        }
    }
    // merge with houses, empty terrain and gardens
    for (int dir = 0; dir < MAX_DIR; dir++) {
        int base_offset = EXPAND_DIRECTION_DELTA[dir].offset + house->grid_offset;
        int ok_tiles = 0;
        for (int i = 0; i < num_tiles; i++) {
            int tile_offset = base_offset + HOUSE_TILE_OFFSETS[i];
            if (!map_terrain_is(tile_offset, TERRAIN_NOT_CLEAR)) {
                ok_tiles++;
            } else if (map_terrain_is(tile_offset, TERRAIN_BUILDING)) {
                struct building_t *other_house = &all_buildings[map_building_at(tile_offset)];
                if (other_house->id == house->id) {
                    ok_tiles++;
                } else if (other_house->state == BUILDING_STATE_IN_USE && other_house->house_size) {
                    if (other_house->subtype.house_level <= house->subtype.house_level) {
                        ok_tiles++;
                    }
                }
            } else if (map_terrain_is(tile_offset, TERRAIN_GARDEN)) {
                ok_tiles++;
            }
        }
        if (ok_tiles == num_tiles) {
            merge_data.x = house->x + EXPAND_DIRECTION_DELTA[dir].x;
            merge_data.y = house->y + EXPAND_DIRECTION_DELTA[dir].y;
            return 1;
        }
    }
    house->data.house.no_space_to_expand = 1;
    return 0;
}

static int house_image_group(int level)
{
    return image_group(HOUSE_IMAGE[level].group) + HOUSE_IMAGE[level].offset;
}

static void create_house_tile(int type, int x, int y, int image_id, int population, const int *inventory)
{
    struct building_t *house = building_create(type, x, y);
    house->house_population = population;
    for (int i = 0; i < INVENTORY_MAX; i++) {
        house->data.house.inventory[i] = inventory[i];
    }
    house->distance_from_entry = 0;
    map_building_tiles_add(house->id, house->x, house->y, 1, image_id + (map_random_get(house->grid_offset) & 1), TERRAIN_BUILDING);
}

static void split_size2(struct building_t *house, int new_type)
{
    int inventory_per_tile[INVENTORY_MAX];
    int inventory_remainder[INVENTORY_MAX];
    for (int i = 0; i < INVENTORY_MAX; i++) {
        inventory_per_tile[i] = house->data.house.inventory[i] / 4;
        inventory_remainder[i] = house->data.house.inventory[i] % 4;
    }
    int population_per_tile = house->house_population / 4;
    int population_remainder = house->house_population % 4;

    map_building_tiles_remove(house->id, house->x, house->y);

    // main tile
    house->type = new_type;
    house->subtype.house_level = house->type - BUILDING_HOUSE_SMALL_TENT;
    house->size = house->house_size = 1;
    house->house_is_merged = 0;
    house->house_population = population_per_tile + population_remainder;
    for (int i = 0; i < INVENTORY_MAX; i++) {
        house->data.house.inventory[i] = inventory_per_tile[i] + inventory_remainder[i];
    }
    house->distance_from_entry = 0;

    int image_id = house_image_group(house->subtype.house_level);
    map_building_tiles_add(house->id, house->x, house->y, house->size,
                           image_id + (map_random_get(house->grid_offset) & 1), TERRAIN_BUILDING);

    // the other tiles (new buildings)
    create_house_tile(house->type, house->x + 1, house->y, image_id, population_per_tile, inventory_per_tile);
    create_house_tile(house->type, house->x, house->y + 1, image_id, population_per_tile, inventory_per_tile);
    create_house_tile(house->type, house->x + 1, house->y + 1, image_id, population_per_tile, inventory_per_tile);
}

static void split(struct building_t *house, int num_tiles)
{
    int grid_offset = map_grid_offset(merge_data.x, merge_data.y);
    for (int i = 0; i < num_tiles; i++) {
        int tile_offset = grid_offset + HOUSE_TILE_OFFSETS[i];
        if (map_terrain_is(tile_offset, TERRAIN_BUILDING)) {
            struct building_t *other_house = &all_buildings[map_building_at(tile_offset)];
            if (other_house->id != house->id && other_house->house_size) {
                if (other_house->house_is_merged == 1) {
                    split_size2(other_house, other_house->type);
                } else if (other_house->house_size == 2) {
                    split_size2(other_house, BUILDING_HOUSE_MEDIUM_INSULA);
                } else if (other_house->house_size == 3) {
                    int inventory_per_tile[INVENTORY_MAX];
                    int inventory_remainder[INVENTORY_MAX];
                    for (int ii = 0; ii < INVENTORY_MAX; ii++) {
                        inventory_per_tile[ii] = other_house->data.house.inventory[ii] / 9;
                        inventory_remainder[ii] = other_house->data.house.inventory[ii] % 9;
                    }
                    int population_per_tile = other_house->house_population / 9;
                    int population_remainder = other_house->house_population % 9;
                    map_building_tiles_remove(other_house->id, other_house->x, other_house->y);
                    // main tile
                    other_house->type = BUILDING_HOUSE_MEDIUM_INSULA;
                    other_house->subtype.house_level = other_house->type - BUILDING_HOUSE_SMALL_TENT;
                    other_house->size = other_house->house_size = 1;
                    other_house->house_is_merged = 0;
                    other_house->house_population = population_per_tile + population_remainder;
                    for (int ii = 0; ii < INVENTORY_MAX; ii++) {
                        other_house->data.house.inventory[ii] = inventory_per_tile[ii] + inventory_remainder[ii];
                    }
                    other_house->distance_from_entry = 0;

                    int image_id = house_image_group(other_house->subtype.house_level);
                    map_building_tiles_add(other_house->id, other_house->x, other_house->y, other_house->size,
                                           image_id + (map_random_get(other_house->grid_offset) & 1), TERRAIN_BUILDING);
                    // the other tiles (new buildings)
                    create_house_tile(other_house->type, other_house->x, other_house->y + 1, image_id, population_per_tile, inventory_per_tile);
                    create_house_tile(other_house->type, other_house->x + 1, other_house->y + 1, image_id, population_per_tile, inventory_per_tile);
                    create_house_tile(other_house->type, other_house->x + 2, other_house->y + 1, image_id, population_per_tile, inventory_per_tile);
                    create_house_tile(other_house->type, other_house->x, other_house->y + 2, image_id, population_per_tile, inventory_per_tile);
                    create_house_tile(other_house->type, other_house->x + 1, other_house->y + 2, image_id, population_per_tile, inventory_per_tile);
                    create_house_tile(other_house->type, other_house->x + 2, other_house->y + 2, image_id, population_per_tile, inventory_per_tile);
                }
            }
        }
    }
}

static int evolve_medium_insula(struct building_t *house, struct house_demands_t *demands)
{
    building_house_merge(house);
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            if (building_house_can_expand(house, 4)) {
                game_undo_disable();
                house->house_is_merged = 0;
                split(house, 4);
                prepare_for_merge(house->id, 4);

                house->type = BUILDING_HOUSE_LARGE_INSULA;
                house->subtype.house_level = HOUSE_LARGE_INSULA;
                house->size = house->house_size = 2;
                house->house_population += merge_data.population;
                for (int i = 0; i < INVENTORY_MAX; i++) {
                    house->data.house.inventory[i] += merge_data.inventory[i];
                }
                int image_id = house_image_group(house->subtype.house_level) + (map_random_get(house->grid_offset) & 1);
                map_building_tiles_remove(house->id, house->x, house->y);
                house->x = merge_data.x;
                house->y = merge_data.y;
                house->grid_offset = map_grid_offset(house->x, house->y);
                map_building_tiles_add(house->id, house->x, house->y, house->size, image_id, TERRAIN_BUILDING);
                map_tiles_update_all_gardens();
                return 1;
            }
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_INSULA);
        }
    }
    return 0;
}

static int evolve_large_insula(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_INSULA);
        } else if (status == DEVOLVE) {
            game_undo_disable();
            split_size2(house, BUILDING_HOUSE_MEDIUM_INSULA);
        }
    }
    return 0;
}

static int evolve_grand_insula(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_VILLA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_INSULA);
        }
    }
    return 0;
}

static int evolve_small_villa(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_MEDIUM_VILLA);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_INSULA);
        }
    }
    return 0;
}

static int evolve_medium_villa(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            if (building_house_can_expand(house, 9)) {
                game_undo_disable();
                split(house, 9);
                prepare_for_merge(house->id, 9);

                house->type = BUILDING_HOUSE_LARGE_VILLA;
                house->subtype.house_level = HOUSE_LARGE_VILLA;
                house->size = house->house_size = 3;
                house->house_population += merge_data.population;
                for (int i = 0; i < INVENTORY_MAX; i++) {
                    house->data.house.inventory[i] += merge_data.inventory[i];
                }
                int image_id = house_image_group(house->subtype.house_level);
                map_building_tiles_remove(house->id, house->x, house->y);
                house->x = merge_data.x;
                house->y = merge_data.y;
                house->grid_offset = map_grid_offset(house->x, house->y);
                map_building_tiles_add(house->id, house->x, house->y, house->size, image_id, TERRAIN_BUILDING);
                map_tiles_update_all_gardens();
                return 1;
            }
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_VILLA);
        }
    }
    return 0;
}

static int evolve_large_villa(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_VILLA);
        } else if (status == DEVOLVE) {
            game_undo_disable();
            int inventory_per_tile[INVENTORY_MAX];
            int inventory_remainder[INVENTORY_MAX];
            for (int i = 0; i < INVENTORY_MAX; i++) {
                inventory_per_tile[i] = house->data.house.inventory[i] / 6;
                inventory_remainder[i] = house->data.house.inventory[i] % 6;
            }
            int population_per_tile = house->house_population / 6;
            int population_remainder = house->house_population % 6;

            map_building_tiles_remove(house->id, house->x, house->y);

            // main tile
            house->type = BUILDING_HOUSE_MEDIUM_VILLA;
            house->subtype.house_level = house->type - BUILDING_HOUSE_SMALL_TENT;
            house->size = house->house_size = 2;
            house->house_is_merged = 0;
            house->house_population = population_per_tile + population_remainder;
            for (int i = 0; i < INVENTORY_MAX; i++) {
                house->data.house.inventory[i] = inventory_per_tile[i] + inventory_remainder[i];
            }
            house->distance_from_entry = 0;

            int image_id = house_image_group(house->subtype.house_level);
            map_building_tiles_add(house->id, house->x, house->y, house->size, image_id + (map_random_get(house->grid_offset) & 1), TERRAIN_BUILDING);

            // the other tiles (new buildings)
            image_id = house_image_group(HOUSE_MEDIUM_INSULA);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 2, house->y, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 2, house->y + 1, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x, house->y + 2, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 1, house->y + 2, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 2, house->y + 2, image_id, population_per_tile, inventory_per_tile);
        }
    }
    return 0;
}

static int evolve_grand_villa(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_PALACE);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LARGE_VILLA);
        }
    }
    return 0;
}

static int evolve_small_palace(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_MEDIUM_PALACE);
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_GRAND_VILLA);
        }
    }
    return 0;
}

static int evolve_medium_palace(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            if (building_house_can_expand(house, 16)) {
                game_undo_disable();
                split(house, 16);
                prepare_for_merge(house->id, 16);

                house->type = BUILDING_HOUSE_LARGE_PALACE;
                house->subtype.house_level = HOUSE_LARGE_PALACE;
                house->size = house->house_size = 4;
                house->house_population += merge_data.population;
                for (int i = 0; i < INVENTORY_MAX; i++) {
                    house->data.house.inventory[i] += merge_data.inventory[i];
                }
                int image_id = house_image_group(house->subtype.house_level);
                map_building_tiles_remove(house->id, house->x, house->y);
                house->x = merge_data.x;
                house->y = merge_data.y;
                house->grid_offset = map_grid_offset(house->x, house->y);
                map_building_tiles_add(house->id, house->x, house->y, house->size, image_id, TERRAIN_BUILDING);
                map_tiles_update_all_gardens();
                return 1;
            }
        } else if (status == DEVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_SMALL_PALACE);
        }
    }
    return 0;
}

static int evolve_large_palace(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_requirements(house, demands);
    if (!has_devolve_delay(house, status)) {
        if (status == EVOLVE) {
            building_house_change_to(house, BUILDING_HOUSE_LUXURY_PALACE);
        } else if (status == DEVOLVE) {
            game_undo_disable();
            int inventory_per_tile[INVENTORY_MAX];
            int inventory_remainder[INVENTORY_MAX];
            for (int i = 0; i < INVENTORY_MAX; i++) {
                inventory_per_tile[i] = house->data.house.inventory[i] / 8;
                inventory_remainder[i] = house->data.house.inventory[i] % 8;
            }
            int population_per_tile = house->house_population / 8;
            int population_remainder = house->house_population % 8;

            map_building_tiles_remove(house->id, house->x, house->y);

            // main tile
            house->type = BUILDING_HOUSE_MEDIUM_PALACE;
            house->subtype.house_level = house->type - BUILDING_HOUSE_SMALL_TENT;
            house->size = house->house_size = 3;
            house->house_is_merged = 0;
            house->house_population = population_per_tile + population_remainder;
            for (int i = 0; i < INVENTORY_MAX; i++) {
                house->data.house.inventory[i] = inventory_per_tile[i] + inventory_remainder[i];
            }
            house->distance_from_entry = 0;

            int image_id = house_image_group(house->subtype.house_level);
            map_building_tiles_add(house->id, house->x, house->y, house->size, image_id, TERRAIN_BUILDING);

            // the other tiles (new buildings)
            image_id = house_image_group(HOUSE_MEDIUM_INSULA);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 3, house->y, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 3, house->y + 1, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 3, house->y + 2, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x, house->y + 3, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 1, house->y + 3, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 2, house->y + 3, image_id, population_per_tile, inventory_per_tile);
            create_house_tile(BUILDING_HOUSE_MEDIUM_INSULA, house->x + 3, house->y + 3, image_id, population_per_tile, inventory_per_tile);
        }
    }
    return 0;
}

static int evolve_luxury_palace(struct building_t *house, struct house_demands_t *demands)
{
    int status = check_evolve_desirability(house);
    if (!has_required_goods_and_services(house, 0, demands)) {
        status = DEVOLVE;
    }
    if (!has_devolve_delay(house, status) && status == DEVOLVE) {
        building_house_change_to(house, BUILDING_HOUSE_LARGE_PALACE);
    }
    return 0;
}

static int (*evolve_callback[])(struct building_t *, struct house_demands_t *) = {
    evolve_small_tent, evolve_large_tent, evolve_small_shack, evolve_large_shack,
    evolve_small_hovel, evolve_large_hovel, evolve_small_casa, evolve_large_casa,
    evolve_small_insula, evolve_medium_insula, evolve_large_insula, evolve_grand_insula,
    evolve_small_villa, evolve_medium_villa, evolve_large_villa, evolve_grand_villa,
    evolve_small_palace, evolve_medium_palace, evolve_large_palace, evolve_luxury_palace
};

static void consume_resource(struct building_t *b, int inventory, int amount)
{
    if (amount > 0) {
        if (amount > b->data.house.inventory[inventory]) {
            b->data.house.inventory[inventory] = 0;
        } else {
            b->data.house.inventory[inventory] -= amount;
        }
    }
}

static void building_house_process_evolve_and_consume_goods(void)
{
    city_houses_reset_demands();
    struct house_demands_t *demands = city_houses_demands();
    int has_expanded = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && building_is_house(b->type) && b->type != BUILDING_HOUSE_VACANT_LOT) {
            int calc_grid_offset = map_grid_offset(b->x, b->y);
            b->data.house.no_space_to_expand = 0;
            if (b->grid_offset != calc_grid_offset || map_building_at(b->grid_offset) != b->id) {
                int map_width, map_height;
                map_grid_size(&map_width, &map_height);
                for (int y = 0; y < map_height; y++) {
                    for (int x = 0; x < map_width; x++) {
                        int grid_offset = map_grid_offset(x, y);
                        if (map_building_at(grid_offset) == b->id) {
                            b->grid_offset = grid_offset;
                            b->x = map_grid_offset_to_x(grid_offset);
                            b->y = map_grid_offset_to_y(grid_offset);
                            building_totals_add_corrupted_house(0);
                            return;
                        }
                    }
                }
                building_totals_add_corrupted_house(1);
                b->state = BUILDING_STATE_RUBBLE;
            }
            has_expanded |= evolve_callback[b->type - BUILDING_HOUSE_SMALL_TENT](b, demands);
            if (game_time_day() == 0 || game_time_day() == 7) {
                consume_resource(b, INVENTORY_POTTERY, house_properties[b->subtype.house_level].pottery);
                consume_resource(b, INVENTORY_FURNITURE, house_properties[b->subtype.house_level].furniture);
                consume_resource(b, INVENTORY_OIL, house_properties[b->subtype.house_level].oil);
                consume_resource(b, INVENTORY_WINE, house_properties[b->subtype.house_level].wine);
            }
        }
    }
    if (has_expanded) {
        map_routing_update_land();
    }
}

static void building_update_desirability(void)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        b->desirability = map_desirability_get_max(b->x, b->y, b->size);
    }
}

static void building_maintenance_update_burning_ruins(void)
{
    int recalculate_terrain = 0;
    building_list_burning_clear();
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_BURNING_RUIN) {
            continue;
        }
        if (b->fire_duration < 0) {
            b->fire_duration = 0;
        }
        b->fire_duration++;
        if (b->fire_duration > 32) {
            game_undo_disable();
            b->state = BUILDING_STATE_RUBBLE;
            map_building_tiles_set_rubble(i, b->x, b->y, b->size);
            recalculate_terrain = 1;
            continue;
        }
        if (b->ruin_has_plague) {
            continue;
        }
        building_list_burning_add(i);
        if (scenario.climate == CLIMATE_DESERT) {
            if (b->fire_duration & 3) { // check spread every 4 ticks
                continue;
            }
        } else {
            if (b->fire_duration & 7) { // check spread every 8 ticks
                continue;
            }
        }
        if ((b->house_figure_generation_delay & 3) != (random_byte() & 3)) {
            continue;
        }
        int dir1 = fire_spread_direction - 1;
        if (dir1 < 0) dir1 = 7;
        int dir2 = fire_spread_direction + 1;
        if (dir2 > 7) dir2 = 0;

        int grid_offset = b->grid_offset;
        int next_building_id = map_building_at(grid_offset + map_grid_direction_delta(fire_spread_direction));
        if (next_building_id && !all_buildings[next_building_id].fire_proof) {
            building_destroy_by_fire(&all_buildings[next_building_id]);
            play_sound_effect(SOUND_EFFECT_EXPLOSION);
            recalculate_terrain = 1;
        } else {
            next_building_id = map_building_at(grid_offset + map_grid_direction_delta(dir1));
            if (next_building_id && !all_buildings[next_building_id].fire_proof) {
                building_destroy_by_fire(&all_buildings[next_building_id]);
                play_sound_effect(SOUND_EFFECT_EXPLOSION);
                recalculate_terrain = 1;
            } else {
                next_building_id = map_building_at(grid_offset + map_grid_direction_delta(dir2));
                if (next_building_id && !all_buildings[next_building_id].fire_proof) {
                    building_destroy_by_fire(&all_buildings[next_building_id]);
                    play_sound_effect(SOUND_EFFECT_EXPLOSION);
                    recalculate_terrain = 1;
                }
            }
        }
    }
    if (recalculate_terrain) {
        map_routing_update_land();
    }
}

static void building_industry_update_wheat_production(void)
{
    if (scenario.climate == CLIMATE_NORTHERN) {
        return;
    }
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || !b->output_resource_id) {
            continue;
        }
        if (b->houses_covered <= 0 || b->num_workers <= 0) {
            continue;
        }
        if (b->type == BUILDING_WHEAT_FARM && !b->data.industry.curse_days_left) {
            b->data.industry.progress += b->num_workers;
            if (b->data.industry.blessing_days_left) {
                b->data.industry.progress += b->num_workers;
            }
            if (b->data.industry.progress > MAX_PROGRESS_RAW) {
                b->data.industry.progress = MAX_PROGRESS_RAW;
            }
            update_farm_image(b);
        }
    }
}

static void house_service_decay_tax_collector(void)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_tax_coverage) {
            b->house_tax_coverage--;
        }
    }
}

static void advance_tick(void)
{
    // NB: these ticks are noop:
    // 0, 9, 11, 13, 14, 15, 26, 29, 41, 42, 47
    switch (game_time_tick()) {
        case 1: city_gods_calculate_moods(1); break;
        case 2: update_music(0); break;
        case 3: widget_minimap_invalidate(); break;
        case 4:
            update_debt_state();
            process_caesar_invasion();
            break;
        case 5: formation_update_all(); break;
        case 6: map_natives_check_land(); break;
        case 7: map_road_network_update(); break;
        case 8: building_granaries_calculate_stocks(); break;
        case 10: building_update_highest_id(); break;
        case 12: house_service_decay_houses_covered(); break;
        case 16: city_resource_calculate_warehouse_stocks(); break;
        case 17: city_resource_calculate_food_stocks_and_supply_wheat(); break;
        case 18: city_resource_calculate_workshop_stocks(); break;
        case 19: building_dock_update_open_water_access(); break;
        case 20: building_industry_update_production(); break;
        case 21: building_maintenance_check_rome_access(); break;
        case 22: house_population_update_room(); break;
        case 23: house_population_update_migration(); break;
        case 24: house_population_evict_overcrowded(); break;
        case 25: city_labor_update(); break;
        case 27: map_water_supply_update_reservoir_fountain(); break;
        case 28: map_water_supply_update_houses(); break;
        case 30: widget_minimap_invalidate(); break;
        case 31: building_figure_generate(); break;
        case 32: city_trade_update(); break;
        case 33: building_count_update(); city_culture_update_coverage(); break;
        case 34: distribute_treasury(); break;
        case 35: house_service_decay_culture(); break;
        case 36: house_service_calculate_culture_aggregates(); break;
        case 37: map_desirability_update(); break;
        case 38: building_update_desirability(); break;
        case 39: building_house_process_evolve_and_consume_goods(); break;
        case 40: building_update_state(); break;
        case 43: building_maintenance_update_burning_ruins(); break;
        case 44: building_maintenance_check_fire_collapse(); break;
        case 45: figure_generate_criminals(); break;
        case 46: building_industry_update_wheat_production(); break;
        case 48: house_service_decay_tax_collector(); break;
        case 49: city_culture_calculate(); break;
    }
    if (game_time_advance_tick()) {
        advance_day();
    }
}

void game_tick_run(void)
{
    if (editor_is_active()) {
        random_generate_next(); // update random to randomize native huts
        for (int i = 1; i < MAX_FIGURES; i++) {
            struct figure_t *f = &figures[i];
            if (f->in_use && f->type == FIGURE_MAP_FLAG) {
                figure_editor_flag_action(f);
            }
        }
        return;
    }
    random_generate_next();
    game_undo_reduce_time_available();
    advance_tick();
    process_earthquake();
    city_victory_check();
    city_data.entertainment.hippodrome_has_race = 0;
    for (int i = 1; i < MAX_FIGURES; i++) {
        struct figure_t *f = &figures[i];
        if (f->is_corpse) {
            figure_handle_corpse(f);
            continue;
        } else if (f->engaged_in_combat) {
            figure_combat_handle_attack(f);
            continue;
        } else if (f->is_fleeing) {
            rout_unit(f);
            continue;
        }
        if (f->in_use) {
            switch (f->type) {
                case FIGURE_IMMIGRANT:
                    figure_immigrant_action(f);
                    break;
                case FIGURE_EMIGRANT:
                    figure_emigrant_action(f);
                    break;
                case FIGURE_HOMELESS:
                    figure_homeless_action(f);
                    break;
                case FIGURE_PATRICIAN:
                    figure_patrician_action(f);
                    break;
                case FIGURE_CART_PUSHER:
                    figure_cartpusher_action(f);
                    break;
                case FIGURE_LABOR_SEEKER:
                    figure_labor_seeker_action(f);
                    break;
                case FIGURE_BARBER:
                    figure_barber_action(f);
                    break;
                case FIGURE_BATHHOUSE_WORKER:
                    figure_bathhouse_worker_action(f);
                    break;
                case FIGURE_DOCTOR:
                case FIGURE_SURGEON:
                    figure_doctor_action(f);
                    break;
                case FIGURE_PRIEST:
                    figure_priest_action(f);
                    break;
                case FIGURE_SCHOOL_CHILD:
                    figure_school_child_action(f);
                    break;
                case FIGURE_TEACHER:
                    figure_teacher_action(f);
                    break;
                case FIGURE_LIBRARIAN:
                    figure_librarian_action(f);
                    break;
                case FIGURE_MISSIONARY:
                    figure_missionary_action(f);
                    break;
                case FIGURE_ACTOR:
                case FIGURE_GLADIATOR:
                case FIGURE_LION_TAMER:
                case FIGURE_CHARIOTEER:
                    figure_entertainer_action(f);
                    break;
                case FIGURE_HIPPODROME_HORSES:
                    figure_hippodrome_horse_action(f);
                    break;
                case FIGURE_TAX_COLLECTOR:
                    figure_tax_collector_action(f);
                    break;
                case FIGURE_ENGINEER:
                    figure_engineer_action(f);
                    break;
                case FIGURE_FISHING_BOAT:
                    figure_fishing_boat_action(f);
                    break;
                case FIGURE_FISH_GULLS:
                    figure_seagulls_action(f);
                    break;
                case FIGURE_SHIPWRECK:
                    figure_shipwreck_action(f);
                    break;
                case FIGURE_DOCKER:
                    figure_docker_action(f);
                    break;
                case FIGURE_FLOTSAM:
                    if (scenario_map_has_river_exit()) {
                        figure_flotsam_action(f);
                    }
                    break;
                case FIGURE_BALLISTA:
                    figure_ballista_action(f);
                    break;
                case FIGURE_BOLT:
                    figure_bolt_action(f);
                    break;
                case FIGURE_TOWER_SENTRY:
                    figure_tower_sentry_action(f);
                    break;
                case FIGURE_JAVELIN:
                    figure_javelin_action(f);
                    break;
                case FIGURE_PREFECT:
                    figure_prefect_action(f);
                    break;
                case FIGURE_FORT_STANDARD:
                    figure_military_standard_action(f);
                    break;
                case FIGURE_FORT_JAVELIN:
                case FIGURE_FORT_MOUNTED:
                case FIGURE_FORT_LEGIONARY:
                    figure_soldier_action(f);
                    break;
                case FIGURE_MARKET_BUYER:
                    figure_market_buyer_action(f);
                    break;
                case FIGURE_MARKET_TRADER:
                    figure_market_trader_action(f);
                    break;
                case FIGURE_DELIVERY_BOY:
                    figure_delivery_boy_action(f);
                    break;
                case FIGURE_WAREHOUSEMAN:
                    figure_warehouseman_action(f);
                    break;
                case FIGURE_PROTESTER:
                    figure_protestor_action(f);
                    break;
                case FIGURE_CRIMINAL:
                    figure_criminal_action(f);
                    break;
                case FIGURE_RIOTER:
                    figure_rioter_action(f);
                    break;
                case FIGURE_TRADE_CARAVAN:
                    figure_trade_caravan_action(f);
                    break;
                case FIGURE_TRADE_CARAVAN_DONKEY:
                    figure_trade_caravan_donkey_action(f);
                    break;
                case FIGURE_TRADE_SHIP:
                    figure_trade_ship_action(f);
                    break;
                case FIGURE_INDIGENOUS_NATIVE:
                    figure_indigenous_native_action(f);
                    break;
                case FIGURE_NATIVE_TRADER:
                    figure_native_trader_action(f);
                    break;
                case FIGURE_WOLF:
                    figure_wolf_action(f);
                    break;
                case FIGURE_SHEEP:
                    figure_sheep_action(f);
                    break;
                case FIGURE_ZEBRA:
                    figure_zebra_action(f);
                    break;
                case FIGURE_ENEMY_GLADIATOR:
                    figure_enemy_gladiator_action(f);
                    break;
                case FIGURE_ENEMY_BARBARIAN_SWORDSMAN:
                case FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN:
                case FIGURE_ENEMY_BRITON_SWORDSMAN:
                case FIGURE_ENEMY_CELT_SWORDSMAN:
                case FIGURE_ENEMY_PICT_SWORDSMAN:
                case FIGURE_ENEMY_EGYPTIAN_SWORDSMAN:
                case FIGURE_ENEMY_ETRUSCAN_SWORDSMAN:
                case FIGURE_ENEMY_SAMNITE_SWORDSMAN:
                case FIGURE_ENEMY_GAUL_SWORDSMAN:
                case FIGURE_ENEMY_HELVETIUS_SWORDSMAN:
                case FIGURE_ENEMY_HUN_SWORDSMAN:
                case FIGURE_ENEMY_GOTH_SWORDSMAN:
                case FIGURE_ENEMY_VISIGOTH_SWORDSMAN:
                case FIGURE_ENEMY_NUMIDIAN_SWORDSMAN:
                case FIGURE_ENEMY_GREEK_SWORDSMAN:
                case FIGURE_ENEMY_MACEDONIAN_SWORDSMAN:
                case FIGURE_ENEMY_PERGAMUM_SWORDSMAN:
                case FIGURE_ENEMY_IBERIAN_SWORDSMAN:
                case FIGURE_ENEMY_JUDEAN_SWORDSMAN:
                case FIGURE_ENEMY_SELEUCID_SWORDSMAN:
                    figure_enemy_swordsman_action(f);
                    break;
                case FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT:
                    figure_enemy_elephant_action(f);
                    break;
                case FIGURE_ENEMY_BRITON_CHARIOT:
                case FIGURE_ENEMY_CELT_CHARIOT:
                case FIGURE_ENEMY_PICT_CHARIOT:
                    figure_enemy_chariot_action(f);
                    break;
                case FIGURE_ENEMY_EGYPTIAN_CAMEL:
                case FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER:
                case FIGURE_ENEMY_SAMNITE_SPEAR_THROWER:
                case FIGURE_ENEMY_GREEK_SPEAR_THROWER:
                case FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER:
                case FIGURE_ENEMY_PERGAMUM_ARCHER:
                case FIGURE_ENEMY_IBERIAN_SPEAR_THROWER:
                case FIGURE_ENEMY_JUDEAN_SPEAR_THROWER:
                case FIGURE_ENEMY_SELEUCID_SPEAR_THROWER:
                    figure_enemy_heavy_ranged_action(f);
                    break;
                case FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER:
                    figure_enemy_light_ranged_spearman_action(f);
                    break;

                case FIGURE_ENEMY_GAUL_AXEMAN:
                case FIGURE_ENEMY_HELVETIUS_AXEMAN:
                    figure_enemy_axeman_action(f);
                    break;
                case FIGURE_ENEMY_HUN_MOUNTED_ARCHER:
                case FIGURE_ENEMY_GOTH_MOUNTED_ARCHER:
                case FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER:
                    figure_enemy_mounted_archer_action(f);
                    break;
                case FIGURE_ENEMY_CAESAR_JAVELIN:
                case FIGURE_ENEMY_CAESAR_MOUNTED:
                case FIGURE_ENEMY_CAESAR_LEGIONARY:
                    figure_enemy_caesar_legionary_action(f);
                    break;
                case FIGURE_ARROW:
                    figure_arrow_action(f);
                    break;
                case FIGURE_MAP_FLAG:
                    figure_editor_flag_action(f);
                    break;
                case FIGURE_EXPLOSION:
                    figure_explosion_cloud_action(f);
                    break;
                default:
                    break;
            }
        }
    }
}