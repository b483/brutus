#ifndef MAP_FIGURE_H
#define MAP_FIGURE_H

#include "core/buffer.h"
#include "figure/figure.h"
#include "map/grid.h"

extern grid_u16 map_figures;

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

void map_figure_add(figure *f);

void map_figure_update(figure *f);

void map_figure_delete(figure *f);

void map_figure_save_state(buffer *buf);

void map_figure_load_state(buffer *buf);

#endif // MAP_FIGURE_H
