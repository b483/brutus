#include "victory_video.h"

#include "graphics/graphics.h"
#include "graphics/screen.h"
#include "graphics/video.h"
#include "graphics/window.h"

static struct {
    int width;
    int height;
    void (*callback)(void);
} data;

static int init(const char *filename, int width, int height, void (*callback)(void))
{
    if (video_start(filename)) {
        data.width = width;
        data.height = height;
        data.callback = callback;
        video_init(0);
        return 1;
    }
    return 0;
}

static void draw_background(void)
{
    graphics_clear_screen();
}

static void draw_foreground(void)
{
    video_draw_fullscreen();
}

static void handle_input(const struct mouse_t *m, __attribute__((unused)) const struct hotkeys_t *h)
{
    if (m->left.went_up || m->right.went_up || video_is_finished()) {
        video_stop();
        data.callback();
    }
}

void window_victory_video_show(const char *filename, int width, int height, void (*callback)(void))
{
    if (init(filename, width, height, callback)) {
        struct window_type_t window = {
            WINDOW_VICTORY_VIDEO,
            draw_background,
            draw_foreground,
            handle_input,
        };
        window_show(&window);
    } else {
        callback();
    }
}
