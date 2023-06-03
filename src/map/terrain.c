#include "terrain.h"

#include "map/routing.h"

struct grid_u16_t terrain_grid;
static struct grid_u16_t terrain_grid_backup;

struct grid_u8_t terrain_elevation;

int map_terrain_is(int grid_offset, int terrain)
{
    return map_grid_is_valid_offset(grid_offset) && terrain_grid.items[grid_offset] & terrain;
}

void map_terrain_add_with_radius(int x, int y, int size, int radius, int terrain)
{
    int x_min, y_min, x_max, y_max;
    map_grid_get_area(x, y, size, radius, &x_min, &y_min, &x_max, &y_max);

    for (int yy = y_min; yy <= y_max; yy++) {
        for (int xx = x_min; xx <= x_max; xx++) {
            terrain_grid.items[map_grid_offset(xx, yy)] |= terrain;
        }
    }
}

int map_terrain_count_directly_adjacent_with_type(int grid_offset, int terrain)
{
    int count = 0;
    if (map_terrain_is(grid_offset + map_grid_delta(0, -1), terrain)) {
        count++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(1, 0), terrain)) {
        count++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(0, 1), terrain)) {
        count++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(-1, 0), terrain)) {
        count++;
    }
    return count;
}

int map_terrain_has_adjacent_x_with_type(int grid_offset, int terrain)
{
    if (map_terrain_is(grid_offset + map_grid_delta(0, -1), terrain) ||
        map_terrain_is(grid_offset + map_grid_delta(0, 1), terrain)) {
        return 1;
    }
    return 0;
}

int map_terrain_has_adjacent_y_with_type(int grid_offset, int terrain)
{
    if (map_terrain_is(grid_offset + map_grid_delta(-1, 0), terrain) ||
        map_terrain_is(grid_offset + map_grid_delta(1, 0), terrain)) {
        return 1;
    }
    return 0;
}

int map_terrain_exists_tile_in_area_with_type(int x, int y, int size, int terrain)
{
    for (int yy = y; yy < y + size; yy++) {
        for (int xx = x; xx < x + size; xx++) {
            if (map_grid_is_inside(xx, yy, 1) && terrain_grid.items[map_grid_offset(xx, yy)] & terrain) {
                return 1;
            }
        }
    }
    return 0;
}

int map_terrain_exists_tile_in_radius_with_type(int x, int y, int size, int radius, int terrain)
{
    int x_min, y_min, x_max, y_max;
    map_grid_get_area(x, y, size, radius, &x_min, &y_min, &x_max, &y_max);

    for (int yy = y_min; yy <= y_max; yy++) {
        for (int xx = x_min; xx <= x_max; xx++) {
            if (map_terrain_is(map_grid_offset(xx, yy), terrain)) {
                return 1;
            }
        }
    }
    return 0;
}

int map_terrain_exist_multiple_tiles_in_radius_with_type(int x, int y, int size, int radius, int terrain, int count)
{
    int x_min, y_min, x_max, y_max;
    map_grid_get_area(x, y, size, radius, &x_min, &y_min, &x_max, &y_max);

    int tiles_found = 0;
    for (int yy = y_min; yy <= y_max; yy++) {
        for (int xx = x_min; xx <= x_max; xx++) {
            if (map_terrain_is(map_grid_offset(xx, yy), terrain)) {
                tiles_found++;
                if (tiles_found >= count) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

int map_terrain_all_tiles_in_radius_are(int x, int y, int size, int radius, int terrain)
{
    int x_min, y_min, x_max, y_max;
    map_grid_get_area(x, y, size, radius, &x_min, &y_min, &x_max, &y_max);

    for (int yy = y_min; yy <= y_max; yy++) {
        for (int xx = x_min; xx <= x_max; xx++) {
            if (!map_terrain_is(map_grid_offset(xx, yy), terrain)) {
                return 0;
            }
        }
    }
    return 1;
}

int map_terrain_is_adjacent_to_open_water(int x, int y, int size)
{
    int base_offset = map_grid_offset(x, y);
    for (const int *tile_delta = map_grid_adjacent_offsets(size); *tile_delta; tile_delta++) {
        if (map_terrain_is(base_offset + *tile_delta, TERRAIN_WATER) &&
            map_routing_distance(base_offset + *tile_delta) > 0) {
            return 1;
        }
    }
    return 0;
}

int map_terrain_get_adjacent_road_or_clear_land(int x, int y, int size, int *x_tile, int *y_tile)
{
    int base_offset = map_grid_offset(x, y);
    for (const int *tile_delta = map_grid_adjacent_offsets(size); *tile_delta; tile_delta++) {
        int grid_offset = base_offset + *tile_delta;
        if (map_terrain_is(grid_offset, TERRAIN_ROAD) ||
            !map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
            *x_tile = map_grid_offset_to_x(grid_offset);
            *y_tile = map_grid_offset_to_y(grid_offset);
            return 1;
        }
    }
    return 0;
}

static void add_road(int grid_offset)
{
    if (!map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
        terrain_grid.items[grid_offset] |= TERRAIN_ROAD;
    }
}

void map_terrain_add_gatehouse_roads(int x, int y, int orientation)
{
    // roads under gatehouse
    terrain_grid.items[map_grid_offset(x, y)] |= TERRAIN_ROAD;
    terrain_grid.items[map_grid_offset(x + 1, y)] |= TERRAIN_ROAD;
    terrain_grid.items[map_grid_offset(x, y + 1)] |= TERRAIN_ROAD;
    terrain_grid.items[map_grid_offset(x + 1, y + 1)] |= TERRAIN_ROAD;

    // free roads before/after gate
    if (orientation == 1) {
        add_road(map_grid_offset(x, y - 1));
        add_road(map_grid_offset(x + 1, y - 1));
        add_road(map_grid_offset(x, y + 2));
        add_road(map_grid_offset(x + 1, y + 2));
    } else if (orientation == 2) {
        add_road(map_grid_offset(x - 1, y));
        add_road(map_grid_offset(x - 1, y + 1));
        add_road(map_grid_offset(x + 2, y));
        add_road(map_grid_offset(x + 2, y + 1));
    }
}

void map_terrain_add_triumphal_arch_roads(int x, int y, int orientation)
{
    if (orientation == 1) {
        // road in the middle
        terrain_grid.items[map_grid_offset(x + 1, y)] |= TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 1, y + 1)] |= TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 1, y + 2)] |= TERRAIN_ROAD;
        // no roads on other tiles
        terrain_grid.items[map_grid_offset(x, y)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x, y + 1)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x, y + 2)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y + 1)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y + 2)] &= ~TERRAIN_ROAD;
    } else if (orientation == 2) {
        // road in the middle
        terrain_grid.items[map_grid_offset(x, y + 1)] |= TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 1, y + 1)] |= TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y + 1)] |= TERRAIN_ROAD;
        // no roads on other tiles
        terrain_grid.items[map_grid_offset(x, y)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 1, y)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x, y + 2)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 1, y + 2)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y + 2)] &= ~TERRAIN_ROAD;
    }
}

void map_terrain_backup(void)
{
    map_grid_copy_u16(terrain_grid.items, terrain_grid_backup.items);
}

void map_terrain_restore(void)
{
    map_grid_copy_u16(terrain_grid_backup.items, terrain_grid.items);
}

void map_terrain_clear(void)
{
    map_grid_clear_u16(terrain_grid.items);
}

static void fix_cliff_tiles(int grid_offset)
{
    // reduce elevation when the surrounding tiles are at least 2 lower
    int max = terrain_elevation.items[grid_offset] - 1;
    if (terrain_elevation.items[grid_offset + map_grid_delta(-1, 0)] < max ||
        terrain_elevation.items[grid_offset + map_grid_delta(0, -1)] < max ||
        terrain_elevation.items[grid_offset + map_grid_delta(1, 0)] < max ||
        terrain_elevation.items[grid_offset + map_grid_delta(0, 1)] < max) {
        terrain_elevation.items[grid_offset]--;
    }
}

void map_elevation_remove_cliffs(void)
{
    // elevation is max 5, so we need 4 passes to fix the lot
    for (int level = 0; level < 4; level++) {
        int grid_offset = map_data.start_offset;
        for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
            for (int x = 0; x < map_data.width; x++, grid_offset++) {
                if (terrain_elevation.items[grid_offset] > 0) {
                    fix_cliff_tiles(grid_offset);
                }
            }
        }
    }
}

void map_terrain_save_state(struct buffer_t *buf)
{
    map_grid_save_state_u16(terrain_grid.items, buf);
    map_grid_save_state_u8(terrain_elevation.items, buf);
}

void map_terrain_load_state(struct buffer_t *buf)
{
    map_grid_load_state_u16(terrain_grid.items, buf);
    map_grid_load_state_u8(terrain_elevation.items, buf);
}
