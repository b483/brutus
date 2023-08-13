#ifndef SOUND_SOUND_H
#define SOUND_SOUND_H

#include "core/buffer.h"

enum {
    SOUND_CHANNEL_SPEECH = 0,

    // user interface effects
    SOUND_CHANNEL_EFFECTS_MIN = 1,
    SOUND_CHANNEL_EFFECTS_MAX = 44,

    // city sounds (from buildings)
    SOUND_CHANNEL_CITY_MIN = 45,
    SOUND_CHANNEL_CITY_MAX = 150,

    SOUND_CHANNEL_MAX = SOUND_CHANNEL_CITY_MAX + 1
};

enum {
    SOUND_DIRECTION_LEFT = 0,
    SOUND_DIRECTION_CENTER = 2,
    SOUND_DIRECTION_RIGHT = 4
};

void save_city_sounds_state(struct buffer_t *buf);

void load_city_sounds_state(struct buffer_t *buf);

void initialize_city_sounds(void);

void set_city_sounds_volume(int percentage);

void city_sounds__mark_building_view(int type, int num_workers, int direction);

void decay_city_sounds_views(void);

void play_city_sounds(void);

enum {
    SOUND_EFFECT_PANEL = 1,
    SOUND_EFFECT_SIDEBAR = 2,
    SOUND_EFFECT_ICON = 3,
    SOUND_EFFECT_BUILD = 4,
    SOUND_EFFECT_EXPLOSION = 5,
    SOUND_EFFECT_FANFARE = 6,
    SOUND_EFFECT_FANFARE_URGENT = 7,
    // battle effects
    SOUND_EFFECT_ARROW = 8,
    SOUND_EFFECT_ARROW_HIT = 9,
    SOUND_EFFECT_AXE = 10,
    SOUND_EFFECT_BALLISTA_SHOOT = 11,
    SOUND_EFFECT_BALLISTA_HIT_GROUND = 12,
    SOUND_EFFECT_BALLISTA_HIT_PERSON = 13,
    SOUND_EFFECT_CLUB = 14,
    SOUND_EFFECT_CAMEL = 15,
    SOUND_EFFECT_ELEPHANT = 16,
    SOUND_EFFECT_ELEPHANT_HIT = 17,
    SOUND_EFFECT_ELEPHANT_DIE = 18,
    SOUND_EFFECT_HORSE = 19,
    SOUND_EFFECT_HORSE2 = 20,
    SOUND_EFFECT_HORSE_MOVING = 21,
    SOUND_EFFECT_JAVELIN = 22,
    SOUND_EFFECT_LION_ATTACK = 23,
    SOUND_EFFECT_LION_DIE = 24,
    SOUND_EFFECT_HORN3 = 25,
    SOUND_EFFECT_SWORD = 26,
    SOUND_EFFECT_SWORD_SWING = 27,
    SOUND_EFFECT_LIGHT_SWORD = 28,
    SOUND_EFFECT_SPEAR = 29,
    SOUND_EFFECT_WOLF_ATTACK = 30,
    SOUND_EFFECT_WOLF_ATTACK_2 = 31,
    SOUND_EFFECT_WOLF_DIE = 32,
    SOUND_EFFECT_SOLDIER_DIE = 33, // 4x
    SOUND_EFFECT_CITIZEN_DIE = 37, // 4x
    SOUND_EFFECT_SHEEP_DIE = 41,
    SOUND_EFFECT_ZEBRA_DIE = 42,
    SOUND_EFFECT_WOLF_HOWL = 43,
    SOUND_EFFECT_FIRE_SPLASH = 44,
    SOUND_EFFECT_FORMATION_SHIELD = 45
};

void set_sound_effect_volume(int percentage);

void play_sound_effect(int effect);

void play_speech_file(const char *filename);

void play_intro_music(void);

void update_music(int force);

void stop_music(void);

#define CHANNEL_FILENAME_MAX 32

int percentage_to_volume(int percentage);

void open_sound_device(void);
void close_sound_device(void);

void init_sound_device_channels(int num_channels, char filenames[][CHANNEL_FILENAME_MAX]);

void set_channel_volume(int channel, int volume_pct);

void stop_sound_channel(int channel);

/**
 * Use a custom music player, for external music data (e.g. videos)
 * @param bitdepth Bitdepth, either 8 or 16
 * @param num_channels Number of channels, 1 = mono, 2 = stereo
 * @param rate Frequency, usually 22050 or 44100
 * @param data First chunk of music data
 * @param len Length of data
 */
void use_custom_music_player(int bitdepth, int num_channels, int rate,
                                          const unsigned char *data, int len);

/**
 * Writes custom music data
 * @param data Music data
 * @param len Length
 */
void write_custom_music_data(const unsigned char *data, int len);

void use_default_music_player(void);

#endif