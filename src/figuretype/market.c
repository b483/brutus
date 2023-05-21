#include "market.h"

#include "building/building.h"
#include "building/granary.h"
#include "building/warehouse.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/image.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "game/resource.h"

static int create_delivery_boy(int leader_id, struct figure_t *f)
{
    struct figure_t *boy = figure_create(FIGURE_DELIVERY_BOY, f->x, f->y, 0);
    boy->is_targetable = 1;
    boy->terrain_usage = TERRAIN_USAGE_ROADS;
    boy->leading_figure_id = leader_id;
    boy->collecting_item_id = f->collecting_item_id;
    boy->building_id = f->building_id;
    return boy->id;
}

static int take_food_from_granary(struct figure_t *f, int market_id, int granary_id)
{
    int resource;
    switch (f->collecting_item_id) {
        case INVENTORY_WHEAT: resource = RESOURCE_WHEAT; break;
        case INVENTORY_VEGETABLES: resource = RESOURCE_VEGETABLES; break;
        case INVENTORY_FRUIT: resource = RESOURCE_FRUIT; break;
        case INVENTORY_MEAT: resource = RESOURCE_MEAT; break;
        default: return 0;
    }
    struct building_t *granary = &all_buildings[granary_id];
    int max_units = (f->collecting_item_id == INVENTORY_WHEAT ? 800 : 600) - all_buildings[market_id].data.market.inventory[f->collecting_item_id];
    int granary_units = granary->data.granary.resource_stored[resource];
    int num_loads;
    if (granary_units >= 800) {
        num_loads = 8;
    } else if (granary_units >= 700) {
        num_loads = 7;
    } else if (granary_units >= 600) {
        num_loads = 6;
    } else if (granary_units >= 500) {
        num_loads = 5;
    } else if (granary_units >= 400) {
        num_loads = 4;
    } else if (granary_units >= 300) {
        num_loads = 3;
    } else if (granary_units >= 200) {
        num_loads = 2;
    } else if (granary_units >= 100) {
        num_loads = 1;
    } else {
        num_loads = 0;
    }
    if (num_loads > max_units / 100) {
        num_loads = max_units / 100;
    }
    if (num_loads <= 0) {
        return 0;
    }
    building_granary_remove_resource(granary, resource, 100 * num_loads);
    // create delivery boys
    int previous_boy = f->id;
    for (int i = 0; i < num_loads; i++) {
        previous_boy = create_delivery_boy(previous_boy, f);
    }
    return 1;
}

static int take_resource_from_warehouse(struct figure_t *f, int warehouse_id)
{
    int resource;
    switch (f->collecting_item_id) {
        case INVENTORY_POTTERY: resource = RESOURCE_POTTERY; break;
        case INVENTORY_FURNITURE: resource = RESOURCE_FURNITURE; break;
        case INVENTORY_OIL: resource = RESOURCE_OIL; break;
        case INVENTORY_WINE: resource = RESOURCE_WINE; break;
        default: return 0;
    }
    struct building_t *warehouse = &all_buildings[warehouse_id];
    int num_loads;
    int stored = building_warehouse_get_amount(warehouse, resource);
    if (stored < 2) {
        num_loads = stored;
    } else {
        num_loads = 2;
    }
    if (num_loads <= 0) {
        return 0;
    }
    building_warehouse_remove_resource(warehouse, resource, num_loads);

    // create delivery boys
    int boy1 = create_delivery_boy(f->id, f);
    if (num_loads > 1) {
        create_delivery_boy(boy1, f);
    }
    return 1;
}

void figure_market_buyer_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id2 != f->id) {
        f->state = FIGURE_STATE_DEAD;
    }
    figure_image_increase_offset(f, 12);
    switch (f->action_state) {
        case FIGURE_ACTION_MARKET_BUYER_GOING_TO_STORAGE:
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                if (f->collecting_item_id > 3) {
                    if (!take_resource_from_warehouse(f, f->destination_building_id)) {
                        f->state = FIGURE_STATE_DEAD;
                    }
                } else {
                    if (!take_food_from_granary(f, f->building_id, f->destination_building_id)) {
                        f->state = FIGURE_STATE_DEAD;
                    }
                }
                f->action_state = FIGURE_ACTION_MARKET_BUYER_RETURNING;
                f->destination_x = f->source_x;
                f->destination_y = f->source_y;
            } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                f->action_state = FIGURE_ACTION_MARKET_BUYER_RETURNING;
                f->destination_x = f->source_x;
                f->destination_y = f->source_y;
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_MARKET_BUYER_RETURNING:
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            }
            break;
    }
    f->image_id = image_group(GROUP_FIGURE_MARKET_LADY) + figure_image_direction(f) + 8 * f->image_offset;
}

void figure_delivery_boy_action(struct figure_t *f)
{
    f->is_ghost = 0;
    figure_image_increase_offset(f, 12);

    struct figure_t *leader = &figures[f->leading_figure_id];
    if (f->leading_figure_id <= 0) {
        f->state = FIGURE_STATE_DEAD;
    } else {
        if (leader->state == FIGURE_STATE_ALIVE) {
            if (leader->type == FIGURE_MARKET_BUYER || leader->type == FIGURE_DELIVERY_BOY) {
                figure_movement_follow_ticks(f, 1);
            } else {
                f->state = FIGURE_STATE_DEAD;
            }
        } else { // leader arrived at market, drop resource at market
            all_buildings[f->building_id].data.market.inventory[f->collecting_item_id] += 100;
            f->state = FIGURE_STATE_DEAD;
        }
    }
    if (leader->is_ghost) {
        f->is_ghost = 1;
    }
    int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);
    f->image_id = image_group(GROUP_FIGURE_DELIVERY_BOY) + dir + 8 * f->image_offset;
}
