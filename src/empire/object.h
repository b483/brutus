#ifndef EMPIRE_OBJECT_H
#define EMPIRE_OBJECT_H

#include "core/buffer.h"
#include "game/resource.h"

typedef struct {
    int id;
    int type;
    int x;
    int y;
    int image_id;
    struct {
        int x;
        int y;
        int image_id;
    } expanded;
    int width;
    int height;
    int animation_index;
    int in_use;
    int city_type;
    int city_name_id;
    int trade_route_id;
    int trade_route_open;
    int trade_route_cost;
    struct {
        int resource[RESOURCE_MAX];
        int resource_limit[RESOURCE_MAX];
    } resource_sell;
    struct {
        int resource[RESOURCE_MAX];
        int resource_limit[RESOURCE_MAX];
    } resource_buy;
    int invasion_path_id;
    int invasion_years;
    int distant_battle_travel_months;
} empire_object;

// loads empire for new maps and on empire state change
void empire_object_load_initial(buffer *buf);

// loads empire map for custom maps
void empire_object_load_state(buffer *buf);
// saves empire map for custom maps
void empire_object_save_state(buffer *buf);

void empire_object_init_cities(void);

int empire_object_init_distant_battle_travel_months(int object_type);

empire_object *empire_object_get(int object_id);

empire_object *empire_object_get_for_trade_route(int trade_route_id);

empire_object *empire_object_get_our_city(void);

void empire_object_foreach(void (*callback)(const empire_object *));

const empire_object *empire_object_get_battle_icon(int path_id, int year);

int empire_object_get_max_invasion_path(void);

int empire_object_get_closest(int x, int y);

void empire_object_set_expanded(int object_id, int new_city_type);

// sets all resources to sell for our city based on allowed buildings in the editor
void empire_object_our_city_set_resources_sell(void);

void empire_object_open_trade(empire_object *object);

// changes city type to disable trade with city or postpone it (via empire expansion)
void empire_object_disable_postpone_trade_city(empire_object *object, int is_down);

// disables default resources that trade cities sell/buy
void empire_object_trade_cities_disable_default_resources(void);

// toggle resource sell/buy status for empire city trade
void empire_object_city_toggle_resource(empire_object *object, int resource, int selling);

/**
 * Sets sell/buy limit (amount) for given resource
 * @param object_id id for empire object
 * @param resource resource type to set value for
 * @param resource_limit trade limit to set for given resource type
 * @param selling whether it's a resource to sell (1) or buy (0) from the trade city perspective
 */
void empire_object_city_set_resource_limit(empire_object *object, int resource, int resource_limit, int selling);

/**
 * Sets the cost to open trade route with city
 * @param object_id id for empire object
 * @param trade_route_cost cost to open trade route
 */
void empire_object_city_set_trade_route_cost(empire_object *object, int trade_route_cost);

int empire_object_is_sea_trade_route(int route_id);

int empire_object_update_animation(const empire_object *obj, int image_id);

#endif // EMPIRE_OBJECT_H
