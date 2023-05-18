#include "city.h"

#include "city/data_private.h"
#include "core/time.h"
#include "game/settings.h"
#include "sound/channel.h"
#include "sound/device.h"

#include <string.h>

#define MAX_CHANNELS 70

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
0,  // BUILDING_NONE
1,  // BUILDING_HOUSE_VACANT_LOT
2,  // BUILDING_HOUSE_SMALL_TENT
2,  // BUILDING_HOUSE_LARGE_TENT
2,  // BUILDING_HOUSE_SMALL_SHACK
2,  // BUILDING_HOUSE_LARGE_SHACK
2,  // BUILDING_HOUSE_SMALL_HOVEL
2,  // BUILDING_HOUSE_LARGE_HOVEL
3,  // BUILDING_HOUSE_SMALL_CASA
3,  // BUILDING_HOUSE_LARGE_CASA
3,  // BUILDING_HOUSE_SMALL_INSULA
3,  // BUILDING_HOUSE_MEDIUM_INSULA
4,  // BUILDING_HOUSE_LARGE_INSULA
4,  // BUILDING_HOUSE_GRAND_INSULA
4,  // BUILDING_HOUSE_SMALL_VILLA
4,  // BUILDING_HOUSE_MEDIUM_VILLA
5,  // BUILDING_HOUSE_LARGE_VILLA
5,  // BUILDING_HOUSE_GRAND_VILLA
5,  // BUILDING_HOUSE_SMALL_PALACE
5,  // BUILDING_HOUSE_MEDIUM_PALACE
6,  // BUILDING_HOUSE_LARGE_PALACE
6,  // BUILDING_HOUSE_LUXURY_PALACE
0,  // BUILDING_CLEAR_LAND
0,  // BUILDING_ROAD
8,  // BUILDING_RESERVOIR
9,  // BUILDING_AQUEDUCT
10,  // BUILDING_FOUNTAIN
11,  // BUILDING_WELL
12,  // BUILDING_BARBER
13,  // BUILDING_BATHHOUSE
14,  // BUILDING_DOCTOR
15,  // BUILDING_HOSPITAL
0,  // BUILDING_MENU_SMALL_TEMPLES
16,  // BUILDING_SMALL_TEMPLE_CERES
17,  // BUILDING_SMALL_TEMPLE_NEPTUNE
18,  // BUILDING_SMALL_TEMPLE_MERCURY
19,  // BUILDING_SMALL_TEMPLE_MARS
20,  // BUILDING_SMALL_TEMPLE_VENUS
0,  // BUILDING_MENU_LARGE_TEMPLES
16,  // BUILDING_LARGE_TEMPLE_CERES
17,  // BUILDING_LARGE_TEMPLE_NEPTUNE
18,  // BUILDING_LARGE_TEMPLE_MERCURY
19,  // BUILDING_LARGE_TEMPLE_MARS
20,  // BUILDING_LARGE_TEMPLE_VENUS
21,  // BUILDING_ORACLE
22,  // BUILDING_SCHOOL
23,  // BUILDING_ACADEMY
24,  // BUILDING_LIBRARY
0,  // BUILDING_MISSION_POST
25,  // BUILDING_THEATER
26,  // BUILDING_AMPHITHEATER
27,  // BUILDING_COLOSSEUM
28,  // BUILDING_HIPPODROME
29,  // BUILDING_GLADIATOR_SCHOOL
30,  // BUILDING_LION_HOUSE
31,  // BUILDING_ACTOR_COLONY
32,  // BUILDING_CHARIOT_MAKER
33,  // BUILDING_FORUM
34,  // BUILDING_SENATE
35,  // BUILDING_GOVERNORS_HOUSE
35,  // BUILDING_GOVERNORS_VILLA
35,  // BUILDING_GOVERNORS_PALACE
36,  // BUILDING_SMALL_STATUE
36,  // BUILDING_MEDIUM_STATUE
36,  // BUILDING_LARGE_STATUE
0,  // BUILDING_TRIUMPHAL_ARCH
37,  // BUILDING_GARDENS
0,  // BUILDING_PLAZA
0,  // BUILDING_ENGINEERS_POST
0,  // BUILDING_LOW_BRIDGE
0,  // BUILDING_SHIP_BRIDGE
38,  // BUILDING_SHIPYARD
39,  // BUILDING_DOCK
40,  // BUILDING_WHARF
0,  // BUILDING_WALL
41,  // BUILDING_TOWER
0,  // BUILDING_GATEHOUSE
0,  // BUILDING_PREFECTURE
42,  // BUILDING_FORT
0,  // BUILDING_FORT_LEGIONARIES
0,  // BUILDING_FORT_JAVELIN
0,  // BUILDING_FORT_MOUNTED
43,  // BUILDING_MILITARY_ACADEMY
44,  // BUILDING_BARRACKS
0,  // BUILDING_MENU_FARMS
45,  // BUILDING_WHEAT_FARM
46,  // BUILDING_VEGETABLE_FARM
47,  // BUILDING_FRUIT_FARM
48,  // BUILDING_OLIVE_FARM
49,  // BUILDING_VINES_FARM
50,  // BUILDING_PIG_FARM
0,  // BUILDING_MENU_RAW_MATERIALS
51,  // BUILDING_CLAY_PIT
52,  // BUILDING_MARBLE_QUARRY
53,  // BUILDING_IRON_MINE
54,  // BUILDING_TIMBER_YARD
0,  // BUILDING_MENU_WORKSHOPS
55,  // BUILDING_WINE_WORKSHOP
56,  // BUILDING_OIL_WORKSHOP
57,  // BUILDING_WEAPONS_WORKSHOP
58,  // BUILDING_FURNITURE_WORKSHOP
59,  // BUILDING_POTTERY_WORKSHOP
60,  // BUILDING_MARKET
61,  // BUILDING_GRANARY
62,  // BUILDING_WAREHOUSE
0,  // BUILDING_WAREHOUSE_SPACE
0,  // BUILDING_NATIVE_HUT
0,  // BUILDING_NATIVE_MEETING
0,  // BUILDING_NATIVE_CROPS
0,  // BUILDING_FORT_GROUND
63,  // BUILDING_BURNING_RUIN
};

static time_millis last_update_time;

void sound_city_init(void)
{
    // last_update_time = time_get_millis();
    memset(channels, 0, MAX_CHANNELS * sizeof(city_channel));
    for (int i = 0; i < MAX_CHANNELS; i++) {
        channels[i].last_played_time = last_update_time;
    }
    for (int i = 1; i < MAX_CHANNELS; i++) {
        channels[i].in_use = 1;
        channels[i].views_threshold = 200;
        channels[i].delay_millis = 100;
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

void sound_city_set_volume(int percentage)
{
    for (int i = SOUND_CHANNEL_CITY_MIN; i <= SOUND_CHANNEL_CITY_MAX; i++) {
        sound_device_set_channel_volume(i, percentage);
    }
}

void sound_city_mark_building_view(building_type type, int num_workers, int direction)
{
    // mute city sounds during invasion
    if (city_data.figure.enemies || city_data.figure.imperial_soldiers) {
        return;
    }
    if (num_workers > 0 || type == BUILDING_RESERVOIR || type == BUILDING_AQUEDUCT || type == BUILDING_WELL || type == BUILDING_GARDENS || type == BUILDING_FORT || building_is_house(type)) {
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

    if (now - last_update_time < 10000) {
        // Only play 1 sound every 10 seconds
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
