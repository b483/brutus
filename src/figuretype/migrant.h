#ifndef FIGURETYPE_MIGRANT_H
#define FIGURETYPE_MIGRANT_H

#include "building/building.h"
#include "figure/figure.h"

void figure_create_immigrant(struct building_t *house, int num_people);

void figure_create_emigrant(struct building_t *house, int num_people);

void figure_create_homeless(int x, int y, int num_people);

void figure_immigrant_action(struct figure_t *f);

void figure_emigrant_action(struct figure_t *f);

void figure_homeless_action(struct figure_t *f);

#endif // FIGURETYPE_MIGRANT_H
