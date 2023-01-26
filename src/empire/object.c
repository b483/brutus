#include "object.h"

#include "city/buildings.h"
#include "city/finance.h"
#include "city/map.h"
#include "city/message.h"
#include "city/trade.h"
#include "core/calc.h"
#include "core/image.h"
#include "empire/trade_route.h"
#include "figuretype/trader.h"
#include "game/animation.h"
#include "scenario/data.h"
#include "scenario/map.h"

struct empire_object_t empire_objects[MAX_OBJECTS];

static void fix_image_ids(void)
{
    int image_id = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use
            && empire_objects[i].type == EMPIRE_OBJECT_CITY
            && empire_objects[i].city_type == EMPIRE_CITY_OURS) {
            image_id = empire_objects[i].image_id;
            break;
        }
    }
    if (image_id > 0 && image_id != image_group(GROUP_EMPIRE_CITY)) {
        // empire map uses old version of graphics: increase every graphic id
        int offset = image_group(GROUP_EMPIRE_CITY) - image_id;
        for (int i = 0; i < MAX_OBJECTS; i++) {
            if (!empire_objects[i].in_use) {
                continue;
            }
            if (empire_objects[i].image_id) {
                empire_objects[i].image_id += offset;
                if (empire_objects[i].expanded.image_id) {
                    empire_objects[i].expanded.image_id += offset;
                }
            }
        }
    }
}

void empire_object_load_initial(buffer *buf)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        empire_objects[i].id = i;
        empire_objects[i].type = buffer_read_u8(buf);
        empire_objects[i].in_use = buffer_read_u8(buf);
        empire_objects[i].animation_index = buffer_read_u8(buf);
        buffer_skip(buf, 1);
        empire_objects[i].x = buffer_read_i16(buf);
        empire_objects[i].y = buffer_read_i16(buf);
        empire_objects[i].width = buffer_read_i16(buf);
        empire_objects[i].height = buffer_read_i16(buf);
        empire_objects[i].image_id = buffer_read_i16(buf);
        empire_objects[i].expanded.image_id = buffer_read_i16(buf);
        buffer_skip(buf, 1);
        empire_objects[i].distant_battle_travel_months = buffer_read_u8(buf);
        buffer_skip(buf, 2);
        empire_objects[i].expanded.x = buffer_read_i16(buf);
        empire_objects[i].expanded.y = buffer_read_i16(buf);
        empire_objects[i].city_type = buffer_read_u8(buf);
        empire_objects[i].city_name_id = buffer_read_u8(buf);
        empire_objects[i].trade_route_id = buffer_read_u8(buf);
        empire_objects[i].trade_route_open = buffer_read_u8(buf);
        empire_objects[i].trade_route_cost = buffer_read_i16(buf);
        for (int r = 0; r < 10; r++) {
            empire_objects[i].resources_sell_list.resource[r] = buffer_read_u8(buf);
        }
        buffer_skip(buf, 2);
        for (int r = 0; r < 8; r++) {
            empire_objects[i].resources_buy_list.resource[r] = buffer_read_u8(buf);
        }
        empire_objects[i].invasion_path_id = buffer_read_u8(buf);
        empire_objects[i].invasion_years = buffer_read_u8(buf);
        buffer_skip(buf, 6); // for the next 3
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
        empire_objects[i].id = i;
        empire_objects[i].type = buffer_read_u8(buf);
        empire_objects[i].x = buffer_read_i16(buf);
        empire_objects[i].y = buffer_read_i16(buf);
        empire_objects[i].image_id = buffer_read_i16(buf);
        empire_objects[i].expanded.x = buffer_read_i16(buf);
        empire_objects[i].expanded.y = buffer_read_i16(buf);
        empire_objects[i].expanded.image_id = buffer_read_i16(buf);
        empire_objects[i].width = buffer_read_i16(buf);
        empire_objects[i].height = buffer_read_i16(buf);
        empire_objects[i].animation_index = buffer_read_u8(buf);
        empire_objects[i].in_use = buffer_read_u8(buf);
        empire_objects[i].city_type = buffer_read_u8(buf);
        empire_objects[i].city_name_id = buffer_read_u8(buf);
        empire_objects[i].trade_route_id = buffer_read_u8(buf);
        empire_objects[i].trade_route_open = buffer_read_u8(buf);
        empire_objects[i].trade_route_cost = buffer_read_u32(buf);
        empire_objects[i].trader_entry_delay = buffer_read_i16(buf);
        empire_objects[i].is_sea_trade = buffer_read_u8(buf);
        for (int f = 0; f < 3; f++) {
            empire_objects[i].trader_figure_ids[f] = buffer_read_i16(buf);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            empire_objects[i].resources_sell_list.resource[r] = buffer_read_u8(buf);
            empire_objects[i].resources_sell_list.resource_limit[r] = buffer_read_u8(buf);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            empire_objects[i].resources_buy_list.resource[r] = buffer_read_u8(buf);
            empire_objects[i].resources_buy_list.resource_limit[r] = buffer_read_u8(buf);
        }
        empire_objects[i].invasion_path_id = buffer_read_u8(buf);
        empire_objects[i].invasion_years = buffer_read_u8(buf);
        empire_objects[i].distant_battle_travel_months = buffer_read_u8(buf);
    }
    fix_image_ids();
}

void empire_object_save_state(buffer *buf)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        buffer_write_u8(buf, empire_objects[i].type);
        buffer_write_i16(buf, empire_objects[i].x);
        buffer_write_i16(buf, empire_objects[i].y);
        buffer_write_i16(buf, empire_objects[i].image_id);
        buffer_write_i16(buf, empire_objects[i].expanded.x);
        buffer_write_i16(buf, empire_objects[i].expanded.y);
        buffer_write_i16(buf, empire_objects[i].expanded.image_id);
        buffer_write_i16(buf, empire_objects[i].width);
        buffer_write_i16(buf, empire_objects[i].height);
        buffer_write_u8(buf, empire_objects[i].animation_index);
        buffer_write_u8(buf, empire_objects[i].in_use);
        buffer_write_u8(buf, empire_objects[i].city_type);
        buffer_write_u8(buf, empire_objects[i].city_name_id);
        buffer_write_u8(buf, empire_objects[i].trade_route_id);
        buffer_write_u8(buf, empire_objects[i].trade_route_open);
        buffer_write_u32(buf, empire_objects[i].trade_route_cost);
        buffer_write_i16(buf, empire_objects[i].trader_entry_delay);
        buffer_write_u8(buf, empire_objects[i].is_sea_trade);
        for (int f = 0; f < 3; f++) {
            buffer_write_i16(buf, empire_objects[i].trader_figure_ids[f]);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            buffer_write_u8(buf, empire_objects[i].resources_sell_list.resource[r]);
            buffer_write_u8(buf, empire_objects[i].resources_sell_list.resource_limit[r]);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            buffer_write_u8(buf, empire_objects[i].resources_buy_list.resource[r]);
            buffer_write_u8(buf, empire_objects[i].resources_buy_list.resource_limit[r]);
        }
        buffer_write_u8(buf, empire_objects[i].invasion_path_id);
        buffer_write_u8(buf, empire_objects[i].invasion_years);
        buffer_write_u8(buf, empire_objects[i].distant_battle_travel_months);
    }
}

void empire_object_init_cities(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!empire_objects[i].in_use || empire_objects[i].type != EMPIRE_OBJECT_CITY) {
            continue;
        }
        if (empire_objects[i].trade_route_id >= 20) {
            empire_objects[i].trade_route_id = 19;
        }
        empire_objects[i].is_sea_trade = empire_object_is_sea_trade_route(empire_objects[i].trade_route_id);

        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            if (empire_objects[i].city_type != EMPIRE_CITY_OURS && empire_objects[i].city_type != EMPIRE_CITY_TRADE && empire_objects[i].city_type != EMPIRE_CITY_FUTURE_TRADE) {
                continue;
            }
            if (empire_objects[i].resources_sell_list.resource[r]) {
                trade_route_init(empire_objects[i].trade_route_id, r, empire_objects[i].resources_sell_list.resource_limit[r]);
            }
            if (empire_objects[i].resources_buy_list.resource[r]) {
                trade_route_init(empire_objects[i].trade_route_id, r, empire_objects[i].resources_buy_list.resource_limit[r]);
            }
        }
        empire_objects[i].trader_entry_delay = 4;
        empire_objects[i].trader_figure_ids[0] = 0;
        empire_objects[i].trader_figure_ids[1] = 0;
        empire_objects[i].trader_figure_ids[2] = 0;
    }
}

int empire_object_init_distant_battle_travel_months(int object_type)
{
    int month = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use && empire_objects[i].type == object_type) {
            month++;
            empire_objects[i].distant_battle_travel_months = month;
        }
    }
    return month;
}

struct empire_object_t *get_empire_object_by_trade_route(int trade_route_id)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use && empire_objects[i].type == EMPIRE_OBJECT_CITY && empire_objects[i].trade_route_id == trade_route_id) {
            return &empire_objects[i];
        }
    }
    return 0;
}

int empire_object_get_closest(int x, int y)
{
    int min_dist = 10000;
    int min_obj_id = 0;
    for (int i = 0; i < MAX_OBJECTS && empire_objects[i].in_use; i++) {
        int obj_x, obj_y;
        if (scenario.empire.is_expanded) {
            obj_x = empire_objects[i].expanded.x;
            obj_y = empire_objects[i].expanded.y;
        } else {
            obj_x = empire_objects[i].x;
            obj_y = empire_objects[i].y;
        }
        if (obj_x - 8 > x || obj_x + empire_objects[i].width + 8 <= x) {
            continue;
        }
        if (obj_y - 8 > y || obj_y + empire_objects[i].height + 8 <= y) {
            continue;
        }
        int dist = calc_maximum_distance(x, y, obj_x + empire_objects[i].width / 2, obj_y + empire_objects[i].height / 2);
        if (dist < min_dist) {
            min_dist = dist;
            min_obj_id = i + 1;
        }
    }
    return min_obj_id;
}

struct empire_object_t *empire_object_get_our_city(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use && empire_objects[i].type == EMPIRE_OBJECT_CITY && empire_objects[i].city_type == EMPIRE_CITY_OURS) {
            return &empire_objects[i];
        }
    }
    return 0;
}

int empire_object_update_animation(struct empire_object_t *obj, int image_id)
{
    // return empire_objects[obj->id].animation_index = get_animation_offset(image_id, obj->animation_index);

    if (obj->animation_index <= 0) {
        obj->animation_index = 1;
    }
    const image *img = image_get(image_id);
    if (!game_animation_should_advance(img->animation_speed_id)) {
        return obj->animation_index;
    }
    if (img->animation_can_reverse) {
        int is_reverse = 0;
        if (obj->animation_index & 0x80) {
            is_reverse = 1;
        }
        int current_sprite = obj->animation_index & 0x7f;
        if (is_reverse) {
            obj->animation_index = current_sprite - 1;
            if (obj->animation_index < 1) {
                obj->animation_index = 1;
                is_reverse = 0;
            }
        } else {
            obj->animation_index = current_sprite + 1;
            if (obj->animation_index > img->num_animation_sprites) {
                obj->animation_index = img->num_animation_sprites;
                is_reverse = 1;
            }
        }
        if (is_reverse) {
            obj->animation_index = obj->animation_index | 0x80;
        }
    } else {
        // Absolutely normal case
        obj->animation_index++;
        if (obj->animation_index > img->num_animation_sprites) {
            obj->animation_index = 1;
        }
    }
    return obj->animation_index;
}

void empire_object_our_city_set_resources_sell(void)
{
    struct empire_object_t *our_city = empire_object_get_our_city();
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
        if (!empire_objects[i].in_use || empire_objects[i].city_type != EMPIRE_CITY_TRADE) {
            continue;
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            empire_objects[i].resources_sell_list.resource[r] = 0;
            empire_objects[i].resources_sell_list.resource_limit[r] = 0;
            empire_objects[i].resources_buy_list.resource[r] = 0;
            empire_objects[i].resources_buy_list.resource_limit[r] = 0;
        }
    }
}

int empire_object_trade_route_is_open(int trade_route_id)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use && empire_objects[i].trade_route_id == trade_route_id) {
            return empire_objects[i].trade_route_open;
        }
    }
    return 0;
}

int empire_object_is_sea_trade_route(int route_id)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use && empire_objects[i].trade_route_id == route_id) {
            return empire_objects[i].type == EMPIRE_OBJECT_SEA_TRADE_ROUTE ? 1 : 0;
        }
    }
    return 0;
}

int empire_can_import_resource(int resource)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use
            && empire_objects[i].city_type == EMPIRE_CITY_TRADE
            && empire_objects[i].trade_route_open
            && empire_objects[i].resources_sell_list.resource[resource]) {
            return 1;
        }
    }
    return 0;
}

int empire_can_export_resource(int resource)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use
            && empire_objects[i].city_type == EMPIRE_CITY_TRADE
            && empire_objects[i].trade_route_open
            && empire_objects[i].resources_buy_list.resource[resource]) {
            return 1;
        }
    }
    return 0;
}

int empire_object_our_city_can_produce_resource(int resource)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use && empire_objects[i].city_type == EMPIRE_CITY_OURS) {
            if (empire_objects[i].resources_sell_list.resource[resource]) {
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
    int raw_resource;
    switch (resource) {
        case RESOURCE_POTTERY:
            raw_resource = RESOURCE_CLAY;
            break;
        case RESOURCE_FURNITURE:
            raw_resource = RESOURCE_TIMBER;
            break;
        case RESOURCE_OIL:
            raw_resource = RESOURCE_OLIVES;
            break;
        case RESOURCE_WINE:
            raw_resource = RESOURCE_VINES;
            break;
        case RESOURCE_WEAPONS:
            raw_resource = RESOURCE_IRON;
            break;
        default:
            raw_resource = resource;
            break;
    }

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
