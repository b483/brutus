#include "settings.h"

#include "city/constants.h"
#include "core/buffer.h"
#include "core/calc.h"
#include "core/io.h"
#include "core/file.h"
#include "core/string.h"

#define INF_SIZE 62

static struct {
    // display settings
    int fullscreen;
    int window_width;
    int window_height;
    // sound settings
    set_sound sound_effects;
    set_sound sound_music;
    set_sound sound_speech;
    set_sound sound_city;
    // speed settings
    int game_speed;
    int scroll_speed;
    // misc settings
    int monthly_autosave;
    set_tooltips tooltips;
    int warnings;
    int victory_video;
    // persistent game state
    int last_advisor;
    // file data
    uint8_t inf_file[INF_SIZE];
} data;

static void load_default_settings(void)
{
    data.fullscreen = 0;
    data.window_width = 1280;
    data.window_height = 800;

    data.sound_effects.enabled = 1;
    data.sound_effects.volume = 50;
    data.sound_music.enabled = 1;
    data.sound_music.volume = 50;
    data.sound_speech.enabled = 1;
    data.sound_speech.volume = 50;
    data.sound_city.enabled = 1;
    data.sound_city.volume = 50;

    data.game_speed = 80;
    data.scroll_speed = 70;

    data.monthly_autosave = 0;
    data.tooltips = TOOLTIPS_NONE;
    data.warnings = 1;
    data.victory_video = 0;
    data.last_advisor = ADVISOR_LABOR;
}

static void load_settings(buffer *buf)
{
    data.fullscreen = buffer_read_i32(buf);
    data.window_width = buffer_read_i32(buf);
    data.window_height = buffer_read_i32(buf);

    data.sound_effects.enabled = buffer_read_u8(buf);
    data.sound_effects.volume = buffer_read_i32(buf);
    data.sound_music.enabled = buffer_read_u8(buf);
    data.sound_music.volume = buffer_read_i32(buf);
    data.sound_speech.enabled = buffer_read_u8(buf);
    data.sound_speech.volume = buffer_read_i32(buf);
    data.sound_city.enabled = buffer_read_u8(buf);
    data.sound_city.volume = buffer_read_i32(buf);

    data.game_speed = buffer_read_i32(buf);
    data.scroll_speed = buffer_read_i32(buf);

    data.monthly_autosave = buffer_read_u8(buf);
    data.tooltips = buffer_read_i32(buf);
    data.warnings = buffer_read_u8(buf);
    data.victory_video = buffer_read_i32(buf);
    data.last_advisor = buffer_read_i32(buf);
}

void settings_load(void)
{
    load_default_settings();

    int size = io_read_file_into_buffer(SETTINGS_FILE_PATH, data.inf_file, INF_SIZE);
    if (!size) {
        return;
    }

    buffer buf;
    buffer_init(&buf, data.inf_file, size);
    load_settings(&buf);

    if (data.window_width + data.window_height < 500) {
        // most likely migration from Caesar 3
        data.window_width = 800;
        data.window_height = 600;
    }
    if (data.last_advisor <= ADVISOR_NONE || data.last_advisor > ADVISOR_CHIEF) {
        data.last_advisor = ADVISOR_LABOR;
    }
}

void settings_save(void)
{
    buffer b;
    buffer *buf = &b;
    buffer_init(buf, data.inf_file, INF_SIZE);

    buffer_write_i32(buf, data.fullscreen);
    buffer_write_i32(buf, data.window_width);
    buffer_write_i32(buf, data.window_height);

    buffer_write_u8(buf, data.sound_effects.enabled);
    buffer_write_i32(buf, data.sound_effects.volume);
    buffer_write_u8(buf, data.sound_music.enabled);
    buffer_write_i32(buf, data.sound_music.volume);
    buffer_write_u8(buf, data.sound_speech.enabled);
    buffer_write_i32(buf, data.sound_speech.volume);
    buffer_write_u8(buf, data.sound_city.enabled);
    buffer_write_i32(buf, data.sound_city.volume);

    buffer_write_i32(buf, data.game_speed);
    buffer_write_i32(buf, data.scroll_speed);

    buffer_write_u8(buf, data.monthly_autosave);
    buffer_write_i32(buf, data.tooltips);
    buffer_write_u8(buf, data.warnings);
    buffer_write_i32(buf, data.victory_video);
    buffer_write_i32(buf, data.last_advisor);

    io_write_buffer_to_file(SETTINGS_FILE_PATH, data.inf_file, INF_SIZE);
}

int setting_fullscreen(void)
{
    return data.fullscreen;
}

void setting_window(int *width, int *height)
{
    *width = data.window_width;
    *height = data.window_height;
}

void setting_set_display(int fullscreen, int width, int height)
{
    data.fullscreen = fullscreen;
    if (!fullscreen) {
        data.window_width = width;
        data.window_height = height;
    }
}

static set_sound *get_sound(set_sound_type type)
{
    switch (type) {
        case SOUND_MUSIC: return &data.sound_music;
        case SOUND_EFFECTS: return &data.sound_effects;
        case SOUND_SPEECH: return &data.sound_speech;
        case SOUND_CITY: return &data.sound_city;
        default: return 0;
    }
}

const set_sound *setting_sound(set_sound_type type)
{
    return get_sound(type);
}

int setting_sound_is_enabled(set_sound_type type)
{
    return get_sound(type)->enabled;
}

void setting_toggle_sound_enabled(set_sound_type type)
{
    set_sound *sound = get_sound(type);
    sound->enabled = sound->enabled ? 0 : 1;
}

void setting_increase_sound_volume(set_sound_type type)
{
    set_sound *sound = get_sound(type);
    sound->volume = calc_bound(sound->volume + 1, 0, 100);
}

void setting_decrease_sound_volume(set_sound_type type)
{
    set_sound *sound = get_sound(type);
    sound->volume = calc_bound(sound->volume - 1, 0, 100);
}

void setting_reset_sound(set_sound_type type, int enabled, int volume)
{
    set_sound *sound = get_sound(type);
    sound->enabled = enabled;
    sound->volume = calc_bound(volume, 0, 100);
}

int setting_game_speed(void)
{
    return data.game_speed;
}

void setting_decrease_game_speed(void)
{
    if (data.game_speed > 100) {
        data.game_speed -= 100;
    } else {
        data.game_speed = calc_bound(data.game_speed - 10, 10, 100);
    }
}

void setting_increase_game_speed(void)
{
    if (data.game_speed >= 100) {
        if (data.game_speed < 500) {
            data.game_speed += 100;
        }
    } else {
        data.game_speed = calc_bound(data.game_speed + 10, 10, 100);
    }
}

int setting_scroll_speed(void)
{
    return data.scroll_speed;
}

void setting_increase_scroll_speed(void)
{
    data.scroll_speed = calc_bound(data.scroll_speed + 10, 0, 100);
}

void setting_decrease_scroll_speed(void)
{
    data.scroll_speed = calc_bound(data.scroll_speed - 10, 0, 100);
}

void setting_reset_speeds(int game_speed, int scroll_speed)
{
    data.game_speed = game_speed;
    data.scroll_speed = scroll_speed;
}

set_tooltips setting_tooltips(void)
{
    return data.tooltips;
}

void setting_cycle_tooltips(void)
{
    switch (data.tooltips) {
        case TOOLTIPS_NONE: data.tooltips = TOOLTIPS_SOME; break;
        case TOOLTIPS_SOME: data.tooltips = TOOLTIPS_FULL; break;
        default: data.tooltips = TOOLTIPS_NONE; break;
    }
}

int setting_warnings(void)
{
    return data.warnings;
}

void setting_toggle_warnings(void)
{
    data.warnings = data.warnings ? 0 : 1;
}

int setting_monthly_autosave(void)
{
    return data.monthly_autosave;
}

void setting_toggle_monthly_autosave(void)
{
    data.monthly_autosave = data.monthly_autosave ? 0 : 1;
}

int setting_victory_video(void)
{
    data.victory_video = data.victory_video ? 0 : 1;
    return data.victory_video;
}

int setting_last_advisor(void)
{
    return data.last_advisor;
}

void setting_set_last_advisor(int advisor)
{
    data.last_advisor = advisor;
}
