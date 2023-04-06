#ifndef FIGURETYPE_CRIME_H
#define FIGURETYPE_CRIME_H

#include "figure/figure.h"

void figure_generate_criminals(void);

void figure_protestor_action(struct figure_t *f);

void figure_criminal_action(struct figure_t *f);

void figure_rioter_action(struct figure_t *f);

int figure_rioter_collapse_building(struct figure_t *f);

#endif // FIGURETYPE_CRIME_H
