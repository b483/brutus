#ifndef CITY_DATA_H
#define CITY_DATA_H

#include "core/buffer.h"

void city_data_init(void);

void city_data_init_scenario(void);

void city_data_save_state(struct buffer_t *main, struct buffer_t *graph_order, struct buffer_t *entry_exit_xy, struct buffer_t *entry_exit_grid_offset);

void city_data_load_state(struct buffer_t *main, struct buffer_t *graph_order, struct buffer_t *entry_exit_xy, struct buffer_t *entry_exit_grid_offset);

#endif // CITY_DATA_H
