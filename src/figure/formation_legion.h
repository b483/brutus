#ifndef FIGURE_FORMATION_LEGION_H
#define FIGURE_FORMATION_LEGION_H

#include "building/building.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "map/point.h"

#define MAX_LEGIONS 6

extern struct formation_t legion_formations[MAX_LEGIONS];

extern int selected_legion_formation;

int create_legion_formation_for_fort(struct building_t *fort);

void update_legion_morale_monthly(void);

void deploy_legion_unit_to_formation_location(struct figure_t *legion_unit, struct formation_t *legion_formation);

void move_legion_formation_to(struct formation_t *m, map_tile *tile);

void return_legion_formation_home(struct formation_t *m);

int formation_legion_at_grid_offset(int grid_offset);

void update_legion_formations(void);

void legion_formations_save_state(buffer *buf);

void legion_formations_load_state(buffer *buf);

#endif // FIGURE_FORMATION_LEGION_H
