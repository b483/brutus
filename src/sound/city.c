#include "city.h"

#include "city/data_private.h"
#include "core/time.h"
#include "game/settings.h"
#include "sound/channel.h"
#include "sound/device.h"

#include <string.h>

#define MAX_CHANNELS 70

// for compatibility with the original game:
#define CITY_CHANNEL_OFFSET 16

enum {
    SOUND_CHANNEL_CITY_HOUSE_SLUM = 30,
    SOUND_CHANNEL_CITY_HOUSE_POOR = 34,
    SOUND_CHANNEL_CITY_HOUSE_MEDIUM = 38,
    SOUND_CHANNEL_CITY_HOUSE_GOOD = 42,
    SOUND_CHANNEL_CITY_HOUSE_POSH = 46,
    SOUND_CHANNEL_CITY_EMPTY_LAND = 50,
    SOUND_CHANNEL_CITY_RESERVOIR = 51,
    SOUND_CHANNEL_CITY_AQUEDUCT = 52,
    SOUND_CHANNEL_CITY_FOUNTAIN = 53,
    SOUND_CHANNEL_CITY_WELL = 54,
    SOUND_CHANNEL_CITY_BARBER = 55,
    SOUND_CHANNEL_CITY_BATHHOUSE = 56,
    SOUND_CHANNEL_CITY_CLINIC = 57,
    SOUND_CHANNEL_CITY_HOSPITAL = 58,
    SOUND_CHANNEL_CITY_TEMPLE_CERES = 59,
    SOUND_CHANNEL_CITY_TEMPLE_NEPTUNE = 60,
    SOUND_CHANNEL_CITY_TEMPLE_MERCURY = 62,
    SOUND_CHANNEL_CITY_TEMPLE_MARS = 63,
    SOUND_CHANNEL_CITY_TEMPLE_VENUS = 65,
    SOUND_CHANNEL_CITY_ORACLE = 66,
    SOUND_CHANNEL_CITY_SCHOOL = 67,
    SOUND_CHANNEL_CITY_ACADEMY = 68,
    SOUND_CHANNEL_CITY_LIBRARY = 69,
    SOUND_CHANNEL_CITY_THEATER = 70,
    SOUND_CHANNEL_CITY_AMPHITHEATER = 71,
    SOUND_CHANNEL_CITY_COLOSSEUM = 72,
    SOUND_CHANNEL_CITY_HIPPODROME = 73,
    SOUND_CHANNEL_CITY_GLADIATOR_SCHOOL = 74,
    SOUND_CHANNEL_CITY_LION_PIT = 75,
    SOUND_CHANNEL_CITY_ACTOR_COLONY = 76,
    SOUND_CHANNEL_CITY_CHARIOT_MAKER = 77,
    SOUND_CHANNEL_CITY_FORUM = 78,
    SOUND_CHANNEL_CITY_SENATE = 79,
    SOUND_CHANNEL_CITY_PALACE = 80,
    SOUND_CHANNEL_CITY_STATUE = 81,
    SOUND_CHANNEL_CITY_GARDEN = 82,
    SOUND_CHANNEL_CITY_SHIPYARD = 86,
    SOUND_CHANNEL_CITY_DOCK = 89,
    SOUND_CHANNEL_CITY_WHARF = 92,
    SOUND_CHANNEL_CITY_TOWER = 95,
    SOUND_CHANNEL_CITY_FORT = 99,
    SOUND_CHANNEL_CITY_BARRACKS = 103,
    SOUND_CHANNEL_CITY_MILITARY_ACADEMY = 104,
    SOUND_CHANNEL_CITY_WHEAT_FARM = 105,
    SOUND_CHANNEL_CITY_VEGETABLE_FARM = 107,
    SOUND_CHANNEL_CITY_FRUIT_FARM = 108,
    SOUND_CHANNEL_CITY_OLIVE_FARM = 109,
    SOUND_CHANNEL_CITY_VINE_FARM = 110,
    SOUND_CHANNEL_CITY_PIG_FARM = 111,
    SOUND_CHANNEL_CITY_CLAY_PIT = 112,
    SOUND_CHANNEL_CITY_QUARRY = 113,
    SOUND_CHANNEL_CITY_IRON_MINE = 114,
    SOUND_CHANNEL_CITY_TIMBER_YARD = 115,
    SOUND_CHANNEL_CITY_WINE_WORKSHOP = 116,
    SOUND_CHANNEL_CITY_OIL_WORKSHOP = 117,
    SOUND_CHANNEL_CITY_WEAPONS_WORKSHOP = 118,
    SOUND_CHANNEL_CITY_FURNITURE_WORKSHOP = 120,
    SOUND_CHANNEL_CITY_POTTERY_WORKSHOP = 122,
    SOUND_CHANNEL_CITY_MARKET = 124,
    SOUND_CHANNEL_CITY_GRANARY = 128,
    SOUND_CHANNEL_CITY_WAREHOUSE = 131,
    SOUND_CHANNEL_CITY_BURNING_RUIN = 134,
};

typedef struct {
    int in_use;
    int available;
    int total_views;
    int views_threshold;
    int direction_views[5];
    int channel;
    int times_played;
    time_millis last_played_time;
    time_millis delay_millis;
    int should_play;
} city_channel;

static city_channel channels[MAX_CHANNELS];

static const int BUILDING_TYPE_TO_CHANNEL_ID[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 8, 0, //0-9
    1, 1, 1, 1, 2, 2, 2, 2, 3, 3, //10-19
    3, 3, 4, 4, 4, 4, 5, 5, 5, 5, //20-29
    25, 24, 27, 26, 28, 29, 30, 31, 0, 36, //30-39
    41, 35, 35, 35, 41, 41, 13, 14, 12, 11, //40-49
    0, 21, 22, 23, 0, 0, 0, 41, 0, 40, //50-59
    15, 16, 17, 18, 19, 15, 16, 17, 18, 19, //60-69
    59, 60, 61, 0, 37, 38, 39, 0, 0, 34, //70-79
    0, 0, 0, 0, 33, 33, 32, 32, 0, 0, // 80-89
    7, 9, 10, 0, 43, 42, 0, 0, 20, 62, //90-99
    44, 45, 46, 47, 48, 49, 51, 52, 53, 50, //100-109
    54, 55, 56, 57, 58, 0, 0, 0, 0, 0, //110-119
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //120-129
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //130-139
    0, 0, 0, 0, 0, 0 //140-145
};

static time_millis last_update_time;

void sound_city_init(void)
{
    last_update_time = time_get_millis();
    memset(channels, 0, MAX_CHANNELS * sizeof(city_channel));
    for (int i = 0; i < MAX_CHANNELS; i++) {
        channels[i].last_played_time = last_update_time;
    }
    for (int i = 1; i < 63; i++) {
        channels[i].in_use = 1;
        channels[i].views_threshold = 200;
        channels[i].delay_millis = 30000;
    }
    channels[1].channel = SOUND_CHANNEL_CITY_HOUSE_SLUM;
    channels[2].channel = SOUND_CHANNEL_CITY_HOUSE_POOR;
    channels[3].channel = SOUND_CHANNEL_CITY_HOUSE_MEDIUM;
    channels[4].channel = SOUND_CHANNEL_CITY_HOUSE_GOOD;
    channels[5].channel = SOUND_CHANNEL_CITY_HOUSE_POSH;
    channels[6].channel = SOUND_CHANNEL_CITY_EMPTY_LAND;
    channels[7].channel = SOUND_CHANNEL_CITY_RESERVOIR;
    channels[8].channel = SOUND_CHANNEL_CITY_AQUEDUCT;
    channels[9].channel = SOUND_CHANNEL_CITY_FOUNTAIN;
    channels[10].channel = SOUND_CHANNEL_CITY_WELL;
    channels[11].channel = SOUND_CHANNEL_CITY_BARBER;
    channels[12].channel = SOUND_CHANNEL_CITY_BATHHOUSE;
    channels[13].channel = SOUND_CHANNEL_CITY_CLINIC;
    channels[14].channel = SOUND_CHANNEL_CITY_HOSPITAL;
    channels[15].channel = SOUND_CHANNEL_CITY_TEMPLE_CERES;
    channels[16].channel = SOUND_CHANNEL_CITY_TEMPLE_NEPTUNE;
    channels[17].channel = SOUND_CHANNEL_CITY_TEMPLE_MERCURY;
    channels[18].channel = SOUND_CHANNEL_CITY_TEMPLE_MARS;
    channels[19].channel = SOUND_CHANNEL_CITY_TEMPLE_VENUS;
    channels[20].channel = SOUND_CHANNEL_CITY_ORACLE;
    channels[21].channel = SOUND_CHANNEL_CITY_SCHOOL;
    channels[22].channel = SOUND_CHANNEL_CITY_ACADEMY;
    channels[23].channel = SOUND_CHANNEL_CITY_LIBRARY;
    channels[24].channel = SOUND_CHANNEL_CITY_THEATER;
    channels[25].channel = SOUND_CHANNEL_CITY_AMPHITHEATER;
    channels[26].channel = SOUND_CHANNEL_CITY_COLOSSEUM;
    channels[27].channel = SOUND_CHANNEL_CITY_HIPPODROME;
    channels[28].channel = SOUND_CHANNEL_CITY_GLADIATOR_SCHOOL;
    channels[29].channel = SOUND_CHANNEL_CITY_LION_PIT;
    channels[30].channel = SOUND_CHANNEL_CITY_ACTOR_COLONY;
    channels[31].channel = SOUND_CHANNEL_CITY_CHARIOT_MAKER;
    channels[32].channel = SOUND_CHANNEL_CITY_FORUM;
    channels[33].channel = SOUND_CHANNEL_CITY_SENATE;
    channels[34].channel = SOUND_CHANNEL_CITY_PALACE;
    channels[35].channel = SOUND_CHANNEL_CITY_STATUE;
    channels[36].channel = SOUND_CHANNEL_CITY_GARDEN;
    channels[37].channel = SOUND_CHANNEL_CITY_SHIPYARD;
    channels[38].channel = SOUND_CHANNEL_CITY_DOCK;
    channels[39].channel = SOUND_CHANNEL_CITY_WHARF;
    channels[40].channel = SOUND_CHANNEL_CITY_TOWER;
    channels[41].channel = SOUND_CHANNEL_CITY_FORT;
    channels[42].channel = SOUND_CHANNEL_CITY_BARRACKS;
    channels[43].channel = SOUND_CHANNEL_CITY_MILITARY_ACADEMY;
    channels[44].channel = SOUND_CHANNEL_CITY_WHEAT_FARM;
    channels[45].channel = SOUND_CHANNEL_CITY_VEGETABLE_FARM;
    channels[46].channel = SOUND_CHANNEL_CITY_FRUIT_FARM;
    channels[47].channel = SOUND_CHANNEL_CITY_OLIVE_FARM;
    channels[48].channel = SOUND_CHANNEL_CITY_VINE_FARM;
    channels[49].channel = SOUND_CHANNEL_CITY_PIG_FARM;
    channels[50].channel = SOUND_CHANNEL_CITY_CLAY_PIT;
    channels[51].channel = SOUND_CHANNEL_CITY_QUARRY;
    channels[52].channel = SOUND_CHANNEL_CITY_IRON_MINE;
    channels[53].channel = SOUND_CHANNEL_CITY_TIMBER_YARD;
    channels[54].channel = SOUND_CHANNEL_CITY_WINE_WORKSHOP;
    channels[55].channel = SOUND_CHANNEL_CITY_OIL_WORKSHOP;
    channels[56].channel = SOUND_CHANNEL_CITY_WEAPONS_WORKSHOP;
    channels[57].channel = SOUND_CHANNEL_CITY_FURNITURE_WORKSHOP;
    channels[58].channel = SOUND_CHANNEL_CITY_POTTERY_WORKSHOP;
    channels[59].channel = SOUND_CHANNEL_CITY_MARKET;
    channels[60].channel = SOUND_CHANNEL_CITY_GRANARY;
    channels[61].channel = SOUND_CHANNEL_CITY_WAREHOUSE;
    channels[62].channel = SOUND_CHANNEL_CITY_BURNING_RUIN;
}

void sound_city_set_volume(int percentage)
{
    for (int i = SOUND_CHANNEL_CITY_MIN; i <= SOUND_CHANNEL_CITY_MAX; i++) {
        sound_device_set_channel_volume(i, percentage);
    }
}
void sound_city_mark_building_view(building_type type, int num_workers, int direction)
{
    if (num_workers > 0 || type == BUILDING_RESERVOIR || type == BUILDING_AQUEDUCT || type == BUILDING_GARDENS || type == BUILDING_FORT || building_is_house(type)) {
        // mute city sounds during invasion
        if (city_data.figure.enemies) {
            return;
        }

        int channel = BUILDING_TYPE_TO_CHANNEL_ID[type];
        if (!channel) {
            return;
        }
        channels[channel].available = 1;
        ++channels[channel].total_views;
        ++channels[channel].direction_views[direction];
    }
}

void sound_city_decay_views(void)
{
    for (int i = 0; i < MAX_CHANNELS; i++) {
        for (int d = 0; d < 5; d++) {
            channels[i].direction_views[d] = 0;
        }
        channels[i].total_views /= 2;
    }
}

static void play_channel(int channel, int direction)
{
    // allows using alternative building sounds that already exist in the game; index 3 means 4 sounds in the same group
    int sound_variety_index = 0;

    switch (channel) {
        case SOUND_CHANNEL_CITY_HOUSE_SLUM:
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
        default:
            break;
    }

    channel += CITY_CHANNEL_OFFSET;
    if (!setting_sound(SOUND_CITY)->enabled) {
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
    sound_device_play_channel_panned(channel, sound_variety_index, setting_sound(SOUND_CITY)->volume, left_pan, right_pan);
}

void sound_city_play(void)
{
    time_millis now = time_get_millis();
    for (int i = 1; i < MAX_CHANNELS; i++) {
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

    if (now - last_update_time < 2000) {
        // Only play 1 sound every 2 seconds
        return;
    }
    time_millis max_delay = 0;
    int max_sound_id = 0;
    for (int i = 1; i < MAX_CHANNELS; i++) {
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

void sound_city_save_state(buffer *buf)
{
    for (int i = 0; i < MAX_CHANNELS; i++) {
        const city_channel *ch = &channels[i];
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

void sound_city_load_state(buffer *buf)
{
    for (int i = 0; i < MAX_CHANNELS; i++) {
        city_channel *ch = &channels[i];
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
