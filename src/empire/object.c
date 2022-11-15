#include "object.h"

#include "city/buildings.h"
#include "city/finance.h"
#include "city/map.h"
#include "city/message.h"
#include "city/trade.h"
#include "core/calc.h"
#include "core/image.h"
#include "empire/trade_route.h"
#include "empire/type.h"
#include "figuretype/trader.h"
#include "game/animation.h"
#include "scenario/data.h"
#include "scenario/map.h"

#define MAX_OBJECTS 200

static empire_object objects[MAX_OBJECTS];

static int get_raw_resource(int resource);
static int generate_trader(empire_object *city);

static void fix_image_ids(void)
{
    int image_id = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use
            && objects[i].type == EMPIRE_OBJECT_CITY
            && objects[i].city_type == EMPIRE_CITY_OURS) {
            image_id = objects[i].image_id;
            break;
        }
    }
    if (image_id > 0 && image_id != image_group(GROUP_EMPIRE_CITY)) {
        // empire map uses old version of graphics: increase every graphic id
        int offset = image_group(GROUP_EMPIRE_CITY) - image_id;
        for (int i = 0; i < MAX_OBJECTS; i++) {
            if (!objects[i].in_use) {
                continue;
            }
            if (objects[i].image_id) {
                objects[i].image_id += offset;
                if (objects[i].expanded.image_id) {
                    objects[i].expanded.image_id += offset;
                }
            }
        }
    }
}

void empire_object_load_initial(buffer *buf)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        empire_object *obj = &objects[i];
        obj->id = i;
        obj->type = buffer_read_u8(buf);
        obj->in_use = buffer_read_u8(buf);
        obj->animation_index = buffer_read_u8(buf);
        buffer_skip(buf, 1);
        obj->x = buffer_read_i16(buf);
        obj->y = buffer_read_i16(buf);
        obj->width = buffer_read_i16(buf);
        obj->height = buffer_read_i16(buf);
        obj->image_id = buffer_read_i16(buf);
        obj->expanded.image_id = buffer_read_i16(buf);
        buffer_skip(buf, 1);
        obj->distant_battle_travel_months = buffer_read_u8(buf);
        buffer_skip(buf, 2);
        obj->expanded.x = buffer_read_i16(buf);
        obj->expanded.y = buffer_read_i16(buf);
        obj->city_type = buffer_read_u8(buf);
        obj->city_name_id = buffer_read_u8(buf);
        obj->trade_route_id = buffer_read_u8(buf);
        obj->trade_route_open = buffer_read_u8(buf);
        obj->trade_route_cost = buffer_read_i16(buf);
        for (int r = 0; r < 10; r++) {
            obj->resources_sell_list.resource[r] = buffer_read_u8(buf);
        }
        buffer_skip(buf, 2);
        for (int r = 0; r < 8; r++) {
            obj->resources_buy_list.resource[r] = buffer_read_u8(buf);
        }
        obj->invasion_path_id = buffer_read_u8(buf);
        obj->invasion_years = buffer_read_u8(buf);
        buffer_skip(buf, 6);
        // full->trade40 = buffer_read_u16(buf);
        // full->trade25 = buffer_read_u16(buf);
        // full->trade15 = buffer_read_u16(buf);
        buffer_skip(buf, 6);
    }
    fix_image_ids();
}

void empire_object_load_state(buffer *buf)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        empire_object *obj = &objects[i];
        obj->id = i;
        obj->type = buffer_read_u8(buf);
        obj->x = buffer_read_i16(buf);
        obj->y = buffer_read_i16(buf);
        obj->image_id = buffer_read_i16(buf);
        obj->expanded.x = buffer_read_i16(buf);
        obj->expanded.y = buffer_read_i16(buf);
        obj->expanded.image_id = buffer_read_i16(buf);
        obj->width = buffer_read_i16(buf);
        obj->height = buffer_read_i16(buf);
        obj->animation_index = buffer_read_u8(buf);
        obj->in_use = buffer_read_u8(buf);
        obj->city_type = buffer_read_u8(buf);
        obj->city_name_id = buffer_read_u8(buf);
        obj->trade_route_id = buffer_read_u8(buf);
        obj->trade_route_open = buffer_read_u8(buf);
        obj->trade_route_cost = buffer_read_u32(buf);
        obj->trader_entry_delay = buffer_read_i16(buf);
        obj->is_sea_trade = buffer_read_u8(buf);
        for (int f = 0; f < 3; f++) {
            obj->trader_figure_ids[f] = buffer_read_i16(buf);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            obj->resources_sell_list.resource[r] = buffer_read_u8(buf);
            obj->resources_sell_list.resource_limit[r] = buffer_read_u8(buf);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            obj->resources_buy_list.resource[r] = buffer_read_u8(buf);
            obj->resources_buy_list.resource_limit[r] = buffer_read_u8(buf);
        }
        obj->invasion_path_id = buffer_read_u8(buf);
        obj->invasion_years = buffer_read_u8(buf);
        obj->distant_battle_travel_months = buffer_read_u8(buf);
    }
    fix_image_ids();
}

void empire_object_save_state(buffer *buf)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        empire_object *obj = &objects[i];
        buffer_write_u8(buf, obj->type);
        buffer_write_i16(buf, obj->x);
        buffer_write_i16(buf, obj->y);
        buffer_write_i16(buf, obj->image_id);
        buffer_write_i16(buf, obj->expanded.x);
        buffer_write_i16(buf, obj->expanded.y);
        buffer_write_i16(buf, obj->expanded.image_id);
        buffer_write_i16(buf, obj->width);
        buffer_write_i16(buf, obj->height);
        buffer_write_u8(buf, obj->animation_index);
        buffer_write_u8(buf, obj->in_use);
        buffer_write_u8(buf, obj->city_type);
        buffer_write_u8(buf, obj->city_name_id);
        buffer_write_u8(buf, obj->trade_route_id);
        buffer_write_u8(buf, obj->trade_route_open);
        buffer_write_u32(buf, obj->trade_route_cost);
        buffer_write_i16(buf, obj->trader_entry_delay);
        buffer_write_u8(buf, obj->is_sea_trade);
        for (int f = 0; f < 3; f++) {
            buffer_write_i16(buf, obj->trader_figure_ids[f]);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            buffer_write_u8(buf, obj->resources_sell_list.resource[r]);
            buffer_write_u8(buf, obj->resources_sell_list.resource_limit[r]);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            buffer_write_u8(buf, obj->resources_buy_list.resource[r]);
            buffer_write_u8(buf, obj->resources_buy_list.resource_limit[r]);
        }
        buffer_write_u8(buf, obj->invasion_path_id);
        buffer_write_u8(buf, obj->invasion_years);
        buffer_write_u8(buf, obj->distant_battle_travel_months);
    }
}

void empire_object_init_cities(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].in_use || objects[i].type != EMPIRE_OBJECT_CITY) {
            continue;
        }
        empire_object *obj = &objects[i];
        if (obj->trade_route_id < 0) {
            obj->trade_route_id = 0;
        }
        if (obj->trade_route_id >= 20) {
            obj->trade_route_id = 19;
        }
        obj->is_sea_trade = empire_object_is_sea_trade_route(obj->trade_route_id);

        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (obj->city_type != EMPIRE_CITY_OURS && obj->city_type != EMPIRE_CITY_TRADE && obj->city_type != EMPIRE_CITY_FUTURE_TRADE) {
                continue;
            }
            if (obj->resources_sell_list.resource[r]) {
                trade_route_init(obj->trade_route_id, r, obj->resources_sell_list.resource_limit[r]);
            }
            if (obj->resources_buy_list.resource[r]) {
                trade_route_init(obj->trade_route_id, r, obj->resources_buy_list.resource_limit[r]);
            }
        }
        obj->trader_entry_delay = 4;
        obj->trader_figure_ids[0] = 0;
        obj->trader_figure_ids[1] = 0;
        obj->trader_figure_ids[2] = 0;
    }
}

int empire_object_init_distant_battle_travel_months(int object_type)
{
    int month = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use && objects[i].type == object_type) {
            month++;
            objects[i].distant_battle_travel_months = month;
        }
    }
    return month;
}

empire_object *empire_object_get(int object_id)
{
    return &objects[object_id];
}

int empire_object_get_trade_route_id(int object_id)
{
    return objects[object_id].trade_route_id;
}

empire_object *empire_object_get_for_trade_route(int trade_route_id)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use && objects[i].type == EMPIRE_OBJECT_CITY && objects[i].trade_route_id == trade_route_id) {
            return &objects[i];
        }
    }
    return 0;
}

const empire_object *empire_object_get_battle_icon(int path_id, int year)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use
            && objects[i].type == EMPIRE_OBJECT_BATTLE_ICON
            && objects[i].invasion_path_id == path_id
            && objects[i].invasion_years == year) {
            return &objects[i];
        }
    }
    return 0;
}

int empire_object_get_max_invasion_path(void)
{
    int max_path = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use && objects[i].type == EMPIRE_OBJECT_BATTLE_ICON) {
            if (objects[i].invasion_path_id > max_path) {
                max_path = objects[i].invasion_path_id;
            }
        }
    }
    return max_path;
}

int empire_object_get_closest(int x, int y)
{
    int min_dist = 10000;
    int min_obj_id = 0;
    for (int i = 0; i < MAX_OBJECTS && objects[i].in_use; i++) {
        const empire_object *obj = &objects[i];
        int obj_x, obj_y;
        if (scenario.empire.is_expanded) {
            obj_x = obj->expanded.x;
            obj_y = obj->expanded.y;
        } else {
            obj_x = obj->x;
            obj_y = obj->y;
        }
        if (obj_x - 8 > x || obj_x + obj->width + 8 <= x) {
            continue;
        }
        if (obj_y - 8 > y || obj_y + obj->height + 8 <= y) {
            continue;
        }
        int dist = calc_maximum_distance(x, y, obj_x + obj->width / 2, obj_y + obj->height / 2);
        if (dist < min_dist) {
            min_dist = dist;
            min_obj_id = i + 1;
        }
    }
    return min_obj_id;
}

empire_object *empire_object_get_our_city(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use && objects[i].type == EMPIRE_OBJECT_CITY && objects[i].city_type == EMPIRE_CITY_OURS) {
            return &objects[i];
        }
    }
    return 0;
}

void empire_object_foreach(void (*callback)(const empire_object *))
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use) {
            callback(&objects[i]);
        }
    }
}

void empire_object_set_expanded(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].in_use || objects[i].type != EMPIRE_OBJECT_CITY) {
            continue;
        }
        if (objects[i].city_type == EMPIRE_CITY_FUTURE_TRADE) {
            objects[i].city_type = EMPIRE_CITY_TRADE;
            objects[i].expanded.image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
        } else if (objects[i].city_type == EMPIRE_CITY_FUTURE_ROMAN) {
            objects[i].city_type = EMPIRE_CITY_DISTANT_ROMAN;
            objects[i].expanded.image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
        } else {
            continue;
        }
    }
}

static int get_animation_offset(int image_id, int current_index)
{
    if (current_index <= 0) {
        current_index = 1;
    }
    const image *img = image_get(image_id);
    int animation_speed = img->animation_speed_id;
    if (!game_animation_should_advance(animation_speed)) {
        return current_index;
    }
    if (img->animation_can_reverse) {
        int is_reverse = 0;
        if (current_index & 0x80) {
            is_reverse = 1;
        }
        int current_sprite = current_index & 0x7f;
        if (is_reverse) {
            current_index = current_sprite - 1;
            if (current_index < 1) {
                current_index = 1;
                is_reverse = 0;
            }
        } else {
            current_index = current_sprite + 1;
            if (current_index > img->num_animation_sprites) {
                current_index = img->num_animation_sprites;
                is_reverse = 1;
            }
        }
        if (is_reverse) {
            current_index = current_index | 0x80;
        }
    } else {
        // Absolutely normal case
        current_index++;
        if (current_index > img->num_animation_sprites) {
            current_index = 1;
        }
    }
    return current_index;
}

int empire_object_update_animation(const empire_object *obj, int image_id)
{
    return objects[obj->id].animation_index = get_animation_offset(image_id, obj->animation_index);
}

void empire_object_our_city_set_resources_sell(void)
{
    empire_object *our_city = empire_object_get_our_city();
    for (int resource = 0; resource < RESOURCE_MAX; resource++) {
        // farms match across enums, rest don't
        if (resource <= RESOURCE_VINES) {
            if (scenario.allowed_buildings[resource + ALLOWED_BUILDING_WHEAT_FARM - 1])
                our_city->resources_sell_list.resource[resource] = resource;
            else {
                our_city->resources_sell_list.resource[resource] = 0;
            }
        }
        if (resource == RESOURCE_MEAT) {
            if (scenario.allowed_buildings[ALLOWED_BUILDING_WHARF]
                || scenario.allowed_buildings[ALLOWED_BUILDING_PIG_FARM]
            ) {
                our_city->resources_sell_list.resource[resource] = resource;
            } else {
                our_city->resources_sell_list.resource[resource] = 0;
            }
        }
        if (resource >= RESOURCE_WINE && resource <= RESOURCE_OIL) {
            if (scenario.allowed_buildings[resource + ALLOWED_BUILDING_WINE_WORKSHOP - 7])
                our_city->resources_sell_list.resource[resource] = resource;
            else {
                our_city->resources_sell_list.resource[resource] = 0;
            }
        }
        if (resource >= RESOURCE_IRON && resource <= RESOURCE_TIMBER) {
            if (scenario.allowed_buildings[resource + ALLOWED_BUILDING_IRON_MINE - 9])
                our_city->resources_sell_list.resource[resource] = resource;
            else {
                our_city->resources_sell_list.resource[resource] = 0;
            }
        }
        if (resource >= RESOURCE_CLAY && resource <= RESOURCE_MARBLE) {
            if (scenario.allowed_buildings[resource + ALLOWED_BUILDING_CLAY_PIT - 11])
                our_city->resources_sell_list.resource[resource] = resource;
            else {
                our_city->resources_sell_list.resource[resource] = 0;
            }
        }
        if (resource >= RESOURCE_WEAPONS) {
            if (scenario.allowed_buildings[resource + ALLOWED_BUILDING_WEAPONS_WORKSHOP - 13])
                our_city->resources_sell_list.resource[resource] = resource;
            else {
                our_city->resources_sell_list.resource[resource] = 0;
            }
        }
    }
}

void empire_object_trade_cities_disable_default_resources(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].in_use || objects[i].city_type != EMPIRE_CITY_TRADE) {
            continue;
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            objects[i].resources_sell_list.resource[r] = 0;
            objects[i].resources_sell_list.resource_limit[r] = 0;
            objects[i].resources_buy_list.resource[r] = 0;
            objects[i].resources_buy_list.resource_limit[r] = 0;
        }
    }
}

void empire_object_disable_postpone_trade_city(empire_object *object, int value)
{
    object->city_type += value;
    if (object->city_type < EMPIRE_CITY_DISTANT_ROMAN) {
        object->city_type = EMPIRE_CITY_FUTURE_TRADE;
    } else if (object->city_type == EMPIRE_CITY_OURS) {
        value == -1 ? object->city_type-- : object->city_type++;
    } else if (object->city_type > EMPIRE_CITY_FUTURE_TRADE) {
        object->city_type = EMPIRE_CITY_DISTANT_ROMAN;
    }

    // fix graphics for city sprite and flag color when changing city types
    if (object->city_type == EMPIRE_CITY_TRADE || object->city_type == EMPIRE_CITY_FUTURE_TRADE) {
        object->image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
        object->expanded.image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
    } else if (object->city_type == EMPIRE_CITY_DISTANT_ROMAN) {
        object->image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
        object->expanded.image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
    }
}

void empire_object_city_toggle_resource(empire_object *object, int resource, int selling)
{
    if (selling) {
        // if resource to sell already enabled, disable
        if (object->resources_sell_list.resource[resource]) {
            object->resources_sell_list.resource[resource] = 0;
        } else {
            // if not enabled, enable by setting resource value in its index place
            object->resources_sell_list.resource[resource] = resource;
            // don't allow simultaneous selling and buying of the same resource
            if (object->resources_buy_list.resource[resource]) {
                object->resources_buy_list.resource[resource] = 0;
            }
        }
    } else {
        // if resource to buy already enabled, disable
        if (object->resources_buy_list.resource[resource]) {
            object->resources_buy_list.resource[resource] = 0;
        } else {
            // if not enabled, enable by setting resource value in its index place
            object->resources_buy_list.resource[resource] = resource;
            // don't allow simultaneous selling and buying of the same resource
            if (object->resources_sell_list.resource[resource]) {
                object->resources_sell_list.resource[resource] = 0;
            }
        }
    }
}

void empire_object_city_set_resource_limit(empire_object *object, int resource, int resource_limit, int selling)
{
    if (selling) {
        object->resources_sell_list.resource_limit[resource] = resource_limit;
    } else {
        object->resources_buy_list.resource_limit[resource] = resource_limit;
    }
}

void empire_object_city_set_trade_route_cost(empire_object *object, int trade_route_cost)
{
    object->trade_route_cost = trade_route_cost;
}

void empire_object_open_trade(empire_object *object)
{
    city_finance_process_construction(object->trade_route_cost);
    object->trade_route_open = 1;
}

int empire_object_trade_route_is_open(int trade_route_id)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use && objects[i].trade_route_id == trade_route_id) {
            return objects[i].trade_route_open;
        }
    }
    return 0;
}

int empire_object_is_sea_trade_route(int route_id)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use && objects[i].trade_route_id == route_id) {
            if (objects[i].type == EMPIRE_OBJECT_SEA_TRADE_ROUTE) {
                return 1;
            }
            if (objects[i].type == EMPIRE_OBJECT_LAND_TRADE_ROUTE) {
                return 0;
            }
        }
    }
    return 0;
}

void empire_object_city_reset_yearly_trade_amounts(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use && objects[i].trade_route_open) {
            trade_route_reset_traded(objects[i].trade_route_id);
        }
    }
}

int empire_object_city_count_wine_sources(void)
{
    int sources = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use
            && objects[i].trade_route_open
            && objects[i].resources_sell_list.resource[RESOURCE_WINE]) {
            sources++;
        }
    }
    return sources;
}

int empire_can_import_resource(int resource)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use
            && objects[i].city_type == EMPIRE_CITY_TRADE
            && objects[i].trade_route_open
            && objects[i].resources_sell_list.resource[resource]) {
            return 1;
        }
    }
    return 0;
}

int empire_can_export_resource(int resource)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use
            && objects[i].city_type == EMPIRE_CITY_TRADE
            && objects[i].trade_route_open
            && objects[i].resources_buy_list.resource[resource]) {
            return 1;
        }
    }
    return 0;
}

static int get_raw_resource(int resource)
{
    switch (resource) {
        case RESOURCE_POTTERY:
            return RESOURCE_CLAY;
        case RESOURCE_FURNITURE:
            return RESOURCE_TIMBER;
        case RESOURCE_OIL:
            return RESOURCE_OLIVES;
        case RESOURCE_WINE:
            return RESOURCE_VINES;
        case RESOURCE_WEAPONS:
            return RESOURCE_IRON;
        default:
            return resource;
    }
}

int empire_object_our_city_can_produce_resource(int resource)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use && objects[i].city_type == EMPIRE_CITY_OURS) {
            if (objects[i].resources_sell_list.resource[resource]) {
                return 1;
            } else {
                // there's only one of our city per empire state, no need to search the rest of the list
                return 0;
            }
        }
    }
    // our city wasn't found or not in use (shouldn't happen)
    return 0;
}

int empire_can_produce_resource(int resource)
{
    int raw_resource = get_raw_resource(resource);

    // if raw resource, available if we can either produce or import it
    if (resource == raw_resource) {
        return (empire_object_our_city_can_produce_resource(resource) || empire_can_import_resource(resource));
    }
    // if finished resource, available if we can either produce the raw material or import it, and we can produce the finished material (workshop is allowed)
    else {
        return (
            (empire_object_our_city_can_produce_resource(raw_resource) || empire_can_import_resource(raw_resource))
            && empire_object_our_city_can_produce_resource(resource)
        );
    }
}

static int generate_trader(empire_object *city)
{
    int max_traders = 0;
    int num_resources = 0;
    for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
        if (city->resources_buy_list.resource[r] || city->resources_sell_list.resource[r]) {
            ++num_resources;
            switch (trade_route_limit(city->trade_route_id, r)) {
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
        if (city_buildings_has_working_dock() && scenario_map_has_river_entry()
            && !city_trade_has_sea_trade_problems()) {
            map_point river_entry = scenario_map_river_entry();
            city->trader_figure_ids[index] = figure_create_trade_ship(river_entry.x, river_entry.y, city->id);
            return 1;
        }
    } else {
        // generate caravan and donkeys
        if (!city_trade_has_land_trade_problems()) {
            // caravan head
            const map_tile *entry = city_map_entry_point();
            city->trader_figure_ids[index] = figure_create_trade_caravan(entry->x, entry->y, city->id);
            return 1;
        }
    }
    return 0;
}

void empire_object_city_generate_trader(void)
{
    for (int i = 1; i < MAX_OBJECTS; i++) {
        if (!objects[i].in_use || !objects[i].trade_route_open) {
            continue;
        }
        if (objects[i].is_sea_trade) {
            if (!city_buildings_has_working_dock()) {
                // delay of 384 = 1 year
                city_message_post_with_message_delay(MESSAGE_CAT_NO_WORKING_DOCK, 1, MESSAGE_NO_WORKING_DOCK, 384);
                continue;
            }
            if (!scenario_map_has_river_entry()) {
                continue;
            }
            city_trade_add_sea_trade_route();
        } else {
            city_trade_add_land_trade_route();
        }
        if (generate_trader(&objects[i])) {
            break;
        }
    }
}

void empire_object_city_remove_trader(int city_id, int figure_id)
{
    for (int i = 0; i < 3; i++) {
        if (objects[city_id].trader_figure_ids[i] == figure_id) {
            objects[city_id].trader_figure_ids[i] = 0;
        }
    }
}

int empire_object_get_vulnerable_roman_city(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use && objects[i].city_type == EMPIRE_CITY_VULNERABLE_ROMAN) {
            return i;
        }
    }
    return 0;
}

void empire_object_city_set_vulnerable(int object_id)
{
    objects[object_id].city_type = EMPIRE_CITY_VULNERABLE_ROMAN;
}

void empire_object_city_set_foreign(int object_id)
{
    objects[object_id].city_type = EMPIRE_CITY_DISTANT_FOREIGN;
}