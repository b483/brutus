#include "object.h"

#include "city/finance.h"
#include "core/calc.h"
#include "core/image.h"
#include "empire/city.h"
#include "empire/trade_route.h"
#include "empire/type.h"
#include "game/animation.h"
#include "scenario/data.h"
#include "scenario/editor.h"
#include "scenario/empire.h"

#define MAX_OBJECTS 200

static empire_object objects[MAX_OBJECTS];


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
            obj->resource_sell.resource[r] = buffer_read_u8(buf);
        }
        buffer_skip(buf, 2);
        for (int r = 0; r < 8; r++) {
            obj->resource_buy.resource[r] = buffer_read_u8(buf);
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
        for (int r = 0; r < RESOURCE_MAX; r++) {
            obj->resource_sell.resource[r] = buffer_read_u8(buf);
            obj->resource_sell.resource_limit[r] = buffer_read_u8(buf);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            obj->resource_buy.resource[r] = buffer_read_u8(buf);
            obj->resource_buy.resource_limit[r] = buffer_read_u8(buf);
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
        for (int r = 0; r < RESOURCE_MAX; r++) {
            buffer_write_u8(buf, obj->resource_sell.resource[r]);
            buffer_write_u8(buf, obj->resource_sell.resource_limit[r]);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            buffer_write_u8(buf, obj->resource_buy.resource[r]);
            buffer_write_u8(buf, obj->resource_buy.resource_limit[r]);
        }
        buffer_write_u8(buf, obj->invasion_path_id);
        buffer_write_u8(buf, obj->invasion_years);
        buffer_write_u8(buf, obj->distant_battle_travel_months);
    }
}


void empire_object_init_cities(void)
{
    empire_city_clear_all();
    int route_index = 1;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].in_use || objects[i].type != EMPIRE_OBJECT_CITY) {
            continue;
        }
        empire_object *obj = &objects[i];
        empire_city *city = empire_city_get(route_index++);
        city->in_use = 1;
        city->type = obj->city_type;
        city->name_id = obj->city_name_id;
        if (obj->trade_route_id < 0) {
            obj->trade_route_id = 0;
        }
        if (obj->trade_route_id >= 20) {
            obj->trade_route_id = 19;
        }
        city->route_id = obj->trade_route_id;
        city->is_open = obj->trade_route_open;
        city->cost_to_open = obj->trade_route_cost;
        city->is_sea_trade = empire_object_is_sea_trade_route(obj->trade_route_id);

        for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
            city->sells_resource[r] = 0;
            city->buys_resource[r] = 0;
            if (city->type != EMPIRE_CITY_OURS && city->type != EMPIRE_CITY_TRADE && city->type != EMPIRE_CITY_FUTURE_TRADE) {
                continue;
            }
            if (obj->resource_sell.resource[r]) {
                city->sells_resource[r] = 1;
                trade_route_init(city->route_id, r, obj->resource_sell.resource_limit[r]);
            }
            if (obj->resource_buy.resource[r]) {
                city->buys_resource[r] = 1;
                trade_route_init(city->route_id, r, obj->resource_buy.resource_limit[r]);
            }
        }
        city->trader_entry_delay = 4;
        city->trader_figure_ids[0] = 0;
        city->trader_figure_ids[1] = 0;
        city->trader_figure_ids[2] = 0;
        city->empire_object_id = i;
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


empire_object *empire_object_get_for_trade_route(int trade_route_id)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].trade_route_id == trade_route_id) {
            return &objects[i];
        }
    }
    return 0;
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
        if (scenario_empire_is_expanded()) {
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


void empire_object_set_expanded(int object_id, int new_city_type)
{
    objects[object_id].city_type = new_city_type;
    if (new_city_type == EMPIRE_CITY_TRADE) {
        objects[object_id].expanded.image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
    } else if (new_city_type == EMPIRE_CITY_DISTANT_ROMAN) {
        objects[object_id].expanded.image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
    }
}


void empire_object_our_city_set_resources_sell(void)
{
    empire_object *our_city = empire_object_get_our_city();
    for (int resource = 0; resource < RESOURCE_MAX; resource++) {
        // farms match across enums, rest don't
        if (resource <= RESOURCE_VINES) {
            if (scenario_editor_is_building_allowed(resource + ALLOWED_BUILDING_WHEAT_FARM - 1))
                our_city->resource_sell.resource[resource] = resource;
            else {
                our_city->resource_sell.resource[resource] = 0;
            }
        }
        if (resource == RESOURCE_MEAT) {
            if (scenario_editor_is_building_allowed(ALLOWED_BUILDING_WHARF)
                || scenario_editor_is_building_allowed(ALLOWED_BUILDING_PIG_FARM)
            ) {
                our_city->resource_sell.resource[resource] = resource;
            } else {
                our_city->resource_sell.resource[resource] = 0;
            }
        }
        if (resource >= RESOURCE_WINE && resource <= RESOURCE_OIL) {
            if (scenario_editor_is_building_allowed(resource + ALLOWED_BUILDING_WINE_WORKSHOP - 7))
                our_city->resource_sell.resource[resource] = resource;
            else {
                our_city->resource_sell.resource[resource] = 0;
            }
        }
        if (resource >= RESOURCE_IRON && resource <= RESOURCE_TIMBER) {
            if (scenario_editor_is_building_allowed(resource + ALLOWED_BUILDING_IRON_MINE - 9))
                our_city->resource_sell.resource[resource] = resource;
            else {
                our_city->resource_sell.resource[resource] = 0;
            }
        }
        if (resource >= RESOURCE_CLAY && resource <= RESOURCE_MARBLE) {
            if (scenario_editor_is_building_allowed(resource + ALLOWED_BUILDING_CLAY_PIT - 11))
                our_city->resource_sell.resource[resource] = resource;
            else {
                our_city->resource_sell.resource[resource] = 0;
            }
        }
        if (resource >= RESOURCE_WEAPONS) {
            if (scenario_editor_is_building_allowed(resource + ALLOWED_BUILDING_WEAPONS_WORKSHOP - 13))
                our_city->resource_sell.resource[resource] = resource;
            else {
                our_city->resource_sell.resource[resource] = 0;
            }
        }
    }
}


void empire_object_open_trade(empire_object *object)
{
    city_finance_process_construction(object->trade_route_cost);
    object->trade_route_open = 1;
}


void empire_object_disable_postpone_trade_city(empire_object *object, int is_down)
{
    object->city_type += is_down ? -1 : 1;
    if (object->city_type < EMPIRE_CITY_DISTANT_ROMAN) {
        object->city_type = EMPIRE_CITY_FUTURE_TRADE;
    } else if (object->city_type == EMPIRE_CITY_OURS) {
        is_down ? object->city_type-- : object->city_type++;
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


void empire_object_trade_cities_disable_default_resources(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].in_use || objects[i].city_type != EMPIRE_CITY_TRADE) {
            continue;
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            objects[i].resource_sell.resource[r] = 0;
            objects[i].resource_sell.resource_limit[r] = 0;
            objects[i].resource_buy.resource[r] = 0;
            objects[i].resource_buy.resource_limit[r] = 0;
        }
    }
}


void empire_object_city_toggle_resource(empire_object *object, int resource, int selling)
{
    if (selling) {
        // if resource to sell already enabled, disable
        if (object->resource_sell.resource[resource]) {
            object->resource_sell.resource[resource] = 0;
        } else {
            // if not enabled, enable by setting resource value in its index place
            object->resource_sell.resource[resource] = resource;
            // don't allow simultaneous selling and buying of the same resource
            if (object->resource_buy.resource[resource]) {
                object->resource_buy.resource[resource] = 0;
            }
        }
    } else {
        // if resource to buy already enabled, disable
        if (object->resource_buy.resource[resource]) {
            object->resource_buy.resource[resource] = 0;
        } else {
            // if not enabled, enable by setting resource value in its index place
            object->resource_buy.resource[resource] = resource;
            // don't allow simultaneous selling and buying of the same resource
            if (object->resource_sell.resource[resource]) {
                object->resource_sell.resource[resource] = 0;
            }
        }
    }
}


void empire_object_city_set_resource_limit(empire_object *object, int resource, int resource_limit, int selling)
{
    if (selling) {
        object->resource_sell.resource_limit[resource] = resource_limit;
    } else {
        object->resource_buy.resource_limit[resource] = resource_limit;
    }
}


void empire_object_city_set_trade_route_cost(empire_object *object, int trade_route_cost)
{
    object->trade_route_cost = trade_route_cost;
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
