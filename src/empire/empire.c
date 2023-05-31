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
    struct empire_object_t *selected_object;
    int viewport_width;
    int viewport_height;
} data;

static void disable_trade_cities_initial_resources(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use
        && (empire_objects[i].city_type == EMPIRE_CITY_TRADE || empire_objects[i].city_type == EMPIRE_CITY_FUTURE_TRADE)) {
        }
        for (int r = 0; r < RESOURCE_TYPES_MAX; r++) {
            empire_objects[i].resource_buy_limit[r] = 0;
            empire_objects[i].resource_sell_limit[r] = 0;
        }
    }
}

void empire_load(int empire_id)
{
    char raw_data[EMPIRE_DATA_SIZE];
    const char *filename = "c32.emp";

    // read header with scroll positions
    if (!io_read_file_part_into_buffer(filename, raw_data, 4, 32 * empire_id)) {
        memset(raw_data, 0, 4);
    }
    struct buffer_t buf;
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
    disable_trade_cities_initial_resources();
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
    empire_object_our_city_set_resources_sell();

    struct empire_object_t *our_city = empire_object_get_our_city();

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

struct empire_object_t *empire_select_object(int x, int y)
{
    int map_x = x + data.scroll_x;
    int map_y = y + data.scroll_y;
    int closest_object_id = empire_object_get_closest(map_x, map_y) - 1;
    // -1 here means "nothing selected" because the first element (0) is a city/not empty
    return (closest_object_id == -1) ? 0 : &empire_objects[closest_object_id];
}

void empire_save_state(struct buffer_t *buf)
{
    buffer_write_i32(buf, data.scroll_x);
    buffer_write_i32(buf, data.scroll_y);
}

void empire_load_state(struct buffer_t *buf)
{
    data.scroll_x = buffer_read_i32(buf);
    data.scroll_y = buffer_read_i32(buf);
}
