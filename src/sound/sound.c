#include "sound.h"

#include "building/building.h"
#include "city/data.h"
#include "core/time.h"
#include "figure/figure.h"
#include "game/game.h"
#include "platform/brutus.h"

#include "SDL.h"
#include "SDL_mixer.h"

#define MAX_CITY_SOUNDS_CHANNELS 70
#define MAX_DEVICE_CHANNELS 151
#define AUDIO_RATE 22050
#define AUDIO_FORMAT AUDIO_S16
#define AUDIO_CHANNELS 2
#define AUDIO_BUFFERS 1024

enum {
    SOUND_CHANNEL_CITY_VACANT_LOT = 46, // house_slum1 and house_slum2
    SOUND_CHANNEL_CITY_HOUSE_SLUM = 48,
    SOUND_CHANNEL_CITY_HOUSE_POOR = 50,
    SOUND_CHANNEL_CITY_HOUSE_MEDIUM = 54,
    SOUND_CHANNEL_CITY_HOUSE_GOOD = 58,
    SOUND_CHANNEL_CITY_HOUSE_POSH = 62,
    SOUND_CHANNEL_CITY_EMPTY_LAND = 66,
    SOUND_CHANNEL_CITY_RESERVOIR = 67,
    SOUND_CHANNEL_CITY_AQUEDUCT = 68,
    SOUND_CHANNEL_CITY_FOUNTAIN = 69,
    SOUND_CHANNEL_CITY_WELL = 70,
    SOUND_CHANNEL_CITY_BARBER = 71,
    SOUND_CHANNEL_CITY_BATHHOUSE = 72,
    SOUND_CHANNEL_CITY_CLINIC = 73,
    SOUND_CHANNEL_CITY_HOSPITAL = 74,
    SOUND_CHANNEL_CITY_TEMPLE_CERES = 75,
    SOUND_CHANNEL_CITY_TEMPLE_NEPTUNE = 76,
    SOUND_CHANNEL_CITY_TEMPLE_MERCURY = 78,
    SOUND_CHANNEL_CITY_TEMPLE_MARS = 79,
    SOUND_CHANNEL_CITY_TEMPLE_VENUS = 81,
    SOUND_CHANNEL_CITY_ORACLE = 82,
    SOUND_CHANNEL_CITY_SCHOOL = 83,
    SOUND_CHANNEL_CITY_ACADEMY = 84,
    SOUND_CHANNEL_CITY_LIBRARY = 85,
    SOUND_CHANNEL_CITY_THEATER = 86,
    SOUND_CHANNEL_CITY_AMPHITHEATER = 87,
    SOUND_CHANNEL_CITY_COLOSSEUM = 88,
    SOUND_CHANNEL_CITY_HIPPODROME = 89,
    SOUND_CHANNEL_CITY_GLADIATOR_SCHOOL = 90,
    SOUND_CHANNEL_CITY_LION_PIT = 91,
    SOUND_CHANNEL_CITY_ACTOR_COLONY = 92,
    SOUND_CHANNEL_CITY_CHARIOT_MAKER = 93,
    SOUND_CHANNEL_CITY_FORUM = 94,
    SOUND_CHANNEL_CITY_SENATE = 95,
    SOUND_CHANNEL_CITY_PALACE = 96,
    SOUND_CHANNEL_CITY_STATUE = 97,
    SOUND_CHANNEL_CITY_GARDEN = 98,
    SOUND_CHANNEL_CITY_SHIPYARD = 102,
    SOUND_CHANNEL_CITY_DOCK = 105,
    SOUND_CHANNEL_CITY_WHARF = 108,
    SOUND_CHANNEL_CITY_TOWER = 111,
    SOUND_CHANNEL_CITY_FORT = 115,
    SOUND_CHANNEL_CITY_BARRACKS = 119,
    SOUND_CHANNEL_CITY_MILITARY_ACADEMY = 120,
    SOUND_CHANNEL_CITY_WHEAT_FARM = 121,
    SOUND_CHANNEL_CITY_VEGETABLE_FARM = 123,
    SOUND_CHANNEL_CITY_FRUIT_FARM = 124,
    SOUND_CHANNEL_CITY_OLIVE_FARM = 125,
    SOUND_CHANNEL_CITY_VINE_FARM = 126,
    SOUND_CHANNEL_CITY_PIG_FARM = 127,
    SOUND_CHANNEL_CITY_CLAY_PIT = 128,
    SOUND_CHANNEL_CITY_QUARRY = 129,
    SOUND_CHANNEL_CITY_IRON_MINE = 130,
    SOUND_CHANNEL_CITY_TIMBER_YARD = 131,
    SOUND_CHANNEL_CITY_WINE_WORKSHOP = 132,
    SOUND_CHANNEL_CITY_OIL_WORKSHOP = 133,
    SOUND_CHANNEL_CITY_WEAPONS_WORKSHOP = 134,
    SOUND_CHANNEL_CITY_FURNITURE_WORKSHOP = 136,
    SOUND_CHANNEL_CITY_POTTERY_WORKSHOP = 138,
    SOUND_CHANNEL_CITY_MARKET = 140,
    SOUND_CHANNEL_CITY_GRANARY = 144,
    SOUND_CHANNEL_CITY_WAREHOUSE = 147,
    SOUND_CHANNEL_CITY_BURNING_RUIN = 150,
};

enum {
    TRACK_NONE = 0,
    TRACK_CITY_1 = 1,
    TRACK_CITY_2 = 2,
    TRACK_CITY_3 = 3,
    TRACK_CITY_4 = 4,
    TRACK_CITY_5 = 5,
    TRACK_COMBAT_SHORT = 6,
    TRACK_COMBAT_LONG = 7,
    TRACK_INTRO = 8,
    TRACK_MAX = 9
};

struct city_channel_t {
    int in_use;
    int available;
    int total_views;
    int views_threshold;
    int direction_views[5];
    int channel;
    int times_played;
    uint32_t last_played_time;
    uint32_t delay_millis;
    int should_play;
};

static struct city_channel_t channels[MAX_CITY_SOUNDS_CHANNELS];

static uint32_t last_update_time;

static struct {
    int current_track;
    int next_check;
} data = { TRACK_NONE, 0 };

static const char tracks[][32] = {
    "",
    "wavs/ROME1.WAV",
    "wavs/ROME2.WAV",
    "wavs/ROME3.WAV",
    "wavs/ROME4.WAV",
    "wavs/ROME5.WAV",
    "wavs/Combat_Short.wav",
    "wavs/Combat_Long.wav",
    "wavs/setup.wav"
};

static const char mp3_tracks[][32] = {
    "",
    "mp3/ROME1.mp3",
    "mp3/ROME2.mp3",
    "mp3/ROME3.mp3",
    "mp3/ROME4.mp3",
    "mp3/ROME5.mp3",
    "mp3/Combat_Short.mp3",
    "mp3/Combat_Long.mp3",
    "mp3/setup.mp3"
};

struct sound_channel_t {
    const char *filename;
    Mix_Chunk *chunk;
};

static struct {
    int initialized;
    Mix_Music *music;
    struct sound_channel_t channels[MAX_DEVICE_CHANNELS];
} data_channels;

static struct {
    SDL_AudioFormat dst_format;
    SDL_AudioStream *stream;
} custom_music;

void save_city_sounds_state(struct buffer_t *buf)
{
    for (int i = 0; i < MAX_CITY_SOUNDS_CHANNELS; i++) {
        const struct city_channel_t *ch = &channels[i];
        buffer_write_i32(buf, ch->available);
        buffer_write_i32(buf, ch->total_views);
        buffer_write_i32(buf, ch->views_threshold);
        for (int d = 0; d < 5; d++) {
            buffer_write_i32(buf, ch->direction_views[d]);
        }
        buffer_write_i32(buf, ch->channel);
        buffer_write_i32(buf, ch->in_use);
        buffer_write_i32(buf, ch->times_played);
        buffer_write_u32(buf, ch->last_played_time);
        buffer_write_u32(buf, ch->delay_millis);
        buffer_write_i32(buf, ch->should_play);
    }
}

void load_city_sounds_state(struct buffer_t *buf)
{
    for (int i = 0; i < MAX_CITY_SOUNDS_CHANNELS; i++) {
        struct city_channel_t *ch = &channels[i];
        ch->available = buffer_read_i32(buf);
        ch->total_views = buffer_read_i32(buf);
        ch->views_threshold = buffer_read_i32(buf);
        for (int d = 0; d < 5; d++) {
            ch->direction_views[d] = buffer_read_i32(buf);
        }
        ch->channel = buffer_read_i32(buf);
        ch->in_use = buffer_read_i32(buf);
        ch->times_played = buffer_read_i32(buf);
        ch->last_played_time = buffer_read_u32(buf);
        ch->delay_millis = buffer_read_u32(buf);
        ch->should_play = buffer_read_i32(buf);
    }
}

void initialize_city_sounds(void)
{
    last_update_time = time_get_millis();
    memset(channels, 0, MAX_CITY_SOUNDS_CHANNELS * sizeof(struct city_channel_t));
    for (int i = 0; i < MAX_CITY_SOUNDS_CHANNELS; i++) {
        channels[i].last_played_time = last_update_time;
    }
    for (int i = 1; i < MAX_CITY_SOUNDS_CHANNELS; i++) {
        channels[i].in_use = 1;
        channels[i].views_threshold = 200;
        channels[i].delay_millis = 1000;
    }
    channels[1].channel = SOUND_CHANNEL_CITY_VACANT_LOT;
    channels[2].channel = SOUND_CHANNEL_CITY_HOUSE_SLUM;
    channels[3].channel = SOUND_CHANNEL_CITY_HOUSE_POOR;
    channels[4].channel = SOUND_CHANNEL_CITY_HOUSE_MEDIUM;
    channels[5].channel = SOUND_CHANNEL_CITY_HOUSE_GOOD;
    channels[6].channel = SOUND_CHANNEL_CITY_HOUSE_POSH;
    channels[7].channel = SOUND_CHANNEL_CITY_EMPTY_LAND;
    channels[8].channel = SOUND_CHANNEL_CITY_RESERVOIR;
    channels[9].channel = SOUND_CHANNEL_CITY_AQUEDUCT;
    channels[10].channel = SOUND_CHANNEL_CITY_FOUNTAIN;
    channels[11].channel = SOUND_CHANNEL_CITY_WELL;
    channels[12].channel = SOUND_CHANNEL_CITY_BARBER;
    channels[13].channel = SOUND_CHANNEL_CITY_BATHHOUSE;
    channels[14].channel = SOUND_CHANNEL_CITY_CLINIC;
    channels[15].channel = SOUND_CHANNEL_CITY_HOSPITAL;
    channels[16].channel = SOUND_CHANNEL_CITY_TEMPLE_CERES;
    channels[17].channel = SOUND_CHANNEL_CITY_TEMPLE_NEPTUNE;
    channels[18].channel = SOUND_CHANNEL_CITY_TEMPLE_MERCURY;
    channels[19].channel = SOUND_CHANNEL_CITY_TEMPLE_MARS;
    channels[20].channel = SOUND_CHANNEL_CITY_TEMPLE_VENUS;
    channels[21].channel = SOUND_CHANNEL_CITY_ORACLE;
    channels[22].channel = SOUND_CHANNEL_CITY_SCHOOL;
    channels[23].channel = SOUND_CHANNEL_CITY_ACADEMY;
    channels[24].channel = SOUND_CHANNEL_CITY_LIBRARY;
    channels[25].channel = SOUND_CHANNEL_CITY_THEATER;
    channels[26].channel = SOUND_CHANNEL_CITY_AMPHITHEATER;
    channels[27].channel = SOUND_CHANNEL_CITY_COLOSSEUM;
    channels[28].channel = SOUND_CHANNEL_CITY_HIPPODROME;
    channels[29].channel = SOUND_CHANNEL_CITY_GLADIATOR_SCHOOL;
    channels[30].channel = SOUND_CHANNEL_CITY_LION_PIT;
    channels[31].channel = SOUND_CHANNEL_CITY_ACTOR_COLONY;
    channels[32].channel = SOUND_CHANNEL_CITY_CHARIOT_MAKER;
    channels[33].channel = SOUND_CHANNEL_CITY_FORUM;
    channels[34].channel = SOUND_CHANNEL_CITY_SENATE;
    channels[35].channel = SOUND_CHANNEL_CITY_PALACE;
    channels[36].channel = SOUND_CHANNEL_CITY_STATUE;
    channels[37].channel = SOUND_CHANNEL_CITY_GARDEN;
    channels[38].channel = SOUND_CHANNEL_CITY_SHIPYARD;
    channels[39].channel = SOUND_CHANNEL_CITY_DOCK;
    channels[40].channel = SOUND_CHANNEL_CITY_WHARF;
    channels[41].channel = SOUND_CHANNEL_CITY_TOWER;
    channels[42].channel = SOUND_CHANNEL_CITY_FORT;
    channels[43].channel = SOUND_CHANNEL_CITY_MILITARY_ACADEMY;
    channels[44].channel = SOUND_CHANNEL_CITY_BARRACKS;
    channels[45].channel = SOUND_CHANNEL_CITY_WHEAT_FARM;
    channels[46].channel = SOUND_CHANNEL_CITY_VEGETABLE_FARM;
    channels[47].channel = SOUND_CHANNEL_CITY_FRUIT_FARM;
    channels[48].channel = SOUND_CHANNEL_CITY_OLIVE_FARM;
    channels[49].channel = SOUND_CHANNEL_CITY_VINE_FARM;
    channels[50].channel = SOUND_CHANNEL_CITY_PIG_FARM;
    channels[51].channel = SOUND_CHANNEL_CITY_CLAY_PIT;
    channels[52].channel = SOUND_CHANNEL_CITY_QUARRY;
    channels[53].channel = SOUND_CHANNEL_CITY_IRON_MINE;
    channels[54].channel = SOUND_CHANNEL_CITY_TIMBER_YARD;
    channels[55].channel = SOUND_CHANNEL_CITY_WINE_WORKSHOP;
    channels[56].channel = SOUND_CHANNEL_CITY_OIL_WORKSHOP;
    channels[57].channel = SOUND_CHANNEL_CITY_WEAPONS_WORKSHOP;
    channels[58].channel = SOUND_CHANNEL_CITY_FURNITURE_WORKSHOP;
    channels[59].channel = SOUND_CHANNEL_CITY_POTTERY_WORKSHOP;
    channels[60].channel = SOUND_CHANNEL_CITY_MARKET;
    channels[61].channel = SOUND_CHANNEL_CITY_GRANARY;
    channels[62].channel = SOUND_CHANNEL_CITY_WAREHOUSE;
    channels[63].channel = SOUND_CHANNEL_CITY_BURNING_RUIN;
}

void set_city_sounds_volume(int percentage)
{
    for (int i = SOUND_CHANNEL_CITY_MIN; i <= SOUND_CHANNEL_CITY_MAX; i++) {
        set_channel_volume(i, percentage);
    }
}

void city_sounds__mark_building_view(int type, int num_workers, int direction)
{
    // mute city sounds during invasion
    if (city_data.figure.enemies || city_data.figure.imperial_soldiers) {
        return;
    }
    if (num_workers > 0
    || building_is_house(type)
    || type == BUILDING_RESERVOIR || type == BUILDING_AQUEDUCT || type == BUILDING_WELL || type == BUILDING_GARDENS
    || building_is_fort(type)) {
        int channel = building_properties[type].sound_channel;
        if (!channel) {
            return;
        }
        channels[channel].available = 1;
        ++channels[channel].total_views;
        ++channels[channel].direction_views[direction];
    }
}

void decay_city_sounds_views(void)
{
    for (int i = 0; i < MAX_CITY_SOUNDS_CHANNELS; i++) {
        for (int d = 0; d < 5; d++) {
            channels[i].direction_views[d] = 0;
        }
        channels[i].total_views /= 2;
    }
}

static int sound_device_is_channel_playing(int channel)
{
    return data_channels.channels[channel].chunk && Mix_Playing(channel);
}

static Mix_Chunk *load_chunk(const char *filename)
{
    if (filename[0]) {
        return Mix_LoadWAV(filename);
    } else {
        return NULL;
    }
}

static int load_channel(struct sound_channel_t *channel)
{
    if (!channel->chunk && channel->filename) {
        channel->chunk = load_chunk(channel->filename);
    }
    return channel->chunk ? 1 : 0;
}

static void play_channel(int channel, int direction)
{
    // allows using alternative building sounds that already exist in the game; index 3 means 4 sounds in the same group
    int sound_variety_index = 0;

    switch (channel) {

        case SOUND_CHANNEL_CITY_HOUSE_POOR:
        case SOUND_CHANNEL_CITY_HOUSE_MEDIUM:
        case SOUND_CHANNEL_CITY_HOUSE_GOOD:
        case SOUND_CHANNEL_CITY_HOUSE_POSH:
        case SOUND_CHANNEL_CITY_GARDEN:
        case SOUND_CHANNEL_CITY_FORT:
        case SOUND_CHANNEL_CITY_TOWER:
        case SOUND_CHANNEL_CITY_MARKET:
            sound_variety_index = 3;
            break;
        case SOUND_CHANNEL_CITY_GRANARY:
        case SOUND_CHANNEL_CITY_WAREHOUSE:
        case SOUND_CHANNEL_CITY_SHIPYARD:
        case SOUND_CHANNEL_CITY_DOCK:
        case SOUND_CHANNEL_CITY_WHARF:
            sound_variety_index = 2;
            break;
        case SOUND_CHANNEL_CITY_VACANT_LOT:
        case SOUND_CHANNEL_CITY_HOUSE_SLUM:
            sound_variety_index = 1;
        default:
            break;
    }

    if (!get_sound(SOUND_CITY)->enabled) {
        return;
    }
    if (sound_device_is_channel_playing(channel)) {
        return;
    }
    int left_pan;
    int right_pan;
    switch (direction) {
        case SOUND_DIRECTION_CENTER:
            left_pan = right_pan = 100;
            break;
        case SOUND_DIRECTION_LEFT:
            left_pan = 100;
            right_pan = 0;
            break;
        case SOUND_DIRECTION_RIGHT:
            left_pan = 0;
            right_pan = 100;
            break;
        default:
            left_pan = right_pan = 0;
            break;
    }
    if (data_channels.initialized) {
        int random_factor = 0;
        if (sound_variety_index) {
            random_factor = rand() % sound_variety_index;
            channel = channel + random_factor;
        }
        struct sound_channel_t *ch = &data_channels.channels[channel];
        if (load_channel(ch)) {
            Mix_SetPanning(channel, left_pan * 255 / 100, right_pan * 255 / 100);
            set_channel_volume(channel, get_sound(SOUND_CITY)->volume);
            Mix_PlayChannel(channel, ch->chunk, 0);
        }
    }
}

void play_city_sounds(void)
{
    uint32_t now = time_get_millis();
    for (int i = 1; i < MAX_CITY_SOUNDS_CHANNELS; i++) {
        channels[i].should_play = 0;
        if (channels[i].available) {
            channels[i].available = 0;
            if (channels[i].total_views >= channels[i].views_threshold) {
                if (now - channels[i].last_played_time >= channels[i].delay_millis) {
                    channels[i].should_play = 1;
                }
            }
        } else {
            channels[i].total_views = 0;
            for (int d = 0; d < 5; d++) {
                channels[i].direction_views[d] = 0;
            }
        }
    }

    if (now - last_update_time < 10000) {
        // Only play 1 sound every 10 seconds
        return;
    }
    uint32_t max_delay = 0;
    int max_sound_id = 0;
    for (int i = 1; i < MAX_CITY_SOUNDS_CHANNELS; i++) {
        if (channels[i].should_play) {
            if (now - channels[i].last_played_time > max_delay) {
                max_delay = now - channels[i].last_played_time;
                max_sound_id = i;
            }
        }
    }
    if (!max_sound_id) {
        return;
    }

    // always only one channel available... use it
    int channel = channels[max_sound_id].channel;
    int direction;
    if (channels[max_sound_id].direction_views[SOUND_DIRECTION_CENTER] > 10) {
        direction = SOUND_DIRECTION_CENTER;
    } else if (channels[max_sound_id].direction_views[SOUND_DIRECTION_LEFT] > 10) {
        direction = SOUND_DIRECTION_LEFT;
    } else if (channels[max_sound_id].direction_views[SOUND_DIRECTION_RIGHT] > 10) {
        direction = SOUND_DIRECTION_RIGHT;
    } else {
        direction = SOUND_DIRECTION_CENTER;
    }

    play_channel(channel, direction);
    last_update_time = now;
    channels[max_sound_id].last_played_time = now;
    channels[max_sound_id].total_views = 0;
    for (int d = 0; d < 5; d++) {
        channels[max_sound_id].direction_views[d] = 0;
    }
    channels[max_sound_id].times_played++;
}

void set_sound_effect_volume(int percentage)
{
    for (int i = SOUND_CHANNEL_EFFECTS_MIN; i <= SOUND_CHANNEL_EFFECTS_MAX; i++) {
        set_channel_volume(i, percentage);
    }
}

void play_sound_effect(int effect)
{
    if (!get_sound(SOUND_EFFECTS)->enabled) {
        return;
    }
    if (sound_device_is_channel_playing(effect)) {
        return;
    }
    if (data_channels.initialized) {
        struct sound_channel_t *ch = &data_channels.channels[effect];
        if (load_channel(ch)) {
            set_channel_volume(effect, get_sound(SOUND_EFFECTS)->volume);
            Mix_PlayChannel(effect, ch->chunk, 0);
        }
    }
}

void play_speech_file(const char *filename)
{
    if (!get_sound(SOUND_SPEECH)->enabled) {
        return;
    }
    stop_sound_channel(SOUND_CHANNEL_SPEECH);
    if (filename) {
        if (data_channels.initialized) {
            stop_sound_channel(SOUND_CHANNEL_SPEECH);
            data_channels.channels[SOUND_CHANNEL_SPEECH].chunk = load_chunk(filename);
            if (data_channels.channels[SOUND_CHANNEL_SPEECH].chunk) {
                set_channel_volume(SOUND_CHANNEL_SPEECH, get_sound(SOUND_SPEECH)->volume);
                Mix_PlayChannel(SOUND_CHANNEL_SPEECH, data_channels.channels[SOUND_CHANNEL_SPEECH].chunk, 0);
            }
        }
    }
}

static int play_music(const char *filename, int volume_pct)
{
    if (data_channels.initialized) {
        stop_music();
        if (!filename) {
            return 0;
        }
        data_channels.music = Mix_LoadMUS(filename);
        if (!data_channels.music) {
            SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                "Error opening music file '%s'. Reason: %s", filename, Mix_GetError());
        } else {
            if (Mix_PlayMusic(data_channels.music, -1) == -1) {
                data_channels.music = 0;
                SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
                    "Error playing music file '%s'. Reason: %s", filename, Mix_GetError());
            } else {
                Mix_VolumeMusic(percentage_to_volume(volume_pct));
            }
        }
        return data_channels.music ? 1 : 0;
    }
    return 0;
}

static void play_track(int track)
{
    stop_music();
    if (track <= TRACK_NONE || track >= TRACK_MAX) {
        return;
    }

    int volume = get_sound(SOUND_MUSIC)->volume;
    if (!play_music(mp3_tracks[track], volume)) {
        play_music(tracks[track], volume);
    }
    data.current_track = track;
}

void play_intro_music(void)
{
    if (get_sound(SOUND_MUSIC)->enabled) {
        play_track(TRACK_INTRO);
    }
}

void update_music(int force)
{
    if (data.next_check && !force) {
        --data.next_check;
        return;
    }
    if (!get_sound(SOUND_MUSIC)->enabled) {
        return;
    }
    int track;
    int total_enemies = city_figures_total_invading_enemies();
    if (total_enemies >= 32) {
        track = TRACK_COMBAT_LONG;
    } else if (total_enemies > 0) {
        track = TRACK_COMBAT_SHORT;
    } else if (city_data.population.population < 1000) {
        track = TRACK_CITY_1;
    } else if (city_data.population.population < 2000) {
        track = TRACK_CITY_2;
    } else if (city_data.population.population < 5000) {
        track = TRACK_CITY_3;
    } else if (city_data.population.population < 7000) {
        track = TRACK_CITY_4;
    } else {
        track = TRACK_CITY_5;
    }

    if (track == data.current_track) {
        return;
    }

    play_track(track);
    data.next_check = 10;
}

void stop_music(void)
{
    if (data_channels.initialized) {
        if (data_channels.music) {
            Mix_HaltMusic();
            Mix_FreeMusic(data_channels.music);
            data_channels.music = 0;
        }
    }
    data.current_track = TRACK_NONE;
    data.next_check = 0;
}

int percentage_to_volume(int percentage)
{
    return percentage * SDL_MIX_MAXVOLUME / 100;
}

static void init_channels(void)
{
    data_channels.initialized = 1;
    for (int i = 0; i < MAX_DEVICE_CHANNELS; i++) {
        data_channels.channels[i].chunk = 0;
    }
}

void open_sound_device(void)
{
    if (0 == Mix_OpenAudio(AUDIO_RATE, AUDIO_FORMAT, AUDIO_CHANNELS, AUDIO_BUFFERS)) {
        init_channels();
        return;
    }
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Sound failed to initialize using default driver: %s", Mix_GetError());
    // Try to work around SDL choosing the wrong driver on Windows sometimes
    for (int i = 0; i < SDL_GetNumAudioDrivers(); i++) {
        const char *driver_name = SDL_GetAudioDriver(i);
        if (SDL_strcmp(driver_name, "disk") == 0 || SDL_strcmp(driver_name, "dummy") == 0) {
            // Skip "write-to-disk" and dummy drivers
            continue;
        }
        if (0 == SDL_AudioInit(driver_name) &&
            0 == Mix_OpenAudio(AUDIO_RATE, AUDIO_FORMAT, AUDIO_CHANNELS, AUDIO_BUFFERS)) {
            SDL_Log("Using audio driver: %s", driver_name);
            init_channels();
            return;
        } else {
            SDL_Log("Not using audio driver %s, reason: %s", driver_name, SDL_GetError());
        }
    }
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Sound failed to initialize: %s", Mix_GetError());
    int max = SDL_GetNumAudioDevices(0);
    SDL_Log("Number of audio devices: %d", max);
    for (int i = 0; i < max; i++) {
        SDL_Log("Audio device: %s", SDL_GetAudioDeviceName(i, 0));
    }
}

void close_sound_device(void)
{
    if (data_channels.initialized) {
        for (int i = 0; i < MAX_DEVICE_CHANNELS; i++) {
            stop_sound_channel(i);
        }
        Mix_CloseAudio();
        data_channels.initialized = 0;
    }
}

void init_sound_device_channels(int num_channels, char filenames[][CHANNEL_FILENAME_MAX])
{
    if (data_channels.initialized) {
        if (num_channels > MAX_DEVICE_CHANNELS) {
            num_channels = MAX_DEVICE_CHANNELS;
        }
        Mix_AllocateChannels(num_channels);
        log_info("Loading audio files", 0, 0);
        for (int i = 0; i < num_channels; i++) {
            data_channels.channels[i].chunk = 0;
            data_channels.channels[i].filename = filenames[i][0] ? filenames[i] : 0;
        }
    }
}

void set_channel_volume(int channel, int volume_pct)
{
    if (data_channels.channels[channel].chunk) {
        Mix_VolumeChunk(data_channels.channels[channel].chunk, percentage_to_volume(volume_pct));
    }
}

void stop_sound_channel(int channel)
{
    if (data_channels.initialized) {
        struct sound_channel_t *ch = &data_channels.channels[channel];
        if (ch->chunk) {
            Mix_HaltChannel(channel);
            Mix_FreeChunk(ch->chunk);
            ch->chunk = 0;
        }
    }
}

static void free_custom_audio_stream(void)
{
    if (custom_music.stream) {
        SDL_FreeAudioStream(custom_music.stream);
        custom_music.stream = 0;
    }
    return;
}

static int create_custom_audio_stream(SDL_AudioFormat src_format, Uint8 src_channels, int src_rate,
                                      SDL_AudioFormat dst_format, Uint8 dst_channels, int dst_rate)
{
    free_custom_audio_stream();

    custom_music.dst_format = dst_format;

    custom_music.stream = SDL_NewAudioStream(
        src_format, src_channels, src_rate,
        dst_format, dst_channels, dst_rate
    );
    return custom_music.stream != 0;
}

static void custom_music_callback(__attribute__((unused)) void *dummy, Uint8 *stream, int len)
{
    // Write silence
    memset(stream, 0, len);

    if (len <= 0 || custom_music.stream == 0) {
        return;
    }
    int bytes_copied = 0;

    // Mix audio to sound effect volume
    Uint8 *mix_buffer = (Uint8 *) malloc(len);
    if (!mix_buffer) {
        return;
    }
    memset(mix_buffer, 0, len);

    bytes_copied = SDL_AudioStreamGet(custom_music.stream, mix_buffer, len);
    if (bytes_copied <= 0) {
        return;
    }

    SDL_MixAudioFormat(stream, mix_buffer,
        custom_music.dst_format, bytes_copied,
        percentage_to_volume(get_sound(SOUND_EFFECTS)->volume));
    free(mix_buffer);
}

void use_custom_music_player(int bitdepth, int num_channels, int rate, const unsigned char *audio_data, int len)
{
    SDL_AudioFormat format;
    if (bitdepth == 8) {
        format = AUDIO_U8;
    } else if (bitdepth == 16) {
        format = AUDIO_S16SYS;
    } else {
        log_error("Custom music bitdepth not supported:", 0, bitdepth);
        return;
    }
    int device_rate;
    Uint16 device_format;
    int device_channels;
    Mix_QuerySpec(&device_rate, &device_format, &device_channels);

    int result = create_custom_audio_stream(
        format, num_channels, rate,
        device_format, device_channels, device_rate
    );
    if (!result) {
        return;
    }

    write_custom_music_data(audio_data, len);

    Mix_HookMusic(custom_music_callback, 0);
}

void write_custom_music_data(const unsigned char *audio_data, int len)
{
    if (!audio_data || len <= 0 || custom_music.stream == 0) {
        return;
    }

    SDL_AudioStreamPut(custom_music.stream, audio_data, len);
}

void use_default_music_player(void)
{
    Mix_HookMusic(0, 0);
    free_custom_audio_stream();
}