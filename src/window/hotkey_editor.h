#ifndef WINDOW_HOTKEY_EDITOR_H
#define WINDOW_HOTKEY_EDITOR_H

#include "core/hotkey_config.h"
#include "input/input.h"

void window_hotkey_editor_show(int action, int index, void (*callback)(int, int, int, int));

void window_hotkey_editor_key_pressed(int key, int modifiers);
void window_hotkey_editor_key_released(int key, int modifiers);

#endif // WINDOW_HOTKEY_EDITOR_H
