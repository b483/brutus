#ifndef WIDGET_MAP_EDITOR_TOOL_H
#define WIDGET_MAP_EDITOR_TOOL_H

#include "graphics/color.h"
#include "map/point.h"

void draw_flat_tile(int x, int y, color_t color_mask);

void map_editor_tool_draw(const map_tile *tile);

#endif // WIDGET_MAP_EDITOR_TOOL_H
