#ifndef FIGURETYPE_ANIMAL_H
#define FIGURETYPE_ANIMAL_H

#include "figure/figure.h"

void figure_create_fishing_points(void);

void figure_create_herds(void);

void figure_seagulls_action(struct figure_t *f);

void figure_sheep_action(struct figure_t *f);

void figure_wolf_action(struct figure_t *f);

void figure_zebra_action(struct figure_t *f);

void figure_hippodrome_horse_action(struct figure_t *f);

void figure_hippodrome_horse_reroute(void);

#endif // FIGURETYPE_ANIMAL_H
