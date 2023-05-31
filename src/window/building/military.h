#ifndef WINDOW_BUILDING_MILITARY_H
#define WINDOW_BUILDING_MILITARY_H

#include "common.h"
#include "input/mouse.h"

void window_building_draw_wall(struct building_info_context_t *c);
void window_building_draw_gatehouse(struct building_info_context_t *c);
void window_building_draw_tower(struct building_info_context_t *c);

void window_building_draw_barracks(struct building_info_context_t *c);
void window_building_draw_military_academy(struct building_info_context_t *c);

void window_building_draw_fort(struct building_info_context_t *c);

void window_building_draw_legion_info(struct building_info_context_t *c);
void window_building_draw_legion_info_foreground(struct building_info_context_t *c);
int window_building_handle_mouse_legion_info(const struct mouse_t *m, struct building_info_context_t *c);

#endif // WINDOW_BUILDING_MILITARY_H
