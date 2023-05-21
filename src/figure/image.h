#ifndef FIGURE_IMAGE_H
#define FIGURE_IMAGE_H

#include "figure/figure.h"

void figure_image_increase_offset(struct figure_t *f, int max);

void figure_image_set_cart_offset(struct figure_t *f, int direction);

int figure_image_missile_launcher_offset(struct figure_t *f);

int figure_image_direction(struct figure_t *f);

int figure_image_normalize_direction(int direction);

#endif // FIGURE_IMAGE_H
