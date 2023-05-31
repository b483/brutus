#ifndef FIGURE_ROUTE_H
#define FIGURE_ROUTE_H

#include "core/buffer.h"
#include "figure/figure.h"

void figure_route_clear_all(void);

void figure_route_clean(void);

void figure_route_add(struct figure_t *f);

void figure_route_remove(struct figure_t *f);

int figure_route_get_direction(int path_id, int index);

void figure_route_save_state(struct buffer_t *figures, struct buffer_t *paths);

void figure_route_load_state(struct buffer_t *figures, struct buffer_t *paths);

#endif // FIGURE_ROUTE_H
