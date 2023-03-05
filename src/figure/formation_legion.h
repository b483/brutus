#ifndef FIGURE_FORMATION_LEGION_H
#define FIGURE_FORMATION_LEGION_H

#include "building/building.h"
#include "figure/formation.h"
#include "map/point.h"

int formation_legion_create_for_fort(building *fort);

void formation_legion_restore_layout(struct formation_t *m);

void formation_legion_move_to(struct formation_t *m, map_tile *tile);

void formation_legion_return_home(struct formation_t *m);

int formation_legion_at_grid_offset(int grid_offset);

int formation_legion_at_building(int grid_offset);

void formation_legion_decrease_damage(void);

#endif // FIGURE_FORMATION_LEGION_H
