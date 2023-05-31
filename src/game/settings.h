#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H

#include <stdint.h>

enum {
    SOUND_MUSIC = 1,
    SOUND_SPEECH = 2,
    SOUND_EFFECTS = 3,
    SOUND_CITY = 4,
};

struct set_sound_t {
    int enabled;
    int volume;
};

void settings_load(void);

void settings_save(void);

int setting_fullscreen(void);
void setting_window(int *width, int *height);
void setting_set_display(int fullscreen, int width, int height);

struct set_sound_t *setting_sound(int type);

void setting_toggle_sound_enabled(int type);
void setting_increase_sound_volume(int type);
void setting_decrease_sound_volume(int type);
void setting_reset_sound(int type, int enabled, int volume);

int setting_game_speed(void);
void setting_decrease_game_speed(void);
void setting_increase_game_speed(void);

int setting_scroll_speed(void);
void setting_increase_scroll_speed(void);
void setting_decrease_scroll_speed(void);
void setting_reset_speeds(int game_speed, int scroll_speed);

int setting_warnings(void);
void setting_toggle_warnings(void);

int setting_monthly_autosave(void);
void setting_toggle_monthly_autosave(void);

int setting_victory_video(void);

int setting_last_advisor(void);
void setting_set_last_advisor(int advisor);

#endif // GAME_SETTINGS_H
