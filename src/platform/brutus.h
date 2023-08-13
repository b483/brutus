#ifndef GAME_SYSTEM_H
#define GAME_SYSTEM_H

#include "graphics/color.h"
#include "input/keys.h"

#include "SDL.h"

enum {
    USER_EVENT_QUIT,
    USER_EVENT_RESIZE,
    USER_EVENT_FULLSCREEN,
    USER_EVENT_WINDOWED,
    USER_EVENT_CENTER_WINDOW,
};

const char *system_version(void);

void system_resize(int width, int height);

int scale_display(int scale_percentage);

int get_max_display_scale(void);

void init_cursors(int scale_percentage);

void set_cursor(int cursor_id);

void system_keyboard_set_input_rect(int x, int y, int width, int height);

void set_relative_mouse_mode(int enabled);

void post_event(int code);

void log_info(const char *msg, const char *param_str, int param_int);

void log_error(const char *msg, const char *param_str, int param_int);

void update_screen(void);

#endif // GAME_SYSTEM_H
