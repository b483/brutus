#ifndef FIGURE_COMBAT_H
#define FIGURE_COMBAT_H

#include "figure/figure.h"
#include "map/point.h"

#define PREFECT_TARGET_ACQUISITION_RANGE 15

void figure__remove_ranged_targeter_from_list(figure *f, figure *targeter);

void figure_combat_handle_attack(figure *f);

int is_valid_target_for_player_unit(figure *target);

int is_valid_target_for_enemy_unit(figure *target);

figure *melee_unit__set_closest_target(figure *f);

int set_missile_target(figure *shooter, map_point *tile, int limit_max_targeters);

void figure_combat_attack_figure_at(figure *f, int grid_offset);

void clear_targeting_on_unit_death(figure *unit);

#endif // FIGURE_COMBAT_H
