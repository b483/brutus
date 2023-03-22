#ifndef FIGURE_COMBAT_H
#define FIGURE_COMBAT_H

#include "figure/figure.h"
#include "map/point.h"

#define PREFECT_TARGET_ACQUISITION_RANGE 15

void figure_combat_handle_corpse(figure *f);

void figure__remove_ranged_targeter_from_list(figure *f, figure *targeter);

void figure_combat_handle_attack(figure *f);

figure *melee_unit__set_closest_target(figure *f);

int set_missile_target(figure *shooter, map_point *tile, int limit_max_targeters);

void figure_combat_attack_figure_at(figure *f, int grid_offset);

#endif // FIGURE_COMBAT_H
