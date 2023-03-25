#ifndef MAP_TERRAIN_H
#define MAP_TERRAIN_H

#include "core/buffer.h"

enum {
    TERRAIN_SHRUB = 1,
    TERRAIN_ROCK = 2,
    TERRAIN_WATER = 4,
    TERRAIN_BUILDING = 8,
    TERRAIN_TREE = 16,
    TERRAIN_GARDEN = 32,
    TERRAIN_ROAD = 64,
    TERRAIN_RESERVOIR_RANGE = 128,
    TERRAIN_AQUEDUCT = 256,
    TERRAIN_ELEVATION = 512,
    TERRAIN_ACCESS_RAMP = 1024,
    TERRAIN_MEADOW = 2048,
    TERRAIN_RUBBLE = 4096,
    TERRAIN_FOUNTAIN_RANGE = 8192,
    TERRAIN_WALL = 16384,
    TERRAIN_GATEHOUSE = 32768,
    // combined
    TERRAIN_WALL_OR_GATEHOUSE = TERRAIN_WALL | TERRAIN_GATEHOUSE,
    TERRAIN_NOT_CLEAR = TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_BUILDING | TERRAIN_TREE | TERRAIN_GARDEN | TERRAIN_ROAD | TERRAIN_AQUEDUCT | TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP | TERRAIN_RUBBLE | TERRAIN_WALL | TERRAIN_GATEHOUSE,
    TERRAIN_CLEARABLE = TERRAIN_SHRUB | TERRAIN_BUILDING | TERRAIN_TREE | TERRAIN_GARDEN | TERRAIN_ROAD | TERRAIN_AQUEDUCT | TERRAIN_RUBBLE | TERRAIN_WALL | TERRAIN_GATEHOUSE,
    TERRAIN_IMPASSABLE = TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_BUILDING | TERRAIN_TREE | TERRAIN_ELEVATION | TERRAIN_WALL | TERRAIN_GATEHOUSE,
    TERRAIN_ALL = 65535
};

int map_terrain_is(int grid_offset, int terrain);

int map_terrain_get(int grid_offset);

void map_terrain_set(int grid_offset, int terrain);

void map_terrain_add(int grid_offset, int terrain);

void map_terrain_remove(int grid_offset, int terrain);

void map_terrain_add_with_radius(int x, int y, int size, int radius, int terrain);

void map_terrain_remove_with_radius(int x, int y, int size, int radius, int terrain);

void map_terrain_remove_all(int terrain);

int map_terrain_count_directly_adjacent_with_type(int grid_offset, int terrain);

int map_terrain_count_diagonally_adjacent_with_type(int grid_offset, int terrain);

int map_terrain_has_adjacent_x_with_type(int grid_offset, int terrain);

int map_terrain_has_adjacent_y_with_type(int grid_offset, int terrain);

int map_terrain_exists_tile_in_area_with_type(int x, int y, int size, int terrain);

int map_terrain_exists_tile_in_radius_with_type(int x, int y, int size, int radius, int terrain);

int map_terrain_exist_multiple_tiles_in_radius_with_type(int x, int y, int size, int radius, int terrain, int count);

int map_terrain_exists_clear_tile_in_radius(int x, int y, int size, int radius, int except_grid_offset,
                                            int *x_tile, int *y_tile);

int map_terrain_all_tiles_in_radius_are(int x, int y, int size, int radius, int terrain);

int map_terrain_has_only_rocks_trees_in_ring(int x, int y, int distance);

int map_terrain_has_only_meadow_in_ring(int x, int y, int distance);

int map_terrain_is_adjacent_to_wall(int x, int y, int size);

int map_terrain_is_adjacent_to_open_water(int x, int y, int size);

int map_terrain_get_adjacent_road_or_clear_land(int x, int y, int size, int *x_tile, int *y_tile);

void map_terrain_add_gatehouse_roads(int x, int y, int orientation);
void map_terrain_add_triumphal_arch_roads(int x, int y, int orientation);

void map_terrain_backup(void);

void map_terrain_restore(void);

void map_terrain_clear(void);

void map_terrain_init_outside_map(void);

void map_terrain_save_state(buffer *buf);

void map_terrain_load_state(buffer *buf);

#endif // MAP_TERRAIN_H
