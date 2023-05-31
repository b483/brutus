#ifndef WIDGET_MAP_EDITOR_H
#define WIDGET_MAP_EDITOR_H

#include "input/hotkey.h"
#include "input/mouse.h"

void widget_map_editor_draw(void);

void request_exit_editor(void);

void widget_map_editor_handle_input(const struct mouse_t *m, const struct hotkeys_t *h);

void widget_map_editor_clear_current_tile(void);

#endif // WIDGET_MAP_EDITOR_H
