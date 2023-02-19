#ifndef FIGURE_COMBAT_H
#define FIGURE_COMBAT_H

#include "figure/figure.h"
#include "map/point.h"

#define PREFECT_TARGET_ACQUISITION_RANGE 20

void figure_combat_handle_corpse(figure *f);
void figure_combat_handle_attack(figure *f);

figure *set_closest_eligible_target(figure *f);

int get_missile_target(figure *shooter, int max_range, map_point *tile);

void figure_combat_attack_figure_at(figure *f, int grid_offset);

#endif // FIGURE_COMBAT_H
