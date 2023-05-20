#ifndef FIGURE_FORMATION_ENEMY_H
#define FIGURE_FORMATION_ENEMY_H

#include "figure/formation.h"

#define MAX_ENEMY_FORMATIONS 100

extern struct formation_t enemy_formations[MAX_ENEMY_FORMATIONS];

int formation_rioter_get_target_building(int *x_tile, int *y_tile);

void update_enemy_formations(void);

void enemy_formations_save_state(buffer *buf);

void enemy_formations_load_state(buffer *buf);

#endif // FIGURE_FORMATION_ENEMY_H
