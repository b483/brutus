#ifndef EMPIRE_OBJECT_H
#define EMPIRE_OBJECT_H

#include "core/buffer.h"
#include "game/resource.h"

typedef struct {
    int id;
    int type;
    int animation_index;
    int x;
    int y;
    int width;
    int height;
    int image_id;
    struct {
        int x;
        int y;
        int image_id;
    } expanded;
    int distant_battle_travel_months;
    int trade_route_id;
    int invasion_path_id;
    int invasion_years;
} empire_object;

typedef struct {
    int in_use;
    int city_type;
    int city_name_id;
    int trade_route_open;
    int trade_route_cost;
    struct {
        int resource_sell[RESOURCE_MAX];
        int resource_sell_limit[RESOURCE_MAX];
    } city_sells_resource;
    struct {
        int resource_buy[RESOURCE_MAX];
        int resource_buy_limit[RESOURCE_MAX];
    } city_buys_resource;
    empire_object obj;
} full_empire_object;

// loads empire for new maps and on empire state change
void empire_object_load_initial(buffer *buf);

// loads empire map for custom maps
void empire_object_load_state(buffer *buf);
// saves empire map for custom maps
void empire_object_save_state(buffer *buf);

void empire_object_init_cities(void);

int empire_object_init_distant_battle_travel_months(int object_type);

const empire_object *empire_object_get(int object_id);

full_empire_object *empire_object_get_our_city(void);

void empire_object_foreach(void (*callback)(const empire_object *));

const empire_object *empire_object_get_battle_icon(int path_id, int year);

int empire_object_get_max_invasion_path(void);

int empire_object_get_closest(int x, int y);

void empire_object_set_expanded(int object_id, int new_city_type);

/**
 * @param object_id id for empire object
 * @param resource resource type to check for
 * @return the resource id if enabled, else 0
 */
int empire_object_city_buys_resource(int object_id, int resource);

/**
 * @param object_id id for empire object
 * @param resource resource type to check for
 * @return the resource id if enabled, else 0
 */
int empire_object_city_sells_resource(int object_id, int resource);

// sets all of full_empire_object->city_sells_resource for our city based on allowed buildings in the editor
void empire_object_our_city_set_resources_sell(void);

// disables default resources that trade cities sell/buy
void empire_object_trade_cities_disable_default_resources(void);

// toggle resource sell/buy status for empire city trade
void empire_object_city_toggle_resource(int object_id, int resource, int selling);

/**
 * Sets sell/buy limit (amount) for given resource
 * @param object_id id for empire object
 * @param resource resource type to set value for
 * @param resource_limit trade limit to set for given resource type
 * @param selling whether it's a resource to sell (1) or buy (0) from the trade city perspective
 */
void empire_object_city_set_resource_limit(int object_id, int resource, int resource_limit, int selling);

int empire_object_update_animation(const empire_object *obj, int image_id);

#endif // EMPIRE_OBJECT_H
