#ifndef EMPIRE_EMPIRE_H
#define EMPIRE_EMPIRE_H

#include "core/buffer.h"
#include "empire/object.h"

void empire_load(int empire_id);
void empire_load_editor(int empire_id, int viewport_width, int viewport_height);

void empire_init_scenario(void);
void empire_scroll_map(int x, int y);
void empire_set_viewport(int width, int height);
void empire_adjust_scroll(int *x_offset, int *y_offset);

struct empire_object_t *empire_select_object(int x, int y);

void empire_save_state(buffer *buf);
void empire_load_state(buffer *buf);

#endif // EMPIRE_EMPIRE_H
