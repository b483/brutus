#ifndef WIDGET_MINIMAP_H
#define WIDGET_MINIMAP_H

#include "input/input.h"

void widget_minimap_invalidate(void);

void widget_minimap_draw(int x_offset, int y_offset, int width, int height, int force);

int widget_minimap_handle_mouse(const struct mouse_t *m);

#endif // WIDGET_MINIMAP_H
