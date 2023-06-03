#ifndef WIDGET_CITY_WITH_OVERLAY_H
#define WIDGET_CITY_WITH_OVERLAY_H

#include "map/tiles.h"

/**
 * Update the internal state after changing overlay
 */
void city_with_overlay_update(void);

void city_with_overlay_draw(const struct map_tile_t *tile);

#endif // WIDGET_CITY_WITH_OVERLAY_H
