#ifndef FIGURE_FORMATION_ENEMY_H
#define FIGURE_FORMATION_ENEMY_H

#include "figure/formation.h"

struct formation_t *formation_create_enemy(figure_type figure_type, int max_num_figures, int x, int y, int layout, direction_type orientation,
    int enemy_type, int attack_type, int invasion_id);

int formation_rioter_get_target_building(int *x_tile, int *y_tile);

int formation_enemy_move_formation_to(const struct formation_t *m, int x, int y, int *x_tile, int *y_tile);

void formation_enemy_update(void);

#endif // FIGURE_FORMATION_ENEMY_H
