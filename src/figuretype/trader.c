#include "trader.h"

#include "building/building.h"
#include "city/city_new.h"
#include "core/calc.h"
#include "core/image.h"
#include "empire/empire.h"
#include "figure/combat.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "figure/trader.h"
#include "map/map.h"
#include "scenario/scenario.h"

int figure_trade_caravan_can_buy(struct figure_t *trader, int warehouse_id, int city_id)
{
    struct building_t *warehouse = &all_buildings[warehouse_id];
    if (warehouse->type != BUILDING_WAREHOUSE) {
        return 0;
    }
    if (trader->trader_amount_bought >= 8) {
        return 0;
    }
    struct building_t *space = warehouse;
    for (int i = 0; i < 8; i++) {
        space = &all_buildings[space->next_part_building_id];
        if (space->id > 0 && space->loads_stored > 0 &&
            can_export_resource_to_trade_city(city_id, space->subtype.warehouse_resource_id)) {
            return 1;
        }
    }
    return 0;
}

int figure_trade_caravan_can_sell(struct figure_t *trader, int warehouse_id, int city_id)
{
    struct building_t *warehouse = &all_buildings[warehouse_id];
    if (warehouse->type != BUILDING_WAREHOUSE) {
        return 0;
    }
    if (trader->loads_sold_or_carrying >= 8) {
        return 0;
    }
    struct building_storage_t *storage = building_storage_get(warehouse->storage_id);
    if (storage->empty_all) {
        return 0;
    }
    int num_importable = 0;
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        if (storage->resource_state[r] != BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
            if (can_import_resource_from_trade_city(city_id, r)) {
                num_importable++;
            }
        }
    }
    if (num_importable <= 0) {
        return 0;
    }
    int can_import = 0;
    int resource = city_data.trade.caravan_import_resource;
    if (storage->resource_state[resource] != BUILDING_STORAGE_STATE_NOT_ACCEPTING &&
        can_import_resource_from_trade_city(city_id, resource)) {
        can_import = 1;
    } else {
        for (int i = RESOURCE_WHEAT; i < RESOURCE_TYPES_MAX; i++) {
            resource = city_trade_next_caravan_import_resource();
            if (storage->resource_state[resource] != BUILDING_STORAGE_STATE_NOT_ACCEPTING &&
                    can_import_resource_from_trade_city(city_id, resource)) {
                can_import = 1;
                break;
            }
        }
    }
    if (can_import) {
        // at least one resource can be imported and accepted by this warehouse
        // check if warehouse can store any importable goods
        struct building_t *space = warehouse;
        for (int s = 0; s < 8; s++) {
            space = &all_buildings[space->next_part_building_id];
            if (space->id > 0 && space->loads_stored < 4) {
                if (!space->loads_stored) {
                    // empty space
                    return 1;
                }
                if (can_import_resource_from_trade_city(city_id, space->subtype.warehouse_resource_id)) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

static int trader_get_buy_resource(int warehouse_id, int city_id)
{
    struct building_t *warehouse = &all_buildings[warehouse_id];
    if (warehouse->type != BUILDING_WAREHOUSE) {
        return RESOURCE_NONE;
    }
    struct building_t *space = warehouse;
    for (int i = 0; i < 8; i++) {
        space = &all_buildings[space->next_part_building_id];
        if (space->id <= 0) {
            continue;
        }
        int resource = space->subtype.warehouse_resource_id;
        if (space->loads_stored > 0 && can_export_resource_to_trade_city(city_id, resource)) {
            // update stocks
            city_resource_remove_from_warehouse(resource, 1);
            space->loads_stored--;
            if (space->loads_stored <= 0) {
                space->subtype.warehouse_resource_id = RESOURCE_NONE;
            }
            // update finances
            city_finance_process_export(trade_prices[resource].sell);

            // update graphics
            building_warehouse_space_set_image(space, resource);
            return resource;
        }
    }
    return 0;
}

static int trader_get_sell_resource(int warehouse_id, int city_id)
{
    struct building_t *warehouse = &all_buildings[warehouse_id];
    if (warehouse->type != BUILDING_WAREHOUSE) {
        return 0;
    }
    int resource_to_import = city_data.trade.caravan_import_resource;
    int imp = RESOURCE_WHEAT;
    while (imp < RESOURCE_TYPES_MAX && !can_import_resource_from_trade_city(city_id, resource_to_import)) {
        imp++;
        resource_to_import = city_trade_next_caravan_import_resource();
    }
    if (imp >= RESOURCE_TYPES_MAX) {
        return 0;
    }
    // add to existing bay with room
    struct building_t *space = warehouse;
    for (int i = 0; i < 8; i++) {
        space = &all_buildings[space->next_part_building_id];
        if (space->id > 0 && space->loads_stored > 0 && space->loads_stored < 4 &&
            space->subtype.warehouse_resource_id == resource_to_import) {
            building_warehouse_space_add_import(space, resource_to_import);
            city_trade_next_caravan_import_resource();
            return resource_to_import;
        }
    }
    // add to empty bay
    space = warehouse;
    for (int i = 0; i < 8; i++) {
        space = &all_buildings[space->next_part_building_id];
        if (space->id > 0 && !space->loads_stored) {
            building_warehouse_space_add_import(space, resource_to_import);
            city_trade_next_caravan_import_resource();
            return resource_to_import;
        }
    }
    // find another importable resource that can be added to this warehouse
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        resource_to_import = city_trade_next_caravan_backup_import_resource();
        if (can_import_resource_from_trade_city(city_id, resource_to_import)) {
            space = warehouse;
            for (int i = 0; i < 8; i++) {
                space = &all_buildings[space->next_part_building_id];
                if (space->id > 0 && space->loads_stored < 4
                    && space->subtype.warehouse_resource_id == resource_to_import) {
                    building_warehouse_space_add_import(space, resource_to_import);
                    return resource_to_import;
                }
            }
        }
    }
    return 0;
}

static int get_closest_warehouse(const struct figure_t *f, int x, int y, int city_id, struct map_point_t *warehouse)
{
    int exportable[RESOURCE_TYPES_MAX];
    int importable[RESOURCE_TYPES_MAX];
    exportable[RESOURCE_NONE] = 0;
    importable[RESOURCE_NONE] = 0;
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        exportable[r] = can_export_resource_to_trade_city(city_id, r);
        if (f->trader_amount_bought >= 8) {
            exportable[r] = 0;
        }
        if (city_id) {
            importable[r] = can_import_resource_from_trade_city(city_id, r);
        } else { // Don't import goods from native traders
            importable[r] = 0;
        }
        if (f->loads_sold_or_carrying >= 8) {
            importable[r] = 0;
        }
    }
    int num_importable = 0;
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        if (importable[r]) {
            num_importable++;
        }
    }
    int min_distance = 10000;
    struct building_t *min_building = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_WAREHOUSE) {
            continue;
        }
        if (!b->has_road_access) {
            continue;
        }
        struct building_storage_t *s = building_storage_get(b->storage_id);
        int num_imports_for_warehouse = 0;
        for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
            if (s->resource_state[r] != BUILDING_STORAGE_STATE_NOT_ACCEPTING
                && can_import_resource_from_trade_city(city_id, r)) {
                num_imports_for_warehouse++;
            }
        }
        int distance_penalty = 32;
        struct building_t *space = b;
        for (int space_cnt = 0; space_cnt < 8; space_cnt++) {
            space = &all_buildings[space->next_part_building_id];
            if (space->id && exportable[space->subtype.warehouse_resource_id]) {
                distance_penalty -= 4;
            }
            if (num_importable && num_imports_for_warehouse && !s->empty_all) {
                for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                    int import_resource = city_trade_next_caravan_import_resource();
                    if (s->resource_state[import_resource] != BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
                        break;
                    }
                }
                int resource = city_data.trade.caravan_import_resource;
                if (s->resource_state[resource] != BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
                    if (space->subtype.warehouse_resource_id == RESOURCE_NONE) {
                        distance_penalty -= 16;
                    }
                    if (space->id && importable[space->subtype.warehouse_resource_id] && space->loads_stored < 4 &&
                        space->subtype.warehouse_resource_id == resource) {
                        distance_penalty -= 8;
                    }
                }
            }
        }
        if (distance_penalty < 32) {
            int distance = calc_maximum_distance(b->x, b->y, x, y);
            distance += distance_penalty;
            if (distance < min_distance) {
                min_distance = distance;
                min_building = b;
            }
        }
    }
    if (!min_building) {
        return 0;
    }
    if (min_building->has_road_access == 1) {
        warehouse->x = min_building->x;
        warehouse->y = min_building->y;
    } else if (!map_has_road_access(min_building->x, min_building->y, 3, warehouse)) {
        return 0;
    }
    return min_building->id;
}

static void go_to_next_warehouse(struct figure_t *f, int x_src, int y_src)
{
    struct map_point_t dst;
    int warehouse_id = get_closest_warehouse(f, x_src, y_src, f->empire_city_id, &dst);
    if (warehouse_id) {
        f->destination_building_id = warehouse_id;
        f->action_state = FIGURE_ACTION_TRADE_CARAVAN_ARRIVING;
        f->destination_x = dst.x;
        f->destination_y = dst.y;
    } else {
        f->action_state = FIGURE_ACTION_TRADE_CARAVAN_LEAVING;
        f->destination_x = scenario.exit_point.x;
        f->destination_y = scenario.exit_point.y;
    }
}

void figure_trade_caravan_action(struct figure_t *f)
{
    f->is_invisible = 0;
    figure_image_increase_offset(f, 12);
    switch (f->action_state) {
        case FIGURE_ACTION_TRADE_CARAVAN_CREATED:
            f->is_invisible = 1;
            f->wait_ticks++;
            if (f->wait_ticks > 20) {
                f->wait_ticks = 0;
                int x_base, y_base;
                if (city_data.building.trade_center_building_id) {
                    struct building_t *trade_center = &all_buildings[city_data.building.trade_center_building_id];
                    x_base = trade_center->x;
                    y_base = trade_center->y;
                } else {
                    x_base = f->x;
                    y_base = f->y;
                }
                go_to_next_warehouse(f, x_base, y_base);
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_TRADE_CARAVAN_ARRIVING:
            figure_movement_move_ticks(f, 1);
            switch (f->direction) {
                case DIR_FIGURE_AT_DESTINATION:
                    f->action_state = FIGURE_ACTION_TRADE_CARAVAN_TRADING;
                    break;
                case DIR_FIGURE_REROUTE:
                    figure_route_remove(f);
                    break;
                case DIR_FIGURE_LOST:
                    figure_delete(f);
                    return;
            }
            if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_TRADE_CARAVAN_TRADING:
            f->wait_ticks++;
            if (f->wait_ticks > 10) {
                f->wait_ticks = 0;
                int move_on = 0;
                if (figure_trade_caravan_can_buy(f, f->destination_building_id, f->empire_city_id)) {
                    int resource = trader_get_buy_resource(f->destination_building_id, f->empire_city_id);
                    if (resource) {
                        empire_objects[f->empire_city_id].resource_bought[resource]++;
                        trader_record_bought_resource(f->trader_id, resource);
                        f->trader_amount_bought++;
                    } else {
                        move_on++;
                    }
                } else {
                    move_on++;
                }
                if (figure_trade_caravan_can_sell(f, f->destination_building_id, f->empire_city_id)) {
                    int resource = trader_get_sell_resource(f->destination_building_id, f->empire_city_id);
                    if (resource) {
                        empire_objects[f->empire_city_id].resource_sold[resource]++;
                        trader_record_sold_resource(f->trader_id, resource);
                        f->loads_sold_or_carrying++;
                    } else {
                        move_on++;
                    }
                } else {
                    move_on++;
                }
                if (move_on == 2) {
                    go_to_next_warehouse(f, f->x, f->y);
                }
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_TRADE_CARAVAN_LEAVING:
            figure_movement_move_ticks(f, 1);
            switch (f->direction) {
                case DIR_FIGURE_AT_DESTINATION:
                    f->action_state = FIGURE_ACTION_TRADE_CARAVAN_CREATED;
                    figure_delete(f);
                    return;
                case DIR_FIGURE_REROUTE:
                    figure_route_remove(f);
                    break;
                case DIR_FIGURE_LOST:
                    figure_delete(f);
                    return;
            }
            break;
    }
    int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
    f->image_id = image_group(GROUP_FIGURE_TRADE_CARAVAN) + dir + 8 * f->image_offset;
}

void figure_trade_caravan_donkey_action(struct figure_t *f)
{
    f->is_invisible = 0;
    figure_image_increase_offset(f, 12);

    struct figure_t *leader = &figures[f->leading_figure_id];
    if (f->leading_figure_id <= 0) {
        figure_delete(f);
        return;
    } else {
        if (leader->type != FIGURE_TRADE_CARAVAN && leader->type != FIGURE_TRADE_CARAVAN_DONKEY) {
            figure_delete(f);
            return;
        } else {
            figure_movement_follow_ticks(f, 1);
        }
    }

    if (leader->is_invisible) {
        f->is_invisible = 1;
    }
    int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
    f->image_id = image_group(GROUP_FIGURE_TRADE_CARAVAN) + dir + 8 * f->image_offset;
}

void figure_native_trader_action(struct figure_t *f)
{
    f->is_invisible = 0;
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    switch (f->action_state) {
        case FIGURE_ACTION_NATIVE_TRADER_GOING_TO_WAREHOUSE:
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_NATIVE_TRADER_AT_WAREHOUSE;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_NATIVE_TRADER_RETURNING:
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_NATIVE_TRADER_CREATED:
            f->is_invisible = 1;
            f->wait_ticks++;
            if (f->wait_ticks > 10) {
                f->wait_ticks = 0;
                struct map_point_t tile;
                int building_id = get_closest_warehouse(f, f->x, f->y, 0, &tile);
                if (building_id) {
                    f->action_state = FIGURE_ACTION_NATIVE_TRADER_GOING_TO_WAREHOUSE;
                    f->destination_building_id = building_id;
                    f->destination_x = tile.x;
                    f->destination_y = tile.y;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_NATIVE_TRADER_AT_WAREHOUSE:
            f->wait_ticks++;
            if (f->wait_ticks > 10) {
                f->wait_ticks = 0;
                if (figure_trade_caravan_can_buy(f, f->destination_building_id, 0)) {
                    int resource = trader_get_buy_resource(f->destination_building_id, 0);
                    trader_record_bought_resource(f->trader_id, resource);
                    f->trader_amount_bought += 3;
                } else {
                    struct map_point_t tile;
                    int building_id = get_closest_warehouse(f, f->x, f->y, 0, &tile);
                    if (building_id) {
                        f->action_state = FIGURE_ACTION_NATIVE_TRADER_GOING_TO_WAREHOUSE;
                        f->destination_building_id = building_id;
                        f->destination_x = tile.x;
                        f->destination_y = tile.y;
                    } else {
                        f->action_state = FIGURE_ACTION_NATIVE_TRADER_RETURNING;
                        f->destination_x = f->source_x;
                        f->destination_y = f->source_y;
                    }
                }
            }
            f->image_offset = 0;
            break;
    }
    int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);

    f->image_id = image_group(GROUP_FIGURE_CARTPUSHER) + dir + 8 * f->image_offset;
    f->cart_image_id = image_group(GROUP_FIGURE_MIGRANT_CART) + 8 + 8 * f->resource_id;
    if (f->cart_image_id) {
        f->cart_image_id += dir;
        figure_image_set_cart_offset(f, dir);
    }
}

int figure_trade_ship_is_trading(struct figure_t *ship)
{
    struct building_t *b = &all_buildings[ship->destination_building_id];
    if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_DOCK) {
        return TRADE_SHIP_BUYING;
    }
    for (int i = 0; i < 3; i++) {
        struct figure_t *f = &figures[b->data.dock.docker_ids[i]];
        if (!b->data.dock.docker_ids[i] || !figure_is_alive(f)) {
            continue;
        }
        switch (f->action_state) {
            case FIGURE_ACTION_DOCKER_IMPORT_QUEUE:
            case FIGURE_ACTION_DOCKER_IMPORT_GOING_TO_WAREHOUSE:
            case FIGURE_ACTION_DOCKER_IMPORT_RETURNING:
            case FIGURE_ACTION_DOCKER_IMPORT_AT_WAREHOUSE:
                return TRADE_SHIP_BUYING;
            case FIGURE_ACTION_DOCKER_EXPORT_QUEUE:
            case FIGURE_ACTION_DOCKER_EXPORT_GOING_TO_WAREHOUSE:
            case FIGURE_ACTION_DOCKER_EXPORT_RETURNING:
            case FIGURE_ACTION_DOCKER_EXPORT_AT_WAREHOUSE:
                return TRADE_SHIP_SELLING;
        }
    }
    return TRADE_SHIP_NONE;
}

static int trade_ship_lost_queue(const struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->destination_building_id];
    if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_DOCK &&
        b->num_workers > 0 && b->data.dock.trade_ship_id == f->id) {
        return 0;
    }
    return 1;
}

static int trade_ship_done_trading(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->destination_building_id];
    if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_DOCK && b->num_workers > 0) {
        for (int i = 0; i < 3; i++) {
            if (b->data.dock.docker_ids[i]) {
                struct figure_t *docker = &figures[b->data.dock.docker_ids[i]];
                if (figure_is_alive(docker) && docker->action_state != FIGURE_ACTION_DOCKER_IDLING) {
                    return 0;
                }
            }
        }
        f->trade_ship_failed_dock_attempts++;
        if (f->trade_ship_failed_dock_attempts >= 10) {
            f->trade_ship_failed_dock_attempts = 11;
            return 1;
        }
        return 0;
    }
    return 1;
}

static int building_dock_get_queue_destination(struct map_point_t *tile)
{
    if (!city_data.building.working_docks) {
        return 0;
    }
    // first queue position
    for (int i = 0; i < 10; i++) {
        int dock_id = city_data.building.working_dock_ids[i];
        if (!dock_id) continue;
        struct building_t *dock = &all_buildings[dock_id];
        int dx, dy;
        switch (dock->data.dock.orientation) {
            case 0: dx = 2; dy = -2; break;
            case 1: dx = 4; dy = 2; break;
            case 2: dx = 2; dy = 4; break;
            default: dx = -2; dy = 2; break;
        }
        tile->x = dock->x + dx;
        tile->y = dock->y + dy;
        if (!map_has_figure_at(map_grid_offset(tile->x, tile->y))) {
            return dock_id;
        }
    }
    // second queue position
    for (int i = 0; i < 10; i++) {
        int dock_id = city_data.building.working_dock_ids[i];
        if (!dock_id) continue;
        struct building_t *dock = &all_buildings[dock_id];
        int dx, dy;
        switch (dock->data.dock.orientation) {
            case 0: dx = 2; dy = -3; break;
            case 1: dx = 5; dy = 2; break;
            case 2: dx = 2; dy = 5; break;
            default: dx = -3; dy = 2; break;
        }
        tile->x = dock->x + dx;
        tile->y = dock->y + dy;
        if (!map_has_figure_at(map_grid_offset(tile->x, tile->y))) {
            return dock_id;
        }
    }
    return 0;
}

static int building_dock_get_free_destination(int ship_id, struct map_point_t *tile)
{
    if (!city_data.building.working_docks) {
        return 0;
    }
    int dock_id = 0;
    for (int i = 0; i < 10; i++) {
        dock_id = city_data.building.working_dock_ids[i];
        if (!dock_id) continue;
        struct building_t *dock = &all_buildings[dock_id];
        if (!dock->data.dock.trade_ship_id || dock->data.dock.trade_ship_id == ship_id) {
            break;
        }
    }
    // BUG: when 10 docks in city, always takes last one... regardless of whether it is free
    if (dock_id <= 0) {
        return 0;
    }
    struct building_t *dock = &all_buildings[dock_id];
    int dx, dy;
    switch (dock->data.dock.orientation) {
        case 0: dx = 1; dy = -1; break;
        case 1: dx = 3; dy = 1; break;
        case 2: dx = 1; dy = 3; break;
        default: dx = -1; dy = 1; break;
    }
    tile->x = dock->x + dx;
    tile->y = dock->y + dy;
    dock->data.dock.trade_ship_id = ship_id;
    return dock_id;
}

void figure_trade_ship_action(struct figure_t *f)
{
    f->is_invisible = 0;
    figure_image_increase_offset(f, 12);
    switch (f->action_state) {
        case FIGURE_ACTION_TRADE_SHIP_CREATED:
            f->loads_sold_or_carrying = 12;
            f->trader_amount_bought = 0;
            f->is_invisible = 1;
            f->wait_ticks++;
            if (f->wait_ticks > 20) {
                f->wait_ticks = 0;
                struct map_point_t tile;
                int dock_id = building_dock_get_free_destination(f->id, &tile);
                if (dock_id) {
                    f->destination_building_id = dock_id;
                    f->action_state = FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK;
                    f->destination_x = tile.x;
                    f->destination_y = tile.y;
                } else if (building_dock_get_queue_destination(&tile)) {
                    f->action_state = FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK_QUEUE;
                    f->destination_x = tile.x;
                    f->destination_y = tile.y;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK:
            figure_movement_move_ticks(f, 1);
            f->height_adjusted_ticks = 0;
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_TRADE_SHIP_MOORED;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                if (!city_message_get_category_count(MESSAGE_CAT_BLOCKED_DOCK)) {
                    city_message_post(1, MESSAGE_NAVIGATION_IMPOSSIBLE, 0, 0);
                    city_message_increase_category_count(MESSAGE_CAT_BLOCKED_DOCK);
                }
                return;
            }
            if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                f->action_state = FIGURE_ACTION_TRADE_SHIP_LEAVING;
                f->wait_ticks = 0;
                f->destination_x = scenario.river_exit_point.x;
                f->destination_y = scenario.river_exit_point.y;
            }
            break;
        case FIGURE_ACTION_TRADE_SHIP_MOORED:
            if (trade_ship_lost_queue(f)) {
                f->trade_ship_failed_dock_attempts = 0;
                f->action_state = FIGURE_ACTION_TRADE_SHIP_LEAVING;
                f->wait_ticks = 0;
                f->destination_x = scenario.river_entry_point.x;
                f->destination_y = scenario.river_entry_point.y;
            } else if (trade_ship_done_trading(f)) {
                f->trade_ship_failed_dock_attempts = 0;
                f->action_state = FIGURE_ACTION_TRADE_SHIP_LEAVING;
                f->wait_ticks = 0;
                f->destination_x = scenario.river_entry_point.x;
                f->destination_y = scenario.river_entry_point.y;
                struct building_t *dst = &all_buildings[f->destination_building_id];
                dst->data.dock.queued_docker_id = 0;
                dst->data.dock.num_ships = 0;
            }
            switch (all_buildings[f->destination_building_id].data.dock.orientation) {
                case 0: f->direction = DIR_2_RIGHT; break;
                case 1: f->direction = DIR_4_BOTTOM; break;
                case 2: f->direction = DIR_6_LEFT; break;
                default:f->direction = DIR_0_TOP; break;
            }
            f->image_offset = 0;
            city_message_reset_category_count(MESSAGE_CAT_BLOCKED_DOCK);
            break;
        case FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK_QUEUE:
            figure_movement_move_ticks(f, 1);
            f->height_adjusted_ticks = 0;
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_TRADE_SHIP_ANCHORED;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_TRADE_SHIP_ANCHORED:
            f->wait_ticks++;
            if (f->wait_ticks > 40) {
                struct map_point_t tile;
                int dock_id = building_dock_get_free_destination(f->id, &tile);
                if (dock_id) {
                    f->destination_building_id = dock_id;
                    f->action_state = FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK;
                    f->destination_x = tile.x;
                    f->destination_y = tile.y;
                } else if (map_figure_at(f->grid_offset) != f->id &&
                    building_dock_get_queue_destination(&tile)) {
                    f->action_state = FIGURE_ACTION_TRADE_SHIP_GOING_TO_DOCK_QUEUE;
                    f->destination_x = tile.x;
                    f->destination_y = tile.y;
                }
                f->wait_ticks = 0;
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_TRADE_SHIP_LEAVING:
            figure_movement_move_ticks(f, 1);
            f->height_adjusted_ticks = 0;
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_TRADE_SHIP_CREATED;
                figure_delete(f);
                return;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
    }
    int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
    f->image_id = image_group(GROUP_FIGURE_SHIP) + dir;
}
