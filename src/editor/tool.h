#ifndef EDITOR_TOOL_H
#define EDITOR_TOOL_H

#include "map/tiles.h"

enum {
    TOOL_GRASS = 0,
    TOOL_SMALL_SHRUB = 1,
    TOOL_MEDIUM_SHRUB = 2,
    TOOL_LARGE_SHRUB = 3,
    TOOL_LARGEST_SHRUB = 4,
    TOOL_WATER = 5,
    TOOL_RAISE_LAND = 6,
    TOOL_LOWER_LAND = 7,
    TOOL_ACCESS_RAMP = 8,
    TOOL_TREES = 9,
    TOOL_SMALL_ROCK = 10,
    TOOL_MEDIUM_ROCK = 11,
    TOOL_LARGE_ROCK = 12,
    TOOL_MEADOW = 13,
    TOOL_ROAD = 14,
    TOOL_EARTHQUAKE_POINT = 15,
    TOOL_INVASION_POINT = 16,
    TOOL_ENTRY_POINT = 17,
    TOOL_EXIT_POINT = 18,
    TOOL_RIVER_ENTRY_POINT = 19,
    TOOL_RIVER_EXIT_POINT = 20,
    TOOL_NATIVE_CENTER = 21,
    TOOL_NATIVE_HUT = 22,
    TOOL_NATIVE_FIELD = 23,
    TOOL_HOUSE_VACANT_LOT = 24,
    TOOL_FISHING_POINT = 25,
    TOOL_HERD_POINT = 26
};

int editor_tool_type(void);
int editor_tool_is_active(void);
void editor_tool_deactivate(void);
void editor_tool_set_with_id(int tool, int id);

int editor_tool_brush_size(void);
void editor_tool_set_brush_size(int size);
void editor_tool_foreach_brush_tile(void (*callback)(const void *user_data, int dx, int dy), const void *user_data);

int editor_tool_is_updatable(void);

int editor_tool_is_in_use(void);

void editor_tool_start_use(const struct map_tile_t *tile);

void draw_brush(int x, int y);

void editor_tool_update_use(const struct map_tile_t *tile);

void editor_tool_end_use(const struct map_tile_t *tile);

int editor_tool_can_place_flag(int type, const struct map_tile_t *tile, int *warning);

int editor_tool_can_place_access_ramp(const struct map_tile_t *tile, int *orientation_index);

int editor_tool_can_place_building(const struct map_tile_t *tile, int num_tiles, int *blocked_tiles);

#endif // EDITOR_TOOL_H
