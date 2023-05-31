#ifndef WIDGET_CITY_OVERLAY_H
#define WIDGET_CITY_OVERLAY_H

#include "building/building.h"
#include "figure/figure.h"

#define NO_COLUMN -1

enum {
    COLUMN_TYPE_RISK,
    COLUMN_TYPE_ACCESS
};

struct city_overlay_t {
    int type;
    int column_type;
    int (*show_building)(const struct building_t *b);
    int (*show_figure)(const struct figure_t *f);
    int (*get_column_height)(const struct building_t *b);
    void (*draw_custom_footprint)(int x, int y, int grid_offset);
    void (*draw_custom_top)(int x, int y, int grid_offset);
};

void city_with_overlay_draw_building_footprint(int x, int y, int grid_offset, int image_offset);

void city_with_overlay_draw_building_top(int x, int y, int grid_offset);

#endif // WIDGET_CITY_OVERLAY_H
