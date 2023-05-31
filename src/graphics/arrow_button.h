#ifndef GRAPHICS_ARROW_BUTTON_H
#define GRAPHICS_ARROW_BUTTON_H

#include "input/mouse.h"

struct arrow_button_t {
    short x_offset;
    short y_offset;
    short image_id;
    short size;
    void (*left_click_handler)(int param1, int param2);
    int parameter1;
    int parameter2;
    // state
    int pressed;
    int repeats;
};

void arrow_buttons_draw(int x, int y, struct arrow_button_t *buttons, int num_buttons);

int arrow_buttons_handle_mouse(
    const struct mouse_t *m, int x, int y, struct arrow_button_t *buttons, int num_buttons, int *focus_button_id);

#endif // GRAPHICS_ARROW_BUTTON_H
