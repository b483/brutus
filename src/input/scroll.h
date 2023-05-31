#ifndef INPUT_SCROLL_H
#define INPUT_SCROLL_H

#include "city/view.h"
#include "input/mouse.h"

enum {
    SCROLL_TYPE_CITY = 0,
    SCROLL_TYPE_EMPIRE = 1,
    SCROLL_TYPE_MAX = 2
};

int scroll_in_progress(void);

void scroll_set_custom_margins(int x, int y, int width, int height);
void scroll_restore_margins(void);

int scroll_get_delta(const struct mouse_t *m, struct pixel_view_coordinates_t *delta, int type);

void scroll_drag_start(void);
int scroll_drag_end(void);

void scroll_stop(void);

void scroll_arrow_left(int value);
void scroll_arrow_right(int value);
void scroll_arrow_up(int value);
void scroll_arrow_down(int value);

#endif // INPUT_SCROLL_H
