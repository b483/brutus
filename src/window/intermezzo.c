#include "intermezzo.h"

#include "core/time.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/screen.h"
#include "graphics/window.h"
#include "sound/music.h"
#include "sound/speech.h"

#define DISPLAY_TIME_MILLIS 1000

static const char SOUND_FILE_LOSE[] = "wavs/lose_game.wav";
static const char SOUND_FILE_WIN[] = "wavs/actors_great1.wav";

static struct {
    intermezzo_type type;
    void (*callback)(void);
    time_millis start_time;
} data;

static void init(intermezzo_type type, void (*callback)(void))
{
    data.type = type;
    data.callback = callback;
    data.start_time = time_get_millis();
    sound_speech_stop();
    if (data.type == INTERMEZZO_FIRED) {
        sound_music_stop();
        sound_speech_play_file(SOUND_FILE_LOSE);
    } else if (data.type == INTERMEZZO_WON) {
        sound_music_stop();
        sound_speech_play_file(SOUND_FILE_WIN);
    }
}

static void draw_background(void)
{
    graphics_clear_screen();
    int x_offset = (screen_width() - 1024) / 2;
    int y_offset = (screen_height() - 768) / 2;

    int image_base = image_group(GROUP_INTERMEZZO_BACKGROUND);
    if (data.type == INTERMEZZO_MISSION_BRIEFING) {
        image_draw(image_base + 1, x_offset, y_offset);
    } else if (data.type == INTERMEZZO_FIRED) {
        image_draw(image_base, x_offset, y_offset);
    } else if (data.type == INTERMEZZO_WON) {
        image_draw(image_base + 2, x_offset, y_offset);
    }
}

static void handle_input(const mouse *m, __attribute__((unused)) const hotkeys *h)
{
    time_millis current_time = time_get_millis();
    if (m->right.went_up || current_time - data.start_time > (data.type ? DISPLAY_TIME_MILLIS : 300)) {
        data.callback();
    }
}

void window_intermezzo_show(intermezzo_type type, void (*callback)(void))
{
    window_type window = {
        WINDOW_INTERMEZZO,
        draw_background,
        0,
        handle_input,
        0
    };
    init(type, callback);
    window_show(&window);
}
