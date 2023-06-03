#ifndef WIDGET_CITY_WITHOUT_OVERLAY_H
#define WIDGET_CITY_WITHOUT_OVERLAY_H

#include "map/tiles.h"
#include "widget/city.h"

void city_without_overlay_draw(int selected_figure_id, struct pixel_coordinate_t *figure_coord, const struct map_tile_t *tile);

#endif // WIDGET_CITY_WITHOUT_OVERLAY_H
