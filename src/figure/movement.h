#ifndef FIGURE_MOVEMENT_H
#define FIGURE_MOVEMENT_H

#include "figure/figure.h"

void figure_movement_init_roaming(struct figure_t *f);

void figure_movement_move_ticks(struct figure_t *f, int num_ticks);

void figure_movement_roam_ticks(struct figure_t *f, int num_ticks);

void figure_movement_follow_ticks(struct figure_t *f, int num_ticks);

void figure_movement_advance_attack(struct figure_t *f);

void figure_movement_set_cross_country_direction(
    struct figure_t *f, int x_src, int y_src, int x_dst, int y_dst, int is_missile);

void figure_movement_set_cross_country_destination(struct figure_t *f, int x_dst, int y_dst);

int figure_movement_move_ticks_cross_country(struct figure_t *f, int num_ticks);

#endif // FIGURE_MOVEMENT_H
