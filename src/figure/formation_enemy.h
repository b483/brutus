#ifndef FIGURE_FORMATION_ENEMY_H
#define FIGURE_FORMATION_ENEMY_H

#include "figure/formation.h"

int formation_rioter_get_target_building(int *x_tile, int *y_tile);

int formation_enemy_move_formation_to(const struct formation_t *m, int x, int y, int *x_tile, int *y_tile);

void register_first_enemy_appearance_time(void);

void update_enemy_formations(void);

#endif // FIGURE_FORMATION_ENEMY_H
