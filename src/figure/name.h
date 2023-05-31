#ifndef FIGURE_NAME_H
#define FIGURE_NAME_H

#include "core/buffer.h"
#include "figure/figure.h"

/**
 * Initializes the figure name generator.
 */
void figure_name_init(void);

int get_figure_name_id(struct figure_t *f);

/**
 * Saves generator state
 * @param buf Buffer to save to
 */
void figure_name_save_state(struct buffer_t *buf);

/**
 * Loads generator state
 * @param buf Buffer to load from
 */
void figure_name_load_state(struct buffer_t *buf);

#endif // FIGURE_NAME_H
