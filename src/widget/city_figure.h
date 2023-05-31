#ifndef WIDGET_CITY_FIGURE_H
#define WIDGET_CITY_FIGURE_H

#include "figure/figure.h"
#include "widget/city.h"

void city_draw_figure(struct figure_t *f, int x, int y, int hover);

void city_draw_selected_figure(struct figure_t *f, int x, int y, struct pixel_coordinate_t *coord);

#endif // WIDGET_CITY_FIGURE_H
