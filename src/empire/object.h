#ifndef EMPIRE_OBJECT_H
#define EMPIRE_OBJECT_H

#include "core/buffer.h"
#include "city/resource.h"

#define MAX_OBJECTS 200

enum {
    EMPIRE_OBJECT_ORNAMENT = 0,
    EMPIRE_OBJECT_CITY = 1,
    EMPIRE_OBJECT_BATTLE_ICON = 3,
    EMPIRE_OBJECT_LAND_TRADE_ROUTE = 4,
    EMPIRE_OBJECT_SEA_TRADE_ROUTE = 5,
    EMPIRE_OBJECT_ROMAN_ARMY = 6,
    EMPIRE_OBJECT_ENEMY_ARMY = 7,
};

enum {
    EMPIRE_CITY_DISTANT_ROMAN = 0,
    EMPIRE_CITY_OURS = 1,
    EMPIRE_CITY_TRADE = 2,
    EMPIRE_CITY_FUTURE_TRADE = 3,
    EMPIRE_CITY_DISTANT_FOREIGN = 4,
    EMPIRE_CITY_VULNERABLE_ROMAN = 5,
    EMPIRE_CITY_FUTURE_ROMAN = 6,
};

struct empire_object_t {
    int32_t id;
    uint8_t type;
    int16_t x;
    int16_t y;
    int16_t image_id;
    struct {
        int16_t x;
        int16_t y;
        int16_t image_id;
    } expanded;
    int16_t width;
    int16_t height;
    uint8_t animation_index;
    uint8_t in_use;
    uint8_t city_type;
    uint8_t city_name_id;
    uint8_t trade_route_id;
    uint8_t trade_route_open;
    uint32_t trade_route_cost;
    int16_t trader_entry_delay;
    uint8_t is_sea_trade;
    int16_t trader_figure_ids[3];
    uint8_t resource_buy_limit[RESOURCE_TYPES_MAX];
    uint8_t resource_sell_limit[RESOURCE_TYPES_MAX];
    uint8_t resource_bought[RESOURCE_TYPES_MAX];
    uint8_t resource_sold[RESOURCE_TYPES_MAX];
    uint8_t invasion_path_id;
    uint8_t invasion_years;
    uint8_t distant_battle_travel_months;
};

extern struct empire_object_t empire_objects[MAX_OBJECTS];

// loads empire for new maps and on empire state change
void empire_object_load_initial(struct buffer_t *buf);

// loads empire map for custom maps
void empire_object_load_state(struct buffer_t *buf);
// saves empire map for custom maps
void empire_object_save_state(struct buffer_t *buf);

int empire_object_init_distant_battle_travel_months(int object_type);

struct empire_object_t *get_trade_city_by_trade_route(int trade_route_id);

int empire_object_get_closest(int x, int y);

struct empire_object_t *empire_object_get_our_city(void);

int empire_object_update_animation(struct empire_object_t *obj, int image_id);

// sets all resources to sell for our city based on allowed buildings in the editor
void empire_object_our_city_set_resources_sell(void);

int resource_import_trade_route_open(int resource);
int resource_export_trade_route_open(int resource);
int can_export_resource_to_trade_city(int city_id, int resource);
int can_import_resource_from_trade_city(int city_id, int resource);

int our_city_can_produce_resource(int resource);

int empire_can_produce_resource(int resource);

#endif // EMPIRE_OBJECT_H
