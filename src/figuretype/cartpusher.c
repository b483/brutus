#include "cartpusher.h"

#include "building/building.h"
#include "city/data.h"
#include "city/resource.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "city/resource.h"
#include "map/map.h"
#include "scenario/scenario.h"

static const int CART_OFFSET_MULTIPLE_LOADS_FOOD[] = { 0, 0, 8, 16, 0, 0, 24, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
static const int CART_OFFSET_MULTIPLE_LOADS_NON_FOOD[] = { 0, 0, 0, 0, 0, 8, 0, 16, 24, 32, 40, 48, 56, 64, 72, 80 };
static const int CART_OFFSET_8_LOADS_FOOD[] = { 0, 40, 48, 56, 0, 0, 64, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static void set_cart_graphic(struct figure_t *f)
{
    f->cart_image_id = EMPTY_CART_IMG_ID + resource_images[f->resource_id].cart_img_id + resource_image_offset(f->resource_id, RESOURCE_IMAGE_CART);
}

static void set_destination(struct figure_t *f, int action, int building_id, int x_dst, int y_dst)
{
    f->destination_building_id = building_id;
    f->action_state = action;
    f->wait_ticks = 0;
    f->destination_x = x_dst;
    f->destination_y = y_dst;
}

static int building_get_workshop_for_raw_material_with_room(int x, int y, int resource, int road_network_id, struct map_point_t *dst)
{
    if (city_data.resource.stockpiled[resource]) {
        return 0;
    }
    int output_type = resource_to_workshop_type(resource);
    if (output_type == WORKSHOP_NONE) {
        return 0;
    }
    int min_dist = INFINITE;
    struct building_t *min_building = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || !building_is_workshop(b->type)) {
            continue;
        }
        if (!b->has_road_access) {
            continue;
        }
        if (b->subtype.workshop_type == output_type && b->road_network_id == road_network_id && b->loads_stored < 2) {
            int dist = calc_maximum_distance(b->x, b->y, x, y);
            if (b->loads_stored > 0) {
                dist += 20;
            }
            if (dist < min_dist) {
                min_dist = dist;
                min_building = b;
            }
        }
    }
    if (min_building) {
        dst->x = min_building->road_access_x;
        dst->y = min_building->road_access_y;
        return min_building->id;
    }
    return 0;
}

static int building_granary_for_storing(int x, int y, int resource, int road_network_id,
                                 int force_on_stockpile, int *understaffed, struct map_point_t *dst)
{
    if (scenario.rome_supplies_wheat) {
        return 0;
    }
    if (!resource_is_food(resource)) {
        return 0;
    }
    if (city_data.resource.stockpiled[resource] && !force_on_stockpile) {
        return 0;
    }
    int min_dist = INFINITE;
    int min_building_id = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_GRANARY) {
            continue;
        }
        if (!b->has_road_access || b->road_network_id != road_network_id) {
            continue;
        }
        if (calc_percentage(b->num_workers, building_properties[b->type].n_laborers) < 100) {
            if (understaffed) {
                *understaffed += 1;
            }
            continue;
        }
        struct building_storage_t *s = building_storage_get(b->storage_id);
        if (s->resource_state[resource] == BUILDING_STORAGE_STATE_NOT_ACCEPTING || s->empty_all) {
            continue;
        }
        if (b->data.granary.resource_stored[RESOURCE_NONE] >= ONE_LOAD) {
            // there is room
            int dist = calc_maximum_distance(b->x + 1, b->y + 1, x, y);
            if (dist < min_dist) {
                min_dist = dist;
                min_building_id = i;
            }
        }
    }
    // deliver to center of granary
    struct building_t *min = &all_buildings[min_building_id];
    dst->x = min->x + 1;
    dst->y = min->y + 1;
    return min_building_id;
}

static void determine_cartpusher_destination(struct figure_t *f, struct building_t *b, int road_network_id)
{
    struct map_point_t dst;
    int understaffed_storages = 0;

    // priority 1: warehouse if resource is on stockpile
    int dst_building_id = building_warehouse_for_storing(0, f->x, f->y,
        b->output_resource_id, road_network_id,
        &understaffed_storages, &dst);
    if (!city_data.resource.stockpiled[b->output_resource_id]) {
        dst_building_id = 0;
    }
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WAREHOUSE, dst_building_id, dst.x, dst.y);
        return;
    }
    // priority 2: accepting granary for food
    dst_building_id = building_granary_for_storing(f->x, f->y,
        b->output_resource_id, road_network_id, 0,
        &understaffed_storages, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY, dst_building_id, dst.x, dst.y);
        return;
    }
    // priority 3: workshop for raw material
    dst_building_id = building_get_workshop_for_raw_material_with_room(f->x, f->y, b->output_resource_id, road_network_id, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WORKSHOP, dst_building_id, dst.x, dst.y);
        return;
    }
    // priority 4: warehouse
    dst_building_id = building_warehouse_for_storing(0, f->x, f->y,
        b->output_resource_id, road_network_id,
        &understaffed_storages, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WAREHOUSE, dst_building_id, dst.x, dst.y);
        return;
    }
    // priority 5: granary forced when on stockpile
    dst_building_id = building_granary_for_storing(f->x, f->y,
        b->output_resource_id, road_network_id, 1,
        &understaffed_storages, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY, dst_building_id, dst.x, dst.y);
        return;
    }
    // no one will accept
    f->wait_ticks = 0;
    // set cartpusher text
    f->min_max_seen = understaffed_storages ? 2 : 1;
}

static void determine_cartpusher_destination_food(struct figure_t *f, int road_network_id)
{
    struct building_t *b = &all_buildings[f->building_id];
    struct map_point_t dst;
    // priority 1: accepting granary for food
    int dst_building_id = building_granary_for_storing(f->x, f->y,
        b->output_resource_id, road_network_id, 0,
        0, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY, dst_building_id, dst.x, dst.y);
        return;
    }
    // priority 2: warehouse
    dst_building_id = building_warehouse_for_storing(0, f->x, f->y,
        b->output_resource_id, road_network_id,
        0, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WAREHOUSE, dst_building_id, dst.x, dst.y);
        return;
    }
    // priority 3: granary
    dst_building_id = building_granary_for_storing(f->x, f->y,
        b->output_resource_id, road_network_id, 1,
        0, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY, dst_building_id, dst.x, dst.y);
        return;
    }
    // no one will accept, stand idle
    f->wait_ticks = 0;
}

static void update_image(struct figure_t *f)
{
    int dir = figure_image_normalize_direction(f->direction < 8 ? f->direction : f->previous_tile_direction);

    f->image_id = image_group(GROUP_FIGURE_CARTPUSHER) + dir + 8 * f->image_offset;
    if (f->cart_image_id) {
        f->cart_image_id += dir;
        figure_image_set_cart_offset(f, dir);
        if (f->loads_sold_or_carrying >= 8) {
            f->y_offset_cart -= 40;
        }
    }
}

static void reroute_cartpusher(struct figure_t *f)
{
    figure_route_remove(f);
    if (terrain_land_citizen.items[f->grid_offset] != CITIZEN_2_PASSABLE_TERRAIN) {
        f->action_state = FIGURE_ACTION_CARTPUSHER_INITIAL;
    }
    f->wait_ticks = 0;
}

static void building_workshop_add_raw_material(struct building_t *b)
{
    if (b->id > 0 && building_is_workshop(b->type)) {
        b->loads_stored++; // BUG: any raw material accepted
    }
}

void figure_cartpusher_action(struct figure_t *f)
{
    f->cart_image_id = 0;
    int road_network_id = map_road_network_get(f->grid_offset);
    struct building_t *b = &all_buildings[f->building_id];

    switch (f->action_state) {
        case FIGURE_ACTION_CARTPUSHER_INITIAL:
            set_cart_graphic(f);
            if (!map_routing_citizen_is_passable(f->grid_offset)) {
                figure_delete(f);
                return;
            }
            if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
                figure_delete(f);
                return;
            }
            f->wait_ticks++;
            if (f->wait_ticks > 30) {
                determine_cartpusher_destination(f, b, road_network_id);
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WAREHOUSE:
            set_cart_graphic(f);
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_CARTPUSHER_AT_WAREHOUSE;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                reroute_cartpusher(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY:
            set_cart_graphic(f);
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_CARTPUSHER_AT_GRANARY;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                reroute_cartpusher(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->action_state = FIGURE_ACTION_CARTPUSHER_INITIAL;
                f->wait_ticks = 0;
            }
            if (all_buildings[f->destination_building_id].state != BUILDING_STATE_IN_USE) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WORKSHOP:
            set_cart_graphic(f);
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_CARTPUSHER_AT_WORKSHOP;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                reroute_cartpusher(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_CARTPUSHER_AT_WAREHOUSE:
            f->wait_ticks++;
            if (f->wait_ticks > 10) {
                if (building_warehouse_add_resource(&all_buildings[f->destination_building_id], f->resource_id)) {
                    f->action_state = FIGURE_ACTION_CARTPUSHER_RETURNING;
                    f->wait_ticks = 0;
                    f->destination_x = f->source_x;
                    f->destination_y = f->source_y;
                } else {
                    figure_route_remove(f);
                    f->action_state = FIGURE_ACTION_CARTPUSHER_INITIAL;
                    f->wait_ticks = 0;
                }
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_CARTPUSHER_AT_GRANARY:
            f->wait_ticks++;
            if (f->wait_ticks > 5) {
                if (building_granary_add_resource(&all_buildings[f->destination_building_id], f->resource_id, 1)) {
                    f->action_state = FIGURE_ACTION_CARTPUSHER_RETURNING;
                    f->wait_ticks = 0;
                    f->destination_x = f->source_x;
                    f->destination_y = f->source_y;
                } else {
                    determine_cartpusher_destination_food(f, road_network_id);
                }
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_CARTPUSHER_AT_WORKSHOP:
            f->wait_ticks++;
            if (f->wait_ticks > 5) {
                building_workshop_add_raw_material(&all_buildings[f->destination_building_id]);
                f->action_state = FIGURE_ACTION_CARTPUSHER_RETURNING;
                f->wait_ticks = 0;
                f->destination_x = f->source_x;
                f->destination_y = f->source_y;
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_CARTPUSHER_RETURNING:
            f->cart_image_id = EMPTY_CART_IMG_ID;
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
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
    figure_image_increase_offset(f, 12);
    update_image(f);
}

static void determine_granaryman_destination(struct figure_t *f, int road_network_id)
{
    struct map_point_t dst;
    int dst_building_id;
    struct building_t *granary = &all_buildings[f->building_id];
    if (!f->resource_id) {
        // getting granaryman
        dst_building_id = building_granary_for_getting(granary, &dst);
        if (dst_building_id) {
            f->loads_sold_or_carrying = 0;
            set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_GETTING_FOOD, dst_building_id, dst.x, dst.y);
        } else {
            figure_delete(f);
        }
        return;
    }
    // delivering resource
    // priority 1: another granary
    dst_building_id = building_granary_for_storing(f->x, f->y, f->resource_id, road_network_id, 0,
        0, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
        building_granary_remove_resource(granary, f->resource_id, 100);
        return;
    }
    // priority 2: warehouse
    dst_building_id = building_warehouse_for_storing(0, f->x, f->y,
        f->resource_id, road_network_id, 0, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
        building_granary_remove_resource(granary, f->resource_id, 100);
        return;
    }
    // priority 3: granary even though resource is on stockpile
    dst_building_id = building_granary_for_storing(f->x, f->y,
        f->resource_id, road_network_id, 1, 0, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
        building_granary_remove_resource(granary, f->resource_id, 100);
        return;
    }
    // nowhere to go to: kill figure
    figure_delete(f);
}

static void remove_resource_from_warehouse(struct figure_t *f)
{
    if (figure_is_alive(f)) {
        int err = building_warehouse_remove_resource(&all_buildings[f->building_id], f->resource_id, 1);
        if (err) {
            figure_delete(f);
        }
    }
}

static int building_get_workshop_for_raw_material(int x, int y, int resource, int road_network_id, struct map_point_t *dst)
{
    if (city_data.resource.stockpiled[resource]) {
        return 0;
    }
    int output_type = resource_to_workshop_type(resource);
    if (output_type == WORKSHOP_NONE) {
        return 0;
    }
    int min_dist = INFINITE;
    struct building_t *min_building = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || !building_is_workshop(b->type)) {
            continue;
        }
        if (!b->has_road_access) {
            continue;
        }
        if (b->subtype.workshop_type == output_type && b->road_network_id == road_network_id) {
            int dist = 10 * b->loads_stored +
                calc_maximum_distance(b->x, b->y, x, y);
            if (dist < min_dist) {
                min_dist = dist;
                min_building = b;
            }
        }
    }
    if (min_building) {
        dst->x = min_building->road_access_x;
        dst->y = min_building->road_access_y;
        return min_building->id;
    }
    return 0;
}

static void determine_warehouseman_destination(struct figure_t *f, int road_network_id)
{
    struct map_point_t dst;
    int dst_building_id = 0;
    if (!f->resource_id) {
        // getting warehouseman
        dst_building_id = building_warehouse_for_getting(
            &all_buildings[f->building_id], f->collecting_item_id, &dst);
        if (dst_building_id) {
            f->loads_sold_or_carrying = 0;
            set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_GETTING_RESOURCE, dst_building_id, dst.x, dst.y);
            f->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
        } else {
            figure_delete(f);
        }
        return;
    }
    // delivering resource
    // priority 1: weapons to barracks
    if (f->resource_id == RESOURCE_WEAPONS && !city_data.resource.stockpiled[RESOURCE_WEAPONS] && building_count_active(BUILDING_BARRACKS)) {
        struct building_t *b = &all_buildings[city_data.building.barracks_building_id];
        if (b->loads_stored < 5 && city_data.military.legionary_legions) {
            if (map_has_road_access(b->x, b->y, b->size, &dst) && b->road_network_id == road_network_id) {
                dst_building_id = b->id;
            }
        }
    }
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
        remove_resource_from_warehouse(f);
        return;
    }
    // priority 2: raw materials to workshop
    dst_building_id = building_get_workshop_for_raw_material_with_room(f->x, f->y, f->resource_id, road_network_id, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
        remove_resource_from_warehouse(f);
        return;
    }
    // priority 3: food to granary
    dst_building_id = building_granary_for_storing(f->x, f->y, f->resource_id, road_network_id, 0, 0, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
        remove_resource_from_warehouse(f);
        return;
    }
    // priority 4: food to getting granary
    if (!scenario.rome_supplies_wheat && resource_is_food(f->resource_id) && !city_data.resource.stockpiled[f->resource_id]) {
        int min_dist = INFINITE;
        int min_building_id = 0;
        for (int i = 1; i < MAX_BUILDINGS; i++) {
            struct building_t *b = &all_buildings[i];
            if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_GRANARY) {
                continue;
            }
            if (!b->has_road_access || b->road_network_id != road_network_id) {
                continue;
            }
            if (calc_percentage(b->num_workers, building_properties[b->type].n_laborers) < 100) {
                continue;
            }
            struct building_storage_t *s = building_storage_get(b->storage_id);
            if (s->resource_state[f->resource_id] != BUILDING_STORAGE_STATE_GETTING || s->empty_all) {
                continue;
            }
            if (b->data.granary.resource_stored[RESOURCE_NONE] > ONE_LOAD) {
                // there is room
                int dist = calc_maximum_distance(b->x + 1, b->y + 1, f->x, f->y);
                if (dist < min_dist) {
                    min_dist = dist;
                    min_building_id = i;
                }
            }
        }
        struct building_t *min = &all_buildings[min_building_id];
        dst.x = min->x + 1;
        dst.y = min->y + 1;
        dst_building_id = min_building_id;
        if (dst_building_id) {
            set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
            remove_resource_from_warehouse(f);
            return;
        }
    }
    // priority 5: resource to other warehouse
    dst_building_id = building_warehouse_for_storing(f->building_id, f->x, f->y, f->resource_id, road_network_id, 0, &dst);
    if (dst_building_id) {
        if (dst_building_id == f->building_id) {
            figure_delete(f);
        } else {
            set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
            remove_resource_from_warehouse(f);
        }
        return;
    }
    // priority 6: raw material to well-stocked workshop
    dst_building_id = building_get_workshop_for_raw_material(f->x, f->y, f->resource_id, road_network_id, &dst);
    if (dst_building_id) {
        set_destination(f, FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE, dst_building_id, dst.x, dst.y);
        remove_resource_from_warehouse(f);
        return;
    }
    // no destination: kill figure
    figure_delete(f);
}

void figure_warehouseman_action(struct figure_t *f)
{
    f->terrain_usage = TERRAIN_USAGE_ROADS;
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    int road_network_id = map_road_network_get(f->grid_offset);

    switch (f->action_state) {
        case FIGURE_ACTION_WAREHOUSEMAN_CREATED:
        {
            struct building_t *b = &all_buildings[f->building_id];
            if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
                figure_delete(f);
                return;
            }
            f->wait_ticks++;
            if (f->wait_ticks > 2) {
                if (all_buildings[f->building_id].type == BUILDING_GRANARY) {
                    determine_granaryman_destination(f, road_network_id);
                } else {
                    determine_warehouseman_destination(f, road_network_id);
                }
            }
            f->image_offset = 0;
            break;
        }
        case FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE:
            if (f->loads_sold_or_carrying == 1) {
                f->cart_image_id = image_group(GROUP_FIGURE_CARTPUSHER_CART_MULTIPLE_FOOD) +
                    8 * f->resource_id - 8 + resource_image_offset(f->resource_id, RESOURCE_IMAGE_FOOD_CART);
            } else {
                set_cart_graphic(f);
            }
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_WAREHOUSEMAN_AT_DELIVERY_BUILDING;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_WAREHOUSEMAN_AT_DELIVERY_BUILDING:
            f->wait_ticks++;
            if (f->wait_ticks > 4) {
                struct building_t *b = &all_buildings[f->destination_building_id];
                switch (b->type) {
                    case BUILDING_GRANARY:
                        building_granary_add_resource(b, f->resource_id, 0);
                        break;
                    case BUILDING_BARRACKS:
                        b->loads_stored++;
                        break;
                    case BUILDING_WAREHOUSE:
                    case BUILDING_WAREHOUSE_SPACE:
                        building_warehouse_add_resource(b, f->resource_id);
                        break;
                    default: // workshop
                        building_workshop_add_raw_material(b);
                        break;
                }
                // BUG: what if warehouse/granary is full and returns false?
                f->action_state = FIGURE_ACTION_WAREHOUSEMAN_RETURNING_EMPTY;
                f->wait_ticks = 0;
                f->destination_x = f->source_x;
                f->destination_y = f->source_y;
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_EMPTY:
            f->cart_image_id = EMPTY_CART_IMG_ID;
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_WAREHOUSEMAN_GETTING_FOOD:
            f->cart_image_id = EMPTY_CART_IMG_ID;
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_WAREHOUSEMAN_AT_GRANARY;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_WAREHOUSEMAN_AT_GRANARY:
            f->wait_ticks++;
            if (f->wait_ticks > 4) {
                struct building_t *src = &all_buildings[f->destination_building_id];
                struct building_t *dst = &all_buildings[f->building_id];
                struct building_storage_t *s_src = building_storage_get(src->storage_id);
                struct building_storage_t *s_dst = building_storage_get(dst->storage_id);
                int max_amount = 0;
                int max_resource = 0;
                if (s_dst->resource_state[RESOURCE_WHEAT] == BUILDING_STORAGE_STATE_GETTING &&
                        s_src->resource_state[RESOURCE_WHEAT] != BUILDING_STORAGE_STATE_GETTING) {
                    if (src->data.granary.resource_stored[RESOURCE_WHEAT] > max_amount) {
                        max_amount = src->data.granary.resource_stored[RESOURCE_WHEAT];
                        max_resource = RESOURCE_WHEAT;
                    }
                }
                if (s_dst->resource_state[RESOURCE_VEGETABLES] == BUILDING_STORAGE_STATE_GETTING &&
                        s_src->resource_state[RESOURCE_VEGETABLES] != BUILDING_STORAGE_STATE_GETTING) {
                    if (src->data.granary.resource_stored[RESOURCE_VEGETABLES] > max_amount) {
                        max_amount = src->data.granary.resource_stored[RESOURCE_VEGETABLES];
                        max_resource = RESOURCE_VEGETABLES;
                    }
                }
                if (s_dst->resource_state[RESOURCE_FRUIT] == BUILDING_STORAGE_STATE_GETTING &&
                        s_src->resource_state[RESOURCE_FRUIT] != BUILDING_STORAGE_STATE_GETTING) {
                    if (src->data.granary.resource_stored[RESOURCE_FRUIT] > max_amount) {
                        max_amount = src->data.granary.resource_stored[RESOURCE_FRUIT];
                        max_resource = RESOURCE_FRUIT;
                    }
                }
                if (s_dst->resource_state[RESOURCE_MEAT] == BUILDING_STORAGE_STATE_GETTING &&
                        s_src->resource_state[RESOURCE_MEAT] != BUILDING_STORAGE_STATE_GETTING) {
                    if (src->data.granary.resource_stored[RESOURCE_MEAT] > max_amount) {
                        max_amount = src->data.granary.resource_stored[RESOURCE_MEAT];
                        max_resource = RESOURCE_MEAT;
                    }
                }
                if (max_amount > 800) {
                    max_amount = 800;
                }
                if (max_amount > dst->data.granary.resource_stored[RESOURCE_NONE]) {
                    max_amount = dst->data.granary.resource_stored[RESOURCE_NONE];
                }
                building_granary_remove_resource(src, max_resource, max_amount);
                f->loads_sold_or_carrying = max_amount / UNITS_PER_LOAD;
                f->resource_id = max_resource;
                f->action_state = FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_FOOD;
                f->wait_ticks = 0;
                f->destination_x = f->source_x;
                f->destination_y = f->source_y;
                figure_route_remove(f);
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_FOOD:
            // update graphic
            if (f->loads_sold_or_carrying <= 0) {
                f->cart_image_id = EMPTY_CART_IMG_ID;
            } else if (f->loads_sold_or_carrying == 1) {
                set_cart_graphic(f);
            } else {
                if (f->loads_sold_or_carrying >= 8) {
                    f->cart_image_id = image_group(GROUP_FIGURE_CARTPUSHER_CART_MULTIPLE_FOOD) +
                        CART_OFFSET_8_LOADS_FOOD[f->resource_id];
                } else {
                    f->cart_image_id = image_group(GROUP_FIGURE_CARTPUSHER_CART_MULTIPLE_FOOD) +
                        CART_OFFSET_MULTIPLE_LOADS_FOOD[f->resource_id];
                }
                f->cart_image_id += resource_image_offset(f->resource_id, RESOURCE_IMAGE_FOOD_CART);
            }
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                for (int i = 0; i < f->loads_sold_or_carrying; i++) {
                    building_granary_add_resource(&all_buildings[f->building_id], f->resource_id, 0);
                }
                figure_delete(f);
                return;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_WAREHOUSEMAN_GETTING_RESOURCE:
            f->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
            f->cart_image_id = EMPTY_CART_IMG_ID;
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_WAREHOUSEMAN_AT_WAREHOUSE;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_WAREHOUSEMAN_AT_WAREHOUSE:
            f->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
            f->wait_ticks++;
            if (f->wait_ticks > 4) {
                f->loads_sold_or_carrying = 0;
                while (f->loads_sold_or_carrying < 4 && 0 == building_warehouse_remove_resource(
                    &all_buildings[f->destination_building_id], f->collecting_item_id, 1)) {
                    f->loads_sold_or_carrying++;
                }
                f->resource_id = f->collecting_item_id;
                f->action_state = FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_RESOURCE;
                f->wait_ticks = 0;
                f->destination_x = f->source_x;
                f->destination_y = f->source_y;
                figure_route_remove(f);
            }
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_RESOURCE:
            f->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
            // update graphic
            if (f->loads_sold_or_carrying <= 0) {
                f->cart_image_id = EMPTY_CART_IMG_ID;
            } else if (f->loads_sold_or_carrying == 1) {
                set_cart_graphic(f);
            } else {
                if (f->resource_id == RESOURCE_WHEAT || f->resource_id == RESOURCE_VEGETABLES ||
                    f->resource_id == RESOURCE_FRUIT || f->resource_id == RESOURCE_MEAT) {
                    f->cart_image_id = image_group(GROUP_FIGURE_CARTPUSHER_CART_MULTIPLE_FOOD) +
                        CART_OFFSET_MULTIPLE_LOADS_FOOD[f->resource_id];
                } else {
                    f->cart_image_id = image_group(GROUP_FIGURE_CARTPUSHER_CART_MULTIPLE_RESOURCE) +
                        CART_OFFSET_MULTIPLE_LOADS_NON_FOOD[f->resource_id];
                }
                f->cart_image_id += resource_image_offset(f->resource_id, RESOURCE_IMAGE_FOOD_CART);
            }
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                for (int i = 0; i < f->loads_sold_or_carrying; i++) {
                    building_warehouse_add_resource(&all_buildings[f->building_id], f->resource_id);
                }
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
    update_image(f);
}
