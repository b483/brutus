#include "trade.h"

#include "building/count.h"
#include "city/constants.h"
#include "city/data.h"
#include "city/message.h"
#include "empire/object.h"
#include "figure/figure.h"
#include "scenario/scenario.h"
#include "map/water.h"

static int generate_trader(struct empire_object_t *city)
{
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
        return 0;
    }

    int index;
    if (max_traders == 1) {
        if (!city->trader_figure_ids[0]) {
            index = 0;
        } else {
            return 0;
        }
    } else if (max_traders == 2) {
        if (!city->trader_figure_ids[0]) {
            index = 0;
        } else if (!city->trader_figure_ids[1]) {
            index = 1;
        } else {
            return 0;
        }
    } else { // 3
        if (!city->trader_figure_ids[0]) {
            index = 0;
        } else if (!city->trader_figure_ids[1]) {
            index = 1;
        } else if (!city->trader_figure_ids[2]) {
            index = 2;
        } else {
            return 0;
        }
    }

    if (city->trader_entry_delay > 0) {
        city->trader_entry_delay--;
        return 0;
    }
    city->trader_entry_delay = city->is_sea_trade ? 30 : 4;

    if (city->is_sea_trade) {
        // generate ship
        if (city_data.building.working_docks && scenario_map_has_river_entry() && !city_data.trade.sea_trade_problem_duration) {
            struct figure_t *ship = figure_create(FIGURE_TRADE_SHIP, scenario.river_entry_point.x, scenario.river_entry_point.y, DIR_0_TOP);
            ship->empire_city_id = city->id;
            ship->action_state = FIGURE_ACTION_TRADE_SHIP_CREATED;
            ship->wait_ticks = 10;
            city->trader_figure_ids[index] = ship->id;
            return 1;
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

            return 1;
        }
    }
    return 0;
}

void city_trade_update(void)
{
    city_data.trade.num_sea_routes = 0;
    city_data.trade.num_land_routes = 0;
    // Wine types
    city_data.resource.wine_types_available = building_count_industry_total(RESOURCE_WINE) > 0 ? 1 : 0;
    if (city_data.resource.trade_status[RESOURCE_WINE] == TRADE_STATUS_IMPORT) {
        for (int i = 0; i < MAX_OBJECTS; i++) {
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

    for (int i = 1; i < MAX_OBJECTS; i++) {
        if (!empire_objects[i].in_use || !empire_objects[i].trade_route_open) {
            continue;
        }
        if (empire_objects[i].is_sea_trade) {
            if (!city_data.building.working_docks) {
                // delay of 384 = 1 year
                city_message_post_with_message_delay(MESSAGE_CAT_NO_WORKING_DOCK, 1, MESSAGE_NO_WORKING_DOCK, 384);
                continue;
            }
            if (!scenario_map_has_river_entry()) {
                continue;
            }
            city_data.trade.num_sea_routes++;
        } else {
            city_data.trade.num_land_routes++;
        }
        if (generate_trader(&empire_objects[i])) {
            break;
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
