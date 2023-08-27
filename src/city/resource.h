#ifndef CITY_RESOURCE_H
#define CITY_RESOURCE_H

#include "core/buffer.h"

#define RESOURCE_TYPES_MAX 16
#define FOOD_TYPES_MAX 7
#define COIN_IMAGE_ID 1202
#define EMPTY_WAREHOUSE_IMG_ID 3337
#define EMPTY_CART_IMG_ID 4650

#include <stdint.h>

enum {
    RESOURCE_NONE = 0,
    RESOURCE_WHEAT = 1,
    RESOURCE_VEGETABLES = 2,
    RESOURCE_FRUIT = 3,
    RESOURCE_MEAT = 4,
    RESOURCE_OLIVES = 5,
    RESOURCE_VINES = 6,
    RESOURCE_CLAY = 7,
    RESOURCE_TIMBER = 8,
    RESOURCE_MARBLE = 9,
    RESOURCE_IRON = 10,
    RESOURCE_OIL = 11,
    RESOURCE_WINE = 12,
    RESOURCE_POTTERY = 13,
    RESOURCE_FURNITURE = 14,
    RESOURCE_WEAPONS = 15,
    RESOURCE_DENARII = 16,
    RESOURCE_TROOPS = 17,
};

enum {
    INVENTORY_WHEAT = 0,
    INVENTORY_VEGETABLES = 1,
    INVENTORY_FRUIT = 2,
    INVENTORY_MEAT = 3,
    INVENTORY_OIL = 4,
    INVENTORY_WINE = 5,
    INVENTORY_POTTERY = 6,
    INVENTORY_FURNITURE = 7,
    // helper constants
    INVENTORY_MAX = 8
};

enum {
    WORKSHOP_NONE = 0,
    WORKSHOP_OLIVES_TO_OIL = 1,
    WORKSHOP_VINES_TO_WINE = 2,
    WORKSHOP_CLAY_TO_POTTERY = 3,
    WORKSHOP_TIMBER_TO_FURNITURE = 4,
    WORKSHOP_IRON_TO_WEAPONS = 5,
};

enum {
    RESOURCE_IMAGE_STORAGE = 0,
    RESOURCE_IMAGE_CART = 1,
    RESOURCE_IMAGE_FOOD_CART = 2,
    RESOURCE_IMAGE_ICON = 3
};

struct resource_list_t {
    int size;
    int items[RESOURCE_TYPES_MAX];
};

struct resource_img_ids_t {
    int icon_img_id;
    int editor_icon_img_id;
    int warehouse_space_img_id;
    int cart_img_id;
    int farm_field_img_id;
    int empire_icon_img_id;
    int editor_empire_icon_img_id;
};

extern struct resource_img_ids_t resource_images[RESOURCE_TYPES_MAX];

extern char *resource_strings[];

struct trade_price_t {
    uint16_t buy;
    uint16_t sell;
};

extern struct trade_price_t DEFAULT_PRICES[RESOURCE_TYPES_MAX];
extern struct trade_price_t trade_prices[RESOURCE_TYPES_MAX];

int resource_image_offset(int resource, int type);

int resource_is_food(int resource);

int resource_to_workshop_type(int resource);

struct resource_list_t *city_resource_get_available(void);

struct resource_list_t *city_resource_get_available_foods(void);

int city_resource_multiple_wine_available(void);

void city_resource_cycle_trade_status(int resource);

void city_resource_toggle_stockpiled(int resource);

void city_resource_add_to_warehouse(int resource, int amount);
void city_resource_remove_from_warehouse(int resource, int amount);
void city_resource_calculate_warehouse_stocks(void);

void city_resource_determine_available(void);

void city_resource_calculate_food_stocks_and_supply_wheat(void);

void city_resource_calculate_workshop_stocks(void);

void city_resource_consume_food(void);

int trade_price_change(int resource, int amount);

void trade_prices_save_state(struct buffer_t *buf);

void trade_prices_load_state(struct buffer_t *buf);

#endif // CITY_RESOURCE_H
