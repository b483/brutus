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
    int city_sells_resource[RESOURCE_MAX];
    int city_buys_resource[RESOURCE_MAX];
    int trade40;
    int trade25;
    int trade15;
    empire_object obj;
} full_empire_object;

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

int empire_object_city_buys_resource(int object_id, int resource);
int empire_object_city_sells_resource(int object_id, int resource);

// sets all of full_empire_object->city_sells_resource for our city based on allowed buildings in the editor
void empire_object_our_city_set_resources_sell(void);

int empire_object_update_animation(const empire_object *obj, int image_id);

#endif // EMPIRE_OBJECT_H
