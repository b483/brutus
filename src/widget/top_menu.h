#ifndef WINDOW_TOP_MENU_H
#define WINDOW_TOP_MENU_H

#include "input/input.h"

void widget_top_menu_draw(int force);
int widget_top_menu_handle_input(const struct mouse_t *m, const struct hotkeys_t *h);

#endif // WINDOW_TOP_MENU_H
