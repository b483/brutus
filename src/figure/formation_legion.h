#ifndef FIGURE_FORMATION_LEGION_H
#define FIGURE_FORMATION_LEGION_H

#include "building/building.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "map/point.h"

int formation_legion_create_for_fort(struct building_t *fort);

int formation_get_num_legions(void);

int get_legion_formation_by_index(int legion_index);

void deploy_legion_unit_to_formation_location(struct figure_t *legion_unit, struct formation_t *legion_formation);

void formation_legion_move_to(struct formation_t *m, map_tile *tile);

void formation_legion_return_home(struct formation_t *m);

int formation_legion_at_grid_offset(int grid_offset);

void formation_legion_update(void);

#endif // FIGURE_FORMATION_LEGION_H
