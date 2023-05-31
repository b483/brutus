#ifndef GRAPHICS_CUSTOM_BUTTON_H
#define GRAPHICS_CUSTOM_BUTTON_H

#include "graphics/button.h"
#include "input/mouse.h"

struct generic_button_t {
    short x;
    short y;
    short width;
    short height;
    void (*left_click_handler)(int param1, int param2);
    void (*right_click_handler)(int param1, int param2);
    int parameter1;
    int parameter2;
};

int generic_buttons_handle_mouse(
    const struct mouse_t *m, int x, int y, struct generic_button_t *buttons, int num_buttons, int *focus_button_id);

#endif // GRAPHICS_CUSTOM_BUTTON_H
