#include "editor.h"

#include "core/image.h"
#include "figure/image.h"
#include "map/figure.h"
#include "map/grid.h"
#include "scenario/data.h"
#include "scenario/map.h"

void figure_create_editor_flags(void)
{
    for (int id = MAP_FLAG_MIN; id < MAP_FLAG_MAX; id++) {
        figure_create(FIGURE_MAP_FLAG, -1, -1, 0)->resource_id = id;
    }
}

void figure_editor_flag_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 16);
    f->image_id = image_group(GROUP_FIGURE_MAP_FLAG_FLAGS) + f->image_offset / 2;
    map_figure_delete(f);

    map_point point = { 0, 0 };
    int image_base = image_group(GROUP_FIGURE_MAP_FLAG_ICONS);
    if (f->resource_id == MAP_FLAG_ENTRY) {
        point.x = scenario.entry_point.x;
        point.y = scenario.entry_point.y;
        f->cart_image_id = image_base + 2;
    } else if (f->resource_id == MAP_FLAG_EXIT) {
        point.x = scenario.exit_point.x;
        point.y = scenario.exit_point.y;
        f->cart_image_id = image_base + 3;
    } else if (f->resource_id == MAP_FLAG_RIVER_ENTRY) {
        point.x = scenario.river_entry_point.x;
        point.y = scenario.river_entry_point.y;
        f->cart_image_id = image_base + 4;
    } else if (f->resource_id == MAP_FLAG_RIVER_EXIT) {
        point.x = scenario.river_exit_point.x;
        point.y = scenario.river_exit_point.y;
        f->cart_image_id = image_base + 5;
    } else if (f->resource_id >= MAP_FLAG_EARTHQUAKE_MIN && f->resource_id <= MAP_FLAG_EARTHQUAKE_MAX) {
        point = scenario.earthquake_points[f->resource_id - MAP_FLAG_EARTHQUAKE_MIN];
        f->cart_image_id = image_base;
    } else if (f->resource_id >= MAP_FLAG_INVASION_MIN && f->resource_id <= MAP_FLAG_INVASION_MAX) {
        point = scenario.invasion_points[f->resource_id - MAP_FLAG_INVASION_MIN];
        f->cart_image_id = image_base + 1;
    } else if (f->resource_id >= MAP_FLAG_FISHING_MIN && f->resource_id <= MAP_FLAG_FISHING_MAX) {
        point = scenario.fishing_points[f->resource_id - MAP_FLAG_FISHING_MIN];
        f->cart_image_id = image_group(GROUP_FIGURE_FORT_STANDARD_ICONS) + 3;
    } else if (f->resource_id >= MAP_FLAG_HERD_MIN && f->resource_id <= MAP_FLAG_HERD_MAX) {
        point = scenario.herd_points[f->resource_id - MAP_FLAG_HERD_MIN];
        f->cart_image_id = image_group(GROUP_FIGURE_FORT_STANDARD_ICONS) + 4;
    }
    f->x = point.x;
    f->y = point.y;

    f->grid_offset = map_grid_offset(f->x, f->y);
    f->cross_country_x = 15 * f->x + 7;
    f->cross_country_y = 15 * f->y + 7;
    map_figure_add(f);
}
