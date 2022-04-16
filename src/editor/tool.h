#ifndef EDITOR_TOOL_H
#define EDITOR_TOOL_H

#include "map/point.h"

typedef enum {
    TOOL_GRASS = 0,
    TOOL_SHRUB = 1,
    TOOL_WATER = 2,
    TOOL_RAISE_LAND = 3,
    TOOL_LOWER_LAND = 4,
    TOOL_ACCESS_RAMP = 5,
    TOOL_TREES = 6,
    TOOL_ROCKS = 7,
    TOOL_MEADOW = 8,
    TOOL_ROAD = 9,
    TOOL_EARTHQUAKE_POINT = 10,
    TOOL_INVASION_POINT = 11,
    TOOL_ENTRY_POINT = 12,
    TOOL_EXIT_POINT = 13,
    TOOL_RIVER_ENTRY_POINT = 14,
    TOOL_RIVER_EXIT_POINT = 15,
    TOOL_NATIVE_HUT = 16,
    TOOL_NATIVE_CENTER = 17,
    TOOL_NATIVE_FIELD = 18,
    TOOL_FISHING_POINT = 19,
    TOOL_HERD_POINT = 20
} tool_type;

tool_type editor_tool_type(void);
int editor_tool_is_active(void);
void editor_tool_deactivate(void);
void editor_tool_set_type(tool_type tool);
void editor_tool_set_with_id(tool_type tool, int id);

int editor_tool_is_brush(void);
int editor_tool_brush_size(void);
void editor_tool_set_brush_size(int size);
void editor_tool_foreach_brush_tile(void (*callback)(const void *user_data, int dx, int dy), const void *user_data);

int editor_tool_is_updatable(void);

int editor_tool_is_in_use(void);

void editor_tool_start_use(const map_tile *tile);

void editor_tool_update_use(const map_tile *tile);

void editor_tool_end_use(const map_tile *tile);

#endif // EDITOR_TOOL_H
