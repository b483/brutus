#include "window.h"

#include "graphics/warning.h"
#include "input/cursor.h"
#include "input/hotkey.h"
#include "input/scroll.h"
#include "window/city.h"

#define MAX_QUEUE 3

static struct {
    struct window_type_t window_queue[MAX_QUEUE];
    int queue_index;
    struct window_type_t *current_window;
    int refresh_immediate;
    int refresh_on_draw;
    int underlying_windows_redrawing;
} data;

static void noop(void)
{}
static void noop_input(__attribute__((unused)) const struct mouse_t *m, __attribute__((unused)) const struct hotkeys_t *h)
{}

static void increase_queue_index(void)
{
    data.queue_index++;
    if (data.queue_index >= MAX_QUEUE) {
        data.queue_index = 0;
    }
}

static void decrease_queue_index(void)
{
    data.queue_index--;
    if (data.queue_index < 0) {
        data.queue_index = MAX_QUEUE - 1;
    }
}

static void reset_input(void)
{
    mouse_reset_button_state();
    scroll_stop();
}

void window_invalidate(void)
{
    data.refresh_immediate = 1;
    data.refresh_on_draw = 1;
}

int window_is_invalid(void)
{
    return data.refresh_immediate;
}

void window_request_refresh(void)
{
    data.refresh_on_draw = 1;
}

int window_is(int id)
{
    return data.current_window->id == id;
}

int window_get_id(void)
{
    return data.current_window->id;
}

void window_show(const struct window_type_t *window)
{
    reset_input();
    increase_queue_index();
    data.window_queue[data.queue_index] = *window;
    data.current_window = &data.window_queue[data.queue_index];
    if (!data.current_window->draw_background) {
        data.current_window->draw_background = noop;
    }
    if (!data.current_window->draw_foreground) {
        data.current_window->draw_foreground = noop;
    }
    if (!data.current_window->handle_input) {
        data.current_window->handle_input = noop_input;
    }
    window_invalidate();
}

void window_go_back(void)
{
    reset_input();
    decrease_queue_index();
    data.current_window = &data.window_queue[data.queue_index];
    window_invalidate();
}

static void update_input_before(void)
{
    mouse_determine_button_state();
    hotkey_handle_global_keys();
}

static void update_input_after(void)
{
    mouse_reset_scroll();
    input_cursor_update(data.current_window->id);
    hotkey_reset_state();
}

void window_draw(int force)
{
    update_input_before();
    struct window_type_t *w = data.current_window;
    if (force || data.refresh_on_draw) {
        w->draw_background();
        data.refresh_on_draw = 0;
        data.refresh_immediate = 0;
    }
    w->draw_foreground();

    const struct mouse_t *m = mouse_get();
    const struct hotkeys_t *h = hotkey_state();
    w->handle_input(m, h);
    warning_draw();
    update_input_after();
}

void window_draw_underlying_window(void)
{
    if (data.underlying_windows_redrawing < MAX_QUEUE) {
        ++data.underlying_windows_redrawing;
        decrease_queue_index();
        struct window_type_t *window_behind = &data.window_queue[data.queue_index];
        if (window_behind->draw_background) {
            window_behind->draw_background();
        }
        if (window_behind->draw_foreground) {
            window_behind->draw_foreground();
        }
        increase_queue_index();
        --data.underlying_windows_redrawing;
    }
}
