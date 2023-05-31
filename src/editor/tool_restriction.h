#ifndef EDITOR_TOOL_RESTRICTION_H
#define EDITOR_TOOL_RESTRICTION_H

#include "editor/tool.h"
#include "map/point.h"

int editor_tool_can_place_flag(int type, const struct map_tile_t *tile, int *warning);

int editor_tool_can_place_access_ramp(const struct map_tile_t *tile, int *orientation_index);

int editor_tool_can_place_building(const struct map_tile_t *tile, int num_tiles, int *blocked_tiles);

#endif // EDITOR_TOOL_RESTRICTION_H
