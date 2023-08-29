#include "building.h"

#include "city/city_new.h"
#include "core/calc.h"
#include "core/config.h"
#include "core/image.h"
#include "core/random.h"
#include "core/string.h"
#include "core/time.h"
#include "empire/empire.h"
#include "figure/figure.h"
#include "figure/formation_herd.h"
#include "figure/formation_legion.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "figure/trader.h"
#include "figuretype/migrant.h"
#include "figuretype/trader.h"
#include "figuretype/wall.h"
#include "game/game.h"
#include "map/map.h"
#include "figuretype/missile.h"
#include "platform/brutus.h"
#include "scenario/scenario.h"
#include "sound/sound.h"
#include "window/window.h"

#include <string.h>

#define MAX_STORAGES 200

#define MAX_GRANARIES 100

#define MAX_SMALL 500
#define MAX_LARGE 2000
#define MAX_BURNING 500

#define MIN_Y_POSITION 32
#define MARGIN_POSITION 16
#define SOUND_FILENAME_MAX 32

struct building_t all_buildings[MAX_BUILDINGS];

static struct {
    int highest_id_in_use;
    int highest_id_ever;
    int created_sequence;
    int incorrect_houses;
    int unfixable_houses;
} extra = { 0, 0, 0, 0, 0 };

struct building_properties_t building_properties[BUILDING_TYPE_MAX] = {
    // size | fireprf | img_grp | img_off | cost | des_value | des_step | des_step_siz | des_range | n_lbrs | lbr_cat | snd_ch
    {  0,     0,        0,        0,        0,     0,          0,         0,             0,          0,      -1,        0}, // BUILDING_NONE
    {  1,     0,        0,        0,        10,    0,          0,         0,             0,          0,      -1,        1}, // BUILDING_HOUSE_VACANT_LOT
    {  1,     0,        0,        0,        0,    -3,          1,         1,             3,          0,      -1,        2}, // BUILDING_HOUSE_SMALL_TENT
    {  1,     0,        0,        0,        0,    -3,          1,         1,             3,          0,      -1,        2}, // BUILDING_HOUSE_LARGE_TENT
    {  1,     0,        0,        0,        0,    -2,          1,         1,             2,          0,      -1,        2}, // BUILDING_HOUSE_SMALL_SHACK
    {  1,     0,        0,        0,        0,    -2,          1,         1,             2,          0,      -1,        2}, // BUILDING_HOUSE_LARGE_SHACK
    {  1,     0,        0,        0,        0,    -2,          1,         1,             2,          0,      -1,        2}, // BUILDING_HOUSE_SMALL_HOVEL
    {  1,     0,        0,        0,        0,    -2,          1,         1,             2,          0,      -1,        2}, // BUILDING_HOUSE_LARGE_HOVEL
    {  1,     0,        0,        0,        0,    -1,          1,         1,             1,          0,      -1,        3}, // BUILDING_HOUSE_SMALL_CASA
    {  1,     0,        0,        0,        0,    -1,          1,         1,             1,          0,      -1,        3}, // BUILDING_HOUSE_LARGE_CASA
    {  1,     0,        0,        0,        0,     0,          1,         1,             1,          0,      -1,        3}, // BUILDING_HOUSE_SMALL_INSULA
    {  1,     0,        0,        0,        0,     0,          1,         1,             1,          0,      -1,        3}, // BUILDING_HOUSE_MEDIUM_INSULA
    {  2,     0,        0,        0,        0,     0,          0,         0,             0,          0,      -1,        4}, // BUILDING_HOUSE_LARGE_INSULA
    {  2,     0,        0,        0,        0,     0,          0,         0,             0,          0,      -1,        4}, // BUILDING_HOUSE_GRAND_INSULA
    {  2,     0,        0,        0,        0,     1,          2,        -1,             2,          0,      -1,        4}, // BUILDING_HOUSE_SMALL_VILLA
    {  2,     0,        0,        0,        0,     1,          2,        -1,             2,          0,      -1,        4}, // BUILDING_HOUSE_MEDIUM_VILLA
    {  3,     0,        0,        0,        0,     2,          2,        -2,             2,          0,      -1,        5}, // BUILDING_HOUSE_LARGE_VILLA
    {  3,     0,        0,        0,        0,     2,          2,        -2,             2,          0,      -1,        5}, // BUILDING_HOUSE_GRAND_VILLA
    {  3,     0,        0,        0,        0,     3,          2,        -1,             6,          0,      -1,        5}, // BUILDING_HOUSE_SMALL_PALACE
    {  3,     0,        0,        0,        0,     3,          2,        -1,             6,          0,      -1,        5}, // BUILDING_HOUSE_MEDIUM_PALACE
    {  4,     0,        0,        0,        0,     4,          2,        -1,             6,          0,      -1,        6}, // BUILDING_HOUSE_LARGE_PALACE
    {  4,     0,        0,        0,        0,     4,          2,        -1,             6,          0,      -1,        6}, // BUILDING_HOUSE_LUXURY_PALACE
    {  0,     0,        0,        0,        2,     0,          0,         0,             0,          0,      -1,        0}, // BUILDING_CLEAR_LAND
    {  1,     0,        112,      0,        4,     0,          0,         0,             0,          0,      -1,        0}, // BUILDING_ROAD
    {  3,     1,        25,       0,        80,   -6,          1,         2,             3,          0,      -1,        8}, // BUILDING_RESERVOIR
    {  1,     0,        19,       0,        8,    -2,          1,         1,             2,          0,      -1,        9}, // BUILDING_AQUEDUCT
    {  1,     1,        54,       0,        15,    0,          0,         0,             0,          4,       3,        10}, // BUILDING_FOUNTAIN
    {  1,     1,        23,       0,        5,     0,          0,         0,             0,          0,      -1,        11}, // BUILDING_WELL
    {  1,     0,        68,       0,        30,    0,          0,         0,             0,          5,       7,        14}, // BUILDING_DOCTOR
    {  2,     0,        185,      0,        50,    4,          1,        -1,             4,          10,      7,        13}, // BUILDING_BATHHOUSE
    {  1,     0,        67,       0,        25,    2,          1,        -1,             2,          2,       7,        12}, // BUILDING_BARBER
    {  3,     0,        70,       0,        300,  -1,          2,         1,             2,          30,      7,        15}, // BUILDING_HOSPITAL
    {  2,     0,        71,       0,        50,    4,          2,        -1,             6,          2,       8,        16}, // BUILDING_SMALL_TEMPLE_CERES
    {  2,     0,        72,       0,        50,    4,          2,        -1,             6,          2,       8,        17}, // BUILDING_SMALL_TEMPLE_NEPTUNE
    {  2,     0,        73,       0,        50,    4,          2,        -1,             6,          2,       8,        18}, // BUILDING_SMALL_TEMPLE_MERCURY
    {  2,     0,        74,       0,        50,    4,          2,        -1,             6,          2,       8,        19}, // BUILDING_SMALL_TEMPLE_MARS
    {  2,     0,        75,       0,        50,    4,          2,        -1,             6,          2,       8,        20}, // BUILDING_SMALL_TEMPLE_VENUS
    {  3,     0,        71,       1,        150,   8,          2,        -1,             8,          5,       8,        16}, // BUILDING_LARGE_TEMPLE_CERES
    {  3,     0,        72,       1,        150,   8,          2,        -1,             8,          5,       8,        17}, // BUILDING_LARGE_TEMPLE_NEPTUNE
    {  3,     0,        73,       1,        150,   8,          2,        -1,             8,          5,       8,        18}, // BUILDING_LARGE_TEMPLE_MERCURY
    {  3,     0,        74,       1,        150,   8,          2,        -1,             8,          5,       8,        19}, // BUILDING_LARGE_TEMPLE_MARS
    {  3,     0,        75,       1,        150,   8,          2,        -1,             8,          5,       8,        20}, // BUILDING_LARGE_TEMPLE_VENUS
    {  2,     0,        76,       0,        200,   8,          2,        -1,             6,          0,       8,        21}, // BUILDING_ORACLE
    {  2,     0,        41,       0,        50,   -2,          1,         1,             2,          10,      7,        22}, // BUILDING_SCHOOL
    {  2,     0,        42,       0,        75,    4,          1,        -1,             4,          20,      7,        24}, // BUILDING_LIBRARY
    {  3,     0,        43,       0,        100,   4,          1,         1,             4,          30,      7,        23}, // BUILDING_ACADEMY
    {  2,     1,        184,      0,        100,  -3,          1,         1,             2,          20,      7,        0}, // BUILDING_MISSION_POST
    {  2,     0,        46,       0,        50,    2,          1,        -1,             2,          8,       6,        25}, // BUILDING_THEATER
    {  3,     0,        51,       0,        50,    2,          1,        -1,             2,          5,       6,        31}, // BUILDING_ACTOR_COLONY
    {  3,     0,        45,       0,        100,   4,          1,        -1,             4,          12,      6,        26}, // BUILDING_AMPHITHEATER
    {  3,     0,        49,       0,        75,   -3,          1,         1,             3,          8,       6,        29}, // BUILDING_GLADIATOR_SCHOOL
    {  3,     0,        50,       0,        75,   -3,          1,         1,             3,          8,       6,        30}, // BUILDING_LION_HOUSE
    {  5,     0,        48,       0,        500,  -3,          1,         1,             3,          25,      6,        27}, // BUILDING_COLOSSEUM
    {  3,     0,        52,       0,        75,   -3,          1,         1,             3,          8,       6,        32}, // BUILDING_CHARIOT_MAKER
    {  5,     0,        213,      0,        2500, -3,          1,         1,             3,          150,     6,        28}, // BUILDING_HIPPODROME
    {  1,     1,        59,       0,        12,    3,          1,        -1,             3,          0,      -1,        37}, // BUILDING_GARDENS
    {  1,     1,        58,       0,        15,    4,          1,        -2,             2,          0,      -1,        0}, // BUILDING_PLAZA
    {  1,     1,        61,       0,        12,    3,          1,        -1,             3,          0,      -1,        36}, // BUILDING_SMALL_STATUE
    {  2,     1,        61,       1,        60,    10,         1,        -2,             4,          0,      -1,        36}, // BUILDING_MEDIUM_STATUE
    {  3,     1,        61,       2,        150,   14,         2,        -2,             5,          0,      -1,        36}, // BUILDING_LARGE_STATUE
    {  3,     0,        85,       0,        150,   12,         2,        -2,             3,          0,      -1,        35}, // BUILDING_GOVERNORS_HOUSE
    {  4,     0,        86,       0,        400,   20,         2,        -3,             4,          0,      -1,        35}, // BUILDING_GOVERNORS_VILLA
    {  5,     0,        87,       0,        700,   28,         2,        -4,             5,          0,      -1,        35}, // BUILDING_GOVERNORS_PALACE
    {  2,     0,        63,       0,        75,    3,          2,        -1,             2,          6,       8,        33}, // BUILDING_FORUM
    {  5,     0,        62,       0,        400,   14,         2,        -1,             8,          30,      8,        34}, // BUILDING_SENATE
    {  3,     1,        205,      0,        0,     18,         2,         3,             5,          0,      -1,        0}, // BUILDING_TRIUMPHAL_ARCH
    {  1,     1,        81,       0,        30,    0,          1,         1,             1,          5,       2,        0}, // BUILDING_ENGINEERS_POST
    {  1,     1,        0,        0,        40,    0,          0,         0,             0,          0,      -1,        0}, // BUILDING_LOW_BRIDGE
    {  1,     1,        0,        0,        100,   0,          0,         0,             0,          0,      -1,        0}, // BUILDING_SHIP_BRIDGE
    {  2,     0,        77,       0,        100,  -8,          2,         2,             3,          10,      0,        38}, // BUILDING_SHIPYARD
    {  2,     0,        79,       0,        60,   -8,          2,         2,             3,          6,       0,        40}, // BUILDING_WHARF
    {  3,     0,        78,       0,        100,  -8,          2,         2,             3,          12,      0,        39}, // BUILDING_DOCK
    {  1,     0,        64,       0,        30,   -2,          1,         1,             2,          6,       4,        0}, // BUILDING_PREFECTURE
    {  1,     0,        24,       26,       12,    0,          0,         0,             0,          0,      -1,        0}, // BUILDING_WALL
    {  2,     1,        17,       0,        150,  -8,          1,         2,             3,          6,       5,        41}, // BUILDING_TOWER
    {  2,     1,        17,       1,        100,  -4,          1,         1,             3,          0,       5,        0}, // BUILDING_GATEHOUSE
    {  3,     1,        66,       0,        1000, -20,         2,         2,             8,          0,      -1,        0}, // BUILDING_FORT_LEGIONARIES
    {  3,     1,        66,       0,        1000, -20,         2,         2,             8,          0,      -1,        0}, // BUILDING_FORT_JAVELIN
    {  3,     1,        66,       0,        1000, -20,         2,         2,             8,          0,      -1,        0}, // BUILDING_FORT_MOUNTED
    {  3,     0,        166,      0,        150,  -6,          1,         1,             3,          10,      5,        44}, // BUILDING_BARRACKS
    {  3,     0,        201,      0,        1000, -3,          1,         1,             3,          20,      5,        43}, // BUILDING_MILITARY_ACADEMY
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10,      1,        45}, // BUILDING_WHEAT_FARM
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10,      1,        46}, // BUILDING_VEGETABLE_FARM
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10,      1,        47}, // BUILDING_FRUIT_FARM
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10,      1,        50}, // BUILDING_PIG_FARM
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10,      0,        48}, // BUILDING_OLIVE_FARM
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10,      0,        49}, // BUILDING_VINES_FARM
    {  2,     0,        40,       0,        40,   -3,          1,         1,             3,          10,      0,        51}, // BUILDING_CLAY_PIT
    {  2,     0,        65,       0,        40,   -4,          1,         1,             3,          10,      0,        54}, // BUILDING_TIMBER_YARD
    {  2,     0,        38,       0,        50,   -6,          1,         1,             4,          10,      0,        52}, // BUILDING_MARBLE_QUARRY
    {  2,     0,        39,       0,        50,   -6,          1,         1,             4,          10,      0,        53}, // BUILDING_IRON_MINE
    {  2,     0,        122,      0,        50,   -4,          1,         1,             2,          10,      0,        56}, // BUILDING_OIL_WORKSHOP
    {  2,     0,        44,       0,        45,   -1,          1,         1,             1,          10,      0,        55}, // BUILDING_WINE_WORKSHOP
    {  2,     0,        125,      0,        40,   -4,          1,         1,             2,          10,      0,        59}, // BUILDING_POTTERY_WORKSHOP
    {  2,     0,        124,      0,        40,   -4,          1,         1,             2,          10,      0,        58}, // BUILDING_FURNITURE_WORKSHOP
    {  2,     0,        123,      0,        50,   -4,          1,         1,             2,          10,      0,        57}, // BUILDING_WEAPONS_WORKSHOP
    {  2,     0,        22,       0,        40,   -2,          1,         1,             6,          5,       0,        60}, // BUILDING_MARKET
    {  3,     0,        99,       0,        100,  -4,          1,         2,             2,          6,       1,        61}, // BUILDING_GRANARY
    {  1,     1,        82,       0,        70,   -5,          2,         2,             3,          6,       0,        62}, // BUILDING_WAREHOUSE
    {  1,     1,        82,       0,        0,     0,          0,         0,             0,          0,      -1,        0}, // BUILDING_WAREHOUSE_SPACE
    {  1,     1,        183,      0,        50,    0,          0,         0,             0,          0,      -1,        0}, // BUILDING_NATIVE_HUT
    {  2,     1,        183,      2,        50,    0,          0,         0,             0,          0,      -1,        0}, // BUILDING_NATIVE_MEETING
    {  1,     1,        100,      0,        0,     0,          0,         0,             0,          0,      -1,        0}, // BUILDING_NATIVE_CROPS
    {  4,     1,        66,       1,        0,     0,          0,         0,             0,          0,      -1,        0}, // BUILDING_FORT_GROUND
    {  1,     1,        0,        0,        0,    -1,          1,         1,             2,          0,      -1,        63}, // BUILDING_BURNING_RUIN
};

struct house_properties_t house_properties[MAX_HOUSE_TYPES] = {
    // dev_des | ev_des | ent | water | relg | edu | barb | bath | health | food | pott | oil | furn | wine | prosp | max_ppl | tax_mult
    { -99,      -10,      0,    0,      0,     0,    0,     0,       0,     0,     0,    0,     0,     0,     5,      5,        1},
    { -12,      -5,       0,    1,      0,     0,    0,     0,       0,     0,     0,    0,     0,     0,     10,     7,        1},
    { -7,        0,       0,    1,      0,     0,    0,     0,       0,     1,     0,    0,     0,     0,     15,     9,        1},
    { -2,        4,       0,    1,      1,     0,    0,     0,       0,     1,     0,    0,     0,     0,     20,     11,       1},
    {  2,        8,       0,    2,      1,     0,    0,     0,       0,     1,     0,    0,     0,     0,     25,     13,       2},
    {  6,        12,      10,   2,      1,     0,    0,     0,       0,     1,     0,    0,     0,     0,     30,     15,       2},
    { 10,        16,      10,   2,      1,     1,    0,     0,       0,     1,     0,    0,     0,     0,     35,     17,       2},
    { 14,        20,      10,   2,      1,     1,    0,     1,       0,     1,     1,    0,     0,     0,     45,     19,       2},
    { 18,        25,      25,   2,      1,     1,    0,     1,       0,     1,     1,    0,     0,     0,     50,     19,       3},
    { 22,        32,      25,   2,      1,     1,    0,     1,       1,     1,     1,    0,     1,     0,     58,     20,       3},
    { 29,        40,      25,   2,      1,     2,    1,     1,       1,     1,     1,    1,     1,     0,     65,     84,       3},
    { 37,        48,      35,   2,      1,     2,    1,     1,       1,     2,     1,    1,     1,     0,     80,     84,       4},
    { 45,        53,      35,   2,      2,     2,    1,     1,       1,     2,     1,    1,     1,     1,     150,    40,       9},
    { 50,        58,      40,   2,      2,     2,    1,     1,       2,     2,     1,    1,     1,     1,     180,    42,       10},
    { 55,        63,      45,   2,      2,     3,    1,     1,       2,     2,     1,    1,     1,     1,     400,    90,       11},
    { 60,        68,      50,   2,      3,     3,    1,     1,       2,     3,     1,    1,     1,     1,     600,    100,      11},
    { 65,        74,      55,   2,      3,     3,    1,     1,       2,     3,     1,    1,     1,     2,     700,    106,      12},
    { 70,        80,      60,   2,      4,     3,    1,     1,       2,     3,     1,    1,     1,     2,     900,    112,      12},
    { 76,        90,      70,   2,      4,     3,    1,     1,       2,     3,     1,    1,     1,     2,     1500,   190,      15},
    { 85,        100,     80,   2,      4,     3,    1,     1,       2,     3,     1,    1,     1,     2,     1750,   200,      16},
};

char *all_buildings_strings[] = {
"None", // 0
"Vacant Lot", // 1
"Small Tent", // 2
"Large Tent", // 3
"Small Shack", // 4
"Large Shack", // 5
"Small Hovel", // 6
"Large Hovel", // 7
"Small Casa", // 8
"Large Casa", // 9
"Small Insula", // 10
"Medium Insula", // 11
"Large Insula", // 12
"Grand Insula", // 13
"Small Villa", // 14
"Medium Villa", // 15
"Large Villa", // 16
"Grand Villa", // 17
"Small Palace", // 18
"Medium Palace", // 19
"Large Palace", // 20
"Luxury Palace", // 21
"Clear Land", // 22
"Road", // 23
"Reservoir", // 24
"Aqueduct", // 25
"Fountain", // 26
"Well", // 27
"Doctor", // 28
"Bathhouse", // 29
"Barber", // 30
"Hospital", // 31
"Small Temple: Ceres", // 32
"Small Temple: Neptune", // 33
"Small Temple: Mercury", // 34
"Small Temple: Mars", // 35
"Small Temple: Venus", // 36
"Large Temple: Ceres", // 37
"Large Temple: Neptune", // 38
"Large Temple: Mercury", // 39
"Large Temple: Mars", // 40
"Large Temple: Venus", // 41
"Oracle", // 42
"School", // 43
"Library", // 44
"Academy", // 45
"Mission Post", // 46
"Theater", // 47
"Actor Colony", // 48
"Amphitheater", // 49
"Gladiator School", // 50
"Lion House", // 51
"Colosseum", // 52
"Chariot Maker", // 53
"Hippodrome", // 54
"Gardens", // 55
"Plaza", // 56
"Small Statue", // 57
"Medium Statue", // 58
"Large Statue", // 59
"Governors House", // 60
"Governors Villa", // 61
"Governors Palace", // 62
"Forum", // 63
"Senate", // 64
"Triumphal Arch", // 65
"Engineers Post", // 66
"Low Bridge", // 67
"Ship Bridge", // 68
"Shipyard", // 69
"Wharf", // 70
"Dock", // 71
"Prefecture", // 72
"Wall", // 73
"Tower", // 74
"Gatehouse", // 75
"Fort: Legionaries", // 76
"Fort: Javelin", // 77
"Fort: Mounted", // 78
"Barracks", // 79
"Military Academy", // 80
"Wheat Farm", // 81
"Vegetable Farm", // 82
"Fruit Farm", // 83
"Pig Farm", // 84
"Olive Farm", // 85
"Vines Farm", // 86
"Clay Pit", // 87
"Timber Yard", // 88
"Marble Quarry", // 89
"Iron Mine", // 90
"Oil Workshop", // 91
"Wine Workshop", // 92
"Pottery Workshop", // 93
"Furniture Workshop", // 94
"Weapons Workshop", // 95
"Market", // 96
"Granary", // 97
"Warehouse", // 98
"Warehouse Space", // 99
"Native Hut", // 100
"Native Meeting", // 101
"Native Crops", // 102
"Fort Ground", // 103
"Burning Ruin", // 104
};

struct data_storage {
    int in_use;
    int building_id;
    struct building_storage_t storage;
};

static struct data_storage storages[MAX_STORAGES];

static int tower_sentry_request = 0;

static struct {
    int x_start;
    int y_start;
    int x_end;
    int y_end;
    int bridge_confirmed;
    int fort_confirmed;
} confirm;

static int has_warning = 0;

static struct {
    int type;
    int in_progress;
    struct map_tile_t start;
    struct map_tile_t end;
    int cost_preview;
    struct {
        int meadow;
        int rock;
        int tree;
        int water;
        int wall;
    } required_terrain;
    int road_orientation;
    uint32_t road_last_update;
    int draw_as_constructing;
    int start_offset_x_view;
    int start_offset_y_view;
} construction_data;

static int last_items_cleared;

struct record {
    int active;
    int total;
};

static struct {
    struct record buildings[BUILDING_TYPE_MAX];
    struct record industry[RESOURCE_TYPES_MAX];
} count_data;

enum {
    GRANARY_TASK_NONE = -1,
    GRANARY_TASK_GETTING = 0
};

static struct {
    int building_ids[MAX_GRANARIES];
    int num_items;
    int total_storage_wheat;
    int total_storage_vegetables;
    int total_storage_fruit;
    int total_storage_meat;
} non_getting_granaries;

static struct {
    struct {
        int size;
        int items[MAX_SMALL];
    } small;
    struct {
        int size;
        int items[MAX_LARGE];
    } large;
    struct {
        int size;
        int items[MAX_BURNING];
        int total;
    } burning;
} building_list_data;

struct resource_data {
    int building_id;
    int distance;
    int num_buildings;
};

enum {
    WAREHOUSE_ROOM = 0,
    WAREHOUSE_FULL = 1,
    WAREHOUSE_SOME_ROOM = 2
};

enum {
    WAREHOUSE_TASK_NONE = -1,
    WAREHOUSE_TASK_GETTING = 0,
    WAREHOUSE_TASK_DELIVERING = 1
};

struct building_info_context_t {
    int x_offset;
    int y_offset;
    int width_blocks;
    int height_blocks;
    int help_id;
    int can_play_sound;
    int building_id;
    int has_road_access;
    int worker_percentage;
    int has_reservoir_pipes;
    int aqueduct_has_water;
    int formation_id;
    int formation_types;
    int barracks_soldiers_requested;
    int worst_desirability_building_id;
    int warehouse_space_text;
    int type;
    int terrain_type;
    int can_go_to_advisor;
    int rubble_building_type;
    int storage_show_special_orders;
    struct {
        int sound_id;
        int phrase_id;
        int selected_index;
        int count;
        int drawn;
        int figure_ids[7];
    } figure;
};

static int focus_image_button_id_building_info;
static struct building_info_context_t b_info_context;

static struct {
    int focus_button_id;
    int orders_focus_button_id;
    int resource_focus_button_id;
    int building_id;
} distribution_data = { 0, 0, 0, 0 };

static struct {
    color_t figure_images[7][48 * 48];
    int focus_button_id;
    struct building_info_context_t *context_for_callback;
} building_figures_data;

static struct {
    int focus_button_id;
    int return_button_id;
    struct building_info_context_t *context_for_callback;
} building_military_data;

enum {
    BUILDING_INFO_NONE = 0,
    BUILDING_INFO_TERRAIN = 1,
    BUILDING_INFO_BUILDING = 2,
    BUILDING_INFO_LEGION = 4
};

enum {
    TERRAIN_INFO_NONE = 0,
    TERRAIN_INFO_TREE = 1,
    TERRAIN_INFO_ROCK = 2,
    TERRAIN_INFO_WATER = 3,
    TERRAIN_INFO_SHRUB = 4,
    TERRAIN_INFO_EARTHQUAKE = 5,
    TERRAIN_INFO_ROAD = 6,
    TERRAIN_INFO_AQUEDUCT = 7,
    TERRAIN_INFO_RUBBLE = 8,
    TERRAIN_INFO_WALL = 9,
    TERRAIN_INFO_EMPTY = 10,
    TERRAIN_INFO_BRIDGE = 11,
    TERRAIN_INFO_GARDEN = 12,
    TERRAIN_INFO_PLAZA = 13,
    TERRAIN_INFO_ENTRY_FLAG = 14,
    TERRAIN_INFO_EXIT_FLAG = 15
};

enum {
    WELL_NECESSARY = 0,
    WELL_UNNECESSARY_FOUNTAIN = 1,
    WELL_UNNECESSARY_NO_HOUSES = 2
};

enum {
    GOD_STATE_NONE = 0,
    GOD_STATE_VERY_ANGRY = 1,
    GOD_STATE_ANGRY = 2
};

static char *figure_desc_strings[] = {
    "Nobody", // 0
    "Immigrant", // 1
    "Emigrant", // 2
    "Homeless", // 3
    "Patrician", // 4
    "Cart pusher", // 5
    "Citizen", // 6
    "Barber", // 7
    "Baths worker", // 8
    "Doctor", // 9
    "Surgeon", // 10
    "Priest", // 11
    "School child", // 12
    "Teacher", // 13
    "Librarian", // 14
    "Missionary", // 15
    "Actor", // 16
    "Gladiator", // 17
    "Lion tamer", // 18
    "Charioteer", // 19
    "Hippodrome horse", // 20
    "Tax collector", // 21
    "Engineer", // 22
    "Fishing boat", // 23
    "Seagulls", // 24
    "Shipwreck", // 25
    "Docker", // 26
    "Flotsam", // 27
    "Ballista", // 28
    "Bolt", // 29
    "Sentry", // 30
    "Javelin", // 31
    "Prefect", // 32
    "Standard bearer", // 33
    "Javelin thrower", // 34
    "Mounted auxiliary", // 35
    "Legionary", // 36
    "Market buyer", // 37
    "Market trader", // 38
    "Delivery boy", // 39
    "Warehouseman", // 40
    "Protestor", // 41
    "Criminal", // 42
    "Rioter", // 43
    "Caravan of merchants from", // 44
    "Caravan of merchants from", // 45
    "Trade ship from", // 46
    "Indigenous native", // 47
    "Native trader", // 48
    "Wolf", // 49
    "Sheep", // 50
    "Zebra", // 51
    "Enemy", // 52
    "Arrow", // 53
    "Map flag", // 54
    "Explosion", // 55
};

static char *enemy_desc_strings[] = {
    "A barbarian warrior", // 0
    "A Carthaginian soldier", // 1
    "A Briton", // 2
    "Celtic warrior", // 3
    "Pictish warrior", // 4
    "An Egyptian soldier", // 5
    "An Etruscan soldier", // 6
    "A Samnite soldier", // 7
    "Gaulish warrior", // 8
    "A warrior of the Helvetii", // 9
    "A Hun warrior", // 10
    "A Goth warrior", // 11
    "A Visigoth warrior", // 12
    "A Greek soldier", // 13
    "A Macedonian soldier", // 14
    "A Numidian warrior", // 15
    "A soldier from Pergamum", // 16
    "An Iberian warrior", // 17
    "A Judaean warrior", // 18
    "A Seleucid soldier", // 19
    "Imperial soldier", // 20
};

static const char FIGURE_SOUNDS[32][20][SOUND_FILENAME_MAX] = {
    { // 0
        "vigils_starv1.wav", "vigils_nojob1.wav", "vigils_needjob1.wav", "vigils_nofun1.wav",
        "vigils_relig1.wav", "vigils_great1.wav", "vigils_great2.wav", "vigils_exact1.wav",
        "vigils_exact2.wav", "vigils_exact3.wav", "vigils_exact4.wav", "vigils_exact5.wav",
        "vigils_exact6.wav", "vigils_exact7.wav", "vigils_exact8.wav", "vigils_exact9.wav",
        "vigils_exact10.wav", "vigils_free1.wav", "vigils_free2.wav", "vigils_free3.wav"
    },
    { // 1
        "wallguard_starv1.wav", "wallguard_nojob1.wav", "wallguard_needjob1.wav", "wallguard_nofun1.wav",
        "wallguard_relig1.wav", "wallguard_great1.wav", "wallguard_great2.wav", "wallguard_exact1.wav",
        "wallguard_exact2.wav", "wallguard_exact3.wav", "wallguard_exact4.wav", "wallguard_exact5.wav",
        "wallguard_exact6.wav", "wallguard_exact7.wav", "wallguard_exact8.wav", "wallguard_exact9.wav",
        "wallguard_exact0.wav", "wallguard_free1.wav", "wallguard_free2.wav", "wallguard_free3.wav"
    },
    { // 2
        "engine_starv1.wav", "engine_nojob1.wav", "engine_needjob1.wav", "engine_nofun1.wav",
        "engine_relig1.wav", "engine_great1.wav", "engine_great2.wav", "engine_exact1.wav",
        "engine_exact2.wav", "engine_exact3.wav", "engine_exact4.wav", "engine_exact5.wav",
        "engine_exact6.wav", "engine_exact7.wav", "engine_exact8.wav", "engine_exact9.wav",
        "engine_exact0.wav", "engine_free1.wav", "engine_free2.wav", "engine_free3.wav"
    },
    { // 3
        "taxman_starv1.wav", "taxman_nojob1.wav", "taxman_needjob1.wav", "taxman_nofun1.wav",
        "taxman_relig1.wav", "taxman_great1.wav", "taxman_great2.wav", "taxman_exact1.wav",
        "taxman_exact2.wav", "taxman_exact3.wav", "taxman_exact4.wav", "taxman_exact5.wav",
        "taxman_exact6.wav", "taxman_exact7.wav", "taxman_exact8.wav", "taxman_exact9.wav",
        "taxman_exact0.wav", "taxman_free1.wav", "taxman_free2.wav", "taxman_free3.wav"
    },
    { // 4
        "market_starv1.wav", "market_nojob1.wav", "market_needjob1.wav", "market_nofun1.wav",
        "market_relig1.wav", "market_great1.wav", "market_great2.wav", "market_exact2.wav",
        "market_exact1.wav", "market_exact3.wav", "market_exact4.wav", "market_exact5.wav",
        "market_exact6.wav", "market_exact7.wav", "market_exact8.wav", "market_exact9.wav",
        "market_exact0.wav", "market_free1.wav", "market_free2.wav", "market_free3.wav"
    },
    { // 5
        "crtpsh_starv1.wav", "crtpsh_nojob1.wav", "crtpsh_needjob1.wav", "crtpsh_nofun1.wav",
        "crtpsh_relig1.wav", "crtpsh_great1.wav", "crtpsh_great2.wav", "crtpsh_exact1.wav",
        "crtpsh_exact2.wav", "crtpsh_exact3.wav", "crtpsh_exact4.wav", "crtpsh_exact5.wav",
        "crtpsh_exact6.wav", "crtpsh_exact7.wav", "crtpsh_exact8.wav", "crtpsh_exact9.wav",
        "crtpsh_exact0.wav", "crtpsh_free1.wav", "crtpsh_free2.wav", "crtpsh_free3.wav"
    },
    { // 6
        "donkey_starv1.wav", "donkey_nojob1.wav", "donkey_needjob1.wav", "donkey_nofun1.wav",
        "donkey_relig1.wav", "donkey_great1.wav", "donkey_great2.wav", "donkey_exact1.wav",
        "donkey_exact2.wav", "donkey_exact3.wav", "donkey_exact4.wav", "donkey_exact5.wav",
        "donkey_exact6.wav", "donkey_exact7.wav", "donkey_exact8.wav", "donkey_exact9.wav",
        "donkey_exact0.wav", "donkey_free1.wav", "donkey_free2.wav", "donkey_free3.wav"
    },
    { // 7
        "boats_starv1.wav", "boats_nojob1.wav", "boats_needjob1.wav", "boats_nofun1.wav",
        "boats_relig1.wav", "boats_great1.wav", "boats_great2.wav", "boats_exact2.wav",
        "boats_exact1.wav", "boats_exact3.wav", "boats_exact4.wav", "boats_exact5.wav",
        "boats_exact6.wav", "boats_exact7.wav", "boats_exact8.wav", "boats_exact9.wav",
        "boats_exact0.wav", "boats_free1.wav", "boats_free2.wav", "boats_free3.wav"
    },
    { // 8
        "priest_starv1.wav", "priest_nojob1.wav", "priest_needjob1.wav", "priest_nofun1.wav",
        "priest_relig1.wav", "priest_great1.wav", "priest_great2.wav", "priest_exact1.wav",
        "priest_exact2.wav", "priest_exact3.wav", "priest_exact4.wav", "priest_exact5.wav",
        "priest_exact6.wav", "priest_exact7.wav", "priest_exact8.wav", "priest_exact9.wav",
        "priest_exact0.wav", "priest_free1.wav", "priest_free2.wav", "priest_free3.wav"
    },
    { // 9
        "teach_starv1.wav", "teach_nojob1.wav", "teach_needjob1.wav", "teach_nofun1.wav",
        "teach_relig1.wav", "teach_great1.wav", "teach_great2.wav", "teach_exact1.wav",
        "teach_exact2.wav", "teach_exact3.wav", "teach_exact4.wav", "teach_exact5.wav",
        "teach_exact6.wav", "teach_exact7.wav", "teach_exact8.wav", "teach_exact9.wav",
        "teach_exact0.wav", "teach_free1.wav", "teach_free2.wav", "teach_free3.wav"
    },
    { // 10
        "pupils_starv1.wav", "pupils_nojob1.wav", "pupils_needjob1.wav", "pupils_nofun1.wav",
        "pupils_relig1.wav", "pupils_great1.wav", "pupils_great2.wav", "pupils_exact1.wav",
        "pupils_exact2.wav", "pupils_exact3.wav", "pupils_exact4.wav", "pupils_exact5.wav",
        "pupils_exact6.wav", "pupils_exact7.wav", "pupils_exact8.wav", "pupils_exact9.wav",
        "pupils_exact0.wav", "pupils_free1.wav", "pupils_free2.wav", "pupils_free3.wav"
    },
    { // 11
        "bather_starv1.wav", "bather_nojob1.wav", "bather_needjob1.wav", "bather_nofun1.wav",
        "bather_relig1.wav", "bather_great1.wav", "bather_great2.wav", "bather_exact1.wav",
        "bather_exact2.wav", "bather_exact3.wav", "bather_exact4.wav", "bather_exact5.wav",
        "bather_exact6.wav", "bather_exact7.wav", "bather_exact8.wav", "bather_exact9.wav",
        "bather_exact0.wav", "bather_free1.wav", "bather_free2.wav", "bather_free3.wav"
    },
    { // 12
        "doctor_starv1.wav", "doctor_nojob1.wav", "doctor_needjob1.wav", "doctor_nofun1.wav",
        "doctor_relig1.wav", "doctor_great1.wav", "doctor_great2.wav", "doctor_exact1.wav",
        "doctor_exact2.wav", "doctor_exact3.wav", "doctor_exact4.wav", "doctor_exact5.wav",
        "doctor_exact6.wav", "doctor_exact7.wav", "doctor_exact8.wav", "doctor_exact9.wav",
        "doctor_exact0.wav", "doctor_free1.wav", "doctor_free2.wav", "doctor_free3.wav"
    },
    { // 13
        "barber_starv1.wav", "barber_nojob1.wav", "barber_needjob1.wav", "barber_nofun1.wav",
        "barber_relig1.wav", "barber_great1.wav", "barber_great2.wav", "barber_exact1.wav",
        "barber_exact2.wav", "barber_exact3.wav", "barber_exact4.wav", "barber_exact5.wav",
        "barber_exact6.wav", "barber_exact7.wav", "barber_exact8.wav", "barber_exact9.wav",
        "barber_exact0.wav", "barber_free1.wav", "barber_free2.wav", "barber_free3.wav"
    },
    { // 14
        "actors_starv1.wav", "actors_nojob1.wav", "actors_needjob1.wav", "actors_nofun1.wav",
        "actors_relig1.wav", "actors_great1.wav", "actors_great2.wav", "actors_exact1.wav",
        "actors_exact2.wav", "actors_exact3.wav", "actors_exact4.wav", "actors_exact5.wav",
        "actors_exact6.wav", "actors_exact7.wav", "actors_exact8.wav", "actors_exact9.wav",
        "actors_exact0.wav", "actors_free1.wav", "actors_free2.wav", "actors_free3.wav"
    },
    { // 15
        "gladtr_starv1.wav", "gladtr_nojob1.wav", "gladtr_needjob1.wav", "gladtr_nofun1.wav",
        "gladtr_relig1.wav", "gladtr_great1.wav", "gladtr_great2.wav", "gladtr_exact1.wav",
        "gladtr_exact2.wav", "gladtr_exact3.wav", "gladtr_exact4.wav", "gladtr_exact5.wav",
        "gladtr_exact6.wav", "gladtr_exact7.wav", "gladtr_exact8.wav", "gladtr_exact9.wav",
        "gladtr_exact0.wav", "gladtr_free1.wav", "gladtr_free2.wav", "gladtr_free3.wav"
    },
    { // 16
        "liontr_starv1.wav", "liontr_nojob1.wav", "liontr_needjob1.wav", "liontr_nofun1.wav",
        "liontr_relig1.wav", "liontr_great1.wav", "liontr_great2.wav", "liontr_exact1.wav",
        "liontr_exact2.wav", "liontr_exact3.wav", "liontr_exact4.wav", "liontr_exact5.wav",
        "liontr_exact6.wav", "liontr_exact7.wav", "liontr_exact8.wav", "liontr_exact9.wav",
        "liontr_exact0.wav", "liontr_free1.wav", "liontr_free2.wav", "liontr_free3.wav"
    },
    { // 17
        "charot_starv1.wav", "charot_nojob1.wav", "charot_needjob1.wav", "charot_nofun1.wav",
        "charot_relig1.wav", "charot_great1.wav", "charot_great2.wav", "charot_exact1.wav",
        "charot_exact2.wav", "charot_exact3.wav", "charot_exact4.wav", "charot_exact5.wav",
        "charot_exact6.wav", "charot_exact7.wav", "charot_exact8.wav", "charot_exact9.wav",
        "charot_exact0.wav", "charot_free1.wav", "charot_free2.wav", "charot_free3.wav"
    },
    { // 18
        "patric_starv1.wav", "patric_nojob1.wav", "patric_needjob1.wav", "patric_nofun1.wav",
        "patric_relig1.wav", "patric_great1.wav", "patric_great2.wav", "patric_exact1.wav",
        "patric_exact2.wav", "patric_exact3.wav", "patric_exact4.wav", "patric_exact5.wav",
        "patric_exact6.wav", "patric_exact7.wav", "patric_exact8.wav", "patric_exact9.wav",
        "patric_exact0.wav", "patric_free1.wav", "patric_free2.wav", "patric_free3.wav"
    },
    { // 19
        "pleb_starv1.wav", "pleb_nojob1.wav", "pleb_needjob1.wav", "pleb_nofun1.wav",
        "pleb_relig1.wav", "pleb_great1.wav", "pleb_great2.wav", "pleb_exact1.wav",
        "pleb_exact2.wav", "pleb_exact3.wav", "pleb_exact4.wav", "pleb_exact5.wav",
        "pleb_exact6.wav", "pleb_exact7.wav", "pleb_exact8.wav", "pleb_exact9.wav",
        "pleb_exact0.wav", "pleb_free1.wav", "pleb_free2.wav", "pleb_free3.wav"
    },
    { // 20
        "rioter_starv1.wav", "rioter_nojob1.wav", "rioter_needjob1.wav", "rioter_nofun1.wav",
        "rioter_relig1.wav", "rioter_great1.wav", "rioter_great2.wav", "rioter_exact1.wav",
        "rioter_exact2.wav", "rioter_exact3.wav", "rioter_exact4.wav", "rioter_exact5.wav",
        "rioter_exact6.wav", "rioter_exact7.wav", "rioter_exact8.wav", "rioter_exact9.wav",
        "rioter_exact0.wav", "rioter_free1.wav", "rioter_free2.wav", "rioter_free3.wav"
    },
    { // 21
        "homeless_starv1.wav", "homeless_nojob1.wav", "homeless_needjob1.wav", "homeless_nofun1.wav",
        "homeless_relig1.wav", "homeless_great1.wav", "homeless_great2.wav", "homeless_exact1.wav",
        "homeless_exact2.wav", "homeless_exact3.wav", "homeless_exact4.wav", "homeless_exact5.wav",
        "homeless_exact6.wav", "homeless_exact7.wav", "homeless_exact8.wav", "homeless_exact9.wav",
        "homeless_exact0.wav", "homeless_free1.wav", "homeless_free2.wav", "homeless_free3.wav"
    },
    { // 22
        "unemploy_starv1.wav", "unemploy_nojob1.wav", "unemploy_needjob1.wav", "unemploy_nofun1.wav",
        "unemploy_relig1.wav", "unemploy_great1.wav", "unemploy_great2.wav", "unemploy_exact1.wav",
        "unemploy_exact2.wav", "unemploy_exact3.wav", "unemploy_exact4.wav", "unemploy_exact5.wav",
        "unemploy_exact6.wav", "unemploy_exact7.wav", "unemploy_exact8.wav", "unemploy_exact9.wav",
        "unemploy_exact0.wav", "unemploy_free1.wav", "unemploy_free2.wav", "unemploy_free3.wav"
    },
    { // 23
        "emigrate_starv1.wav", "emigrate_nojob1.wav", "emigrate_needjob1.wav", "emigrate_nofun1.wav",
        "emigrate_relig1.wav", "emigrate_great1.wav", "emigrate_great2.wav", "emigrate_exact1.wav",
        "emigrate_exact2.wav", "emigrate_exact3.wav", "emigrate_exact4.wav", "emigrate_exact5.wav",
        "emigrate_exact6.wav", "emigrate_exact7.wav", "emigrate_exact8.wav", "emigrate_exact9.wav",
        "emigrate_exact0.wav", "emigrate_free1.wav", "emigrate_free2.wav", "emigrate_free3.wav"
    },
    { // 24
        "immigrant_starv1.wav", "immigrant_nojob1.wav", "immigrant_needjob1.wav", "immigrant_nofun1.wav",
        "immigrant_relig1.wav", "immigrant_great1.wav", "immigrant_great2.wav", "immigrant_exact1.wav",
        "immigrant_exact2.wav", "immigrant_exact3.wav", "immigrant_exact4.wav", "immigrant_exact5.wav",
        "immigrant_exact6.wav", "immigrant_exact7.wav", "immigrant_exact8.wav", "immigrant_exact9.wav",
        "immigrant_exact0.wav", "immigrant_free1.wav", "immigrant_free2.wav", "immigrant_free3.wav"
    },
    { // 25
        "enemy_starv1.wav", "enemy_nojob1.wav", "enemy_needjob1.wav", "enemy_nofun1.wav",
        "enemy_relig1.wav", "enemy_great1.wav", "enemy_great2.wav", "enemy_exact1.wav",
        "enemy_exact2.wav", "enemy_exact3.wav", "enemy_exact4.wav", "enemy_exact5.wav",
        "enemy_exact6.wav", "enemy_exact7.wav", "enemy_exact8.wav", "enemy_exact9.wav",
        "enemy_exact0.wav", "enemy_free1.wav", "enemy_free2.wav", "enemy_free3.wav"
    },
    { // 26
        "local_starv1.wav", "local_nojob1.wav", "local_needjob1.wav", "local_nofun1.wav",
        "local_relig1.wav", "local_great1.wav", "local_great2.wav", "local_exact1.wav",
        "local_exact2.wav", "local_exact3.wav", "local_exact4.wav", "local_exact5.wav",
        "local_exact6.wav", "local_exact7.wav", "local_exact8.wav", "local_exact9.wav",
        "local_exact0.wav", "local_free1.wav", "local_free2.wav", "local_free3.wav"
    },
    { // 27
        "libary_starv1.wav", "libary_nojob1.wav", "libary_needjob1.wav", "libary_nofun1.wav",
        "libary_relig1.wav", "libary_great1.wav", "libary_great2.wav", "libary_exact1.wav",
        "libary_exact2.wav", "libary_exact3.wav", "libary_exact4.wav", "libary_exact5.wav",
        "libary_exact6.wav", "libary_exact7.wav", "libary_exact8.wav", "libary_exact9.wav",
        "libary_exact0.wav", "libary_free1.wav", "libary_free2.wav", "libary_free3.wav"
    },
    { // 28
        "srgeon_starv1.wav", "srgeon_nojob1.wav", "srgeon_needjob1.wav", "srgeon_nofun1.wav",
        "srgeon_relig1.wav", "srgeon_great1.wav", "srgeon_great2.wav", "srgeon_exact1.wav",
        "srgeon_exact2.wav", "srgeon_exact3.wav", "srgeon_exact4.wav", "srgeon_exact5.wav",
        "srgeon_exact6.wav", "srgeon_exact7.wav", "srgeon_exact8.wav", "srgeon_exact9.wav",
        "srgeon_exact0.wav", "srgeon_free1.wav", "srgeon_free2.wav", "srgeon_free3.wav"
    },
    { // 29
        "docker_starv1.wav", "docker_nojob1.wav", "docker_needjob1.wav", "docker_nofun1.wav",
        "docker_relig1.wav", "docker_great1.wav", "docker_great2.wav", "docker_exact1.wav",
        "docker_exact2.wav", "docker_exact3.wav", "docker_exact4.wav", "docker_exact5.wav",
        "docker_exact6.wav", "docker_exact7.wav", "docker_exact8.wav", "docker_exact9.wav",
        "docker_exact0.wav", "docker_free1.wav", "docker_free2.wav", "docker_free3.wav"
    },
    { // 30
        "missionary_starv1.wav", "missionary_nojob1.wav", "missionary_needjob1.wav", "missionary_nofun1.wav",
        "missionary_relig1.wav", "missionary_great1.wav", "missionary_great2.wav", "missionary_exact1.wav",
        "missionary_exact2.wav", "missionary_exact3.wav", "missionary_exact4.wav", "missionary_exact5.wav",
        "missionary_exact6.wav", "missionary_exact7.wav", "missionary_exact8.wav", "missionary_exact9.wav",
        "missionary_exact0.wav", "missionary_free1.wav", "missionary_free2.wav", "missionary_free3.wav"
    },
    { // 31
        "granboy_starv1.wav", "granboy_nojob1.wav", "granboy_needjob1.wav", "granboy_nofun1.wav",
        "granboy_relig1.wav", "granboy_great1.wav", "granboy_great2.wav", "granboy_exact1.wav",
        "granboy_exact2.wav", "granboy_exact3.wav", "granboy_exact4.wav", "granboy_exact5.wav",
        "granboy_exact6.wav", "granboy_exact7.wav", "granboy_exact8.wav", "granboy_exact9.wav",
        "granboy_exact0.wav", "granboy_free1.wav", "granboy_free2.wav", "granboy_free3.wav"
    }
};

struct building_t *building_main(struct building_t *b)
{
    for (int guard = 0; guard < 9; guard++) {
        if (b->prev_part_building_id <= 0) {
            return b;
        }
        b = &all_buildings[b->prev_part_building_id];
    }
    return &all_buildings[0];
}

struct building_t *building_create(int type, int x, int y)
{
    struct building_t *b = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        if (all_buildings[i].state == BUILDING_STATE_UNUSED && !game_undo_contains_building(i)) {
            b = &all_buildings[i];
            break;
        }
    }
    if (!b) {
        city_warning_show(WARNING_DATA_LIMIT_REACHED);
        return &all_buildings[0];
    }

    memset(&(b->data), 0, sizeof(b->data));

    b->state = BUILDING_STATE_CREATED;
    b->type = type;
    b->size = building_properties[type].size;
    b->created_sequence = extra.created_sequence++;
    b->sentiment.house_happiness = 50;

    // house size
    b->house_size = 0;
    if (type >= BUILDING_HOUSE_VACANT_LOT && type <= BUILDING_HOUSE_MEDIUM_INSULA) {
        b->house_size = 1;
    } else if (type >= BUILDING_HOUSE_LARGE_INSULA && type <= BUILDING_HOUSE_MEDIUM_VILLA) {
        b->house_size = 2;
    } else if (type >= BUILDING_HOUSE_LARGE_VILLA && type <= BUILDING_HOUSE_MEDIUM_PALACE) {
        b->house_size = 3;
    } else if (type >= BUILDING_HOUSE_LARGE_PALACE && type <= BUILDING_HOUSE_LUXURY_PALACE) {
        b->house_size = 4;
    }

    // subtype
    if (building_is_house(type) && type != BUILDING_HOUSE_VACANT_LOT) {
        b->subtype.house_level = type - BUILDING_HOUSE_SMALL_TENT;
    } else {
        b->subtype.house_level = 0;
    }

    // input/output resources
    switch (type) {
        case BUILDING_WHEAT_FARM:
            b->output_resource_id = RESOURCE_WHEAT;
            break;
        case BUILDING_VEGETABLE_FARM:
            b->output_resource_id = RESOURCE_VEGETABLES;
            break;
        case BUILDING_FRUIT_FARM:
            b->output_resource_id = RESOURCE_FRUIT;
            break;
        case BUILDING_OLIVE_FARM:
            b->output_resource_id = RESOURCE_OLIVES;
            break;
        case BUILDING_VINES_FARM:
            b->output_resource_id = RESOURCE_VINES;
            break;
        case BUILDING_PIG_FARM:
            b->output_resource_id = RESOURCE_MEAT;
            break;
        case BUILDING_MARBLE_QUARRY:
            b->output_resource_id = RESOURCE_MARBLE;
            break;
        case BUILDING_IRON_MINE:
            b->output_resource_id = RESOURCE_IRON;
            break;
        case BUILDING_TIMBER_YARD:
            b->output_resource_id = RESOURCE_TIMBER;
            break;
        case BUILDING_CLAY_PIT:
            b->output_resource_id = RESOURCE_CLAY;
            break;
        case BUILDING_WINE_WORKSHOP:
            b->output_resource_id = RESOURCE_WINE;
            b->subtype.workshop_type = WORKSHOP_VINES_TO_WINE;
            break;
        case BUILDING_OIL_WORKSHOP:
            b->output_resource_id = RESOURCE_OIL;
            b->subtype.workshop_type = WORKSHOP_OLIVES_TO_OIL;
            break;
        case BUILDING_WEAPONS_WORKSHOP:
            b->output_resource_id = RESOURCE_WEAPONS;
            b->subtype.workshop_type = WORKSHOP_IRON_TO_WEAPONS;
            break;
        case BUILDING_FURNITURE_WORKSHOP:
            b->output_resource_id = RESOURCE_FURNITURE;
            b->subtype.workshop_type = WORKSHOP_TIMBER_TO_FURNITURE;
            break;
        case BUILDING_POTTERY_WORKSHOP:
            b->output_resource_id = RESOURCE_POTTERY;
            b->subtype.workshop_type = WORKSHOP_CLAY_TO_POTTERY;
            break;
        default:
            b->output_resource_id = RESOURCE_NONE;
            break;
    }

    if (type == BUILDING_GRANARY) {
        b->data.granary.resource_stored[RESOURCE_NONE] = 2400;
    }

    b->x = x;
    b->y = y;
    b->grid_offset = map_grid_offset(x, y);
    b->house_figure_generation_delay = map_random_get(b->grid_offset) & 0x7f;
    b->figure_roam_direction = b->house_figure_generation_delay & 6;
    b->fire_proof = building_properties[type].fire_proof;

    return b;
}

static void building_clear_related_data(struct building_t *b)
{
    if (b->storage_id) {
        storages[b->storage_id].in_use = 0;
        b->storage_id = 0;
    }
    if (b->type == BUILDING_SENATE) {
        if (b->grid_offset == city_data.building.senate_grid_offset) {
            city_data.building.senate_grid_offset = 0;
            city_data.building.senate_x = 0;
            city_data.building.senate_y = 0;
            city_data.building.senate_placed = 0;
        }
    }
    if (b->type == BUILDING_DOCK) {
        city_data.building.working_docks--;
    }
    if (b->type == BUILDING_BARRACKS) {
        if (b->grid_offset == city_data.building.barracks_grid_offset) {
            city_data.building.barracks_grid_offset = 0;
            city_data.building.barracks_x = 0;
            city_data.building.barracks_y = 0;
            city_data.building.barracks_placed = 0;
        }
    }
    if (building_is_fort(b->type)) {
        city_data.military.total_legions--;
        struct formation_t *m = &legion_formations[b->formation_id];
        if (m->in_use) {
            for (int i = 0; i < m->num_figures; i++) {
                struct figure_t *f = &figures[m->figures[i]];
                struct map_point_t nearest_barracks_road_tile = { 0 };
                set_destination__closest_building_of_type(b->id, BUILDING_BARRACKS, &nearest_barracks_road_tile);
                figure_route_remove(f);
                if (nearest_barracks_road_tile.x) {
                    f->destination_x = nearest_barracks_road_tile.x;
                    f->destination_y = nearest_barracks_road_tile.y;
                } else {
                    f->destination_x = scenario.exit_point.x;
                    f->destination_y = scenario.exit_point.y;
                }
                f->action_state = FIGURE_ACTION_SOLDIER_RETURNING_TO_BARRACKS;
            }
            map_figure_delete(&figures[m->legion_standard__figure_id]);
            memset(&legion_formations[b->formation_id], 0, sizeof(struct formation_t));
            m->id = b->formation_id;
        }
    }
    if (b->type == BUILDING_HIPPODROME) {
        city_data.building.hippodrome_placed = 0;
    }
}

static void building_delete(struct building_t *b)
{
    building_clear_related_data(b);
    int id = b->id;
    memset(b, 0, sizeof(struct building_t));
    b->id = id;
}

void building_update_state(void)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_CREATED) {
            b->state = BUILDING_STATE_IN_USE;
        }
        if (b->state != BUILDING_STATE_IN_USE || !b->house_size) {
            if (b->state == BUILDING_STATE_UNDO || b->state == BUILDING_STATE_DELETED_BY_PLAYER) {
                if (b->type == BUILDING_TOWER || b->type == BUILDING_GATEHOUSE) {
                    map_tiles_update_all_walls();
                    map_tiles_update_all_roads();
                } else if (b->type == BUILDING_RESERVOIR) {
                    map_tiles_update_all_aqueducts(0);
                } else if (b->type == BUILDING_GRANARY) {
                    map_tiles_update_all_roads();
                }
                map_building_tiles_remove(i, b->x, b->y);
                map_routing_update_land();
                building_delete(b);
            } else if (b->state == BUILDING_STATE_RUBBLE) {
                if (b->house_size) {
                    city_population_remove_home_removed(b->house_population);
                }
                building_delete(b);
            } else if (b->state == BUILDING_STATE_DELETED_BY_GAME) {
                building_delete(b);
            }
        }
        if (b->type == BUILDING_TIMBER_YARD && !map_terrain_exist_multiple_tiles_in_radius_with_type(b->x, b->y, 2, 1, TERRAIN_TREE | TERRAIN_SHRUB, 3)) {
            destroy_on_fire(b, 1);
        }
    }
}

int align_bulding_type_index_to_strings(int building_type_index)
{
    if (building_type_index >= BUILDING_HOUSE_SMALL_TENT) {
        building_type_index += MAX_HOUSE_TYPES;
    }
    return building_type_index;
}

int building_is_house(int type)
{
    return type >= BUILDING_HOUSE_VACANT_LOT && type <= BUILDING_HOUSE_LUXURY_PALACE;
}

int building_is_fort(int type)
{
    return type >= BUILDING_FORT_LEGIONARIES && type <= BUILDING_FORT_MOUNTED;
}

int building_get_highest_id(void)
{
    return extra.highest_id_in_use;
}

void building_update_highest_id(void)
{
    extra.highest_id_in_use = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        if (all_buildings[i].state != BUILDING_STATE_UNUSED) {
            extra.highest_id_in_use = i;
        }
    }
    if (extra.highest_id_in_use > extra.highest_id_ever) {
        extra.highest_id_ever = extra.highest_id_in_use;
    }
}

void set_destination__closest_building_of_type(int closest_to__building_id, int closest_building_of_type__type, struct map_point_t *closest_building_of_type__road_tile)
{
    int min_distance = 10000;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->type == closest_building_of_type__type && b->num_workers >= building_properties[closest_building_of_type__type].n_laborers) {
            struct building_t *closest_to__building = &all_buildings[closest_to__building_id];
            int dist = calc_maximum_distance(closest_to__building->x, closest_to__building->y, b->x, b->y);
            if (dist < min_distance) {
                if (map_has_road_access(b->x, b->y, b->size, closest_building_of_type__road_tile)) {
                    min_distance = dist;
                }
            }
        }
    }
}

void building_totals_add_corrupted_house(int unfixable)
{
    extra.incorrect_houses++;
    if (unfixable) {
        extra.unfixable_houses++;
    }
}

void building_clear_all(void)
{
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        memset(&all_buildings[i], 0, sizeof(struct building_t));
        all_buildings[i].id = i;
    }
    extra.highest_id_in_use = 0;
    extra.highest_id_ever = 0;
    extra.created_sequence = 0;
    extra.incorrect_houses = 0;
    extra.unfixable_houses = 0;
}

void building_save_state(struct buffer_t *buf, struct buffer_t *highest_id, struct buffer_t *highest_id_ever,
                         struct buffer_t *sequence, struct buffer_t *corrupt_houses)
{
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        building_state_save_to_buffer(buf, &all_buildings[i]);
    }
    buffer_write_i32(highest_id, extra.highest_id_in_use);
    buffer_write_i32(highest_id_ever, extra.highest_id_ever);
    buffer_write_i32(sequence, extra.created_sequence);

    buffer_write_i32(corrupt_houses, extra.incorrect_houses);
    buffer_write_i32(corrupt_houses, extra.unfixable_houses);
}

void building_load_state(struct buffer_t *buf, struct buffer_t *highest_id, struct buffer_t *highest_id_ever,
                         struct buffer_t *sequence, struct buffer_t *corrupt_houses)
{
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        building_state_load_from_buffer(buf, &all_buildings[i]);
        all_buildings[i].id = i;
    }
    extra.highest_id_in_use = buffer_read_i32(highest_id);
    extra.highest_id_ever = buffer_read_i32(highest_id_ever);
    extra.created_sequence = buffer_read_i32(sequence);

    extra.incorrect_houses = buffer_read_i32(corrupt_houses);
    extra.unfixable_houses = buffer_read_i32(corrupt_houses);
}

int building_animation_offset(struct building_t *b, int image_id, int grid_offset)
{
    if (b->type == BUILDING_FOUNTAIN && (b->num_workers <= 0 || !b->has_water_access)) {
        return 0;
    }
    if (b->type == BUILDING_RESERVOIR && !b->has_water_access) {
        return 0;
    }
    if (building_is_workshop(b->type)) {
        if (b->loads_stored <= 0 || b->num_workers <= 0) {
            return 0;
        }
    }
    if ((b->type == BUILDING_PREFECTURE || b->type == BUILDING_ENGINEERS_POST) && b->num_workers <= 0) {
        return 0;
    }
    if (b->type == BUILDING_MARKET && b->num_workers <= 0) {
        return 0;
    }
    if (b->type == BUILDING_WAREHOUSE && b->num_workers < building_properties[b->type].n_laborers) {
        return 0;
    }
    if (b->type == BUILDING_DOCK && b->data.dock.num_ships <= 0) {
        map_sprite_animation_set(grid_offset, 1);
        return 1;
    }
    if (b->type == BUILDING_MARBLE_QUARRY && b->num_workers <= 0) {
        map_sprite_animation_set(grid_offset, 1);
        return 1;
    } else if ((b->type == BUILDING_IRON_MINE || b->type == BUILDING_CLAY_PIT ||
        b->type == BUILDING_TIMBER_YARD) && b->num_workers <= 0) {
        return 0;
    }
    if (b->type == BUILDING_GLADIATOR_SCHOOL) {
        if (b->num_workers <= 0) {
            map_sprite_animation_set(grid_offset, 1);
            return 1;
        }
    } else if (b->type >= BUILDING_THEATER && b->type <= BUILDING_CHARIOT_MAKER &&
        b->type != BUILDING_HIPPODROME && b->num_workers <= 0) {
        return 0;
    }
    if (b->type == BUILDING_GRANARY && b->num_workers < building_properties[b->type].n_laborers) {
        return 0;
    }

    struct image_t *img = image_get(image_id);
    if (!game_animation_should_advance(img->animation_speed_id)) {
        return map_sprite_animation_at(grid_offset) & 0x7f;
    }
    // advance animation
    int new_sprite = 0;
    int is_reverse = 0;
    if (b->type == BUILDING_WINE_WORKSHOP) {
        // exception for wine...
        int pct_done = calc_percentage(b->data.industry.progress, 400);
        if (pct_done <= 0) {
            new_sprite = 0;
        } else if (pct_done < 4) {
            new_sprite = 1;
        } else if (pct_done < 8) {
            new_sprite = 2;
        } else if (pct_done < 12) {
            new_sprite = 3;
        } else if (pct_done < 96) {
            if (map_sprite_animation_at(grid_offset) < 4) {
                new_sprite = 4;
            } else {
                new_sprite = map_sprite_animation_at(grid_offset) + 1;
                if (new_sprite > 8) {
                    new_sprite = 4;
                }
            }
        } else {
            // close to done
            if (map_sprite_animation_at(grid_offset) < 9) {
                new_sprite = 9;
            } else {
                new_sprite = map_sprite_animation_at(grid_offset) + 1;
                if (new_sprite > 12) {
                    new_sprite = 12;
                }
            }
        }
    } else if (img->animation_can_reverse) {
        if (map_sprite_animation_at(grid_offset) & 0x80) {
            is_reverse = 1;
        }
        int current_sprite = map_sprite_animation_at(grid_offset) & 0x7f;
        if (is_reverse) {
            new_sprite = current_sprite - 1;
            if (new_sprite < 1) {
                new_sprite = 1;
                is_reverse = 0;
            }
        } else {
            new_sprite = current_sprite + 1;
            if (new_sprite > img->num_animation_sprites) {
                new_sprite = img->num_animation_sprites;
                is_reverse = 1;
            }
        }
    } else {
        // Absolutely normal case
        new_sprite = map_sprite_animation_at(grid_offset) + 1;
        if (new_sprite > img->num_animation_sprites) {
            new_sprite = 1;
        }
    }

    map_sprite_animation_set(grid_offset, is_reverse ? new_sprite | 0x80 : new_sprite);
    return new_sprite;
}

void building_barracks_save_state(struct buffer_t *buf)
{
    buffer_write_i32(buf, tower_sentry_request);
}

void building_barracks_load_state(struct buffer_t *buf)
{
    tower_sentry_request = buffer_read_i32(buf);
}

static int is_industry_type(const struct building_t *b)
{
    return b->output_resource_id || b->type == BUILDING_NATIVE_CROPS
        || b->type == BUILDING_SHIPYARD || b->type == BUILDING_WHARF;
}

void building_state_save_to_buffer(struct buffer_t *buf, const struct building_t *b)
{
    buffer_write_u8(buf, b->state);
    buffer_write_u8(buf, b->size);
    buffer_write_u8(buf, b->house_is_merged);
    buffer_write_u8(buf, b->house_size);
    buffer_write_u8(buf, b->x);
    buffer_write_u8(buf, b->y);
    buffer_write_i16(buf, b->grid_offset);
    buffer_write_i16(buf, b->type);
    buffer_write_i16(buf, b->subtype.house_level); // which union field we use does not matter
    buffer_write_u8(buf, b->road_network_id);
    buffer_write_u16(buf, b->created_sequence);
    buffer_write_i16(buf, b->houses_covered);
    buffer_write_i16(buf, b->percentage_houses_covered);
    buffer_write_i16(buf, b->house_population);
    buffer_write_i16(buf, b->house_population_room);
    buffer_write_i16(buf, b->house_highest_population);
    buffer_write_u8(buf, b->road_access_x);
    buffer_write_u8(buf, b->road_access_y);
    buffer_write_i16(buf, b->figure_id);
    buffer_write_i16(buf, b->figure_id2);
    buffer_write_i16(buf, b->immigrant_figure_id);
    buffer_write_i16(buf, b->figure_id4);
    buffer_write_u8(buf, b->figure_spawn_delay);
    buffer_write_u8(buf, b->figure_roam_direction);
    buffer_write_u8(buf, b->has_water_access);
    buffer_write_i16(buf, b->prev_part_building_id);
    buffer_write_i16(buf, b->next_part_building_id);
    buffer_write_i16(buf, b->loads_stored);
    buffer_write_u8(buf, b->has_well_access);
    buffer_write_i16(buf, b->num_workers);
    buffer_write_i8(buf, b->labor_category);
    buffer_write_u8(buf, b->output_resource_id);
    buffer_write_u8(buf, b->has_road_access);
    buffer_write_u8(buf, b->house_criminal_active);
    buffer_write_i16(buf, b->damage_risk);
    buffer_write_i16(buf, b->fire_risk);
    buffer_write_i16(buf, b->fire_duration);
    buffer_write_u8(buf, b->fire_proof);
    buffer_write_u8(buf, b->house_figure_generation_delay);
    buffer_write_u8(buf, b->house_tax_coverage);
    buffer_write_i16(buf, b->formation_id);
    if (building_is_house(b->type)) {
        for (int i = 0; i < INVENTORY_MAX; i++) {
            buffer_write_i16(buf, b->data.house.inventory[i]);
        }
        buffer_write_u8(buf, b->data.house.theater);
        buffer_write_u8(buf, b->data.house.amphitheater_actor);
        buffer_write_u8(buf, b->data.house.amphitheater_gladiator);
        buffer_write_u8(buf, b->data.house.colosseum_gladiator);
        buffer_write_u8(buf, b->data.house.colosseum_lion);
        buffer_write_u8(buf, b->data.house.hippodrome);
        buffer_write_u8(buf, b->data.house.school);
        buffer_write_u8(buf, b->data.house.library);
        buffer_write_u8(buf, b->data.house.academy);
        buffer_write_u8(buf, b->data.house.barber);
        buffer_write_u8(buf, b->data.house.clinic);
        buffer_write_u8(buf, b->data.house.bathhouse);
        buffer_write_u8(buf, b->data.house.hospital);
        buffer_write_u8(buf, b->data.house.temple_ceres);
        buffer_write_u8(buf, b->data.house.temple_neptune);
        buffer_write_u8(buf, b->data.house.temple_mercury);
        buffer_write_u8(buf, b->data.house.temple_mars);
        buffer_write_u8(buf, b->data.house.temple_venus);
        buffer_write_u8(buf, b->data.house.no_space_to_expand);
        buffer_write_u8(buf, b->data.house.num_foods);
        buffer_write_u8(buf, b->data.house.entertainment);
        buffer_write_u8(buf, b->data.house.education);
        buffer_write_u8(buf, b->data.house.health);
        buffer_write_u8(buf, b->data.house.num_gods);
        buffer_write_u8(buf, b->data.house.devolve_delay);
        buffer_write_u8(buf, b->data.house.evolve_text_id);
    } else if (b->type == BUILDING_MARKET) {
        for (int i = 0; i < INVENTORY_MAX; i++) {
            buffer_write_i16(buf, b->data.market.inventory[i]);
        }
        buffer_write_i16(buf, b->data.market.pottery_demand);
        buffer_write_i16(buf, b->data.market.furniture_demand);
        buffer_write_i16(buf, b->data.market.oil_demand);
        buffer_write_i16(buf, b->data.market.wine_demand);
        buffer_write_u8(buf, b->data.market.fetch_inventory_id);
    } else if (b->type == BUILDING_GRANARY) {
        for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
            buffer_write_i16(buf, b->data.granary.resource_stored[i]);
        }
    } else if (b->type == BUILDING_DOCK) {
        buffer_write_i16(buf, b->data.dock.queued_docker_id);
        buffer_write_u8(buf, b->data.dock.num_ships);
        buffer_write_i8(buf, b->data.dock.orientation);
        for (int i = 0; i < 3; i++) {
            buffer_write_i16(buf, b->data.dock.docker_ids[i]);
        }
        buffer_write_i16(buf, b->data.dock.trade_ship_id);
    } else if (is_industry_type(b)) {
        buffer_write_i16(buf, b->data.industry.progress);
        buffer_write_u8(buf, b->data.industry.has_fish);
        buffer_write_u8(buf, b->data.industry.blessing_days_left);
        buffer_write_u8(buf, b->data.industry.orientation);
        buffer_write_u8(buf, b->data.industry.has_raw_materials);
        buffer_write_u8(buf, b->data.industry.curse_days_left);
        buffer_write_i16(buf, b->data.industry.fishing_boat_id);
    } else {
        buffer_write_u8(buf, b->data.entertainment.num_shows);
        buffer_write_u8(buf, b->data.entertainment.days1);
        buffer_write_u8(buf, b->data.entertainment.days2);
        buffer_write_u8(buf, b->data.entertainment.play);
    }
    buffer_write_i32(buf, b->tax_income_or_storage);
    buffer_write_u8(buf, b->house_days_without_food);
    buffer_write_u8(buf, b->ruin_has_plague);
    buffer_write_i8(buf, b->desirability);
    buffer_write_u8(buf, b->is_deleted);
    buffer_write_u8(buf, b->storage_id);
    buffer_write_i8(buf, b->sentiment.house_happiness); // which union field we use does not matter
    buffer_write_u8(buf, b->show_on_problem_overlay);
}

void building_state_load_from_buffer(struct buffer_t *buf, struct building_t *b)
{
    b->state = buffer_read_u8(buf);
    b->size = buffer_read_u8(buf);
    b->house_is_merged = buffer_read_u8(buf);
    b->house_size = buffer_read_u8(buf);
    b->x = buffer_read_u8(buf);
    b->y = buffer_read_u8(buf);
    b->grid_offset = buffer_read_i16(buf);
    b->type = buffer_read_i16(buf);
    b->subtype.house_level = buffer_read_i16(buf); // which union field we use does not matter
    b->road_network_id = buffer_read_u8(buf);
    b->created_sequence = buffer_read_u16(buf);
    b->houses_covered = buffer_read_i16(buf);
    b->percentage_houses_covered = buffer_read_i16(buf);
    b->house_population = buffer_read_i16(buf);
    b->house_population_room = buffer_read_i16(buf);
    b->house_highest_population = buffer_read_i16(buf);
    b->road_access_x = buffer_read_u8(buf);
    b->road_access_y = buffer_read_u8(buf);
    b->figure_id = buffer_read_i16(buf);
    b->figure_id2 = buffer_read_i16(buf);
    b->immigrant_figure_id = buffer_read_i16(buf);
    b->figure_id4 = buffer_read_i16(buf);
    b->figure_spawn_delay = buffer_read_u8(buf);
    b->figure_roam_direction = buffer_read_u8(buf);
    b->has_water_access = buffer_read_u8(buf);
    b->prev_part_building_id = buffer_read_i16(buf);
    b->next_part_building_id = buffer_read_i16(buf);
    b->loads_stored = buffer_read_i16(buf);
    b->has_well_access = buffer_read_u8(buf);
    b->num_workers = buffer_read_i16(buf);
    b->labor_category = buffer_read_i8(buf);
    b->output_resource_id = buffer_read_u8(buf);
    b->has_road_access = buffer_read_u8(buf);
    b->house_criminal_active = buffer_read_u8(buf);
    b->damage_risk = buffer_read_i16(buf);
    b->fire_risk = buffer_read_i16(buf);
    b->fire_duration = buffer_read_i16(buf);
    b->fire_proof = buffer_read_u8(buf);
    b->house_figure_generation_delay = buffer_read_u8(buf);
    b->house_tax_coverage = buffer_read_u8(buf);
    b->formation_id = buffer_read_i16(buf);
    if (building_is_house(b->type)) {
        for (int i = 0; i < INVENTORY_MAX; i++) {
            b->data.house.inventory[i] = buffer_read_i16(buf);
        }
        b->data.house.theater = buffer_read_u8(buf);
        b->data.house.amphitheater_actor = buffer_read_u8(buf);
        b->data.house.amphitheater_gladiator = buffer_read_u8(buf);
        b->data.house.colosseum_gladiator = buffer_read_u8(buf);
        b->data.house.colosseum_lion = buffer_read_u8(buf);
        b->data.house.hippodrome = buffer_read_u8(buf);
        b->data.house.school = buffer_read_u8(buf);
        b->data.house.library = buffer_read_u8(buf);
        b->data.house.academy = buffer_read_u8(buf);
        b->data.house.barber = buffer_read_u8(buf);
        b->data.house.clinic = buffer_read_u8(buf);
        b->data.house.bathhouse = buffer_read_u8(buf);
        b->data.house.hospital = buffer_read_u8(buf);
        b->data.house.temple_ceres = buffer_read_u8(buf);
        b->data.house.temple_neptune = buffer_read_u8(buf);
        b->data.house.temple_mercury = buffer_read_u8(buf);
        b->data.house.temple_mars = buffer_read_u8(buf);
        b->data.house.temple_venus = buffer_read_u8(buf);
        b->data.house.no_space_to_expand = buffer_read_u8(buf);
        b->data.house.num_foods = buffer_read_u8(buf);
        b->data.house.entertainment = buffer_read_u8(buf);
        b->data.house.education = buffer_read_u8(buf);
        b->data.house.health = buffer_read_u8(buf);
        b->data.house.num_gods = buffer_read_u8(buf);
        b->data.house.devolve_delay = buffer_read_u8(buf);
        b->data.house.evolve_text_id = buffer_read_u8(buf);
    } else if (b->type == BUILDING_MARKET) {
        for (int i = 0; i < INVENTORY_MAX; i++) {
            b->data.market.inventory[i] = buffer_read_i16(buf);
        }
        b->data.market.pottery_demand = buffer_read_i16(buf);
        b->data.market.furniture_demand = buffer_read_i16(buf);
        b->data.market.oil_demand = buffer_read_i16(buf);
        b->data.market.wine_demand = buffer_read_i16(buf);
        b->data.market.fetch_inventory_id = buffer_read_u8(buf);
    } else if (b->type == BUILDING_GRANARY) {
        for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
            b->data.granary.resource_stored[i] = buffer_read_i16(buf);
        }
    } else if (b->type == BUILDING_DOCK) {
        b->data.dock.queued_docker_id = buffer_read_i16(buf);
        b->data.dock.num_ships = buffer_read_u8(buf);
        b->data.dock.orientation = buffer_read_i8(buf);
        for (int i = 0; i < 3; i++) {
            b->data.dock.docker_ids[i] = buffer_read_i16(buf);
        }
        b->data.dock.trade_ship_id = buffer_read_i16(buf);
    } else if (is_industry_type(b)) {
        b->data.industry.progress = buffer_read_i16(buf);
        b->data.industry.has_fish = buffer_read_u8(buf);
        b->data.industry.blessing_days_left = buffer_read_u8(buf);
        b->data.industry.orientation = buffer_read_u8(buf);
        b->data.industry.has_raw_materials = buffer_read_u8(buf);
        b->data.industry.curse_days_left = buffer_read_u8(buf);
        b->data.industry.fishing_boat_id = buffer_read_i16(buf);
    } else {
        b->data.entertainment.num_shows = buffer_read_u8(buf);
        b->data.entertainment.days1 = buffer_read_u8(buf);
        b->data.entertainment.days2 = buffer_read_u8(buf);
        b->data.entertainment.play = buffer_read_u8(buf);
    }
    b->tax_income_or_storage = buffer_read_i32(buf);
    b->house_days_without_food = buffer_read_u8(buf);
    b->ruin_has_plague = buffer_read_u8(buf);
    b->desirability = buffer_read_i8(buf);
    b->is_deleted = buffer_read_u8(buf);
    b->storage_id = buffer_read_u8(buf);
    b->sentiment.house_happiness = buffer_read_i8(buf); // which union field we use does not matter
    b->show_on_problem_overlay = buffer_read_u8(buf);
}

static struct building_t *get_deletable_building(int grid_offset)
{
    int building_id = map_building_at(grid_offset);
    if (!building_id) {
        return 0;
    }
    struct building_t *b = building_main(&all_buildings[building_id]);
    if (b->type == BUILDING_BURNING_RUIN || b->type == BUILDING_NATIVE_CROPS ||
        b->type == BUILDING_NATIVE_HUT || b->type == BUILDING_NATIVE_MEETING) {
        return 0;
    }
    if (b->state == BUILDING_STATE_DELETED_BY_PLAYER || b->is_deleted) {
        return 0;
    }
    return b;
}

static void map_grid_start_end_to_area(int x_start, int y_start, int x_end, int y_end, int *x_min, int *y_min, int *x_max, int *y_max)
{
    if (x_start < x_end) {
        *x_min = x_start;
        *x_max = x_end;
    } else {
        *x_min = x_end;
        *x_max = x_start;
    }
    if (y_start < y_end) {
        *y_min = y_start;
        *y_max = y_end;
    } else {
        *y_min = y_end;
        *y_max = y_start;
    }
    map_grid_bound_area(x_min, y_min, x_max, y_max);
}

static int clear_land_confirmed(int measure_only, int x_start, int y_start, int x_end, int y_end)
{
    int items_placed = 0;
    game_undo_restore_building_state();
    game_undo_restore_map(0);

    int x_min, x_max, y_min, y_max;
    map_grid_start_end_to_area(x_start, y_start, x_end, y_end, &x_min, &y_min, &x_max, &y_max);

    int visual_feedback_on_delete = config_get(CONFIG_UI_VISUAL_FEEDBACK_ON_DELETE);

    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (measure_only && visual_feedback_on_delete) {
                struct building_t *b = get_deletable_building(grid_offset);
                if (map_property_is_deleted(grid_offset) || (b && map_property_is_deleted(b->grid_offset))) {
                    continue;
                }
                map_building_tiles_mark_deleting(grid_offset);
                if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
                    if (b) {
                        items_placed++;
                    }
                } else if (map_terrain_is(grid_offset, TERRAIN_SHRUB | TERRAIN_TREE) && !scenario.allowed_buildings[BUILDING_CLEAR_LAND]) {
                    continue;
                } else if (map_terrain_is(grid_offset, TERRAIN_ROCK | TERRAIN_ELEVATION)) {
                    continue;
                } else if (map_terrain_is(grid_offset, TERRAIN_WATER)) { // keep the "bridge is free" bug from C3
                    continue;
                } else if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT)
                    || map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
                    items_placed++;
                }
                continue;
            }
            if (map_terrain_is(grid_offset, TERRAIN_SHRUB | TERRAIN_TREE) && !scenario.allowed_buildings[BUILDING_CLEAR_LAND]) {
                continue;
            }
            if (map_terrain_is(grid_offset, TERRAIN_ROCK | TERRAIN_ELEVATION)) {
                continue;
            }
            if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
                struct building_t *b = get_deletable_building(grid_offset);
                if (!b) {
                    continue;
                }
                if (b->type == BUILDING_FORT_GROUND || building_is_fort(b->type)) {
                    if (!measure_only && confirm.fort_confirmed != 1) {
                        continue;
                    }
                    if (!measure_only && confirm.fort_confirmed == 1) {
                        game_undo_disable();
                    }
                }
                if (b->house_size && b->house_population && !measure_only) {
                    figure_create_homeless(b->x, b->y, b->house_population);
                    b->house_population = 0;
                }
                if (b->state != BUILDING_STATE_DELETED_BY_PLAYER) {
                    items_placed++;
                    game_undo_add_building(b);
                }
                b->state = BUILDING_STATE_DELETED_BY_PLAYER;
                b->is_deleted = 1;
                struct building_t *space = b;
                for (int i = 0; i < 9; i++) {
                    if (space->prev_part_building_id <= 0) {
                        break;
                    }
                    space = &all_buildings[space->prev_part_building_id];
                    game_undo_add_building(space);
                    space->state = BUILDING_STATE_DELETED_BY_PLAYER;
                }
                space = b;
                for (int i = 0; i < 9; i++) {
                    space = &all_buildings[space->next_part_building_id];
                    if (space->id <= 0) {
                        break;
                    }
                    game_undo_add_building(space);
                    space->state = BUILDING_STATE_DELETED_BY_PLAYER;
                }
            } else if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT)) {
                terrain_grid.items[grid_offset] &= ~TERRAIN_CLEARABLE;
                items_placed++;
                map_aqueduct_remove(grid_offset);
            } else if (map_terrain_is(grid_offset, TERRAIN_WATER)) {
                if (!measure_only && map_bridge_count_figures(grid_offset) > 0) {
                    city_warning_show(WARNING_PEOPLE_ON_BRIDGE);
                } else if (confirm.bridge_confirmed == 1) {
                    map_bridge_remove(grid_offset, measure_only);
                    items_placed++;
                }
            } else if (map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
                if (map_terrain_is(grid_offset, TERRAIN_ROAD)) {
                    map_property_clear_plaza_or_earthquake(grid_offset);
                }
                terrain_grid.items[grid_offset] &= ~TERRAIN_CLEARABLE;
                items_placed++;
            }
        }
    }
    if (!measure_only || !visual_feedback_on_delete) {
        int radius;
        if (x_max - x_min <= y_max - y_min) {
            radius = y_max - y_min + 3;
        } else {
            radius = x_max - x_min + 3;
        }
        map_tiles_update_region_empty_land(x_min, y_min, x_max, y_max);
        foreach_region_tile(x_min, y_min, x_max, y_max, update_meadow_tile);
        foreach_region_tile(x_min, y_min, x_max, y_max, set_rubble_image);
        map_tiles_update_all_gardens();
        foreach_region_tile(x_min - 1, y_min - 1, x_min + radius - 2, y_min + radius - 2, set_road_image);
        map_tiles_update_all_plazas();
        foreach_region_tile(x_min - 1, y_min - 1, x_min + radius - 2, y_min + radius - 2, set_wall_image);
        foreach_region_tile(x_min - 3, y_min - 3, x_max + 3, y_max + 3, update_aqueduct_tile);
    }
    if (!measure_only) {
        map_routing_update_land();
        map_routing_update_walls();
        map_routing_update_water();
        window_invalidate();
    }
    return items_placed;
}

static int place_routed_building(int x_start, int y_start, int x_end, int y_end, int type, int *items)
{
    static const int direction_indices[8][4] = {
        {0, 2, 6, 4},
        {0, 2, 6, 4},
        {2, 4, 0, 6},
        {2, 4, 0, 6},
        {4, 6, 2, 0},
        {4, 6, 2, 0},
        {6, 0, 4, 2},
        {6, 0, 4, 2}
    };
    *items = 0;
    int grid_offset = map_grid_offset(x_end, y_end);
    int guard = 0;
    // reverse routing
    while (1) {
        if (++guard >= 400) {
            return 0;
        }
        int distance = map_routing_distance(grid_offset);
        if (distance <= 0) {
            return 0;
        }
        switch (type) {
            case ROUTED_BUILDING_ROAD:
                *items += map_tiles_set_road(x_end, y_end);
                break;
            case ROUTED_BUILDING_WALL:
                *items += map_tiles_set_wall(x_end, y_end);
                break;
            case ROUTED_BUILDING_AQUEDUCT:
                terrain_grid.items[grid_offset] |= TERRAIN_AQUEDUCT;
                map_property_clear_constructing(grid_offset);
                break;
            case ROUTED_BUILDING_AQUEDUCT_WITHOUT_GRAPHIC:
                *items += 1;
                break;
        }
        int direction = calc_general_direction(x_end, y_end, x_start, y_start);
        if (direction == DIR_8_NONE) {
            return 1; // destination reached
        }
        int routed = 0;
        for (int i = 0; i < 4; i++) {
            int index = direction_indices[direction][i];
            int new_grid_offset = grid_offset + map_grid_direction_delta(index);
            int new_dist = map_routing_distance(new_grid_offset);
            if (new_dist > 0 && new_dist < distance) {
                grid_offset = new_grid_offset;
                x_end = map_grid_offset_to_x(grid_offset);
                y_end = map_grid_offset_to_y(grid_offset);
                routed = 1;
                break;
            }
        }
        if (!routed) {
            return 0;
        }
    }
}

int building_construction_place_road(int measure_only, int x_start, int y_start, int x_end, int y_end)
{
    game_undo_restore_map(0);

    int start_offset = map_grid_offset(x_start, y_start);
    int end_offset = map_grid_offset(x_end, y_end);
    int forbidden_terrain_mask =
        TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER |
        TERRAIN_TREE | TERRAIN_GARDEN | TERRAIN_ELEVATION |
        TERRAIN_RUBBLE | TERRAIN_BUILDING | TERRAIN_WALL;
    if (map_terrain_is(start_offset, forbidden_terrain_mask)) {
        return 0;
    }
    if (map_terrain_is(end_offset, forbidden_terrain_mask)) {
        return 0;
    }

    int items_placed = 0;
    if (map_routing_calculate_distances_for_building(ROUTED_BUILDING_ROAD, x_start, y_start) &&
            place_routed_building(x_start, y_start, x_end, y_end, ROUTED_BUILDING_ROAD, &items_placed)) {
        if (!measure_only) {
            map_routing_update_land();
            window_invalidate();
        }
    }
    return items_placed;
}

static void show(int warning)
{
    city_warning_show(warning);
    has_warning = 1;
}

static void building_construction_warning_check_food_stocks(int type)
{
    if (!has_warning && type == BUILDING_HOUSE_VACANT_LOT) {
        if (city_data.population.population >= 200 && !scenario.rome_supplies_wheat) {
            if (calc_percentage(city_data.resource.food_produced_last_month, city_data.resource.food_consumed_last_month) <= 95) {
                show(WARNING_MORE_FOOD_NEEDED);
            }
        }
    }
}

static void mark_construction(int x, int y, int size, int terrain, int absolute_xy)
{
    if (map_building_tiles_mark_construction(x, y, size, terrain, absolute_xy)) {
        construction_data.draw_as_constructing = 1;
    }
}

static int place_houses(int measure_only, int x_start, int y_start, int x_end, int y_end)
{
    int x_min, x_max, y_min, y_max;
    map_grid_start_end_to_area(x_start, y_start, x_end, y_end, &x_min, &y_min, &x_max, &y_max);

    int needs_road_warning = 0;
    int items_placed = 0;
    game_undo_restore_building_state();
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
                continue;
            }
            if (measure_only) {
                map_property_mark_constructing(grid_offset);
                items_placed++;
            } else {
                struct building_t *b = building_create(BUILDING_HOUSE_VACANT_LOT, x, y);
                game_undo_add_building(b);
                if (b->id > 0) {
                    items_placed++;
                    map_building_tiles_add(b->id, x, y, 1,
                        image_group(GROUP_BUILDING_HOUSE_VACANT_LOT), TERRAIN_BUILDING);
                    if (!map_terrain_exists_tile_in_radius_with_type(x, y, 1, 2, TERRAIN_ROAD)) {
                        needs_road_warning = 1;
                    }
                }
            }
        }
    }
    if (!measure_only) {
        building_construction_warning_check_food_stocks(BUILDING_HOUSE_VACANT_LOT);
        if (needs_road_warning) {
            city_warning_show(WARNING_HOUSE_TOO_FAR_FROM_ROAD);
        }
        map_routing_update_land();
        window_invalidate();
    }
    return items_placed;
}

static int place_plaza(int x_start, int y_start, int x_end, int y_end)
{
    int x_min, y_min, x_max, y_max;
    map_grid_start_end_to_area(x_start, y_start, x_end, y_end, &x_min, &y_min, &x_max, &y_max);
    game_undo_restore_map(1);

    int items_placed = 0;
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (map_terrain_is(grid_offset, TERRAIN_ROAD) &&
                !map_terrain_is(grid_offset, TERRAIN_WATER | TERRAIN_BUILDING | TERRAIN_AQUEDUCT)) {
                if (!map_property_is_plaza_or_earthquake(grid_offset)) {
                    items_placed++;
                }
                map_image_set(grid_offset, 0);
                map_property_mark_plaza_or_earthquake(grid_offset);
                map_property_set_multi_tile_size(grid_offset, 1);
                map_property_mark_draw_tile(grid_offset);
            }
        }
    }
    map_tiles_update_all_plazas();
    return items_placed;
}

static int place_garden(int x_start, int y_start, int x_end, int y_end)
{
    game_undo_restore_map(1);

    int x_min, y_min, x_max, y_max;
    map_grid_start_end_to_area(x_start, y_start, x_end, y_end, &x_min, &y_min, &x_max, &y_max);

    int items_placed = 0;
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
                items_placed++;
                terrain_grid.items[grid_offset] |= TERRAIN_GARDEN;
            }
        }
    }
    map_tiles_update_all_gardens();
    return items_placed;
}

void building_construction_set_cost(int cost)
{
    construction_data.cost_preview = cost;
}

void building_construction_set_type(int type)
{
    construction_data.type = type;
    construction_data.in_progress = 0;
    construction_data.start.x = 0;
    construction_data.start.y = 0;
    construction_data.end.x = 0;
    construction_data.end.y = 0;
    construction_data.cost_preview = 0;

    if (type != BUILDING_NONE) {
        construction_data.required_terrain.wall = 0;
        construction_data.required_terrain.water = 0;
        construction_data.required_terrain.tree = 0;
        construction_data.required_terrain.rock = 0;
        construction_data.required_terrain.meadow = 0;
        construction_data.road_orientation = 0;
        construction_data.road_last_update = time_get_millis();
        construction_data.start.grid_offset = 0;

        switch (type) {
            case BUILDING_WHEAT_FARM:
            case BUILDING_VEGETABLE_FARM:
            case BUILDING_FRUIT_FARM:
            case BUILDING_OLIVE_FARM:
            case BUILDING_VINES_FARM:
            case BUILDING_PIG_FARM:
                construction_data.required_terrain.meadow = 1;
                break;
            case BUILDING_MARBLE_QUARRY:
            case BUILDING_IRON_MINE:
                construction_data.required_terrain.rock = 1;
                break;
            case BUILDING_TIMBER_YARD:
                construction_data.required_terrain.tree = 1;
                break;
            case BUILDING_CLAY_PIT:
                construction_data.required_terrain.water = 1;
                break;
            case BUILDING_GATEHOUSE:
            case BUILDING_TRIUMPHAL_ARCH:
                construction_data.road_orientation = 1;
                break;
            case BUILDING_TOWER:
                construction_data.required_terrain.wall = 1;
                break;
            default:
                break;
        }
    }
}

void building_construction_clear_type(void)
{
    construction_data.cost_preview = 0;
    construction_data.type = BUILDING_NONE;
}

int building_construction_type(void)
{
    return construction_data.type;
}

int building_construction_cost(void)
{
    return construction_data.cost_preview;
}

static int building_construction_is_updatable(void)
{
    switch (construction_data.type) {
        case BUILDING_CLEAR_LAND:
        case BUILDING_ROAD:
        case BUILDING_AQUEDUCT:
        case BUILDING_WALL:
        case BUILDING_PLAZA:
        case BUILDING_GARDENS:
        case BUILDING_HOUSE_VACANT_LOT:
            return 1;
        default:
            return 0;
    }
}

int building_construction_size(int *x, int *y)
{
    if (!building_construction_is_updatable() || !construction_data.in_progress
        || (construction_data.type != BUILDING_CLEAR_LAND && !construction_data.cost_preview)) {
        return 0;
    }
    int size_x = construction_data.end.x - construction_data.start.x;
    int size_y = construction_data.end.y - construction_data.start.y;
    if (size_x < 0) {
        size_x = -size_x;
    }
    if (size_y < 0) {
        size_y = -size_y;
    }
    size_x++;
    size_y++;
    *x = size_x;
    *y = size_y;
    return 1;
}

int building_construction_in_progress(void)
{
    return construction_data.in_progress;
}

void building_construction_start(int x, int y, int grid_offset)
{
    construction_data.start.grid_offset = grid_offset;
    construction_data.start.x = construction_data.end.x = x;
    construction_data.start.y = construction_data.end.y = y;

    if (game_undo_start_build(construction_data.type)) {
        construction_data.in_progress = 1;
        int can_start = 1;
        switch (construction_data.type) {
            case BUILDING_ROAD:
                can_start = map_routing_calculate_distances_for_building(
                    ROUTED_BUILDING_ROAD, construction_data.start.x, construction_data.start.y);
                break;
            case BUILDING_AQUEDUCT:
                can_start = map_routing_calculate_distances_for_building(
                    ROUTED_BUILDING_AQUEDUCT, construction_data.start.x, construction_data.start.y);
                break;
            case BUILDING_WALL:
                can_start = map_routing_calculate_distances_for_building(
                    ROUTED_BUILDING_WALL, construction_data.start.x, construction_data.start.y);
                break;
            default:
                break;
        }
        if (!can_start) {
            building_construction_cancel();
        }
    }
}

void building_construction_cancel(void)
{
    map_property_clear_constructing_and_deleted();
    if (construction_data.in_progress && building_construction_is_updatable()) {
        game_undo_restore_map(1);
        construction_data.in_progress = 0;
        construction_data.cost_preview = 0;
    } else {
        building_construction_clear_type();
    }
}

static int building_construction_place_aqueduct(int x_start, int y_start, int x_end, int y_end, int *cost)
{
    game_undo_restore_map(0);

    *cost = 0;
    int blocked = 0;
    int grid_offset = map_grid_offset(x_start, y_start);
    if (map_terrain_is(grid_offset, TERRAIN_ROAD)) {
        if (map_property_is_plaza_or_earthquake(grid_offset)) {
            blocked = 1;
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
        blocked = 1;
    }
    grid_offset = map_grid_offset(x_end, y_end);
    if (map_terrain_is(grid_offset, TERRAIN_ROAD)) {
        if (map_property_is_plaza_or_earthquake(grid_offset)) {
            blocked = 1;
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
        blocked = 1;
    }
    if (blocked) {
        return 0;
    }
    if (!map_routing_calculate_distances_for_building(ROUTED_BUILDING_AQUEDUCT, x_start, y_start)) {
        return 0;
    }
    int num_items;
    place_routed_building(x_start, y_start, x_end, y_end, ROUTED_BUILDING_AQUEDUCT, &num_items);
    *cost = building_properties[BUILDING_AQUEDUCT].cost * num_items;
    return 1;
}

static int building_construction_place_wall(int measure_only, int x_start, int y_start, int x_end, int y_end)
{
    game_undo_restore_map(0);

    int start_offset = map_grid_offset(x_start, y_start);
    int end_offset = map_grid_offset(x_end, y_end);
    int forbidden_terrain_mask =
        TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_BUILDING |
        TERRAIN_TREE | TERRAIN_ROAD | TERRAIN_GARDEN | TERRAIN_ELEVATION |
        TERRAIN_RUBBLE | TERRAIN_AQUEDUCT | TERRAIN_ACCESS_RAMP;
    if (map_terrain_is(start_offset, forbidden_terrain_mask)) {
        return 0;
    }
    if (map_terrain_is(end_offset, forbidden_terrain_mask)) {
        return 0;
    }
    int items_placed = 0;
    if (place_routed_building(x_start, y_start, x_end, y_end, ROUTED_BUILDING_WALL, &items_placed)) {
        if (!measure_only) {
            map_routing_update_land();
            map_routing_update_walls();
            window_invalidate();
        }
    }
    return items_placed;
}

static void confirm_delete_fort(void)
{
    confirm.fort_confirmed = 1;
    clear_land_confirmed(0, confirm.x_start, confirm.y_start, confirm.x_end, confirm.y_end);
}

static void confirm_delete_bridge(void)
{
    confirm.bridge_confirmed = 1;
    clear_land_confirmed(0, confirm.x_start, confirm.y_start, confirm.x_end, confirm.y_end);
}

static int building_construction_clear_land(int measure_only, int x_start, int y_start, int x_end, int y_end)
{
    confirm.fort_confirmed = 0;
    confirm.bridge_confirmed = 0;
    if (measure_only) {
        return clear_land_confirmed(measure_only, x_start, y_start, x_end, y_end);
    }

    int x_min, x_max, y_min, y_max;
    map_grid_start_end_to_area(x_start, y_start, x_end, y_end, &x_min, &y_min, &x_max, &y_max);

    int ask_confirm_bridge = 0;
    int ask_confirm_fort = 0;
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            int grid_offset = map_grid_offset(x, y);
            int building_id = map_building_at(grid_offset);
            if (building_id) {
                struct building_t *b = &all_buildings[building_id];
                if (building_is_fort(b->type) || b->type == BUILDING_FORT_GROUND) {
                    ask_confirm_fort = 1;
                }
            }
            if (map_is_bridge(grid_offset)) {
                ask_confirm_bridge = 1;
            }
        }
    }
    confirm.x_start = x_start;
    confirm.y_start = y_start;
    confirm.x_end = x_end;
    confirm.y_end = y_end;
    if (ask_confirm_fort) {
        window_popup_dialog_show(POPUP_DIALOG_DELETE_FORT, confirm_delete_fort, 2);
        return -1;
    } else if (ask_confirm_bridge) {
        window_popup_dialog_show(POPUP_DIALOG_DELETE_BRIDGE, confirm_delete_bridge, 2);
        return -1;
    } else {
        return clear_land_confirmed(measure_only, x_start, y_start, x_end, y_end);
    }
}

static int blocked_land_terrain(void)
{
    return
        TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER |
        TERRAIN_BUILDING | TERRAIN_TREE | TERRAIN_GARDEN |
        TERRAIN_ROAD | TERRAIN_ELEVATION | TERRAIN_RUBBLE;
}

int map_water_determine_orientation_size2(int x, int y, int adjust_xy, int *orientation_absolute, int *orientation_relative)
{
    if (adjust_xy == 1) {
        switch (city_view_orientation()) {
            case DIR_0_TOP: break;
            case DIR_2_RIGHT: x--; break;
            case DIR_6_LEFT: y--; break;
            case DIR_4_BOTTOM: x--; y--; break;
        }
    }
    if (!map_grid_is_inside(x, y, 2)) {
        return 999;
    }

    int base_offset = map_grid_offset(x, y);
    int tile_offsets[] = { OFFSET(0,0), OFFSET(1,0), OFFSET(0,1), OFFSET(1,1) };
    const int should_be_water[4][4] = { {1, 1, 0, 0}, {0, 1, 0, 1}, {0, 0, 1, 1}, {1, 0, 1, 0} };
    for (int dir = 0; dir < 4; dir++) {
        int ok_tiles = 0;
        int blocked_tiles = 0;
        for (int i = 0; i < 4; i++) {
            int grid_offset = base_offset + tile_offsets[i];
            if (should_be_water[dir][i]) {
                if (!map_terrain_is(grid_offset, TERRAIN_WATER)) {
                    break;
                }
                ok_tiles++;
                if (map_terrain_is(grid_offset, TERRAIN_ROCK | TERRAIN_ROAD)) {
                    // bridge or map edge
                    blocked_tiles++;
                }
            } else {
                if (map_terrain_is(grid_offset, TERRAIN_WATER)) {
                    break;
                }
                ok_tiles++;
                if (map_terrain_is(grid_offset, blocked_land_terrain())) {
                    blocked_tiles++;
                }
            }
        }
        // check six water tiles in front
        const int tiles_to_check[4][6] = {
            {OFFSET(-1,0), OFFSET(-1,-1), OFFSET(0,-1), OFFSET(1,-1), OFFSET(2,-1), OFFSET(2,0)},
            {OFFSET(1,-1), OFFSET(2,-1), OFFSET(2,0), OFFSET(2,1), OFFSET(2,2), OFFSET(1,2)},
            {OFFSET(2,1), OFFSET(2,2), OFFSET(1,2), OFFSET(0,2), OFFSET(-1,2), OFFSET(-1,1)},
            {OFFSET(0,2), OFFSET(-1,2), OFFSET(-1,1), OFFSET(-1,0), OFFSET(-1,-1), OFFSET(0,-1)},
        };
        for (int i = 0; i < 6; i++) {
            if (!map_terrain_is(base_offset + tiles_to_check[dir][i], TERRAIN_WATER)) {
                ok_tiles = 0;
            }
        }
        if (ok_tiles == 4) {
            // water/land is OK in this orientation
            if (orientation_absolute) {
                *orientation_absolute = dir;
            }
            if (orientation_relative) {
                *orientation_relative = (4 + dir - city_view_orientation() / 2) % 4;
            }
            return blocked_tiles;
        }
    }
    return 999;
}

int map_water_determine_orientation_size3(int x, int y, int adjust_xy, int *orientation_absolute, int *orientation_relative)
{
    if (adjust_xy == 1) {
        switch (city_view_orientation()) {
            case DIR_0_TOP: break;
            case DIR_2_RIGHT: x -= 2; break;
            case DIR_6_LEFT: y -= 2; break;
            case DIR_4_BOTTOM: x -= 2; y -= 2; break;
        }
    }
    if (!map_grid_is_inside(x, y, 3)) {
        return 999;
    }

    int base_offset = map_grid_offset(x, y);
    int tile_offsets[] = {
        OFFSET(0,0), OFFSET(1,0), OFFSET(2,0),
        OFFSET(0,1), OFFSET(1,1), OFFSET(2,1),
        OFFSET(0,2), OFFSET(1,2), OFFSET(2,2)
    };
    int should_be_water[4][9] = {
        {1, 1, 1, 0, 0, 0, 0, 0, 0},
        {0, 0, 1, 0, 0, 1, 0, 0, 1},
        {0, 0, 0, 0, 0, 0, 1, 1, 1},
        {1, 0, 0, 1, 0, 0, 1, 0, 0}
    };
    for (int dir = 0; dir < 4; dir++) {
        int ok_tiles = 0;
        int blocked_tiles = 0;
        for (int i = 0; i < 9; i++) {
            int grid_offset = base_offset + tile_offsets[i];
            if (should_be_water[dir][i]) {
                if (!map_terrain_is(grid_offset, TERRAIN_WATER)) {
                    break;
                }
                ok_tiles++;
                if (map_terrain_is(grid_offset, TERRAIN_ROCK | TERRAIN_ROAD)) {
                    // bridge or map edge
                    blocked_tiles++;
                }
            } else {
                if (map_terrain_is(grid_offset, TERRAIN_WATER)) {
                    break;
                }
                ok_tiles++;
                if (map_terrain_is(grid_offset, blocked_land_terrain())) {
                    blocked_tiles++;
                }
            }
        }
        // check two water tiles at the side
        const int tiles_to_check[4][2] = {
            {OFFSET(-1,0), OFFSET(3,0)}, {OFFSET(2,-1), OFFSET(2,3)},
            {OFFSET(3,2), OFFSET(-1,2)}, {OFFSET(0,-1), OFFSET(0,3)}
        };
        for (int i = 0; i < 2; i++) {
            if (!map_terrain_is(base_offset + tiles_to_check[dir][i], TERRAIN_WATER)) {
                ok_tiles = 0;
            }
        }
        if (ok_tiles == 9) {
            // water/land is OK in this orientation
            if (orientation_absolute) {
                *orientation_absolute = dir;
            }
            if (orientation_relative) {
                *orientation_relative = (4 + dir - city_view_orientation() / 2) % 4;
            }
            return blocked_tiles;
        }
    }
    return 999;
}

void building_construction_update(int x, int y, int grid_offset)
{
    if (grid_offset) {
        construction_data.end.x = x;
        construction_data.end.y = y;
        construction_data.end.grid_offset = grid_offset;
    } else {
        x = construction_data.end.x;
        y = construction_data.end.y;
    }
    if (!construction_data.type) {
        construction_data.cost_preview = 0;
        return;
    }
    map_property_clear_constructing_and_deleted();
    int current_cost = building_properties[construction_data.type].cost;

    if (construction_data.type == BUILDING_CLEAR_LAND) {
        int items_placed = last_items_cleared = building_construction_clear_land(1, construction_data.start.x, construction_data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (construction_data.type == BUILDING_WALL) {
        int items_placed = building_construction_place_wall(1, construction_data.start.x, construction_data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (construction_data.type == BUILDING_ROAD) {
        int items_placed = building_construction_place_road(1, construction_data.start.x, construction_data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (construction_data.type == BUILDING_PLAZA) {
        int items_placed = place_plaza(construction_data.start.x, construction_data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (construction_data.type == BUILDING_GARDENS) {
        int items_placed = place_garden(construction_data.start.x, construction_data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (construction_data.type == BUILDING_LOW_BRIDGE || construction_data.type == BUILDING_SHIP_BRIDGE) {
        int length = map_bridge_building_length();
        if (length > 1) current_cost *= length;
    } else if (construction_data.type == BUILDING_AQUEDUCT) {
        building_construction_place_aqueduct(construction_data.start.x, construction_data.start.y, x, y, &current_cost);
        map_tiles_update_all_aqueducts(0);
    } else if (construction_data.type == BUILDING_HOUSE_VACANT_LOT) {
        int items_placed = place_houses(1, construction_data.start.x, construction_data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (construction_data.type == BUILDING_GATEHOUSE) {
        mark_construction(x, y, 2, ~TERRAIN_ROAD, 0);
    } else if (construction_data.type == BUILDING_TRIUMPHAL_ARCH) {
        mark_construction(x, y, 3, ~TERRAIN_ROAD, 0);
    } else if (construction_data.type == BUILDING_WAREHOUSE) {
        mark_construction(x, y, 3, TERRAIN_ALL, 0);
    } else if (building_is_fort(construction_data.type)) {
        if (city_data.military.total_legions < MAX_LEGIONS) {
            const int offsets_x[] = { 3, 4, 4, 3 };
            const int offsets_y[] = { -1, -1, 0, 0 };
            int orient_index = city_view_orientation() / 2;
            int x_offset = offsets_x[orient_index];
            int y_offset = offsets_y[orient_index];
            if (map_building_tiles_are_clear(x, y, 3, TERRAIN_ALL) &&
                map_building_tiles_are_clear(x + x_offset, y + y_offset, 4, TERRAIN_ALL)) {
                mark_construction(x, y, 3, TERRAIN_ALL, 0);
                mark_construction(x + x_offset, y + y_offset, 4, TERRAIN_ALL, 0);
            }
        }
    } else if (construction_data.type == BUILDING_HIPPODROME) {
        if (map_building_tiles_are_clear(x, y, 5, TERRAIN_ALL) &&
            map_building_tiles_are_clear(x + 5, y, 5, TERRAIN_ALL) &&
            map_building_tiles_are_clear(x + 10, y, 5, TERRAIN_ALL) &&
            !city_data.building.hippodrome_placed) {
            mark_construction(x, y, 5, TERRAIN_ALL, 0);
            mark_construction(x + 5, y, 5, TERRAIN_ALL, 0);
            mark_construction(x + 10, y, 5, TERRAIN_ALL, 0);
        }
    } else if (construction_data.type == BUILDING_SHIPYARD || construction_data.type == BUILDING_WHARF) {
        if (!map_water_determine_orientation_size2(x, y, 1, 0, 0)) {
            construction_data.draw_as_constructing = 1;
        }
    } else if (construction_data.type == BUILDING_DOCK) {
        if (!map_water_determine_orientation_size3(x, y, 1, 0, 0)) {
            construction_data.draw_as_constructing = 1;
        }
    } else if (construction_data.required_terrain.meadow || construction_data.required_terrain.rock || construction_data.required_terrain.tree ||
            construction_data.required_terrain.water || construction_data.required_terrain.wall) {
        // never mark as constructing
    } else {
        if (!(construction_data.type == BUILDING_SENATE && city_data.building.senate_placed) &&
            !(construction_data.type == BUILDING_BARRACKS && building_count_total(BUILDING_BARRACKS) > 0)) {
            mark_construction(x, y, building_properties[construction_data.type].size, TERRAIN_ALL, 0);
        }
    }
    if (!city_finance_can_afford(current_cost)) {
        map_property_clear_constructing_and_deleted();
        building_construction_cancel();
        construction_data.cost_preview = 0;
        city_warning_show(WARNING_OUT_OF_MONEY);
        return;
    }
    construction_data.cost_preview = current_cost;
}

static struct building_t *add_warehouse_space(int x, int y, struct building_t *prev)
{
    struct building_t *b = building_create(BUILDING_WAREHOUSE_SPACE, x, y);
    game_undo_add_building(b);
    b->prev_part_building_id = prev->id;
    prev->next_part_building_id = b->id;
    map_building_tiles_add(b->id, x, y, 1, EMPTY_WAREHOUSE_IMG_ID, TERRAIN_BUILDING);
    return b;
}

static int building_storage_create(void)
{
    for (int i = 1; i < MAX_STORAGES; i++) {
        if (!storages[i].in_use) {
            memset(&storages[i], 0, sizeof(struct data_storage));
            storages[i].in_use = 1;
            return i;
        }
    }
    return 0;
}

static void add_warehouse(struct building_t *b)
{
    b->storage_id = building_storage_create();
    b->prev_part_building_id = 0;
    map_building_tiles_add(b->id, b->x, b->y, 1, image_group(GROUP_BUILDING_WAREHOUSE), TERRAIN_BUILDING);

    struct building_t *prev = b;
    prev = add_warehouse_space(b->x + 1, b->y, prev);
    prev = add_warehouse_space(b->x + 2, b->y, prev);
    prev = add_warehouse_space(b->x, b->y + 1, prev);
    prev = add_warehouse_space(b->x + 1, b->y + 1, prev);
    prev = add_warehouse_space(b->x + 2, b->y + 1, prev);
    prev = add_warehouse_space(b->x, b->y + 2, prev);
    prev = add_warehouse_space(b->x + 1, b->y + 2, prev);
    prev = add_warehouse_space(b->x + 2, b->y + 2, prev);
    prev->next_part_building_id = 0;
}

void building_construction_place(void)
{
    construction_data.cost_preview = 0;
    construction_data.in_progress = 0;
    int x_start = construction_data.start.x;
    int y_start = construction_data.start.y;
    int x_end = construction_data.end.x;
    int y_end = construction_data.end.y;
    has_warning = 0;
    if (!construction_data.type) {
        return;
    }
    if (construction_data.type >= BUILDING_LARGE_TEMPLE_CERES && construction_data.type <= BUILDING_LARGE_TEMPLE_VENUS
        && city_data.resource.stored_in_warehouses[RESOURCE_MARBLE] < 2) {
        map_property_clear_constructing_and_deleted();
        city_warning_show(WARNING_MARBLE_NEEDED_LARGE_TEMPLE);
        return;
    }
    if (construction_data.type == BUILDING_ORACLE && city_data.resource.stored_in_warehouses[RESOURCE_MARBLE] < 2) {
        map_property_clear_constructing_and_deleted();
        city_warning_show(WARNING_MARBLE_NEEDED_ORACLE);
        return;
    }
    int enemy_type = 0;
    for (int i = 1; i < MAX_FIGURES; i++) {
        struct figure_t *f = &figures[i];
        if (figure_is_alive(f) && (figure_properties[f->type].is_enemy_unit || figure_properties[f->type].is_caesar_legion_unit || f->type == FIGURE_WOLF)) {
            int dx = (f->x > x_start) ? (f->x - x_start) : (x_start - f->x);
            int dy = (f->y > y_start) ? (f->y - y_start) : (y_start - f->y);
            if (dx <= 12 && dy <= 12) {
                enemy_type = f->type;
                break;
            }
            dx = (f->x > x_end) ? (f->x - x_end) : (x_end - f->x);
            dy = (f->y > y_end) ? (f->y - y_end) : (y_end - f->y);
            if (dx <= 12 && dy <= 12) {
                enemy_type = f->type;
                break;
            }
        }
    }
    if (construction_data.type != BUILDING_CLEAR_LAND && enemy_type) {
        if (construction_data.type == BUILDING_WALL || construction_data.type == BUILDING_ROAD || construction_data.type == BUILDING_AQUEDUCT) {
            game_undo_restore_map(0);
        } else if (construction_data.type == BUILDING_PLAZA || construction_data.type == BUILDING_GARDENS) {
            game_undo_restore_map(1);
        } else if (construction_data.type == BUILDING_LOW_BRIDGE || construction_data.type == BUILDING_SHIP_BRIDGE) {
            map_bridge_reset_building_length();
        } else {
            map_property_clear_constructing_and_deleted();
        }
        if (enemy_type == FIGURE_WOLF) {
            play_sound_effect(SOUND_EFFECT_WOLF_ATTACK_2);
        } else {
            city_warning_show(WARNING_ENEMY_NEARBY);
        }
        return;
    }

    int placement_cost = building_properties[construction_data.type].cost;
    if (construction_data.type == BUILDING_CLEAR_LAND) {
        // BUG in original (keep this behaviour): if confirmation has to be asked (bridge/fort),
        // the previous cost is deducted from treasury and if user chooses 'no', they still pay for removal.
        // If we don't do it this way, the user doesn't pay for the removal at all since we don't come back
        // here when the user says yes.
        int items_placed = building_construction_clear_land(0, x_start, y_start, x_end, y_end);
        if (items_placed < 0) {
            items_placed = last_items_cleared;
        }
        placement_cost *= items_placed;
        map_property_clear_constructing_and_deleted();
    } else if (construction_data.type == BUILDING_WALL) {
        placement_cost *= building_construction_place_wall(0, x_start, y_start, x_end, y_end);
    } else if (construction_data.type == BUILDING_ROAD) {
        placement_cost *= building_construction_place_road(0, x_start, y_start, x_end, y_end);
    } else if (construction_data.type == BUILDING_PLAZA) {
        placement_cost *= place_plaza(x_start, y_start, x_end, y_end);
    } else if (construction_data.type == BUILDING_GARDENS) {
        placement_cost *= place_garden(x_start, y_start, x_end, y_end);
        map_routing_update_land();
    } else if (construction_data.type == BUILDING_LOW_BRIDGE) {
        int length = map_bridge_add(x_end, y_end, 0);
        if (length <= 1) {
            city_warning_show(WARNING_SHORE_NEEDED);
            return;
        }
        placement_cost *= length;
    } else if (construction_data.type == BUILDING_SHIP_BRIDGE) {
        int length = map_bridge_add(x_end, y_end, 1);
        if (length <= 1) {
            city_warning_show(WARNING_SHORE_NEEDED);
            return;
        }
        placement_cost *= length;
    } else if (construction_data.type == BUILDING_AQUEDUCT) {
        int cost;
        if (!building_construction_place_aqueduct(x_start, y_start, x_end, y_end, &cost)) {
            city_warning_show(WARNING_CLEAR_LAND_NEEDED);
            return;
        }
        placement_cost = cost;
        map_tiles_update_all_aqueducts(0);
        map_routing_update_land();
    } else if (construction_data.type == BUILDING_HOUSE_VACANT_LOT) {
        placement_cost *= place_houses(0, x_start, y_start, x_end, y_end);
    } else {
        int terrain_mask = TERRAIN_ALL;
        if (construction_data.type == BUILDING_GATEHOUSE || construction_data.type == BUILDING_TRIUMPHAL_ARCH) {
            terrain_mask = ~TERRAIN_ROAD;
        } else if (construction_data.type == BUILDING_TOWER) {
            terrain_mask = ~TERRAIN_WALL;
        }
        int size = building_properties[construction_data.type].size;
        if (construction_data.type == BUILDING_WAREHOUSE) {
            size = 3;
        }
        int building_orientation = 0;
        if (construction_data.type == BUILDING_GATEHOUSE) {
            building_orientation = map_orientation_for_gatehouse(x_end, y_end);
        } else if (construction_data.type == BUILDING_TRIUMPHAL_ARCH) {
            building_orientation = map_orientation_for_triumphal_arch(x_end, y_end);
        }
        switch (city_view_orientation()) {
            case DIR_2_RIGHT: x_end = x_end - size + 1; break;
            case DIR_4_BOTTOM: x_end = x_end - size + 1; y_end = y_end - size + 1; break;
            case DIR_6_LEFT: y_end = y_end - size + 1; break;
        }
        // extra checks
        if (construction_data.type == BUILDING_GATEHOUSE) {
            if (!is_clear(x_end, y_end, size, terrain_mask, 0)) {
                city_warning_show(WARNING_CLEAR_LAND_NEEDED);
                return;
            }
            if (!building_orientation) {
                if (building_construction_road_orientation() == 1) {
                    building_orientation = 1;
                } else {
                    building_orientation = 2;
                }
            }
        }
        if (construction_data.type == BUILDING_TRIUMPHAL_ARCH) {
            if (!is_clear(x_end, y_end, size, terrain_mask, 0)) {
                city_warning_show(WARNING_CLEAR_LAND_NEEDED);
                return;
            }
            if (!building_orientation) {
                if (building_construction_road_orientation() == 1) {
                    building_orientation = 1;
                } else {
                    building_orientation = 3;
                }
            }
        }
        int waterside_orientation_abs = 0, waterside_orientation_rel = 0;
        if (construction_data.type == BUILDING_SHIPYARD || construction_data.type == BUILDING_WHARF) {
            if (map_water_determine_orientation_size2(
                x_end, y_end, 0, &waterside_orientation_abs, &waterside_orientation_rel)) {
                city_warning_show(WARNING_SHORE_NEEDED);
                return;
            }
        } else if (construction_data.type == BUILDING_DOCK) {
            if (map_water_determine_orientation_size3(
                x_end, y_end, 0, &waterside_orientation_abs, &waterside_orientation_rel)) {
                city_warning_show(WARNING_SHORE_NEEDED);
                return;
            }
            map_routing_calculate_distances_water_boat(scenario.river_entry_point.x, scenario.river_entry_point.y);
            if (!map_terrain_is_adjacent_to_open_water(x_end, y_end, 3)) {
                city_warning_show(WARNING_DOCK_OPEN_WATER_NEEDED);
                return;
            }
        } else {
            if (!is_clear(x_end, y_end, size, terrain_mask, 0)) {
                city_warning_show(WARNING_CLEAR_LAND_NEEDED);
                return;
            }
            int warning_id;
            if (!check_building_terrain_requirements(x_end, y_end, &warning_id)) {
                city_warning_show(warning_id);
                return;
            }
        }
        if (building_is_fort(construction_data.type)) {
            if (!is_clear(x_end + 3, y_end - 1, 4, terrain_mask, 0)) {
                city_warning_show(WARNING_CLEAR_LAND_NEEDED);
                return;
            }
            if (city_data.military.total_legions >= MAX_LEGIONS) {
                city_warning_show(WARNING_MAX_LEGIONS_REACHED);
                return;
            }
        }
        if (construction_data.type == BUILDING_HIPPODROME) {
            if (city_data.building.hippodrome_placed) {
                city_warning_show(WARNING_ONE_BUILDING_OF_TYPE);
                return;
            }
            if (!is_clear(x_end + 5, y_end, 5, terrain_mask, 0)
               || !is_clear(x_end + 10, y_end, 5, terrain_mask, 0)) {
                city_warning_show(WARNING_CLEAR_LAND_NEEDED);
                return;
            }
        }
        if (construction_data.type == BUILDING_SENATE && city_data.building.senate_placed) {
            city_warning_show(WARNING_ONE_BUILDING_OF_TYPE);
            return;
        }
        if (construction_data.type == BUILDING_BARRACKS && building_count_total(BUILDING_BARRACKS) > 0) {
            city_warning_show(WARNING_ONE_BUILDING_OF_TYPE);
            return;
        }
        building_construction_warning_check_food_stocks(construction_data.type);
        if (!has_warning && construction_data.type != BUILDING_WELL && !building_is_fort(construction_data.type)) {
            if (building_properties[construction_data.type].n_laborers > 0 && city_data.labor.workers_needed >= 10) {
                show(WARNING_WORKERS_NEEDED);
            }
        }
        if (!has_warning && construction_data.type == BUILDING_GRANARY) {
            if (building_count_active(BUILDING_MARKET) <= 0) {
                show(WARNING_BUILD_MARKET);
            }
        }
        if (!has_warning && construction_data.type == BUILDING_THEATER) {
            if (building_count_active(BUILDING_ACTOR_COLONY) <= 0) {
                show(WARNING_BUILD_ACTOR_COLONY);
            }
        }
        if (!has_warning && construction_data.type == BUILDING_AMPHITHEATER) {
            if (building_count_active(BUILDING_GLADIATOR_SCHOOL) <= 0) {
                show(WARNING_BUILD_GLADIATOR_SCHOOL);
            }
        }
        if (!has_warning && construction_data.type == BUILDING_COLOSSEUM) {
            if (building_count_active(BUILDING_LION_HOUSE) <= 0) {
                show(WARNING_BUILD_LION_HOUSE);
            }
        }
        if (!has_warning && construction_data.type == BUILDING_HIPPODROME) {
            if (building_count_active(BUILDING_CHARIOT_MAKER) <= 0) {
                show(WARNING_BUILD_CHARIOT_MAKER);
            }
        }
        if (!has_warning) {
            if (building_is_fort(construction_data.type) && building_count_active(BUILDING_BARRACKS) <= 0) {
                show(WARNING_BUILD_BARRACKS);
            }
        }
        if (!has_warning && construction_data.type == BUILDING_BARRACKS) {
            if (city_data.resource.stored_in_warehouses[RESOURCE_WEAPONS] <= 0) {
                show(WARNING_WEAPONS_NEEDED);
            }
        }
        if (!has_warning && construction_data.type == BUILDING_TOWER) {
            int terrain_is_adjacent_to_wall = 0;
            int base_offset = map_grid_offset(x_end, y_end);
            for (const int *tile_delta = map_grid_adjacent_offsets(size); *tile_delta; tile_delta++) {
                if (map_terrain_is(base_offset + *tile_delta, TERRAIN_WALL)) {
                    terrain_is_adjacent_to_wall = 1;
                    break;
                }
            }
            if (!terrain_is_adjacent_to_wall) {
                show(WARNING_SENTRIES_NEED_WALL);
            }
        }
        if (!has_warning
        && (construction_data.type == BUILDING_FOUNTAIN || construction_data.type == BUILDING_BATHHOUSE)) {
            int grid_offset = map_grid_offset(x_end, y_end);
            int has_water = 0;
            if (map_terrain_is(grid_offset, TERRAIN_RESERVOIR_RANGE)) {
                has_water = 1;
            } else if (construction_data.type == BUILDING_BATHHOUSE) {
                if (map_terrain_is(grid_offset + map_grid_delta(1, 0), TERRAIN_RESERVOIR_RANGE) ||
                    map_terrain_is(grid_offset + map_grid_delta(0, 1), TERRAIN_RESERVOIR_RANGE) ||
                    map_terrain_is(grid_offset + map_grid_delta(1, 1), TERRAIN_RESERVOIR_RANGE)) {
                    has_water = 1;
                }
            }
            if (!has_water) {
                show(WARNING_WATER_PIPE_ACCESS_NEEDED);
            }
        }
        switch (construction_data.type) {
            case BUILDING_SMALL_STATUE:
            case BUILDING_MEDIUM_STATUE:
            case BUILDING_LARGE_STATUE:
            case BUILDING_FOUNTAIN:
            case BUILDING_WELL:
            case BUILDING_RESERVOIR:
            case BUILDING_GATEHOUSE:
            case BUILDING_TRIUMPHAL_ARCH:
            case BUILDING_HOUSE_VACANT_LOT:
            case BUILDING_FORT_LEGIONARIES:
            case BUILDING_FORT_JAVELIN:
            case BUILDING_FORT_MOUNTED:
                break;
            default:
            {
                int has_road = 0;
                if (map_has_road_access(x_end, y_end, size, 0)
                || (construction_data.type == BUILDING_WAREHOUSE && map_has_road_access(x_end, y_end, size, 0))
                || (construction_data.type == BUILDING_HIPPODROME && map_has_road_access_hippodrome(x_end, y_end, 0))) {
                    has_road = 1;
                }
                if (!has_road) {
                    show(WARNING_ROAD_ACCESS_NEEDED);
                }
            }
        }
        // check raw resources availability
        int raw_resource = 0;
        int finished_good = 0;
        int warning_resource_needed = 0;
        int warning_resource_building = 0;
        switch (construction_data.type) {
            case BUILDING_OIL_WORKSHOP:
                raw_resource = RESOURCE_OLIVES;
                finished_good = RESOURCE_OIL;
                warning_resource_needed = WARNING_OLIVES_NEEDED;
                warning_resource_building = WARNING_BUILD_OLIVE_FARM;
                break;
            case BUILDING_WINE_WORKSHOP:
                raw_resource = RESOURCE_VINES;
                finished_good = RESOURCE_WINE;
                warning_resource_needed = WARNING_VINES_NEEDED;
                warning_resource_building = WARNING_BUILD_VINES_FARM;
                break;
            case BUILDING_WEAPONS_WORKSHOP:
                raw_resource = RESOURCE_IRON;
                finished_good = RESOURCE_WEAPONS;
                warning_resource_needed = WARNING_IRON_NEEDED;
                warning_resource_building = WARNING_BUILD_IRON_MINE;
                break;
            case BUILDING_FURNITURE_WORKSHOP:
                raw_resource = RESOURCE_TIMBER;
                finished_good = RESOURCE_FURNITURE;
                warning_resource_needed = WARNING_TIMBER_NEEDED;
                warning_resource_building = WARNING_BUILD_TIMBER_YARD;
                break;
            case BUILDING_POTTERY_WORKSHOP:
                raw_resource = RESOURCE_CLAY;
                finished_good = RESOURCE_POTTERY;
                warning_resource_needed = WARNING_CLAY_NEEDED;
                warning_resource_building = WARNING_BUILD_CLAY_PIT;
                break;
            default:
                break;
        }
        if (raw_resource && !building_count_industry_active(raw_resource)) {
            if (city_data.resource.stored_in_warehouses[finished_good] <= 0 && city_data.resource.stored_in_warehouses[raw_resource] <= 0) {
                show(warning_resource_needed);
                int empire_offers_resource = 0;
                for (int i = 0; i < MAX_EMPIRE_OBJECTS; i++) {
                    if (empire_objects[i].in_use
                        && empire_objects[i].city_type == EMPIRE_CITY_TRADE
                        && empire_objects[i].resource_sell_limit[raw_resource]) {
                        empire_offers_resource = 1;
                        break;
                    }
                }
                if (our_city_can_produce_resource(raw_resource)) {
                    show(warning_resource_building);
                } else if (empire_offers_resource && !resource_import_trade_route_open(raw_resource)) {
                    show(WARNING_OPEN_TRADE_TO_IMPORT);
                } else if (city_data.resource.trade_status[raw_resource] != TRADE_STATUS_IMPORT) {
                    show(WARNING_TRADE_IMPORT_RESOURCE);
                }
            }
        }
        // phew, checks done!
        struct building_t *b;
        b = building_create(construction_data.type, x_end, y_end);
        game_undo_add_building(b);
        if (b->id <= 0) {
            return;
        }
        switch (b->type) {
            case BUILDING_HOUSE_LARGE_TENT:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_TENT) + 2, TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_SMALL_SHACK:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_SHACK), TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_LARGE_SHACK:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_SHACK) + 2, TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_SMALL_HOVEL:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_HOVEL), TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_LARGE_HOVEL:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_HOVEL) + 2, TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_SMALL_CASA:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_CASA), TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_LARGE_CASA:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_CASA) + 2, TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_SMALL_INSULA:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_INSULA_1), TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_MEDIUM_INSULA:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_INSULA_1) + 2, TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_LARGE_INSULA:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_INSULA_2), TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_GRAND_INSULA:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_INSULA_2) + 2, TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_SMALL_VILLA:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_VILLA_1), TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_MEDIUM_VILLA:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_VILLA_1) + 2, TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_LARGE_VILLA:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_VILLA_2), TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_GRAND_VILLA:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_VILLA_2) + 1, TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_SMALL_PALACE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_PALACE_1), TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_MEDIUM_PALACE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_PALACE_1) + 1, TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_LARGE_PALACE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_PALACE_2), TERRAIN_BUILDING);
                break;
            case BUILDING_HOUSE_LUXURY_PALACE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_PALACE_2) + 1, TERRAIN_BUILDING);
                break;
            case BUILDING_AMPHITHEATER:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_AMPHITHEATER), TERRAIN_BUILDING);
                break;
            case BUILDING_THEATER:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_THEATER), TERRAIN_BUILDING);
                break;
            case BUILDING_COLOSSEUM:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_COLOSSEUM), TERRAIN_BUILDING);
                break;
            case BUILDING_GLADIATOR_SCHOOL:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_GLADIATOR_SCHOOL), TERRAIN_BUILDING);
                break;
            case BUILDING_LION_HOUSE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_LION_HOUSE), TERRAIN_BUILDING);
                break;
            case BUILDING_ACTOR_COLONY:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_ACTOR_COLONY), TERRAIN_BUILDING);
                break;
            case BUILDING_CHARIOT_MAKER:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_CHARIOT_MAKER), TERRAIN_BUILDING);
                break;
            case BUILDING_SMALL_STATUE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_STATUE), TERRAIN_BUILDING);
                break;
            case BUILDING_MEDIUM_STATUE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_STATUE) + 1, TERRAIN_BUILDING);
                break;
            case BUILDING_LARGE_STATUE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_STATUE) + 2, TERRAIN_BUILDING);
                break;
            case BUILDING_DOCTOR:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_DOCTOR), TERRAIN_BUILDING);
                break;
            case BUILDING_HOSPITAL:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOSPITAL), TERRAIN_BUILDING);
                break;
            case BUILDING_BATHHOUSE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_BATHHOUSE_NO_WATER), TERRAIN_BUILDING);
                break;
            case BUILDING_BARBER:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_BARBER), TERRAIN_BUILDING);
                break;
            case BUILDING_SCHOOL:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_SCHOOL), TERRAIN_BUILDING);
                break;
            case BUILDING_ACADEMY:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_ACADEMY), TERRAIN_BUILDING);
                break;
            case BUILDING_LIBRARY:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_LIBRARY), TERRAIN_BUILDING);
                break;
            case BUILDING_PREFECTURE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_PREFECTURE), TERRAIN_BUILDING);
                break;
            case BUILDING_WHEAT_FARM:
                map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS), 0);
                break;
            case BUILDING_VEGETABLE_FARM:
                map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS) + 5, 0);
                break;
            case BUILDING_FRUIT_FARM:
                map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS) + 10, 0);
                break;
            case BUILDING_OLIVE_FARM:
                map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS) + 15, 0);
                break;
            case BUILDING_VINES_FARM:
                map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS) + 20, 0);
                break;
            case BUILDING_PIG_FARM:
                map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS) + 25, 0);
                break;
            case BUILDING_MARBLE_QUARRY:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_MARBLE_QUARRY), TERRAIN_BUILDING);
                break;
            case BUILDING_IRON_MINE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_IRON_MINE), TERRAIN_BUILDING);
                break;
            case BUILDING_TIMBER_YARD:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TIMBER_YARD), TERRAIN_BUILDING);
                break;
            case BUILDING_CLAY_PIT:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_CLAY_PIT), TERRAIN_BUILDING);
                break;
            case BUILDING_WINE_WORKSHOP:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_WINE_WORKSHOP), TERRAIN_BUILDING);
                break;
            case BUILDING_OIL_WORKSHOP:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_OIL_WORKSHOP), TERRAIN_BUILDING);
                break;
            case BUILDING_WEAPONS_WORKSHOP:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_WEAPONS_WORKSHOP), TERRAIN_BUILDING);
                break;
            case BUILDING_FURNITURE_WORKSHOP:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_FURNITURE_WORKSHOP), TERRAIN_BUILDING);
                break;
            case BUILDING_POTTERY_WORKSHOP:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_POTTERY_WORKSHOP), TERRAIN_BUILDING);
                break;
            case BUILDING_GRANARY:
                b->storage_id = building_storage_create();
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_GRANARY), TERRAIN_BUILDING);
                foreach_region_tile(b->x - 1, b->y - 1, b->x + 5 - 2, b->y + 5 - 2, set_road_image);
                break;
            case BUILDING_MARKET:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_MARKET), TERRAIN_BUILDING);
                break;
            case BUILDING_GOVERNORS_HOUSE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_GOVERNORS_HOUSE), TERRAIN_BUILDING);
                break;
            case BUILDING_GOVERNORS_VILLA:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_GOVERNORS_VILLA), TERRAIN_BUILDING);
                break;
            case BUILDING_GOVERNORS_PALACE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_GOVERNORS_PALACE), TERRAIN_BUILDING);
                break;
            case BUILDING_MISSION_POST:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_MISSION_POST), TERRAIN_BUILDING);
                break;
            case BUILDING_ENGINEERS_POST:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_ENGINEERS_POST), TERRAIN_BUILDING);
                break;
            case BUILDING_FORUM:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_FORUM), TERRAIN_BUILDING);
                break;
            case BUILDING_RESERVOIR:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_RESERVOIR), TERRAIN_BUILDING);
                break;
            case BUILDING_FOUNTAIN:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_FOUNTAIN_1), TERRAIN_BUILDING);
                break;
            case BUILDING_WELL:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_WELL), TERRAIN_BUILDING);
                break;
            case BUILDING_MILITARY_ACADEMY:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_MILITARY_ACADEMY), TERRAIN_BUILDING);
                break;
            case BUILDING_SMALL_TEMPLE_CERES:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_CERES), TERRAIN_BUILDING);
                break;
            case BUILDING_SMALL_TEMPLE_NEPTUNE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_NEPTUNE), TERRAIN_BUILDING);
                break;
            case BUILDING_SMALL_TEMPLE_MERCURY:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_MERCURY), TERRAIN_BUILDING);
                break;
            case BUILDING_SMALL_TEMPLE_MARS:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_MARS), TERRAIN_BUILDING);
                break;
            case BUILDING_SMALL_TEMPLE_VENUS:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_VENUS), TERRAIN_BUILDING);
                break;
            case BUILDING_LARGE_TEMPLE_CERES:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_CERES) + 1, TERRAIN_BUILDING);
                break;
            case BUILDING_LARGE_TEMPLE_NEPTUNE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_NEPTUNE) + 1, TERRAIN_BUILDING);
                break;
            case BUILDING_LARGE_TEMPLE_MERCURY:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_MERCURY) + 1, TERRAIN_BUILDING);
                break;
            case BUILDING_LARGE_TEMPLE_MARS:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_MARS) + 1, TERRAIN_BUILDING);
                break;
            case BUILDING_LARGE_TEMPLE_VENUS:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_VENUS) + 1, TERRAIN_BUILDING);
                break;
            case BUILDING_ORACLE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_ORACLE), TERRAIN_BUILDING);
                break;
            case BUILDING_SHIPYARD:
                b->data.industry.orientation = waterside_orientation_abs;
                map_water_add_building(b->id, b->x, b->y, 2,
                    image_group(GROUP_BUILDING_SHIPYARD) + waterside_orientation_rel);
                break;
            case BUILDING_WHARF:
                b->data.industry.orientation = waterside_orientation_abs;
                map_water_add_building(b->id, b->x, b->y, 2,
                    image_group(GROUP_BUILDING_WHARF) + waterside_orientation_rel);
                break;
            case BUILDING_DOCK:
                city_data.building.working_docks++;
                b->data.dock.orientation = waterside_orientation_abs;
                {
                    int image_id;
                    switch (waterside_orientation_rel) {
                        case 0: image_id = image_group(GROUP_BUILDING_DOCK_1); break;
                        case 1: image_id = image_group(GROUP_BUILDING_DOCK_2); break;
                        case 2: image_id = image_group(GROUP_BUILDING_DOCK_3); break;
                        default:image_id = image_group(GROUP_BUILDING_DOCK_4); break;
                    }
                    map_water_add_building(b->id, b->x, b->y, b->size, image_id);
                }
                break;
            case BUILDING_TOWER:
            {
                int x_min, y_min, x_max, y_max;
                map_grid_get_area(b->x, b->y, size, 0, &x_min, &y_min, &x_max, &y_max);

                for (int yy = y_min; yy <= y_max; yy++) {
                    for (int xx = x_min; xx <= x_max; xx++) {
                        terrain_grid.items[map_grid_offset(xx, yy)] &= ~TERRAIN_WALL;
                    }
                }
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TOWER),
                    TERRAIN_BUILDING | TERRAIN_GATEHOUSE);
                foreach_region_tile(b->x - 1, b->y - 1, b->x + 5 - 2, b->y + 5 - 2, set_wall_image);
                break;
            }
            case BUILDING_GATEHOUSE:
                map_building_tiles_add(b->id, b->x, b->y, b->size,
                    image_group(GROUP_BUILDING_TOWER) + building_orientation, TERRAIN_BUILDING | TERRAIN_GATEHOUSE);
                b->subtype.orientation = building_orientation;
                map_orientation_update_buildings();
                map_terrain_add_gatehouse_roads(b->x, b->y, building_orientation);
                foreach_region_tile(b->x - 1, b->y - 1, b->x + 5 - 2, b->y + 5 - 2, set_road_image);
                map_tiles_update_all_plazas();
                foreach_region_tile(b->x - 1, b->y - 1, b->x + 5 - 2, b->y + 5 - 2, set_wall_image);
                break;
            case BUILDING_TRIUMPHAL_ARCH:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TRIUMPHAL_ARCH) + building_orientation - 1, TERRAIN_BUILDING);
                b->subtype.orientation = building_orientation;
                map_orientation_update_buildings();
                map_terrain_add_triumphal_arch_roads(b->x, b->y, building_orientation);
                foreach_region_tile(b->x - 1, b->y - 1, b->x + 5 - 2, b->y + 5 - 2, set_road_image);
                map_tiles_update_all_plazas();
                city_data.building.triumphal_arches_available--;
                if (!city_data.building.triumphal_arches_available) { // none left
                    build_menus[MENU_ADMINISTRATION].menu_items[10].building_id = 0;
                    // disable menu if this was the only enabled item
                    int menu_enabled = 0;
                    for (int j = 0; j < MAX_ITEMS_PER_BUILD_MENU; j++) {
                        if (build_menus[MENU_ADMINISTRATION].menu_items[j].building_id) {
                            menu_enabled = 1;
                            break;
                        }
                    }
                    if (!menu_enabled) {
                        build_menus[MENU_ADMINISTRATION].is_enabled = 0;
                    }
                }
                building_construction_clear_type();
                break;
            case BUILDING_SENATE:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_SENATE), TERRAIN_BUILDING);
                city_data.building.senate_placed = 1;
                if (!city_data.building.senate_grid_offset) {
                    city_data.building.senate_building_id = b->id;
                    city_data.building.senate_x = b->x;
                    city_data.building.senate_y = b->y;
                    city_data.building.senate_grid_offset = b->grid_offset;
                }
                break;
            case BUILDING_BARRACKS:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_BARRACKS), TERRAIN_BUILDING);
                if (!city_data.building.barracks_grid_offset) {
                    city_data.building.barracks_building_id = b->id;
                    city_data.building.barracks_x = b->x;
                    city_data.building.barracks_y = b->y;
                    city_data.building.barracks_grid_offset = b->grid_offset;
                }
                break;
            case BUILDING_WAREHOUSE:
                add_warehouse(b);
                break;
            case BUILDING_HIPPODROME:
            {
                int image1 = image_group(GROUP_BUILDING_HIPPODROME_1);
                int image2 = image_group(GROUP_BUILDING_HIPPODROME_2);
                city_data.building.hippodrome_placed = 1;
                int orientation = city_view_orientation();
                struct building_t *part1 = b;
                if (orientation == DIR_0_TOP || orientation == DIR_4_BOTTOM) {
                    part1->subtype.orientation = 0;
                } else {
                    part1->subtype.orientation = 3;
                }
                part1->prev_part_building_id = 0;
                int image_id;
                switch (orientation) {
                    case DIR_0_TOP:
                        image_id = image2;
                        break;
                    case DIR_2_RIGHT:
                        image_id = image1 + 4;
                        break;
                    case DIR_4_BOTTOM:
                        image_id = image2 + 4;
                        break;
                    case DIR_6_LEFT:
                        image_id = image1;
                        break;
                    default:
                        return;
                }
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_id, TERRAIN_BUILDING);
                struct building_t *part2 = building_create(BUILDING_HIPPODROME, b->x + 5, b->y);
                game_undo_add_building(part2);
                if (orientation == DIR_0_TOP || orientation == DIR_4_BOTTOM) {
                    part2->subtype.orientation = 1;
                } else {
                    part2->subtype.orientation = 4;
                }
                part2->prev_part_building_id = part1->id;
                part1->next_part_building_id = part2->id;
                part2->next_part_building_id = 0;
                switch (orientation) {
                    case DIR_0_TOP:
                    case DIR_4_BOTTOM:
                        image_id = image2 + 2;
                        break;
                    case DIR_2_RIGHT:
                    case DIR_6_LEFT:
                        image_id = image1 + 2;
                        break;
                }
                map_building_tiles_add(part2->id, b->x + 5, b->y, b->size, image_id, TERRAIN_BUILDING);
                struct building_t *part3 = building_create(BUILDING_HIPPODROME, b->x + 10, b->y);
                game_undo_add_building(part3);
                if (orientation == DIR_0_TOP || orientation == DIR_4_BOTTOM) {
                    part3->subtype.orientation = 2;
                } else {
                    part3->subtype.orientation = 5;
                }
                part3->prev_part_building_id = part2->id;
                part2->next_part_building_id = part3->id;
                part3->next_part_building_id = 0;
                switch (orientation) {
                    case DIR_0_TOP:
                        image_id = image2 + 4;
                        break;
                    case DIR_2_RIGHT:
                        image_id = image1;
                        break;
                    case DIR_4_BOTTOM:
                        image_id = image2;
                        break;
                    case DIR_6_LEFT:
                        image_id = image1 + 4;
                        break;
                }
                map_building_tiles_add(part3->id, b->x + 10, b->y, b->size, image_id, TERRAIN_BUILDING);
                break;
            }
            case BUILDING_FORT_LEGIONARIES:
            case BUILDING_FORT_JAVELIN:
            case BUILDING_FORT_MOUNTED:
                b->prev_part_building_id = 0;
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_FORT), TERRAIN_BUILDING);
                if (b->type == BUILDING_FORT_LEGIONARIES) {
                    b->subtype.fort_figure_type = FIGURE_FORT_LEGIONARY;
                } else if (b->type == BUILDING_FORT_JAVELIN) {
                    b->subtype.fort_figure_type = FIGURE_FORT_JAVELIN;
                } else if (b->type == BUILDING_FORT_MOUNTED) {
                    b->subtype.fort_figure_type = FIGURE_FORT_MOUNTED;
                }

                b->formation_id = create_legion_formation_for_fort(b);
                // create parade ground
                struct building_t *ground = building_create(BUILDING_FORT_GROUND, b->x + 3, b->y - 1);
                game_undo_add_building(ground);
                ground->formation_id = b->formation_id;
                ground->prev_part_building_id = b->id;
                b->next_part_building_id = ground->id;
                ground->next_part_building_id = 0;
                map_building_tiles_add(ground->id, b->x + 3, b->y - 1, 4, image_group(GROUP_BUILDING_FORT) + 1, TERRAIN_BUILDING);
                break;
            case BUILDING_NATIVE_HUT:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_NATIVE) + (random_byte() & 1), TERRAIN_BUILDING);
                break;
            case BUILDING_NATIVE_MEETING:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_NATIVE) + 2, TERRAIN_BUILDING);
                break;
            case BUILDING_NATIVE_CROPS:
                map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_FARM_CROPS), TERRAIN_BUILDING);
                break;
        }
        map_routing_update_land();
        map_routing_update_walls();
    }
    if ((construction_data.type >= BUILDING_LARGE_TEMPLE_CERES && construction_data.type <= BUILDING_LARGE_TEMPLE_VENUS) || construction_data.type == BUILDING_ORACLE) {
        building_warehouses_remove_resource(RESOURCE_MARBLE, 2);
    }
    if (construction_data.type >= BUILDING_SMALL_TEMPLE_CERES && construction_data.type <= BUILDING_SMALL_TEMPLE_VENUS) {
        construction_data.type++;
        if (construction_data.type > BUILDING_SMALL_TEMPLE_VENUS) {
            construction_data.type = BUILDING_SMALL_TEMPLE_CERES;
        }
    }
    if (construction_data.type >= BUILDING_LARGE_TEMPLE_CERES && construction_data.type <= BUILDING_LARGE_TEMPLE_VENUS) {
        construction_data.type++;
        if (construction_data.type > BUILDING_LARGE_TEMPLE_VENUS) {
            construction_data.type = BUILDING_LARGE_TEMPLE_CERES;
        }
    }
    city_finance_process_construction(placement_cost);
    // move herds away
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        if (herd_formations[i].in_use && herd_formations[i].figure_type != FIGURE_WOLF) {
            if (calc_maximum_distance(x_end, y_end, herd_formations[i].destination_x, herd_formations[i].destination_y) <= 6) {
                // force new roaming destination search
                herd_formations[i].wait_ticks_movement = SHEEP_HERD_ROAM_DELAY; // largest roam delay
            }
        }
    }
    if (construction_data.type != BUILDING_TRIUMPHAL_ARCH) {
        game_undo_finish_build(placement_cost);
    }
}

static void set_warning(int *warning_id, int warning)
{
    if (warning_id) {
        *warning_id = warning;
    }
}

int check_building_terrain_requirements(int x, int y, int *warning_id)
{
    if (construction_data.required_terrain.meadow) {
        if (!map_terrain_exists_tile_in_radius_with_type(x, y, 3, 1, TERRAIN_MEADOW)) {
            set_warning(warning_id, WARNING_MEADOW_NEEDED);
            return 0;
        }
    } else if (construction_data.required_terrain.rock) {
        if (!map_terrain_exists_tile_in_radius_with_type(x, y, 2, 1, TERRAIN_ELEVATION)) {
            set_warning(warning_id, WARNING_ROCK_NEEDED);
            return 0;
        }
    } else if (construction_data.required_terrain.tree) {
        if (!map_terrain_exist_multiple_tiles_in_radius_with_type(x, y, 2, 1, TERRAIN_TREE | TERRAIN_SHRUB, 3)) {
            set_warning(warning_id, WARNING_TREE_NEEDED);
            return 0;
        }
    } else if (construction_data.required_terrain.water) {
        if (!map_terrain_exists_tile_in_radius_with_type(x, y, 2, 3, TERRAIN_WATER)) {
            set_warning(warning_id, WARNING_WATER_NEEDED);
            return 0;
        }
    } else if (construction_data.required_terrain.wall) {
        if (!map_terrain_all_tiles_in_radius_are(x, y, 2, 0, TERRAIN_WALL)) {
            set_warning(warning_id, WARNING_WALL_NEEDED);
            return 0;
        }
    }
    return 1;
}

void building_construction_update_road_orientation(void)
{
    if (construction_data.road_orientation > 0) {
        if (time_get_millis() - construction_data.road_last_update > 1500) {
            construction_data.road_last_update = time_get_millis();
            construction_data.road_orientation = construction_data.road_orientation == 1 ? 2 : 1;
        }
    }
}

int building_construction_road_orientation(void)
{
    return construction_data.road_orientation;
}

void building_construction_record_view_position(int view_x, int view_y, int grid_offset)
{
    if (grid_offset == construction_data.start.grid_offset) {
        construction_data.start_offset_x_view = view_x;
        construction_data.start_offset_y_view = view_y;
    }
}

void building_construction_get_view_position(int *view_x, int *view_y)
{
    *view_x = construction_data.start_offset_x_view;
    *view_y = construction_data.start_offset_y_view;
}

int building_construction_get_start_grid_offset(void)
{
    return construction_data.start.grid_offset;
}

void building_construction_reset_draw_as_constructing(void)
{
    construction_data.draw_as_constructing = 0;
}

int building_construction_draw_as_constructing(void)
{
    return construction_data.draw_as_constructing;
}

static void clear_counters(void)
{
    memset(&count_data, 0, sizeof(count_data));
}

static void increase_count(int type, int active)
{
    ++count_data.buildings[type].total;
    if (active) {
        ++count_data.buildings[type].active;
    }
}

static void increase_industry_count(int resource, int active)
{
    ++count_data.industry[resource].total;
    if (active) {
        ++count_data.industry[resource].active;
    }
}

static void limit_hippodrome(void)
{
    if (count_data.buildings[BUILDING_HIPPODROME].total > 1) {
        count_data.buildings[BUILDING_HIPPODROME].total = 1;
    }
    if (count_data.buildings[BUILDING_HIPPODROME].active > 1) {
        count_data.buildings[BUILDING_HIPPODROME].active = 1;
    }
}

void building_count_update(void)
{
    clear_counters();
    city_data.building.working_wharfs = 0;
    city_data.building.shipyard_boats_requested = 0;
    for (int i = 0; i < 8; i++) {
        city_data.building.working_dock_ids[i] = 0;
    }
    city_data.building.working_docks = 0;
    city_data.health.num_hospital_workers = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->house_size) {
            continue;
        }
        int is_entertainment_venue = 0;
        int type = b->type;
        switch (type) {
            // SPECIAL TREATMENT
            // entertainment venues
            case BUILDING_THEATER:
            case BUILDING_AMPHITHEATER:
            case BUILDING_COLOSSEUM:
            case BUILDING_HIPPODROME:
                is_entertainment_venue = 1;
                increase_count(type, b->num_workers > 0);
                break;
            case BUILDING_BARRACKS:
                city_data.building.barracks_building_id = i;
                increase_count(type, b->num_workers > 0);
                break;
            case BUILDING_HOSPITAL:
                increase_count(type, b->num_workers > 0);
                city_data.health.num_hospital_workers += b->num_workers;
                break;
                // water
            case BUILDING_RESERVOIR:
            case BUILDING_FOUNTAIN:
                increase_count(type, b->has_water_access);
                break;
                // DEFAULT TREATMENT
                // education
            case BUILDING_SCHOOL:
            case BUILDING_LIBRARY:
            case BUILDING_ACADEMY:
                // health
            case BUILDING_BARBER:
            case BUILDING_BATHHOUSE:
            case BUILDING_DOCTOR:
                // government
            case BUILDING_FORUM:
            case BUILDING_SENATE:
                // entertainment schools
            case BUILDING_ACTOR_COLONY:
            case BUILDING_GLADIATOR_SCHOOL:
            case BUILDING_LION_HOUSE:
            case BUILDING_CHARIOT_MAKER:
                // distribution
            case BUILDING_MARKET:
                // military
            case BUILDING_MILITARY_ACADEMY:
                // religion
            case BUILDING_SMALL_TEMPLE_CERES:
            case BUILDING_SMALL_TEMPLE_NEPTUNE:
            case BUILDING_SMALL_TEMPLE_MERCURY:
            case BUILDING_SMALL_TEMPLE_MARS:
            case BUILDING_SMALL_TEMPLE_VENUS:
            case BUILDING_LARGE_TEMPLE_CERES:
            case BUILDING_LARGE_TEMPLE_NEPTUNE:
            case BUILDING_LARGE_TEMPLE_MERCURY:
            case BUILDING_LARGE_TEMPLE_MARS:
            case BUILDING_LARGE_TEMPLE_VENUS:
            case BUILDING_ORACLE:
                increase_count(type, b->num_workers > 0);
                break;
                // industry
            case BUILDING_WHEAT_FARM:
                increase_industry_count(RESOURCE_WHEAT, b->num_workers > 0);
                break;
            case BUILDING_VEGETABLE_FARM:
                increase_industry_count(RESOURCE_VEGETABLES, b->num_workers > 0);
                break;
            case BUILDING_FRUIT_FARM:
                increase_industry_count(RESOURCE_FRUIT, b->num_workers > 0);
                break;
            case BUILDING_OLIVE_FARM:
                increase_industry_count(RESOURCE_OLIVES, b->num_workers > 0);
                break;
            case BUILDING_VINES_FARM:
                increase_industry_count(RESOURCE_VINES, b->num_workers > 0);
                break;
            case BUILDING_PIG_FARM:
                increase_industry_count(RESOURCE_MEAT, b->num_workers > 0);
                break;
            case BUILDING_MARBLE_QUARRY:
                increase_industry_count(RESOURCE_MARBLE, b->num_workers > 0);
                break;
            case BUILDING_IRON_MINE:
                increase_industry_count(RESOURCE_IRON, b->num_workers > 0);
                break;
            case BUILDING_TIMBER_YARD:
                increase_industry_count(RESOURCE_TIMBER, b->num_workers > 0);
                break;
            case BUILDING_CLAY_PIT:
                increase_industry_count(RESOURCE_CLAY, b->num_workers > 0);
                break;
            case BUILDING_WINE_WORKSHOP:
                increase_industry_count(RESOURCE_WINE, b->num_workers > 0);
                break;
            case BUILDING_OIL_WORKSHOP:
                increase_industry_count(RESOURCE_OIL, b->num_workers > 0);
                break;
            case BUILDING_WEAPONS_WORKSHOP:
                increase_industry_count(RESOURCE_WEAPONS, b->num_workers > 0);
                break;
            case BUILDING_FURNITURE_WORKSHOP:
                increase_industry_count(RESOURCE_FURNITURE, b->num_workers > 0);
                break;
            case BUILDING_POTTERY_WORKSHOP:
                increase_industry_count(RESOURCE_POTTERY, b->num_workers > 0);
                break;
                // water-side
            case BUILDING_WHARF:
                if (b->num_workers > 0) {
                    city_data.building.working_wharfs++;
                    if (!b->data.industry.fishing_boat_id) {
                        city_data.building.shipyard_boats_requested++;
                    }
                }
                break;
            case BUILDING_DOCK:
                if (b->num_workers > 0 && b->has_water_access) {
                    city_data.building.working_dock_ids[city_data.building.working_docks] = i;
                    city_data.building.working_docks++;
                }
                break;
            default:
                continue;
        }
        if (b->immigrant_figure_id) {
            struct figure_t *f = &figures[b->immigrant_figure_id];
            if (!figure_is_alive(f) || f->destination_building_id != i) {
                b->immigrant_figure_id = 0;
            }
        }
        if (is_entertainment_venue) {
            // update number of shows
            int shows = 0;
            if (b->data.entertainment.days1 > 0) {
                --b->data.entertainment.days1;
                ++shows;
            }
            if (b->data.entertainment.days2 > 0) {
                --b->data.entertainment.days2;
                ++shows;
            }
            b->data.entertainment.num_shows = shows;
        }
    }
    limit_hippodrome();
}

int building_count_active(int type)
{
    return count_data.buildings[type].active;
}

int building_count_total(int type)
{
    return count_data.buildings[type].total;
}

int building_count_industry_active(int resource)
{
    return count_data.industry[resource].active;
}

int building_count_industry_total(int resource)
{
    return count_data.industry[resource].total;
}

void building_count_save_state(struct buffer_t *industry, struct buffer_t *culture1, struct buffer_t *culture2,
                                struct buffer_t *culture3, struct buffer_t *military, struct buffer_t *support)
{
    // industry
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        buffer_write_i32(industry, count_data.industry[i].total);
    }
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        buffer_write_i32(industry, count_data.industry[i].active);
    }

    // culture 1
    buffer_write_i32(culture1, count_data.buildings[BUILDING_THEATER].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_THEATER].active);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_AMPHITHEATER].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_AMPHITHEATER].active);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_COLOSSEUM].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_COLOSSEUM].active);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_HIPPODROME].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_HIPPODROME].active);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_SCHOOL].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_SCHOOL].active);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_LIBRARY].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_LIBRARY].active);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_ACADEMY].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_ACADEMY].active);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_BARBER].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_BARBER].active);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_BATHHOUSE].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_BATHHOUSE].active);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_DOCTOR].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_DOCTOR].active);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_HOSPITAL].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_HOSPITAL].active);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_SMALL_TEMPLE_CERES].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_SMALL_TEMPLE_NEPTUNE].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_SMALL_TEMPLE_MERCURY].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_SMALL_TEMPLE_MARS].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_SMALL_TEMPLE_VENUS].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_LARGE_TEMPLE_CERES].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_LARGE_TEMPLE_NEPTUNE].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_LARGE_TEMPLE_MERCURY].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_LARGE_TEMPLE_MARS].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_LARGE_TEMPLE_VENUS].total);
    buffer_write_i32(culture1, count_data.buildings[BUILDING_ORACLE].total);

    // culture 2
    buffer_write_i32(culture2, count_data.buildings[BUILDING_ACTOR_COLONY].total);
    buffer_write_i32(culture2, count_data.buildings[BUILDING_ACTOR_COLONY].active);
    buffer_write_i32(culture2, count_data.buildings[BUILDING_GLADIATOR_SCHOOL].total);
    buffer_write_i32(culture2, count_data.buildings[BUILDING_GLADIATOR_SCHOOL].active);
    buffer_write_i32(culture2, count_data.buildings[BUILDING_LION_HOUSE].total);
    buffer_write_i32(culture2, count_data.buildings[BUILDING_LION_HOUSE].active);
    buffer_write_i32(culture2, count_data.buildings[BUILDING_CHARIOT_MAKER].total);
    buffer_write_i32(culture2, count_data.buildings[BUILDING_CHARIOT_MAKER].active);

    // culture 3
    buffer_write_i32(culture3, count_data.buildings[BUILDING_SMALL_TEMPLE_CERES].active);
    buffer_write_i32(culture3, count_data.buildings[BUILDING_SMALL_TEMPLE_NEPTUNE].active);
    buffer_write_i32(culture3, count_data.buildings[BUILDING_SMALL_TEMPLE_MERCURY].active);
    buffer_write_i32(culture3, count_data.buildings[BUILDING_SMALL_TEMPLE_MARS].active);
    buffer_write_i32(culture3, count_data.buildings[BUILDING_SMALL_TEMPLE_VENUS].active);
    buffer_write_i32(culture3, count_data.buildings[BUILDING_LARGE_TEMPLE_CERES].active);
    buffer_write_i32(culture3, count_data.buildings[BUILDING_LARGE_TEMPLE_NEPTUNE].active);
    buffer_write_i32(culture3, count_data.buildings[BUILDING_LARGE_TEMPLE_MERCURY].active);
    buffer_write_i32(culture3, count_data.buildings[BUILDING_LARGE_TEMPLE_MARS].active);
    buffer_write_i32(culture3, count_data.buildings[BUILDING_LARGE_TEMPLE_VENUS].active);

    // military
    buffer_write_i32(military, count_data.buildings[BUILDING_MILITARY_ACADEMY].total);
    buffer_write_i32(military, count_data.buildings[BUILDING_MILITARY_ACADEMY].active);
    buffer_write_i32(military, count_data.buildings[BUILDING_BARRACKS].total);
    buffer_write_i32(military, count_data.buildings[BUILDING_BARRACKS].active);

    // support
    buffer_write_i32(support, count_data.buildings[BUILDING_MARKET].total);
    buffer_write_i32(support, count_data.buildings[BUILDING_MARKET].active);
    buffer_write_i32(support, count_data.buildings[BUILDING_RESERVOIR].total);
    buffer_write_i32(support, count_data.buildings[BUILDING_RESERVOIR].active);
    buffer_write_i32(support, count_data.buildings[BUILDING_FOUNTAIN].total);
    buffer_write_i32(support, count_data.buildings[BUILDING_FOUNTAIN].active);
}

void building_count_load_state(struct buffer_t *industry, struct buffer_t *culture1, struct buffer_t *culture2,
                                struct buffer_t *culture3, struct buffer_t *military, struct buffer_t *support)
{
    // industry
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        count_data.industry[i].total = buffer_read_i32(industry);
    }
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        count_data.industry[i].active = buffer_read_i32(industry);
    }

    // culture 1
    count_data.buildings[BUILDING_THEATER].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_THEATER].active = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_AMPHITHEATER].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_AMPHITHEATER].active = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_COLOSSEUM].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_COLOSSEUM].active = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_HIPPODROME].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_HIPPODROME].active = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_SCHOOL].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_SCHOOL].active = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_LIBRARY].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_LIBRARY].active = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_ACADEMY].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_ACADEMY].active = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_BARBER].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_BARBER].active = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_BATHHOUSE].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_BATHHOUSE].active = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_DOCTOR].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_DOCTOR].active = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_HOSPITAL].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_HOSPITAL].active = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_SMALL_TEMPLE_CERES].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_SMALL_TEMPLE_NEPTUNE].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_SMALL_TEMPLE_MERCURY].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_SMALL_TEMPLE_MARS].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_SMALL_TEMPLE_VENUS].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_LARGE_TEMPLE_CERES].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_LARGE_TEMPLE_NEPTUNE].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_LARGE_TEMPLE_MERCURY].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_LARGE_TEMPLE_MARS].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_LARGE_TEMPLE_VENUS].total = buffer_read_i32(culture1);
    count_data.buildings[BUILDING_ORACLE].total = buffer_read_i32(culture1);

    // culture 2
    count_data.buildings[BUILDING_ACTOR_COLONY].total = buffer_read_i32(culture2);
    count_data.buildings[BUILDING_ACTOR_COLONY].active = buffer_read_i32(culture2);
    count_data.buildings[BUILDING_GLADIATOR_SCHOOL].total = buffer_read_i32(culture2);
    count_data.buildings[BUILDING_GLADIATOR_SCHOOL].active = buffer_read_i32(culture2);
    count_data.buildings[BUILDING_LION_HOUSE].total = buffer_read_i32(culture2);
    count_data.buildings[BUILDING_LION_HOUSE].active = buffer_read_i32(culture2);
    count_data.buildings[BUILDING_CHARIOT_MAKER].total = buffer_read_i32(culture2);
    count_data.buildings[BUILDING_CHARIOT_MAKER].active = buffer_read_i32(culture2);

    // culture 3
    count_data.buildings[BUILDING_SMALL_TEMPLE_CERES].active = buffer_read_i32(culture3);
    count_data.buildings[BUILDING_SMALL_TEMPLE_NEPTUNE].active = buffer_read_i32(culture3);
    count_data.buildings[BUILDING_SMALL_TEMPLE_MERCURY].active = buffer_read_i32(culture3);
    count_data.buildings[BUILDING_SMALL_TEMPLE_MARS].active = buffer_read_i32(culture3);
    count_data.buildings[BUILDING_SMALL_TEMPLE_VENUS].active = buffer_read_i32(culture3);
    count_data.buildings[BUILDING_LARGE_TEMPLE_CERES].active = buffer_read_i32(culture3);
    count_data.buildings[BUILDING_LARGE_TEMPLE_NEPTUNE].active = buffer_read_i32(culture3);
    count_data.buildings[BUILDING_LARGE_TEMPLE_MERCURY].active = buffer_read_i32(culture3);
    count_data.buildings[BUILDING_LARGE_TEMPLE_MARS].active = buffer_read_i32(culture3);
    count_data.buildings[BUILDING_LARGE_TEMPLE_VENUS].active = buffer_read_i32(culture3);

    // military
    count_data.buildings[BUILDING_MILITARY_ACADEMY].total = buffer_read_i32(military);
    count_data.buildings[BUILDING_MILITARY_ACADEMY].active = buffer_read_i32(military);
    count_data.buildings[BUILDING_BARRACKS].total = buffer_read_i32(military);
    count_data.buildings[BUILDING_BARRACKS].active = buffer_read_i32(military);

    // support
    count_data.buildings[BUILDING_MARKET].total = buffer_read_i32(support);
    count_data.buildings[BUILDING_MARKET].active = buffer_read_i32(support);
    count_data.buildings[BUILDING_RESERVOIR].total = buffer_read_i32(support);
    count_data.buildings[BUILDING_RESERVOIR].active = buffer_read_i32(support);
    count_data.buildings[BUILDING_FOUNTAIN].total = buffer_read_i32(support);
    count_data.buildings[BUILDING_FOUNTAIN].active = buffer_read_i32(support);
}

void destroy_on_fire(struct building_t *b, int plagued)
{
    game_undo_disable();
    b->fire_risk = 0;
    b->damage_risk = 0;
    if (b->house_size && b->house_population) {
        city_population_remove_home_removed(b->house_population);
    }
    int was_tent = b->house_size && b->subtype.house_level <= HOUSE_LARGE_TENT;
    b->house_population = 0;
    b->house_size = 0;
    b->output_resource_id = 0;
    building_clear_related_data(b);

    int waterside_building = 0;
    if (b->type == BUILDING_DOCK || b->type == BUILDING_WHARF || b->type == BUILDING_SHIPYARD) {
        waterside_building = 1;
    }
    int num_tiles;
    if (b->size >= 2 && b->size <= 5) {
        num_tiles = b->size * b->size;
    } else {
        num_tiles = 0;
    }
    map_building_tiles_remove(b->id, b->x, b->y);
    if (map_terrain_is(b->grid_offset, TERRAIN_WATER)) {
        b->state = BUILDING_STATE_DELETED_BY_GAME;
    } else {
        b->type = BUILDING_BURNING_RUIN;
        b->figure_id4 = 0;
        b->tax_income_or_storage = 0;
        b->fire_duration = (b->house_figure_generation_delay & 7) + 1;
        b->fire_proof = 1;
        b->size = 1;
        b->ruin_has_plague = plagued;
        memset(&b->data, 0, 42);
        int image_id;
        if (was_tent) {
            image_id = image_group(GROUP_TERRAIN_RUBBLE_TENT);
        } else {
            int random = map_random_get(b->grid_offset) & 3;
            image_id = image_group(GROUP_TERRAIN_RUBBLE_GENERAL) + 9 * random;
        }
        map_building_tiles_add(b->id, b->x, b->y, 1, image_id, TERRAIN_BUILDING);
    }
    static const int x_tiles[] = {
        0, 1, 1, 0, 2, 2, 2, 1, 0, 3, 3, 3, 3, 2, 1, 0, 4, 4, 4, 4, 4, 3, 2, 1, 0, 5, 5, 5, 5, 5, 5, 4, 3, 2, 1, 0
    };
    static const int y_tiles[] = {
        0, 0, 1, 1, 0, 1, 2, 2, 2, 0, 1, 2, 3, 3, 3, 3, 0, 1, 2, 3, 4, 4, 4, 4, 4, 0, 1, 2, 3, 4, 5, 5, 5, 5, 5, 5
    };
    for (int tile = 1; tile < num_tiles; tile++) {
        int x = x_tiles[tile] + b->x;
        int y = y_tiles[tile] + b->y;
        if (map_terrain_is(map_grid_offset(x, y), TERRAIN_WATER)) {
            continue;
        }
        struct building_t *ruin = building_create(BUILDING_BURNING_RUIN, x, y);
        int image_id;
        if (was_tent) {
            image_id = image_group(GROUP_TERRAIN_RUBBLE_TENT);
        } else {
            int random = map_random_get(ruin->grid_offset) & 3;
            image_id = image_group(GROUP_TERRAIN_RUBBLE_GENERAL) + 9 * random;
        }
        map_building_tiles_add(ruin->id, ruin->x, ruin->y, 1, image_id, TERRAIN_BUILDING);
        ruin->fire_duration = (ruin->house_figure_generation_delay & 7) + 1;
        ruin->figure_id4 = 0;
        ruin->fire_proof = 1;
        ruin->ruin_has_plague = plagued;
    }
    if (waterside_building) {
        map_routing_update_water();
    }
}

static void destroy_linked_parts(struct building_t *b, int on_fire)
{
    struct building_t *part = b;
    for (int i = 0; i < 9; i++) {
        if (part->prev_part_building_id <= 0) {
            break;
        }
        int part_id = part->prev_part_building_id;
        part = &all_buildings[part_id];
        if (on_fire) {
            destroy_on_fire(part, 0);
        } else {
            map_building_tiles_set_rubble(part_id, part->x, part->y, part->size);
            part->state = BUILDING_STATE_RUBBLE;
        }
    }
    part = b;
    for (int i = 0; i < 9; i++) {
        part = &all_buildings[part->next_part_building_id];
        if (part->id <= 0) {
            break;
        }
        if (on_fire) {
            destroy_on_fire(part, 0);
        } else {
            map_building_tiles_set_rubble(part->id, part->x, part->y, part->size);
            part->state = BUILDING_STATE_RUBBLE;
        }
    }
    // Unlink the buildings to prevent corrupting the building table
    part = building_main(b);
    for (int i = 0; i < 9 && part->id > 0; i++) {
        struct building_t *next_part = &all_buildings[part->next_part_building_id];
        part->next_part_building_id = 0;
        part->prev_part_building_id = 0;
        part = next_part;
    }
}

void building_destroy_by_collapse(struct building_t *b)
{
    b->state = BUILDING_STATE_RUBBLE;
    map_building_tiles_set_rubble(b->id, b->x, b->y, b->size);
    figure_create_explosion_cloud(b->x, b->y, b->size);
    destroy_linked_parts(b, 0);
    play_sound_effect(SOUND_EFFECT_EXPLOSION);
}

void building_destroy_by_fire(struct building_t *b)
{
    destroy_on_fire(b, 0);
    destroy_linked_parts(b, 1);
}

void building_destroy_by_enemy(int x, int y, int grid_offset)
{
    int building_id = map_building_at(grid_offset);
    if (building_id > 0) {
        struct building_t *b = &all_buildings[building_id];
        if (b->state == BUILDING_STATE_IN_USE) {
            switch (b->type) {
                case BUILDING_HOUSE_SMALL_TENT:
                case BUILDING_HOUSE_LARGE_TENT:
                case BUILDING_PREFECTURE:
                case BUILDING_ENGINEERS_POST:
                case BUILDING_WELL:
                case BUILDING_GATEHOUSE:
                case BUILDING_TOWER:
                    break;
                default:
                    city_data.ratings.peace_destroyed_buildings++;
                    break;
            }
            if (city_data.ratings.peace_destroyed_buildings >= 12) {
                city_data.ratings.peace_destroyed_buildings = 12;
            }
            building_destroy_by_collapse(b);
        }
    } else {
        if (map_terrain_is(grid_offset, TERRAIN_WALL)) {
            figure_kill_tower_sentries_at(x, y);
        }
        map_building_tiles_set_rubble(0, x, y, 1);
    }
    figure_tower_sentry_reroute();
    foreach_region_tile(x - 1, y - 1, x + 3 - 2, y + 3 - 2, set_wall_image);
    foreach_region_tile(x - 3, y - 3, x + 3, y + 3, update_aqueduct_tile);
    map_routing_update_land();
    map_routing_update_walls();
}

static void check_labor_problem(struct building_t *b)
{
    if (b->houses_covered <= 0) {
        b->show_on_problem_overlay = 2;
    }
}

static void generate_labor_seeker(struct building_t *b, int x, int y)
{
    if (city_data.population.population <= 0) {
        return;
    }
    if (b->figure_id2) {
        struct figure_t *f = &figures[b->figure_id2];
        if (!figure_is_alive(f) || f->type != FIGURE_LABOR_SEEKER || f->building_id != b->id) {
            b->figure_id2 = 0;
        }
    } else {
        struct figure_t *f = figure_create(FIGURE_LABOR_SEEKER, x, y, DIR_0_TOP);
        f->action_state = FIGURE_ACTION_ROAMING;
        f->is_targetable = 1;
        f->terrain_usage = TERRAIN_USAGE_ROADS;
        f->building_id = b->id;
        b->figure_id2 = f->id;
        figure_movement_init_roaming(f);
    }
}

static void spawn_labor_seeker(struct building_t *b, int x, int y, int min_houses)
{
    if (b->houses_covered <= min_houses) {
        generate_labor_seeker(b, x, y);
    }
}

static int has_figure_of_types(struct building_t *b, int type1, int type2)
{
    if (b->figure_id <= 0) {
        return 0;
    }
    struct figure_t *f = &figures[b->figure_id];
    if (figure_is_alive(f) && f->building_id == b->id && (f->type == type1 || f->type == type2)) {
        return 1;
    } else {
        b->figure_id = 0;
        return 0;
    }
}

static int default_spawn_delay(struct building_t *b)
{
    int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
    if (pct_workers >= 100) {
        return 3;
    } else if (pct_workers >= 75) {
        return 7;
    } else if (pct_workers >= 50) {
        return 15;
    } else if (pct_workers >= 25) {
        return 29;
    } else if (pct_workers >= 1) {
        return 44;
    } else {
        return 0;
    }
}

static void create_roaming_figure(struct building_t *b, int x, int y, int type)
{
    struct figure_t *f = figure_create(type, x, y, DIR_0_TOP);
    f->action_state = FIGURE_ACTION_ROAMING;
    f->is_targetable = 1;
    f->terrain_usage = TERRAIN_USAGE_ROADS;
    f->building_id = b->id;
    b->figure_id = f->id;
    figure_movement_init_roaming(f);
}

static int contains_non_stockpiled_food(struct building_t *space, const int *resources)
{
    if (space->id <= 0) {
        return 0;
    }
    if (space->loads_stored <= 0) {
        return 0;
    }
    int resource = space->subtype.warehouse_resource_id;
    if (city_data.resource.stockpiled[resource]) {
        return 0;
    }
    if (resource == RESOURCE_WHEAT || resource == RESOURCE_VEGETABLES ||
        resource == RESOURCE_FRUIT || resource == RESOURCE_MEAT) {
        if (resources[resource] > 0) {
            return 1;
        }
    }
    return 0;
}

static void update_food_resource(struct resource_data *data, int resource, const struct building_t *b, int distance)
{
    if (b->data.granary.resource_stored[resource]) {
        data->num_buildings++;
        if (distance < data->distance) {
            data->distance = distance;
            data->building_id = b->id;
        }
    }
}

static void update_good_resource(struct resource_data *data, int resource, struct building_t *b, int distance)
{
    if (!city_data.resource.stockpiled[resource] && building_warehouse_get_amount(b, resource) > 0) {
        data->num_buildings++;
        if (distance < data->distance) {
            data->distance = distance;
            data->building_id = b->id;
        }
    }
}

void building_figure_generate(void)
{
    if (tower_sentry_request > 0) {
        tower_sentry_request--;
    }
    int max_id = building_get_highest_id();
    for (int i = 1; i <= max_id; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        if (b->type == BUILDING_WAREHOUSE_SPACE || (b->type == BUILDING_HIPPODROME && b->prev_part_building_id)) {
            continue;
        }
        b->show_on_problem_overlay = 0;
        // range of building types
        if (b->type >= BUILDING_HOUSE_SMALL_VILLA && b->type <= BUILDING_HOUSE_LUXURY_PALACE) {
            struct map_point_t road;
            if (map_has_road_access(b->x, b->y, b->size, &road)) {
                b->figure_spawn_delay++;
                if (b->figure_spawn_delay > 40) {
                    b->figure_spawn_delay = 0;
                    struct figure_t *f = figure_create(FIGURE_PATRICIAN, road.x, road.y, DIR_4_BOTTOM);
                    f->action_state = FIGURE_ACTION_ROAMING;
                    f->is_targetable = 1;
                    f->terrain_usage = TERRAIN_USAGE_ROADS;
                    f->building_id = b->id;
                    figure_movement_init_roaming(f);
                }
            }
        } else if (b->type >= BUILDING_WHEAT_FARM && b->type <= BUILDING_WEAPONS_WORKSHOP) {
            check_labor_problem(b);
            struct map_point_t road;
            if (map_has_road_access(b->x, b->y, b->size, &road)) {
                spawn_labor_seeker(b, road.x, road.y, 50);
                if (!has_figure_of_types(b, FIGURE_CART_PUSHER, 0)) {
                    if (b->data.industry.progress >= (b->subtype.workshop_type ? MAX_PROGRESS_WORKSHOP : MAX_PROGRESS_RAW)) {
                        b->data.industry.progress = 0;
                        if (b->subtype.workshop_type) {
                            if (b->loads_stored) {
                                if (b->loads_stored > 1) {
                                    b->data.industry.has_raw_materials = 1;
                                }
                                b->loads_stored--;
                            }
                        }
                        if (building_is_farm(b->type)) {
                            update_farm_image(b);
                        }
                        struct figure_t *f = figure_create(FIGURE_CART_PUSHER, road.x, road.y, DIR_4_BOTTOM);
                        f->action_state = FIGURE_ACTION_CARTPUSHER_INITIAL;
                        f->is_targetable = 1;
                        f->terrain_usage = TERRAIN_USAGE_ROADS;
                        f->resource_id = b->output_resource_id;
                        f->building_id = b->id;
                        b->figure_id = f->id;
                        f->wait_ticks = 30;
                    }
                }
            }
        } else if (b->type == BUILDING_SENATE || b->type == BUILDING_FORUM) {
            if (b->type == BUILDING_SENATE && b->state == BUILDING_STATE_IN_USE) {
                if (map_desirability_get(b->grid_offset) <= 30) {
                    map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_SENATE), TERRAIN_BUILDING);
                } else {
                    map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_SENATE_FANCY), TERRAIN_BUILDING);
                }
            }
            check_labor_problem(b);
            if (!has_figure_of_types(b, FIGURE_TAX_COLLECTOR, 0)) {
                struct map_point_t road;
                if (map_has_road_access(b->x, b->y, b->size, &road)) {
                    spawn_labor_seeker(b, road.x, road.y, 50);
                    int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                    int spawn_delay;
                    if (pct_workers >= 100) {
                        spawn_delay = 0;
                    } else if (pct_workers >= 75) {
                        spawn_delay = 1;
                    } else if (pct_workers >= 50) {
                        spawn_delay = 3;
                    } else if (pct_workers >= 25) {
                        spawn_delay = 7;
                    } else if (pct_workers >= 1) {
                        spawn_delay = 15;
                    } else {
                        spawn_delay = INFINITE;
                    }
                    b->figure_spawn_delay++;
                    if (b->figure_spawn_delay > spawn_delay) {
                        b->figure_spawn_delay = 0;
                        struct figure_t *f = figure_create(FIGURE_TAX_COLLECTOR, road.x, road.y, DIR_0_TOP);
                        f->action_state = FIGURE_ACTION_TAX_COLLECTOR_CREATED;
                        f->is_targetable = 1;
                        f->terrain_usage = TERRAIN_USAGE_ROADS;
                        f->building_id = b->id;
                        b->figure_id = f->id;
                    }
                }
            }
        } else if (b->type >= BUILDING_SMALL_TEMPLE_CERES && b->type <= BUILDING_LARGE_TEMPLE_VENUS) {
            check_labor_problem(b);
            if (!has_figure_of_types(b, FIGURE_PRIEST, 0)) {
                struct map_point_t road;
                if (map_has_road_access(b->x, b->y, b->size, &road)) {
                    spawn_labor_seeker(b, road.x, road.y, 50);
                    int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                    int spawn_delay;
                    if (building_properties[b->type].n_laborers <= 0) {
                        spawn_delay = 7;
                    } else if (pct_workers >= 100) {
                        spawn_delay = 3;
                    } else if (pct_workers >= 75) {
                        spawn_delay = 7;
                    } else if (pct_workers >= 50) {
                        spawn_delay = 10;
                    } else if (pct_workers >= 25) {
                        spawn_delay = 15;
                    } else if (pct_workers >= 1) {
                        spawn_delay = 20;
                    } else {
                        spawn_delay = INFINITE;
                    }
                    b->figure_spawn_delay++;
                    if (b->figure_spawn_delay > spawn_delay) {
                        b->figure_spawn_delay = 0;
                        create_roaming_figure(b, road.x, road.y, FIGURE_PRIEST);
                    }
                }
            }
        } else {
            int x_out, y_out;
            struct map_point_t road;
            // single building type
            switch (b->type) {
                case BUILDING_WAREHOUSE:
                    check_labor_problem(b);
                    struct building_t *space = b;
                    for (int ii = 0; ii < 8; ii++) {
                        space = &all_buildings[space->next_part_building_id];
                        if (space->id) {
                            space->show_on_problem_overlay = b->show_on_problem_overlay;
                        }
                    }
                    if (map_has_road_access(b->x, b->y, b->size, &road) ||
                        map_has_road_access(b->x, b->y, 3, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 100);
                        if (!has_figure_of_types(b, FIGURE_WAREHOUSEMAN, 0)) {
                            int resource;
                            int task = WAREHOUSE_TASK_NONE;
                            if (calc_percentage(b->num_workers, building_properties[b->type].n_laborers) < 50) {
                                task = WAREHOUSE_TASK_NONE;
                            } else {
                                struct building_storage_t *s = building_storage_get(b->storage_id);
                                // get resources
                                for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                                    if (s->resource_state[r] != BUILDING_STORAGE_STATE_GETTING || city_data.resource.stockpiled[r]) {
                                        continue;
                                    }
                                    int loads_stored = 0;
                                    space = b;
                                    for (int ii = 0; ii < 8; ii++) {
                                        space = &all_buildings[space->next_part_building_id];
                                        if (space->id > 0 && space->loads_stored > 0) {
                                            if (space->subtype.warehouse_resource_id == r) {
                                                loads_stored += space->loads_stored;
                                            }
                                        }
                                    }
                                    int room = 0;
                                    space = b;
                                    for (int ii = 0; ii < 8; ii++) {
                                        space = &all_buildings[space->next_part_building_id];
                                        if (space->id > 0) {
                                            if (space->loads_stored <= 0) {
                                                room += 4;
                                            }
                                            if (space->subtype.warehouse_resource_id == r) {
                                                room += 4 - space->loads_stored;
                                            }
                                        }
                                    }
                                    if (room >= 8 && loads_stored <= 4 && city_data.resource.stored_in_warehouses[r] - loads_stored > 4) {
                                        resource = r;
                                        task = WAREHOUSE_TASK_GETTING;
                                    }
                                }
                                if (task == WAREHOUSE_TASK_NONE) {
                                    // deliver weapons to barracks
                                    if (building_count_active(BUILDING_BARRACKS) > 0 && city_data.military.legionary_legions && !city_data.resource.stockpiled[RESOURCE_WEAPONS]) {
                                        struct building_t *barracks = &all_buildings[city_data.building.barracks_building_id];
                                        if (barracks->loads_stored < 4 && b->road_network_id == barracks->road_network_id) {
                                            space = b;
                                            for (int ii = 0; ii < 8; ii++) {
                                                space = &all_buildings[space->next_part_building_id];
                                                if (space->id > 0 && space->loads_stored > 0 && space->subtype.warehouse_resource_id == RESOURCE_WEAPONS) {
                                                    resource = RESOURCE_WEAPONS;
                                                    task = WAREHOUSE_TASK_DELIVERING;
                                                }
                                            }
                                        }
                                    }
                                }
                                if (task == WAREHOUSE_TASK_NONE) {
                                    // deliver raw materials to workshops
                                    space = b;
                                    for (int ii = 0; ii < 8; ii++) {
                                        space = &all_buildings[space->next_part_building_id];
                                        if (space->id > 0 && space->loads_stored > 0) {
                                            if (!city_data.resource.stockpiled[space->subtype.warehouse_resource_id]) {
                                                int workshop_type = resource_to_workshop_type(space->subtype.warehouse_resource_id);
                                                if (workshop_type != WORKSHOP_NONE && city_data.resource.space_in_workshops[workshop_type]) {
                                                    resource = space->subtype.warehouse_resource_id;
                                                    task = WAREHOUSE_TASK_DELIVERING;
                                                }
                                            }
                                        }
                                    }
                                }
                                if (!scenario.rome_supplies_wheat && task == WAREHOUSE_TASK_NONE) {
                                    // deliver food to getting granary
                                    int granary_resources[FOOD_TYPES_MAX];
                                    int can_deliver_to_granary = 0;
                                    for (int ii = 0; ii < FOOD_TYPES_MAX; ii++) {
                                        granary_resources[ii] = 0;
                                    }
                                    for (int ii = 1; ii < MAX_BUILDINGS; ii++) {
                                        struct building_t *bb = &all_buildings[ii];
                                        if (bb->state != BUILDING_STATE_IN_USE || bb->type != BUILDING_GRANARY || !bb->has_road_access) {
                                            continue;
                                        }
                                        if (calc_percentage(bb->num_workers, building_properties[bb->type].n_laborers) >= 100 && bb->data.granary.resource_stored[RESOURCE_NONE] > 100) {
                                            struct building_storage_t *st = building_storage_get(b->storage_id);
                                            if (!st->empty_all) {
                                                for (int r = 0; r < FOOD_TYPES_MAX; r++) {
                                                    if (st->resource_state[r] == BUILDING_STORAGE_STATE_GETTING) {
                                                        granary_resources[r]++;
                                                        can_deliver_to_granary = 1;
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    if (can_deliver_to_granary) {
                                        space = b;
                                        for (int ii = 0; ii < 8; ii++) {
                                            space = &all_buildings[space->next_part_building_id];
                                            if (contains_non_stockpiled_food(space, granary_resources)) {
                                                resource = space->subtype.warehouse_resource_id;
                                                task = WAREHOUSE_TASK_DELIVERING;
                                            }
                                        }
                                    }
                                    // deliver food to accepting granary
                                    if (task == WAREHOUSE_TASK_NONE) {
                                        int granary_can_accept = 0;
                                        for (int ii = 0; ii < FOOD_TYPES_MAX; ii++) {
                                            granary_resources[ii] = 0;
                                        }
                                        for (int ii = 1; ii < MAX_BUILDINGS; ii++) {
                                            struct building_t *bb = &all_buildings[ii];
                                            if (bb->state != BUILDING_STATE_IN_USE || bb->type != BUILDING_GRANARY || !bb->has_road_access) {
                                                continue;
                                            }
                                            if (calc_percentage(bb->num_workers, building_properties[bb->type].n_laborers) >= 100 && bb->data.granary.resource_stored[RESOURCE_NONE] >= 1200) {
                                                struct building_storage_t *st = building_storage_get(bb->storage_id);
                                                if (!st->empty_all) {
                                                    for (int r = 0; r < FOOD_TYPES_MAX; r++) {
                                                        if (st->resource_state[r] != BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
                                                            granary_resources[r]++;
                                                            granary_can_accept = 1;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                        if (granary_can_accept) {
                                            space = b;
                                            for (int ii = 0; ii < 8; ii++) {
                                                space = &all_buildings[space->next_part_building_id];
                                                if (contains_non_stockpiled_food(space, granary_resources)) {
                                                    resource = space->subtype.warehouse_resource_id;
                                                    task = WAREHOUSE_TASK_DELIVERING;
                                                }
                                            }
                                        }
                                    }
                                }
                                // move goods to other warehouses
                                if (s->empty_all) {
                                    space = b;
                                    for (int ii = 0; ii < 8; ii++) {
                                        space = &all_buildings[space->next_part_building_id];
                                        if (space->id > 0 && space->loads_stored > 0) {
                                            resource = space->subtype.warehouse_resource_id;
                                            task = WAREHOUSE_TASK_DELIVERING;
                                        }
                                    }
                                }
                            }
                            if (task != WAREHOUSE_TASK_NONE) {
                                struct figure_t *f = figure_create(FIGURE_WAREHOUSEMAN, road.x, road.y, DIR_4_BOTTOM);
                                f->is_targetable = 1;
                                f->action_state = FIGURE_ACTION_WAREHOUSEMAN_CREATED;
                                if (task == WAREHOUSE_TASK_GETTING) {
                                    f->resource_id = RESOURCE_NONE;
                                    f->collecting_item_id = resource;
                                } else {
                                    f->resource_id = resource;
                                }
                                b->figure_id = f->id;
                                f->building_id = b->id;
                            }
                        }
                    }
                    break;
                case BUILDING_GRANARY:
                    check_labor_problem(b);
                    if (map_has_road_access_granary(b->x, b->y, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 100);
                        if (!has_figure_of_types(b, FIGURE_WAREHOUSEMAN, 0)) {
                            struct building_storage_t *s = building_storage_get(b->storage_id);
                            int task = GRANARY_TASK_NONE;
                            if (calc_percentage(b->num_workers, building_properties[b->type].n_laborers) < 50
                            || b->data.granary.resource_stored[RESOURCE_NONE] <= 0) { // granary full, nothing to get
                                task = GRANARY_TASK_NONE;
                            } else if (s->empty_all) {
                                // bring food to another granary
                                for (int ii = RESOURCE_WHEAT; ii < FOOD_TYPES_MAX; ii++) {
                                    if (b->data.granary.resource_stored[i]) {
                                        task = GRANARY_TASK_GETTING;
                                        break;
                                    }
                                }
                            } else if ((s->resource_state[RESOURCE_WHEAT] == BUILDING_STORAGE_STATE_GETTING && non_getting_granaries.total_storage_wheat > ONE_LOAD)
                            || (s->resource_state[RESOURCE_VEGETABLES] == BUILDING_STORAGE_STATE_GETTING && non_getting_granaries.total_storage_vegetables > ONE_LOAD)
                            || (s->resource_state[RESOURCE_FRUIT] == BUILDING_STORAGE_STATE_GETTING && non_getting_granaries.total_storage_fruit > ONE_LOAD)
                            || (s->resource_state[RESOURCE_MEAT] == BUILDING_STORAGE_STATE_GETTING && non_getting_granaries.total_storage_meat > ONE_LOAD)) {
                                task = GRANARY_TASK_GETTING;
                            }
                            if (task != GRANARY_TASK_NONE) {
                                struct figure_t *f = figure_create(FIGURE_WAREHOUSEMAN, road.x, road.y, DIR_4_BOTTOM);
                                f->is_targetable = 1;
                                f->action_state = FIGURE_ACTION_WAREHOUSEMAN_CREATED;
                                f->resource_id = task;
                                b->figure_id = f->id;
                                f->building_id = b->id;
                            }
                        }
                    }
                    break;
                case BUILDING_TOWER:
                    check_labor_problem(b);
                    if (map_has_road_access(b->x, b->y, b->size, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 50);
                        if (b->num_workers) {
                            if (!b->figure_id4 && b->figure_id) { // has sentry but no ballista -> create
                                struct figure_t *f = figure_create(FIGURE_BALLISTA, b->x, b->y, DIR_0_TOP);
                                b->figure_id4 = f->id;
                                f->building_id = b->id;
                                f->action_state = FIGURE_ACTION_BALLISTA_READY;
                                f->terrain_usage = TERRAIN_USAGE_WALLS;
                            }
                            has_figure_of_types(b, FIGURE_TOWER_SENTRY, 0);
                            if (b->figure_id <= 0) {
                                tower_sentry_request = 2;
                            }
                        }
                    }
                    break;
                case BUILDING_ENGINEERS_POST:
                    check_labor_problem(b);
                    if (!has_figure_of_types(b, FIGURE_ENGINEER, 0)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            spawn_labor_seeker(b, road.x, road.y, 100);
                            int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                            int spawn_delay;
                            if (pct_workers >= 100) {
                                spawn_delay = 0;
                            } else if (pct_workers >= 75) {
                                spawn_delay = 1;
                            } else if (pct_workers >= 50) {
                                spawn_delay = 3;
                            } else if (pct_workers >= 25) {
                                spawn_delay = 7;
                            } else if (pct_workers >= 1) {
                                spawn_delay = 15;
                            } else {
                                spawn_delay = INFINITE;
                            }
                            b->figure_spawn_delay++;
                            if (b->figure_spawn_delay > spawn_delay) {
                                b->figure_spawn_delay = 0;
                                struct figure_t *f = figure_create(FIGURE_ENGINEER, road.x, road.y, DIR_0_TOP);
                                f->action_state = FIGURE_ACTION_ENGINEER_CREATED;
                                f->is_targetable = 1;
                                f->terrain_usage = TERRAIN_USAGE_ROADS;
                                f->building_id = b->id;
                                b->figure_id = f->id;
                            }
                        }
                    }
                    break;
                case BUILDING_PREFECTURE:
                    check_labor_problem(b);
                    if (!has_figure_of_types(b, FIGURE_PREFECT, 0)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            spawn_labor_seeker(b, road.x, road.y, 100);
                            int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                            int spawn_delay;
                            if (pct_workers >= 100) {
                                spawn_delay = 0;
                            } else if (pct_workers >= 75) {
                                spawn_delay = 1;
                            } else if (pct_workers >= 50) {
                                spawn_delay = 3;
                            } else if (pct_workers >= 25) {
                                spawn_delay = 7;
                            } else if (pct_workers >= 1) {
                                spawn_delay = 15;
                            } else {
                                spawn_delay = INFINITE;;
                            }
                            b->figure_spawn_delay++;
                            if (b->figure_spawn_delay > spawn_delay) {
                                b->figure_spawn_delay = 0;
                                struct figure_t *f = figure_create(FIGURE_PREFECT, road.x, road.y, DIR_0_TOP);
                                f->action_state = FIGURE_ACTION_PREFECT_CREATED;
                                f->is_targetable = 1;
                                f->building_id = b->id;
                                b->figure_id = f->id;
                            }
                        }
                    }
                    break;
                case BUILDING_ACTOR_COLONY:
                    check_labor_problem(b);
                    if (map_has_road_access(b->x, b->y, b->size, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 50);
                        int spawn_delay = default_spawn_delay(b);
                        if (spawn_delay) {
                            b->figure_spawn_delay++;
                            if (b->figure_spawn_delay > spawn_delay) {
                                b->figure_spawn_delay = 0;
                                struct figure_t *f = figure_create(FIGURE_ACTOR, road.x, road.y, DIR_0_TOP);
                                f->action_state = FIGURE_ACTION_ENTERTAINER_AT_SCHOOL_CREATED;
                                f->is_targetable = 1;
                                f->terrain_usage = TERRAIN_USAGE_ROADS;
                                f->building_id = b->id;
                                b->figure_id = f->id;
                            }
                        }
                    }
                    break;
                case BUILDING_GLADIATOR_SCHOOL:
                    check_labor_problem(b);
                    if (map_has_road_access(b->x, b->y, b->size, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 50);
                        int spawn_delay = default_spawn_delay(b);
                        if (spawn_delay) {
                            b->figure_spawn_delay++;
                            if (b->figure_spawn_delay > spawn_delay) {
                                b->figure_spawn_delay = 0;
                                if (scenario.gladiator_revolt.state != EVENT_IN_PROGRESS) {
                                    struct figure_t *f = figure_create(FIGURE_GLADIATOR, road.x, road.y, DIR_0_TOP);
                                    f->action_state = FIGURE_ACTION_ENTERTAINER_AT_SCHOOL_CREATED;
                                    f->is_targetable = 1;
                                    f->terrain_usage = TERRAIN_USAGE_ROADS;
                                    f->building_id = b->id;
                                    b->figure_id = f->id;
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_LION_HOUSE:
                    check_labor_problem(b);
                    if (map_has_road_access(b->x, b->y, b->size, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 50);
                        int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                        int spawn_delay;
                        if (pct_workers >= 100) {
                            spawn_delay = 5;
                        } else if (pct_workers >= 75) {
                            spawn_delay = 10;
                        } else if (pct_workers >= 50) {
                            spawn_delay = 20;
                        } else if (pct_workers >= 25) {
                            spawn_delay = 35;
                        } else if (pct_workers >= 1) {
                            spawn_delay = 60;
                        } else {
                            spawn_delay = INFINITE;;
                        }
                        b->figure_spawn_delay++;
                        if (b->figure_spawn_delay > spawn_delay) {
                            b->figure_spawn_delay = 0;
                            struct figure_t *f = figure_create(FIGURE_LION_TAMER, road.x, road.y, DIR_0_TOP);
                            f->action_state = FIGURE_ACTION_ENTERTAINER_AT_SCHOOL_CREATED;
                            f->is_targetable = 1;
                            f->terrain_usage = TERRAIN_USAGE_ROADS;
                            f->building_id = b->id;
                            b->figure_id = f->id;
                        }
                    }
                    break;
                case BUILDING_CHARIOT_MAKER:
                    check_labor_problem(b);
                    if (map_has_road_access(b->x, b->y, b->size, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 50);
                        int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                        int spawn_delay;
                        if (pct_workers >= 100) {
                            spawn_delay = 7;
                        } else if (pct_workers >= 75) {
                            spawn_delay = 15;
                        } else if (pct_workers >= 50) {
                            spawn_delay = 30;
                        } else if (pct_workers >= 25) {
                            spawn_delay = 60;
                        } else if (pct_workers >= 1) {
                            spawn_delay = 90;
                        } else {
                            spawn_delay = INFINITE;;
                        }
                        b->figure_spawn_delay++;
                        if (b->figure_spawn_delay > spawn_delay) {
                            b->figure_spawn_delay = 0;
                            struct figure_t *f = figure_create(FIGURE_CHARIOTEER, road.x, road.y, DIR_0_TOP);
                            f->action_state = FIGURE_ACTION_ENTERTAINER_AT_SCHOOL_CREATED;
                            f->is_targetable = 1;
                            f->terrain_usage = TERRAIN_USAGE_ROADS;
                            f->building_id = b->id;
                            b->figure_id = f->id;
                        }
                    }
                    break;
                case BUILDING_AMPHITHEATER:
                    check_labor_problem(b);
                    if (!has_figure_of_types(b, FIGURE_ACTOR, FIGURE_GLADIATOR)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            if (b->houses_covered <= 50 ||
                                (b->data.entertainment.days1 <= 0 && b->data.entertainment.days2 <= 0)) {
                                generate_labor_seeker(b, road.x, road.y);
                            }
                            int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                            int spawn_delay;
                            if (pct_workers >= 100) {
                                spawn_delay = 3;
                            } else if (pct_workers >= 75) {
                                spawn_delay = 7;
                            } else if (pct_workers >= 50) {
                                spawn_delay = 15;
                            } else if (pct_workers >= 25) {
                                spawn_delay = 29;
                            } else if (pct_workers >= 1) {
                                spawn_delay = 44;
                            } else {
                                spawn_delay = INFINITE;;
                            }
                            b->figure_spawn_delay++;
                            if (b->figure_spawn_delay > spawn_delay) {
                                b->figure_spawn_delay = 0;
                                if (b->data.entertainment.days1 > 0) {
                                    if (scenario.gladiator_revolt.state != EVENT_IN_PROGRESS) {
                                        struct figure_t *f = figure_create(FIGURE_GLADIATOR, road.x, road.y, DIR_0_TOP);
                                        f->action_state = FIGURE_ACTION_ENTERTAINER_ROAMING;
                                        f->is_targetable = 1;
                                        f->terrain_usage = TERRAIN_USAGE_ROADS;
                                        f->building_id = b->id;
                                        b->figure_id = f->id;
                                        figure_movement_init_roaming(f);
                                    }
                                } else {
                                    struct figure_t *f = figure_create(FIGURE_ACTOR, road.x, road.y, DIR_0_TOP);
                                    f->action_state = FIGURE_ACTION_ENTERTAINER_ROAMING;
                                    f->is_targetable = 1;
                                    f->terrain_usage = TERRAIN_USAGE_ROADS;
                                    f->building_id = b->id;
                                    b->figure_id = f->id;
                                    figure_movement_init_roaming(f);
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_THEATER:
                    check_labor_problem(b);
                    if (!has_figure_of_types(b, FIGURE_ACTOR, 0)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            if (b->houses_covered <= 50 || b->data.entertainment.days1 <= 0) {
                                generate_labor_seeker(b, road.x, road.y);
                            }
                            int spawn_delay = default_spawn_delay(b);
                            if (spawn_delay) {
                                b->figure_spawn_delay++;
                                if (b->figure_spawn_delay > spawn_delay) {
                                    b->figure_spawn_delay = 0;
                                    struct figure_t *f = figure_create(FIGURE_ACTOR, road.x, road.y, DIR_0_TOP);
                                    f->action_state = FIGURE_ACTION_ENTERTAINER_ROAMING;
                                    f->is_targetable = 1;
                                    f->terrain_usage = TERRAIN_USAGE_ROADS;
                                    f->building_id = b->id;
                                    b->figure_id = f->id;
                                    figure_movement_init_roaming(f);
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_HIPPODROME:
                    check_labor_problem(b);
                    if (!b->prev_part_building_id) {
                        struct building_t *part = b;
                        for (int ii = 0; ii < 2; ii++) {
                            part = &all_buildings[part->next_part_building_id];
                            if (part->id) {
                                part->show_on_problem_overlay = b->show_on_problem_overlay;
                            }
                        }
                        if (!has_figure_of_types(b, FIGURE_CHARIOTEER, 0)) {
                            if (map_has_road_access_hippodrome(b->x, b->y, &road)) {
                                if (b->houses_covered <= 50 || b->data.entertainment.days1 <= 0) {
                                    generate_labor_seeker(b, road.x, road.y);
                                }
                                int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                                int spawn_delay;
                                if (pct_workers >= 100) {
                                    spawn_delay = 7;
                                } else if (pct_workers >= 75) {
                                    spawn_delay = 15;
                                } else if (pct_workers >= 50) {
                                    spawn_delay = 30;
                                } else if (pct_workers >= 25) {
                                    spawn_delay = 50;
                                } else if (pct_workers >= 1) {
                                    spawn_delay = 80;
                                } else {
                                    spawn_delay = INFINITE;;
                                }
                                b->figure_spawn_delay++;
                                if (b->figure_spawn_delay > spawn_delay) {
                                    b->figure_spawn_delay = 0;
                                    struct figure_t *f = figure_create(FIGURE_CHARIOTEER, road.x, road.y, DIR_0_TOP);
                                    f->action_state = FIGURE_ACTION_ENTERTAINER_ROAMING;
                                    f->is_targetable = 1;
                                    f->terrain_usage = TERRAIN_USAGE_ROADS;
                                    f->building_id = b->id;
                                    b->figure_id = f->id;
                                    figure_movement_init_roaming(f);

                                    if (!city_data.entertainment.hippodrome_has_race) {
                                        // create mini-horses
                                        struct figure_t *horse1 = figure_create(FIGURE_HIPPODROME_HORSES, b->x + 2, b->y + 1, DIR_2_RIGHT);
                                        horse1->action_state = FIGURE_ACTION_HIPPODROME_HORSE_CREATED;
                                        horse1->use_cross_country = 1;
                                        horse1->building_id = b->id;
                                        horse1->resource_id = 0;
                                        horse1->speed_multiplier = 3;

                                        struct figure_t *horse2 = figure_create(FIGURE_HIPPODROME_HORSES, b->x + 2, b->y + 2, DIR_2_RIGHT);
                                        horse2->action_state = FIGURE_ACTION_HIPPODROME_HORSE_CREATED;
                                        horse2->use_cross_country = 1;
                                        horse2->building_id = b->id;
                                        horse2->resource_id = 1;
                                        horse2->speed_multiplier = 2;

                                        if (b->data.entertainment.days1 > 0) {
                                            if (!city_data.entertainment.hippodrome_message_shown) {
                                                city_data.entertainment.hippodrome_message_shown = 1;
                                                city_message_post(1, MESSAGE_WORKING_HIPPODROME, 0, 0);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_COLOSSEUM:
                    check_labor_problem(b);
                    if (!has_figure_of_types(b, FIGURE_GLADIATOR, FIGURE_LION_TAMER)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            if (b->houses_covered <= 50 ||
                                (b->data.entertainment.days1 <= 0 && b->data.entertainment.days2 <= 0)) {
                                generate_labor_seeker(b, road.x, road.y);
                            }
                            int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                            int spawn_delay;
                            if (pct_workers >= 100) {
                                spawn_delay = 6;
                            } else if (pct_workers >= 75) {
                                spawn_delay = 12;
                            } else if (pct_workers >= 50) {
                                spawn_delay = 20;
                            } else if (pct_workers >= 25) {
                                spawn_delay = 40;
                            } else if (pct_workers >= 1) {
                                spawn_delay = 70;
                            } else {
                                spawn_delay = INFINITE;;
                            }
                            b->figure_spawn_delay++;
                            if (b->figure_spawn_delay > spawn_delay) {
                                b->figure_spawn_delay = 0;
                                if (b->data.entertainment.days1 > 0) {
                                    if (scenario.gladiator_revolt.state != EVENT_IN_PROGRESS) {
                                        struct figure_t *f = figure_create(FIGURE_LION_TAMER, road.x, road.y, DIR_0_TOP);
                                        f->action_state = FIGURE_ACTION_ENTERTAINER_ROAMING;
                                        f->is_targetable = 1;
                                        f->terrain_usage = TERRAIN_USAGE_ROADS;
                                        f->building_id = b->id;
                                        b->figure_id = f->id;
                                        figure_movement_init_roaming(f);
                                    }
                                } else {
                                    struct figure_t *f = figure_create(FIGURE_GLADIATOR, road.x, road.y, DIR_0_TOP);
                                    f->action_state = FIGURE_ACTION_ENTERTAINER_ROAMING;
                                    f->is_targetable = 1;
                                    f->terrain_usage = TERRAIN_USAGE_ROADS;
                                    f->building_id = b->id;
                                    b->figure_id = f->id;
                                    figure_movement_init_roaming(f);
                                }

                                if (b->data.entertainment.days1 > 0 || b->data.entertainment.days2 > 0) {
                                    if (!city_data.entertainment.colosseum_message_shown) {
                                        city_data.entertainment.colosseum_message_shown = 1;
                                        city_message_post(1, MESSAGE_WORKING_COLOSSEUM, 0, 0);
                                    }
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_MARKET:
                    if (b->state == BUILDING_STATE_IN_USE) {
                        if (map_desirability_get(b->grid_offset) <= 30) {
                            map_building_tiles_add(b->id, b->x, b->y, b->size,
                                image_group(GROUP_BUILDING_MARKET), TERRAIN_BUILDING);
                        } else {
                            map_building_tiles_add(b->id, b->x, b->y, b->size,
                                image_group(GROUP_BUILDING_MARKET_FANCY), TERRAIN_BUILDING);
                        }
                        check_labor_problem(b);
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            spawn_labor_seeker(b, road.x, road.y, 50);
                            int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                            int spawn_delay;
                            if (pct_workers >= 100) {
                                spawn_delay = 2;
                            } else if (pct_workers >= 75) {
                                spawn_delay = 5;
                            } else if (pct_workers >= 50) {
                                spawn_delay = 10;
                            } else if (pct_workers >= 25) {
                                spawn_delay = 20;
                            } else if (pct_workers >= 1) {
                                spawn_delay = 30;
                            } else {
                                spawn_delay = INFINITE;;
                            }
                            // market trader
                            if (!has_figure_of_types(b, FIGURE_MARKET_TRADER, 0)) {
                                b->figure_spawn_delay++;
                                if (b->figure_spawn_delay > spawn_delay) {
                                    b->figure_spawn_delay = 0;
                                    create_roaming_figure(b, road.x, road.y, FIGURE_MARKET_TRADER);
                                }
                            }
                            // market buyer or labor seeker
                            if (b->figure_id2) {
                                struct figure_t *f = &figures[b->figure_id2];
                                if (!figure_is_alive(f) || (f->type != FIGURE_MARKET_BUYER && f->type != FIGURE_LABOR_SEEKER)) {
                                    b->figure_id2 = 0;
                                }
                            } else {
                                map_has_road_access(b->x, b->y, b->size, &road);
                                struct resource_data resources[INVENTORY_MAX];
                                for (int ii = 0; ii < INVENTORY_MAX; ii++) {
                                    resources[ii].building_id = 0;
                                    resources[ii].num_buildings = 0;
                                    resources[ii].distance = 40;
                                }
                                for (int ii = 1; ii < MAX_BUILDINGS; ii++) {
                                    struct building_t *bb = &all_buildings[ii];
                                    if (bb->state != BUILDING_STATE_IN_USE) {
                                        continue;
                                    }
                                    if (bb->type != BUILDING_GRANARY && bb->type != BUILDING_WAREHOUSE) {
                                        continue;
                                    }
                                    if (!bb->has_road_access || bb->road_network_id != b->road_network_id) {
                                        continue;
                                    }
                                    int distance = calc_maximum_distance(b->x, b->y, bb->x, bb->y);
                                    if (distance >= 40) {
                                        continue;
                                    }
                                    if (bb->type == BUILDING_GRANARY) {
                                        if (scenario.rome_supplies_wheat) {
                                            continue;
                                        }
                                        update_food_resource(&resources[INVENTORY_WHEAT], RESOURCE_WHEAT, bb, distance);
                                        update_food_resource(&resources[INVENTORY_VEGETABLES], RESOURCE_VEGETABLES, bb, distance);
                                        update_food_resource(&resources[INVENTORY_FRUIT], RESOURCE_FRUIT, bb, distance);
                                        update_food_resource(&resources[INVENTORY_MEAT], RESOURCE_MEAT, bb, distance);
                                    } else if (bb->type == BUILDING_WAREHOUSE) {
                                        // goods
                                        update_good_resource(&resources[INVENTORY_WINE], RESOURCE_WINE, bb, distance);
                                        update_good_resource(&resources[INVENTORY_OIL], RESOURCE_OIL, bb, distance);
                                        update_good_resource(&resources[INVENTORY_POTTERY], RESOURCE_POTTERY, bb, distance);
                                        update_good_resource(&resources[INVENTORY_FURNITURE], RESOURCE_FURNITURE, bb, distance);
                                    }
                                }
                                // update demands
                                if (b->data.market.pottery_demand) {
                                    b->data.market.pottery_demand--;
                                } else {
                                    resources[INVENTORY_POTTERY].num_buildings = 0;
                                }
                                if (b->data.market.furniture_demand) {
                                    b->data.market.furniture_demand--;
                                } else {
                                    resources[INVENTORY_FURNITURE].num_buildings = 0;
                                }
                                if (b->data.market.oil_demand) {
                                    b->data.market.oil_demand--;
                                } else {
                                    resources[INVENTORY_OIL].num_buildings = 0;
                                }
                                if (b->data.market.wine_demand) {
                                    b->data.market.wine_demand--;
                                } else {
                                    resources[INVENTORY_WINE].num_buildings = 0;
                                }
                                int can_go = 0;
                                for (int ii = 0; ii < INVENTORY_MAX; ii++) {
                                    if (resources[ii].num_buildings) {
                                        can_go = 1;
                                        break;
                                    }
                                }
                                if (can_go) {
                                    int dst_building_id = 0;
                                    int min_stock = 50;
                                    int fetch_inventory = -1;
                                    if (!b->data.market.inventory[INVENTORY_WHEAT] && resources[INVENTORY_WHEAT].num_buildings) { // prefer food if we don't have it
                                        b->data.market.fetch_inventory_id = INVENTORY_WHEAT;
                                        dst_building_id = resources[INVENTORY_WHEAT].building_id;
                                    } else if (!b->data.market.inventory[INVENTORY_VEGETABLES] && resources[INVENTORY_VEGETABLES].num_buildings) {
                                        b->data.market.fetch_inventory_id = INVENTORY_VEGETABLES;
                                        dst_building_id = resources[INVENTORY_VEGETABLES].building_id;
                                    } else if (!b->data.market.inventory[INVENTORY_FRUIT] && resources[INVENTORY_FRUIT].num_buildings) {
                                        b->data.market.fetch_inventory_id = INVENTORY_FRUIT;
                                        dst_building_id = resources[INVENTORY_FRUIT].building_id;
                                    } else if (!b->data.market.inventory[INVENTORY_MEAT] && resources[INVENTORY_MEAT].num_buildings) {
                                        b->data.market.fetch_inventory_id = INVENTORY_MEAT;
                                        dst_building_id = resources[INVENTORY_MEAT].building_id;
                                    } else if (!b->data.market.inventory[INVENTORY_POTTERY] && resources[INVENTORY_POTTERY].num_buildings) { // then prefer resource if we don't have it
                                        b->data.market.fetch_inventory_id = INVENTORY_POTTERY;
                                        dst_building_id = resources[INVENTORY_POTTERY].building_id;
                                    } else if (!b->data.market.inventory[INVENTORY_FURNITURE] && resources[INVENTORY_FURNITURE].num_buildings) {
                                        b->data.market.fetch_inventory_id = INVENTORY_FURNITURE;
                                        dst_building_id = resources[INVENTORY_FURNITURE].building_id;
                                    } else if (!b->data.market.inventory[INVENTORY_OIL] && resources[INVENTORY_OIL].num_buildings) {
                                        b->data.market.fetch_inventory_id = INVENTORY_OIL;
                                        dst_building_id = resources[INVENTORY_OIL].building_id;
                                    } else if (!b->data.market.inventory[INVENTORY_WINE] && resources[INVENTORY_WINE].num_buildings) {
                                        b->data.market.fetch_inventory_id = INVENTORY_WINE;
                                        dst_building_id = resources[INVENTORY_WINE].building_id;
                                    } else { // then prefer smallest stock below 50
                                        if (resources[INVENTORY_WHEAT].num_buildings && b->data.market.inventory[INVENTORY_WHEAT] < min_stock) {
                                            min_stock = b->data.market.inventory[INVENTORY_WHEAT];
                                            fetch_inventory = INVENTORY_WHEAT;
                                        }
                                        if (resources[INVENTORY_VEGETABLES].num_buildings && b->data.market.inventory[INVENTORY_VEGETABLES] < min_stock) {
                                            min_stock = b->data.market.inventory[INVENTORY_VEGETABLES];
                                            fetch_inventory = INVENTORY_VEGETABLES;
                                        }
                                        if (resources[INVENTORY_FRUIT].num_buildings && b->data.market.inventory[INVENTORY_FRUIT] < min_stock) {
                                            min_stock = b->data.market.inventory[INVENTORY_FRUIT];
                                            fetch_inventory = INVENTORY_FRUIT;
                                        }
                                        if (resources[INVENTORY_MEAT].num_buildings && b->data.market.inventory[INVENTORY_MEAT] < min_stock) {
                                            min_stock = b->data.market.inventory[INVENTORY_MEAT];
                                            fetch_inventory = INVENTORY_MEAT;
                                        }
                                        if (resources[INVENTORY_POTTERY].num_buildings && b->data.market.inventory[INVENTORY_POTTERY] < min_stock) {
                                            min_stock = b->data.market.inventory[INVENTORY_POTTERY];
                                            fetch_inventory = INVENTORY_POTTERY;
                                        }
                                        if (resources[INVENTORY_FURNITURE].num_buildings && b->data.market.inventory[INVENTORY_FURNITURE] < min_stock) {
                                            min_stock = b->data.market.inventory[INVENTORY_FURNITURE];
                                            fetch_inventory = INVENTORY_FURNITURE;
                                        }
                                        if (resources[INVENTORY_OIL].num_buildings && b->data.market.inventory[INVENTORY_OIL] < min_stock) {
                                            min_stock = b->data.market.inventory[INVENTORY_OIL];
                                            fetch_inventory = INVENTORY_OIL;
                                        }
                                        if (resources[INVENTORY_WINE].num_buildings && b->data.market.inventory[INVENTORY_WINE] < min_stock) {
                                            fetch_inventory = INVENTORY_WINE;
                                        }
                                        if (fetch_inventory == -1) { // all items well stocked: pick food below threshold
                                            if (resources[INVENTORY_WHEAT].num_buildings && b->data.market.inventory[INVENTORY_WHEAT] < 600) {
                                                fetch_inventory = INVENTORY_WHEAT;
                                            }
                                            if (resources[INVENTORY_VEGETABLES].num_buildings && b->data.market.inventory[INVENTORY_VEGETABLES] < 400) {
                                                fetch_inventory = INVENTORY_VEGETABLES;
                                            }
                                            if (resources[INVENTORY_FRUIT].num_buildings && b->data.market.inventory[INVENTORY_FRUIT] < 400) {
                                                fetch_inventory = INVENTORY_FRUIT;
                                            }
                                            if (resources[INVENTORY_MEAT].num_buildings && b->data.market.inventory[INVENTORY_MEAT] < 400) {
                                                fetch_inventory = INVENTORY_MEAT;
                                            }
                                        }
                                        if (fetch_inventory > 0) {
                                            b->data.market.fetch_inventory_id = fetch_inventory;
                                            dst_building_id = resources[fetch_inventory].building_id;
                                        }
                                    }
                                    if (dst_building_id > 0) {
                                        struct figure_t *f = figure_create(FIGURE_MARKET_BUYER, road.x, road.y, DIR_0_TOP);
                                        f->action_state = FIGURE_ACTION_MARKET_BUYER_GOING_TO_STORAGE;
                                        f->is_targetable = 1;
                                        f->terrain_usage = TERRAIN_USAGE_ROADS;
                                        f->building_id = b->id;
                                        b->figure_id2 = f->id;
                                        f->destination_building_id = dst_building_id;
                                        f->collecting_item_id = b->data.market.fetch_inventory_id;
                                        struct building_t *b_dst = &all_buildings[dst_building_id];
                                        if (map_has_road_access(b_dst->x, b_dst->y, b_dst->size, &road) ||
                                            map_has_road_access(b_dst->x, b_dst->y, 3, &road)) {
                                            f->destination_x = road.x;
                                            f->destination_y = road.y;
                                        } else {
                                            f->action_state = FIGURE_ACTION_MARKET_BUYER_RETURNING;
                                            f->destination_x = f->x;
                                            f->destination_y = f->y;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_BATHHOUSE:
                    if (b->state == BUILDING_STATE_IN_USE) {
                        if (map_terrain_exists_tile_in_area_with_type(b->x, b->y, b->size, TERRAIN_RESERVOIR_RANGE)) {
                            b->has_water_access = 1;
                        } else {
                            b->has_water_access = 0;
                        }
                        if (b->has_water_access && b->num_workers) {
                            if (map_desirability_get(b->grid_offset) <= 30) {
                                map_building_tiles_add(b->id, b->x, b->y, b->size,
                                    image_group(GROUP_BUILDING_BATHHOUSE_WATER), TERRAIN_BUILDING);
                            } else {
                                map_building_tiles_add(b->id, b->x, b->y, b->size,
                                    image_group(GROUP_BUILDING_BATHHOUSE_FANCY_WATER), TERRAIN_BUILDING);
                            }
                        } else {
                            if (map_desirability_get(b->grid_offset) <= 30) {
                                map_building_tiles_add(b->id, b->x, b->y, b->size,
                                    image_group(GROUP_BUILDING_BATHHOUSE_NO_WATER), TERRAIN_BUILDING);
                            } else {
                                map_building_tiles_add(b->id, b->x, b->y, b->size,
                                    image_group(GROUP_BUILDING_BATHHOUSE_FANCY_NO_WATER), TERRAIN_BUILDING);
                            }
                        }
                        check_labor_problem(b);
                        if (!b->has_water_access) {
                            b->show_on_problem_overlay = 2;
                        }
                        if (!has_figure_of_types(b, FIGURE_BATHHOUSE_WORKER, 0)) {
                            if (map_has_road_access(b->x, b->y, b->size, &road) && b->has_water_access) {
                                spawn_labor_seeker(b, road.x, road.y, 50);
                                int spawn_delay = default_spawn_delay(b);
                                if (spawn_delay) {
                                    b->figure_spawn_delay++;
                                    if (b->figure_spawn_delay > spawn_delay) {
                                        b->figure_spawn_delay = 0;
                                        create_roaming_figure(b, road.x, road.y, FIGURE_BATHHOUSE_WORKER);
                                    }
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_SCHOOL:
                    check_labor_problem(b);
                    if (!has_figure_of_types(b, FIGURE_SCHOOL_CHILD, 0)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            spawn_labor_seeker(b, road.x, road.y, 50);
                            int spawn_delay = default_spawn_delay(b);
                            if (spawn_delay) {
                                b->figure_spawn_delay++;
                                if (b->figure_spawn_delay > spawn_delay) {
                                    b->figure_spawn_delay = 0;
                                    struct figure_t *child1 = figure_create(FIGURE_SCHOOL_CHILD, road.x, road.y, DIR_0_TOP);
                                    child1->action_state = FIGURE_ACTION_ROAMING;
                                    child1->is_targetable = 1;
                                    child1->terrain_usage = TERRAIN_USAGE_ROADS;
                                    child1->building_id = b->id;
                                    b->figure_id = child1->id;
                                    figure_movement_init_roaming(child1);
                                    struct figure_t *child2 = figure_create(FIGURE_SCHOOL_CHILD, road.x, road.y, DIR_0_TOP);
                                    child2->action_state = FIGURE_ACTION_ROAMING;
                                    child2->is_targetable = 1;
                                    child2->terrain_usage = TERRAIN_USAGE_ROADS;
                                    child2->building_id = b->id;
                                    figure_movement_init_roaming(child2);
                                    struct figure_t *child3 = figure_create(FIGURE_SCHOOL_CHILD, road.x, road.y, DIR_0_TOP);
                                    child3->action_state = FIGURE_ACTION_ROAMING;
                                    child3->is_targetable = 1;
                                    child3->terrain_usage = TERRAIN_USAGE_ROADS;
                                    child3->building_id = b->id;
                                    figure_movement_init_roaming(child3);
                                    struct figure_t *child4 = figure_create(FIGURE_SCHOOL_CHILD, road.x, road.y, DIR_0_TOP);
                                    child4->action_state = FIGURE_ACTION_ROAMING;
                                    child4->is_targetable = 1;
                                    child4->terrain_usage = TERRAIN_USAGE_ROADS;
                                    child4->building_id = b->id;
                                    figure_movement_init_roaming(child4);
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_LIBRARY:
                    check_labor_problem(b);
                    if (!has_figure_of_types(b, FIGURE_LIBRARIAN, 0)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            spawn_labor_seeker(b, road.x, road.y, 50);
                            int spawn_delay = default_spawn_delay(b);
                            if (spawn_delay) {
                                b->figure_spawn_delay++;
                                if (b->figure_spawn_delay > spawn_delay) {
                                    b->figure_spawn_delay = 0;
                                    create_roaming_figure(b, road.x, road.y, FIGURE_LIBRARIAN);
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_ACADEMY:
                    check_labor_problem(b);
                    if (!has_figure_of_types(b, FIGURE_TEACHER, 0)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            spawn_labor_seeker(b, road.x, road.y, 50);
                            int spawn_delay = default_spawn_delay(b);
                            if (spawn_delay) {
                                b->figure_spawn_delay++;
                                if (b->figure_spawn_delay > spawn_delay) {
                                    b->figure_spawn_delay = 0;
                                    create_roaming_figure(b, road.x, road.y, FIGURE_TEACHER);
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_BARBER:
                    check_labor_problem(b);
                    if (!has_figure_of_types(b, FIGURE_BARBER, 0)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            spawn_labor_seeker(b, road.x, road.y, 50);
                            int spawn_delay = default_spawn_delay(b);
                            if (spawn_delay) {
                                b->figure_spawn_delay++;
                                if (b->figure_spawn_delay > spawn_delay) {
                                    b->figure_spawn_delay = 0;
                                    create_roaming_figure(b, road.x, road.y, FIGURE_BARBER);
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_DOCTOR:
                    check_labor_problem(b);
                    if (!has_figure_of_types(b, FIGURE_DOCTOR, 0)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            spawn_labor_seeker(b, road.x, road.y, 50);
                            int spawn_delay = default_spawn_delay(b);
                            if (spawn_delay) {
                                b->figure_spawn_delay++;
                                if (b->figure_spawn_delay > spawn_delay) {
                                    b->figure_spawn_delay = 0;
                                    create_roaming_figure(b, road.x, road.y, FIGURE_DOCTOR);
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_HOSPITAL:
                    check_labor_problem(b);
                    if (!has_figure_of_types(b, FIGURE_SURGEON, 0)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            spawn_labor_seeker(b, road.x, road.y, 50);
                            int spawn_delay = default_spawn_delay(b);
                            if (spawn_delay) {
                                b->figure_spawn_delay++;
                                if (b->figure_spawn_delay > spawn_delay) {
                                    b->figure_spawn_delay = 0;
                                    create_roaming_figure(b, road.x, road.y, FIGURE_SURGEON);
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_MISSION_POST:
                    if (!has_figure_of_types(b, FIGURE_MISSIONARY, 0)) {
                        if (map_has_road_access(b->x, b->y, b->size, &road)) {
                            if (city_data.population.population > 0) {
                                city_data.building.mission_post_operational = 1;
                                b->figure_spawn_delay++;
                                if (b->figure_spawn_delay > 1) {
                                    b->figure_spawn_delay = 0;
                                    create_roaming_figure(b, road.x, road.y, FIGURE_MISSIONARY);
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_DOCK:
                    check_labor_problem(b);
                    if (map_has_road_access(b->x, b->y, b->size, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 50);
                        int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                        int max_dockers;
                        if (pct_workers >= 75) {
                            max_dockers = 3;
                        } else if (pct_workers >= 50) {
                            max_dockers = 2;
                        } else if (pct_workers > 0) {
                            max_dockers = 1;
                        } else {
                            max_dockers = 0;
                        }
                        // count existing dockers
                        int existing_dockers = 0;
                        for (int ii = 0; ii < 3; ii++) {
                            if (b->data.dock.docker_ids[ii]) {
                                if (figures[b->data.dock.docker_ids[ii]].type == FIGURE_DOCKER) {
                                    existing_dockers++;
                                } else {
                                    b->data.dock.docker_ids[ii] = 0;
                                }
                            }
                        }
                        if (existing_dockers > max_dockers) {
                            // too many dockers, kill one of them
                            for (int ii = 2; ii >= 0; ii--) {
                                if (b->data.dock.docker_ids[ii]) {
                                    figure_delete(&figures[b->data.dock.docker_ids[ii]]);
                                    break;
                                }
                            }
                        } else if (existing_dockers < max_dockers) {
                            struct figure_t *f = figure_create(FIGURE_DOCKER, road.x, road.y, DIR_4_BOTTOM);
                            f->action_state = FIGURE_ACTION_DOCKER_IDLING;
                            f->is_targetable = 1;
                            f->terrain_usage = TERRAIN_USAGE_ROADS;
                            f->building_id = b->id;
                            for (int ii = 0; ii < 3; ii++) {
                                if (!b->data.dock.docker_ids[ii]) {
                                    b->data.dock.docker_ids[ii] = f->id;
                                    break;
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_WHARF:
                    check_labor_problem(b);
                    if (b->data.industry.fishing_boat_id) {
                        struct figure_t *f = &figures[b->data.industry.fishing_boat_id];
                        if (!figure_is_alive(f) || f->type != FIGURE_FISHING_BOAT) {
                            b->data.industry.fishing_boat_id = 0;
                        }
                    }
                    if (map_has_road_access(b->x, b->y, b->size, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 50);
                        if (!has_figure_of_types(b, FIGURE_CART_PUSHER, 0)) {
                            if (b->figure_spawn_delay) {
                                b->figure_spawn_delay = 0;
                                b->data.industry.has_fish = 0;
                                b->output_resource_id = RESOURCE_MEAT;
                                struct figure_t *f = figure_create(FIGURE_CART_PUSHER, road.x, road.y, DIR_4_BOTTOM);
                                f->action_state = FIGURE_ACTION_CARTPUSHER_INITIAL;
                                f->is_targetable = 1;
                                f->terrain_usage = TERRAIN_USAGE_ROADS;
                                f->resource_id = RESOURCE_MEAT;
                                f->building_id = b->id;
                                b->figure_id = f->id;
                                f->wait_ticks = 30;
                            }
                        }
                    }
                    break;
                case BUILDING_SHIPYARD:
                    check_labor_problem(b);
                    if (map_has_road_access(b->x, b->y, b->size, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 50);
                        if (!has_figure_of_types(b, FIGURE_FISHING_BOAT, 0)) {
                            int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                            if (pct_workers >= 100) {
                                b->data.industry.progress += 10;
                            } else if (pct_workers >= 75) {
                                b->data.industry.progress += 8;
                            } else if (pct_workers >= 50) {
                                b->data.industry.progress += 6;
                            } else if (pct_workers >= 25) {
                                b->data.industry.progress += 4;
                            } else if (pct_workers >= 1) {
                                b->data.industry.progress += 2;
                            }
                            if (b->data.industry.progress >= 160) {
                                b->data.industry.progress = 0;
                                struct map_point_t boat;
                                int base_offset = map_grid_offset(b->x, b->y);
                                for (const int *tile_delta = map_grid_adjacent_offsets(b->size); *tile_delta; tile_delta++) {
                                    int grid_offset = base_offset + *tile_delta;
                                    if (map_terrain_is(grid_offset, TERRAIN_WATER)) {
                                        if (!map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
                                            int surrounding_water_tiles = 0;
                                            for (int j = 0; j < DIR_8_NONE; j++) {
                                                if (map_terrain_is(grid_offset + map_grid_direction_delta(j), TERRAIN_WATER)) {
                                                    surrounding_water_tiles++;
                                                }
                                            }
                                            if (surrounding_water_tiles >= 8) {
                                                boat.x = map_grid_offset_to_x(grid_offset);
                                                boat.y = map_grid_offset_to_y(grid_offset);
                                                struct figure_t *f = figure_create(FIGURE_FISHING_BOAT, boat.x, boat.y, DIR_0_TOP);
                                                f->action_state = FIGURE_ACTION_FISHING_BOAT_CREATED;
                                                f->building_id = b->id;
                                                b->figure_id = f->id;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_NATIVE_HUT:
                    map_image_set(b->grid_offset, image_group(GROUP_BUILDING_NATIVE) + (map_random_get(b->grid_offset) & 1));
                    if (!has_figure_of_types(b, FIGURE_INDIGENOUS_NATIVE, 0)) {
                        if (b->subtype.native_meeting_center_id > 0
                            && map_terrain_get_adjacent_road_or_clear_land(b->x, b->y, b->size, &x_out, &y_out)) {
                            b->figure_spawn_delay++;
                            if (b->figure_spawn_delay > 4) {
                                b->figure_spawn_delay = 0;
                                struct figure_t *f = figure_create(FIGURE_INDIGENOUS_NATIVE, x_out, y_out, DIR_0_TOP);
                                f->is_targetable = 1;
                                f->action_state = FIGURE_ACTION_NATIVE_CREATED;
                                f->terrain_usage = TERRAIN_USAGE_ANY;
                                f->building_id = b->id;
                                b->figure_id = f->id;
                            }
                        }
                    }
                    break;
                case BUILDING_NATIVE_MEETING:
                    map_building_tiles_add(b->id, b->x, b->y, 2, image_group(GROUP_BUILDING_NATIVE) + 2, TERRAIN_BUILDING);
                    if (city_data.building.mission_post_operational > 0 && !has_figure_of_types(b, FIGURE_NATIVE_TRADER, 0)) {
                        if (map_terrain_get_adjacent_road_or_clear_land(b->x, b->y, b->size, &x_out, &y_out)) {
                            b->figure_spawn_delay++;
                            if (b->figure_spawn_delay > 8) {
                                b->figure_spawn_delay = 0;
                                struct figure_t *f = figure_create(FIGURE_NATIVE_TRADER, x_out, y_out, DIR_0_TOP);
                                f->is_targetable = 1;
                                f->action_state = FIGURE_ACTION_NATIVE_TRADER_CREATED;
                                f->terrain_usage = TERRAIN_USAGE_ANY;
                                f->building_id = b->id;
                                b->figure_id = f->id;
                            }
                        }
                    }
                    break;
                case BUILDING_NATIVE_CROPS:
                    b->data.industry.progress++;
                    if (b->data.industry.progress >= 5) {
                        b->data.industry.progress = 0;
                    }
                    map_image_set(b->grid_offset, image_group(GROUP_BUILDING_FARM_CROPS) + b->data.industry.progress);
                    break;
                case BUILDING_BARRACKS:
                    check_labor_problem(b);
                    if (map_has_road_access(b->x, b->y, b->size, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 100);
                        int pct_workers = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
                        int spawn_delay;
                        if (pct_workers >= 100) {
                            spawn_delay = 8;
                        } else if (pct_workers >= 75) {
                            spawn_delay = 12;
                        } else if (pct_workers >= 50) {
                            spawn_delay = 16;
                        } else if (pct_workers >= 25) {
                            spawn_delay = 32;
                        } else if (pct_workers >= 1) {
                            spawn_delay = 48;
                        } else {
                            spawn_delay = INFINITE;
                        }
                        b->figure_spawn_delay++;
                        if (b->figure_spawn_delay > spawn_delay) {
                            b->figure_spawn_delay = 0;
                            map_has_road_access(b->x, b->y, b->size, &road);
                            int create_tower_sentry = 0;
                            if (tower_sentry_request) {
                                struct building_t *tower = 0;
                                for (int ii = 1; ii < MAX_BUILDINGS; ii++) {
                                    struct building_t *bb = &all_buildings[ii];
                                    if (bb->state == BUILDING_STATE_IN_USE && bb->type == BUILDING_TOWER && bb->num_workers > 0 &&
                                        !bb->figure_id && b->road_network_id == bb->road_network_id) {
                                        tower = bb;
                                        break;
                                    }
                                }
                                if (tower) {
                                    struct map_point_t tower_road;
                                    if (map_has_road_access(tower->x, tower->y, tower->size, &tower_road)) {
                                        struct figure_t *f = figure_create(FIGURE_TOWER_SENTRY, road.x, road.y, DIR_0_TOP);
                                        tower->figure_id = f->id;
                                        f->building_id = tower->id;
                                        struct map_point_t mil_acad_road = { 0 };
                                        set_destination__closest_building_of_type(tower->id, BUILDING_MILITARY_ACADEMY, &mil_acad_road);
                                        if (mil_acad_road.x) {
                                            f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_MILITARY_ACADEMY;
                                            f->destination_x = mil_acad_road.x;
                                            f->destination_y = mil_acad_road.y;
                                            f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
                                        } else {
                                            f->action_state = FIGURE_ACTION_TOWER_SENTRY_GOING_TO_TOWER;
                                        }
                                        create_tower_sentry = 1;
                                    }
                                }
                            }
                            if (!create_tower_sentry) {
                                struct formation_t *closest_formation = 0;
                                int closest_formation_distance = 10000;
                                for (int j = 0; j < MAX_LEGIONS; j++) {
                                    struct formation_t *m = &legion_formations[j];
                                    if (m->in_use && !m->in_distant_battle && !m->cursed_by_mars && m->num_figures < m->max_figures) {
                                        if (m->figure_type == FIGURE_FORT_LEGIONARY && !b->loads_stored) {
                                            continue;
                                        }
                                        struct building_t *fort = &all_buildings[m->building_id];
                                        int dist = calc_maximum_distance(b->x, b->y, fort->x, fort->y);
                                        if (dist < closest_formation_distance) {
                                            // prefer legionaries
                                            if (closest_formation && closest_formation->figure_type == FIGURE_FORT_LEGIONARY && m->figure_type != FIGURE_FORT_LEGIONARY) {
                                                continue;
                                            }
                                            closest_formation = &legion_formations[j];
                                            closest_formation_distance = dist;
                                        }
                                    }
                                }
                                if (closest_formation) {
                                    struct figure_t *f = figure_create(closest_formation->figure_type, road.x, road.y, DIR_0_TOP);
                                    f->is_targetable = 1;
                                    f->terrain_usage = TERRAIN_USAGE_ANY;
                                    f->formation_id = closest_formation->id;
                                    switch (f->type) {
                                        case FIGURE_FORT_JAVELIN:
                                            f->speed_multiplier = 2;
                                            break;
                                        case FIGURE_FORT_MOUNTED:
                                            f->mounted_charge_ticks = 10;
                                            f->mounted_charge_ticks_max = 10;
                                            f->speed_multiplier = 3;
                                            break;
                                        case FIGURE_FORT_LEGIONARY:
                                            if (b->loads_stored) {
                                                b->loads_stored--;
                                            }
                                            break;
                                    }
                                    f->building_id = closest_formation->building_id;
                                    add_figure_to_formation(f, closest_formation);
                                    struct map_point_t mil_acad_road = { 0 };
                                    set_destination__closest_building_of_type(closest_formation->building_id, BUILDING_MILITARY_ACADEMY, &mil_acad_road);
                                    if (mil_acad_road.x) {
                                        f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_MILITARY_ACADEMY;
                                        f->destination_x = mil_acad_road.x;
                                        f->destination_y = mil_acad_road.y;
                                        f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
                                    } else {
                                        if (closest_formation->is_at_rest) {
                                            f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_FORT;
                                        } else {
                                            deploy_legion_unit_to_formation_location(f, closest_formation);
                                        }
                                    }
                                    city_data.figure.soldiers++;
                                }
                            }
                        }
                    }
                    break;
                case BUILDING_MILITARY_ACADEMY:
                    check_labor_problem(b);
                    if (map_has_road_access(b->x, b->y, b->size, &road)) {
                        spawn_labor_seeker(b, road.x, road.y, 100);
                    }
                    break;
            }
        }
    }
}

int building_granary_add_resource(struct building_t *granary, int resource, int is_produced)
{
    if (granary->id <= 0) {
        return 1;
    }
    if (!resource_is_food(resource)) {
        return 0;
    }
    if (granary->type != BUILDING_GRANARY) {
        return 0;
    }
    if (granary->data.granary.resource_stored[RESOURCE_NONE] <= 0) {
        return 0; // no space
    }
    if (is_produced) {
        city_data.resource.food_produced_this_month += ONE_LOAD;
    }
    if (granary->data.granary.resource_stored[RESOURCE_NONE] <= ONE_LOAD) {
        granary->data.granary.resource_stored[resource] += granary->data.granary.resource_stored[RESOURCE_NONE];
        granary->data.granary.resource_stored[RESOURCE_NONE] = 0;
    } else {
        granary->data.granary.resource_stored[resource] += ONE_LOAD;
        granary->data.granary.resource_stored[RESOURCE_NONE] -= ONE_LOAD;
    }
    return 1;
}

int building_granary_remove_resource(struct building_t *granary, int resource, int amount)
{
    if (amount <= 0) {
        return 0;
    }
    int removed;
    if (granary->data.granary.resource_stored[resource] >= amount) {
        removed = amount;
    } else {
        removed = granary->data.granary.resource_stored[resource];
    }
    city_data.resource.granary_food_stored[resource] -= removed;
    granary->data.granary.resource_stored[resource] -= removed;
    granary->data.granary.resource_stored[RESOURCE_NONE] += removed;
    return amount - removed;
}

void building_granaries_calculate_stocks(void)
{
    non_getting_granaries.num_items = 0;
    for (int i = 0; i < MAX_GRANARIES; i++) {
        non_getting_granaries.building_ids[i] = 0;
    }
    non_getting_granaries.total_storage_wheat = 0;
    non_getting_granaries.total_storage_vegetables = 0;
    non_getting_granaries.total_storage_fruit = 0;
    non_getting_granaries.total_storage_meat = 0;

    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_GRANARY) {
            continue;
        }
        if (!b->has_road_access) {
            continue;
        }
        struct building_storage_t *s = building_storage_get(b->storage_id);
        int total_non_getting = 0;
        if (s->resource_state[RESOURCE_WHEAT] != BUILDING_STORAGE_STATE_GETTING) {
            total_non_getting += b->data.granary.resource_stored[RESOURCE_WHEAT];
            non_getting_granaries.total_storage_wheat += b->data.granary.resource_stored[RESOURCE_WHEAT];
        }
        if (s->resource_state[RESOURCE_VEGETABLES] != BUILDING_STORAGE_STATE_GETTING) {
            total_non_getting += b->data.granary.resource_stored[RESOURCE_VEGETABLES];
            non_getting_granaries.total_storage_vegetables += b->data.granary.resource_stored[RESOURCE_VEGETABLES];
        }
        if (s->resource_state[RESOURCE_FRUIT] != BUILDING_STORAGE_STATE_GETTING) {
            total_non_getting += b->data.granary.resource_stored[RESOURCE_FRUIT];
            non_getting_granaries.total_storage_fruit += b->data.granary.resource_stored[RESOURCE_FRUIT];
        }
        if (s->resource_state[RESOURCE_MEAT] != BUILDING_STORAGE_STATE_GETTING) {
            total_non_getting += b->data.granary.resource_stored[RESOURCE_MEAT];
            non_getting_granaries.total_storage_meat += b->data.granary.resource_stored[RESOURCE_MEAT];
        }
        if (total_non_getting > MAX_GRANARIES) {
            non_getting_granaries.building_ids[non_getting_granaries.num_items] = i;
            if (non_getting_granaries.num_items < MAX_GRANARIES - 2) {
                non_getting_granaries.num_items++;
            }
        }
    }
}

int building_granary_for_getting(struct building_t *src, struct map_point_t *dst)
{
    struct building_storage_t *s_src = building_storage_get(src->storage_id);
    if (s_src->empty_all) {
        return 0;
    }
    if (scenario.rome_supplies_wheat) {
        return 0;
    }
    int is_getting = 0;
    if (s_src->resource_state[RESOURCE_WHEAT] == BUILDING_STORAGE_STATE_GETTING ||
            s_src->resource_state[RESOURCE_VEGETABLES] == BUILDING_STORAGE_STATE_GETTING ||
            s_src->resource_state[RESOURCE_FRUIT] == BUILDING_STORAGE_STATE_GETTING ||
            s_src->resource_state[RESOURCE_MEAT] == BUILDING_STORAGE_STATE_GETTING) {
        is_getting = 1;
    }
    if (is_getting <= 0) {
        return 0;
    }

    int min_dist = INFINITE;
    int min_building_id = 0;
    for (int i = 0; i < non_getting_granaries.num_items; i++) {
        struct building_t *b = &all_buildings[non_getting_granaries.building_ids[i]];
        if (b->road_network_id != src->road_network_id) {
            continue;
        }
        struct building_storage_t *s = building_storage_get(b->storage_id);
        int amount_gettable = 0;
        if (s_src->resource_state[RESOURCE_WHEAT] == BUILDING_STORAGE_STATE_GETTING &&
            s->resource_state[RESOURCE_WHEAT] != BUILDING_STORAGE_STATE_GETTING) {
            amount_gettable += b->data.granary.resource_stored[RESOURCE_WHEAT];
        }
        if (s_src->resource_state[RESOURCE_VEGETABLES] == BUILDING_STORAGE_STATE_GETTING &&
            s->resource_state[RESOURCE_VEGETABLES] != BUILDING_STORAGE_STATE_GETTING) {
            amount_gettable += b->data.granary.resource_stored[RESOURCE_VEGETABLES];
        }
        if (s_src->resource_state[RESOURCE_FRUIT] == BUILDING_STORAGE_STATE_GETTING &&
            s->resource_state[RESOURCE_FRUIT] != BUILDING_STORAGE_STATE_GETTING) {
            amount_gettable += b->data.granary.resource_stored[RESOURCE_FRUIT];
        }
        if (s_src->resource_state[RESOURCE_MEAT] == BUILDING_STORAGE_STATE_GETTING &&
            s->resource_state[RESOURCE_MEAT] != BUILDING_STORAGE_STATE_GETTING) {
            amount_gettable += b->data.granary.resource_stored[RESOURCE_MEAT];
        }
        if (amount_gettable > 0) {
            int dist = calc_maximum_distance(
                b->x + 1, b->y + 1,
                src->x + 1, src->y + 1);
            if (amount_gettable <= 400) {
                dist *= 2; // penalty for less food
            }
            if (dist < min_dist) {
                min_dist = dist;
                min_building_id = b->id;
            }
        }
    }
    struct building_t *min = &all_buildings[min_building_id];
    dst->x = min->x + 1;
    dst->y = min->y + 1;
    return min_building_id;
}

int building_is_farm(int type)
{
    return type >= BUILDING_WHEAT_FARM && type <= BUILDING_VINES_FARM;
}

int building_is_workshop(int type)
{
    return type >= BUILDING_OIL_WORKSHOP && type <= BUILDING_WEAPONS_WORKSHOP;
}

void update_farm_image(const struct building_t *b)
{
    map_building_tiles_add_farm(b->id, b->x, b->y,
        image_group(GROUP_BUILDING_FARM_CROPS) + resource_images[b->output_resource_id].farm_field_img_id,
        b->data.industry.progress);
}

void building_list_small_clear(void)
{
    building_list_data.small.size = 0;
}

void building_list_small_add(int building_id)
{
    building_list_data.small.items[building_list_data.small.size++] = building_id;
    if (building_list_data.small.size >= MAX_SMALL) {
        building_list_data.small.size = MAX_SMALL - 1;
    }
}

int building_list_small_size(void)
{
    return building_list_data.small.size;
}

const int *building_list_small_items(void)
{
    return building_list_data.small.items;
}

void building_list_large_clear(int clear_entries)
{
    building_list_data.large.size = 0;
    if (clear_entries) {
        memset(building_list_data.large.items, 0, MAX_LARGE * sizeof(int));
    }
}

void building_list_large_add(int building_id)
{
    if (building_list_data.large.size < MAX_LARGE) {
        building_list_data.large.items[building_list_data.large.size++] = building_id;
    }
}

int building_list_large_size(void)
{
    return building_list_data.large.size;
}

const int *building_list_large_items(void)
{
    return building_list_data.large.items;
}

void building_list_burning_clear(void)
{
    building_list_data.burning.size = 0;
    building_list_data.burning.total = 0;
}

void building_list_burning_add(int building_id)
{
    building_list_data.burning.total++;
    building_list_data.burning.items[building_list_data.burning.size++] = building_id;
    if (building_list_data.burning.size >= MAX_BURNING) {
        building_list_data.burning.size = MAX_BURNING - 1;
    }
}

int building_list_burning_size(void)
{
    return building_list_data.burning.size;
}

const int *building_list_burning_items(void)
{
    return building_list_data.burning.items;
}

void building_list_save_state(struct buffer_t *small, struct buffer_t *large, struct buffer_t *burning, struct buffer_t *burning_totals)
{
    for (int i = 0; i < MAX_SMALL; i++) {
        buffer_write_i16(small, building_list_data.small.items[i]);
    }
    for (int i = 0; i < MAX_LARGE; i++) {
        buffer_write_i16(large, building_list_data.large.items[i]);
    }
    for (int i = 0; i < MAX_BURNING; i++) {
        buffer_write_i16(burning, building_list_data.burning.items[i]);
    }
    buffer_write_i32(burning_totals, building_list_data.burning.total);
    buffer_write_i32(burning_totals, building_list_data.burning.size);
}

void building_list_load_state(struct buffer_t *small, struct buffer_t *large, struct buffer_t *burning, struct buffer_t *burning_totals)
{
    for (int i = 0; i < MAX_SMALL; i++) {
        building_list_data.small.items[i] = buffer_read_i16(small);
    }
    for (int i = 0; i < MAX_LARGE; i++) {
        building_list_data.large.items[i] = buffer_read_i16(large);
    }
    for (int i = 0; i < MAX_BURNING; i++) {
        building_list_data.burning.items[i] = buffer_read_i16(burning);
    }
    building_list_data.burning.total = buffer_read_i32(burning_totals);
    building_list_data.burning.size = buffer_read_i32(burning_totals);
}

int building_market_get_max_food_stock(struct building_t *market)
{
    int max_stock = 0;
    if (market->id > 0 && market->type == BUILDING_MARKET) {
        for (int i = INVENTORY_WHEAT; i <= INVENTORY_MEAT; i++) {
            int stock = market->data.market.inventory[i];
            if (stock > max_stock) {
                max_stock = stock;
            }
        }
    }
    return max_stock;
}

void building_storage_clear_all(void)
{
    memset(storages, 0, MAX_STORAGES * sizeof(struct data_storage));
}

void building_storage_reset_building_ids(void)
{
    for (int i = 1; i < MAX_STORAGES; i++) {
        storages[i].building_id = 0;
    }

    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_UNUSED) {
            continue;
        }
        if (b->type == BUILDING_GRANARY || b->type == BUILDING_WAREHOUSE) {
            if (b->storage_id) {
                if (storages[b->storage_id].building_id) {
                    // storage is already connected to a building: corrupt, create new
                    b->storage_id = building_storage_create();
                } else {
                    storages[b->storage_id].building_id = i;
                }
            }
        }
    }
}

int building_storage_restore(int storage_id)
{
    if (storages[storage_id].in_use) {
        return 0;
    }
    storages[storage_id].in_use = 1;
    return storage_id;
}

struct building_storage_t *building_storage_get(int storage_id)
{
    return &storages[storage_id].storage;
}

void building_storage_save_state(struct buffer_t *buf)
{
    for (int i = 0; i < MAX_STORAGES; i++) {
        buffer_write_i32(buf, storages[i].building_id);
        buffer_write_u8(buf, (uint8_t) storages[i].in_use);
        buffer_write_u8(buf, (uint8_t) storages[i].storage.empty_all);
        for (int r = 0; r < RESOURCE_TYPES_MAX; r++) {
            buffer_write_u8(buf, storages[i].storage.resource_state[r]);
        }
    }
}

void building_storage_load_state(struct buffer_t *buf)
{
    for (int i = 0; i < MAX_STORAGES; i++) {
        storages[i].building_id = buffer_read_i32(buf);
        storages[i].in_use = buffer_read_u8(buf);
        storages[i].storage.empty_all = buffer_read_u8(buf);
        for (int r = 0; r < RESOURCE_TYPES_MAX; r++) {
            storages[i].storage.resource_state[r] = buffer_read_u8(buf);
        }
    }
}

int building_warehouse_get_amount(struct building_t *warehouse, int resource)
{
    int loads = 0;
    struct building_t *space = warehouse;
    for (int i = 0; i < 8; i++) {
        space = &all_buildings[space->next_part_building_id];
        if (space->id <= 0) {
            return 0;
        }
        if (space->subtype.warehouse_resource_id && space->subtype.warehouse_resource_id == resource) {
            loads += space->loads_stored;
        }
    }
    return loads;
}

int building_warehouse_add_resource(struct building_t *b, int resource)
{
    if (b->id <= 0) {
        return 0;
    }
    // check building itself
    int find_space = 0;
    if (b->subtype.warehouse_resource_id && b->subtype.warehouse_resource_id != resource) {
        find_space = 1;
    } else if (b->loads_stored >= 4) {
        find_space = 1;
    } else if (b->type == BUILDING_WAREHOUSE) {
        find_space = 1;
    }
    if (find_space) {
        int space_found = 0;
        struct building_t *space = building_main(b);
        for (int i = 0; i < 8; i++) {
            space = &all_buildings[space->next_part_building_id];
            if (!space->id) {
                return 0;
            }
            if (!space->subtype.warehouse_resource_id || space->subtype.warehouse_resource_id == resource) {
                if (space->loads_stored < 4) {
                    space_found = 1;
                    b = space;
                    break;
                }
            }
        }
        if (!space_found) {
            return 0;
        }
    }
    city_resource_add_to_warehouse(resource, 1);
    b->subtype.warehouse_resource_id = resource;
    b->loads_stored++;
    building_warehouse_space_set_image(b, resource);
    return 1;
}

int building_warehouse_remove_resource(struct building_t *warehouse, int resource, int amount)
{
    // returns amount still needing removal
    if (warehouse->type != BUILDING_WAREHOUSE) {
        return amount;
    }
    struct building_t *space = warehouse;
    for (int i = 0; i < 8; i++) {
        if (amount <= 0) {
            return 0;
        }
        space = &all_buildings[space->next_part_building_id];
        if (space->id <= 0) {
            continue;
        }
        if (space->subtype.warehouse_resource_id != resource || space->loads_stored <= 0) {
            continue;
        }
        if (space->loads_stored > amount) {
            city_resource_remove_from_warehouse(resource, amount);
            space->loads_stored -= amount;
            amount = 0;
        } else {
            city_resource_remove_from_warehouse(resource, space->loads_stored);
            amount -= space->loads_stored;
            space->loads_stored = 0;
            space->subtype.warehouse_resource_id = RESOURCE_NONE;
        }
        building_warehouse_space_set_image(space, resource);
    }
    return amount;
}

void building_warehouse_space_set_image(struct building_t *space, int resource)
{
    int image_id;
    if (space->loads_stored <= 0) {
        image_id = EMPTY_WAREHOUSE_IMG_ID;
    } else {
        image_id = resource_images[resource].warehouse_space_img_id +
            resource_image_offset(resource, RESOURCE_IMAGE_STORAGE) +
            space->loads_stored - 1;
    }
    map_image_set(space->grid_offset, image_id);
}

void building_warehouse_space_add_import(struct building_t *space, int resource)
{
    city_resource_add_to_warehouse(resource, 1);
    space->loads_stored++;
    space->subtype.warehouse_resource_id = resource;

    city_data.finance.treasury -= trade_prices[resource].buy;
    city_data.finance.this_year.expenses.imports += trade_prices[resource].buy;

    building_warehouse_space_set_image(space, resource);
}

int building_warehouses_remove_resource(int resource, int amount)
{
    int amount_left = amount;
    int building_id = city_data.resource.last_used_warehouse;
    // first go for non-getting warehouses
    for (int i = 1; i < MAX_BUILDINGS && amount_left > 0; i++) {
        building_id++;
        if (building_id >= MAX_BUILDINGS) {
            building_id = 1;
        }
        struct building_t *b = &all_buildings[building_id];
        if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_WAREHOUSE) {
            if (building_storage_get(b->storage_id)->resource_state[resource] != BUILDING_STORAGE_STATE_GETTING) {
                city_data.resource.last_used_warehouse = building_id;
                amount_left = building_warehouse_remove_resource(b, resource, amount_left);
            }
        }
    }
    // if that doesn't work, take it anyway
    for (int i = 1; i < MAX_BUILDINGS && amount_left > 0; i++) {
        building_id++;
        if (building_id >= MAX_BUILDINGS) {
            building_id = 1;
        }
        struct building_t *b = &all_buildings[building_id];
        if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_WAREHOUSE) {
            city_data.resource.last_used_warehouse = building_id;
            amount_left = building_warehouse_remove_resource(b, resource, amount_left);
        }
    }
    return amount - amount_left;
}

int building_warehouse_for_storing(int src_building_id, int x, int y, int resource,
                                   int road_network_id, int *understaffed,
                                   struct map_point_t *dst)
{
    int min_dist = 10000;
    int min_building_id = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_WAREHOUSE_SPACE) {
            continue;
        }
        if (!b->has_road_access || b->road_network_id != road_network_id) {
            continue;
        }
        struct building_t *building_dst = building_main(b);
        if (src_building_id == building_dst->id) {
            continue;
        }
        struct building_storage_t *s = building_storage_get(building_dst->storage_id);
        if (s->resource_state[resource] == BUILDING_STORAGE_STATE_NOT_ACCEPTING || s->empty_all) {
            continue;
        }
        if (calc_percentage(building_dst->num_workers, building_properties[building_dst->type].n_laborers) < 100) {
            if (understaffed) {
                *understaffed += 1;
            }
            continue;
        }
        int dist;
        if (b->subtype.warehouse_resource_id == RESOURCE_NONE) { // empty warehouse space
            dist = calc_maximum_distance(b->x, b->y, x, y);
        } else if (b->subtype.warehouse_resource_id == resource && b->loads_stored < 4) {
            dist = calc_maximum_distance(b->x, b->y, x, y);
        } else {
            dist = 0;
        }
        if (dist > 0 && dist < min_dist) {
            min_dist = dist;
            min_building_id = i;
        }
    }
    struct building_t *b = building_main(&all_buildings[min_building_id]);
    if (b->has_road_access == 1) {
        dst->x = b->x;
        dst->y = b->y;
    } else if (!map_has_road_access(b->x, b->y, 3, dst)) {
        return 0;
    }
    return min_building_id;
}

int building_warehouse_for_getting(struct building_t *src, int resource, struct map_point_t *dst)
{
    int min_dist = 10000;
    struct building_t *min_building = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_WAREHOUSE) {
            continue;
        }
        if (i == src->id) {
            continue;
        }
        int loads_stored = 0;
        struct building_t *space = b;
        struct building_storage_t *s = building_storage_get(b->storage_id);
        for (int t = 0; t < 8; t++) {
            space = &all_buildings[space->next_part_building_id];
            if (space->id > 0 && space->loads_stored > 0) {
                if (space->subtype.warehouse_resource_id == resource) {
                    loads_stored += space->loads_stored;
                }
            }
        }
        if (loads_stored > 0 && s->resource_state[resource] != BUILDING_STORAGE_STATE_GETTING) {
            int dist = calc_maximum_distance(b->x, b->y, src->x, src->y);
            dist -= 4 * loads_stored;
            if (dist < min_dist) {
                min_dist = dist;
                min_building = b;
            }
        }
    }
    if (min_building) {
        dst->x = min_building->road_access_x;
        dst->y = min_building->road_access_y;
        return min_building->id;
    } else {
        return 0;
    }
}

static void window_building_info_show_storage_orders(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    b_info_context.storage_show_special_orders = 1;
    window_invalidate();
}

static struct generic_button_t go_to_orders_button[] = {
    {0, 0, 304, 20, window_building_info_show_storage_orders, button_none, 0, 0}
};

static void building_storage_cycle_resource_state(int storage_id, int resource_id)
{
    int state = storages[storage_id].storage.resource_state[resource_id];
    if (state == BUILDING_STORAGE_STATE_ACCEPTING) {
        state = BUILDING_STORAGE_STATE_NOT_ACCEPTING;
    } else if (state == BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
        state = BUILDING_STORAGE_STATE_GETTING;
    } else if (state == BUILDING_STORAGE_STATE_GETTING) {
        state = BUILDING_STORAGE_STATE_ACCEPTING;
    }
    storages[storage_id].storage.resource_state[resource_id] = state;
}

static void toggle_resource_state(int index, __attribute__((unused)) int param2)
{
    struct building_t *b = &all_buildings[distribution_data.building_id];
    int resource;
    if (b->type == BUILDING_WAREHOUSE) {
        resource = city_resource_get_available()->items[index - 1];
    } else {
        resource = city_resource_get_available_foods()->items[index - 1];
    }
    building_storage_cycle_resource_state(b->storage_id, resource);
    window_invalidate();
}

static struct generic_button_t orders_resource_buttons[] = {
    {0, 0, 210, 22, toggle_resource_state, button_none, 1, 0},
    {0, 22, 210, 22, toggle_resource_state, button_none, 2, 0},
    {0, 44, 210, 22, toggle_resource_state, button_none, 3, 0},
    {0, 66, 210, 22, toggle_resource_state, button_none, 4, 0},
    {0, 88, 210, 22, toggle_resource_state, button_none, 5, 0},
    {0, 110, 210, 22, toggle_resource_state, button_none, 6, 0},
    {0, 132, 210, 22, toggle_resource_state, button_none, 7, 0},
    {0, 154, 210, 22, toggle_resource_state, button_none, 8, 0},
    {0, 176, 210, 22, toggle_resource_state, button_none, 9, 0},
    {0, 198, 210, 22, toggle_resource_state, button_none, 10, 0},
    {0, 220, 210, 22, toggle_resource_state, button_none, 11, 0},
    {0, 242, 210, 22, toggle_resource_state, button_none, 12, 0},
    {0, 264, 210, 22, toggle_resource_state, button_none, 13, 0},
    {0, 286, 210, 22, toggle_resource_state, button_none, 14, 0},
    {0, 308, 210, 22, toggle_resource_state, button_none, 15, 0},
};

static void building_storage_accept_none(int storage_id)
{
    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
        storages[storage_id].storage.resource_state[r] = BUILDING_STORAGE_STATE_NOT_ACCEPTING;
    }
}

static void granary_orders(int index, __attribute__((unused)) int param2)
{
    int storage_id = all_buildings[distribution_data.building_id].storage_id;
    if (index == 0) {
        storages[storage_id].storage.empty_all = 1 - storages[storage_id].storage.empty_all;
    } else if (index == 1) {
        building_storage_accept_none(storage_id);
    }
    window_invalidate();
}

static struct generic_button_t granary_order_buttons[] = {
    {0, 0, 304, 20, granary_orders, button_none, 0, 0},
    {314, 0, 20, 20, granary_orders, button_none, 1, 0},
};

static void warehouse_orders(int index, __attribute__((unused)) int param2)
{
    if (index == 0) {
        int storage_id = all_buildings[distribution_data.building_id].storage_id;
        storages[storage_id].storage.empty_all = 1 - storages[storage_id].storage.empty_all;
    } else if (index == 1) {
        city_data.building.trade_center_building_id = distribution_data.building_id;
    } else if (index == 2) {
        int storage_id = all_buildings[distribution_data.building_id].storage_id;
        building_storage_accept_none(storage_id);
    }
    window_invalidate();
}

static struct generic_button_t warehouse_order_buttons[] = {
    {0, 0, 304, 20, warehouse_orders, button_none, 0, 0},
    {0, -22, 304, 20, warehouse_orders, button_none, 1, 0},
    {314, 0, 20, 20, warehouse_orders, button_none, 2, 0},
};

static void draw_accept_none_button(int x, int y, int focused)
{
    char refuse_button_text[] = { 'x', 0 };
    button_border_draw(x, y, 20, 20, focused ? 1 : 0);
    text_draw_centered(refuse_button_text, x + 1, y + 4, 20, FONT_NORMAL_BLACK, 0);
}

static void window_building_play_figure_phrase(struct building_info_context_t *c)
{
    int figure_id = c->figure.figure_ids[c->figure.selected_index];
    struct figure_t *f = &figures[figure_id];

    c->figure.sound_id = figure_properties[f->type].sound_category;
    c->figure.phrase_id = f->phrase_id;
    if (figure_properties[f->type].sound_category >= 0 && f->phrase_id >= 0) {
        char path[SOUND_FILENAME_MAX];
        string_copy("wavs/", path, SOUND_FILENAME_MAX - 1);
        strcat(path, FIGURE_SOUNDS[figure_properties[f->type].sound_category][f->phrase_id]);
        play_speech_file(path);
    }
}

static void select_figure(int index, __attribute__((unused)) int param2)
{
    building_figures_data.context_for_callback->figure.selected_index = index;
    window_building_play_figure_phrase(building_figures_data.context_for_callback);
    window_invalidate();
}

static struct generic_button_t figure_buttons[] = {
    {26, 46, 50, 50, select_figure, button_none, 0, 0},
    {86, 46, 50, 50, select_figure, button_none, 1, 0},
    {146, 46, 50, 50, select_figure, button_none, 2, 0},
    {206, 46, 50, 50, select_figure, button_none, 3, 0},
    {266, 46, 50, 50, select_figure, button_none, 4, 0},
    {326, 46, 50, 50, select_figure, button_none, 5, 0},
    {386, 46, 50, 50, select_figure, button_none, 6, 0},
};

static void window_building_play_sound(struct building_info_context_t *c, const char *sound_file)
{
    if (c->can_play_sound) {
        play_speech_file(sound_file);
        c->can_play_sound = 0;
    }
}

static void window_building_draw_description_at(struct building_info_context_t *c, int y_offset, int text_group, int text_id)
{
    lang_text_draw_multiline(text_group, text_id, c->x_offset + 32, c->y_offset + y_offset, BLOCK_SIZE * (c->width_blocks - 4), FONT_NORMAL_BLACK);
}

static void draw_employment_details(struct building_info_context_t *c, struct building_t *b, int y_offset, int text_id)
{
    y_offset += c->y_offset;
    image_draw(image_group(GROUP_CONTEXT_ICONS) + 14, c->x_offset + 40, y_offset + 6);
    if (text_id) {
        int width = lang_text_draw_amount(8, 12, b->num_workers,
            c->x_offset + 60, y_offset + 10, FONT_NORMAL_BROWN);
        width += text_draw_number(building_properties[b->type].n_laborers, '(', "",
            c->x_offset + 70 + width, y_offset + 10, FONT_NORMAL_BROWN);
        lang_text_draw(69, 0, c->x_offset + 70 + width, y_offset + 10, FONT_NORMAL_BROWN);
        lang_text_draw(69, text_id, c->x_offset + 70, y_offset + 26, FONT_NORMAL_BROWN);
    } else {
        int width = lang_text_draw_amount(8, 12, b->num_workers,
            c->x_offset + 60, y_offset + 16, FONT_NORMAL_BROWN);
        width += text_draw_number(building_properties[b->type].n_laborers, '(', "",
            c->x_offset + 70 + width, y_offset + 16, FONT_NORMAL_BROWN);
        lang_text_draw(69, 0, c->x_offset + 70 + width, y_offset + 16, FONT_NORMAL_BROWN);
    }
}

static int draw_employment_info(struct building_t *b, int consider_house_covering)
{
    int text_id;
    if (b->num_workers >= building_properties[b->type].n_laborers) {
        text_id = 0;
    } else if (city_data.population.population <= 0) {
        text_id = 16; // no people in city
    } else if (!consider_house_covering) {
        text_id = 19;
    } else if (b->houses_covered <= 0) {
        text_id = 17; // no employees nearby
    } else if (b->houses_covered < 40) {
        text_id = 20; // poor access to employees
    } else if (!city_data.labor.categories[b->labor_category].workers_allocated) {
        text_id = 18; // no people allocated
    } else {
        text_id = 19; // too few people allocated
    }
    if (!text_id && consider_house_covering && b->houses_covered < 40) {
        text_id = 20; // poor access to employees
    }
    return text_id;
}

static void window_building_draw_employment(struct building_info_context_t *c, int y_offset)
{
    struct building_t *b = &all_buildings[c->building_id];
    int text_id = draw_employment_info(b, 1);
    draw_employment_details(c, b, y_offset, text_id);
}

static void draw_farm(struct building_info_context_t *c, int help_id, const char *sound_file, int group_id, int resource)
{
    c->help_id = help_id;
    window_building_play_sound(c, sound_file);

    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    image_draw(resource_images[resource].icon_img_id, c->x_offset + 10, c->y_offset + 10);
    lang_text_draw_centered(group_id, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);

    struct building_t *b = &all_buildings[c->building_id];
    int pct_grown = calc_percentage(b->data.industry.progress, 200);
    int width = lang_text_draw(group_id, 2, c->x_offset + 32, c->y_offset + 44, FONT_NORMAL_BLACK);
    width += text_draw_percentage(pct_grown, c->x_offset + 32 + width, c->y_offset + 44, FONT_NORMAL_BLACK);
    lang_text_draw(group_id, 3, c->x_offset + 32 + width, c->y_offset + 44, FONT_NORMAL_BLACK);
    if (!c->has_road_access) {
        window_building_draw_description_at(c, 70, 69, 25);
    } else if (city_data.resource.mothballed[resource]) {
        window_building_draw_description_at(c, 70, group_id, 4);
    } else if (b->data.industry.curse_days_left > 4) {
        window_building_draw_description_at(c, 70, group_id, 11);
    } else if (b->num_workers <= 0) {
        window_building_draw_description_at(c, 70, group_id, 5);
    } else if (c->worker_percentage >= 100) {
        window_building_draw_description_at(c, 70, group_id, 6);
    } else if (c->worker_percentage >= 75) {
        window_building_draw_description_at(c, 70, group_id, 7);
    } else if (c->worker_percentage >= 50) {
        window_building_draw_description_at(c, 70, group_id, 8);
    } else if (c->worker_percentage >= 25) {
        window_building_draw_description_at(c, 70, group_id, 9);
    } else {
        window_building_draw_description_at(c, 70, group_id, 10);
    }
    inner_panel_draw(c->x_offset + 16, c->y_offset + 136, c->width_blocks - 2, 4);
    window_building_draw_employment(c, 142);
    window_building_draw_description_at(c, BLOCK_SIZE * c->height_blocks - 136, group_id, 1);
}

static void draw_raw_material(struct building_info_context_t *c, int help_id, const char *sound_file, int group_id, int resource)
{
    c->help_id = help_id;
    window_building_play_sound(c, sound_file);
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    image_draw(resource_images[resource].icon_img_id, c->x_offset + 10, c->y_offset + 10);
    lang_text_draw_centered(group_id, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    struct building_t *b = &all_buildings[c->building_id];
    int pct_done = calc_percentage(b->data.industry.progress, 200);
    int width = lang_text_draw(group_id, 2, c->x_offset + 32, c->y_offset + 44, FONT_NORMAL_BLACK);
    width += text_draw_percentage(pct_done, c->x_offset + 32 + width, c->y_offset + 44, FONT_NORMAL_BLACK);
    lang_text_draw(group_id, 3, c->x_offset + 32 + width, c->y_offset + 44, FONT_NORMAL_BLACK);
    if (!c->has_road_access) {
        window_building_draw_description_at(c, 70, 69, 25);
    } else if (city_data.resource.mothballed[resource]) {
        window_building_draw_description_at(c, 70, group_id, 4);
    } else if (b->num_workers <= 0) {
        window_building_draw_description_at(c, 70, group_id, 5);
    } else if (c->worker_percentage >= 100) {
        window_building_draw_description_at(c, 70, group_id, 6);
    } else if (c->worker_percentage >= 75) {
        window_building_draw_description_at(c, 70, group_id, 7);
    } else if (c->worker_percentage >= 50) {
        window_building_draw_description_at(c, 70, group_id, 8);
    } else if (c->worker_percentage >= 25) {
        window_building_draw_description_at(c, 70, group_id, 9);
    } else {
        window_building_draw_description_at(c, 70, group_id, 10);
    }
    inner_panel_draw(c->x_offset + 16, c->y_offset + 136, c->width_blocks - 2, 4);
    window_building_draw_employment(c, 142);
    window_building_draw_description_at(c, BLOCK_SIZE * c->height_blocks - 136, group_id, 1);
}

static void draw_workshop(struct building_info_context_t *c, int help_id, const char *sound_file, int group_id, int resource, int input_resource)
{
    c->help_id = help_id;
    window_building_play_sound(c, sound_file);

    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    image_draw(resource_images[resource].icon_img_id, c->x_offset + 10, c->y_offset + 10);
    lang_text_draw_centered(group_id, 0, c->x_offset, c->y_offset + 10,
        BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);

    struct building_t *b = &all_buildings[c->building_id];
    int pct_done = calc_percentage(b->data.industry.progress, 400);
    int width = lang_text_draw(group_id, 2, c->x_offset + 32, c->y_offset + 40, FONT_NORMAL_BLACK);
    width += text_draw_percentage(pct_done, c->x_offset + 32 + width, c->y_offset + 40, FONT_NORMAL_BLACK);
    lang_text_draw(group_id, 3, c->x_offset + 32 + width, c->y_offset + 40, FONT_NORMAL_BLACK);

    image_draw(resource_images[input_resource].icon_img_id, c->x_offset + 32, c->y_offset + 56);
    width = lang_text_draw(group_id, 12, c->x_offset + 60, c->y_offset + 60, FONT_NORMAL_BLACK);
    if (b->loads_stored < 1) {
        lang_text_draw_amount(8, 10, 0, c->x_offset + 60 + width, c->y_offset + 60, FONT_NORMAL_BLACK);
    } else {
        lang_text_draw_amount(8, 10, b->loads_stored, c->x_offset + 60 + width, c->y_offset + 60, FONT_NORMAL_BLACK);
    }
    if (!c->has_road_access) {
        window_building_draw_description_at(c, 86, 69, 25);
    } else if (city_data.resource.mothballed[resource]) {
        window_building_draw_description_at(c, 86, group_id, 4);
    } else if (b->num_workers <= 0) {
        window_building_draw_description_at(c, 86, group_id, 5);
    } else if (b->loads_stored <= 0) {
        window_building_draw_description_at(c, 86, group_id, 11);
    } else if (c->worker_percentage >= 100) {
        window_building_draw_description_at(c, 86, group_id, 6);
    } else if (c->worker_percentage >= 75) {
        window_building_draw_description_at(c, 86, group_id, 7);
    } else if (c->worker_percentage >= 50) {
        window_building_draw_description_at(c, 86, group_id, 8);
    } else if (c->worker_percentage >= 25) {
        window_building_draw_description_at(c, 86, group_id, 9);
    } else {
        window_building_draw_description_at(c, 86, group_id, 10);
    }
    inner_panel_draw(c->x_offset + 16, c->y_offset + 136, c->width_blocks - 2, 4);
    window_building_draw_employment(c, 142);
}

static void button_layout(int index, __attribute__((unused)) int param2)
{
    struct formation_t *m = &legion_formations[building_military_data.context_for_callback->formation_id];
    if (m->in_distant_battle || m->cursed_by_mars) {
        return;
    }

    if (m->figure_type == FIGURE_FORT_LEGIONARY) {
        switch (index) {
            case 0:
                if (m->has_military_training) {
                    m->layout = FORMATION_TORTOISE; break;
                } else {
                    return;
                }
            case 1: return;
            case 2: m->layout = FORMATION_DOUBLE_LINE_1; break;
            case 3: m->layout = FORMATION_DOUBLE_LINE_2; break;
            default: break;
        }
    } else {
        switch (index) {
            case 0: m->layout = FORMATION_SINGLE_LINE_1; break;
            case 1: m->layout = FORMATION_SINGLE_LINE_2; break;
            case 2: m->layout = FORMATION_DOUBLE_LINE_1; break;
            case 3: m->layout = FORMATION_DOUBLE_LINE_2; break;
            default: break;
        }
    }
    if (index == 4) { // mop up
        for (int i = 0; i < m->num_figures; i++) {
            struct figure_t *unit = &figures[m->figures[i]];
            unit->action_state = FIGURE_ACTION_SOLDIER_MOPPING_UP;
        }
    }
    switch (index) {
        case 0: play_speech_file("wavs/cohort1.wav"); break;
        case 1: play_speech_file("wavs/cohort2.wav"); break;
        case 2: play_speech_file("wavs/cohort3.wav"); break;
        case 3: play_speech_file("wavs/cohort4.wav"); break;
        case 4: play_speech_file("wavs/cohort5.wav"); break;
    }
    window_city_military_show(building_military_data.context_for_callback->formation_id);
}

static struct generic_button_t layout_buttons[] = {
    {19, 139, 84, 84, button_layout, button_none, 0, 0},
    {104, 139, 84, 84, button_layout, button_none, 1, 0},
    {189, 139, 84, 84, button_layout, button_none, 2, 0},
    {274, 139, 84, 84, button_layout, button_none, 3, 0},
    {359, 139, 84, 84, button_layout, button_none, 4, 0}
};

static void button_return_to_fort(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    struct formation_t *m = &legion_formations[building_military_data.context_for_callback->formation_id];
    if (!m->in_distant_battle && !m->is_at_rest) {
        return_legion_formation_home(m);
        window_city_show();
    }
}

static struct generic_button_t return_button[] = {
    {0, 0, 288, 32, button_return_to_fort, button_none, 0, 0},
};

static void button_help(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (b_info_context.help_id > 0) {
        window_message_dialog_show(b_info_context.help_id, window_city_draw_all);
    } else {
        window_message_dialog_show(MESSAGE_DIALOG_HELP, window_city_draw_all);
    }
    window_invalidate();
}

static void button_close(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (b_info_context.storage_show_special_orders) {
        b_info_context.storage_show_special_orders = 0;
        window_invalidate();
    } else {
        window_city_show();
    }
}

static struct image_button_t image_buttons_help_close[] = {
    {14, 0, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help, button_none, 0, 0, 1, 0, 0, 0},
    {424, 3, 24, 24, IB_NORMAL, GROUP_CONTEXT_ICONS, 4, button_close, button_none, 0, 0, 1, 0, 0, 0}
};

static void button_advisor(int advisor, __attribute__((unused)) int param2)
{
    window_advisors_show(advisor);
}

static struct image_button_t image_buttons_advisor[] = {
    {350, -38, 28, 28, IB_NORMAL, GROUP_MESSAGE_ADVISOR_BUTTONS, 9, button_advisor, button_none, ADVISOR_RATINGS, 0, 1, 0, 0, 0}
};

static int get_height_id(void)
{
    if (b_info_context.type == BUILDING_INFO_TERRAIN) {
        switch (b_info_context.terrain_type) {
            case TERRAIN_INFO_AQUEDUCT:
                return 4;
            case TERRAIN_INFO_RUBBLE:
            case TERRAIN_INFO_WALL:
            case TERRAIN_INFO_GARDEN:
                return 1;
            default:
                return 5;
        }
    } else if (b_info_context.type == BUILDING_INFO_BUILDING) {
        const struct building_t *b = &all_buildings[b_info_context.building_id];
        if (building_is_house(b->type) && b->house_population <= 0) {
            return 5;
        }
        switch (b->type) {
            case BUILDING_SMALL_TEMPLE_CERES:
            case BUILDING_SMALL_TEMPLE_NEPTUNE:
            case BUILDING_SMALL_TEMPLE_MERCURY:
            case BUILDING_SMALL_TEMPLE_MARS:
            case BUILDING_SMALL_TEMPLE_VENUS:
            case BUILDING_LARGE_TEMPLE_CERES:
            case BUILDING_LARGE_TEMPLE_NEPTUNE:
            case BUILDING_LARGE_TEMPLE_MERCURY:
            case BUILDING_LARGE_TEMPLE_MARS:
            case BUILDING_LARGE_TEMPLE_VENUS:
            case BUILDING_ORACLE:
            case BUILDING_SMALL_STATUE:
            case BUILDING_MEDIUM_STATUE:
            case BUILDING_LARGE_STATUE:
            case BUILDING_GLADIATOR_SCHOOL:
            case BUILDING_LION_HOUSE:
            case BUILDING_ACTOR_COLONY:
            case BUILDING_CHARIOT_MAKER:
            case BUILDING_DOCTOR:
            case BUILDING_HOSPITAL:
            case BUILDING_BATHHOUSE:
            case BUILDING_BARBER:
            case BUILDING_BURNING_RUIN:
            case BUILDING_RESERVOIR:
            case BUILDING_NATIVE_HUT:
            case BUILDING_NATIVE_MEETING:
            case BUILDING_NATIVE_CROPS:
            case BUILDING_MISSION_POST:
            case BUILDING_PREFECTURE:
            case BUILDING_ENGINEERS_POST:
            case BUILDING_SCHOOL:
            case BUILDING_ACADEMY:
            case BUILDING_LIBRARY:
            case BUILDING_GATEHOUSE:
            case BUILDING_TOWER:
            case BUILDING_FORT_LEGIONARIES:
            case BUILDING_FORT_JAVELIN:
            case BUILDING_FORT_MOUNTED:
            case BUILDING_MILITARY_ACADEMY:
            case BUILDING_BARRACKS:
            case BUILDING_MARKET:
            case BUILDING_GRANARY:
            case BUILDING_SHIPYARD:
            case BUILDING_DOCK:
            case BUILDING_WHARF:
            case BUILDING_GOVERNORS_HOUSE:
            case BUILDING_GOVERNORS_VILLA:
            case BUILDING_GOVERNORS_PALACE:
            case BUILDING_FORUM:
            case BUILDING_WINE_WORKSHOP:
            case BUILDING_OIL_WORKSHOP:
            case BUILDING_WEAPONS_WORKSHOP:
            case BUILDING_FURNITURE_WORKSHOP:
            case BUILDING_POTTERY_WORKSHOP:
                return 1;
            case BUILDING_THEATER:
            case BUILDING_HIPPODROME:
            case BUILDING_COLOSSEUM:
            case BUILDING_SENATE:
            case BUILDING_FOUNTAIN:
                return 2;
            case BUILDING_AMPHITHEATER:
                return 3;
            case BUILDING_WELL:
                return 4;
            default:
                return 0;
        }
    }
    return 0;
}

static int trade_caravan_phrase(struct figure_t *f)
{
    if (++f->phrase_sequence_exact >= 2) {
        f->phrase_sequence_exact = 0;
    }
    if (f->action_state == FIGURE_ACTION_TRADE_CARAVAN_LEAVING) {
        if (!trader_has_traded(f->trader_id)) {
            return 7; // no trade
        }
    } else if (f->action_state == FIGURE_ACTION_TRADE_CARAVAN_TRADING) {
        if (figure_trade_caravan_can_buy(f, f->destination_building_id, f->empire_city_id)) {
            return 11; // buying goods
        } else if (figure_trade_caravan_can_sell(f, f->destination_building_id, f->empire_city_id)) {
            return 10; // selling goods
        }
    }
    return 8 + f->phrase_sequence_exact;
}

static void window_building_draw_figure_list(struct building_info_context_t *c)
{
    inner_panel_draw(c->x_offset + 16, c->y_offset + 40, c->width_blocks - 2, 13);
    if (c->figure.count <= 0) {
        lang_text_draw_centered(70, 0, c->x_offset, c->y_offset + 120,
            BLOCK_SIZE * c->width_blocks, FONT_NORMAL_BROWN);
    } else {
        for (int i = 0; i < c->figure.count; i++) {
            button_border_draw(c->x_offset + 60 * i + 25, c->y_offset + 45, 52, 52, i == c->figure.selected_index);
            graphics_draw_from_buffer(c->x_offset + 27 + 60 * i, c->y_offset + 47, 48, 48, building_figures_data.figure_images[i]);
        }
        button_border_draw(c->x_offset + 24, c->y_offset + 102, BLOCK_SIZE * (c->width_blocks - 3), 138, 0);
        struct figure_t *f = &figures[c->figure.figure_ids[c->figure.selected_index]];
        if (figure_properties[f->type].is_empire_trader) {
            while (f->type == FIGURE_TRADE_CARAVAN_DONKEY) {
                f = &figures[f->leading_figure_id];
            }
            lang_text_draw(65, f->name_id, c->x_offset + 40, c->y_offset + 110, FONT_NORMAL_BROWN);
            int width = text_draw(figure_desc_strings[f->type], c->x_offset + 40, c->y_offset + 130, FONT_NORMAL_BROWN, COLOR_BLACK);
            lang_text_draw(21, empire_objects[f->empire_city_id].city_name_id, c->x_offset + 40 + width, c->y_offset + 130, FONT_NORMAL_BROWN);

            width = lang_text_draw(129, 1, c->x_offset + 40, c->y_offset + 150, FONT_NORMAL_BROWN);
            lang_text_draw_amount(8, 10, f->type == FIGURE_TRADE_SHIP ? 12 : 8, c->x_offset + 40 + width, c->y_offset + 150, FONT_NORMAL_BROWN);

            int trader_id = f->trader_id;
            int text_id;
            if (f->type == FIGURE_TRADE_SHIP) {
                switch (f->action_state) {
                    case FIGURE_ACTION_TRADE_SHIP_ANCHORED: text_id = 6; break;
                    case FIGURE_ACTION_TRADE_SHIP_MOORED: text_id = 7; break;
                    case FIGURE_ACTION_TRADE_SHIP_LEAVING: text_id = 8; break;
                    default: text_id = 9; break;
                }
            } else {
                switch (f->action_state) {
                    case FIGURE_ACTION_TRADE_CARAVAN_ARRIVING:
                        text_id = 12;
                        break;
                    case FIGURE_ACTION_TRADE_CARAVAN_TRADING:
                        text_id = 10;
                        break;
                    case FIGURE_ACTION_TRADE_CARAVAN_LEAVING:
                        if (trader_has_traded(trader_id)) {
                            text_id = 11;
                        } else {
                            text_id = 13;
                        }
                        break;
                    default:
                        text_id = 11;
                        break;
                }
            }
            lang_text_draw(129, text_id, c->x_offset + 40, c->y_offset + 170, FONT_NORMAL_BROWN);
            if (trader_has_traded(trader_id)) {
                // bought
                int y_base = c->y_offset + 192;
                width = lang_text_draw(129, 4, c->x_offset + 40, y_base, FONT_NORMAL_BROWN);
                for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                    if (trader_bought_resources(trader_id, r)) {
                        width += text_draw_number(trader_bought_resources(trader_id, r), ' ', "", c->x_offset + 40 + width, y_base, FONT_NORMAL_BROWN);
                        image_draw(resource_images[r].icon_img_id + resource_image_offset(r, RESOURCE_IMAGE_ICON), c->x_offset + 40 + width, y_base - 3);
                        width += 25;
                    }
                }
                // sold
                y_base = c->y_offset + 213;
                width = lang_text_draw(129, 5, c->x_offset + 40, y_base, FONT_NORMAL_BROWN);
                for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                    if (trader_sold_resources(trader_id, r)) {
                        width += text_draw_number(trader_sold_resources(trader_id, r), ' ', "", c->x_offset + 40 + width, y_base, FONT_NORMAL_BROWN);
                        image_draw(resource_images[r].icon_img_id + resource_image_offset(r, RESOURCE_IMAGE_ICON), c->x_offset + 40 + width, y_base - 3);
                        width += 25;
                    }
                }
            } else { // nothing sold/bought (yet)
                // buying
                int y_base = c->y_offset + 192;
                width = lang_text_draw(129, 2, c->x_offset + 40, y_base, FONT_NORMAL_BROWN);
                for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                    if (empire_objects[f->empire_city_id].resource_buy_limit[r]) {
                        image_draw(resource_images[r].icon_img_id + resource_image_offset(r, RESOURCE_IMAGE_ICON), c->x_offset + 40 + width, y_base - 3);
                        width += 25;
                    }
                }
                // selling
                y_base = c->y_offset + 213;
                width = lang_text_draw(129, 3, c->x_offset + 40, y_base, FONT_NORMAL_BROWN);
                for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                    if (empire_objects[f->empire_city_id].resource_sell_limit[r]) {
                        image_draw(resource_images[r].icon_img_id + resource_image_offset(r, RESOURCE_IMAGE_ICON), c->x_offset + 40 + width, y_base - 3);
                        width += 25;
                    }
                }
            }
        } else if (figure_properties[f->type].is_enemy_unit) {
            image_draw(image_group(GROUP_BIG_PEOPLE) + figure_properties[f->type].big_img_id - 1, c->x_offset + 28, c->y_offset + 112);

            lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);

            switch (f->type) {
                case FIGURE_ENEMY_BARBARIAN_SWORDSMAN:
                    text_draw(enemy_desc_strings[0], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN:
                case FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT:
                    text_draw(enemy_desc_strings[1], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_BRITON_SWORDSMAN:
                case FIGURE_ENEMY_BRITON_CHARIOT:
                    text_draw(enemy_desc_strings[2], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_CELT_SWORDSMAN:
                case FIGURE_ENEMY_CELT_CHARIOT:
                    text_draw(enemy_desc_strings[3], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_PICT_SWORDSMAN:
                case FIGURE_ENEMY_PICT_CHARIOT:
                    text_draw(enemy_desc_strings[4], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_EGYPTIAN_SWORDSMAN:
                case FIGURE_ENEMY_EGYPTIAN_CAMEL:
                    text_draw(enemy_desc_strings[5], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_ETRUSCAN_SWORDSMAN:
                case FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER:
                    text_draw(enemy_desc_strings[6], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_SAMNITE_SWORDSMAN:
                case FIGURE_ENEMY_SAMNITE_SPEAR_THROWER:
                    text_draw(enemy_desc_strings[7], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_GAUL_SWORDSMAN:
                case FIGURE_ENEMY_GAUL_AXEMAN:
                    text_draw(enemy_desc_strings[8], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_HELVETIUS_SWORDSMAN:
                case FIGURE_ENEMY_HELVETIUS_AXEMAN:
                    text_draw(enemy_desc_strings[9], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_HUN_SWORDSMAN:
                case FIGURE_ENEMY_HUN_MOUNTED_ARCHER:
                    text_draw(enemy_desc_strings[10], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_GOTH_SWORDSMAN:
                case FIGURE_ENEMY_GOTH_MOUNTED_ARCHER:
                    text_draw(enemy_desc_strings[11], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_VISIGOTH_SWORDSMAN:
                case FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER:
                    text_draw(enemy_desc_strings[12], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_GREEK_SWORDSMAN:
                case FIGURE_ENEMY_GREEK_SPEAR_THROWER:
                    text_draw(enemy_desc_strings[13], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_MACEDONIAN_SWORDSMAN:
                case FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER:
                    text_draw(enemy_desc_strings[14], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_NUMIDIAN_SWORDSMAN:
                case FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER:
                    text_draw(enemy_desc_strings[15], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_PERGAMUM_SWORDSMAN:
                case FIGURE_ENEMY_PERGAMUM_ARCHER:
                    text_draw(enemy_desc_strings[16], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_IBERIAN_SWORDSMAN:
                case FIGURE_ENEMY_IBERIAN_SPEAR_THROWER:
                    text_draw(enemy_desc_strings[17], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_JUDEAN_SWORDSMAN:
                case FIGURE_ENEMY_JUDEAN_SPEAR_THROWER:
                    text_draw(enemy_desc_strings[18], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
                case FIGURE_ENEMY_SELEUCID_SWORDSMAN:
                case FIGURE_ENEMY_SELEUCID_SPEAR_THROWER:
                    text_draw(enemy_desc_strings[19], c->x_offset + 92, c->y_offset + 149, FONT_NORMAL_BROWN, COLOR_BLACK);
                    break;
            }
        } else if (f->type == FIGURE_SHIPWRECK || figure_properties[f->type].is_herd_animal) {
            image_draw(image_group(GROUP_BIG_PEOPLE) + figure_properties[f->type].big_img_id - 1, c->x_offset + 28, c->y_offset + 112);
            text_draw(figure_desc_strings[f->type], c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);
        } else if (f->type == FIGURE_CART_PUSHER || f->type == FIGURE_WAREHOUSEMAN || f->type == FIGURE_DOCKER) {
            image_draw(image_group(GROUP_BIG_PEOPLE) + figure_properties[f->type].big_img_id - 1, c->x_offset + 28, c->y_offset + 112);
            lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);
            int width = text_draw(figure_desc_strings[f->type], c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);
            if (f->action_state != FIGURE_ACTION_DOCKER_IDLING && f->resource_id) {
                int resource = f->resource_id;
                image_draw(resource_images[resource].icon_img_id + resource_image_offset(resource, RESOURCE_IMAGE_ICON),
                    c->x_offset + 92 + width, c->y_offset + 135);
            }
            int phrase_height = lang_text_draw_multiline(130, 21 * c->figure.sound_id + c->figure.phrase_id + 1,
                c->x_offset + 90, c->y_offset + 160, BLOCK_SIZE * (c->width_blocks - 8), FONT_NORMAL_BROWN);

            if (!f->building_id) {
                return;
            }
            int is_returning = 0;
            switch (f->action_state) {
                case FIGURE_ACTION_CARTPUSHER_RETURNING:
                case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_EMPTY:
                case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_FOOD:
                case FIGURE_ACTION_WAREHOUSEMAN_RETURNING_WITH_RESOURCE:
                case FIGURE_ACTION_DOCKER_EXPORT_QUEUE:
                case FIGURE_ACTION_DOCKER_EXPORT_RETURNING:
                case FIGURE_ACTION_DOCKER_IMPORT_RETURNING:
                    is_returning = 1;
                    break;
            }
            if (f->action_state != FIGURE_ACTION_DOCKER_IDLING) {
                int x_base = c->x_offset + 40;
                int y_base = c->y_offset + 216;
                if (phrase_height > 60) {
                    y_base += 8;
                }
                struct building_t *source_building = &all_buildings[f->building_id];
                struct building_t *target_building = &all_buildings[f->destination_building_id];
                if (is_returning) {
                    width = lang_text_draw(129, 16, x_base, y_base, FONT_NORMAL_BROWN);
                    width += text_draw(all_buildings_strings[source_building->type], x_base + width, y_base, FONT_NORMAL_BROWN, COLOR_BLACK);
                    width += lang_text_draw(129, 14, x_base + width, y_base, FONT_NORMAL_BROWN);
                    text_draw(all_buildings_strings[target_building->type], x_base + width, y_base, FONT_NORMAL_BROWN, COLOR_BLACK);
                } else {
                    width = lang_text_draw(129, 15, x_base, y_base, FONT_NORMAL_BROWN);
                    width += text_draw(all_buildings_strings[target_building->type], x_base + width, y_base, FONT_NORMAL_BROWN, COLOR_BLACK);
                    width += lang_text_draw(129, 14, x_base + width, y_base, FONT_NORMAL_BROWN);
                    text_draw(all_buildings_strings[source_building->type], x_base + width, y_base, FONT_NORMAL_BROWN, COLOR_BLACK);
                }
            }
        } else if (f->type == FIGURE_MARKET_BUYER) {
            image_draw(image_group(GROUP_BIG_PEOPLE) + figure_properties[f->type].big_img_id - 1, c->x_offset + 28, c->y_offset + 112);

            lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);
            int width = text_draw(figure_desc_strings[f->type], c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);

            if (f->action_state == FIGURE_ACTION_MARKET_BUYER_GOING_TO_STORAGE) {
                width += lang_text_draw(129, 17, c->x_offset + 90 + width, c->y_offset + 139, FONT_NORMAL_BROWN);
                image_draw(resource_images[f->collecting_item_id + 1].icon_img_id + resource_image_offset(f->collecting_item_id + 1, RESOURCE_IMAGE_ICON),
                    c->x_offset + 90 + width, c->y_offset + 135);
            } else if (f->action_state == FIGURE_ACTION_MARKET_BUYER_RETURNING) {
                width += lang_text_draw(129, 18, c->x_offset + 90 + width, c->y_offset + 139, FONT_NORMAL_BROWN);
                image_draw(resource_images[f->collecting_item_id + 1].icon_img_id + resource_image_offset(f->collecting_item_id + 1, RESOURCE_IMAGE_ICON),
                    c->x_offset + 90 + width, c->y_offset + 135);
            }
            if (c->figure.phrase_id >= 0) {
                lang_text_draw_multiline(130, 21 * c->figure.sound_id + c->figure.phrase_id + 1,
                    c->x_offset + 90, c->y_offset + 160, BLOCK_SIZE * (c->width_blocks - 8), FONT_NORMAL_BROWN);
            }
        } else {
            int image_id = image_group(GROUP_BIG_PEOPLE) + figure_properties[f->type].big_img_id - 1;
            if (f->action_state == FIGURE_ACTION_PREFECT_GOING_TO_FIRE ||
                f->action_state == FIGURE_ACTION_PREFECT_AT_FIRE) {
                image_id = image_group(GROUP_BIG_PEOPLE) + 18;
            }
            image_draw(image_id, c->x_offset + 28, c->y_offset + 112);

            lang_text_draw(65, f->name_id, c->x_offset + 90, c->y_offset + 108, FONT_LARGE_BROWN);
            if (figure_properties[f->type].is_caesar_legion_unit) {
                text_draw(enemy_desc_strings[20], c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);
            } else {
                text_draw(figure_desc_strings[f->type], c->x_offset + 92, c->y_offset + 139, FONT_NORMAL_BROWN, COLOR_BLACK);
            }
            if (c->figure.phrase_id >= 0) {
                lang_text_draw_multiline(130, 21 * c->figure.sound_id + c->figure.phrase_id + 1,
                    c->x_offset + 90, c->y_offset + 160, BLOCK_SIZE * (c->width_blocks - 8), FONT_NORMAL_BROWN);
            }
        }
    }
    c->figure.drawn = 1;
}

static void window_building_prepare_figure_list(struct building_info_context_t *c)
{
    if (c->figure.count > 0) {
        struct pixel_coordinate_t coord = { 0, 0 };
        for (int i = 0; i < c->figure.count; i++) {
            int x_cam, y_cam;
            city_view_get_camera(&x_cam, &y_cam);
            int x, y;
            city_view_grid_offset_to_xy_view(figures[c->figure.figure_ids[i]].grid_offset, &x, &y);
            city_view_set_camera(x - 2, y - 6);
            widget_city_draw_for_figure(c->figure.figure_ids[i], &coord);
            city_view_set_camera(x_cam, y_cam);
            graphics_save_to_buffer(coord.x, coord.y, 48, 48, building_figures_data.figure_images[i]);
        }
        widget_city_draw();
    }
}

static void window_building_draw_employment_without_house_cover(struct building_info_context_t *c, int y_offset)
{
    struct building_t *b = &all_buildings[c->building_id];
    int text_id = draw_employment_info(b, 0);
    draw_employment_details(c, b, y_offset, text_id);
}

static int window_building_get_vertical_offset(struct building_info_context_t *c, int new_window_height)
{
    new_window_height = new_window_height * BLOCK_SIZE;
    int old_window_height = c->height_blocks * BLOCK_SIZE;
    int y_offset = c->y_offset;

    int center = (old_window_height / 2) + y_offset;
    int new_window_y = center - (new_window_height / 2);

    if (new_window_y < MIN_Y_POSITION) {
        new_window_y = MIN_Y_POSITION;
    } else {
        int height = screen_height() - MARGIN_POSITION;

        if (new_window_y + new_window_height > height) {
            new_window_y = height - new_window_height;
        }
    }

    return new_window_y;
}

static void draw_native(struct building_info_context_t *c, int group_id)
{
    c->help_id = 0;
    window_building_play_sound(c, "wavs/empty_land.wav");
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(group_id, 0, c->x_offset, c->y_offset + 10,
        BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    window_building_draw_description_at(c, 106, group_id, 1);
}

static void window_building_draw_description(struct building_info_context_t *c, int text_group, int text_id)
{
    lang_text_draw_multiline(text_group, text_id, c->x_offset + 32, c->y_offset + 56, BLOCK_SIZE * (c->width_blocks - 4), FONT_NORMAL_BLACK);
}

static void draw_entertainment_school(struct building_info_context_t *c, const char *sound_file, int group_id)
{
    c->help_id = 75;
    window_building_play_sound(c, sound_file);

    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(group_id, 0, c->x_offset, c->y_offset + 10,
        BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    if (!c->has_road_access) {
        window_building_draw_description(c, 69, 25);
    } else if (all_buildings[c->building_id].num_workers <= 0) {
        window_building_draw_description(c, group_id, 7);
    } else if (c->worker_percentage >= 100) {
        window_building_draw_description(c, group_id, 2);
    } else if (c->worker_percentage >= 75) {
        window_building_draw_description(c, group_id, 3);
    } else if (c->worker_percentage >= 50) {
        window_building_draw_description(c, group_id, 4);
    } else if (c->worker_percentage >= 25) {
        window_building_draw_description(c, group_id, 5);
    } else {
        window_building_draw_description(c, group_id, 6);
    }
    inner_panel_draw(c->x_offset + 16, c->y_offset + 136, c->width_blocks - 2, 4);
    window_building_draw_employment(c, 142);
}

static void draw_temple(struct building_info_context_t *c, const char *sound_file, int group_id, int image_offset)
{
    c->help_id = 67;
    window_building_play_sound(c, sound_file);
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(group_id, 0, c->x_offset, c->y_offset + 12,
        BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    inner_panel_draw(c->x_offset + 16, c->y_offset + 56, c->width_blocks - 2, 4);
    window_building_draw_employment(c, 62);
    if (c->has_road_access) {
        image_draw(image_offset + image_group(GROUP_PANEL_WINDOWS),
            c->x_offset + 190, c->y_offset + BLOCK_SIZE * c->height_blocks - 118);
    } else {
        window_building_draw_description_at(c, BLOCK_SIZE * c->height_blocks - 128, 69, 25);
    }
}

static void draw_culture_info(struct building_info_context_t *c, int help_id, const char *sound_file, int group_id)
{
    c->help_id = help_id;
    window_building_play_sound(c, sound_file);
    outer_panel_draw(c->x_offset, c->y_offset, c->width_blocks, c->height_blocks);
    lang_text_draw_centered(group_id, 0, c->x_offset, c->y_offset + 10, BLOCK_SIZE * c->width_blocks, FONT_LARGE_BLACK);
    if (!c->has_road_access) {
        window_building_draw_description(c, 69, 25);
    } else if (all_buildings[c->building_id].num_workers <= 0) {
        window_building_draw_description(c, group_id, 2);
    } else {
        window_building_draw_description(c, group_id, 3);
    }
    inner_panel_draw(c->x_offset + 16, c->y_offset + 136, c->width_blocks - 2, 4);
    window_building_draw_employment(c, 142);
}

static void draw_background_building_info(void)
{
    window_city_draw_background();
    widget_city_draw();
    if (b_info_context.type == BUILDING_INFO_NONE) {
        outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
        lang_text_draw_centered(70, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
    } else if (b_info_context.type == BUILDING_INFO_TERRAIN) {
        switch (b_info_context.terrain_type) {
            case TERRAIN_INFO_ROAD: b_info_context.help_id = 57; break;
            case TERRAIN_INFO_AQUEDUCT: b_info_context.help_id = 60; break;
            case TERRAIN_INFO_WALL: b_info_context.help_id = 85; break;
            case TERRAIN_INFO_BRIDGE: b_info_context.help_id = 58; break;
            default: b_info_context.help_id = 0; break;
        }

        if (b_info_context.terrain_type == TERRAIN_INFO_AQUEDUCT) {
            b_info_context.help_id = 60;
            window_building_play_sound(&b_info_context, "wavs/aquaduct.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(141, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 144, 141, b_info_context.aqueduct_has_water ? 1 : 2);
        } else if (b_info_context.terrain_type == TERRAIN_INFO_RUBBLE) {
            b_info_context.help_id = 0;
            window_building_play_sound(&b_info_context, "wavs/ruin.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(140, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);

            lang_text_draw(41, b_info_context.rubble_building_type,
                b_info_context.x_offset + 32, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 173, FONT_NORMAL_BLACK);
            lang_text_draw_multiline(140, 1, b_info_context.x_offset + 32, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 143,
                BLOCK_SIZE * (b_info_context.width_blocks - 4), FONT_NORMAL_BLACK);
        } else if (b_info_context.terrain_type == TERRAIN_INFO_WALL) {
            b_info_context.help_id = 85;
            window_building_play_sound(&b_info_context, "wavs/wall.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(139, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 158, 139, 1);
        } else if (b_info_context.terrain_type == TERRAIN_INFO_GARDEN) {
            b_info_context.help_id = 80;
            window_building_play_sound(&b_info_context, "wavs/park.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(79, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 158, 79, 1);
        } else if (b_info_context.terrain_type == TERRAIN_INFO_PLAZA && b_info_context.figure.count <= 0) {
            b_info_context.help_id = 80;
            window_building_play_sound(&b_info_context, "wavs/plaza.wav");
            window_building_prepare_figure_list(&b_info_context);
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(137, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            window_building_draw_figure_list(&b_info_context);
            window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 113, 137, 1);
        } else {
            if (b_info_context.can_play_sound) {
                b_info_context.can_play_sound = 0;
                if (b_info_context.figure.count > 0) {
                    window_building_play_figure_phrase(&b_info_context);
                } else {
                    play_speech_file("wavs/empty_land.wav");
                }
            }
            if (b_info_context.figure.count > 0 && b_info_context.figure.figure_ids[b_info_context.figure.selected_index]) {
                struct figure_t *f = &figures[b_info_context.figure.figure_ids[b_info_context.figure.selected_index]];
                if (f->type < FIGURE_SHIPWRECK) {
                    b_info_context.help_id = 42;
                } else {
                    b_info_context.help_id = 330;
                }
            }

            window_building_prepare_figure_list(&b_info_context);
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            if (!b_info_context.figure.count) {
                lang_text_draw_centered(70, b_info_context.terrain_type + 10, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            }
            if (b_info_context.terrain_type != TERRAIN_INFO_ROAD && b_info_context.terrain_type != TERRAIN_INFO_PLAZA) {
                lang_text_draw_multiline(70, b_info_context.terrain_type + 25, b_info_context.x_offset + 40, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 113,
                    BLOCK_SIZE * (b_info_context.width_blocks - 4), FONT_NORMAL_BLACK);
            }
            if (b_info_context.figure.count) {
                window_building_draw_figure_list(&b_info_context);
            }
        }
    } else if (b_info_context.type == BUILDING_INFO_BUILDING) {
        int btype = all_buildings[b_info_context.building_id].type;
        if (building_is_house(btype)) {
            b_info_context.help_id = 56;
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (b->house_population == 0) {
                window_building_play_sound(&b_info_context, "wavs/empty_land.wav");
            } else {
                window_building_play_sound(&b_info_context, "wavs/housing.wav");
            }
            if (b->house_population <= 0) {
                window_building_prepare_figure_list(&b_info_context);
                outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
                lang_text_draw_centered(128, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
                window_building_draw_figure_list(&b_info_context);

                int text_id = 2;
                b = &all_buildings[b_info_context.building_id];
                if (map_closest_road_within_radius(b->x, b->y, 1, 2, 0, 0)) {
                    text_id = 1;
                }
                window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 113, 128, text_id);
                return;
            }
            int level = b->type - 2;
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(29, level, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 148, b_info_context.width_blocks - 2, 10);

            // draw population info
            b = &all_buildings[b_info_context.building_id];
            image_draw(image_group(GROUP_CONTEXT_ICONS) + 13, b_info_context.x_offset + 34, b_info_context.y_offset + 154 + 4);
            int width = text_draw_number(b->house_population, '@', " ", b_info_context.x_offset + 50, b_info_context.y_offset + 154 + 14, FONT_NORMAL_BROWN);
            width += lang_text_draw(127, 20, b_info_context.x_offset + 50 + width, b_info_context.y_offset + 154 + 14, FONT_NORMAL_BROWN);

            if (b->house_population_room < 0) {
                width += text_draw_number(-b->house_population_room, '@', " ",
                    b_info_context.x_offset + 50 + width, b_info_context.y_offset + 154 + 14, FONT_NORMAL_BROWN);
                lang_text_draw(127, 21, b_info_context.x_offset + 50 + width, b_info_context.y_offset + 154 + 14, FONT_NORMAL_BROWN);
            } else if (b->house_population_room > 0) {
                width += lang_text_draw(127, 22, b_info_context.x_offset + 50 + width, b_info_context.y_offset + 154 + 14, FONT_NORMAL_BROWN);
                text_draw_number(b->house_population_room, '@', " ",
                    b_info_context.x_offset + 50 + width, b_info_context.y_offset + 154 + 14, FONT_NORMAL_BROWN);
            }

            // draw tax info
            b = &all_buildings[b_info_context.building_id];
            if (b->house_tax_coverage) {
                width = lang_text_draw(127, 24, b_info_context.x_offset + 36, b_info_context.y_offset + 194, FONT_NORMAL_BROWN);
                width += lang_text_draw_amount(8, 0, calc_adjust_with_percentage(b->tax_income_or_storage / 2, city_data.finance.tax_percentage), b_info_context.x_offset + 36 + width, b_info_context.y_offset + 194, FONT_NORMAL_BROWN);
                lang_text_draw(127, 25, b_info_context.x_offset + 36 + width, b_info_context.y_offset + 194, FONT_NORMAL_BROWN);
            } else {
                lang_text_draw(127, 23, b_info_context.x_offset + 36, b_info_context.y_offset + 194, FONT_NORMAL_BROWN);
            }

            // draw happiness info
            int happiness = all_buildings[b_info_context.building_id].sentiment.house_happiness;
            int text_id;
            if (happiness >= 50) {
                text_id = 26;
            } else if (happiness >= 40) {
                text_id = 27;
            } else if (happiness >= 30) {
                text_id = 28;
            } else if (happiness >= 20) {
                text_id = 29;
            } else if (happiness >= 10) {
                text_id = 30;
            } else if (happiness >= 1) {
                text_id = 31;
            } else {
                text_id = 32;
            }
            lang_text_draw(127, text_id, b_info_context.x_offset + 36, b_info_context.y_offset + 214, FONT_NORMAL_BROWN);

            // food inventory
            if (house_properties[b->subtype.house_level].food_types) {
                // wheat
                image_draw(resource_images[RESOURCE_WHEAT].icon_img_id, b_info_context.x_offset + 32, b_info_context.y_offset + 234);
                text_draw_number(b->data.house.inventory[INVENTORY_WHEAT], '@', " ",
                    b_info_context.x_offset + 64, b_info_context.y_offset + 238, FONT_NORMAL_BROWN);
                // vegetables
                image_draw(resource_images[RESOURCE_VEGETABLES].icon_img_id, b_info_context.x_offset + 142, b_info_context.y_offset + 234);
                text_draw_number(b->data.house.inventory[INVENTORY_VEGETABLES], '@', " ",
                    b_info_context.x_offset + 174, b_info_context.y_offset + 238, FONT_NORMAL_BROWN);
                // fruit
                image_draw(resource_images[RESOURCE_FRUIT].icon_img_id, b_info_context.x_offset + 252, b_info_context.y_offset + 234);
                text_draw_number(b->data.house.inventory[INVENTORY_FRUIT], '@', " ",
                    b_info_context.x_offset + 284, b_info_context.y_offset + 238, FONT_NORMAL_BROWN);
                // meat/fish
                image_draw(resource_images[RESOURCE_MEAT].icon_img_id + resource_image_offset(RESOURCE_MEAT, RESOURCE_IMAGE_ICON),
                    b_info_context.x_offset + 362, b_info_context.y_offset + 234);
                text_draw_number(b->data.house.inventory[INVENTORY_MEAT], '@', " ",
                    b_info_context.x_offset + 394, b_info_context.y_offset + 238, FONT_NORMAL_BROWN);
            } else {
                // no food necessary
                lang_text_draw_multiline(127, 33, b_info_context.x_offset + 36, b_info_context.y_offset + 234,
                    BLOCK_SIZE * (b_info_context.width_blocks - 6), FONT_NORMAL_BROWN);
            }
            // goods inventory
            // pottery
            image_draw(resource_images[RESOURCE_POTTERY].icon_img_id, b_info_context.x_offset + 32, b_info_context.y_offset + 274);
            text_draw_number(b->data.house.inventory[INVENTORY_POTTERY], '@', " ",
                b_info_context.x_offset + 64, b_info_context.y_offset + 278, FONT_NORMAL_BROWN);
            // furniture
            image_draw(resource_images[RESOURCE_FURNITURE].icon_img_id, b_info_context.x_offset + 142, b_info_context.y_offset + 274);
            text_draw_number(b->data.house.inventory[INVENTORY_FURNITURE], '@', " ",
                b_info_context.x_offset + 174, b_info_context.y_offset + 278, FONT_NORMAL_BROWN);
            // oil
            image_draw(resource_images[RESOURCE_OIL].icon_img_id, b_info_context.x_offset + 252, b_info_context.y_offset + 274);
            text_draw_number(b->data.house.inventory[INVENTORY_OIL], '@', " ",
                b_info_context.x_offset + 284, b_info_context.y_offset + 278, FONT_NORMAL_BROWN);
            // wine
            image_draw(resource_images[RESOURCE_WINE].icon_img_id, b_info_context.x_offset + 362, b_info_context.y_offset + 274);
            text_draw_number(b->data.house.inventory[INVENTORY_WINE], '@', " ",
                b_info_context.x_offset + 394, b_info_context.y_offset + 278, FONT_NORMAL_BROWN);

            if (b->data.house.evolve_text_id == 62) {
                width = lang_text_draw(127, 40 + b->data.house.evolve_text_id, b_info_context.x_offset + 32, b_info_context.y_offset + 60, FONT_NORMAL_BLACK);
                width += text_draw(all_buildings_strings[all_buildings[b_info_context.worst_desirability_building_id].type], b_info_context.x_offset + 32 + width, b_info_context.y_offset + 60, FONT_NORMAL_PLAIN, COLOR_FONT_RED);
                text_draw(")", b_info_context.x_offset + 32 + width, b_info_context.y_offset + 60, FONT_NORMAL_BLACK, 0);
                lang_text_draw_multiline(127, 41 + b->data.house.evolve_text_id,
                    b_info_context.x_offset + 32, b_info_context.y_offset + 76, BLOCK_SIZE * (b_info_context.width_blocks - 4), FONT_NORMAL_BLACK);
            } else {
                lang_text_draw_multiline(127, 40 + b->data.house.evolve_text_id, b_info_context.x_offset + 32, b_info_context.y_offset + 70, BLOCK_SIZE * (b_info_context.width_blocks - 4), FONT_NORMAL_BLACK);
            }
        } else if (btype == BUILDING_WHEAT_FARM) {
            draw_farm(&b_info_context, 89, "wavs/wheat_farm.wav", 112, RESOURCE_WHEAT);
        } else if (btype == BUILDING_VEGETABLE_FARM) {
            draw_farm(&b_info_context, 90, "wavs/veg_farm.wav", 113, RESOURCE_VEGETABLES);
        } else if (btype == BUILDING_FRUIT_FARM) {
            draw_farm(&b_info_context, 90, "wavs/figs_farm.wav", 114, RESOURCE_FRUIT);
        } else if (btype == BUILDING_OLIVE_FARM) {
            draw_farm(&b_info_context, 91, "wavs/olives_farm.wav", 115, RESOURCE_OLIVES);
        } else if (btype == BUILDING_VINES_FARM) {
            draw_farm(&b_info_context, 91, "wavs/vines_farm.wav", 116, RESOURCE_VINES);
        } else if (btype == BUILDING_PIG_FARM) {
            draw_farm(&b_info_context, 90, "wavs/meat_farm.wav", 117, RESOURCE_MEAT);
        } else if (btype == BUILDING_MARBLE_QUARRY) {
            draw_raw_material(&b_info_context, 95, "wavs/quarry.wav", 118, RESOURCE_MARBLE);
        } else if (btype == BUILDING_IRON_MINE) {
            draw_raw_material(&b_info_context, 93, "wavs/mine.wav", 119, RESOURCE_IRON);
        } else if (btype == BUILDING_TIMBER_YARD) {
            draw_raw_material(&b_info_context, 94, "wavs/timber.wav", 120, RESOURCE_TIMBER);
        } else if (btype == BUILDING_CLAY_PIT) {
            draw_raw_material(&b_info_context, 92, "wavs/clay.wav", 121, RESOURCE_CLAY);
        } else if (btype == BUILDING_WINE_WORKSHOP) {
            draw_workshop(&b_info_context, 96, "wavs/wine_workshop.wav", 122, RESOURCE_WINE, RESOURCE_VINES);
        } else if (btype == BUILDING_OIL_WORKSHOP) {
            draw_workshop(&b_info_context, 97, "wavs/oil_workshop.wav", 123, RESOURCE_OIL, RESOURCE_OLIVES);
        } else if (btype == BUILDING_WEAPONS_WORKSHOP) {
            draw_workshop(&b_info_context, 98, "wavs/weapons_workshop.wav", 124, RESOURCE_WEAPONS, RESOURCE_IRON);
        } else if (btype == BUILDING_FURNITURE_WORKSHOP) {
            draw_workshop(&b_info_context, 99, "wavs/furniture_workshop.wav", 125, RESOURCE_FURNITURE, RESOURCE_TIMBER);
        } else if (btype == BUILDING_POTTERY_WORKSHOP) {
            draw_workshop(&b_info_context, 1, "wavs/pottery_workshop.wav", 126, RESOURCE_POTTERY, RESOURCE_CLAY);
        } else if (btype == BUILDING_MARKET) {
            b_info_context.help_id = 2;
            window_building_play_sound(&b_info_context, "wavs/market.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(97, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description(&b_info_context, 97, 2);
            } else {
                if (b->data.market.inventory[INVENTORY_WHEAT] || b->data.market.inventory[INVENTORY_VEGETABLES] ||
                    b->data.market.inventory[INVENTORY_FRUIT] || b->data.market.inventory[INVENTORY_MEAT]) {
                    // food stocks
                    image_draw(resource_images[RESOURCE_WHEAT].icon_img_id, b_info_context.x_offset + 32, b_info_context.y_offset + 64);
                    text_draw_number(b->data.market.inventory[INVENTORY_WHEAT], '@', " ",
                        b_info_context.x_offset + 64, b_info_context.y_offset + 70, FONT_NORMAL_BLACK);

                    image_draw(resource_images[RESOURCE_VEGETABLES].icon_img_id, b_info_context.x_offset + 142, b_info_context.y_offset + 64);
                    text_draw_number(b->data.market.inventory[INVENTORY_VEGETABLES], '@', " ",
                        b_info_context.x_offset + 174, b_info_context.y_offset + 70, FONT_NORMAL_BLACK);

                    image_draw(resource_images[RESOURCE_FRUIT].icon_img_id, b_info_context.x_offset + 252, b_info_context.y_offset + 64);
                    text_draw_number(b->data.market.inventory[INVENTORY_FRUIT], '@', " ",
                        b_info_context.x_offset + 284, b_info_context.y_offset + 70, FONT_NORMAL_BLACK);

                    image_draw(resource_images[RESOURCE_MEAT].icon_img_id + resource_image_offset(RESOURCE_MEAT, RESOURCE_IMAGE_ICON),
                        b_info_context.x_offset + 362, b_info_context.y_offset + 64);
                    text_draw_number(b->data.market.inventory[INVENTORY_MEAT], '@', " ",
                        b_info_context.x_offset + 394, b_info_context.y_offset + 70, FONT_NORMAL_BLACK);
                } else {
                    window_building_draw_description_at(&b_info_context, 48, 97, 4);
                }
                // good stocks
                image_draw(resource_images[RESOURCE_POTTERY].icon_img_id, b_info_context.x_offset + 32, b_info_context.y_offset + 104);
                text_draw_number(b->data.market.inventory[INVENTORY_POTTERY], '@', " ",
                    b_info_context.x_offset + 64, b_info_context.y_offset + 110, FONT_NORMAL_BLACK);

                image_draw(resource_images[RESOURCE_FURNITURE].icon_img_id, b_info_context.x_offset + 142, b_info_context.y_offset + 104);
                text_draw_number(b->data.market.inventory[INVENTORY_FURNITURE], '@', " ",
                    b_info_context.x_offset + 174, b_info_context.y_offset + 110, FONT_NORMAL_BLACK);

                image_draw(resource_images[RESOURCE_OIL].icon_img_id, b_info_context.x_offset + 252, b_info_context.y_offset + 104);
                text_draw_number(b->data.market.inventory[INVENTORY_OIL], '@', " ",
                    b_info_context.x_offset + 284, b_info_context.y_offset + 110, FONT_NORMAL_BLACK);

                image_draw(resource_images[RESOURCE_WINE].icon_img_id, b_info_context.x_offset + 362, b_info_context.y_offset + 104);
                text_draw_number(b->data.market.inventory[INVENTORY_WINE], '@', " ",
                    b_info_context.x_offset + 394, b_info_context.y_offset + 110, FONT_NORMAL_BLACK);
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
        } else if (btype == BUILDING_GRANARY) {
            if (b_info_context.storage_show_special_orders) {
                b_info_context.help_id = 3;
                int y_offset = window_building_get_vertical_offset(&b_info_context, 28);
                outer_panel_draw(b_info_context.x_offset, y_offset, 29, 28);
                lang_text_draw_centered(98, 6, b_info_context.x_offset, y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
                inner_panel_draw(b_info_context.x_offset + 16, y_offset + 42, b_info_context.width_blocks - 2, 21);
            } else {
                b_info_context.help_id = 3;
                window_building_play_sound(&b_info_context, "wavs/granary.wav");
                outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
                lang_text_draw_centered(98, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
                struct building_t *b = &all_buildings[b_info_context.building_id];
                if (!b_info_context.has_road_access) {
                    window_building_draw_description_at(&b_info_context, 40, 69, 25);
                } else if (scenario.rome_supplies_wheat) {
                    window_building_draw_description_at(&b_info_context, 40, 98, 4);
                } else {
                    int total_stored = 0;
                    for (int i = RESOURCE_WHEAT; i < FOOD_TYPES_MAX; i++) {
                        total_stored += b->data.granary.resource_stored[i];
                    }
                    int width = lang_text_draw(98, 2, b_info_context.x_offset + 34, b_info_context.y_offset + 40, FONT_NORMAL_BLACK);
                    lang_text_draw_amount(8, 16, total_stored, b_info_context.x_offset + 34 + width, b_info_context.y_offset + 40, FONT_NORMAL_BLACK);

                    width = lang_text_draw(98, 3, b_info_context.x_offset + 220, b_info_context.y_offset + 40, FONT_NORMAL_BLACK);
                    lang_text_draw_amount(8, 16, b->data.granary.resource_stored[RESOURCE_NONE],
                        b_info_context.x_offset + 220 + width, b_info_context.y_offset + 40, FONT_NORMAL_BLACK);

                    // wheat
                    image_draw(resource_images[RESOURCE_WHEAT].icon_img_id, b_info_context.x_offset + 34, b_info_context.y_offset + 68);
                    width = text_draw_number(b->data.granary.resource_stored[RESOURCE_WHEAT], '@', " ",
                        b_info_context.x_offset + 68, b_info_context.y_offset + 75, FONT_NORMAL_BLACK);
                    text_draw(resource_strings[RESOURCE_WHEAT], b_info_context.x_offset + 68 + width, b_info_context.y_offset + 75, FONT_NORMAL_BLACK, COLOR_BLACK);

                    // vegetables
                    image_draw(resource_images[RESOURCE_VEGETABLES].icon_img_id, b_info_context.x_offset + 34, b_info_context.y_offset + 92);
                    width = text_draw_number(b->data.granary.resource_stored[RESOURCE_VEGETABLES], '@', " ",
                        b_info_context.x_offset + 68, b_info_context.y_offset + 99, FONT_NORMAL_BLACK);
                    text_draw(resource_strings[RESOURCE_VEGETABLES], b_info_context.x_offset + 68 + width, b_info_context.y_offset + 99, FONT_NORMAL_BLACK, COLOR_BLACK);

                    // fruit
                    image_draw(resource_images[RESOURCE_FRUIT].icon_img_id, b_info_context.x_offset + 240, b_info_context.y_offset + 68);
                    width = text_draw_number(b->data.granary.resource_stored[RESOURCE_FRUIT], '@', " ",
                        b_info_context.x_offset + 274, b_info_context.y_offset + 75, FONT_NORMAL_BLACK);
                    text_draw(resource_strings[RESOURCE_FRUIT], b_info_context.x_offset + 274 + width, b_info_context.y_offset + 75, FONT_NORMAL_BLACK, COLOR_BLACK);

                    // meat/fish
                    image_draw(resource_images[RESOURCE_MEAT].icon_img_id + resource_image_offset(RESOURCE_MEAT, RESOURCE_IMAGE_ICON),
                        b_info_context.x_offset + 240, b_info_context.y_offset + 92);
                    width = text_draw_number(b->data.granary.resource_stored[RESOURCE_MEAT], '@', " ",
                        b_info_context.x_offset + 274, b_info_context.y_offset + 99, FONT_NORMAL_BLACK);
                    text_draw(resource_strings[RESOURCE_MEAT], b_info_context.x_offset + 274 + width, b_info_context.y_offset + 99, FONT_NORMAL_BLACK, COLOR_BLACK);
                }
                // cartpusher state
                int cartpusher = b->figure_id;
                int has_cart_orders = cartpusher && figure_is_alive(&figures[cartpusher]);
                inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, has_cart_orders ? 5 : 4);
                window_building_draw_employment(&b_info_context, 142);
                if (has_cart_orders) {
                    image_draw(resource_images[figures[cartpusher].resource_id].icon_img_id +
                        resource_image_offset(figures[cartpusher].resource_id, RESOURCE_IMAGE_ICON),
                        b_info_context.x_offset + 32, b_info_context.y_offset + 190);
                    lang_text_draw_multiline(99, 17, b_info_context.x_offset + 64, b_info_context.y_offset + 193,
                        BLOCK_SIZE * (b_info_context.width_blocks - 5), FONT_NORMAL_BROWN);
                }
            }
        } else if (btype == BUILDING_WAREHOUSE) {
            if (b_info_context.storage_show_special_orders) {
                int y_offset = window_building_get_vertical_offset(&b_info_context, 28);
                b_info_context.help_id = 4;
                outer_panel_draw(b_info_context.x_offset, y_offset, 29, 28);
                lang_text_draw_centered(99, 3, b_info_context.x_offset, y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
                inner_panel_draw(b_info_context.x_offset + 16, y_offset + 42, b_info_context.width_blocks - 2, 21);
            } else {
                b_info_context.help_id = 4;
                window_building_play_sound(&b_info_context, "wavs/warehouse.wav");
                outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
                lang_text_draw_centered(99, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
                struct building_t *b = &all_buildings[b_info_context.building_id];
                if (!b_info_context.has_road_access) {
                    window_building_draw_description(&b_info_context, 69, 25);
                } else {
                    for (int r = RESOURCE_WHEAT; r < RESOURCE_TYPES_MAX; r++) {
                        int x, y;
                        if (r <= 5) {
                            x = b_info_context.x_offset + 20;
                            y = b_info_context.y_offset + 24 * (r - 1) + 36;
                        } else if (r <= 10) {
                            x = b_info_context.x_offset + 170;
                            y = b_info_context.y_offset + 24 * (r - 6) + 36;
                        } else {
                            x = b_info_context.x_offset + 320;
                            y = b_info_context.y_offset + 24 * (r - 11) + 36;
                        }
                        int amount = building_warehouse_get_amount(b, r);
                        int image_id = resource_images[r].icon_img_id + resource_image_offset(r, RESOURCE_IMAGE_ICON);
                        image_draw(image_id, x, y);
                        int width = text_draw_number(amount, '@', " ", x + 24, y + 7, FONT_SMALL_PLAIN);
                        text_draw(resource_strings[r], x + 24 + width, y + 7, FONT_SMALL_PLAIN, COLOR_BLACK);
                    }
                }
                inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 168, b_info_context.width_blocks - 2, 5);
                window_building_draw_employment(&b_info_context, 173);
                // cartpusher state
                int cartpusher = b->figure_id;
                if (cartpusher && figure_is_alive(&figures[cartpusher])) {
                    image_draw(resource_images[figures[cartpusher].resource_id].icon_img_id +
                        resource_image_offset(figures[cartpusher].resource_id, RESOURCE_IMAGE_ICON),
                        b_info_context.x_offset + 32, b_info_context.y_offset + 220);
                    lang_text_draw_multiline(99, 17, b_info_context.x_offset + 64, b_info_context.y_offset + 223,
                        BLOCK_SIZE * (b_info_context.width_blocks - 5), FONT_NORMAL_BROWN);
                } else if (b->num_workers) {
                    // cartpusher is waiting for orders
                    lang_text_draw_multiline(99, 15, b_info_context.x_offset + 32, b_info_context.y_offset + 223,
                        BLOCK_SIZE * (b_info_context.width_blocks - 3), FONT_NORMAL_BROWN);
                }

                if (b_info_context.warehouse_space_text == 1) { // full
                    lang_text_draw_multiline(99, 13, b_info_context.x_offset + 32, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 93,
                        BLOCK_SIZE * (b_info_context.width_blocks - 4), FONT_NORMAL_BLACK);
                } else if (b_info_context.warehouse_space_text == 2) {
                    lang_text_draw_multiline(99, 14, b_info_context.x_offset + 32, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 93,
                        BLOCK_SIZE * (b_info_context.width_blocks - 4), FONT_NORMAL_BLACK);
                }
            }
        } else if (btype == BUILDING_AMPHITHEATER) {
            b_info_context.help_id = 72;
            window_building_play_sound(&b_info_context, "wavs/ampitheatre.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(71, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description(&b_info_context, 71, 6);
            } else if (!b->data.entertainment.num_shows) {
                window_building_draw_description(&b_info_context, 71, 2);
            } else if (b->data.entertainment.num_shows == 2) {
                window_building_draw_description(&b_info_context, 71, 3);
            } else if (b->data.entertainment.days1) {
                window_building_draw_description(&b_info_context, 71, 4);
            } else if (b->data.entertainment.days2) {
                window_building_draw_description(&b_info_context, 71, 5);
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 7);
            window_building_draw_employment(&b_info_context, 138);
            if (b->data.entertainment.days1 > 0) {
                int width = lang_text_draw(71, 8, b_info_context.x_offset + 32, b_info_context.y_offset + 182, FONT_NORMAL_BROWN);
                lang_text_draw_amount(8, 44, 2 * b->data.entertainment.days1,
                    b_info_context.x_offset + width + 32, b_info_context.y_offset + 182, FONT_NORMAL_BROWN);
            } else {
                lang_text_draw(71, 7, b_info_context.x_offset + 32, b_info_context.y_offset + 182, FONT_NORMAL_BROWN);
            }
            if (b->data.entertainment.days2 > 0) {
                int width = lang_text_draw(71, 10, b_info_context.x_offset + 32, b_info_context.y_offset + 202, FONT_NORMAL_BROWN);
                lang_text_draw_amount(8, 44, 2 * b->data.entertainment.days2,
                    b_info_context.x_offset + width + 32, b_info_context.y_offset + 202, FONT_NORMAL_BROWN);
                lang_text_draw(72, 7 + b->data.entertainment.play,
                    b_info_context.x_offset + 32, b_info_context.y_offset + 222, FONT_NORMAL_BROWN);
            } else {
                lang_text_draw(71, 9, b_info_context.x_offset + 32, b_info_context.y_offset + 202, FONT_NORMAL_BROWN);
            }
        } else if (btype == BUILDING_THEATER) {
            b_info_context.help_id = 71;
            window_building_play_sound(&b_info_context, "wavs/theatre.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(72, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description(&b_info_context, 72, 4);
            } else if (!b->data.entertainment.num_shows) {
                window_building_draw_description(&b_info_context, 72, 2);
            } else if (b->data.entertainment.days1) {
                window_building_draw_description(&b_info_context, 72, 3);
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 6);
            window_building_draw_employment(&b_info_context, 138);
            if (b->data.entertainment.days1 > 0) {
                int width = lang_text_draw(72, 6, b_info_context.x_offset + 32, b_info_context.y_offset + 182, FONT_NORMAL_BROWN);
                lang_text_draw_amount(8, 44, 2 * b->data.entertainment.days1,
                    b_info_context.x_offset + width + 32, b_info_context.y_offset + 182, FONT_NORMAL_BROWN);
                lang_text_draw(72, 7 + b->data.entertainment.play,
                    b_info_context.x_offset + 32, b_info_context.y_offset + 202, FONT_NORMAL_BROWN);
            } else {
                lang_text_draw(72, 5, b_info_context.x_offset + 32, b_info_context.y_offset + 182, FONT_NORMAL_BROWN);
            }
        } else if (btype == BUILDING_HIPPODROME) {
            b_info_context.help_id = 74;
            window_building_play_sound(&b_info_context, "wavs/hippodrome.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(73, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description(&b_info_context, 73, 4);
            } else if (!b->data.entertainment.num_shows) {
                window_building_draw_description(&b_info_context, 73, 2);
            } else if (b->data.entertainment.days1) {
                window_building_draw_description(&b_info_context, 73, 3);
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 6);
            window_building_draw_employment(&b_info_context, 138);
            if (b->data.entertainment.days1 > 0) {
                int width = lang_text_draw(73, 6, b_info_context.x_offset + 32, b_info_context.y_offset + 202, FONT_NORMAL_BROWN);
                lang_text_draw_amount(8, 44, 2 * b->data.entertainment.days1,
                    b_info_context.x_offset + width + 32, b_info_context.y_offset + 202, FONT_NORMAL_BROWN);
            } else {
                lang_text_draw(73, 5, b_info_context.x_offset + 32, b_info_context.y_offset + 202, FONT_NORMAL_BROWN);
            }
        } else if (btype == BUILDING_COLOSSEUM) {
            b_info_context.help_id = 73;
            window_building_play_sound(&b_info_context, "wavs/colloseum.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(74, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description(&b_info_context, 74, 6);
            } else if (!b->data.entertainment.num_shows) {
                window_building_draw_description(&b_info_context, 74, 2);
            } else if (b->data.entertainment.num_shows == 2) {
                window_building_draw_description(&b_info_context, 74, 3);
            } else if (b->data.entertainment.days1) {
                window_building_draw_description(&b_info_context, 74, 5);
            } else if (b->data.entertainment.days2) {
                window_building_draw_description(&b_info_context, 74, 4);
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 6);
            window_building_draw_employment(&b_info_context, 138);
            if (b->data.entertainment.days1 > 0) {
                int width = lang_text_draw(74, 8, b_info_context.x_offset + 32, b_info_context.y_offset + 182, FONT_NORMAL_BROWN);
                lang_text_draw_amount(8, 44, 2 * b->data.entertainment.days1,
                    b_info_context.x_offset + width + 32, b_info_context.y_offset + 182, FONT_NORMAL_BROWN);
            } else {
                lang_text_draw(74, 7, b_info_context.x_offset + 32, b_info_context.y_offset + 182, FONT_NORMAL_BROWN);
            }
            if (b->data.entertainment.days2 > 0) {
                int width = lang_text_draw(74, 10, b_info_context.x_offset + 32, b_info_context.y_offset + 202, FONT_NORMAL_BROWN);
                lang_text_draw_amount(8, 44, 2 * b->data.entertainment.days2,
                    b_info_context.x_offset + width + 32, b_info_context.y_offset + 202, FONT_NORMAL_BROWN);
            } else {
                lang_text_draw(74, 9, b_info_context.x_offset + 32, b_info_context.y_offset + 202, FONT_NORMAL_BROWN);
            }
        } else if (btype == BUILDING_GLADIATOR_SCHOOL) {
            draw_entertainment_school(&b_info_context, "wavs/glad_pit.wav", 75);
        } else if (btype == BUILDING_LION_HOUSE) {
            draw_entertainment_school(&b_info_context, "wavs/lion_pit.wav", 76);
        } else if (btype == BUILDING_ACTOR_COLONY) {
            draw_entertainment_school(&b_info_context, "wavs/art_pit.wav", 77);
        } else if (btype == BUILDING_CHARIOT_MAKER) {
            draw_entertainment_school(&b_info_context, "wavs/char_pit.wav", 78);
        } else if (btype == BUILDING_DOCTOR) {
            draw_culture_info(&b_info_context, 65, "wavs/clinic.wav", 81);
        } else if (btype == BUILDING_HOSPITAL) {
            draw_culture_info(&b_info_context, 66, "wavs/hospital.wav", 82);
        } else if (btype == BUILDING_BATHHOUSE) {
            b_info_context.help_id = 64;
            window_building_play_sound(&b_info_context, "wavs/baths.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(83, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (!b->has_water_access) {
                window_building_draw_description(&b_info_context, 83, 4);
            } else if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description(&b_info_context, 83, 2);
            } else {
                window_building_draw_description(&b_info_context, 83, 3);
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
        } else if (btype == BUILDING_BARBER) {
            draw_culture_info(&b_info_context, 63, "wavs/barber.wav", 84);
        } else if (btype == BUILDING_SCHOOL) {
            draw_culture_info(&b_info_context, 68, "wavs/school.wav", 85);
        } else if (btype == BUILDING_ACADEMY) {
            draw_culture_info(&b_info_context, 69, "wavs/academy.wav", 86);
        } else if (btype == BUILDING_LIBRARY) {
            draw_culture_info(&b_info_context, 70, "wavs/library.wav", 87);
        } else if (btype == BUILDING_SMALL_TEMPLE_CERES || btype == BUILDING_LARGE_TEMPLE_CERES) {
            draw_temple(&b_info_context, "wavs/temple_farm.wav", 92, 21);
        } else if (btype == BUILDING_SMALL_TEMPLE_NEPTUNE || btype == BUILDING_LARGE_TEMPLE_NEPTUNE) {
            draw_temple(&b_info_context, "wavs/temple_ship.wav", 93, 22);
        } else if (btype == BUILDING_SMALL_TEMPLE_MERCURY || btype == BUILDING_LARGE_TEMPLE_MERCURY) {
            draw_temple(&b_info_context, "wavs/temple_comm.wav", 94, 23);
        } else if (btype == BUILDING_SMALL_TEMPLE_MARS || btype == BUILDING_LARGE_TEMPLE_MARS) {
            draw_temple(&b_info_context, "wavs/temple_war.wav", 95, 24);
        } else if (btype == BUILDING_SMALL_TEMPLE_VENUS || btype == BUILDING_LARGE_TEMPLE_VENUS) {
            draw_temple(&b_info_context, "wavs/temple_love.wav", 96, 25);
        } else if (btype == BUILDING_ORACLE) {
            b_info_context.help_id = 67;
            window_building_play_sound(&b_info_context, "wavs/oracle.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(110, 0, b_info_context.x_offset, b_info_context.y_offset + 12, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 158, 110, 1);
        } else if (btype == BUILDING_GOVERNORS_HOUSE || btype == BUILDING_GOVERNORS_VILLA || btype == BUILDING_GOVERNORS_PALACE) {
            b_info_context.help_id = 78;
            window_building_play_sound(&b_info_context, "wavs/gov_palace.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(103, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 143, 103, 1);
        } else if (btype == BUILDING_FORUM) {
            b_info_context.help_id = 76;
            window_building_play_sound(&b_info_context, "wavs/forum.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(106, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            image_draw(COIN_IMAGE_ID, b_info_context.x_offset + 16, b_info_context.y_offset + 36);

            struct building_t *b = &all_buildings[b_info_context.building_id];
            int width = lang_text_draw(106, 2, b_info_context.x_offset + 44, b_info_context.y_offset + 43, FONT_NORMAL_BLACK);
            lang_text_draw_amount(8, 0, b->tax_income_or_storage,
                b_info_context.x_offset + 44 + width, b_info_context.y_offset + 43, FONT_NORMAL_BLACK);

            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description_at(&b_info_context, 72, 106, 10);
            } else if (b_info_context.worker_percentage >= 100) {
                window_building_draw_description_at(&b_info_context, 72, 106, 5);
            } else if (b_info_context.worker_percentage >= 75) {
                window_building_draw_description_at(&b_info_context, 72, 106, 6);
            } else if (b_info_context.worker_percentage >= 50) {
                window_building_draw_description_at(&b_info_context, 72, 106, 7);
            } else if (b_info_context.worker_percentage >= 25) {
                window_building_draw_description_at(&b_info_context, 72, 106, 8);
            } else {
                window_building_draw_description_at(&b_info_context, 72, 106, 9);
            }

            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
        } else if (btype == BUILDING_SENATE) {
            b_info_context.can_go_to_advisor = 1;
            b_info_context.help_id = 77;
            window_building_play_sound(&b_info_context, "wavs/senate.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(105, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            image_draw(COIN_IMAGE_ID, b_info_context.x_offset + 16, b_info_context.y_offset + 36);

            struct building_t *b = &all_buildings[b_info_context.building_id];
            int width = lang_text_draw(105, 2, b_info_context.x_offset + 44, b_info_context.y_offset + 43, FONT_NORMAL_BLACK);
            lang_text_draw_amount(8, 0, b->tax_income_or_storage,
                b_info_context.x_offset + 44 + width, b_info_context.y_offset + 43, FONT_NORMAL_BLACK);

            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description_at(&b_info_context, 72, 106, 10);
            } else if (b_info_context.worker_percentage >= 100) {
                window_building_draw_description_at(&b_info_context, 72, 106, 5);
            } else if (b_info_context.worker_percentage >= 75) {
                window_building_draw_description_at(&b_info_context, 72, 106, 6);
            } else if (b_info_context.worker_percentage >= 50) {
                window_building_draw_description_at(&b_info_context, 72, 106, 7);
            } else if (b_info_context.worker_percentage >= 25) {
                window_building_draw_description_at(&b_info_context, 72, 106, 8);
            } else {
                window_building_draw_description_at(&b_info_context, 72, 106, 9);
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
            lang_text_draw(105, 3, b_info_context.x_offset + 60, b_info_context.y_offset + 220, FONT_NORMAL_BLACK);
        } else if (btype == BUILDING_ENGINEERS_POST) {
            b_info_context.help_id = 81;
            window_building_play_sound(&b_info_context, "wavs/eng_post.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(104, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);

            struct building_t *b = &all_buildings[b_info_context.building_id];

            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (!b->num_workers) {
                window_building_draw_description(&b_info_context, 104, 9);
            } else {
                if (b->figure_id) {
                    window_building_draw_description(&b_info_context, 104, 2);
                } else {
                    window_building_draw_description(&b_info_context, 104, 3);
                }
                if (b_info_context.worker_percentage >= 100) {
                    window_building_draw_description_at(&b_info_context, 72, 104, 4);
                } else if (b_info_context.worker_percentage >= 75) {
                    window_building_draw_description_at(&b_info_context, 72, 104, 5);
                } else if (b_info_context.worker_percentage >= 50) {
                    window_building_draw_description_at(&b_info_context, 72, 104, 6);
                } else if (b_info_context.worker_percentage >= 25) {
                    window_building_draw_description_at(&b_info_context, 72, 104, 7);
                } else {
                    window_building_draw_description_at(&b_info_context, 72, 104, 8);
                }
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
        } else if (btype == BUILDING_SHIPYARD) {
            b_info_context.help_id = 82;
            window_building_play_sound(&b_info_context, "wavs/shipyard.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(100, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else {
                int pct_done = calc_percentage(b->data.industry.progress, 160);
                int width = lang_text_draw(100, 2, b_info_context.x_offset + 32, b_info_context.y_offset + 56, FONT_NORMAL_BLACK);
                width += text_draw_percentage(pct_done, b_info_context.x_offset + 32 + width, b_info_context.y_offset + 56, FONT_NORMAL_BLACK);
                lang_text_draw(100, 3, b_info_context.x_offset + 32 + width, b_info_context.y_offset + 56, FONT_NORMAL_BLACK);
                if (city_data.building.shipyard_boats_requested) {
                    lang_text_draw_multiline(100, 5, b_info_context.x_offset + 32, b_info_context.y_offset + 80,
                        BLOCK_SIZE * (b_info_context.width_blocks - 6), FONT_NORMAL_BLACK);
                } else {
                    lang_text_draw_multiline(100, 4, b_info_context.x_offset + 32, b_info_context.y_offset + 80,
                        BLOCK_SIZE * (b_info_context.width_blocks - 6), FONT_NORMAL_BLACK);
                }
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
        } else if (btype == BUILDING_DOCK) {
            b_info_context.help_id = 83;
            window_building_play_sound(&b_info_context, "wavs/dock.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(101, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);

            struct building_t *b = &all_buildings[b_info_context.building_id];

            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->data.dock.trade_ship_id) {
                if (b_info_context.worker_percentage <= 0) {
                    window_building_draw_description(&b_info_context, 101, 2);
                } else if (b_info_context.worker_percentage < 50) {
                    window_building_draw_description(&b_info_context, 101, 3);
                } else if (b_info_context.worker_percentage < 75) {
                    window_building_draw_description(&b_info_context, 101, 4);
                } else {
                    window_building_draw_description(&b_info_context, 101, 5);
                }
            } else {
                if (b_info_context.worker_percentage <= 0) {
                    window_building_draw_description(&b_info_context, 101, 6);
                } else if (b_info_context.worker_percentage < 50) {
                    window_building_draw_description(&b_info_context, 101, 7);
                } else if (b_info_context.worker_percentage < 75) {
                    window_building_draw_description(&b_info_context, 101, 8);
                } else {
                    window_building_draw_description(&b_info_context, 101, 9);
                }
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
        } else if (btype == BUILDING_WHARF) {
            b_info_context.help_id = 84;
            window_building_play_sound(&b_info_context, "wavs/wharf.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(102, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            image_draw(resource_images[RESOURCE_MEAT].icon_img_id + resource_image_offset(RESOURCE_MEAT, RESOURCE_IMAGE_ICON), b_info_context.x_offset + 10, b_info_context.y_offset + 10);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (!b->data.industry.fishing_boat_id) {
                window_building_draw_description(&b_info_context, 102, 2);
            } else {
                int text_id;
                switch (figures[b->data.industry.fishing_boat_id].action_state) {
                    case FIGURE_ACTION_FISHING_BOAT_GOING_TO_FISH: text_id = 3; break;
                    case FIGURE_ACTION_FISHING_BOAT_FISHING: text_id = 4; break;
                    case FIGURE_ACTION_FISHING_BOAT_GOING_TO_WHARF: text_id = 5; break;
                    case FIGURE_ACTION_FISHING_BOAT_AT_WHARF: text_id = 6; break;
                    case FIGURE_ACTION_FISHING_BOAT_RETURNING_WITH_FISH: text_id = 7; break;
                    default: text_id = 8; break;
                }
                window_building_draw_description(&b_info_context, 102, text_id);
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
        } else if (btype == BUILDING_RESERVOIR) {
            b_info_context.help_id = 59;
            window_building_play_sound(&b_info_context, "wavs/resevoir.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(107, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            int text_id = all_buildings[b_info_context.building_id].has_water_access ? 1 : 3;
            window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 173, 107, text_id);
        } else if (btype == BUILDING_FOUNTAIN) {
            b_info_context.help_id = 61;
            window_building_play_sound(&b_info_context, "wavs/fountain.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(108, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            int text_id;
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (b->has_water_access) {
                if (b->num_workers > 0) {
                    text_id = 1;
                } else {
                    text_id = 2;
                }
            } else if (b_info_context.has_reservoir_pipes) {
                text_id = 2;
            } else {
                text_id = 3;
            }
            window_building_draw_description(&b_info_context, 108, text_id);
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 166, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment_without_house_cover(&b_info_context, 172);
        } else if (btype == BUILDING_WELL) {
            b_info_context.help_id = 62;
            window_building_play_sound(&b_info_context, "wavs/well.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(109, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            int well_necessity = WELL_UNNECESSARY_NO_HOUSES;
            struct building_t *well = &all_buildings[b_info_context.building_id];
            int x_min, y_min, x_max, y_max;
            map_grid_get_area(well->x, well->y, 1, 2, &x_min, &y_min, &x_max, &y_max);
            for (int yy = y_min; yy <= y_max; yy++) {
                for (int xx = x_min; xx <= x_max; xx++) {
                    int grid_offset = map_grid_offset(xx, yy);
                    int building_id = map_building_at(grid_offset);
                    if (building_id && all_buildings[building_id].house_size) {
                        if (!map_terrain_is(grid_offset, TERRAIN_FOUNTAIN_RANGE)) {
                            well_necessity = WELL_NECESSARY;
                            break;
                        }
                    }
                }
            }
            int text_id = 0;
            if (well_necessity == WELL_NECESSARY) { // well is OK
                text_id = 1;
            } else if (well_necessity == WELL_UNNECESSARY_FOUNTAIN) { // all houses have fountain
                text_id = 2;
            } else if (well_necessity == WELL_UNNECESSARY_NO_HOUSES) { // no houses around
                text_id = 3;
            }
            if (text_id) {
                window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 160, 109, text_id);
            }
        } else if (btype == BUILDING_SMALL_STATUE
            || btype == BUILDING_MEDIUM_STATUE
            || btype == BUILDING_LARGE_STATUE) {
            b_info_context.help_id = 79;
            window_building_play_sound(&b_info_context, "wavs/statue.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(80, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 158, 80, 1);
        } else if (btype == BUILDING_TRIUMPHAL_ARCH) {
            b_info_context.help_id = 79;
            window_building_play_sound(&b_info_context, "wavs/statue.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(80, 2, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 158, 80, 3);
        } else if (btype == BUILDING_PREFECTURE) {
            b_info_context.help_id = 86;
            window_building_play_sound(&b_info_context, "wavs/prefecture.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(88, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description(&b_info_context, 88, 9);
            } else {
                if (b->figure_id) {
                    window_building_draw_description(&b_info_context, 88, 2);
                } else {
                    window_building_draw_description(&b_info_context, 88, 3);
                }
                if (b_info_context.worker_percentage >= 100) {
                    window_building_draw_description_at(&b_info_context, 72, 88, 4);
                } else if (b_info_context.worker_percentage >= 75) {
                    window_building_draw_description_at(&b_info_context, 72, 88, 5);
                } else if (b_info_context.worker_percentage >= 50) {
                    window_building_draw_description_at(&b_info_context, 72, 88, 6);
                } else if (b_info_context.worker_percentage >= 25) {
                    window_building_draw_description_at(&b_info_context, 72, 88, 7);
                } else {
                    window_building_draw_description_at(&b_info_context, 72, 88, 8);
                }
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
        } else if (btype == BUILDING_GATEHOUSE) {
            b_info_context.help_id = 85;
            window_building_play_sound(&b_info_context, "wavs/gatehouse.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(90, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 158, 90, 1);
        } else if (btype == BUILDING_TOWER) {
            b_info_context.help_id = 85;
            window_building_play_sound(&b_info_context, "wavs/tower.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(91, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description(&b_info_context, 91, 2);
            } else if (b->figure_id) {
                window_building_draw_description(&b_info_context, 91, 3);
            } else {
                window_building_draw_description(&b_info_context, 91, 4);
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
        } else if (btype == BUILDING_MILITARY_ACADEMY) {
            b_info_context.help_id = 88;
            window_building_play_sound(&b_info_context, "wavs/mil_acad.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(135, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (!b_info_context.has_road_access) {
                window_building_draw_description(&b_info_context, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description(&b_info_context, 135, 2);
            } else if (b_info_context.worker_percentage < 100) {
                window_building_draw_description(&b_info_context, 135, 1);
            } else {
                window_building_draw_description(&b_info_context, 135, 3);
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
        } else if (btype == BUILDING_BARRACKS) {
            b_info_context.help_id = 37;
            window_building_play_sound(&b_info_context, "wavs/barracks.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(136, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            image_draw(resource_images[RESOURCE_WEAPONS].icon_img_id, b_info_context.x_offset + 64, b_info_context.y_offset + 38);
            struct building_t *b = &all_buildings[b_info_context.building_id];
            if (b->loads_stored < 1) {
                lang_text_draw_amount(8, 10, 0, b_info_context.x_offset + 92, b_info_context.y_offset + 44, FONT_NORMAL_BLACK);
            } else {
                lang_text_draw_amount(8, 10, b->loads_stored, b_info_context.x_offset + 92, b_info_context.y_offset + 44, FONT_NORMAL_BLACK);
            }
            if (!b_info_context.has_road_access) {
                window_building_draw_description_at(&b_info_context, 70, 69, 25);
            } else if (b->num_workers <= 0) {
                window_building_draw_description_at(&b_info_context, 70, 136, 3);
            } else if (!b_info_context.barracks_soldiers_requested) {
                window_building_draw_description_at(&b_info_context, 70, 136, 4);
            } else {
                int offset = 0;
                if (b->loads_stored > 0) {
                    offset = 4;
                }
                if (b_info_context.worker_percentage >= 100) {
                    window_building_draw_description_at(&b_info_context, 70, 136, 5 + offset);
                } else if (b_info_context.worker_percentage >= 66) {
                    window_building_draw_description_at(&b_info_context, 70, 136, 6 + offset);
                } else if (b_info_context.worker_percentage >= 33) {
                    window_building_draw_description_at(&b_info_context, 70, 136, 7 + offset);
                } else {
                    window_building_draw_description_at(&b_info_context, 70, 136, 8 + offset);
                }
            }
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment(&b_info_context, 142);
        } else if (building_is_fort(btype)) {
            b_info_context.help_id = 87;
            window_building_play_sound(&b_info_context, "wavs/fort.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(89, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            window_building_draw_description_at(&b_info_context, BLOCK_SIZE * b_info_context.height_blocks - 158, 89, legion_formations[b_info_context.formation_id].cursed_by_mars ? 1 : 2);
        } else if (btype == BUILDING_BURNING_RUIN) {
            b_info_context.help_id = 0;
            window_building_play_sound(&b_info_context, "wavs/ruin.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(111, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            lang_text_draw(41, b_info_context.rubble_building_type, b_info_context.x_offset + 32, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 173, FONT_NORMAL_BLACK);
            lang_text_draw_multiline(111, 1, b_info_context.x_offset + 32, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 143, BLOCK_SIZE * (b_info_context.width_blocks - 4), FONT_NORMAL_BLACK);
        } else if (btype == BUILDING_NATIVE_HUT) {
            draw_native(&b_info_context, 131);
        } else if (btype == BUILDING_NATIVE_MEETING) {
            draw_native(&b_info_context, 132);
        } else if (btype == BUILDING_NATIVE_CROPS) {
            draw_native(&b_info_context, 133);
        } else if (btype == BUILDING_MISSION_POST) {
            b_info_context.help_id = 8;
            window_building_play_sound(&b_info_context, "wavs/mission.wav");
            outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
            lang_text_draw_centered(134, 0, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
            window_building_draw_description(&b_info_context, 134, 1);
            inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 136, b_info_context.width_blocks - 2, 4);
            window_building_draw_employment_without_house_cover(&b_info_context, 142);
        }
    } else if (b_info_context.type == BUILDING_INFO_LEGION) {
        int text_id;
        b_info_context.help_id = 87;
        struct formation_t *m = &legion_formations[b_info_context.formation_id];
        outer_panel_draw(b_info_context.x_offset, b_info_context.y_offset, b_info_context.width_blocks, b_info_context.height_blocks);
        // legion name
        lang_text_draw_centered(138, m->id, b_info_context.x_offset, b_info_context.y_offset + 10, BLOCK_SIZE * b_info_context.width_blocks, FONT_LARGE_BLACK);
        // animal symbol at the top of banner pole
        int icon_image_id = image_group(GROUP_FIGURE_FORT_STANDARD_ICONS) + m->id;
        const struct image_t *icon_image = image_get(icon_image_id);
        image_draw(icon_image_id, b_info_context.x_offset + 16 + (40 - icon_image->width) / 2, b_info_context.y_offset + 16);
        // legion banner
        int flag_image_id = image_group(GROUP_FIGURE_FORT_FLAGS);
        if (m->figure_type == FIGURE_FORT_JAVELIN) {
            flag_image_id += 9;
        } else if (m->figure_type == FIGURE_FORT_MOUNTED) {
            flag_image_id += 18;
        }
        const struct image_t *flag_image = image_get(flag_image_id);
        image_draw(flag_image_id, b_info_context.x_offset + 16 + (40 - flag_image->width) / 2, b_info_context.y_offset + 16 + icon_image->height);
        // banner pole and morale ball
        int pole_image_id = image_group(GROUP_FIGURE_FORT_STANDARD_POLE) + 20 - m->morale / 5;
        image_draw(pole_image_id, b_info_context.x_offset + 16 + (40 - image_get(pole_image_id)->width) / 2, b_info_context.y_offset + 16 + icon_image->height + flag_image->height);
        // number of soldiers
        lang_text_draw(138, 23, b_info_context.x_offset + 100, b_info_context.y_offset + 60, FONT_NORMAL_BLACK);
        text_draw_number(m->num_figures, ' ', "", b_info_context.x_offset + 294, b_info_context.y_offset + 60, FONT_NORMAL_BLACK);
        // health
        lang_text_draw(138, 24, b_info_context.x_offset + 100, b_info_context.y_offset + 80, FONT_NORMAL_BLACK);
        int formation_damage = 0;
        int formation_max_damage = 0;
        for (int i = 0; i < m->num_figures; i++) {
            struct figure_t *f = &figures[m->figures[i]];
            if (figure_is_alive(f)) {
                formation_damage += f->damage;
                formation_max_damage += figure_properties[f->type].max_damage;
            }
        }
        int formation_damage_perc = calc_percentage(formation_damage, formation_max_damage);
        if (formation_damage_perc <= 0) {
            text_id = 26;
        } else if (formation_damage_perc <= 20) {
            text_id = 27;
        } else if (formation_damage_perc <= 40) {
            text_id = 28;
        } else if (formation_damage_perc <= 55) {
            text_id = 29;
        } else if (formation_damage_perc <= 70) {
            text_id = 30;
        } else if (formation_damage_perc <= 90) {
            text_id = 31;
        } else {
            text_id = 32;
        }
        lang_text_draw(138, text_id, b_info_context.x_offset + 300, b_info_context.y_offset + 80, FONT_NORMAL_BLACK);
        // military training
        lang_text_draw(138, 25, b_info_context.x_offset + 100, b_info_context.y_offset + 100, FONT_NORMAL_BLACK);
        lang_text_draw(18, m->has_military_training, b_info_context.x_offset + 300, b_info_context.y_offset + 100, FONT_NORMAL_BLACK);
        // morale
        if (m->cursed_by_mars) {
            lang_text_draw(138, 59, b_info_context.x_offset + 100, b_info_context.y_offset + 120, FONT_NORMAL_BLACK);
        } else {
            lang_text_draw(138, 36, b_info_context.x_offset + 100, b_info_context.y_offset + 120, FONT_NORMAL_BLACK);
            lang_text_draw(138, 37 + m->morale / 5, b_info_context.x_offset + 300, b_info_context.y_offset + 120, FONT_NORMAL_BLACK);
        }
        if (m->num_figures) {
            // layout
            static const int OFFSETS_LEGIONARY[2][5] = {
                {0, 0, 2, 3, 4},
                {0, 0, 3, 2, 4},
            };
            static const int OFFSETS_OTHER[2][5] = {
                {5, 6, 2, 3, 4},
                {6, 5, 3, 2, 4},
            };
            const int *offsets;
            int rotation_index = 0;
            if (city_view_orientation() == DIR_6_LEFT || city_view_orientation() == DIR_2_RIGHT) {
                rotation_index = 1;
            }
            if (m->figure_type == FIGURE_FORT_LEGIONARY) {
                offsets = OFFSETS_LEGIONARY[rotation_index];
            } else {
                offsets = OFFSETS_OTHER[rotation_index];
            }
            for (int i = 0; i < 5; i++) {
                // for legionaries, draw tortoise formation in first position if academy trained, skip second position
                if (m->figure_type == FIGURE_FORT_LEGIONARY) {
                    if ((i == 0 && !m->has_military_training) || i == 1) {
                        continue;
                    }
                }
                image_draw(image_group(GROUP_FORT_FORMATIONS) + offsets[i], b_info_context.x_offset + 21 + 85 * i, b_info_context.y_offset + 141);
            }
        } else {
            // no soldiers
            int group_id;
            if (m->cursed_by_mars) {
                group_id = 89;
                text_id = 1;
            } else if (building_count_active(BUILDING_BARRACKS)) {
                group_id = 138;
                text_id = 10;
            } else {
                group_id = 138;
                text_id = 11;
            }
            window_building_draw_description_at(&b_info_context, 172, group_id, text_id);
        }
    }
}

static void draw_foreground_building_info(void)
{
    // building-specific buttons
    if (b_info_context.type == BUILDING_INFO_BUILDING) {
        int btype = all_buildings[b_info_context.building_id].type;
        if (btype == BUILDING_GRANARY) {
            if (b_info_context.storage_show_special_orders) {
                int y_offset = window_building_get_vertical_offset(&b_info_context, 28);
                // empty button
                button_border_draw(b_info_context.x_offset + 80, y_offset + 404, BLOCK_SIZE * (b_info_context.width_blocks - 10), 20,
                    distribution_data.orders_focus_button_id == 1 ? 1 : 0);
                struct building_storage_t *storage = building_storage_get(all_buildings[b_info_context.building_id].storage_id);
                if (storage->empty_all) {
                    lang_text_draw_centered(98, 8, b_info_context.x_offset + 80, y_offset + 408, BLOCK_SIZE * (b_info_context.width_blocks - 10), FONT_NORMAL_BLACK);
                    lang_text_draw_centered(98, 9, b_info_context.x_offset, y_offset + 384, BLOCK_SIZE * b_info_context.width_blocks, FONT_NORMAL_BLACK);
                } else {
                    lang_text_draw_centered(98, 7, b_info_context.x_offset + 80, y_offset + 408, BLOCK_SIZE * (b_info_context.width_blocks - 10), FONT_NORMAL_BLACK);
                }
                // accept none button
                draw_accept_none_button(b_info_context.x_offset + 394, y_offset + 404, distribution_data.orders_focus_button_id == 2);
                struct resource_list_t *list = city_resource_get_available_foods();
                for (int i = 0; i < list->size; i++) {
                    int resource = list->items[i];
                    int image_id = resource_images[resource].icon_img_id + resource_image_offset(resource, RESOURCE_IMAGE_ICON);
                    image_draw(image_id, b_info_context.x_offset + 32, y_offset + 46 + 22 * i);
                    image_draw(image_id, b_info_context.x_offset + 408, y_offset + 46 + 22 * i);
                    text_draw(resource_strings[resource], b_info_context.x_offset + 72, y_offset + 50 + 22 * i, FONT_NORMAL_WHITE, COLOR_BLACK);
                    button_border_draw(b_info_context.x_offset + 180, y_offset + 46 + 22 * i, 210, 22, distribution_data.resource_focus_button_id == i + 1);

                    int state = storage->resource_state[resource];
                    if (state == BUILDING_STORAGE_STATE_ACCEPTING) {
                        lang_text_draw(99, 7, b_info_context.x_offset + 230, y_offset + 51 + 22 * i, FONT_NORMAL_WHITE);
                    } else if (state == BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
                        lang_text_draw(99, 8, b_info_context.x_offset + 230, y_offset + 51 + 22 * i, FONT_NORMAL_RED);
                    } else if (state == BUILDING_STORAGE_STATE_GETTING) {
                        image_draw(image_group(GROUP_CONTEXT_ICONS) + 12, b_info_context.x_offset + 186, y_offset + 49 + 22 * i);
                        lang_text_draw(99, 10, b_info_context.x_offset + 230, y_offset + 51 + 22 * i, FONT_NORMAL_WHITE);
                    }
                }
            } else {
                button_border_draw(b_info_context.x_offset + 80, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 34, BLOCK_SIZE * (b_info_context.width_blocks - 10), 20, distribution_data.focus_button_id == 1 ? 1 : 0);
                lang_text_draw_centered(98, 5, b_info_context.x_offset + 80, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 30, BLOCK_SIZE * (b_info_context.width_blocks - 10), FONT_NORMAL_BLACK);
            }
        } else if (btype == BUILDING_WAREHOUSE) {
            if (b_info_context.storage_show_special_orders) {
                int y_offset = window_building_get_vertical_offset(&b_info_context, 28);
                // emptying button
                button_border_draw(b_info_context.x_offset + 80, y_offset + 404, BLOCK_SIZE * (b_info_context.width_blocks - 10),
                    20, distribution_data.orders_focus_button_id == 1 ? 1 : 0);
                struct building_storage_t *storage = building_storage_get(all_buildings[b_info_context.building_id].storage_id);
                if (storage->empty_all) {
                    lang_text_draw_centered(99, 5, b_info_context.x_offset + 80, y_offset + 408,
                        BLOCK_SIZE * (b_info_context.width_blocks - 10), FONT_NORMAL_BLACK);
                    lang_text_draw_centered(99, 6, b_info_context.x_offset, y_offset + 426, BLOCK_SIZE * b_info_context.width_blocks, FONT_SMALL_PLAIN);
                } else {
                    lang_text_draw_centered(99, 4, b_info_context.x_offset + 80, y_offset + 408,
                        BLOCK_SIZE * (b_info_context.width_blocks - 10), FONT_NORMAL_BLACK);
                }
                // trade center
                button_border_draw(b_info_context.x_offset + 80, y_offset + 382, BLOCK_SIZE * (b_info_context.width_blocks - 10),
                    20, distribution_data.orders_focus_button_id == 2 ? 1 : 0);
                int is_trade_center = b_info_context.building_id == city_data.building.trade_center_building_id;
                lang_text_draw_centered(99, is_trade_center ? 11 : 12, b_info_context.x_offset + 80, y_offset + 386,
                    BLOCK_SIZE * (b_info_context.width_blocks - 10), FONT_NORMAL_BLACK);
                // accept none button
                draw_accept_none_button(b_info_context.x_offset + 394, y_offset + 404, distribution_data.orders_focus_button_id == 3);
                struct resource_list_t *list = city_resource_get_available();
                for (int i = 0; i < list->size; i++) {
                    int resource = list->items[i];
                    int image_id = resource_images[resource].icon_img_id + resource_image_offset(resource, RESOURCE_IMAGE_ICON);
                    image_draw(image_id, b_info_context.x_offset + 32, y_offset + 46 + 22 * i);
                    image_draw(image_id, b_info_context.x_offset + 408, y_offset + 46 + 22 * i);
                    text_draw(resource_strings[resource], b_info_context.x_offset + 72, y_offset + 50 + 22 * i, FONT_NORMAL_WHITE, COLOR_BLACK);
                    button_border_draw(b_info_context.x_offset + 180, y_offset + 46 + 22 * i, 210, 22, distribution_data.resource_focus_button_id == i + 1);
                    int state = storage->resource_state[resource];
                    if (state == BUILDING_STORAGE_STATE_ACCEPTING) {
                        lang_text_draw(99, 7, b_info_context.x_offset + 230, y_offset + 51 + 22 * i, FONT_NORMAL_WHITE);
                    } else if (state == BUILDING_STORAGE_STATE_NOT_ACCEPTING) {
                        lang_text_draw(99, 8, b_info_context.x_offset + 230, y_offset + 51 + 22 * i, FONT_NORMAL_RED);
                    } else if (state == BUILDING_STORAGE_STATE_GETTING) {
                        image_draw(image_group(GROUP_CONTEXT_ICONS) + 12, b_info_context.x_offset + 186, y_offset + 49 + 22 * i);
                        lang_text_draw(99, 9, b_info_context.x_offset + 230, y_offset + 51 + 22 * i, FONT_NORMAL_WHITE);
                    }
                }
            } else {
                button_border_draw(b_info_context.x_offset + 80, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 34, BLOCK_SIZE * (b_info_context.width_blocks - 10), 20, distribution_data.focus_button_id == 1 ? 1 : 0);
                lang_text_draw_centered(99, 2, b_info_context.x_offset + 80, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 30, BLOCK_SIZE * (b_info_context.width_blocks - 10), FONT_NORMAL_BLACK);
            }
        }
    } else if (b_info_context.type == BUILDING_INFO_LEGION) {
        struct formation_t *m = &legion_formations[b_info_context.formation_id];
        if (!m->num_figures) {
            return;
        }
        for (int i = 0; i < 5; i++) {
            // for legionaries, draw tortoise formation border in first position if academy trained, skip second position
            if (m->figure_type == FIGURE_FORT_LEGIONARY) {
                if ((i == 0 && !m->has_military_training) || i == 1) {
                    continue;
                }
            }
            button_border_draw(b_info_context.x_offset + 19 + 85 * i, b_info_context.y_offset + 139, 84, 84, building_military_data.focus_button_id == i + 1);
        }

        inner_panel_draw(b_info_context.x_offset + 16, b_info_context.y_offset + 230, b_info_context.width_blocks - 2, 4);

        int title_id;
        int text_id;
        switch (building_military_data.focus_button_id) {
            case 1: // tortoise formation for legionaries, single line for aux
                if (m->figure_type == FIGURE_FORT_LEGIONARY) {
                    title_id = 12;
                    text_id = m->has_military_training ? 18 : 17;
                } else {
                    title_id = 16;
                    text_id = 22;
                }
                break;
            case 2: // skip for legionaries, single line for aux
                if (m->figure_type == FIGURE_FORT_LEGIONARY) {
                    goto default_label;
                } else {
                    title_id = 16;
                    text_id = 22;
                }
                break;
            case 3: // double line
            case 4: // double line
                title_id = 14;
                text_id = 20;
                break;
            case 5: // mop up
                title_id = 15;
                text_id = 21;
                break;
            default_label:
            default:
                // no button selected: go for formation layout
                switch (m->layout) {
                    case FORMATION_TORTOISE:
                        title_id = 12;
                        text_id = 18;
                        break;
                    case FORMATION_SINGLE_LINE_1:
                    case FORMATION_SINGLE_LINE_2:
                        title_id = 16;
                        text_id = 22;
                        break;
                    case FORMATION_DOUBLE_LINE_1:
                    case FORMATION_DOUBLE_LINE_2:
                        title_id = 14;
                        text_id = 20;
                        break;
                    case FORMATION_MOP_UP:
                        title_id = 15;
                        text_id = 21;
                        break;
                    default:
                        title_id = 16;
                        text_id = 22;
                        log_info("Unknown formation", 0, m->layout);
                        break;
                }
                break;
        }
        // draw formation info
        lang_text_draw(138, title_id, b_info_context.x_offset + 24, b_info_context.y_offset + 236, FONT_NORMAL_WHITE);
        lang_text_draw_multiline(138, text_id, b_info_context.x_offset + 24, b_info_context.y_offset + 252, BLOCK_SIZE * (b_info_context.width_blocks - 4), FONT_NORMAL_GREEN);

        // Return to fort
        if (!m->is_at_rest && !m->in_distant_battle) {
            button_border_draw(b_info_context.x_offset + BLOCK_SIZE * (b_info_context.width_blocks - 18) / 2, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 48, 288, 32, building_military_data.return_button_id == 1);
            lang_text_draw_centered(138, 58, b_info_context.x_offset + BLOCK_SIZE * (b_info_context.width_blocks - 18) / 2, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 39, 288, FONT_NORMAL_BLACK);
        }
    }
    // general buttons
    if (b_info_context.storage_show_special_orders) {
        int y_offset = window_building_get_vertical_offset(&b_info_context, 28);
        image_buttons_draw(b_info_context.x_offset, y_offset + 400, image_buttons_help_close, 2);
    } else {
        image_buttons_draw(b_info_context.x_offset, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 40,
            image_buttons_help_close, 2);
    }
    if (b_info_context.can_go_to_advisor) {
        image_buttons_draw(b_info_context.x_offset, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 40,
            image_buttons_advisor, 1);
    }
}

static void handle_input_building_info(const struct mouse_t *m, const struct hotkeys_t *h)
{
    int handled = 0;
    // general buttons
    if (b_info_context.storage_show_special_orders) {
        int y_offset = window_building_get_vertical_offset(&b_info_context, 28);
        handled |= image_buttons_handle_mouse(m, b_info_context.x_offset, y_offset + 400, image_buttons_help_close, 2, &focus_image_button_id_building_info);
    } else {
        handled |= image_buttons_handle_mouse(m, b_info_context.x_offset, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 40, image_buttons_help_close, 2, &focus_image_button_id_building_info);
    }
    if (b_info_context.can_go_to_advisor) {
        handled |= image_buttons_handle_mouse(m, b_info_context.x_offset, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 40, image_buttons_advisor, 1, 0);
    }
    if (!handled) {
        // building-specific buttons
        if (b_info_context.type == BUILDING_INFO_LEGION) {
            building_military_data.context_for_callback = &b_info_context;
            handled = generic_buttons_handle_mouse(m, b_info_context.x_offset, b_info_context.y_offset, layout_buttons, sizeof(layout_buttons) / sizeof(struct generic_button_t), &building_military_data.focus_button_id);
            if (legion_formations[b_info_context.formation_id].figure_type == FIGURE_FORT_LEGIONARY) {
                if (building_military_data.focus_button_id == 2 || (building_military_data.focus_button_id == 1 && b_info_context.formation_types == 3)) {
                    building_military_data.focus_button_id = 0;
                }
            }
            if (!handled) {
                handled = generic_buttons_handle_mouse(m, b_info_context.x_offset + BLOCK_SIZE * (b_info_context.width_blocks - 18) / 2,
                    b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 48, return_button, 1, &building_military_data.return_button_id);
            }
            building_military_data.context_for_callback = 0;
        } else if (b_info_context.figure.drawn) {
            building_figures_data.context_for_callback = &b_info_context;
            handled = generic_buttons_handle_mouse(m, b_info_context.x_offset, b_info_context.y_offset, figure_buttons, b_info_context.figure.count, &building_figures_data.focus_button_id);
            building_figures_data.context_for_callback = 0;
        } else if (b_info_context.type == BUILDING_INFO_BUILDING) {
            int btype = all_buildings[b_info_context.building_id].type;
            if (btype == BUILDING_GRANARY) {
                if (b_info_context.storage_show_special_orders) {
                    int y_offset = window_building_get_vertical_offset(&b_info_context, 28);
                    distribution_data.building_id = b_info_context.building_id;
                    if (generic_buttons_handle_mouse(m, b_info_context.x_offset + 180, y_offset + 46,
                        orders_resource_buttons, city_resource_get_available_foods()->size,
                        &distribution_data.resource_focus_button_id)) {
                    }
                    generic_buttons_handle_mouse(m, b_info_context.x_offset + 80, y_offset + 404, granary_order_buttons, 2, &distribution_data.orders_focus_button_id);
                } else {
                    generic_buttons_handle_mouse(m, b_info_context.x_offset + 80, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 34, go_to_orders_button, 1, &distribution_data.focus_button_id);
                }
            } else if (btype == BUILDING_WAREHOUSE) {
                if (b_info_context.storage_show_special_orders) {
                    int y_offset = window_building_get_vertical_offset(&b_info_context, 28);
                    distribution_data.building_id = b_info_context.building_id;
                    if (generic_buttons_handle_mouse(m, b_info_context.x_offset + 180, y_offset + 46, orders_resource_buttons, city_resource_get_available()->size,
                        &distribution_data.resource_focus_button_id)) {
                    }
                    generic_buttons_handle_mouse(m, b_info_context.x_offset + 80, y_offset + 404, warehouse_order_buttons, 3, &distribution_data.orders_focus_button_id);
                } else {
                    generic_buttons_handle_mouse(m, b_info_context.x_offset + 80, b_info_context.y_offset + BLOCK_SIZE * b_info_context.height_blocks - 34, go_to_orders_button, 1, &distribution_data.focus_button_id);
                }
            }
        }
    }
    if (m->right.went_up || h->escape_pressed) {
        window_city_show();
    }
}

void window_building_info_show(int grid_offset)
{
    struct window_type_t window = {
        WINDOW_BUILDING_INFO,
        draw_background_building_info,
        draw_foreground_building_info,
        handle_input_building_info,
    };
    b_info_context.can_play_sound = 1;
    b_info_context.storage_show_special_orders = 0;
    b_info_context.can_go_to_advisor = 0;
    b_info_context.building_id = map_building_at(grid_offset);
    b_info_context.rubble_building_type = map_rubble_building_type(grid_offset);
    b_info_context.has_reservoir_pipes = map_terrain_is(grid_offset, TERRAIN_RESERVOIR_RANGE);
    b_info_context.aqueduct_has_water = map_aqueduct_at(grid_offset)
        && map_image_at(grid_offset) - image_group(GROUP_BUILDING_AQUEDUCT) < 15;

    city_resource_determine_available();
    b_info_context.type = BUILDING_INFO_TERRAIN;
    b_info_context.figure.drawn = 0;
    if (!b_info_context.building_id && map_sprite_bridge_at(grid_offset) > 0) {
        if (map_terrain_is(grid_offset, TERRAIN_WATER)) {
            b_info_context.terrain_type = TERRAIN_INFO_BRIDGE;
        } else {
            b_info_context.terrain_type = TERRAIN_INFO_EMPTY;
        }
    } else if (map_property_is_plaza_or_earthquake(grid_offset)) {
        if (map_terrain_is(grid_offset, TERRAIN_ROAD)) {
            b_info_context.terrain_type = TERRAIN_INFO_PLAZA;
        }
        if (map_terrain_is(grid_offset, TERRAIN_ROCK)) {
            b_info_context.terrain_type = TERRAIN_INFO_EARTHQUAKE;
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_SHRUB)) {
        b_info_context.terrain_type = TERRAIN_INFO_TREE;
    } else if (map_terrain_is(grid_offset, TERRAIN_ROCK)) {
        if (grid_offset == map_grid_offset(scenario.entry_point.x, scenario.entry_point.y)) {
            b_info_context.terrain_type = TERRAIN_INFO_ENTRY_FLAG;
        } else if (grid_offset == map_grid_offset(scenario.exit_point.x, scenario.exit_point.y)) {
            b_info_context.terrain_type = TERRAIN_INFO_EXIT_FLAG;
        } else {
            b_info_context.terrain_type = TERRAIN_INFO_ROCK;
        }
    } else if ((terrain_grid.items[grid_offset] & (TERRAIN_WATER | TERRAIN_BUILDING)) == TERRAIN_WATER) {
        b_info_context.terrain_type = TERRAIN_INFO_WATER;
    } else if (map_terrain_is(grid_offset, TERRAIN_TREE)) {
        b_info_context.terrain_type = TERRAIN_INFO_SHRUB;
    } else if (map_terrain_is(grid_offset, TERRAIN_GARDEN)) {
        b_info_context.terrain_type = TERRAIN_INFO_GARDEN;
    } else if ((terrain_grid.items[grid_offset] & (TERRAIN_ROAD | TERRAIN_BUILDING)) == TERRAIN_ROAD) {
        b_info_context.terrain_type = TERRAIN_INFO_ROAD;
    } else if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT)) {
        b_info_context.terrain_type = TERRAIN_INFO_AQUEDUCT;
    } else if (map_terrain_is(grid_offset, TERRAIN_RUBBLE)) {
        b_info_context.terrain_type = TERRAIN_INFO_RUBBLE;
    } else if (map_terrain_is(grid_offset, TERRAIN_WALL)) {
        b_info_context.terrain_type = TERRAIN_INFO_WALL;
    } else if (!b_info_context.building_id) {
        b_info_context.terrain_type = TERRAIN_INFO_EMPTY;
    } else {
        struct building_t *b = &all_buildings[b_info_context.building_id];
        b_info_context.type = BUILDING_INFO_BUILDING;
        b_info_context.worker_percentage = calc_percentage(b->num_workers, building_properties[b->type].n_laborers);
        switch (b->type) {
            case BUILDING_FORT_GROUND:
                b_info_context.building_id = b->prev_part_building_id;
                // fallthrough
            case BUILDING_FORT_LEGIONARIES:
            case BUILDING_FORT_JAVELIN:
            case BUILDING_FORT_MOUNTED:
                b_info_context.formation_id = b->formation_id;
                break;
            case BUILDING_WAREHOUSE_SPACE:
            case BUILDING_HIPPODROME:
                b = building_main(b);
                b_info_context.building_id = b->id;
                break;
            case BUILDING_BARRACKS:
                b_info_context.barracks_soldiers_requested = 0;
                for (int i = 0; i < MAX_LEGIONS; i++) {
                    if (legion_formations[i].in_use && legion_formations[i].num_figures < legion_formations[i].max_figures) {
                        b_info_context.barracks_soldiers_requested = 1;
                        break;
                    }
                }
                b_info_context.barracks_soldiers_requested += tower_sentry_request;
                break;
            default:
                if (b->house_size) {
                    int lowest_desirability = 0;
                    int lowest_building_id = 0;
                    int x_min, y_min, x_max, y_max;
                    map_grid_get_area(b->x, b->y, 1, 6, &x_min, &y_min, &x_max, &y_max);
                    for (int y = y_min; y <= y_max; y++) {
                        for (int x = x_min; x <= x_max; x++) {
                            int building_id = map_building_at(map_grid_offset(x, y));
                            if (building_id <= 0) {
                                continue;
                            }
                            struct building_t *bb = &all_buildings[building_id];
                            if (bb->state != BUILDING_STATE_IN_USE || building_id == b->id) {
                                continue;
                            }
                            if (!bb->house_size || bb->type < b->type) {
                                int des = building_properties[bb->type].desirability_value;
                                if (des < 0) {
                                    // simplified desirability calculation
                                    int step_size = building_properties[bb->type].desirability_step_size;
                                    int range = building_properties[bb->type].desirability_range;
                                    int dist = calc_maximum_distance(x, y, b->x, b->y);
                                    if (dist <= range) {
                                        while (--dist > 1) {
                                            des += step_size;
                                        }
                                        if (des < lowest_desirability) {
                                            lowest_desirability = des;
                                            lowest_building_id = building_id;
                                        }
                                    }
                                }
                            }
                        }
                    }
                    b_info_context.worst_desirability_building_id = lowest_building_id;
                    int level = b->subtype.house_level;
                    int foodtypes_available = 0;
                    for (int i = INVENTORY_WHEAT; i <= INVENTORY_MEAT; i++) {
                        if (b->data.house.inventory[i]) {
                            foodtypes_available++;
                        }
                    }
                    // this house will devolve soon because...
                    if (b->desirability <= house_properties[level].devolve_desirability) {
                        b->data.house.evolve_text_id = 0;
                    } else if (house_properties[level].water == 1 && !b->has_water_access && !b->has_well_access) {
                        b->data.house.evolve_text_id = 1;
                    } else if (house_properties[level].water == 2 && !b->has_water_access) {
                        b->data.house.evolve_text_id = 2;
                    } else if (b->data.house.entertainment < house_properties[level].entertainment) {
                        if (!b->data.house.entertainment) {
                            b->data.house.evolve_text_id = 3;
                        } else if (house_properties[level].entertainment < 10) {
                            b->data.house.evolve_text_id = 4;
                        } else if (house_properties[level].entertainment < 25) {
                            b->data.house.evolve_text_id = 5;
                        } else if (house_properties[level].entertainment < 50) {
                            b->data.house.evolve_text_id = 6;
                        } else if (house_properties[level].entertainment < 80) {
                            b->data.house.evolve_text_id = 7;
                        } else {
                            b->data.house.evolve_text_id = 8;
                        }
                    } else if (foodtypes_available < house_properties[level].food_types) {
                        if (house_properties[level].food_types == 1) {
                            b->data.house.evolve_text_id = 9;
                        } else if (house_properties[level].food_types == 2) {
                            b->data.house.evolve_text_id = 10;
                        } else if (house_properties[level].food_types == 3) {
                            b->data.house.evolve_text_id = 11;
                        }
                    } else if (b->data.house.education < house_properties[level].education) {
                        if (house_properties[level].education == 1) {
                            b->data.house.evolve_text_id = 14;
                        } else if (house_properties[level].education == 2) {
                            if (b->data.house.school) {
                                b->data.house.evolve_text_id = 15;
                            } else if (b->data.house.library) {
                                b->data.house.evolve_text_id = 16;
                            }
                        } else if (house_properties[level].education == 3) {
                            b->data.house.evolve_text_id = 17;
                        }
                    } else if (b->data.house.bathhouse < house_properties[level].bathhouse) {
                        b->data.house.evolve_text_id = 18;
                    } else if (b->data.house.inventory[INVENTORY_POTTERY] < house_properties[level].pottery) {
                        b->data.house.evolve_text_id = 19;
                    } else if (b->data.house.num_gods < house_properties[level].religion) {
                        if (house_properties[level].religion == 1) {
                            b->data.house.evolve_text_id = 20;
                        } else if (house_properties[level].religion == 2) {
                            b->data.house.evolve_text_id = 21;
                        } else if (house_properties[level].religion == 3) {
                            b->data.house.evolve_text_id = 22;
                        }
                    } else if (b->data.house.barber < house_properties[level].barber) {
                        b->data.house.evolve_text_id = 23;
                    } else if (b->data.house.health < house_properties[level].health) {
                        if (house_properties[level].health == 1) {
                            b->data.house.evolve_text_id = 24;
                        } else if (b->data.house.clinic) {
                            b->data.house.evolve_text_id = 25;
                        } else {
                            b->data.house.evolve_text_id = 26;
                        }
                    } else if (b->data.house.inventory[INVENTORY_OIL] < house_properties[level].oil) {
                        b->data.house.evolve_text_id = 27;
                    } else if (b->data.house.inventory[INVENTORY_FURNITURE] < house_properties[level].furniture) {
                        b->data.house.evolve_text_id = 28;
                    } else if (b->data.house.inventory[INVENTORY_WINE] < house_properties[level].wine) {
                        b->data.house.evolve_text_id = 29;
                    } else if (house_properties[level].wine > 1 && !city_resource_multiple_wine_available()) {
                        b->data.house.evolve_text_id = 65;
                    } else if (level >= HOUSE_LUXURY_PALACE) { // max level!
                        b->data.house.evolve_text_id = 60;
                    }
                    // this house will evolve if ...
                    if (b->desirability < house_properties[level].evolve_desirability) {
                        if (b_info_context.worst_desirability_building_id) {
                            b->data.house.evolve_text_id = 62;
                        } else {
                            b->data.house.evolve_text_id = 30;
                        }
                    }
                    // water
                    if (house_properties[level + 1].water == 1 && !b->has_water_access && !b->has_well_access) {
                        b->data.house.evolve_text_id = 31;
                    } else if (house_properties[level + 1].water == 2 && !b->has_water_access) {
                        b->data.house.evolve_text_id = 32;
                    } else if (b->data.house.entertainment < house_properties[level + 1].entertainment) {
                        if (!b->data.house.entertainment) {
                            b->data.house.evolve_text_id = 33;
                        } else if (house_properties[level + 1].entertainment < 10) {
                            b->data.house.evolve_text_id = 34;
                        } else if (house_properties[level + 1].entertainment < 25) {
                            b->data.house.evolve_text_id = 35;
                        } else if (house_properties[level + 1].entertainment < 50) {
                            b->data.house.evolve_text_id = 36;
                        } else if (house_properties[level + 1].entertainment < 80) {
                            b->data.house.evolve_text_id = 37;
                        } else {
                            b->data.house.evolve_text_id = 38;
                        }
                    } else if (foodtypes_available < house_properties[level + 1].food_types) {
                        if (house_properties[level + 1].food_types == 1) {
                            b->data.house.evolve_text_id = 39;
                        } else if (house_properties[level + 1].food_types == 2) {
                            b->data.house.evolve_text_id = 40;
                        } else if (house_properties[level + 1].food_types == 3) {
                            b->data.house.evolve_text_id = 41;
                        }
                    } else if (b->data.house.education < house_properties[level + 1].education) {
                        if (house_properties[level + 1].education == 1) {
                            b->data.house.evolve_text_id = 44;
                        } else if (house_properties[level + 1].education == 2) {
                            if (b->data.house.school) {
                                b->data.house.evolve_text_id = 45;
                            } else if (b->data.house.library) {
                                b->data.house.evolve_text_id = 46;
                            }
                        } else if (house_properties[level + 1].education == 3) {
                            b->data.house.evolve_text_id = 47;
                        }
                    } else if (b->data.house.bathhouse < house_properties[level + 1].bathhouse) {
                        b->data.house.evolve_text_id = 48;
                    } else if (b->data.house.inventory[INVENTORY_POTTERY] < house_properties[level + 1].pottery) {
                        b->data.house.evolve_text_id = 49;
                    } else if (b->data.house.num_gods < house_properties[level + 1].religion) {
                        if (house_properties[level + 1].religion == 1) {
                            b->data.house.evolve_text_id = 50;
                        } else if (house_properties[level + 1].religion == 2) {
                            b->data.house.evolve_text_id = 51;
                        } else if (house_properties[level + 1].religion == 3) {
                            b->data.house.evolve_text_id = 52;
                        }
                    } else if (b->data.house.barber < house_properties[level + 1].barber) {
                        b->data.house.evolve_text_id = 53;
                    } else if (b->data.house.health < house_properties[level + 1].health) {
                        if (house_properties[level + 1].health == 1) {
                            b->data.house.evolve_text_id = 54;
                        } else if (b->data.house.clinic) {
                            b->data.house.evolve_text_id = 55;
                        } else {
                            b->data.house.evolve_text_id = 56;
                        }
                    } else if (b->data.house.inventory[INVENTORY_OIL] < house_properties[level + 1].oil) {
                        b->data.house.evolve_text_id = 57;
                    } else if (b->data.house.inventory[INVENTORY_FURNITURE] < house_properties[level + 1].furniture) {
                        b->data.house.evolve_text_id = 58;
                    } else if (b->data.house.inventory[INVENTORY_WINE] < house_properties[level + 1].wine) {
                        b->data.house.evolve_text_id = 59;
                    } else if (house_properties[level + 1].wine > 1 && !city_resource_multiple_wine_available()) {
                        b->data.house.evolve_text_id = 66;
                    }
                    // house is evolving
                    b->data.house.evolve_text_id = 61;
                    if (b->data.house.no_space_to_expand == 1) {
                        // house would like to evolve but can't
                        b->data.house.evolve_text_id = 64;
                    }
                }
                break;
        }
        b_info_context.has_road_access = 0;
        switch (b->type) {
            case BUILDING_GRANARY:
                if (map_has_road_access_granary(b->x, b->y, 0)) {
                    b_info_context.has_road_access = 1;
                }
                break;
            case BUILDING_HIPPODROME:
                if (map_has_road_access_hippodrome(b->x, b->y, 0)) {
                    b_info_context.has_road_access = 1;
                }
                break;
            case BUILDING_WAREHOUSE:
                if (map_has_road_access(b->x, b->y, 3, 0)) {
                    b_info_context.has_road_access = 1;
                }
                int total_loads = 0;
                int empty_spaces = 0;
                struct building_t *space = b;
                for (int i = 0; i < 8; i++) {
                    space = &all_buildings[space->next_part_building_id];
                    if (space->subtype.warehouse_resource_id) {
                        total_loads += space->loads_stored;
                    } else {
                        empty_spaces++;
                    }
                }
                if (empty_spaces > 0) {
                    b_info_context.warehouse_space_text = WAREHOUSE_ROOM;
                } else if (total_loads < 32) {
                    b_info_context.warehouse_space_text = WAREHOUSE_SOME_ROOM;
                } else {
                    b_info_context.warehouse_space_text = WAREHOUSE_FULL;
                }
                break;
            default:
                if (map_has_road_access(b->x, b->y, b->size, 0)) {
                    b_info_context.has_road_access = 1;
                }
                break;
        }
    }
    // figures
    b_info_context.figure.selected_index = 0;
    b_info_context.figure.count = 0;
    for (int i = 0; i < 7; i++) {
        b_info_context.figure.figure_ids[i] = 0;
    }
    static const int FIGURE_OFFSETS[] = {
        OFFSET(0,0), OFFSET(0,-1), OFFSET(0,1), OFFSET(1,0), OFFSET(-1,0),
        OFFSET(-1,-1), OFFSET(1,-1), OFFSET(-1,1), OFFSET(1,1)
    };
    for (int i = 0; i < 9 && b_info_context.figure.count < 7; i++) {
        int figure_id = map_figure_at(grid_offset + FIGURE_OFFSETS[i]);
        while (figure_id > 0 && b_info_context.figure.count < 7) {
            struct figure_t *f = &figures[figure_id];
            if (figure_is_alive(f)) {
                switch (f->type) {
                    case FIGURE_NONE:
                    case FIGURE_EXPLOSION:
                    case FIGURE_MAP_FLAG:
                    case FIGURE_FLOTSAM:
                    case FIGURE_ARROW:
                    case FIGURE_JAVELIN:
                    case FIGURE_BOLT:
                    case FIGURE_BALLISTA:
                    case FIGURE_FISH_GULLS:
                    case FIGURE_HIPPODROME_HORSES:
                        break;
                    default:
                        b_info_context.figure.figure_ids[b_info_context.figure.count++] = figure_id;
                        f->phrase_id = 0;
                        if (figure_properties[f->type].is_enemy_unit || figure_properties[f->type].is_caesar_legion_unit || figure_properties[f->type].is_native_unit) {
                            f->phrase_id = -1;
                        }
                        int phrase_id = 0;
                        switch (f->type) {
                            case FIGURE_LION_TAMER:
                                if (f->engaged_in_combat) {
                                    if (++f->phrase_sequence_exact >= 3) {
                                        f->phrase_sequence_exact = 0;
                                    }
                                    phrase_id = 7 + f->phrase_sequence_exact;
                                }
                                break;
                            case FIGURE_GLADIATOR:
                                phrase_id = f->engaged_in_combat ? 7 : 0;
                                break;
                            case FIGURE_TAX_COLLECTOR:
                                if (f->min_max_seen >= HOUSE_LARGE_CASA) {
                                    phrase_id = 7;
                                } else if (f->min_max_seen >= HOUSE_SMALL_HOVEL) {
                                    phrase_id = 8;
                                } else if (f->min_max_seen >= HOUSE_LARGE_TENT) {
                                    phrase_id = 9;
                                }
                                break;
                            case FIGURE_MARKET_TRADER:
                                if (f->action_state == FIGURE_ACTION_ROAMER_RETURNING) {
                                    if (building_market_get_max_food_stock(&all_buildings[f->building_id]) <= 0) {
                                        phrase_id = 9; // run out of goods
                                    }
                                }
                                break;
                            case FIGURE_MARKET_BUYER:
                                if (f->action_state == FIGURE_ACTION_MARKET_BUYER_GOING_TO_STORAGE) {
                                    phrase_id = 7;
                                } else if (f->action_state == FIGURE_ACTION_MARKET_BUYER_RETURNING) {
                                    phrase_id = 8;
                                }
                                break;
                            case FIGURE_CART_PUSHER:
                                if (f->action_state == FIGURE_ACTION_CARTPUSHER_INITIAL) {
                                    if (f->min_max_seen == 2) {
                                        phrase_id = 7;
                                    } else if (f->min_max_seen == 1) {
                                        phrase_id = 8;
                                    }
                                } else if (f->action_state == FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WAREHOUSE ||
                                        f->action_state == FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_GRANARY ||
                                        f->action_state == FIGURE_ACTION_CARTPUSHER_DELIVERING_TO_WORKSHOP) {
                                    if (calc_maximum_distance(
                                        f->destination_x, f->destination_y, f->source_x, f->source_y) >= 25) {
                                        phrase_id = 9; // too far
                                    }
                                }
                                break;
                            case FIGURE_WAREHOUSEMAN:
                                if (f->action_state == FIGURE_ACTION_WAREHOUSEMAN_DELIVERING_RESOURCE) {
                                    if (calc_maximum_distance(
                                        f->destination_x, f->destination_y, f->source_x, f->source_y) >= 25) {
                                        phrase_id = 9; // too far
                                    }
                                }
                                break;
                            case FIGURE_PREFECT:
                                if (++f->phrase_sequence_exact >= 4) {
                                    f->phrase_sequence_exact = 0;
                                }
                                if (f->action_state == FIGURE_ACTION_PREFECT_GOING_TO_FIRE) {
                                    phrase_id = 10;
                                } else if (f->action_state == FIGURE_ACTION_PREFECT_AT_FIRE) {
                                    phrase_id = 11 + (f->phrase_sequence_exact % 2);
                                } else if (f->engaged_in_combat) {
                                    phrase_id = 13 + f->phrase_sequence_exact;
                                } else if (f->min_max_seen >= 50) {
                                    // alternate between "no sign of crime around here" and the regular city phrases
                                    if (f->phrase_sequence_exact % 2) {
                                        phrase_id = 7;
                                    }
                                } else if (f->min_max_seen >= 10) {
                                    phrase_id = 8;
                                } else {
                                    phrase_id = 9;
                                }
                                break;
                            case FIGURE_ENGINEER:
                                if (f->min_max_seen >= 60) {
                                    phrase_id = 7;
                                } else if (f->min_max_seen >= 10) {
                                    phrase_id = 8;
                                }
                                break;
                            case FIGURE_PROTESTER:
                            case FIGURE_CRIMINAL:
                            case FIGURE_RIOTER:
                            case FIGURE_DELIVERY_BOY:
                            case FIGURE_MISSIONARY:
                                if (++f->phrase_sequence_exact >= 3) {
                                    f->phrase_sequence_exact = 0;
                                }
                                phrase_id = 7 + f->phrase_sequence_exact;
                                break;
                            case FIGURE_HOMELESS:
                            case FIGURE_IMMIGRANT:
                                if (++f->phrase_sequence_exact >= 3) {
                                    f->phrase_sequence_exact = 0;
                                }
                                phrase_id = 7 + f->phrase_sequence_exact;
                                break;
                            case FIGURE_EMIGRANT:
                                switch (city_data.sentiment.low_mood_cause) {
                                    case LOW_MOOD_CAUSE_NO_JOBS:
                                        phrase_id = 7;
                                        break;
                                    case LOW_MOOD_CAUSE_NO_FOOD:
                                        phrase_id = 8;
                                        break;
                                    case LOW_MOOD_CAUSE_HIGH_TAXES:
                                        phrase_id = 9;
                                        break;
                                    case LOW_MOOD_CAUSE_LOW_WAGES:
                                        phrase_id = 10;
                                        break;
                                    default:
                                        phrase_id = 11;
                                        break;
                                }
                                break;
                            case FIGURE_TOWER_SENTRY:
                                if (++f->phrase_sequence_exact >= 2) {
                                    f->phrase_sequence_exact = 0;
                                }
                                if (!city_data.figure.enemies) {
                                    phrase_id = 7 + f->phrase_sequence_exact;
                                } else if (city_data.figure.enemies <= 10) {
                                    phrase_id = 9;
                                } else if (city_data.figure.enemies <= 30) {
                                    phrase_id = 10;
                                } else {
                                    phrase_id = 11;
                                }
                                break;
                            case FIGURE_FORT_JAVELIN:
                            case FIGURE_FORT_MOUNTED:
                            case FIGURE_FORT_LEGIONARY:
                                if (city_data.figure.enemies >= 40) {
                                    phrase_id = 11;
                                } else if (city_data.figure.enemies > 20) {
                                    phrase_id = 10;
                                } else if (city_data.figure.enemies) {
                                    phrase_id = 9;
                                }
                                break;
                            case FIGURE_DOCKER:
                                if (f->action_state == FIGURE_ACTION_DOCKER_IMPORT_GOING_TO_WAREHOUSE
                                    || f->action_state == FIGURE_ACTION_DOCKER_EXPORT_GOING_TO_WAREHOUSE) {
                                    if (calc_maximum_distance(f->destination_x, f->destination_y, f->source_x, f->source_y) >= 25) {
                                        phrase_id = 9; // too far
                                    }
                                }
                                break;
                            case FIGURE_TRADE_CARAVAN:
                                phrase_id = trade_caravan_phrase(f);
                                break;
                            case FIGURE_TRADE_CARAVAN_DONKEY:
                                while (f->type == FIGURE_TRADE_CARAVAN_DONKEY && f->leading_figure_id) {
                                    f = &figures[f->leading_figure_id];
                                }
                                phrase_id = f->type == FIGURE_TRADE_CARAVAN ? trade_caravan_phrase(f) : 0;
                                break;
                            case FIGURE_TRADE_SHIP:
                                if (f->action_state == FIGURE_ACTION_TRADE_SHIP_LEAVING) {
                                    if (!trader_has_traded(f->trader_id)) {
                                        phrase_id = 9; // no trade
                                    } else {
                                        phrase_id = 11; // good trade
                                    }
                                } else if (f->action_state == FIGURE_ACTION_TRADE_SHIP_MOORED) {
                                    int state = figure_trade_ship_is_trading(f);
                                    if (state == TRADE_SHIP_BUYING) {
                                        phrase_id = 8; // buying goods
                                    } else if (state == TRADE_SHIP_SELLING) {
                                        phrase_id = 7; // selling goods
                                    } else {
                                        if (!trader_has_traded(f->trader_id)) {
                                            phrase_id = 9; // no trade
                                        } else {
                                            phrase_id = 11; // good trade
                                        }
                                    }
                                } else {
                                    phrase_id = 10; // can't wait to trade
                                }
                        }
                        if (phrase_id) {
                            f->phrase_id = phrase_id;
                        } else {
                            f->phrase_sequence_city = 0;
                            int god_state = GOD_STATE_NONE;
                            int least_god_happiness = 100;
                            for (int god = 0; god < MAX_GODS; god++) {
                                if (city_data.religion.gods[god].happiness < least_god_happiness) {
                                    least_god_happiness = city_data.religion.gods[god].happiness;
                                }
                            }
                            if (least_god_happiness < 20) {
                                god_state = GOD_STATE_VERY_ANGRY;
                            } else if (least_god_happiness < 40) {
                                god_state = GOD_STATE_ANGRY;
                            }
                            int unemployment_pct = city_data.labor.unemployment_percentage;
                            if (city_data.resource.food_supply_months <= 0) {
                                f->phrase_id = 0;
                            } else if (unemployment_pct >= 17) {
                                f->phrase_id = 1;
                            } else if (city_data.labor.workers_needed >= 10) {
                                f->phrase_id = 2;
                            } else if (city_data.culture.average_entertainment == 0) {
                                f->phrase_id = 3;
                            } else if (god_state == GOD_STATE_VERY_ANGRY) {
                                f->phrase_id = 4;
                            } else if (city_data.culture.average_entertainment <= 10) {
                                f->phrase_id = 3;
                            } else if (god_state == GOD_STATE_ANGRY) {
                                f->phrase_id = 4;
                            } else if (city_data.culture.average_entertainment <= 20) {
                                f->phrase_id = 3;
                            } else if (city_data.resource.food_supply_months >= 4 &&
                                    unemployment_pct <= 5 &&
                                    city_data.culture.average_health > 0 &&
                                    city_data.culture.average_education > 0) {
                                if (city_data.population.population < 500) {
                                    f->phrase_id = 5;
                                } else {
                                    f->phrase_id = 6;
                                }
                            } else if (unemployment_pct >= 10) {
                                f->phrase_id = 1;
                            } else {
                                f->phrase_id = 5;
                            }
                        }
                        break;
                }
            }
            figure_id = f->next_figure_id_on_same_tile;
        }
    }
    // check for legion figures
    for (int i = 0; i < 7; i++) {
        int figure_id = b_info_context.figure.figure_ids[i];
        if (figure_id <= 0) {
            continue;
        }
        struct figure_t *f = &figures[figure_id];
        if (f->type == FIGURE_FORT_STANDARD || figure_properties[f->type].is_player_legion_unit) {
            b_info_context.type = BUILDING_INFO_LEGION;
            b_info_context.formation_id = f->formation_id;
            if (legion_formations[b_info_context.formation_id].figure_type != FIGURE_FORT_LEGIONARY) {
                b_info_context.formation_types = 5;
            } else if (legion_formations[b_info_context.formation_id].has_military_training) {
                b_info_context.formation_types = 4;
            } else {
                b_info_context.formation_types = 3;
            }
            break;
        }
    }
    // dialog size
    b_info_context.width_blocks = 29;
    switch (get_height_id()) {
        case 1:  b_info_context.height_blocks = 16; break;
        case 2:  b_info_context.height_blocks = 18; break;
        case 3:  b_info_context.height_blocks = 19; break;
        case 4:  b_info_context.height_blocks = 14; break;
        case 5:  b_info_context.height_blocks = 23; break;
        default:  b_info_context.height_blocks = 22; break;
    }
    // dialog placement
    int s_width = screen_width();
    int s_height = screen_height();
    b_info_context.x_offset = center_in_city(BLOCK_SIZE * b_info_context.width_blocks);
    if (s_width >= 1024 && s_height >= 768) {
        b_info_context.x_offset = mouse_get()->x - 225;
        b_info_context.y_offset = mouse_get()->y - 250;
        int dialog_width = BLOCK_SIZE * b_info_context.width_blocks;
        int dialog_height = BLOCK_SIZE * b_info_context.height_blocks;
        int stub;
        int width;
        city_view_get_viewport(&stub, &stub, &width, &stub);
        width -= MARGIN_POSITION;
        if (b_info_context.y_offset + dialog_height > screen_height() - MARGIN_POSITION) {
            b_info_context.y_offset -= dialog_height;
        }
        b_info_context.y_offset = (b_info_context.y_offset < MIN_Y_POSITION) ? MIN_Y_POSITION : b_info_context.y_offset;
        if (b_info_context.x_offset + dialog_width > width) {
            b_info_context.x_offset = width - dialog_width;
        }
    } else if (s_height >= 600 && mouse_get()->y <= (s_height - 24) / 2 + 24) {
        b_info_context.y_offset = s_height - BLOCK_SIZE * b_info_context.height_blocks - MARGIN_POSITION;
    } else {
        b_info_context.y_offset = MIN_Y_POSITION;
    }
    window_show(&window);
}