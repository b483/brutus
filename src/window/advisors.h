#ifndef WINDOW_ADVISORS_H
#define WINDOW_ADVISORS_H

#include "input/input.h"

struct advisor_window_type_t {
    /**
     * @return height of the advisor in blocks of 16px
     */
    int (*draw_background)(void);
    void (*draw_foreground)(void);
    int (*handle_mouse)(const struct mouse_t *m);
};

int window_advisors_get_advisor(void);

void window_advisors_draw_dialog_background(void);

void window_advisors_show(int advisor);

#endif // WINDOW_ADVISORS_H
