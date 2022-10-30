#include "empire.h"

#include "building/count.h"
#include "city/constants.h"
#include "city/data_private.h"
#include "city/population.h"
#include "city/resource.h"
#include "core/calc.h"
#include "core/log.h"
#include "core/io.h"
#include "empire/object.h"
#include "empire/trade_route.h"

#include <string.h>

enum {
    EMPIRE_WIDTH = 2000,
    EMPIRE_HEIGHT = 1000,
    EMPIRE_HEADER_SIZE = 1280,
    EMPIRE_DATA_SIZE = 12800
};

static struct {
    int initial_scroll_x;
    int initial_scroll_y;
    int scroll_x;
    int scroll_y;
    empire_object *selected_object;
    int viewport_width;
    int viewport_height;
} data;


void empire_load(int empire_id)
{
    char raw_data[EMPIRE_DATA_SIZE];
    const char *filename = "c32.emp";

    // read header with scroll positions
    if (!io_read_file_part_into_buffer(filename, raw_data, 4, 32 * empire_id)) {
        memset(raw_data, 0, 4);
    }
    buffer buf;
    buffer_init(&buf, raw_data, 4);
    data.initial_scroll_x = buffer_read_i16(&buf);
    data.initial_scroll_y = buffer_read_i16(&buf);

    // read data section with objects
    int offset = EMPIRE_HEADER_SIZE + EMPIRE_DATA_SIZE * empire_id;
    int read_size = io_read_file_part_into_buffer(filename, raw_data, EMPIRE_DATA_SIZE, offset);
    if (read_size != EMPIRE_DATA_SIZE) {
        // load empty empire when loading fails
        log_error("Unable to load empire data from file", filename, 0);
        memset(raw_data, 0, EMPIRE_DATA_SIZE);
    }
    buffer_init(&buf, raw_data, EMPIRE_DATA_SIZE);
    empire_object_load_initial(&buf);
}

static void check_scroll_boundaries(void)
{
    int max_x = EMPIRE_WIDTH - data.viewport_width;
    int max_y = EMPIRE_HEIGHT - data.viewport_height;

    data.scroll_x = calc_bound(data.scroll_x, 0, max_x);
    data.scroll_y = calc_bound(data.scroll_y, 0, max_y);
}

void empire_load_editor(int empire_id, int viewport_width, int viewport_height)
{
    empire_load(empire_id);
    empire_object_init_cities();
    empire_object_our_city_set_resources_sell();
    empire_object_trade_cities_disable_default_resources();

    empire_object *our_city = empire_object_get_our_city();

    data.viewport_width = viewport_width;
    data.viewport_height = viewport_height;
    if (our_city) {
        data.scroll_x = our_city->x - data.viewport_width / 2;
        data.scroll_y = our_city->y - data.viewport_height / 2;
    } else {
        data.scroll_x = data.initial_scroll_x;
        data.scroll_y = data.initial_scroll_y;
    }
    check_scroll_boundaries();
}

void empire_init_scenario(void)
{
    data.scroll_x = data.initial_scroll_x;
    data.scroll_y = data.initial_scroll_y;
    data.viewport_width = EMPIRE_WIDTH;
    data.viewport_height = EMPIRE_HEIGHT;

    empire_object_init_cities();
}

void empire_set_viewport(int width, int height)
{
    data.viewport_width = width;
    data.viewport_height = height;
    check_scroll_boundaries();
}

void empire_adjust_scroll(int *x_offset, int *y_offset)
{
    *x_offset = *x_offset - data.scroll_x;
    *y_offset = *y_offset - data.scroll_y;
}

void empire_scroll_map(int x, int y)
{
    data.scroll_x += x;
    data.scroll_y += y;
    check_scroll_boundaries();
}


empire_object *empire_select_object(int x, int y)
{
    int map_x = x + data.scroll_x;
    int map_y = y + data.scroll_y;
    int closest_object_id = empire_object_get_closest(map_x, map_y) - 1;
    // -1 here means "nothing selected" because the first element (0) is a city/not empty
    return (closest_object_id == -1) ? 0 : empire_object_get(closest_object_id);
}

int empire_can_export_resource_to_city(int city_id, int resource)
{
    empire_object *city = empire_object_get(city_id);
    if (city_id && trade_route_limit_reached(city->trade_route_id, resource)) {
        // quota reached
        return 0;
    }
    if (city_data.resource.stored_in_warehouses[resource] <= city_data.resource.export_over[resource]) {
        // stocks too low
        return 0;
    }
    if (city_id == 0 || city->resources_buy_list.resource[resource]) {
        return city_data.resource.trade_status[resource] == TRADE_STATUS_EXPORT;
    } else {
        return 0;
    }
}

static int get_max_stock_for_population(void)
{
    int population = city_population();
    if (population < 2000) {
        return 10;
    } else if (population < 4000) {
        return 20;
    } else if (population < 6000) {
        return 30;
    } else {
        return 40;
    }
}

int empire_can_import_resource_from_city(int city_id, int resource)
{
    empire_object *city = empire_object_get(city_id);
    if (!city->resources_sell_list.resource[resource]) {
        return 0;
    }
    if (city_data.resource.trade_status[resource] != TRADE_STATUS_IMPORT) {
        return 0;
    }
    if (trade_route_limit_reached(city->trade_route_id, resource)) {
        return 0;
    }

    int in_stock = city_data.resource.stored_in_warehouses[resource];
    int max_in_stock = 0;
    int finished_good = RESOURCE_NONE;
    switch (resource) {
        // food and finished materials
        case RESOURCE_WHEAT:
        case RESOURCE_VEGETABLES:
        case RESOURCE_FRUIT:
        case RESOURCE_MEAT:
        case RESOURCE_POTTERY:
        case RESOURCE_FURNITURE:
        case RESOURCE_OIL:
        case RESOURCE_WINE:
            max_in_stock = get_max_stock_for_population();
            break;

        case RESOURCE_MARBLE:
        case RESOURCE_WEAPONS:
            max_in_stock = 10;
            break;

        case RESOURCE_CLAY:
            finished_good = RESOURCE_POTTERY;
            break;
        case RESOURCE_TIMBER:
            finished_good = RESOURCE_FURNITURE;
            break;
        case RESOURCE_OLIVES:
            finished_good = RESOURCE_OIL;
            break;
        case RESOURCE_VINES:
            finished_good = RESOURCE_WINE;
            break;
        case RESOURCE_IRON:
            finished_good = RESOURCE_WEAPONS;
            break;
    }
    if (finished_good) {
        max_in_stock = 2 + 2 * building_count_industry_active(finished_good);
    }
    return in_stock < max_in_stock ? 1 : 0;
}

void empire_save_state(buffer *buf)
{
    buffer_write_i32(buf, data.scroll_x);
    buffer_write_i32(buf, data.scroll_y);
}

void empire_load_state(buffer *buf)
{
    data.scroll_x = buffer_read_i32(buf);
    data.scroll_y = buffer_read_i32(buf);
}
