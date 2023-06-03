#include "desirability.h"

#include "building/building.h"
#include "core/calc.h"
#include "map/grid.h"
#include "map/property.h"
#include "map/terrain.h"

static struct grid_i8_t desirability_grid;

struct ring_tile_t {
    int x;
    int y;
    int grid_offset;
};

static struct {
    struct ring_tile_t tiles[1080];
    int index[6][7];
} data;

static void map_ring_init(void)
{
    int index = 0;
    int x, y;
    for (int size = 1; size <= 5; size++) {
        for (int dist = 1; dist <= 6; dist++) {
            data.index[size][dist] = index;
            // top row, from x=0
            for (y = -dist, x = 0; x < size + dist; x++, index++) {
                data.tiles[index].x = x;
                data.tiles[index].y = y;
            }
            // right row down
            for (x = size + dist - 1, y = -dist + 1; y < size + dist; y++, index++) {
                data.tiles[index].x = x;
                data.tiles[index].y = y;
            }
            // bottom row to the left
            for (y = size + dist - 1, x = size + dist - 2; x >= -dist; x--, index++) {
                data.tiles[index].x = x;
                data.tiles[index].y = y;
            }
            // exception (bug in game): size 4 distance 2, left corner is off by x+1, y-1
            if (size == 4 && dist == 2) {
                data.tiles[index - 1].x += 1;
                data.tiles[index - 1].y -= 1;
            }
            // left row up
            for (x = -dist, y = size + dist - 2; y >= -dist; y--, index++) {
                data.tiles[index].x = x;
                data.tiles[index].y = y;
            }
            // top row up to x=0
            for (y = -dist, x = -dist + 1; x < 0; x++, index++) {
                data.tiles[index].x = x;
                data.tiles[index].y = y;
            }
        }
    }
    for (int i = 0; i < index; i++) {
        data.tiles[i].grid_offset = map_grid_delta(data.tiles[i].x, data.tiles[i].y);
    }
}

static int map_ring_is_inside_map(int x, int y)
{
    return x >= -1 && x <= map_data.width &&
        y >= -1 && y <= map_data.height;
}

void map_desirability_clear(void)
{
    map_grid_clear_i8(desirability_grid.items);
}

static void add_desirability_at_distance(int x, int y, int size, int distance, int desirability)
{
    int partially_outside_map = 0;
    if (x - distance < -1 || x + distance + size - 1 > map_data.width) {
        partially_outside_map = 1;
    }
    if (y - distance < -1 || y + distance + size - 1 > map_data.height) {
        partially_outside_map = 1;
    }
    int base_offset = map_grid_offset(x, y);

    if (partially_outside_map) {
        for (int i = data.index[size][distance]; i < data.index[size][distance] + 4 * (size - 1) + 8 * distance; i++) {
            const struct ring_tile_t *tile = &data.tiles[i];
            if (map_ring_is_inside_map(x + tile->x, y + tile->y)) {
                desirability_grid.items[base_offset + tile->grid_offset] += desirability;
                // BUG: bounding on wrong tile:
                desirability_grid.items[base_offset] = calc_bound(desirability_grid.items[base_offset], -100, 100);
            }
        }
    } else {
        for (int i = data.index[size][distance]; i < data.index[size][distance] + 4 * (size - 1) + 8 * distance; i++) {
            const struct ring_tile_t *tile = &data.tiles[i];
            desirability_grid.items[base_offset + tile->grid_offset] =
                calc_bound(desirability_grid.items[base_offset + tile->grid_offset] + desirability, -100, 100);
        }
    }
}

static void add_to_terrain(int x, int y, int size, int desirability, int step, int step_size, int range)
{
    if (size > 0) {
        if (range > 6) {
            range = 6;
        }
        int tiles_within_step = 0;
        for (int distance = 1; distance <= range; distance++) {
            add_desirability_at_distance(x, y, size, distance, desirability);
            tiles_within_step++;
            if (tiles_within_step >= step) {
                desirability += step_size;
                tiles_within_step = 0;
            }
        }
    }
}

static void update_buildings(void)
{
    int max_id = building_get_highest_id();
    for (int i = 1; i <= max_id; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE) {
            add_to_terrain(
                b->x, b->y, b->size,
                building_properties[b->type].desirability_value,
                building_properties[b->type].desirability_step,
                building_properties[b->type].desirability_step_size,
                building_properties[b->type].desirability_range);
        }
    }
}

static void update_terrain(void)
{
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_property_is_plaza_or_earthquake(grid_offset)) {
                int type;
                if (terrain_grid.items[grid_offset] & TERRAIN_ROAD) {
                    type = BUILDING_PLAZA;
                } else if (terrain_grid.items[grid_offset] & TERRAIN_ROCK) {
                    // earthquake fault line: slight negative
                    type = BUILDING_HOUSE_VACANT_LOT;
                } else {
                    // invalid plaza/earthquake flag
                    map_property_clear_plaza_or_earthquake(grid_offset);
                    continue;
                }
                add_to_terrain(x, y, 1,
                    building_properties[type].desirability_value,
                    building_properties[type].desirability_step,
                    building_properties[type].desirability_step_size,
                    building_properties[type].desirability_range);
            } else if (terrain_grid.items[grid_offset] & TERRAIN_GARDEN) {
                add_to_terrain(x, y, 1,
                    building_properties[BUILDING_GARDENS].desirability_value,
                    building_properties[BUILDING_GARDENS].desirability_step,
                    building_properties[BUILDING_GARDENS].desirability_step_size,
                    building_properties[BUILDING_GARDENS].desirability_range);
            } else if (terrain_grid.items[grid_offset] & TERRAIN_RUBBLE) {
                add_to_terrain(x, y, 1, -2, 1, 1, 2);
            } else if (terrain_grid.items[grid_offset] & TERRAIN_WATER) {
                add_to_terrain(x, y, 1, 1, 1, 0, 3);
            } else if (terrain_grid.items[grid_offset] & TERRAIN_SHRUB) {
                add_to_terrain(x, y, 1, 1, 1, 0, 1);
            } else if (terrain_grid.items[grid_offset] & TERRAIN_TREE) {
                add_to_terrain(x, y, 1, 1, 1, 0, 3);
            }
        }
    }
}

void map_desirability_update(void)
{
    map_desirability_clear();
    update_buildings();
    update_terrain();
    map_ring_init();
}

int map_desirability_get(int grid_offset)
{
    return desirability_grid.items[grid_offset];
}

int map_desirability_get_max(int x, int y, int size)
{
    if (size == 1) {
        return desirability_grid.items[map_grid_offset(x, y)];
    }
    int max = -9999;
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            if (desirability_grid.items[grid_offset] > max) {
                max = desirability_grid.items[grid_offset];
            }
        }
    }
    return max;
}

void map_desirability_save_state(struct buffer_t *buf)
{
    map_grid_save_state_i8(desirability_grid.items, buf);
}

void map_desirability_load_state(struct buffer_t *buf)
{
    map_grid_load_state_i8(desirability_grid.items, buf);
}
