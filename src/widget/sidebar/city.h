#ifndef WIDGET_SIDEBAR_CITY_H
#define WIDGET_SIDEBAR_CITY_H

#include "input/mouse.h"

void widget_sidebar_city_draw_background(void);
void widget_sidebar_city_draw_foreground(void);

int widget_sidebar_city_handle_mouse(const struct mouse_t *m);
int widget_sidebar_city_handle_mouse_build_menu(const struct mouse_t *m);

void button_go_to_problem(int param1, int param2);

#endif // WIDGET_SIDEBAR_CITY_H
