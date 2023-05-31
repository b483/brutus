#ifndef MAP_FIGURE_H
#define MAP_FIGURE_H

#include "core/buffer.h"
#include "figure/figure.h"
#include "map/grid.h"

extern struct grid_u16_t map_figures;

/**
 * Returns the first figure at the given offset
 * @param grid_offset Map offset
 * @return Figure ID of first figure at offset
 */
int map_figure_at(int grid_offset);

/**
 * Returns whether there is a figure at the given offset
 * @param grid_offset Map offset
 * @return True if there is a figure, otherwise false
 */
int map_has_figure_at(int grid_offset);

void map_figure_add(struct figure_t *f);

void map_figure_update(struct figure_t *f);

void map_figure_delete(struct figure_t *f);

void map_figure_save_state(struct buffer_t *buf);

void map_figure_load_state(struct buffer_t *buf);

#endif // MAP_FIGURE_H
