#include "resource.h"

#include "building/building.h"
#include "building/industry.h"
#include "city/data.h"
#include "core/calc.h"
#include "empire/object.h"
#include "map/road_access.h"
#include "scenario/data.h"

struct resource_img_ids_t resource_images[RESOURCE_TYPES_MAX] = {
{0, 0, 0, 0, -1, 0, 0}, // RESOURCE_NONE
{1187, 1190, 3338, 8, 0, 7983, 3233}, // RESOURCE_WHEAT
{1188, 1191, 3342, 16, 5, 7984, 3234}, // RESOURCE_VEGETABLES
{1189, 1192, 3346, 24, 10, 7985, 3235}, // RESOURCE_FRUIT
{1192, 1195, 3358, 48, 25, 7988, 3238}, // RESOURCE_MEAT
{1190, 1193, 3350, 32, 15, 7986, 3236}, // RESOURCE_OLIVES
{1191, 1194, 3354, 40, 20, 7987, 3237}, // RESOURCE_VINES
{1197, 1200, 3378, 88, -1, 7993, 3243}, // RESOURCE_CLAY
{1196, 1199, 3366, 80, -1, 7992, 3242}, // RESOURCE_TIMBER
{1198, 1201, 3382, 96, -1, 7994, 3244}, // RESOURCE_MARBLE
{1195, 1198, 3370, 72, -1, 7991, 3241}, // RESOURCE_IRON
{1194, 1197, 3366, 64, -1, 7990, 3240}, // RESOURCE_OIL
{1193, 1196, 3362, 56, -1, 7989, 3239}, // RESOURCE_WINE
{1201, 1204, 3394, 120, -1, 7997, 3247}, // RESOURCE_POTTERY
{1200, 1203, 3390, 112, -1, 7996, 3246}, // RESOURCE_FURNITURE
{1199, 1202, 3386, 104, -1, 7995, 3245}, // RESOURCE_WEAPONS
};

char *resource_strings[] = {
"No resource", // RESOURCE_NONE
"Wheat", // RESOURCE_WHEAT
"Vegetables", // RESOURCE_VEGETABLES
"Fruit", // RESOURCE_FRUIT
"Meat", // RESOURCE_MEAT
"Olives", // RESOURCE_OLIVES
"Vines", // RESOURCE_VINES
"Clay", // RESOURCE_CLAY
"Timber", // RESOURCE_TIMBER
"Marble", // RESOURCE_MARBLE
"Iron", // RESOURCE_IRON
"Oil", // RESOURCE_OIL
"Wine", // RESOURCE_WINE
"Pottery", // RESOURCE_POTTERY
"Furniture", // RESOURCE_FURNITURE
"Weapons", // RESOURCE_WEAPONS
"Denarii" // RESOURCE_DENARII
};

struct trade_price_t DEFAULT_PRICES[RESOURCE_TYPES_MAX] = {
    {0, 0}, // RESOURCE_NONE
    {28, 22}, // RESOURCE_WHEAT
    {38, 30}, // RESOURCE_VEGETABLES
    {38, 30}, // RESOURCE_FRUIT
    {44, 36}, // RESOURCE_MEAT
    {42, 34}, // RESOURCE_OLIVES
    {44, 36}, // RESOURCE_VINES
    {40, 30}, // RESOURCE_CLAY
    {50, 35}, // RESOURCE_TIMBER
    {200, 140}, // RESOURCE_MARBLE
    {60, 40}, // RESOURCE_IRON
    {180, 140}, // RESOURCE_OIL
    {215, 160}, // RESOURCE_WINE
    {180, 140}, // RESOURCE_POTTERY
    {200, 150}, // RESOURCE_FURNITURE
    {250, 180}, // RESOURCE_WEAPONS
};

struct trade_price_t trade_prices[RESOURCE_TYPES_MAX];

int resource_image_offset(int resource, int type)
{
    if (resource == RESOURCE_MEAT && scenario.allowed_buildings[BUILDING_WHARF]) {
        switch (type) {
            case RESOURCE_IMAGE_STORAGE: return 40;
            case RESOURCE_IMAGE_CART: return 648;
            case RESOURCE_IMAGE_FOOD_CART: return 8;
            case RESOURCE_IMAGE_ICON: return 11;
            default: return 0;
        }
    } else {
        return 0;
    }
}

int resource_is_food(int resource)
{
    return resource == RESOURCE_WHEAT || resource == RESOURCE_VEGETABLES ||
        resource == RESOURCE_FRUIT || resource == RESOURCE_MEAT;
}

int resource_to_workshop_type(int resource)
{
    switch (resource) {
        case RESOURCE_OLIVES:
            return WORKSHOP_OLIVES_TO_OIL;
        case RESOURCE_VINES:
            return WORKSHOP_VINES_TO_WINE;
        case RESOURCE_CLAY:
            return WORKSHOP_CLAY_TO_POTTERY;
        case RESOURCE_TIMBER:
            return WORKSHOP_TIMBER_TO_FURNITURE;
        case RESOURCE_IRON:
            return WORKSHOP_IRON_TO_WEAPONS;
        default:
            return WORKSHOP_NONE;
    }
}

static struct {
    struct resource_list_t resource_list;
    struct resource_list_t food_list;
} available;

struct resource_list_t *city_resource_get_available(void)
{
    return &available.resource_list;
}

struct resource_list_t *city_resource_get_available_foods(void)
{
    return &available.food_list;
}

int city_resource_multiple_wine_available(void)
{
    return city_data.resource.wine_types_available > 1;
}

void city_resource_cycle_trade_status(int resource)
{
    ++city_data.resource.trade_status[resource];
    if (city_data.resource.trade_status[resource] > TRADE_STATUS_EXPORT) {
        city_data.resource.trade_status[resource] = TRADE_STATUS_NONE;
    }

    if (city_data.resource.trade_status[resource] == TRADE_STATUS_IMPORT &&
        !resource_import_trade_route_open(resource)) {
        city_data.resource.trade_status[resource] = TRADE_STATUS_EXPORT;
    }
    if (city_data.resource.trade_status[resource] == TRADE_STATUS_EXPORT &&
        !resource_export_trade_route_open(resource)) {
        city_data.resource.trade_status[resource] = TRADE_STATUS_NONE;
    }
    if (city_data.resource.trade_status[resource] == TRADE_STATUS_EXPORT) {
        city_data.resource.stockpiled[resource] = 0;
    }
}

void city_resource_toggle_stockpiled(int resource)
{
    if (city_data.resource.stockpiled[resource]) {
        city_data.resource.stockpiled[resource] = 0;
    } else {
        city_data.resource.stockpiled[resource] = 1;
        if (city_data.resource.trade_status[resource] == TRADE_STATUS_EXPORT) {
            city_data.resource.trade_status[resource] = TRADE_STATUS_NONE;
        }
    }
}

void city_resource_add_to_warehouse(int resource, int amount)
{
    city_data.resource.space_in_warehouses[resource] -= amount;
    city_data.resource.stored_in_warehouses[resource] += amount;
}

void city_resource_remove_from_warehouse(int resource, int amount)
{
    city_data.resource.space_in_warehouses[resource] += amount;
    city_data.resource.stored_in_warehouses[resource] -= amount;
}

void city_resource_calculate_warehouse_stocks(void)
{
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        city_data.resource.space_in_warehouses[i] = 0;
        city_data.resource.stored_in_warehouses[i] = 0;
    }
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_WAREHOUSE) {
            b->has_road_access = 0;
            if (map_has_road_access(b->x, b->y, b->size, 0)) {
                b->has_road_access = 1;
            } else if (map_has_road_access(b->x, b->y, 3, 0)) {
                b->has_road_access = 2;
            }
        }
    }
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_WAREHOUSE_SPACE) {
            continue;
        }
        struct building_t *warehouse = building_main(b);
        if (warehouse->has_road_access) {
            b->has_road_access = warehouse->has_road_access;
            if (b->subtype.warehouse_resource_id) {
                int loads = b->loads_stored;
                int resource = b->subtype.warehouse_resource_id;
                city_data.resource.stored_in_warehouses[resource] += loads;
                city_data.resource.space_in_warehouses[resource] += 4 - loads;
            } else {
                city_data.resource.space_in_warehouses[RESOURCE_NONE] += 4;
            }
        }
    }
}

void city_resource_determine_available(void)
{
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        available.resource_list.items[i] = 0;
        available.food_list.items[i] = 0;
    }
    available.resource_list.size = 0;
    available.food_list.size = 0;

    for (int i = RESOURCE_WHEAT; i < RESOURCE_TYPES_MAX; i++) {
        if (empire_can_produce_resource(i)) {
            available.resource_list.items[available.resource_list.size++] = i;
        }
    }
    for (int i = RESOURCE_WHEAT; i <= RESOURCE_MEAT; i++) {
        if (empire_can_produce_resource(i)) {
            available.food_list.items[available.food_list.size++] = i;
        }
    }
}

static void calculate_available_food(void)
{
    for (int i = 0; i < FOOD_TYPES_MAX; i++) {
        city_data.resource.granary_food_stored[i] = 0;
    }
    city_data.resource.granary_total_stored = 0;
    city_data.resource.food_types_available = 0;
    city_data.resource.food_supply_months = 0;
    city_data.resource.granaries.operating = 0;
    city_data.resource.granaries.understaffed = 0;
    city_data.resource.granaries.not_operating = 0;
    city_data.resource.granaries.not_operating_with_food = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_GRANARY) {
            continue;
        }
        b->has_road_access = 0;
        if (map_has_road_access_granary(b->x, b->y, 0)) {
            b->has_road_access = 1;
            int pct_workers = calc_percentage(
                b->num_workers, building_properties[b->type].n_laborers);
            if (pct_workers < 100) {
                city_data.resource.granaries.understaffed++;
            }
            int amount_stored = 0;
            for (int r = RESOURCE_WHEAT; r < FOOD_TYPES_MAX; r++) {
                amount_stored += b->data.granary.resource_stored[r];
            }
            if (pct_workers < 50) {
                city_data.resource.granaries.not_operating++;
                if (amount_stored > 0) {
                    city_data.resource.granaries.not_operating_with_food++;
                }
            } else {
                city_data.resource.granaries.operating++;
                for (int r = 0; r < FOOD_TYPES_MAX; r++) {
                    city_data.resource.granary_food_stored[r] += b->data.granary.resource_stored[r];
                }
            }
        }
    }
    for (int i = RESOURCE_WHEAT; i < FOOD_TYPES_MAX; i++) {
        if (city_data.resource.granary_food_stored[i]) {
            city_data.resource.granary_total_stored += city_data.resource.granary_food_stored[i];
            city_data.resource.food_types_available++;
        }
    }
    city_data.resource.food_needed_per_month =
        calc_adjust_with_percentage(city_data.population.population, 50);
    if (city_data.resource.food_needed_per_month > 0) {
        city_data.resource.food_supply_months =
            city_data.resource.granary_total_stored / city_data.resource.food_needed_per_month;
    } else {
        city_data.resource.food_supply_months =
            city_data.resource.granary_total_stored > 0 ? 1 : 0;
    }
    if (scenario.rome_supplies_wheat) {
        city_data.resource.food_types_available = 1;
        city_data.resource.food_supply_months = 12;
    }
}

void city_resource_calculate_food_stocks_and_supply_wheat(void)
{
    calculate_available_food();
    if (scenario.rome_supplies_wheat) {
        for (int i = 1; i < MAX_BUILDINGS; i++) {
            struct building_t *b = &all_buildings[i];
            if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_MARKET) {
                b->data.market.inventory[INVENTORY_WHEAT] = 200;
            }
        }
    }
}

void city_resource_calculate_workshop_stocks(void)
{
    for (int i = 0; i < 6; i++) {
        city_data.resource.stored_in_workshops[i] = 0;
        city_data.resource.space_in_workshops[i] = 0;
    }
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || !building_is_workshop(b->type)) {
            continue;
        }
        b->has_road_access = 0;
        if (map_has_road_access(b->x, b->y, b->size, 0)) {
            b->has_road_access = 1;
            int room = 2 - b->loads_stored;
            if (room < 0) {
                room = 0;
            }
            int workshop_resource = b->subtype.workshop_type;
            city_data.resource.space_in_workshops[workshop_resource] += room;
            city_data.resource.stored_in_workshops[workshop_resource] += b->loads_stored;
        }
    }
}

void city_resource_consume_food(void)
{
    calculate_available_food();
    city_data.resource.food_types_eaten = 0;
    int total_consumed = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size) {
            int amount_per_type = calc_adjust_with_percentage(b->house_population, 50);
            if (house_properties[b->subtype.house_level].food_types > 1) {
                amount_per_type /= house_properties[b->subtype.house_level].food_types;
            }
            b->data.house.num_foods = 0;
            if (scenario.rome_supplies_wheat) {
                city_data.resource.food_types_eaten = 1;
                city_data.resource.food_types_available = 1;
                b->data.house.inventory[INVENTORY_WHEAT] = amount_per_type;
                b->data.house.num_foods = 1;
            } else if (house_properties[b->subtype.house_level].food_types > 0) {
                for (int t = INVENTORY_WHEAT; t <= INVENTORY_MEAT && b->data.house.num_foods < house_properties[b->subtype.house_level].food_types; t++) {
                    if (b->data.house.inventory[t] >= amount_per_type) {
                        b->data.house.inventory[t] -= amount_per_type;
                        b->data.house.num_foods++;
                        total_consumed += amount_per_type;
                    } else if (b->data.house.inventory[t]) {
                        // has food but not enough
                        b->data.house.inventory[t] = 0;
                        b->data.house.num_foods++;
                        total_consumed += amount_per_type;
                    }
                    if (b->data.house.num_foods > city_data.resource.food_types_eaten) {
                        city_data.resource.food_types_eaten = b->data.house.num_foods;
                    }
                }
            }
        }
    }
    city_data.resource.food_consumed_last_month = total_consumed;
    city_data.resource.food_produced_last_month = city_data.resource.food_produced_this_month;
    city_data.resource.food_produced_this_month = 0;
}

int trade_price_change(int resource, int amount)
{
    if (amount < 0 && trade_prices[resource].sell <= 0) {
        // cannot lower the price to negative
        return 0;
    }
    if (amount < 0 && trade_prices[resource].sell <= -amount) {
        trade_prices[resource].buy = 2;
        trade_prices[resource].sell = 0;
    } else {
        trade_prices[resource].buy += amount;
        trade_prices[resource].sell += amount;
    }
    return 1;
}

void trade_prices_save_state(struct buffer_t *buf)
{
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        buffer_write_u16(buf, trade_prices[i].buy);
        buffer_write_u16(buf, trade_prices[i].sell);
    }
}

void trade_prices_load_state(struct buffer_t *buf)
{
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        trade_prices[i].buy = buffer_read_u16(buf);
        trade_prices[i].sell = buffer_read_u16(buf);
    }
}