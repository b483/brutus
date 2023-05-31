#ifndef WINDOW_BUILDING_FIGURES_H
#define WINDOW_BUILDING_FIGURES_H

#include "common.h"
#include "input/mouse.h"

void window_building_prepare_figure_list(struct building_info_context_t *c);

void window_building_draw_figure_list(struct building_info_context_t *c);

int window_building_handle_mouse_figure_list(const struct mouse_t *m, struct building_info_context_t *c);

void window_building_play_figure_phrase(struct building_info_context_t *c);

#endif // WINDOW_BUILDING_FIGURES_H
