#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H

#include <stdint.h>

typedef enum {
    TOOLTIPS_NONE = 0,
    TOOLTIPS_SOME = 1,
    TOOLTIPS_FULL = 2
} set_tooltips;

typedef enum {
    SOUND_MUSIC = 1,
    SOUND_SPEECH = 2,
    SOUND_EFFECTS = 3,
    SOUND_CITY = 4,
} set_sound_type;

typedef struct {
    int enabled;
    int volume;
} set_sound;

void settings_load(void);

void settings_save(void);

int setting_fullscreen(void);
void setting_window(int *width, int *height);
void setting_set_display(int fullscreen, int width, int height);

const set_sound *setting_sound(set_sound_type type);

int setting_sound_is_enabled(set_sound_type type);
void setting_toggle_sound_enabled(set_sound_type type);
void setting_increase_sound_volume(set_sound_type type);
void setting_decrease_sound_volume(set_sound_type type);
void setting_reset_sound(set_sound_type type, int enabled, int volume);

int setting_game_speed(void);
void setting_decrease_game_speed(void);
void setting_increase_game_speed(void);

int setting_scroll_speed(void);
void setting_increase_scroll_speed(void);
void setting_decrease_scroll_speed(void);
void setting_reset_speeds(int game_speed, int scroll_speed);

set_tooltips setting_tooltips(void);
void setting_cycle_tooltips(void);

int setting_warnings(void);
void setting_toggle_warnings(void);

int setting_monthly_autosave(void);
void setting_toggle_monthly_autosave(void);

int setting_victory_video(void);

int setting_last_advisor(void);
void setting_set_last_advisor(int advisor);

#endif // GAME_SETTINGS_H
