#include "object.h"

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

static full_empire_object objects[MAX_OBJECTS];

static int is_sea_trade_route(int route_id);

static void fix_image_ids(void)
{
    int image_id = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use
            && objects[i].obj.type == EMPIRE_OBJECT_CITY
            && objects[i].city_type == EMPIRE_CITY_OURS) {
            image_id = objects[i].obj.image_id;
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
            if (objects[i].obj.image_id) {
                objects[i].obj.image_id += offset;
                if (objects[i].obj.expanded.image_id) {
                    objects[i].obj.expanded.image_id += offset;
                }
            }
        }
    }
}


void empire_object_load_initial(buffer *buf)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        full_empire_object *full = &objects[i];
        empire_object *obj = &full->obj;
        obj->id = i;
        obj->type = buffer_read_u8(buf);
        full->in_use = buffer_read_u8(buf);
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
        full->city_type = buffer_read_u8(buf);
        full->city_name_id = buffer_read_u8(buf);
        obj->trade_route_id = buffer_read_u8(buf);
        full->trade_route_open = buffer_read_u8(buf);
        full->trade_route_cost = buffer_read_i16(buf);
        for (int r = 0; r < 10; r++) {
            full->city_sells_resource.resource_sell[r] = buffer_read_u8(buf);
        }
        buffer_skip(buf, 2);
        for (int r = 0; r < 8; r++) {
            full->city_buys_resource.resource_buy[r] = buffer_read_u8(buf);
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
        full_empire_object *full = &objects[i];
        empire_object *obj = &full->obj;
        obj->id = i;
        obj->type = buffer_read_u8(buf);
        obj->animation_index = buffer_read_u8(buf);
        obj->x = buffer_read_i16(buf);
        obj->y = buffer_read_i16(buf);
        obj->width = buffer_read_i16(buf);
        obj->height = buffer_read_i16(buf);
        obj->image_id = buffer_read_i16(buf);
        obj->expanded.x = buffer_read_i16(buf);
        obj->expanded.y = buffer_read_i16(buf);
        obj->expanded.image_id = buffer_read_i16(buf);
        obj->distant_battle_travel_months = buffer_read_u8(buf);
        obj->trade_route_id = buffer_read_u8(buf);
        obj->invasion_path_id = buffer_read_u8(buf);
        obj->invasion_years = buffer_read_u8(buf);
        full->in_use = buffer_read_u8(buf);
        full->city_type = buffer_read_u8(buf);
        full->city_name_id = buffer_read_u8(buf);
        full->trade_route_open = buffer_read_u8(buf);
        full->trade_route_cost = buffer_read_u32(buf);
        for (int r = 0; r < RESOURCE_MAX; r++) {
            full->city_sells_resource.resource_sell[r] = buffer_read_u8(buf);
            full->city_sells_resource.resource_sell_limit[r] = buffer_read_u8(buf);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            full->city_buys_resource.resource_buy[r] = buffer_read_u8(buf);
            full->city_buys_resource.resource_buy_limit[r] = buffer_read_u8(buf);
        }
    }
    fix_image_ids();
}

void empire_object_save_state(buffer *buf)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        full_empire_object *full = &objects[i];
        empire_object *obj = &full->obj;
        obj->id = i;
        buffer_write_u8(buf, obj->type);
        buffer_write_u8(buf, obj->animation_index);
        buffer_write_i16(buf, obj->x);
        buffer_write_i16(buf, obj->y);
        buffer_write_i16(buf, obj->width);
        buffer_write_i16(buf, obj->height);
        buffer_write_i16(buf, obj->image_id);
        buffer_write_i16(buf, obj->expanded.x);
        buffer_write_i16(buf, obj->expanded.y);
        buffer_write_i16(buf, obj->expanded.image_id);
        buffer_write_u8(buf, obj->distant_battle_travel_months);
        buffer_write_u8(buf, obj->trade_route_id);
        buffer_write_u8(buf, obj->invasion_path_id);
        buffer_write_u8(buf, obj->invasion_years);
        buffer_write_u8(buf, full->in_use);
        buffer_write_u8(buf, full->city_type);
        buffer_write_u8(buf, full->city_name_id);
        buffer_write_u8(buf, full->trade_route_open);
        buffer_write_u32(buf, full->trade_route_cost);
        for (int r = 0; r < RESOURCE_MAX; r++) {
            buffer_write_u8(buf, full->city_sells_resource.resource_sell[r]);
            buffer_write_u8(buf, full->city_sells_resource.resource_sell_limit[r]);
        }
        for (int r = 0; r < RESOURCE_MAX; r++) {
            buffer_write_u8(buf, full->city_buys_resource.resource_buy[r]);
            buffer_write_u8(buf, full->city_buys_resource.resource_buy_limit[r]);
        }
    }
}

void empire_object_init_cities(void)
{
    empire_city_clear_all();
    int route_index = 1;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!objects[i].in_use || objects[i].obj.type != EMPIRE_OBJECT_CITY) {
            continue;
        }
        full_empire_object *obj = &objects[i];
        empire_city *city = empire_city_get(route_index++);
        city->in_use = 1;
        city->type = obj->city_type;
        city->name_id = obj->city_name_id;
        if (obj->obj.trade_route_id < 0) {
            obj->obj.trade_route_id = 0;
        }
        if (obj->obj.trade_route_id >= 20) {
            obj->obj.trade_route_id = 19;
        }
        city->route_id = obj->obj.trade_route_id;
        city->is_open = obj->trade_route_open;
        city->cost_to_open = obj->trade_route_cost;
        city->is_sea_trade = is_sea_trade_route(obj->obj.trade_route_id);

        for (int resource = RESOURCE_MIN; resource < RESOURCE_MAX; resource++) {
            city->sells_resource[resource] = 0;
            city->buys_resource[resource] = 0;
            if (city->type != EMPIRE_CITY_OURS && city->type != EMPIRE_CITY_TRADE) {
                continue;
            }
            if (empire_object_city_sells_resource(i, resource)) {
                city->sells_resource[resource] = 1;
                trade_route_init(city->route_id, resource, obj->city_sells_resource.resource_sell_limit[resource]);
            }
            if (empire_object_city_buys_resource(i, resource)) {
                city->buys_resource[resource] = 1;
                trade_route_init(city->route_id, resource, obj->city_buys_resource.resource_buy_limit[resource]);
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
        if (objects[i].in_use && objects[i].obj.type == object_type) {
            month++;
            objects[i].obj.distant_battle_travel_months = month;
        }
    }
    return month;
}

const empire_object *empire_object_get(int object_id)
{
    return &objects[object_id].obj;
}

full_empire_object *empire_object_get_our_city(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use) {
            if (objects[i].obj.type == EMPIRE_OBJECT_CITY && objects[i].city_type == EMPIRE_CITY_OURS) {
                return &objects[i];
            }
        }
    }
    return 0;
}


void empire_object_foreach(void (*callback)(const empire_object *))
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use) {
            callback(&objects[i].obj);
        }
    }
}

const empire_object *empire_object_get_battle_icon(int path_id, int year)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use) {
            empire_object *obj = &objects[i].obj;
            if (obj->type == EMPIRE_OBJECT_BATTLE_ICON &&
                obj->invasion_path_id == path_id && obj->invasion_years == year) {
                return obj;
            }
        }
    }
    return 0;
}

int empire_object_get_max_invasion_path(void)
{
    int max_path = 0;
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use && objects[i].obj.type == EMPIRE_OBJECT_BATTLE_ICON) {
            if (objects[i].obj.invasion_path_id > max_path) {
                max_path = objects[i].obj.invasion_path_id;
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
        const empire_object *obj = &objects[i].obj;
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
        objects[object_id].obj.expanded.image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
    } else if (new_city_type == EMPIRE_CITY_DISTANT_ROMAN) {
        objects[object_id].obj.expanded.image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
    }
}

int empire_object_city_buys_resource(int object_id, int resource)
{
    const full_empire_object *object = &objects[object_id];
    return object->city_buys_resource.resource_buy[resource];
}

int empire_object_city_sells_resource(int object_id, int resource)
{
    const full_empire_object *object = &objects[object_id];
    return object->city_sells_resource.resource_sell[resource];
}


void empire_object_our_city_set_resources_sell(void)
{
    full_empire_object *our_city = empire_object_get_our_city();
    for (int resource = 0; resource < RESOURCE_MAX; resource++) {
        // farms match across enums, rest don't
        if (resource <= RESOURCE_VINES) {
            if (scenario_editor_is_building_allowed(resource + ALLOWED_BUILDING_WHEAT_FARM - 1))
                our_city->city_sells_resource.resource_sell[resource] = resource;
            else {
                our_city->city_sells_resource.resource_sell[resource] = 0;
            }
        }
        if (resource == RESOURCE_MEAT) {
            if (scenario_editor_is_building_allowed(ALLOWED_BUILDING_WHARF)
                || scenario_editor_is_building_allowed(ALLOWED_BUILDING_PIG_FARM)
            ) {
                our_city->city_sells_resource.resource_sell[resource] = resource;
            } else {
                our_city->city_sells_resource.resource_sell[resource] = 0;
            }
        }
        if (resource >= RESOURCE_WINE && resource <= RESOURCE_OIL) {
            if (scenario_editor_is_building_allowed(resource + ALLOWED_BUILDING_WINE_WORKSHOP - 7))
                our_city->city_sells_resource.resource_sell[resource] = resource;
            else {
                our_city->city_sells_resource.resource_sell[resource] = 0;
            }
        }
        if (resource >= RESOURCE_IRON && resource <= RESOURCE_TIMBER) {
            if (scenario_editor_is_building_allowed(resource + ALLOWED_BUILDING_IRON_MINE - 9))
                our_city->city_sells_resource.resource_sell[resource] = resource;
            else {
                our_city->city_sells_resource.resource_sell[resource] = 0;
            }
        }
        if (resource >= RESOURCE_CLAY && resource <= RESOURCE_MARBLE) {
            if (scenario_editor_is_building_allowed(resource + ALLOWED_BUILDING_CLAY_PIT - 11))
                our_city->city_sells_resource.resource_sell[resource] = resource;
            else {
                our_city->city_sells_resource.resource_sell[resource] = 0;
            }
        }
        if (resource >= RESOURCE_WEAPONS) {
            if (scenario_editor_is_building_allowed(resource + ALLOWED_BUILDING_WEAPONS_WORKSHOP - 13))
                our_city->city_sells_resource.resource_sell[resource] = resource;
            else {
                our_city->city_sells_resource.resource_sell[resource] = 0;
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
            objects[i].city_sells_resource.resource_sell[r] = 0;
            objects[i].city_sells_resource.resource_sell_limit[r] = 0;
            objects[i].city_buys_resource.resource_buy[r] = 0;
            objects[i].city_buys_resource.resource_buy_limit[r] = 0;
        }
    }
}


void empire_object_city_toggle_resource(int object_id, int resource, int selling)
{
    full_empire_object *object = &objects[object_id];
    if (selling) {
        if (object->city_sells_resource.resource_sell[resource]) {
            object->city_sells_resource.resource_sell[resource] = 0;
        } else {
            object->city_sells_resource.resource_sell[resource] = resource;
            // don't allow simultaneous selling and buying of the same resource
            if (object->city_buys_resource.resource_buy[resource]) {
                object->city_buys_resource.resource_buy[resource] = 0;
            }
        }
    } else {
        if (object->city_buys_resource.resource_buy[resource]) {
            object->city_buys_resource.resource_buy[resource] = 0;
        } else {
            object->city_buys_resource.resource_buy[resource] = resource;
            // don't allow simultaneous selling and buying of the same resource
            if (object->city_sells_resource.resource_sell[resource]) {
                object->city_sells_resource.resource_sell[resource] = 0;
            }
        }
    }

}


void empire_object_city_set_resource_limit(int object_id, int resource, int resource_limit, int selling)
{
    full_empire_object *object = &objects[object_id];
    if (selling) {
        object->city_sells_resource.resource_sell_limit[resource] = resource_limit;
    } else {
        object->city_buys_resource.resource_buy_limit[resource] = resource_limit;
    }
}


void empire_object_city_set_trade_route_cost(int object_id, int trade_route_cost)
{
    full_empire_object *object = &objects[object_id];
    object->trade_route_cost = trade_route_cost;
}


static int is_sea_trade_route(int route_id)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (objects[i].in_use && objects[i].obj.trade_route_id == route_id) {
            if (objects[i].obj.type == EMPIRE_OBJECT_SEA_TRADE_ROUTE) {
                return 1;
            }
            if (objects[i].obj.type == EMPIRE_OBJECT_LAND_TRADE_ROUTE) {
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
    return objects[obj->id].obj.animation_index = get_animation_offset(image_id, obj->animation_index);
}
