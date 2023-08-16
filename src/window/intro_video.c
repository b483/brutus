#include "intro_video.h"

#include "graphics/graphics.h"
#include "sound/sound.h"

#define NUM_INTRO_VIDEOS 3

static int current_video;

static int started;

static const char *intro_videos[NUM_INTRO_VIDEOS] = { "smk/logo.smk", "smk/intro.smk", "smk/credits.smk" };

static int start_next_video(void)
{
    graphics_clear_screen();
    while (current_video < NUM_INTRO_VIDEOS) {
        if (video_start(intro_videos[current_video++])) {
            video_init(0);
            return 1;
        }
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

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (!started || m->left.went_up || m->right.went_up || video_is_finished() || h->enter_pressed) {
        video_stop();
        if (!start_next_video()) {
            play_intro_music();
            window_go_back();
        }
        started = 1;
    }
}

void window_intro_video_show(void)
{
    current_video = 0;
    started = 0;
    struct window_type_t window = {
        WINDOW_INTRO_VIDEO,
        draw_background,
        draw_foreground,
        handle_input,
    };
    window_show(&window);
}
