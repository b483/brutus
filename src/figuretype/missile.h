#ifndef FIGURETYPE_MISSILE_H
#define FIGURETYPE_MISSILE_H

#include "figure/figure.h"
#include "map/point.h"

void figure_create_explosion_cloud(int x, int y, int size);

void figure_create_missile(figure *shooter, map_point *target_tile, figure_type type);

void figure_explosion_cloud_action(figure *f);

void figure_arrow_action(figure *f);

void figure_javelin_action(figure *f);

void figure_bolt_action(figure *f);

#endif // FIGURETYPE_MISSILE_H
