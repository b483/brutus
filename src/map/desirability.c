#include "desirability.h"

#include "building/building.h"
#include "core/calc.h"
#include "map/grid.h"
#include "map/property.h"
#include "map/ring.h"
#include "map/terrain.h"

static grid_i8 desirability_grid;

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
    int start = map_ring_start(size, distance);
    int end = map_ring_end(size, distance);

    if (partially_outside_map) {
        for (int i = start; i < end; i++) {
            const ring_tile *tile = map_ring_tile(i);
            if (map_ring_is_inside_map(x + tile->x, y + tile->y)) {
                desirability_grid.items[base_offset + tile->grid_offset] += desirability;
                // BUG: bounding on wrong tile:
                desirability_grid.items[base_offset] = calc_bound(desirability_grid.items[base_offset], -100, 100);
            }
        }
    } else {
        for (int i = start; i < end; i++) {
            const ring_tile *tile = map_ring_tile(i);
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

void map_desirability_save_state(buffer *buf)
{
    map_grid_save_state_i8(desirability_grid.items, buf);
}

void map_desirability_load_state(buffer *buf)
{
    map_grid_load_state_i8(desirability_grid.items, buf);
}
