#ifndef WINDOW_BUILDING_DISTRIBUTION_H
#define WINDOW_BUILDING_DISTRIBUTION_H

#include "common.h"
#include "input/mouse.h"

void window_building_draw_dock(struct building_info_context_t *c);

void window_building_draw_market(struct building_info_context_t *c);

void window_building_draw_granary(struct building_info_context_t *c);
void window_building_draw_granary_foreground(struct building_info_context_t *c);
void window_building_draw_granary_orders(struct building_info_context_t *c);
void window_building_draw_granary_orders_foreground(struct building_info_context_t *c);

int window_building_handle_mouse_granary(const struct mouse_t *m, struct building_info_context_t *c);
int window_building_handle_mouse_granary_orders(const struct mouse_t *m, struct building_info_context_t *c);

void window_building_draw_warehouse(struct building_info_context_t *c);
void window_building_draw_warehouse_foreground(struct building_info_context_t *c);
void window_building_draw_warehouse_orders(struct building_info_context_t *c);
void window_building_draw_warehouse_orders_foreground(struct building_info_context_t *c);

int window_building_handle_mouse_warehouse(const struct mouse_t *m, struct building_info_context_t *c);
int window_building_handle_mouse_warehouse_orders(const struct mouse_t *m, struct building_info_context_t *c);

#endif // WINDOW_BUILDING_DISTRIBUTION_H
