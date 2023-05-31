#ifndef WIDGET_TOP_MENU_EDITOR_H
#define WIDGET_TOP_MENU_EDITOR_H

#include "input/hotkey.h"
#include "input/mouse.h"

void widget_top_menu_editor_draw(void);
int widget_top_menu_editor_handle_input(const struct mouse_t *m, const struct hotkeys_t *h);

#endif // WIDGET_TOP_MENU_EDITOR_H
