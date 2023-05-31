#ifndef FIGURE_COMBAT_H
#define FIGURE_COMBAT_H

#include "figure/figure.h"
#include "map/point.h"

#define PREFECT_TARGET_ACQUISITION_RANGE 15

void figure__remove_ranged_targeter_from_list(struct figure_t *f, struct figure_t *targeter);

void figure_combat_handle_attack(struct figure_t *f);

int is_valid_target_for_player_unit(struct figure_t *target);

int is_valid_target_for_enemy_unit(struct figure_t *target);

struct figure_t *melee_unit__set_closest_target(struct figure_t *f);

int set_missile_target(struct figure_t *shooter, struct map_point_t *tile);

void melee_attack_figure_at_offset(struct figure_t *f, int grid_offset);

void clear_targeting_on_unit_death(struct figure_t *unit);

#endif // FIGURE_COMBAT_H
