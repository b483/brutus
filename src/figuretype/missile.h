#ifndef FIGURETYPE_MISSILE_H
#define FIGURETYPE_MISSILE_H

#include "figure/figure.h"
#include "map/point.h"

void figure_create_explosion_cloud(int x, int y, int size);

void figure_create_missile(struct figure_t *shooter, map_point *target_tile, int type);

void figure_explosion_cloud_action(struct figure_t *f);

void figure_arrow_action(struct figure_t *f);

void figure_javelin_action(struct figure_t *f);

void figure_bolt_action(struct figure_t *f);

#endif // FIGURETYPE_MISSILE_H
