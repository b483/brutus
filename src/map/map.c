#include "map.h"

#include "building/building.h"
#include "city/data.h"
#include "city/view.h"
#include "core/calc.h"
#include "core/image.h"
#include "core/random.h"
#include "figuretype/animal.h"
#include "figuretype/wall.h"
#include "game/game.h"
#include "editor/editor.h"
#include "scenario/scenario.h"

#include <string.h>

#define MAX_BOOKMARKS 4

#define MAX_TILES 8

#define MAX_PATH 500

#define MAX_QUEUE_ROUTING GRID_SIZE * GRID_SIZE
#define GUARD 50000

#define UNTIL_STOP 0
#define UNTIL_CONTINUE 1

#define FORBIDDEN_TERRAIN_MEADOW (TERRAIN_AQUEDUCT | TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP | TERRAIN_RUBBLE | TERRAIN_ROAD | TERRAIN_BUILDING | TERRAIN_GARDEN)

#define FORBIDDEN_TERRAIN_RUBBLE (TERRAIN_AQUEDUCT | TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP | TERRAIN_ROAD | TERRAIN_BUILDING | TERRAIN_GARDEN)

#define MAX_QUEUE 1000


/**
 * The aqueduct grid is used in two ways:
 * 1) to mark water/no water (0/1, see map/water_supply.c)
 * 2) to store image IDs for the aqueduct (0-15)
 * This leads to some strange results
 */
static struct grid_u8_t aqueduct;
static struct grid_u8_t aqueduct_backup;

static struct map_point_t bookmarks[MAX_BOOKMARKS];

static struct {
    int end_grid_offset;
    int length;
    int direction;
    int direction_grid_delta;
} bridge;

static struct grid_u16_t buildings_grid;
static struct grid_u8_t damage_grid;
static struct grid_u8_t rubble_type_grid;

static struct grid_i8_t desirability_grid;

struct ring_tile_t {
    int x;
    int y;
    int grid_offset;
};

static struct {
    struct ring_tile_t tiles[1080];
    int index[6][7];
} desirability_data;

struct grid_u16_t map_figures;

struct map_data_t map_data;

static const int DIRECTION_DELTA[] = {
    -OFFSET(0,1), OFFSET(1,-1), 1, OFFSET(1,1), OFFSET(0,1), OFFSET(-1,1), -1, -OFFSET(1,1)
};

static const int ADJACENT_OFFSETS_MAP_GRID[][21] = {
    {0},
    {OFFSET(0,-1), OFFSET(1,0), OFFSET(0,1), OFFSET(-1,0), 0},
    {OFFSET(0,-1), OFFSET(1,-1), OFFSET(2,0), OFFSET(2,1), OFFSET(1,2), OFFSET(0,2), OFFSET(-1,1), OFFSET(-1,0), 0},
    {
        OFFSET(0,-1), OFFSET(1,-1), OFFSET(2,-1),
        OFFSET(3,0), OFFSET(3,1), OFFSET(3,2),
        OFFSET(2,3), OFFSET(1,3), OFFSET(0,3),
        OFFSET(-1,2), OFFSET(-1,1), OFFSET(-1,0), 0
    },
    {
        OFFSET(0,-1), OFFSET(1,-1), OFFSET(2,-1), OFFSET(3,-1),
        OFFSET(4,0), OFFSET(4,1), OFFSET(4,2), OFFSET(4,3),
        OFFSET(3,4), OFFSET(2,4), OFFSET(1,4), OFFSET(0,4),
        OFFSET(-1,3), OFFSET(-1,2), OFFSET(-1,1), OFFSET(-1,0), 0
    },
    {
        OFFSET(0,-1), OFFSET(1,-1), OFFSET(2,-1), OFFSET(3,-1), OFFSET(4,-1),
        OFFSET(5,0), OFFSET(5,1), OFFSET(5,2), OFFSET(5,3), OFFSET(5,4),
        OFFSET(4,5), OFFSET(3,5), OFFSET(2,5), OFFSET(1,5), OFFSET(0,5),
        OFFSET(-1,4), OFFSET(-1,3), OFFSET(-1,2), OFFSET(-1,1), OFFSET(-1,0), 0
    },
};

struct terrain_image_context {
    const unsigned char tiles[MAX_TILES];
    const unsigned char offset_for_orientation[4];
    const unsigned char aqueduct_offset;
    const unsigned char max_item_offset;
    unsigned char current_item_offset;
};

// 0 = no match
// 1 = match
// 2 = don't care

static struct terrain_image_context terrain_images_water[48] = {
    {{1, 2, 1, 2, 1, 2, 1, 2}, {79, 79, 79, 79}, 0, 1, 0},
    {{1, 2, 1, 2, 1, 2, 0, 2}, {47, 46, 45, 44}, 0, 1, 0},
    {{0, 2, 1, 2, 1, 2, 1, 2}, {44, 47, 46, 45}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 2, 1, 2}, {45, 44, 47, 46}, 0, 1, 0},
    {{1, 2, 1, 2, 0, 2, 1, 2}, {46, 45, 44, 47}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 2, 0, 2}, {40, 42, 40, 42}, 0, 2, 0},
    {{0, 2, 1, 2, 0, 2, 1, 2}, {42, 40, 42, 40}, 0, 2, 0},
    {{1, 2, 1, 2, 0, 0, 0, 2}, {32, 28, 24, 36}, 0, 4, 0},
    {{0, 2, 1, 2, 1, 2, 0, 0}, {36, 32, 28, 24}, 0, 4, 0},
    {{0, 0, 0, 2, 1, 2, 1, 2}, {24, 36, 32, 28}, 0, 4, 0},
    {{1, 2, 0, 0, 0, 2, 1, 2}, {28, 24, 36, 32}, 0, 4, 0},
    {{1, 2, 1, 2, 0, 1, 0, 2}, {77, 76, 75, 78}, 0, 1, 0},
    {{0, 2, 1, 2, 1, 2, 0, 1}, {78, 77, 76, 75}, 0, 1, 0},
    {{0, 1, 0, 2, 1, 2, 1, 2}, {75, 78, 77, 76}, 0, 1, 0},
    {{1, 2, 0, 1, 0, 2, 1, 2}, {76, 75, 78, 77}, 0, 1, 0},
    {{1, 2, 0, 0, 0, 0, 0, 2}, {16, 12, 8, 20}, 0, 4, 0},
    {{0, 2, 1, 2, 0, 0, 0, 0}, {20, 16, 12, 8}, 0, 4, 0},
    {{0, 0, 0, 2, 1, 2, 0, 0}, {8, 20, 16, 12}, 0, 4, 0},
    {{0, 0, 0, 0, 0, 2, 1, 2}, {12, 8, 20, 16}, 0, 4, 0},
    {{1, 2, 0, 1, 0, 0, 0, 2}, {69, 66, 63, 72}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 1, 0, 0}, {72, 69, 66, 63}, 0, 1, 0},
    {{0, 0, 0, 2, 1, 2, 0, 1}, {63, 72, 69, 66}, 0, 1, 0},
    {{0, 1, 0, 0, 0, 2, 1, 2}, {66, 63, 72, 69}, 0, 1, 0},
    {{1, 2, 0, 0, 0, 1, 0, 2}, {70, 67, 64, 73}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 0, 0, 1}, {73, 70, 67, 64}, 0, 1, 0},
    {{0, 1, 0, 2, 1, 2, 0, 0}, {64, 73, 70, 67}, 0, 1, 0},
    {{0, 0, 0, 1, 0, 2, 1, 2}, {67, 64, 73, 70}, 0, 1, 0},
    {{1, 2, 0, 1, 0, 1, 0, 2}, {71, 68, 65, 74}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 1, 0, 1}, {74, 71, 68, 65}, 0, 1, 0},
    {{0, 1, 0, 2, 1, 2, 0, 1}, {65, 74, 71, 68}, 0, 1, 0},
    {{0, 1, 0, 1, 0, 2, 1, 2}, {68, 65, 74, 71}, 0, 1, 0},
    {{0, 1, 0, 1, 0, 1, 0, 1}, {62, 62, 62, 62}, 0, 1, 0},
    {{0, 1, 0, 1, 0, 1, 0, 0}, {60, 59, 58, 61}, 0, 1, 0},
    {{0, 0, 0, 1, 0, 1, 0, 1}, {61, 60, 59, 58}, 0, 1, 0},
    {{0, 1, 0, 0, 0, 1, 0, 1}, {58, 61, 60, 59}, 0, 1, 0},
    {{0, 1, 0, 1, 0, 0, 0, 1}, {59, 58, 61, 60}, 0, 1, 0},
    {{0, 1, 0, 0, 0, 1, 0, 0}, {48, 49, 48, 49}, 0, 1, 0},
    {{0, 0, 0, 1, 0, 0, 0, 1}, {49, 48, 49, 48}, 0, 1, 0},
    {{0, 1, 0, 1, 0, 0, 0, 0}, {56, 55, 54, 57}, 0, 1, 0},
    {{0, 0, 0, 1, 0, 1, 0, 0}, {57, 56, 55, 54}, 0, 1, 0},
    {{0, 0, 0, 0, 0, 1, 0, 1}, {54, 57, 56, 55}, 0, 1, 0},
    {{0, 1, 0, 0, 0, 0, 0, 1}, {55, 54, 57, 56}, 0, 1, 0},
    {{0, 1, 0, 0, 0, 0, 0, 0}, {52, 51, 50, 53}, 0, 1, 0},
    {{0, 0, 0, 1, 0, 0, 0, 0}, {53, 52, 51, 50}, 0, 1, 0},
    {{0, 0, 0, 0, 0, 1, 0, 0}, {50, 53, 52, 51}, 0, 1, 0},
    {{0, 0, 0, 0, 0, 0, 0, 1}, {51, 50, 53, 52}, 0, 1, 0},
    {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, 0, 6, 0},
    {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0},
};

static struct terrain_image_context terrain_images_wall[48] = {
    {{1, 2, 1, 2, 1, 2, 1, 2}, {26, 26, 26, 26}, 0, 1, 0},
    {{1, 2, 1, 2, 1, 2, 0, 2}, {15, 10, 5, 16}, 0, 1, 0},
    {{0, 2, 1, 2, 1, 2, 1, 2}, {16, 15, 10, 5}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 2, 1, 2}, {5, 16, 15, 10}, 0, 1, 0},
    {{1, 2, 1, 2, 0, 2, 1, 2}, {10, 5, 16, 15}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 2, 0, 2}, {1, 4, 1, 4}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 2, 1, 2}, {4, 1, 4, 1}, 0, 1, 0},
    {{1, 2, 1, 2, 0, 0, 0, 2}, {10, 7, 5, 12}, 0, 1, 0},
    {{0, 2, 1, 2, 1, 2, 0, 0}, {12, 10, 7, 5}, 0, 1, 0},
    {{0, 0, 0, 2, 1, 2, 1, 2}, {5, 12, 10, 7}, 0, 1, 0},
    {{1, 2, 0, 0, 0, 2, 1, 2}, {7, 5, 12, 10}, 0, 1, 0},
    {{1, 2, 1, 2, 0, 1, 0, 2}, {10, 22, 5, 12}, 0, 1, 0},
    {{0, 2, 1, 2, 1, 2, 0, 1}, {12, 10, 22, 5}, 0, 1, 0},
    {{0, 1, 0, 2, 1, 2, 1, 2}, {5, 12, 10, 22}, 0, 1, 0},
    {{1, 2, 0, 1, 0, 2, 1, 2}, {22, 5, 12, 10}, 0, 1, 0},
    {{1, 2, 0, 0, 0, 0, 0, 2}, {3, 2, 1, 4}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 0, 0, 0}, {4, 3, 2, 1}, 0, 1, 0},
    {{0, 0, 0, 2, 1, 2, 0, 0}, {1, 4, 3, 2}, 0, 1, 0},
    {{0, 0, 0, 0, 0, 2, 1, 2}, {2, 1, 4, 3}, 0, 1, 0},
    {{1, 2, 0, 1, 0, 0, 0, 2}, {22, 24, 1, 4}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 1, 0, 0}, {4, 22, 24, 1}, 0, 1, 0},
    {{0, 0, 0, 2, 1, 2, 0, 1}, {1, 4, 22, 24}, 0, 1, 0},
    {{0, 1, 0, 0, 0, 2, 1, 2}, {24, 1, 4, 22}, 0, 1, 0},
    {{1, 2, 0, 0, 0, 1, 0, 2}, {25, 22, 1, 4}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 0, 0, 1}, {4, 25, 22, 1}, 0, 1, 0},
    {{0, 1, 0, 2, 1, 2, 0, 0}, {1, 4, 25, 22}, 0, 1, 0},
    {{0, 0, 0, 1, 0, 2, 1, 2}, {22, 1, 4, 25}, 0, 1, 0},
    {{1, 2, 0, 1, 0, 1, 0, 2}, {22, 22, 1, 4}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 1, 0, 1}, {4, 22, 22, 1}, 0, 1, 0},
    {{0, 1, 0, 2, 1, 2, 0, 1}, {1, 4, 22, 22}, 0, 1, 0},
    {{0, 1, 0, 1, 0, 2, 1, 2}, {22, 1, 4, 22}, 0, 1, 0},
    {{0, 1, 0, 1, 0, 1, 0, 1}, {22, 22, 22, 22}, 0, 1, 0},
    {{0, 1, 0, 1, 0, 1, 0, 0}, {22, 22, 23, 22}, 0, 1, 0},
    {{0, 0, 0, 1, 0, 1, 0, 1}, {22, 22, 22, 23}, 0, 1, 0},
    {{0, 1, 0, 0, 0, 1, 0, 1}, {23, 22, 22, 22}, 0, 1, 0},
    {{0, 1, 0, 1, 0, 0, 0, 1}, {22, 23, 22, 22}, 0, 1, 0},
    {{0, 1, 0, 0, 0, 1, 0, 0}, {17, 18, 17, 18}, 0, 1, 0},
    {{0, 0, 0, 1, 0, 0, 0, 1}, {18, 17, 18, 17}, 0, 1, 0},
    {{0, 1, 0, 1, 0, 0, 0, 0}, {22, 21, 19, 22}, 0, 1, 0},
    {{0, 0, 0, 1, 0, 1, 0, 0}, {22, 22, 21, 19}, 0, 1, 0},
    {{0, 0, 0, 0, 0, 1, 0, 1}, {19, 22, 22, 21}, 0, 1, 0},
    {{0, 1, 0, 0, 0, 0, 0, 1}, {21, 19, 22, 22}, 0, 1, 0},
    {{0, 1, 0, 0, 0, 0, 0, 0}, {21, 20, 19, 22}, 0, 1, 0},
    {{0, 0, 0, 1, 0, 0, 0, 0}, {22, 21, 20, 19}, 0, 1, 0},
    {{0, 0, 0, 0, 0, 1, 0, 0}, {19, 22, 21, 20}, 0, 1, 0},
    {{0, 0, 0, 0, 0, 0, 0, 1}, {20, 19, 22, 21}, 0, 1, 0},
    {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, 0, 1, 0},
    {{0, 0, 0, 0, 0, 0, 0, 0}, {0, 0, 0, 0}, 0, 0, 0},
};

static struct terrain_image_context terrain_images_wall_gatehouse[10] = {
    {{1, 2, 0, 2, 0, 2, 0, 2}, {16, 15, 10, 5}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 2, 0, 2}, {5, 16, 15, 10}, 0, 1, 0},
    {{0, 2, 0, 2, 1, 2, 0, 2}, {10, 5, 16, 15}, 0, 1, 0},
    {{0, 2, 0, 2, 0, 2, 1, 2}, {15, 10, 5, 16}, 0, 1, 0},
    {{1, 2, 1, 2, 0, 2, 0, 2}, {27, 12, 28, 22}, 0, 1, 0},
    {{0, 2, 1, 2, 1, 2, 0, 2}, {22, 27, 12, 28}, 0, 1, 0},
    {{0, 2, 0, 2, 1, 2, 1, 2}, {28, 22, 27, 12}, 0, 1, 0},
    {{1, 2, 0, 2, 0, 2, 1, 2}, {12, 28, 22, 27}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 2, 0, 2}, {31, 32, 31, 32}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 2, 1, 2}, {32, 31, 32, 31}, 0, 1, 0},
};

static struct terrain_image_context terrain_images_elevation[14] = {
    {{1, 1, 1, 1, 1, 1, 1, 1}, {44, 44, 44, 44}, 2, 1, 0},
    {{1, 1, 1, 1, 1, 0, 1, 1}, {30, 18, 28, 22}, 4, 2, 0},
    {{1, 1, 1, 1, 1, 1, 1, 0}, {22, 30, 18, 28}, 4, 2, 0},
    {{1, 0, 1, 1, 1, 1, 1, 1}, {28, 22, 30, 18}, 4, 2, 0},
    {{1, 1, 1, 0, 1, 1, 1, 1}, {18, 28, 22, 30}, 4, 2, 0},
    {{1, 1, 1, 2, 2, 2, 1, 1}, {0, 8, 12, 4}, 4, 4, 0},
    {{1, 1, 1, 1, 1, 2, 2, 2}, {4, 0, 8, 12}, 4, 4, 0},
    {{2, 2, 1, 1, 1, 1, 1, 2}, {12, 4, 0, 8}, 4, 4, 0},
    {{1, 2, 2, 2, 1, 1, 1, 1}, {8, 12, 4, 0}, 4, 4, 0},
    {{1, 1, 1, 2, 2, 2, 2, 2}, {24, 16, 26, 20}, 4, 2, 0},
    {{2, 2, 1, 1, 1, 2, 2, 2}, {20, 24, 16, 26}, 4, 2, 0},
    {{2, 2, 2, 2, 1, 1, 1, 2}, {26, 20, 24, 16}, 4, 2, 0},
    {{1, 2, 2, 2, 2, 2, 1, 1}, {16, 26, 20, 24}, 4, 2, 0},
    {{2, 2, 2, 2, 2, 2, 2, 2}, {32, 32, 32, 32}, 4, 4, 0},
};

static struct terrain_image_context terrain_images_earthquake[17] = {
    {{1, 2, 1, 2, 1, 2, 1, 2}, {29, 29, 29, 29}, 0, 1, 0},
    {{1, 2, 1, 2, 1, 2, 0, 2}, {25, 28, 27, 26}, 0, 1, 0},
    {{0, 2, 1, 2, 1, 2, 1, 2}, {26, 25, 28, 27}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 2, 1, 2}, {27, 26, 25, 28}, 0, 1, 0},
    {{1, 2, 1, 2, 0, 2, 1, 2}, {28, 27, 26, 25}, 0, 1, 0},
    {{1, 2, 1, 2, 0, 2, 0, 2}, {8, 14, 12, 10}, 0, 2, 0},
    {{0, 2, 1, 2, 1, 2, 0, 2}, {10, 8, 14, 12}, 0, 2, 0},
    {{0, 2, 0, 2, 1, 2, 1, 2}, {12, 10, 8, 14}, 0, 2, 0},
    {{1, 2, 0, 2, 0, 2, 1, 2}, {14, 12, 10, 8}, 0, 2, 0},
    {{1, 2, 0, 2, 1, 2, 0, 2}, {0, 4, 0, 4}, 0, 4, 0},
    {{0, 2, 1, 2, 0, 2, 1, 2}, {4, 0, 4, 0}, 0, 4, 0},
    {{1, 2, 0, 2, 0, 2, 0, 2}, {16, 22, 18, 20}, 0, 2, 0},
    {{0, 2, 1, 2, 0, 2, 0, 2}, {20, 16, 22, 18}, 0, 2, 0},
    {{0, 2, 0, 2, 1, 2, 0, 2}, {18, 20, 16, 22}, 0, 2, 0},
    {{0, 2, 0, 2, 0, 2, 1, 2}, {22, 18, 20, 16}, 0, 2, 0},
    {{0, 2, 0, 2, 0, 2, 0, 2}, {24, 24, 24, 24}, 0, 1, 0},
    {{2, 2, 2, 2, 2, 2, 2, 2}, {24, 24, 24, 24}, 0, 1, 0},
};

static struct terrain_image_context terrain_images_dirt_road[17] = {
    {{1, 2, 1, 2, 1, 2, 1, 2}, {17, 17, 17, 17}, 0, 1, 0},
    {{1, 2, 1, 2, 1, 2, 0, 2}, {13, 16, 15, 14}, 0, 1, 0},
    {{0, 2, 1, 2, 1, 2, 1, 2}, {14, 13, 16, 15}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 2, 1, 2}, {15, 14, 13, 16}, 0, 1, 0},
    {{1, 2, 1, 2, 0, 2, 1, 2}, {16, 15, 14, 13}, 0, 1, 0},
    {{1, 2, 1, 2, 0, 2, 0, 2}, {4, 7, 6, 5}, 0, 1, 0},
    {{0, 2, 1, 2, 1, 2, 0, 2}, {5, 4, 7, 6}, 0, 1, 0},
    {{0, 2, 0, 2, 1, 2, 1, 2}, {6, 5, 4, 7}, 0, 1, 0},
    {{1, 2, 0, 2, 0, 2, 1, 2}, {7, 6, 5, 4}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 2, 0, 2}, {0, 1, 0, 1}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 2, 1, 2}, {1, 0, 1, 0}, 0, 1, 0},
    {{1, 2, 0, 2, 0, 2, 0, 2}, {8, 11, 10, 9}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 2, 0, 2}, {9, 8, 11, 10}, 0, 1, 0},
    {{0, 2, 0, 2, 1, 2, 0, 2}, {10, 9, 8, 11}, 0, 1, 0},
    {{0, 2, 0, 2, 0, 2, 1, 2}, {11, 10, 9, 8}, 0, 1, 0},
    {{0, 2, 0, 2, 0, 2, 0, 2}, {12, 12, 12, 12}, 0, 1, 0},
    {{2, 2, 2, 2, 2, 2, 2, 2}, {12, 12, 12, 12}, 0, 1, 0},
};

static struct terrain_image_context terrain_images_paved_road[48] = {
    {{1, 0, 1, 0, 1, 0, 1, 0}, {17, 17, 17, 17}, 0, 1, 0},
    {{1, 0, 1, 0, 1, 2, 0, 2}, {13, 16, 15, 14}, 0, 1, 0},
    {{1, 1, 1, 1, 1, 2, 0, 2}, {18, 21, 20, 19}, 0, 1, 0},
    {{1, 0, 1, 1, 1, 2, 0, 2}, {26, 33, 32, 31}, 0, 1, 0},
    {{1, 1, 1, 0, 1, 2, 0, 2}, {30, 29, 28, 27}, 0, 1, 0},
    {{0, 2, 1, 0, 1, 0, 1, 2}, {14, 13, 16, 15}, 0, 1, 0},
    {{0, 2, 1, 1, 1, 1, 1, 2}, {19, 18, 21, 20}, 0, 1, 0},
    {{0, 2, 1, 0, 1, 1, 1, 2}, {31, 26, 33, 32}, 0, 1, 0},
    {{0, 2, 1, 1, 1, 0, 1, 2}, {27, 30, 29, 28}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 0, 1, 0}, {15, 14, 13, 16}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 1, 1, 1}, {20, 19, 18, 21}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 0, 1, 1}, {32, 31, 26, 33}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 1, 1, 0}, {28, 27, 30, 29}, 0, 1, 0},
    {{1, 0, 1, 2, 0, 2, 1, 0}, {16, 15, 14, 13}, 0, 1, 0},
    {{1, 1, 1, 2, 0, 2, 1, 1}, {21, 20, 19, 18}, 0, 1, 0},
    {{1, 1, 1, 2, 0, 2, 1, 0}, {33, 32, 31, 26}, 0, 1, 0},
    {{1, 0, 1, 2, 0, 2, 1, 1}, {29, 28, 27, 30}, 0, 1, 0},
    {{1, 1, 1, 2, 0, 0, 0, 2}, {22, 25, 24, 23}, 0, 1, 0},
    {{0, 2, 1, 1, 1, 2, 0, 0}, {23, 22, 25, 24}, 0, 1, 0},
    {{0, 0, 0, 2, 1, 1, 1, 2}, {24, 23, 22, 25}, 0, 1, 0},
    {{1, 2, 0, 0, 0, 2, 1, 1}, {25, 24, 23, 22}, 0, 1, 0},
    {{1, 0, 1, 0, 1, 1, 1, 1}, {34, 37, 36, 35}, 0, 1, 0},
    {{1, 1, 1, 0, 1, 0, 1, 1}, {35, 34, 37, 36}, 0, 1, 0},
    {{1, 1, 1, 1, 1, 0, 1, 0}, {36, 35, 34, 37}, 0, 1, 0},
    {{1, 0, 1, 1, 1, 1, 1, 0}, {37, 36, 35, 34}, 0, 1, 0},
    {{1, 0, 1, 0, 1, 0, 1, 1}, {38, 41, 40, 39}, 0, 1, 0},
    {{1, 1, 1, 0, 1, 0, 1, 0}, {39, 38, 41, 40}, 0, 1, 0},
    {{1, 0, 1, 1, 1, 0, 1, 0}, {40, 39, 38, 41}, 0, 1, 0},
    {{1, 0, 1, 0, 1, 1, 1, 0}, {41, 40, 39, 38}, 0, 1, 0},
    {{1, 1, 1, 1, 1, 0, 1, 1}, {42, 45, 44, 43}, 0, 1, 0},
    {{1, 1, 1, 1, 1, 1, 1, 0}, {43, 42, 45, 44}, 0, 1, 0},
    {{1, 0, 1, 1, 1, 1, 1, 1}, {44, 43, 42, 45}, 0, 1, 0},
    {{1, 1, 1, 0, 1, 1, 1, 1}, {45, 44, 43, 42}, 0, 1, 0},
    {{1, 1, 1, 0, 1, 1, 1, 0}, {46, 47, 46, 47}, 0, 1, 0},
    {{1, 0, 1, 1, 1, 0, 1, 1}, {47, 46, 47, 46}, 0, 1, 0},
    {{1, 2, 1, 2, 0, 2, 0, 2}, {4, 7, 6, 5}, 0, 1, 0},
    {{0, 2, 1, 2, 1, 2, 0, 2}, {5, 4, 7, 6}, 0, 1, 0},
    {{0, 2, 0, 2, 1, 2, 1, 2}, {6, 5, 4, 7}, 0, 1, 0},
    {{1, 2, 0, 2, 0, 2, 1, 2}, {7, 6, 5, 4}, 0, 1, 0},
    {{1, 2, 0, 2, 1, 2, 0, 2}, {0, 1, 0, 1}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 2, 1, 2}, {1, 0, 1, 0}, 0, 1, 0},
    {{1, 2, 0, 2, 0, 2, 0, 2}, {8, 11, 10, 9}, 0, 1, 0},
    {{0, 2, 1, 2, 0, 2, 0, 2}, {9, 8, 11, 10}, 0, 1, 0},
    {{0, 2, 0, 2, 1, 2, 0, 2}, {10, 9, 8, 11}, 0, 1, 0},
    {{0, 2, 0, 2, 0, 2, 1, 2}, {11, 10, 9, 8}, 0, 1, 0},
    {{0, 0, 0, 0, 0, 0, 0, 0}, {12, 12, 12, 12}, 0, 1, 0},
    {{1, 1, 1, 1, 1, 1, 1, 1}, {48, 48, 48, 48}, 0, 1, 0},
    {{2, 2, 2, 2, 2, 2, 2, 2}, {12, 12, 12, 12}, 0, 1, 0},
};

static struct terrain_image_context terrain_images_aqueduct[16] = {
    {{1, 2, 1, 2, 0, 2, 0, 2}, {4, 7, 6, 5}, 7, 1, 0},
    {{0, 2, 1, 2, 1, 2, 0, 2}, {5, 4, 7, 6}, 8, 1, 0},
    {{0, 2, 0, 2, 1, 2, 1, 2}, {6, 5, 4, 7}, 9, 1, 0},
    {{1, 2, 0, 2, 0, 2, 1, 2}, {7, 6, 5, 4}, 10, 1, 0},
    {{1, 2, 0, 2, 1, 2, 0, 2}, {2, 3, 2, 3}, 5, 1, 0},
    {{0, 2, 1, 2, 0, 2, 1, 2}, {3, 2, 3, 2}, 6, 1, 0},
    {{1, 2, 0, 2, 0, 2, 0, 2}, {2, 3, 2, 3}, 1, 1, 0},
    {{0, 2, 1, 2, 0, 2, 0, 2}, {3, 2, 3, 2}, 2, 1, 0},
    {{0, 2, 0, 2, 1, 2, 0, 2}, {2, 3, 2, 3}, 3, 1, 0},
    {{0, 2, 0, 2, 0, 2, 1, 2}, {3, 2, 3, 2}, 4, 1, 0},
    {{1, 2, 1, 2, 1, 2, 0, 2}, {10, 13, 12, 11}, 11, 1, 0},
    {{0, 2, 1, 2, 1, 2, 1, 2}, {11, 10, 13, 12}, 12, 1, 0},
    {{1, 2, 0, 2, 1, 2, 1, 2}, {12, 11, 10, 13}, 13, 1, 0},
    {{1, 2, 1, 2, 0, 2, 1, 2}, {13, 12, 11, 10}, 14, 1, 0},
    {{1, 2, 1, 2, 1, 2, 1, 2}, {14, 14, 14, 14}, 15, 1, 0},
    {{2, 2, 2, 2, 2, 2, 2, 2}, {2, 2, 2, 2}, 0, 1, 0},
};

enum {
    CONTEXT_WATER,
    CONTEXT_WALL,
    CONTEXT_WALL_GATEHOUSE,
    CONTEXT_ELEVATION,
    CONTEXT_EARTHQUAKE,
    CONTEXT_DIRT_ROAD,
    CONTEXT_PAVED_ROAD,
    CONTEXT_AQUEDUCT,
    CONTEXT_MAX_ITEMS
};

static struct {
    struct terrain_image_context *context;
    int size;
} context_pointers[] = {
    {terrain_images_water, 48},
    {terrain_images_wall, 48},
    {terrain_images_wall_gatehouse, 10},
    {terrain_images_elevation, 14},
    {terrain_images_earthquake, 17},
    {terrain_images_dirt_road, 17},
    {terrain_images_paved_road, 48},
    {terrain_images_aqueduct, 16}
};

static struct grid_u16_t images;
static struct grid_u16_t images_backup;

enum {
    BIT_SIZE1 = 0x00,
    BIT_SIZE2 = 0x01,
    BIT_SIZE3 = 0x02,
    BIT_SIZE4 = 0x04,
    BIT_SIZE5 = 0x08,
    BIT_SIZES = 0x0f,
    BIT_NO_SIZES = 0xf0,
    BIT_CONSTRUCTION = 0x10,
    BIT_NO_CONSTRUCTION = 0xef,
    BIT_ALTERNATE_TERRAIN = 0x20,
    BIT_DELETED = 0x40,
    BIT_NO_DELETED = 0xbf,
    BIT_PLAZA_OR_EARTHQUAKE = 0x80,
    BIT_NO_PLAZA = 0x7f,
    BIT_NO_CONSTRUCTION_AND_DELETED = 0xaf,
    EDGE_MASK_X = 0x7,
    EDGE_MASK_Y = 0x38,
    EDGE_MASK_XY = 0x3f,
    EDGE_LEFTMOST_TILE = 0x40,
    EDGE_NO_LEFTMOST_TILE = 0xbf,
    EDGE_NATIVE_LAND = 0x80,
    EDGE_NO_NATIVE_LAND = 0x7f,
};

static struct grid_u8_t edge_grid;
static struct grid_u8_t bitfields_grid;

static struct grid_u8_t edge_backup;
static struct grid_u8_t bitfields_backup;

static struct grid_u8_t random;

static const int ADJACENT_OFFSETS[] = { -GRID_SIZE, 1, GRID_SIZE, -1 };

static struct grid_u8_t network;

static struct {
    int items[MAX_QUEUE];
    int head;
    int tail;
} road_network_queue;

struct grid_i8_t terrain_land_citizen;
struct grid_i8_t terrain_land_noncitizen;
struct grid_i8_t terrain_water;
struct grid_i8_t terrain_walls;

static int direction_path[MAX_PATH];

static const int ROUTE_OFFSETS[] = { -162, 1, 162, -1, -161, 163, 161, -163 };

static struct grid_i16_t routing_distance;

static struct {
    int total_routes_calculated;
    int enemy_routes_calculated;
} stats = { 0, 0 };

static struct {
    int head;
    int tail;
    int items[MAX_QUEUE_ROUTING];
} routing_queue;

static struct grid_u8_t water_drag;

static int route_through_building_id;

static struct grid_u8_t sprite;
static struct grid_u8_t sprite_backup;

struct grid_u16_t terrain_grid;
static struct grid_u16_t terrain_grid_backup;

struct grid_u8_t terrain_elevation;

static int aqueduct_include_construction = 0;

static struct {
    int items[MAX_QUEUE];
    int head;
    int tail;
} water_supply_queue;

int map_aqueduct_at(int grid_offset)
{
    return aqueduct.items[grid_offset];
}

void map_aqueduct_set(int grid_offset, int value)
{
    aqueduct.items[grid_offset] = value;
}

void map_aqueduct_remove(int grid_offset)
{
    aqueduct.items[grid_offset] = 0;
    if (aqueduct.items[grid_offset + map_grid_delta(0, -1)] == 5) {
        aqueduct.items[grid_offset + map_grid_delta(0, -1)] = 1;
    }
    if (aqueduct.items[grid_offset + map_grid_delta(1, 0)] == 6) {
        aqueduct.items[grid_offset + map_grid_delta(1, 0)] = 2;
    }
    if (aqueduct.items[grid_offset + map_grid_delta(0, 1)] == 5) {
        aqueduct.items[grid_offset + map_grid_delta(0, 1)] = 3;
    }
    if (aqueduct.items[grid_offset + map_grid_delta(-1, 0)] == 6) {
        aqueduct.items[grid_offset + map_grid_delta(-1, 0)] = 4;
    }
}

void map_aqueduct_clear(void)
{
    memset(aqueduct.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

void map_aqueduct_backup(void)
{
    memcpy(aqueduct_backup.items, aqueduct.items, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

void map_aqueduct_restore(void)
{
    memcpy(aqueduct.items, aqueduct_backup.items, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

void map_aqueduct_save_state(struct buffer_t *buf, struct buffer_t *backup)
{
    buffer_write_raw(buf, aqueduct.items, GRID_SIZE * GRID_SIZE);
    buffer_write_raw(backup, aqueduct_backup.items, GRID_SIZE * GRID_SIZE);
}

void map_aqueduct_load_state(struct buffer_t *buf, struct buffer_t *backup)
{
    buffer_read_raw(buf, aqueduct.items, GRID_SIZE * GRID_SIZE);
    buffer_read_raw(backup, aqueduct_backup.items, GRID_SIZE * GRID_SIZE);
}

void map_bookmarks_clear(void)
{
    for (int i = 0; i < MAX_BOOKMARKS; i++) {
        bookmarks[i].x = -1;
        bookmarks[i].y = -1;
    }
}

void map_bookmark_save(int number)
{
    if (number >= 0 && number < MAX_BOOKMARKS) {
        city_view_get_camera(&bookmarks[number].x, &bookmarks[number].y);
    }
}

int map_bookmark_go_to(int number)
{
    if (number >= 0 && number < MAX_BOOKMARKS) {
        int x = bookmarks[number].x;
        int y = bookmarks[number].y;
        if (x > -1 && map_grid_offset(x, y) > -1) {
            city_view_set_camera(x, y);
            return 1;
        }
    }
    return 0;
}

void map_bookmark_save_state(struct buffer_t *buf)
{
    for (int i = 0; i < MAX_BOOKMARKS; i++) {
        buffer_write_i32(buf, bookmarks[i].x);
        buffer_write_i32(buf, bookmarks[i].y);
    }
}

void map_bookmark_load_state(struct buffer_t *buf)
{
    for (int i = 0; i < MAX_BOOKMARKS; i++) {
        bookmarks[i].x = buffer_read_i32(buf);
        bookmarks[i].y = buffer_read_i32(buf);
    }
}

int map_bridge_building_length(void)
{
    return bridge.length;
}

void map_bridge_reset_building_length(void)
{
    bridge.length = 0;
}

int map_bridge_calculate_length_direction(int x, int y, int *length, int *direction)
{
    int grid_offset = map_grid_offset(x, y);
    bridge.end_grid_offset = 0;
    bridge.direction_grid_delta = 0;
    bridge.length = *length = 0;
    bridge.direction = *direction = 0;

    if (!map_terrain_is(grid_offset, TERRAIN_WATER)) {
        return 0;
    }
    if (map_terrain_is(grid_offset, TERRAIN_ROAD | TERRAIN_BUILDING)) {
        return 0;
    }
    if (map_terrain_count_directly_adjacent_with_type(grid_offset, TERRAIN_WATER) != 3) {
        return 0;
    }
    if (!map_terrain_is(grid_offset + map_grid_delta(0, -1), TERRAIN_WATER)) {
        bridge.direction_grid_delta = map_grid_delta(0, 1);
        bridge.direction = DIR_4_BOTTOM;
    } else if (!map_terrain_is(grid_offset + map_grid_delta(1, 0), TERRAIN_WATER)) {
        bridge.direction_grid_delta = map_grid_delta(-1, 0);
        bridge.direction = DIR_6_LEFT;
    } else if (!map_terrain_is(grid_offset + map_grid_delta(0, 1), TERRAIN_WATER)) {
        bridge.direction_grid_delta = map_grid_delta(0, -1);
        bridge.direction = DIR_0_TOP;
    } else if (!map_terrain_is(grid_offset + map_grid_delta(-1, 0), TERRAIN_WATER)) {
        bridge.direction_grid_delta = map_grid_delta(1, 0);
        bridge.direction = DIR_2_RIGHT;
    } else {
        return 0;
    }
    *direction = bridge.direction;
    bridge.length = 1;
    for (int i = 0; i < 40; i++) {
        grid_offset += bridge.direction_grid_delta;
        bridge.length++;
        int next_offset = grid_offset + bridge.direction_grid_delta;
        if (map_terrain_is(next_offset, TERRAIN_SHRUB)) {
            break;
        }
        if (!map_terrain_is(next_offset, TERRAIN_WATER)) {
            bridge.end_grid_offset = grid_offset;
            if (map_terrain_count_directly_adjacent_with_type(grid_offset, TERRAIN_WATER) != 3) {
                bridge.end_grid_offset = 0;
            }
            *length = bridge.length;
            return bridge.end_grid_offset;
        }
        if (map_terrain_is(next_offset, TERRAIN_ROAD | TERRAIN_BUILDING)) {
            break;
        }
        int count = 0;
        if (map_terrain_is(grid_offset + map_grid_delta(1, -1), TERRAIN_WATER)) {
            count++;
        }
        if (map_terrain_is(grid_offset + map_grid_delta(1, 1), TERRAIN_WATER)) {
            count++;
        }
        if (map_terrain_is(grid_offset + map_grid_delta(-1, 1), TERRAIN_WATER)) {
            count++;
        }
        if (map_terrain_is(grid_offset + map_grid_delta(-1, -1), TERRAIN_WATER)) {
            count++;
        }
        if (count != 4) {
            break;
        }
    }
    // invalid bridge
    *length = bridge.length;
    return 0;
}

int map_bridge_get_sprite_id(int index, int length, int direction, int is_ship_bridge)
{
    if (is_ship_bridge) {
        int pillar_distance = 0;
        switch (bridge.length) {
            case 9:
            case 10:
                pillar_distance = 4;
                break;
            case 11:
            case 12:
                pillar_distance = 5;
                break;
            case 13:
            case 14:
                pillar_distance = 6;
                break;
            case 15:
            case 16:
                pillar_distance = 7;
                break;
            default:
                pillar_distance = 8;
                break;
        }
        if (index == 1 || index == length - 2) {
            // platform after ramp
            return 13;
        } else if (index == 0) {
            // ramp at start
            switch (direction) {
                case DIR_0_TOP:
                    return 7;
                case DIR_2_RIGHT:
                    return 8;
                case DIR_4_BOTTOM:
                    return 9;
                case DIR_6_LEFT:
                    return 10;
            }
        } else if (index == length - 1) {
            // ramp at end
            switch (direction) {
                case DIR_0_TOP:
                    return 9;
                case DIR_2_RIGHT:
                    return 10;
                case DIR_4_BOTTOM:
                    return 7;
                case DIR_6_LEFT:
                    return 8;
            }
        } else if (index == pillar_distance) {
            if (direction == DIR_0_TOP || direction == DIR_4_BOTTOM) {
                return 14;
            } else {
                return 15;
            }
        } else {
            // middle of the bridge
            if (direction == DIR_0_TOP || direction == DIR_4_BOTTOM) {
                return 11;
            } else {
                return 12;
            }
        }
    } else {
        if (index == 0) {
            // ramp at start
            switch (direction) {
                case DIR_0_TOP:
                    return 1;
                case DIR_2_RIGHT:
                    return 2;
                case DIR_4_BOTTOM:
                    return 3;
                case DIR_6_LEFT:
                    return 4;
            }
        } else if (index == length - 1) {
            // ramp at end
            switch (direction) {
                case DIR_0_TOP:
                    return 3;
                case DIR_2_RIGHT:
                    return 4;
                case DIR_4_BOTTOM:
                    return 1;
                case DIR_6_LEFT:
                    return 2;
            }
        } else {
            // middle part
            if (direction == DIR_0_TOP || direction == DIR_4_BOTTOM) {
                return 5;
            } else {
                return 6;
            }
        }
    }
    return 0;
}

int map_bridge_add(int x, int y, int is_ship_bridge)
{
    int min_length = is_ship_bridge ? 5 : 2;
    if (bridge.end_grid_offset <= 0 || bridge.length < min_length) {
        bridge.length = 0;
        return bridge.length;
    }

    bridge.direction -= city_view_orientation();
    if (bridge.direction < 0) {
        bridge.direction += 8;
    }

    int grid_offset = map_grid_offset(x, y);
    for (int i = 0; i < bridge.length; i++) {
        terrain_grid.items[grid_offset] |= TERRAIN_ROAD;
        int value = map_bridge_get_sprite_id(i, bridge.length, bridge.direction, is_ship_bridge);
        map_sprite_bridge_set(grid_offset, value);
        grid_offset += bridge.direction_grid_delta;
    }

    map_routing_update_land();
    map_routing_update_water();

    return bridge.length;
}

int map_is_bridge(int grid_offset)
{
    return map_terrain_is(grid_offset, TERRAIN_WATER) && map_sprite_bridge_at(grid_offset);
}

static int get_y_bridge_tiles(int grid_offset)
{
    int tiles = 0;
    if (map_is_bridge(grid_offset + map_grid_delta(0, -1))) {
        tiles++;
    }
    if (map_is_bridge(grid_offset + map_grid_delta(0, -2))) {
        tiles++;
    }
    if (map_is_bridge(grid_offset + map_grid_delta(0, 1))) {
        tiles++;
    }
    if (map_is_bridge(grid_offset + map_grid_delta(0, 2))) {
        tiles++;
    }
    return tiles;
}

static int get_x_bridge_tiles(int grid_offset)
{
    int tiles = 0;
    if (map_is_bridge(grid_offset + map_grid_delta(-1, 0))) {
        tiles++;
    }
    if (map_is_bridge(grid_offset + map_grid_delta(-2, 0))) {
        tiles++;
    }
    if (map_is_bridge(grid_offset + map_grid_delta(1, 0))) {
        tiles++;
    }
    if (map_is_bridge(grid_offset + map_grid_delta(2, 0))) {
        tiles++;
    }
    return tiles;
}

void map_bridge_remove(int grid_offset, int mark_deleted)
{
    if (!map_is_bridge(grid_offset)) {
        return;
    }

    int tiles_x = get_x_bridge_tiles(grid_offset);
    int tiles_y = get_y_bridge_tiles(grid_offset);

    int offset_up = tiles_x > tiles_y ? map_grid_delta(1, 0) : map_grid_delta(0, 1);
    // find lower end of the bridge
    while (map_is_bridge(grid_offset - offset_up)) {
        grid_offset -= offset_up;
    }

    if (mark_deleted) {
        map_property_mark_deleted(grid_offset);
    } else {
        map_sprite_clear_tile(grid_offset);
        terrain_grid.items[grid_offset] &= ~TERRAIN_ROAD;
    }
    while (map_is_bridge(grid_offset + offset_up)) {
        grid_offset += offset_up;
        if (mark_deleted) {
            map_property_mark_deleted(grid_offset);
        } else {
            map_sprite_clear_tile(grid_offset);
            terrain_grid.items[grid_offset] &= ~TERRAIN_ROAD;
        }
    }
}

int map_bridge_count_figures(int grid_offset)
{
    if (!map_is_bridge(grid_offset)) {
        return 0;
    }
    int tiles_x = get_x_bridge_tiles(grid_offset);
    int tiles_y = get_y_bridge_tiles(grid_offset);

    int offset_up = tiles_x > tiles_y ? map_grid_delta(1, 0) : map_grid_delta(0, 1);
    // find lower end of the bridge
    while (map_is_bridge(grid_offset - offset_up)) {
        grid_offset -= offset_up;
    }

    int figures = 0;
    if (map_has_figure_at(grid_offset)) {
        figures = 1;
    }
    while (map_is_bridge(grid_offset + offset_up)) {
        grid_offset += offset_up;
        if (map_has_figure_at(grid_offset)) {
            figures++;
        }
    }
    return figures;
}

void map_building_tiles_add(int building_id, int x, int y, int size, int image_id, int terrain)
{
    if (!map_grid_is_inside(x, y, size)) {
        return;
    }
    int x_leftmost, y_leftmost;
    switch (city_view_orientation()) {
        case DIR_0_TOP:
            x_leftmost = 0;
            y_leftmost = size - 1;
            break;
        case DIR_2_RIGHT:
            x_leftmost = y_leftmost = 0;
            break;
        case DIR_4_BOTTOM:
            x_leftmost = size - 1;
            y_leftmost = 0;
            break;
        case DIR_6_LEFT:
            x_leftmost = y_leftmost = size - 1;
            break;
        default:
            return;
    }
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            terrain_grid.items[grid_offset] &= ~TERRAIN_CLEARABLE;
            terrain_grid.items[grid_offset] |= terrain;
            map_building_set(grid_offset, building_id);
            map_property_clear_constructing(grid_offset);
            map_property_set_multi_tile_size(grid_offset, size);
            map_image_set(grid_offset, image_id);
            map_property_set_multi_tile_xy(grid_offset, dx, dy, dx == x_leftmost && dy == y_leftmost);
        }
    }
}

static void set_crop_tile(int building_id, int x, int y, int dx, int dy, int crop_image_id, int growth)
{
    int grid_offset = map_grid_offset(x + dx, y + dy);
    terrain_grid.items[grid_offset] &= ~TERRAIN_CLEARABLE;
    terrain_grid.items[grid_offset] |= TERRAIN_BUILDING;
    map_building_set(grid_offset, building_id);
    map_property_clear_constructing(grid_offset);
    map_property_set_multi_tile_xy(grid_offset, dx, dy, 1);
    map_image_set(grid_offset, crop_image_id + (growth < 4 ? growth : 4));
}

void map_building_tiles_add_farm(int building_id, int x, int y, int crop_image_id, int progress)
{
    if (!map_grid_is_inside(x, y, 3)) {
        return;
    }
    // farmhouse
    int x_leftmost, y_leftmost;
    switch (city_view_orientation()) {
        case DIR_0_TOP:
            x_leftmost = 0;
            y_leftmost = 1;
            break;
        case DIR_2_RIGHT:
            x_leftmost = 0;
            y_leftmost = 0;
            break;
        case DIR_4_BOTTOM:
            x_leftmost = 1;
            y_leftmost = 0;
            break;
        case DIR_6_LEFT:
            x_leftmost = 1;
            y_leftmost = 1;
            break;
        default:
            return;
    }
    for (int dy = 0; dy < 2; dy++) {
        for (int dx = 0; dx < 2; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            terrain_grid.items[grid_offset] &= ~TERRAIN_CLEARABLE;
            terrain_grid.items[grid_offset] |= TERRAIN_BUILDING;
            map_building_set(grid_offset, building_id);
            map_property_clear_constructing(grid_offset);
            map_property_set_multi_tile_size(grid_offset, 2);
            map_image_set(grid_offset, image_group(GROUP_BUILDING_FARM_HOUSE));
            map_property_set_multi_tile_xy(grid_offset, dx, dy,
                dx == x_leftmost && dy == y_leftmost);
        }
    }
    // crop tile 1
    int growth = progress / 10;
    set_crop_tile(building_id, x, y, 0, 2, crop_image_id, growth);

    // crop tile 2
    growth -= 4;
    if (growth < 0) {
        growth = 0;
    }
    set_crop_tile(building_id, x, y, 1, 2, crop_image_id, growth);

    // crop tile 3
    growth -= 4;
    if (growth < 0) {
        growth = 0;
    }
    set_crop_tile(building_id, x, y, 2, 2, crop_image_id, growth);

    // crop tile 4
    growth -= 4;
    if (growth < 0) {
        growth = 0;
    }
    set_crop_tile(building_id, x, y, 2, 1, crop_image_id, growth);

    // crop tile 5
    growth -= 4;
    if (growth < 0) {
        growth = 0;
    }
    set_crop_tile(building_id, x, y, 2, 0, crop_image_id, growth);
}

static void fill_matches(int grid_offset, int terrain, int match_value, int no_match_value, int tiles[MAX_TILES])
{
    for (int i = 0; i < MAX_TILES; i++) {
        tiles[i] = map_terrain_is(grid_offset + map_grid_direction_delta(i), terrain) ? match_value : no_match_value;
    }
}

static int context_matches_tiles(const struct terrain_image_context *context, const int tiles[MAX_TILES])
{
    for (int i = 0; i < MAX_TILES; i++) {
        if (context->tiles[i] != 2 && tiles[i] != context->tiles[i]) {
            return 0;
        }
    }
    return 1;
}

static const struct terrain_image_t *get_image(int group, int tiles[MAX_TILES])
{
    static struct terrain_image_t result;

    result.is_valid = 0;
    struct terrain_image_context *context = context_pointers[group].context;
    int size = context_pointers[group].size;
    for (int i = 0; i < size; i++) {
        if (context_matches_tiles(&context[i], tiles)) {
            context[i].current_item_offset++;
            if (context[i].current_item_offset >= context[i].max_item_offset) {
                context[i].current_item_offset = 0;
            }
            result.is_valid = 1;
            result.group_offset = context[i].offset_for_orientation[city_view_orientation() / 2];
            result.item_offset = context[i].current_item_offset;
            result.aqueduct_offset = context[i].aqueduct_offset;
            break;
        }
    }
    return &result;
}

static void set_water_image(int x, int y, int grid_offset)
{
    if ((terrain_grid.items[grid_offset] & (TERRAIN_WATER | TERRAIN_BUILDING)) == TERRAIN_WATER) {
        int tiles[MAX_TILES];
        fill_matches(grid_offset, TERRAIN_WATER, 0, 1, tiles);
        const struct terrain_image_t *img = get_image(CONTEXT_WATER, tiles);
        int image_id = image_group(GROUP_TERRAIN_WATER) + img->group_offset + img->item_offset;
        if (map_terrain_exists_tile_in_radius_with_type(x, y, 1, 2, TERRAIN_BUILDING)) {
            // fortified shore
            int base = image_group(GROUP_TERRAIN_WATER_SHORE);
            switch (img->group_offset) {
                case 8: image_id = base + 10; break;
                case 12: image_id = base + 11; break;
                case 16: image_id = base + 9; break;
                case 20: image_id = base + 8; break;
                case 24: image_id = base + 18; break;
                case 28: image_id = base + 16; break;
                case 32: image_id = base + 19; break;
                case 36: image_id = base + 17; break;
                case 50: image_id = base + 12; break;
                case 51: image_id = base + 14; break;
                case 52: image_id = base + 13; break;
                case 53: image_id = base + 15; break;
            }
        }
        map_image_set(grid_offset, image_id);
    }
}

static void map_tiles_set_water(int x, int y)
{
    terrain_grid.items[map_grid_offset(x, y)] |= TERRAIN_WATER;
    foreach_region_tile(x - 1, y - 1, x + 1, y + 1, set_water_image);
}

void map_building_tiles_remove(int building_id, int x, int y)
{
    if (!map_grid_is_inside(x, y, 1)) {
        return;
    }
    int size;
    int base_grid_offset = map_grid_offset(x, y);
    size = map_property_multi_tile_size(base_grid_offset);
    for (int i = 0; i < size && map_property_multi_tile_x(base_grid_offset); i++) {
        base_grid_offset += map_grid_delta(-1, 0);
    }
    for (int i = 0; i < size && map_property_multi_tile_y(base_grid_offset); i++) {
        base_grid_offset += map_grid_delta(0, -1);
    }
    x = map_grid_offset_to_x(base_grid_offset);
    y = map_grid_offset_to_y(base_grid_offset);
    if (terrain_grid.items[base_grid_offset] == TERRAIN_ROCK) {
        return;
    }
    struct building_t *b = &all_buildings[building_id];
    if (building_id && building_is_farm(b->type)) {
        size = 3;
    }
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            if (building_id && map_building_at(grid_offset) != building_id) {
                continue;
            }
            if (building_id && b->type != BUILDING_BURNING_RUIN) {
                map_set_rubble_building_type(grid_offset, b->type);
            }
            map_property_clear_constructing(grid_offset);
            map_property_set_multi_tile_size(grid_offset, 1);
            map_property_clear_multi_tile_xy(grid_offset);
            map_property_mark_draw_tile(grid_offset);
            map_aqueduct_set(grid_offset, 0);
            map_building_set(grid_offset, 0);
            map_building_damage_clear(grid_offset);
            map_sprite_clear_tile(grid_offset);
            if (map_terrain_is(grid_offset, TERRAIN_WATER)) {
                terrain_grid.items[grid_offset] = TERRAIN_WATER; // clear other flags
                map_tiles_set_water(x + dx, y + dy);
            } else {
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_UGLY_GRASS) + (map_random_get(grid_offset) & 7));
                terrain_grid.items[grid_offset] &= ~TERRAIN_CLEARABLE;
            }
        }
    }
    map_tiles_update_region_empty_land(x, y, x + size, y + size);
    foreach_region_tile(x, y, x + size, y + size, update_meadow_tile);
    foreach_region_tile(x, y, x + size, y + size, set_rubble_image);
}

void map_building_tiles_set_rubble(int building_id, int x, int y, int size)
{
    if (!map_grid_is_inside(x, y, size)) {
        return;
    }
    struct building_t *b = &all_buildings[building_id];
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            if (map_building_at(grid_offset) != building_id) {
                continue;
            }
            if (building_id && all_buildings[map_building_at(grid_offset)].type != BUILDING_BURNING_RUIN) {
                map_set_rubble_building_type(grid_offset, b->type);
            } else if (!building_id && terrain_grid.items[grid_offset] & TERRAIN_WALL) {
                map_set_rubble_building_type(grid_offset, BUILDING_WALL);
            }
            map_property_clear_constructing(grid_offset);
            map_property_set_multi_tile_size(grid_offset, 1);
            map_aqueduct_set(grid_offset, 0);
            map_building_set(grid_offset, 0);
            map_building_damage_clear(grid_offset);
            map_sprite_clear_tile(grid_offset);
            map_property_set_multi_tile_xy(grid_offset, 0, 0, 1);
            if (map_terrain_is(grid_offset, TERRAIN_WATER)) {
                terrain_grid.items[grid_offset] = TERRAIN_WATER; // clear other flags
                map_tiles_set_water(x + dx, y + dy);
            } else {
                terrain_grid.items[grid_offset] &= ~TERRAIN_CLEARABLE;
                terrain_grid.items[grid_offset] |= TERRAIN_RUBBLE;
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_RUBBLE) + (map_random_get(grid_offset) & 7));
            }
        }
    }
}

static void adjust_to_absolute_xy(int *x, int *y, int size)
{
    switch (city_view_orientation()) {
        case DIR_2_RIGHT:
            *x = *x - size + 1;
            break;
        case DIR_4_BOTTOM:
            *x = *x - size + 1;
            /* fall through */
        case DIR_6_LEFT:
            *y = *y - size + 1;
            break;
    }
}

int map_building_tiles_mark_construction(int x, int y, int size, int terrain, int absolute_xy)
{
    if (!absolute_xy) {
        adjust_to_absolute_xy(&x, &y, size);
    }
    if (!map_grid_is_inside(x, y, size)) {
        return 0;
    }
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            if (map_terrain_is(grid_offset, terrain & TERRAIN_NOT_CLEAR) || map_has_figure_at(grid_offset)) {
                return 0;
            }
        }
    }
    // mark as being constructed
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            map_property_mark_constructing(grid_offset);
        }
    }
    return 1;
}

void map_building_tiles_mark_deleting(int grid_offset)
{
    int building_id = map_building_at(grid_offset);
    if (!building_id) {
        map_bridge_remove(grid_offset, 1);
    } else {
        grid_offset = building_main(&all_buildings[building_id])->grid_offset;
    }
    map_property_mark_deleted(grid_offset);
}

int map_building_tiles_are_clear(int x, int y, int size, int terrain)
{
    adjust_to_absolute_xy(&x, &y, size);
    if (!map_grid_is_inside(x, y, size)) {
        return 0;
    }
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            if (map_terrain_is(grid_offset, terrain & TERRAIN_NOT_CLEAR)) {
                return 0;
            }
        }
    }
    return 1;
}

int map_building_at(int grid_offset)
{
    return map_grid_is_valid_offset(grid_offset) ? buildings_grid.items[grid_offset] : 0;
}

void map_building_set(int grid_offset, int building_id)
{
    buildings_grid.items[grid_offset] = building_id;
}

void map_building_damage_clear(int grid_offset)
{
    damage_grid.items[grid_offset] = 0;
}

int map_building_damage_increase(int grid_offset)
{
    return ++damage_grid.items[grid_offset];
}

int map_rubble_building_type(int grid_offset)
{
    return rubble_type_grid.items[grid_offset];
}

void map_set_rubble_building_type(int grid_offset, int type)
{
    rubble_type_grid.items[grid_offset] = type;
}

void map_building_clear(void)
{
    memset(buildings_grid.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
    memset(damage_grid.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
    memset(rubble_type_grid.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

void map_building_save_state(struct buffer_t *buildings, struct buffer_t *damage)
{
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        buffer_write_u16(buildings, buildings_grid.items[i]);
    }
    buffer_write_raw(damage, damage_grid.items, GRID_SIZE * GRID_SIZE);
}

void map_building_load_state(struct buffer_t *buildings, struct buffer_t *damage)
{
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        buildings_grid.items[i] = buffer_read_u16(buildings);
    }
    buffer_read_raw(damage, damage_grid.items, GRID_SIZE * GRID_SIZE);
}

void map_desirability_clear(void)
{
    memset(desirability_grid.items, 0, GRID_SIZE * GRID_SIZE * sizeof(int8_t));
}

static void add_to_terrain(int x, int y, int size, int desirability, int step, int step_size, int range)
{
    if (size > 0) {
        if (range > 6) {
            range = 6;
        }
        int tiles_within_step = 0;
        for (int distance = 1; distance <= range; distance++) {
            int partially_outside_map = 0;
            if (x - distance < -1 || x + distance + size - 1 > map_data.width) {
                partially_outside_map = 1;
            }
            if (y - distance < -1 || y + distance + size - 1 > map_data.height) {
                partially_outside_map = 1;
            }
            int base_offset = map_grid_offset(x, y);

            if (partially_outside_map) {
                for (int i = desirability_data.index[size][distance]; i < desirability_data.index[size][distance] + 4 * (size - 1) + 8 * distance; i++) {
                    const struct ring_tile_t *tile = &desirability_data.tiles[i];
                    if (x + tile->x >= -1 && x <= map_data.width && y + tile->y >= -1 && y <= map_data.height) {
                        desirability_grid.items[base_offset + tile->grid_offset] += desirability;
                        // BUG: bounding on wrong tile:
                        desirability_grid.items[base_offset] = calc_bound(desirability_grid.items[base_offset], -100, 100);
                    }
                }
            } else {
                for (int i = desirability_data.index[size][distance]; i < desirability_data.index[size][distance] + 4 * (size - 1) + 8 * distance; i++) {
                    const struct ring_tile_t *tile = &desirability_data.tiles[i];
                    desirability_grid.items[base_offset + tile->grid_offset] =
                        calc_bound(desirability_grid.items[base_offset + tile->grid_offset] + desirability, -100, 100);
                }
            }
            tiles_within_step++;
            if (tiles_within_step >= step) {
                desirability += step_size;
                tiles_within_step = 0;
            }
        }
    }
}

void map_desirability_update(void)
{
    map_desirability_clear();
    int max_id = building_get_highest_id();
    for (int i = 1; i <= max_id; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE) {
            add_to_terrain(
                b->x, b->y, b->size,
                building_properties[b->type].desirability_value,
                building_properties[b->type].desirability_step,
                building_properties[b->type].desirability_step_size,
                building_properties[b->type].desirability_range);
        }
    }
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_property_is_plaza_or_earthquake(grid_offset)) {
                int type;
                if (terrain_grid.items[grid_offset] & TERRAIN_ROAD) {
                    type = BUILDING_PLAZA;
                } else if (terrain_grid.items[grid_offset] & TERRAIN_ROCK) {
                    // earthquake fault line: slight negative
                    type = BUILDING_HOUSE_VACANT_LOT;
                } else {
                    // invalid plaza/earthquake flag
                    map_property_clear_plaza_or_earthquake(grid_offset);
                    continue;
                }
                add_to_terrain(x, y, 1,
                    building_properties[type].desirability_value,
                    building_properties[type].desirability_step,
                    building_properties[type].desirability_step_size,
                    building_properties[type].desirability_range);
            } else if (terrain_grid.items[grid_offset] & TERRAIN_GARDEN) {
                add_to_terrain(x, y, 1,
                    building_properties[BUILDING_GARDENS].desirability_value,
                    building_properties[BUILDING_GARDENS].desirability_step,
                    building_properties[BUILDING_GARDENS].desirability_step_size,
                    building_properties[BUILDING_GARDENS].desirability_range);
            } else if (terrain_grid.items[grid_offset] & TERRAIN_RUBBLE) {
                add_to_terrain(x, y, 1, -2, 1, 1, 2);
            } else if (terrain_grid.items[grid_offset] & TERRAIN_WATER) {
                add_to_terrain(x, y, 1, 1, 1, 0, 3);
            } else if (terrain_grid.items[grid_offset] & TERRAIN_SHRUB) {
                add_to_terrain(x, y, 1, 1, 1, 0, 1);
            } else if (terrain_grid.items[grid_offset] & TERRAIN_TREE) {
                add_to_terrain(x, y, 1, 1, 1, 0, 3);
            }
        }
    }
    int index = 0;
    int x, y;
    for (int size = 1; size <= 5; size++) {
        for (int dist = 1; dist <= 6; dist++) {
            desirability_data.index[size][dist] = index;
            // top row, from x=0
            for (y = -dist, x = 0; x < size + dist; x++, index++) {
                desirability_data.tiles[index].x = x;
                desirability_data.tiles[index].y = y;
            }
            // right row down
            for (x = size + dist - 1, y = -dist + 1; y < size + dist; y++, index++) {
                desirability_data.tiles[index].x = x;
                desirability_data.tiles[index].y = y;
            }
            // bottom row to the left
            for (y = size + dist - 1, x = size + dist - 2; x >= -dist; x--, index++) {
                desirability_data.tiles[index].x = x;
                desirability_data.tiles[index].y = y;
            }
            // left row up
            for (x = -dist, y = size + dist - 2; y >= -dist; y--, index++) {
                desirability_data.tiles[index].x = x;
                desirability_data.tiles[index].y = y;
            }
            // top row up to x=0
            for (y = -dist, x = -dist + 1; x < 0; x++, index++) {
                desirability_data.tiles[index].x = x;
                desirability_data.tiles[index].y = y;
            }
        }
    }
    for (int i = 0; i < index; i++) {
        desirability_data.tiles[i].grid_offset = map_grid_delta(desirability_data.tiles[i].x, desirability_data.tiles[i].y);
    }
}

int map_desirability_get(int grid_offset)
{
    return desirability_grid.items[grid_offset];
}

int map_desirability_get_max(int x, int y, int size)
{
    if (size == 1) {
        return desirability_grid.items[map_grid_offset(x, y)];
    }
    int max = -9999;
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            if (desirability_grid.items[grid_offset] > max) {
                max = desirability_grid.items[grid_offset];
            }
        }
    }
    return max;
}

void map_desirability_save_state(struct buffer_t *buf)
{
    buffer_write_raw(buf, desirability_grid.items, GRID_SIZE * GRID_SIZE);
}

void map_desirability_load_state(struct buffer_t *buf)
{
    buffer_read_raw(buf, desirability_grid.items, GRID_SIZE * GRID_SIZE);
}

int map_has_figure_at(int grid_offset)
{
    return map_grid_is_valid_offset(grid_offset) && map_figures.items[grid_offset] > 0;
}

int map_figure_at(int grid_offset)
{
    return map_grid_is_valid_offset(grid_offset) ? map_figures.items[grid_offset] : 0;
}

void map_figure_add(struct figure_t *f)
{
    if (!map_grid_is_valid_offset(f->grid_offset)) {
        return;
    }
    f->next_figure_id_on_same_tile = 0;

    if (map_figures.items[f->grid_offset]) {
        struct figure_t *next = &figures[map_figures.items[f->grid_offset]];
        while (next->next_figure_id_on_same_tile) {
            next = &figures[next->next_figure_id_on_same_tile];
        }
        next->next_figure_id_on_same_tile = f->id;
    } else {
        map_figures.items[f->grid_offset] = f->id;
    }
}

void map_figure_update(struct figure_t *f)
{
    if (!map_grid_is_valid_offset(f->grid_offset)) {
        return;
    }
    struct figure_t *next = &figures[map_figures.items[f->grid_offset]];
    while (next->id) {
        if (next->id == f->id) {
            return;
        }
        next = &figures[next->next_figure_id_on_same_tile];
    }
}

void map_figure_delete(struct figure_t *f)
{
    if (!map_grid_is_valid_offset(f->grid_offset) || !map_figures.items[f->grid_offset]) {
        f->next_figure_id_on_same_tile = 0;
        return;
    }

    if (map_figures.items[f->grid_offset] == f->id) {
        map_figures.items[f->grid_offset] = f->next_figure_id_on_same_tile;
    } else {
        struct figure_t *prev = &figures[map_figures.items[f->grid_offset]];
        while (prev->id && prev->next_figure_id_on_same_tile != f->id) {
            prev = &figures[prev->next_figure_id_on_same_tile];
        }
        prev->next_figure_id_on_same_tile = f->next_figure_id_on_same_tile;
    }
    f->next_figure_id_on_same_tile = 0;
}

void map_figure_save_state(struct buffer_t *buf)
{
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        buffer_write_u16(buf, map_figures.items[i]);
    }
}

void map_figure_load_state(struct buffer_t *buf)
{
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        map_figures.items[i] = buffer_read_u16(buf);
    }
}

void map_grid_init(int width, int height, int start_offset, int border_size)
{
    map_data.width = width;
    map_data.height = height;
    map_data.start_offset = start_offset;
    map_data.border_size = border_size;
}

int map_grid_is_valid_offset(int grid_offset)
{
    return grid_offset >= 0 && grid_offset < GRID_SIZE * GRID_SIZE;
}

int map_grid_offset(int x, int y)
{
    return map_data.start_offset + x + y * GRID_SIZE;
}

int map_grid_offset_to_x(int grid_offset)
{
    return (grid_offset - map_data.start_offset) % GRID_SIZE;
}

int map_grid_offset_to_y(int grid_offset)
{
    return (grid_offset - map_data.start_offset) / GRID_SIZE;
}

int map_grid_delta(int x, int y)
{
    return y * GRID_SIZE + x;
}

int map_grid_direction_delta(int direction)
{
    if (direction >= 0 && direction < 8) {
        return DIRECTION_DELTA[direction];
    } else {
        return 0;
    }
}

void map_grid_size(int *width, int *height)
{
    *width = map_data.width;
    *height = map_data.height;
}

void map_grid_bound(int *x, int *y)
{
    if (*x < 0) {
        *x = 0;
    }
    if (*y < 0) {
        *y = 0;
    }
    if (*x >= map_data.width) {
        *x = map_data.width - 1;
    }
    if (*y >= map_data.height) {
        *y = map_data.height - 1;
    }
}

void map_grid_bound_area(int *x_min, int *y_min, int *x_max, int *y_max)
{
    if (*x_min < 0) {
        *x_min = 0;
    }
    if (*y_min < 0) {
        *y_min = 0;
    }
    if (*x_max >= map_data.width) {
        *x_max = map_data.width - 1;
    }
    if (*y_max >= map_data.height) {
        *y_max = map_data.height - 1;
    }
}

void map_grid_get_area(int x, int y, int size, int radius, int *x_min, int *y_min, int *x_max, int *y_max)
{
    *x_min = x - radius;
    *y_min = y - radius;
    *x_max = x + size + radius - 1;
    *y_max = y + size + radius - 1;
    map_grid_bound_area(x_min, y_min, x_max, y_max);
}

int map_grid_is_inside(int x, int y, int size)
{
    return x >= 0 && x + size <= map_data.width && y >= 0 && y + size <= map_data.height;
}

const int *map_grid_adjacent_offsets(int size)
{
    return ADJACENT_OFFSETS_MAP_GRID[size];
}

static void clear_current_offset(struct terrain_image_context *items, int num_items)
{
    for (int i = 0; i < num_items; i++) {
        items[i].current_item_offset = 0;
    }
}

void map_image_context_init(void)
{
    for (int i = 0; i < CONTEXT_MAX_ITEMS; i++) {
        clear_current_offset(context_pointers[i].context, context_pointers[i].size);
    }
}

void map_image_context_reset_water(void)
{
    clear_current_offset(context_pointers[CONTEXT_WATER].context, context_pointers[CONTEXT_WATER].size);
}

void map_image_context_reset_elevation(void)
{
    clear_current_offset(context_pointers[CONTEXT_ELEVATION].context, context_pointers[CONTEXT_ELEVATION].size);
}

static void set_tiles_road(int grid_offset, int tiles[MAX_TILES])
{
    fill_matches(grid_offset, TERRAIN_ROAD, 1, 0, tiles);
    for (int i = 0; i < MAX_TILES; i += 2) {
        int offset = grid_offset + map_grid_direction_delta(i);
        if (map_terrain_is(offset, TERRAIN_GATEHOUSE)) {
            struct building_t *b = &all_buildings[map_building_at(offset)];
            if (b->type == BUILDING_GATEHOUSE &&
                b->subtype.orientation == 1 + ((i / 2) & 1)) { // 1,2,1,2
                tiles[i] = 1;
            }
        } else if (map_terrain_is(offset, TERRAIN_ACCESS_RAMP)) {
            tiles[i] = 1;
        } else if (map_terrain_is(offset, TERRAIN_BUILDING)) {
            struct building_t *b = &all_buildings[map_building_at(offset)];
            if (b->type == BUILDING_GRANARY) {
                tiles[i] = (offset == b->grid_offset + map_grid_delta(1, 0)) ? 1 : 0;
                tiles[i] |= (offset == b->grid_offset + map_grid_delta(0, 1)) ? 1 : 0;
                tiles[i] |= (offset == b->grid_offset + map_grid_delta(2, 1)) ? 1 : 0;
                tiles[i] |= (offset == b->grid_offset + map_grid_delta(1, 2)) ? 1 : 0;
            }
        }
    }
}

static int is_reservoir_construction_entrance(int grid_offset)
{
    if (!map_property_is_constructing(grid_offset)) {
        return 0;
    }
    if (map_property_is_constructing(grid_offset + map_grid_direction_delta(0)) &&
        map_property_is_constructing(grid_offset + map_grid_direction_delta(4))) {
        return !map_property_is_constructing(grid_offset + 2 * map_grid_direction_delta(0)) ||
            !map_property_is_constructing(grid_offset + 2 * map_grid_direction_delta(4));
    }
    if (map_property_is_constructing(grid_offset + map_grid_direction_delta(2)) &&
        map_property_is_constructing(grid_offset + map_grid_direction_delta(6))) {
        return !map_property_is_constructing(grid_offset + 2 * map_grid_direction_delta(2)) ||
            !map_property_is_constructing(grid_offset + 2 * map_grid_direction_delta(6));
    }
    return 0;
}

static void set_terrain_reservoir(int grid_offset, int direction, int multi_tile_mask, int tiles[MAX_TILES], int include_construction)
{
    int offset = grid_offset + map_grid_direction_delta(direction);
    if (map_terrain_is(offset, TERRAIN_BUILDING)) {
        struct building_t *b = &all_buildings[map_building_at(offset)];
        if (b->type == BUILDING_RESERVOIR && map_property_multi_tile_xy(offset) == multi_tile_mask) {
            tiles[direction] = 1;
            return;
        }
    }
    if (include_construction && is_reservoir_construction_entrance(offset)) {
        tiles[direction] = 1;
    }
}

const struct terrain_image_t *map_image_context_get_aqueduct(int grid_offset, int include_construction)
{
    int tiles[MAX_TILES] = { 0,0,0,0,0,0,0,0 };
    int has_road = map_terrain_is(grid_offset, TERRAIN_ROAD) ? 1 : 0;
    for (int i = 0; i < MAX_TILES; i += 2) {
        int offset = grid_offset + map_grid_direction_delta(i);
        if (map_terrain_is(offset, TERRAIN_AQUEDUCT)) {
            if (has_road) {
                if (!map_terrain_is(offset, TERRAIN_ROAD)) {
                    tiles[i] = 1;
                }
            } else {
                tiles[i] = 1;
            }
        }
    }
    set_terrain_reservoir(grid_offset, 0, EDGE_X1Y2, tiles, include_construction);
    set_terrain_reservoir(grid_offset, 2, EDGE_X0Y1, tiles, include_construction);
    set_terrain_reservoir(grid_offset, 4, EDGE_X1Y0, tiles, include_construction);
    set_terrain_reservoir(grid_offset, 6, EDGE_X2Y1, tiles, include_construction);
    return get_image(CONTEXT_AQUEDUCT, tiles);
}

int map_image_at(int grid_offset)
{
    return images.items[grid_offset];
}

void map_image_set(int grid_offset, int image_id)
{
    images.items[grid_offset] = image_id;
}

void map_image_backup(void)
{
    memcpy(images_backup.items, images.items, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
}

void map_image_restore(void)
{
    memcpy(images.items, images_backup.items, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
}

void map_image_restore_at(int grid_offset)
{
    images.items[grid_offset] = images_backup.items[grid_offset];
}

void map_image_clear(void)
{
    memset(images.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
}

void map_image_init_edges(void)
{
    int width, height;
    map_grid_size(&width, &height);
    for (int x = 1; x < width; x++) {
        images.items[map_grid_offset(x, height)] = 1;
    }
    for (int y = 1; y < height; y++) {
        images.items[map_grid_offset(width, y)] = 2;
    }
    images.items[map_grid_offset(0, height)] = 3;
    images.items[map_grid_offset(width, 0)] = 4;
    images.items[map_grid_offset(width, height)] = 5;
}

void map_image_save_state(struct buffer_t *buf)
{
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        buffer_write_u16(buf, images.items[i]);
    }
}

void map_image_load_state(struct buffer_t *buf)
{
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        images.items[i] = buffer_read_u16(buf);
    }
}

static void mark_native_land(int x, int y, int size, int radius)
{
    int x_min, y_min, x_max, y_max;
    map_grid_get_area(x, y, size, radius, &x_min, &y_min, &x_max, &y_max);
    for (int yy = y_min; yy <= y_max; yy++) {
        for (int xx = x_min; xx <= x_max; xx++) {
            map_property_mark_native_land(map_grid_offset(xx, yy));
        }
    }
}

void map_natives_init(void)
{
    int meeting_center_set = 0;
    int native_image = image_group(GROUP_BUILDING_NATIVE);
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (!map_terrain_is(grid_offset, TERRAIN_BUILDING) || map_building_at(grid_offset)) {
                continue;
            }
            int random_bit = map_random_get(grid_offset) & 1;
            int type;
            int image_id = map_image_at(grid_offset);
            if (image_id == scenario.native_images.hut) {
                type = BUILDING_NATIVE_HUT;
                map_image_set(grid_offset, native_image);
            } else if (image_id == scenario.native_images.hut + 1) {
                type = BUILDING_NATIVE_HUT;
                map_image_set(grid_offset, native_image + 1);
            } else if (image_id == scenario.native_images.meeting) {
                type = BUILDING_NATIVE_MEETING;
                map_image_set(grid_offset, native_image + 2);
                map_image_set(grid_offset + map_grid_delta(1, 0), native_image + 2);
                map_image_set(grid_offset + map_grid_delta(0, 1), native_image + 2);
                map_image_set(grid_offset + map_grid_delta(1, 1), native_image + 2);
            } else if (image_id == scenario.native_images.crops) {
                type = BUILDING_NATIVE_CROPS;
                map_image_set(grid_offset, image_group(GROUP_BUILDING_FARM_CROPS) + random_bit);
            } else if (image_group(GROUP_EDITOR_BUILDING_NATIVE) - 4) {
                type = BUILDING_HOUSE_VACANT_LOT;
                map_image_set(grid_offset, image_group(GROUP_EDITOR_BUILDING_NATIVE) - 4);
            } else { //unknown building
                map_building_tiles_remove(0, x, y);
                continue;
            }
            struct building_t *b = building_create(type, x, y);
            map_building_set(grid_offset, b->id);
            b->state = BUILDING_STATE_IN_USE;
            switch (type) {
                case BUILDING_NATIVE_CROPS:
                    b->data.industry.progress = random_bit;
                    break;
                case BUILDING_NATIVE_MEETING:
                    b->sentiment.native_anger = 100;
                    map_building_set(grid_offset + map_grid_delta(1, 0), b->id);
                    map_building_set(grid_offset + map_grid_delta(0, 1), b->id);
                    map_building_set(grid_offset + map_grid_delta(1, 1), b->id);
                    mark_native_land(b->x, b->y, 2, 6);
                    if (!meeting_center_set) {
                        city_data.building.main_native_meeting.x = b->x;
                        city_data.building.main_native_meeting.y = b->y;
                    }
                    break;
                case BUILDING_NATIVE_HUT:
                    b->sentiment.native_anger = 100;
                    b->figure_spawn_delay = random_bit;
                    mark_native_land(b->x, b->y, 1, 3);
                    break;
                case BUILDING_HOUSE_VACANT_LOT:
                    map_building_set(grid_offset, b->id);
                    break;
            }
        }
    }
    // gather list of meeting centers
    building_list_small_clear();
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_NATIVE_MEETING) {
            building_list_small_add(i);
        }
    }
    int total_meetings = building_list_small_size();
    if (total_meetings <= 0) {
        return;
    }
    const int *meetings = building_list_small_items();
    // determine closest meeting center for hut
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_NATIVE_HUT) {
            int min_dist = 1000;
            int min_meeting_id = 0;
            for (int n = 0; n < total_meetings; n++) {
                struct building_t *meeting = &all_buildings[meetings[n]];
                int dist = calc_maximum_distance(b->x, b->y, meeting->x, meeting->y);
                if (dist < min_dist) {
                    min_dist = dist;
                    min_meeting_id = meetings[n];
                }
            }
            b->subtype.native_meeting_center_id = min_meeting_id;
        }
    }
}

void map_natives_init_editor(void)
{
    int native_image = image_group(GROUP_EDITOR_BUILDING_NATIVE);
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (!map_terrain_is(grid_offset, TERRAIN_BUILDING) || map_building_at(grid_offset)) {
                continue;
            }
            int type;
            int image_id = map_image_at(grid_offset);
            if (image_id == scenario.native_images.hut) {
                type = BUILDING_NATIVE_HUT;
                map_image_set(grid_offset, native_image);
            } else if (image_id == scenario.native_images.hut + 1) {
                type = BUILDING_NATIVE_HUT;
                map_image_set(grid_offset, native_image + 1);
            } else if (image_id == scenario.native_images.meeting) {
                type = BUILDING_NATIVE_MEETING;
                map_image_set(grid_offset, native_image + 2);
                map_image_set(grid_offset + map_grid_delta(1, 0), native_image + 2);
                map_image_set(grid_offset + map_grid_delta(0, 1), native_image + 2);
                map_image_set(grid_offset + map_grid_delta(1, 1), native_image + 2);
            } else if (image_id == scenario.native_images.crops) {
                type = BUILDING_NATIVE_CROPS;
                map_image_set(grid_offset, image_group(GROUP_EDITOR_BUILDING_CROPS));
            } else if (image_group(GROUP_EDITOR_BUILDING_NATIVE) - 4) {
                type = BUILDING_HOUSE_VACANT_LOT;
                map_image_set(grid_offset, image_group(GROUP_EDITOR_BUILDING_NATIVE) - 4);
            } else { //unknown building
                map_building_tiles_remove(0, x, y);
                continue;
            }
            struct building_t *b = building_create(type, x, y);
            b->state = BUILDING_STATE_IN_USE;
            map_building_set(grid_offset, b->id);
            if (type == BUILDING_NATIVE_MEETING) {
                map_building_set(grid_offset + map_grid_delta(1, 0), b->id);
                map_building_set(grid_offset + map_grid_delta(0, 1), b->id);
                map_building_set(grid_offset + map_grid_delta(1, 1), b->id);
            }
        }
    }
}

void map_natives_check_land(void)
{
    map_property_clear_all_native_land();
    if (city_data.military.native_attack_duration) {
        city_data.military.native_attack_duration--;
    }

    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        int size, radius;
        if (b->type == BUILDING_NATIVE_HUT) {
            size = 1;
            radius = 3;
        } else if (b->type == BUILDING_NATIVE_MEETING) {
            size = 2;
            radius = 6;
        } else {
            continue;
        }
        if (b->sentiment.native_anger >= 100) {
            mark_native_land(b->x, b->y, size, radius);
            int x_min, y_min, x_max, y_max;
            map_grid_get_area(b->x, b->y, size, radius, &x_min, &y_min, &x_max, &y_max);
            for (int yy = y_min; yy <= y_max; yy++) {
                for (int xx = x_min; xx <= x_max; xx++) {
                    int building_id = map_building_at(map_grid_offset(xx, yy));
                    if (building_id > 0) {
                        int type = all_buildings[building_id].type;
                        if (type != BUILDING_MISSION_POST &&
                            type != BUILDING_NATIVE_HUT &&
                            type != BUILDING_NATIVE_MEETING &&
                            type != BUILDING_NATIVE_CROPS) {
                            city_data.military.native_attack_duration = 1;
                            break;
                        }
                    }
                }
            }
        } else {
            b->sentiment.native_anger++;
        }
    }
}

void map_orientation_change(int counter_clockwise)
{
    game_undo_disable();
    int orientation = city_view_orientation();
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            int size = map_property_multi_tile_size(grid_offset);
            if (size == 1) {
                map_property_mark_draw_tile(grid_offset);
                continue;
            }
            map_property_clear_draw_tile(grid_offset);
            int dx = orientation == DIR_4_BOTTOM || orientation == DIR_6_LEFT ? size - 1 : 0;
            int dy = orientation == DIR_0_TOP || orientation == DIR_6_LEFT ? size - 1 : 0;
            if (map_property_is_multi_tile_xy(grid_offset, dx, dy)) {
                map_property_mark_draw_tile(grid_offset);
            }
        }
    }

    map_tiles_update_all_elevation();
    map_tiles_update_all_water();
    map_tiles_update_all_earthquake();
    map_tiles_update_all_gardens();

    map_tiles_add_entry_exit_flags();

    map_tiles_update_all_empty_land();
    for (int y = 0, grid_offset2 = map_data.start_offset; y < map_data.height; y++, grid_offset2 += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset2++) {
            set_rubble_image(x, y, grid_offset2);
        }
    }
    map_tiles_update_all_roads();
    map_tiles_update_all_plazas();
    map_tiles_update_all_walls();
    map_tiles_update_all_aqueducts(0);

    map_orientation_update_buildings();
    int grid_offset3 = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset3 += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset3++) {
            if (map_is_bridge(grid_offset3)) {
                int new_value;
                switch (map_sprite_bridge_at(grid_offset3)) {
                    case 1: new_value = counter_clockwise ? 2 : 4; break;
                    case 2: new_value = counter_clockwise ? 3 : 1; break;
                    case 3: new_value = counter_clockwise ? 4 : 2; break;
                    case 4: new_value = counter_clockwise ? 1 : 3; break;
                    case 5: new_value = 6; break;
                    case 6: new_value = 5; break;
                    case 7: new_value = counter_clockwise ? 8 : 10; break;
                    case 8: new_value = counter_clockwise ? 9 : 7; break;
                    case 9: new_value = counter_clockwise ? 10 : 8; break;
                    case 10: new_value = counter_clockwise ? 7 : 9; break;
                    case 11: new_value = 12; break;
                    case 12: new_value = 11; break;
                    case 13: new_value = 13; break;
                    case 14: new_value = 15; break;
                    case 15: new_value = 14; break;
                    default: new_value = map_sprite_bridge_at(grid_offset3);
                }
                map_sprite_bridge_set(grid_offset3, new_value);
            }
        }
    }
    map_routing_update_walls();

    figure_tower_sentry_reroute();
    if (city_data.entertainment.hippodrome_has_race) {
        figure_hippodrome_horse_reroute();
    }

}

int map_orientation_for_gatehouse(int x, int y)
{
    switch (city_view_orientation()) {
        case DIR_2_RIGHT: x--; break;
        case DIR_4_BOTTOM: x--; y--; break;
        case DIR_6_LEFT: y--; break;
    }
    int grid_offset = map_grid_offset(x, y);
    int num_road_tiles_within = 0;
    int road_tiles_within_flags = 0;
    // tiles within gate, flags:
    // 1  2
    // 4  8
    if (map_terrain_is(map_grid_offset(x, y), TERRAIN_ROAD)) {
        road_tiles_within_flags |= 1;
        num_road_tiles_within++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(1, 0), TERRAIN_ROAD)) {
        road_tiles_within_flags |= 2;
        num_road_tiles_within++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(0, 1), TERRAIN_ROAD)) {
        road_tiles_within_flags |= 4;
        num_road_tiles_within++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(1, 1), TERRAIN_ROAD)) {
        road_tiles_within_flags |= 8;
        num_road_tiles_within++;
    }

    if (num_road_tiles_within != 2 && num_road_tiles_within != 4) {
        return 0;
    }
    if (num_road_tiles_within == 2) {
        if (road_tiles_within_flags == 6 || road_tiles_within_flags == 9) { // diagonals
            return 0;
        }
        if (road_tiles_within_flags == 5 || road_tiles_within_flags == 10) { // top to bottom
            return 1;
        }
        if (road_tiles_within_flags == 3 || road_tiles_within_flags == 12) { // left to right
            return 2;
        }
        return 0;
    }
    // all 4 tiles are road: check adjacent roads
    int num_road_tiles_top = 0;
    int num_road_tiles_right = 0;
    int num_road_tiles_bottom = 0;
    int num_road_tiles_left = 0;
    // top
    if (map_terrain_is(grid_offset + map_grid_delta(0, -1), TERRAIN_ROAD)) {
        num_road_tiles_top++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(1, -1), TERRAIN_ROAD)) {
        num_road_tiles_top++;
    }
    // bottom
    if (map_terrain_is(grid_offset + map_grid_delta(0, 2), TERRAIN_ROAD)) {
        num_road_tiles_bottom++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(1, 2), TERRAIN_ROAD)) {
        num_road_tiles_bottom++;
    }
    // left
    if (map_terrain_is(grid_offset + map_grid_delta(-1, 0), TERRAIN_ROAD)) {
        num_road_tiles_left++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(-1, 1), TERRAIN_ROAD)) {
        num_road_tiles_left++;
    }
    // right
    if (map_terrain_is(grid_offset + map_grid_delta(2, 0), TERRAIN_ROAD)) {
        num_road_tiles_right++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(2, 1), TERRAIN_ROAD)) {
        num_road_tiles_right++;
    }
    // determine direction
    if (num_road_tiles_top || num_road_tiles_bottom) {
        if (num_road_tiles_left || num_road_tiles_right) {
            return 0;
        }
        return 1;
    } else if (num_road_tiles_left || num_road_tiles_right) {
        return 2;
    }
    return 0;
}

int map_orientation_for_triumphal_arch(int x, int y)
{
    switch (city_view_orientation()) {
        case DIR_2_RIGHT: x -= 2; break;
        case DIR_4_BOTTOM: x -= 2; y -= 2; break;
        case DIR_6_LEFT: y -= 2; break;
    }
    int num_road_tiles_top_bottom = 0;
    int num_road_tiles_left_right = 0;
    int num_blocked_tiles = 0;

    int grid_offset = map_grid_offset(x, y);
    // check corner tiles
    if (map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
        num_blocked_tiles++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(2, 0), TERRAIN_NOT_CLEAR)) {
        num_blocked_tiles++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(0, 2), TERRAIN_NOT_CLEAR)) {
        num_blocked_tiles++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(2, 2), TERRAIN_NOT_CLEAR)) {
        num_blocked_tiles++;
    }
    // road tiles top to bottom
    int top_offset = grid_offset + map_grid_delta(1, 0);
    if ((terrain_grid.items[top_offset] & TERRAIN_NOT_CLEAR) == TERRAIN_ROAD) {
        num_road_tiles_top_bottom++;
    } else if (map_terrain_is(top_offset, TERRAIN_NOT_CLEAR)) {
        num_blocked_tiles++;
    }
    int bottom_offset = grid_offset + map_grid_delta(1, 2);
    if ((terrain_grid.items[bottom_offset] & TERRAIN_NOT_CLEAR) == TERRAIN_ROAD) {
        num_road_tiles_top_bottom++;
    } else if (map_terrain_is(bottom_offset, TERRAIN_NOT_CLEAR)) {
        num_blocked_tiles++;
    }
    // road tiles left to right
    int left_offset = grid_offset + map_grid_delta(0, 1);
    if ((terrain_grid.items[left_offset] & TERRAIN_NOT_CLEAR) == TERRAIN_ROAD) {
        num_road_tiles_left_right++;
    } else if (map_terrain_is(left_offset, TERRAIN_NOT_CLEAR)) {
        num_blocked_tiles++;
    }
    int right_offset = grid_offset + map_grid_delta(2, 1);
    if ((terrain_grid.items[right_offset] & TERRAIN_NOT_CLEAR) == TERRAIN_ROAD) {
        num_road_tiles_left_right++;
    } else if (map_terrain_is(right_offset, TERRAIN_NOT_CLEAR)) {
        num_blocked_tiles++;
    }
    // center tile
    int center_offset = grid_offset + map_grid_delta(2, 1);
    if ((terrain_grid.items[center_offset] & TERRAIN_NOT_CLEAR) == TERRAIN_ROAD) {
        // do nothing
    } else if (map_terrain_is(center_offset, TERRAIN_NOT_CLEAR)) {
        num_blocked_tiles++;
    }
    // judgement time
    if (num_blocked_tiles) {
        return 0;
    }
    if (!num_road_tiles_left_right && !num_road_tiles_top_bottom) {
        return 0; // no road: can't determine direction
    }
    if (num_road_tiles_top_bottom == 2 && !num_road_tiles_left_right) {
        return 1;
    }
    if (num_road_tiles_left_right == 2 && !num_road_tiles_top_bottom) {
        return 2;
    }
    return 0;
}

void map_orientation_update_buildings(void)
{
    int map_orientation = city_view_orientation();
    int orientation_is_top_bottom = map_orientation == DIR_0_TOP || map_orientation == DIR_4_BOTTOM;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_UNUSED || b->state == BUILDING_STATE_DELETED_BY_GAME) {
            continue;
        }
        int image_id;
        int image_offset;
        switch (b->type) {
            case BUILDING_GATEHOUSE:
                if (b->subtype.orientation == 1) {
                    if (orientation_is_top_bottom) {
                        image_id = image_group(GROUP_BUILDING_TOWER) + 1;
                    } else {
                        image_id = image_group(GROUP_BUILDING_TOWER) + 2;
                    }
                } else {
                    if (orientation_is_top_bottom) {
                        image_id = image_group(GROUP_BUILDING_TOWER) + 2;
                    } else {
                        image_id = image_group(GROUP_BUILDING_TOWER) + 1;
                    }
                }
                map_building_tiles_add(i, b->x, b->y, b->size, image_id, TERRAIN_GATEHOUSE | TERRAIN_BUILDING);
                map_terrain_add_gatehouse_roads(b->x, b->y, 0);
                break;
            case BUILDING_TRIUMPHAL_ARCH:
                if (b->subtype.orientation == 1) {
                    if (orientation_is_top_bottom) {
                        image_id = image_group(GROUP_BUILDING_TRIUMPHAL_ARCH);
                    } else {
                        image_id = image_group(GROUP_BUILDING_TRIUMPHAL_ARCH) + 2;
                    }
                } else {
                    if (orientation_is_top_bottom) {
                        image_id = image_group(GROUP_BUILDING_TRIUMPHAL_ARCH) + 2;
                    } else {
                        image_id = image_group(GROUP_BUILDING_TRIUMPHAL_ARCH);
                    }
                }
                map_building_tiles_add(i, b->x, b->y, b->size, image_id, TERRAIN_BUILDING);
                map_terrain_add_triumphal_arch_roads(b->x, b->y, b->subtype.orientation);
                break;
            case BUILDING_HIPPODROME:
                if (map_orientation == DIR_0_TOP) {
                    image_id = image_group(GROUP_BUILDING_HIPPODROME_2);
                    switch (b->subtype.orientation) {
                        case 0: case 3: image_id += 0; break;
                        case 1: case 4: image_id += 2; break;
                        case 2: case 5: image_id += 4; break;
                    }
                } else if (map_orientation == DIR_4_BOTTOM) {
                    image_id = image_group(GROUP_BUILDING_HIPPODROME_2);
                    switch (b->subtype.orientation) {
                        case 0: case 3: image_id += 4; break;
                        case 1: case 4: image_id += 2; break;
                        case 2: case 5: image_id += 0; break;
                    }
                } else if (map_orientation == DIR_6_LEFT) {
                    image_id = image_group(GROUP_BUILDING_HIPPODROME_1);
                    switch (b->subtype.orientation) {
                        case 0: case 3: image_id += 0; break;
                        case 1: case 4: image_id += 2; break;
                        case 2: case 5: image_id += 4; break;
                    }
                } else { // DIR_2_RIGHT
                    image_id = image_group(GROUP_BUILDING_HIPPODROME_1);
                    switch (b->subtype.orientation) {
                        case 0: case 3: image_id += 4; break;
                        case 1: case 4: image_id += 2; break;
                        case 2: case 5: image_id += 0; break;
                    }
                }
                map_building_tiles_add(i, b->x, b->y, b->size, image_id, TERRAIN_BUILDING);
                break;
            case BUILDING_SHIPYARD:
                image_offset = (4 + b->data.industry.orientation - map_orientation / 2) % 4;
                image_id = image_group(GROUP_BUILDING_SHIPYARD) + image_offset;
                map_water_add_building(i, b->x, b->y, 2, image_id);
                break;
            case BUILDING_WHARF:
                image_offset = (4 + b->data.industry.orientation - map_orientation / 2) % 4;
                image_id = image_group(GROUP_BUILDING_WHARF) + image_offset;
                map_water_add_building(i, b->x, b->y, 2, image_id);
                break;
            case BUILDING_DOCK:
                image_offset = (4 + b->data.dock.orientation - map_orientation / 2) % 4;
                switch (image_offset) {
                    case 0: image_id = image_group(GROUP_BUILDING_DOCK_1); break;
                    case 1: image_id = image_group(GROUP_BUILDING_DOCK_2); break;
                    case 2: image_id = image_group(GROUP_BUILDING_DOCK_3); break;
                    default:image_id = image_group(GROUP_BUILDING_DOCK_4); break;
                }
                map_water_add_building(i, b->x, b->y, 3, image_id);
                break;
        }
    }
}

static int edge_for(int x, int y)
{
    return 8 * y + x;
}

int map_property_is_draw_tile(int grid_offset)
{
    return edge_grid.items[grid_offset] & EDGE_LEFTMOST_TILE;
}

void map_property_mark_draw_tile(int grid_offset)
{
    edge_grid.items[grid_offset] |= EDGE_LEFTMOST_TILE;
}

void map_property_clear_draw_tile(int grid_offset)
{
    edge_grid.items[grid_offset] &= ~EDGE_LEFTMOST_TILE;
}

int map_property_is_native_land(int grid_offset)
{
    return edge_grid.items[grid_offset] & EDGE_NATIVE_LAND;
}

void map_property_mark_native_land(int grid_offset)
{
    edge_grid.items[grid_offset] |= EDGE_NATIVE_LAND;
}

void map_property_clear_all_native_land(void)
{
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        edge_grid.items[i] &= EDGE_NO_NATIVE_LAND;
    }
}

int map_property_multi_tile_xy(int grid_offset)
{
    return edge_grid.items[grid_offset] & EDGE_MASK_XY;
}

int map_property_multi_tile_x(int grid_offset)
{
    return edge_grid.items[grid_offset] & EDGE_MASK_X;
}

int map_property_multi_tile_y(int grid_offset)
{
    return edge_grid.items[grid_offset] & EDGE_MASK_Y;
}

int map_property_is_multi_tile_xy(int grid_offset, int x, int y)
{
    return (edge_grid.items[grid_offset] & EDGE_MASK_XY) == edge_for(x, y);
}

void map_property_set_multi_tile_xy(int grid_offset, int x, int y, int is_draw_tile)
{
    if (is_draw_tile) {
        edge_grid.items[grid_offset] = edge_for(x, y) | EDGE_LEFTMOST_TILE;
    } else {
        edge_grid.items[grid_offset] = edge_for(x, y);
    }
}

void map_property_clear_multi_tile_xy(int grid_offset)
{
    // only keep native land marker
    edge_grid.items[grid_offset] &= EDGE_NATIVE_LAND;
}

int map_property_multi_tile_size(int grid_offset)
{
    switch (bitfields_grid.items[grid_offset] & BIT_SIZES) {
        case BIT_SIZE2: return 2;
        case BIT_SIZE3: return 3;
        case BIT_SIZE4: return 4;
        case BIT_SIZE5: return 5;
        default: return 1;
    }
}

void map_property_set_multi_tile_size(int grid_offset, int size)
{
    bitfields_grid.items[grid_offset] &= BIT_NO_SIZES;
    switch (size) {
        case 2: bitfields_grid.items[grid_offset] |= BIT_SIZE2; break;
        case 3: bitfields_grid.items[grid_offset] |= BIT_SIZE3; break;
        case 4: bitfields_grid.items[grid_offset] |= BIT_SIZE4; break;
        case 5: bitfields_grid.items[grid_offset] |= BIT_SIZE5; break;
    }
}

void map_property_init_alternate_terrain(void)
{
    int map_width, map_height;
    map_grid_size(&map_width, &map_height);
    for (int y = 0; y < map_height; y++) {
        for (int x = 0; x < map_width; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (map_random_get(grid_offset) & 1) {
                bitfields_grid.items[grid_offset] |= BIT_ALTERNATE_TERRAIN;
            }
        }
    }
}

int map_property_is_alternate_terrain(int grid_offset)
{
    return bitfields_grid.items[grid_offset] & BIT_ALTERNATE_TERRAIN;
}

int map_property_is_plaza_or_earthquake(int grid_offset)
{
    return bitfields_grid.items[grid_offset] & BIT_PLAZA_OR_EARTHQUAKE;
}

void map_property_mark_plaza_or_earthquake(int grid_offset)
{
    bitfields_grid.items[grid_offset] |= BIT_PLAZA_OR_EARTHQUAKE;
}

void map_property_clear_plaza_or_earthquake(int grid_offset)
{
    bitfields_grid.items[grid_offset] &= BIT_NO_PLAZA;
}

int map_property_is_constructing(int grid_offset)
{
    return bitfields_grid.items[grid_offset] & BIT_CONSTRUCTION;
}

void map_property_mark_constructing(int grid_offset)
{
    bitfields_grid.items[grid_offset] |= BIT_CONSTRUCTION;
}

void map_property_clear_constructing(int grid_offset)
{
    bitfields_grid.items[grid_offset] &= BIT_NO_CONSTRUCTION;
}

int map_property_is_deleted(int grid_offset)
{
    return bitfields_grid.items[grid_offset] & BIT_DELETED;
}

void map_property_mark_deleted(int grid_offset)
{
    bitfields_grid.items[grid_offset] |= BIT_DELETED;
}

void map_property_clear_deleted(int grid_offset)
{
    bitfields_grid.items[grid_offset] &= BIT_NO_DELETED;
}

void map_property_clear_constructing_and_deleted(void)
{
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        bitfields_grid.items[i] &= BIT_NO_CONSTRUCTION_AND_DELETED;
    }
}

void map_property_clear(void)
{
    memset(bitfields_grid.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
    memset(edge_grid.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

void map_property_backup(void)
{
    memcpy(bitfields_backup.items, bitfields_grid.items, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
    memcpy(edge_backup.items, edge_grid.items, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

void map_property_restore(void)
{
    memcpy(bitfields_grid.items, bitfields_backup.items, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
    memcpy(edge_grid.items, edge_backup.items, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

void map_property_save_state(struct buffer_t *bitfields, struct buffer_t *edge)
{
    buffer_write_raw(bitfields, bitfields_grid.items, GRID_SIZE * GRID_SIZE);
    buffer_write_raw(edge, edge_grid.items, GRID_SIZE * GRID_SIZE);
}

void map_property_load_state(struct buffer_t *bitfields, struct buffer_t *edge)
{
    buffer_read_raw(bitfields, bitfields_grid.items, GRID_SIZE * GRID_SIZE);
    buffer_read_raw(edge, edge_grid.items, GRID_SIZE * GRID_SIZE);
}

void map_random_clear(void)
{
    memset(random.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

void map_random_init(void)
{
    int grid_offset = 0;
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++, grid_offset++) {
            random_generate_next();
            random.items[grid_offset] = (uint8_t) random_short();
        }
    }
}

int map_random_get(int grid_offset)
{
    return random.items[grid_offset];
}

void map_random_save_state(struct buffer_t *buf)
{
    buffer_write_raw(buf, random.items, GRID_SIZE * GRID_SIZE);
}

void map_random_load_state(struct buffer_t *buf)
{
    buffer_read_raw(buf, random.items, GRID_SIZE * GRID_SIZE);
}

static int city_map_road_network_index(int network_id)
{
    for (int n = 0; n < 10; n++) {
        if (city_data.map.largest_road_networks[n].id == network_id) {
            return n;
        }
    }
    return 11;
}

static void find_minimum_road_tile(int x, int y, int size, int *min_value, int *min_grid_offset)
{
    int base_offset = map_grid_offset(x, y);
    for (const int *tile_delta = map_grid_adjacent_offsets(size); *tile_delta; tile_delta++) {
        int grid_offset = base_offset + *tile_delta;
        if (!map_terrain_is(grid_offset, TERRAIN_BUILDING) ||
            all_buildings[map_building_at(grid_offset)].type != BUILDING_GATEHOUSE) {
            if (map_terrain_is(grid_offset, TERRAIN_ROAD)) {
                int road_index = city_map_road_network_index(map_road_network_get(grid_offset));
                if (road_index < *min_value) {
                    *min_value = road_index;
                    *min_grid_offset = grid_offset;
                }
            }
        }
    }
}

int map_has_road_access(int x, int y, int size, struct map_point_t *road)
{
    int min_value = 12;
    int min_grid_offset = map_grid_offset(x, y);
    find_minimum_road_tile(x, y, size, &min_value, &min_grid_offset);
    if (min_value < 12) {
        if (road) {
            road->x = map_grid_offset_to_x(min_grid_offset);
            road->y = map_grid_offset_to_y(min_grid_offset);
        }
        return 1;
    }
    return 0;
}

int map_has_road_access_hippodrome(int x, int y, struct map_point_t *road)
{
    int min_value = 12;
    int min_grid_offset = map_grid_offset(x, y);
    find_minimum_road_tile(x, y, 5, &min_value, &min_grid_offset);
    find_minimum_road_tile(x + 5, y, 5, &min_value, &min_grid_offset);
    find_minimum_road_tile(x + 10, y, 5, &min_value, &min_grid_offset);
    if (min_value < 12) {
        if (road) {
            road->x = map_grid_offset_to_x(min_grid_offset);
            road->y = map_grid_offset_to_y(min_grid_offset);
        }
        return 1;
    }
    return 0;
}

int map_has_road_access_granary(int x, int y, struct map_point_t *road)
{
    int rx = -1, ry = -1;
    if (map_terrain_is(map_grid_offset(x + 1, y - 1), TERRAIN_ROAD)) {
        rx = x + 1;
        ry = y - 1;
    } else if (map_terrain_is(map_grid_offset(x + 3, y + 1), TERRAIN_ROAD)) {
        rx = x + 3;
        ry = y + 1;
    } else if (map_terrain_is(map_grid_offset(x + 1, y + 3), TERRAIN_ROAD)) {
        rx = x + 1;
        ry = y + 3;
    } else if (map_terrain_is(map_grid_offset(x - 1, y + 1), TERRAIN_ROAD)) {
        rx = x - 1;
        ry = y + 1;
    }
    if (rx >= 0 && ry >= 0) {
        if (road) {
            road->x = rx;
            road->y = ry;
        }
        return 1;
    }
    return 0;
}

int map_closest_road_within_radius(int x, int y, int size, int radius, int *x_road, int *y_road)
{
    for (int r = 1; r <= radius; r++) {
        int x_min, y_min, x_max, y_max;
        map_grid_get_area(x, y, size, r, &x_min, &y_min, &x_max, &y_max);
        for (int yy = y_min; yy <= y_max; yy++) {
            for (int xx = x_min; xx <= x_max; xx++) {
                if (map_terrain_is(map_grid_offset(xx, yy), TERRAIN_ROAD)) {
                    if (x_road && y_road) {
                        *x_road = xx;
                        *y_road = yy;
                    }
                    return 1;
                }
            }
        }
    }
    return 0;
}

int map_closest_reachable_road_within_radius(int x, int y, int size, int radius, int *x_road, int *y_road)
{
    for (int r = 1; r <= radius; r++) {

        int x_min, y_min, x_max, y_max;
        map_grid_get_area(x, y, size, r, &x_min, &y_min, &x_max, &y_max);
        for (int yy = y_min; yy <= y_max; yy++) {
            for (int xx = x_min; xx <= x_max; xx++) {
                int grid_offset = map_grid_offset(xx, yy);
                if (map_terrain_is(grid_offset, TERRAIN_ROAD)) {
                    if (map_routing_distance(grid_offset) > 0) {
                        if (x_road && y_road) {
                            *x_road = xx;
                            *y_road = yy;
                        }
                        return 1;
                    }
                }
            }
        }
    }
    return 0;
}

int map_road_to_largest_network(int x, int y, int size, int *x_road, int *y_road)
{
    int min_index = 12;
    int min_grid_offset = -1;
    int base_offset = map_grid_offset(x, y);
    for (const int *tile_delta = map_grid_adjacent_offsets(size); *tile_delta; tile_delta++) {
        int grid_offset = base_offset + *tile_delta;
        if (map_terrain_is(grid_offset, TERRAIN_ROAD) && map_routing_distance(grid_offset) > 0) {
            int index = city_map_road_network_index(map_road_network_get(grid_offset));
            if (index < min_index) {
                min_index = index;
                min_grid_offset = grid_offset;
            }
        }
    }
    if (min_index < 12) {
        *x_road = map_grid_offset_to_x(min_grid_offset);
        *y_road = map_grid_offset_to_y(min_grid_offset);
        return min_grid_offset;
    }
    int min_dist = 100000;
    min_grid_offset = -1;
    for (const int *tile_delta = map_grid_adjacent_offsets(size); *tile_delta; tile_delta++) {
        int grid_offset = base_offset + *tile_delta;
        int dist = map_routing_distance(grid_offset);
        if (dist > 0 && dist < min_dist) {
            min_dist = dist;
            min_grid_offset = grid_offset;
        }
    }
    if (min_grid_offset >= 0) {
        *x_road = map_grid_offset_to_x(min_grid_offset);
        *y_road = map_grid_offset_to_y(min_grid_offset);
        return min_grid_offset;
    }
    return -1;
}

static void check_road_to_largest_network_hippodrome(int x, int y, int *min_index, int *min_grid_offset)
{
    int base_offset = map_grid_offset(x, y);
    for (const int *tile_delta = map_grid_adjacent_offsets(5); *tile_delta; tile_delta++) {
        int grid_offset = base_offset + *tile_delta;
        if (map_terrain_is(grid_offset, TERRAIN_ROAD) && map_routing_distance(grid_offset) > 0) {
            int index = city_map_road_network_index(map_road_network_get(grid_offset));
            if (index < *min_index) {
                *min_index = index;
                *min_grid_offset = grid_offset;
            }
        }
    }
}

static void check_min_dist_hippodrome(int base_offset, int x_offset, int *min_dist, int *min_grid_offset, int *min_x_offset)
{
    for (const int *tile_delta = map_grid_adjacent_offsets(5); *tile_delta; tile_delta++) {
        int grid_offset = base_offset + *tile_delta;
        int dist = map_routing_distance(grid_offset);
        if (dist > 0 && dist < *min_dist) {
            *min_dist = dist;
            *min_grid_offset = grid_offset;
            *min_x_offset = x_offset;
        }
    }
}

int map_road_to_largest_network_hippodrome(int x, int y, int *x_road, int *y_road)
{
    int min_index = 12;
    int min_grid_offset = -1;
    check_road_to_largest_network_hippodrome(x, y, &min_index, &min_grid_offset);
    check_road_to_largest_network_hippodrome(x + 5, y, &min_index, &min_grid_offset);
    check_road_to_largest_network_hippodrome(x + 10, y, &min_index, &min_grid_offset);

    if (min_index < 12) {
        *x_road = map_grid_offset_to_x(min_grid_offset);
        *y_road = map_grid_offset_to_y(min_grid_offset);
        return min_grid_offset;
    }

    int min_dist = 100000;
    min_grid_offset = -1;
    int min_x_offset = -1;
    check_min_dist_hippodrome(map_grid_offset(x, y), 0, &min_dist, &min_grid_offset, &min_x_offset);
    check_min_dist_hippodrome(map_grid_offset(x + 5, y), 5, &min_dist, &min_grid_offset, &min_x_offset);
    check_min_dist_hippodrome(map_grid_offset(x + 10, y), 10, &min_dist, &min_grid_offset, &min_x_offset);

    if (min_grid_offset >= 0) {
        *x_road = map_grid_offset_to_x(min_grid_offset) + min_x_offset;
        *y_road = map_grid_offset_to_y(min_grid_offset);
        return min_grid_offset + min_x_offset;
    }
    return -1;
}

int map_can_place_road_under_aqueduct(int grid_offset)
{
    int image_id = map_image_at(grid_offset) - image_group(GROUP_BUILDING_AQUEDUCT);
    int check_y;
    switch (image_id) {
        case 0:
        case 2:
        case 8:
        case 15:
        case 17:
        case 23:
            check_y = 1;
            break;
        case 1:
        case 3:
        case 9: case 10: case 11: case 12: case 13: case 14:
        case 16:
        case 18:
        case 24: case 25: case 26: case 27: case 28: case 29:
            check_y = 0;
            break;
        default: // not a straight aqueduct
            return 0;
    }
    if (city_view_orientation() == DIR_6_LEFT || city_view_orientation() == DIR_2_RIGHT) {
        check_y = !check_y;
    }
    if (check_y) {
        int dy_up = map_grid_delta(0, -1);
        int dy_down = map_grid_delta(0, 1);
        if (map_terrain_is(grid_offset + dy_up, TERRAIN_ROAD) ||
            map_routing_distance(grid_offset + dy_up) > 0) {
            return 0;
        }
        if (map_terrain_is(grid_offset + dy_down, TERRAIN_ROAD) ||
            map_routing_distance(grid_offset + dy_down) > 0) {
            return 0;
        }
    } else {
        int dx_left = map_grid_delta(-1, 0);
        int dx_right = map_grid_delta(1, 0);
        if (map_terrain_is(grid_offset + dx_left, TERRAIN_ROAD) ||
            map_routing_distance(grid_offset + dx_left) > 0) {
            return 0;
        }
        if (map_terrain_is(grid_offset + dx_right, TERRAIN_ROAD) ||
            map_routing_distance(grid_offset + dx_right) > 0) {
            return 0;
        }
    }
    return 1;
}

static int map_can_place_aqueduct_on_road(int grid_offset)
{
    int image_id = map_image_at(grid_offset) - image_group(GROUP_TERRAIN_ROAD);
    if (image_id != 0 && image_id != 1 && image_id != 49 && image_id != 50) {
        return 0;
    }
    int check_y = image_id == 0 || image_id == 49;
    if (city_view_orientation() == DIR_6_LEFT || city_view_orientation() == DIR_2_RIGHT) {
        check_y = !check_y;
    }
    if (check_y) {
        if (map_routing_distance(grid_offset + map_grid_delta(0, -1)) > 0 ||
            map_routing_distance(grid_offset + map_grid_delta(0, 1)) > 0) {
            return 0;
        }
    } else {
        if (map_routing_distance(grid_offset + map_grid_delta(-1, 0)) > 0 ||
            map_routing_distance(grid_offset + map_grid_delta(1, 0)) > 0) {
            return 0;
        }
    }
    return 1;
}

int map_get_aqueduct_with_road_image(int grid_offset)
{
    int image_id = map_image_at(grid_offset) - image_group(GROUP_BUILDING_AQUEDUCT);
    switch (image_id) {
        case 2:
            return 8;
        case 17:
            return 23;
        case 3:
            return 9;
        case 18:
            return 24;
        case 0:
        case 1:
        case 8:
        case 9:
        case 15:
        case 16:
        case 23:
        case 24:
            // unchanged
            return image_id;
        default:
            // shouldn't happen
            return 8;
    }
}

void map_road_network_clear(void)
{
    memset(network.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

int map_road_network_get(int grid_offset)
{
    return network.items[grid_offset];
}

void map_road_network_update(void)
{
    for (int i = 0; i < 10; i++) {
        city_data.map.largest_road_networks[i].id = 0;
        city_data.map.largest_road_networks[i].size = 0;
    }
    memset(network.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
    int network_id = 1;
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_terrain_is(grid_offset, TERRAIN_ROAD) && !network.items[grid_offset]) {
                memset(&road_network_queue, 0, sizeof(road_network_queue));
                int guard = 0;
                int next_offset;
                int size = 1;
                do {
                    if (++guard >= GRID_SIZE * GRID_SIZE) {
                        break;
                    }
                    network.items[grid_offset] = network_id;
                    next_offset = -1;
                    for (int i = 0; i < 4; i++) {
                        int new_offset = grid_offset + ADJACENT_OFFSETS[i];
                        if (map_routing_citizen_is_passable(new_offset) && !network.items[new_offset]) {
                            if (terrain_land_citizen.items[new_offset] == CITIZEN_0_ROAD || map_terrain_is(new_offset, TERRAIN_ACCESS_RAMP)) {
                                network.items[new_offset] = network_id;
                                size++;
                                if (next_offset == -1) {
                                    next_offset = new_offset;
                                } else {
                                    road_network_queue.items[road_network_queue.tail++] = new_offset;
                                    if (road_network_queue.tail >= MAX_QUEUE) {
                                        road_network_queue.tail = 0;
                                    }
                                }
                            }
                        }
                    }
                    if (next_offset == -1) {
                        if (road_network_queue.head == road_network_queue.tail) {
                            break;
                        }
                        next_offset = road_network_queue.items[road_network_queue.head++];
                        if (road_network_queue.head >= MAX_QUEUE) {
                            road_network_queue.head = 0;
                        }
                    }
                    grid_offset = next_offset;
                } while (next_offset > -1);
                for (int n = 0; n < 10; n++) {
                    if (size > city_data.map.largest_road_networks[n].size) {
                        // move everyone down
                        for (int m = 9; m > n; m--) {
                            city_data.map.largest_road_networks[m].id = city_data.map.largest_road_networks[m - 1].id;
                            city_data.map.largest_road_networks[m].size = city_data.map.largest_road_networks[m - 1].size;
                        }
                        city_data.map.largest_road_networks[n].id = network_id;
                        city_data.map.largest_road_networks[n].size = size;
                        break;
                    }
                }
                network_id++;
            }
        }
    }
}

static void adjust_tile_in_direction(int direction, int *x, int *y, int *grid_offset)
{
    switch (direction) {
        case DIR_0_TOP:
            --*y;
            break;
        case DIR_1_TOP_RIGHT:
            ++*x;
            --*y;
            break;
        case DIR_2_RIGHT:
            ++*x;
            break;
        case DIR_3_BOTTOM_RIGHT:
            ++*x;
            ++*y;
            break;
        case DIR_4_BOTTOM:
            ++*y;
            break;
        case DIR_5_BOTTOM_LEFT:
            --*x;
            ++*y;
            break;
        case DIR_6_LEFT:
            --*x;
            break;
        case DIR_7_TOP_LEFT:
            --*x;
            --*y;
            break;
    }
    *grid_offset += map_grid_direction_delta(direction);
}

int map_routing_get_path(uint8_t *path, int src_x, int src_y, int dst_x, int dst_y, int num_directions)
{
    int dst_grid_offset = map_grid_offset(dst_x, dst_y);
    int distance = map_routing_distance(dst_grid_offset);
    if (distance <= 0 || distance >= 998) {
        return 0;
    }

    int num_tiles = 0;
    int last_direction = -1;
    int x = dst_x;
    int y = dst_y;
    int grid_offset = dst_grid_offset;
    int step = num_directions == 8 ? 1 : 2;

    while (distance > 1) {
        distance = map_routing_distance(grid_offset);
        int direction = -1;
        int general_direction = calc_general_direction(x, y, src_x, src_y);
        for (int d = 0; d < 8; d += step) {
            if (d != last_direction) {
                int next_offset = grid_offset + map_grid_direction_delta(d);
                int next_distance = map_routing_distance(next_offset);
                if (next_distance) {
                    if (next_distance < distance) {
                        distance = next_distance;
                        direction = d;
                    } else if (next_distance == distance && (d == general_direction || direction == -1)) {
                        distance = next_distance;
                        direction = d;
                    }
                }
            }
        }
        if (direction == -1) {
            return 0;
        }
        adjust_tile_in_direction(direction, &x, &y, &grid_offset);
        int forward_direction = (direction + 4) % 8;
        direction_path[num_tiles++] = forward_direction;
        last_direction = forward_direction;
        if (num_tiles >= MAX_PATH) {
            return 0;
        }
    }
    for (int i = 0; i < num_tiles; i++) {
        path[i] = direction_path[num_tiles - i - 1];
    }
    return num_tiles;
}

int map_routing_get_closest_tile_within_range(int src_x, int src_y, int dst_x, int dst_y, int num_directions, int range, int *out_x, int *out_y)
{
    int dst_grid_offset = map_grid_offset(dst_x, dst_y);
    int distance = map_routing_distance(dst_grid_offset);
    if (distance <= 0 || distance >= 998) {
        return 0;
    }

    int num_tiles = 0;
    int last_direction = -1;
    int x = dst_x;
    int y = dst_y;
    int grid_offset = dst_grid_offset;
    int step = num_directions == 8 ? 1 : 2;

    while (distance > 1) {
        distance = map_routing_distance(grid_offset);
        *out_x = x;
        *out_y = y;
        if (distance <= range) {
            return 1;
        }
        int direction = -1;
        int general_direction = calc_general_direction(x, y, src_x, src_y);
        for (int d = 0; d < 8; d += step) {
            if (d != last_direction) {
                int next_offset = grid_offset + map_grid_direction_delta(d);
                int next_distance = map_routing_distance(next_offset);
                if (next_distance) {
                    if (next_distance < distance) {
                        distance = next_distance;
                        direction = d;
                    } else if (next_distance == distance && (d == general_direction || direction == -1)) {
                        distance = next_distance;
                        direction = d;
                    }
                }
            }
        }
        if (direction == -1) {
            return 0;
        }
        adjust_tile_in_direction(direction, &x, &y, &grid_offset);
        int forward_direction = (direction + 4) % 8;
        direction_path[num_tiles++] = forward_direction;
        last_direction = forward_direction;
        if (num_tiles >= MAX_PATH) {
            return 0;
        }
    }
    return 0;
}

int map_routing_get_path_on_water(uint8_t *path, int dst_x, int dst_y, int is_flotsam)
{
    int rand = random_byte() & 3;
    int dst_grid_offset = map_grid_offset(dst_x, dst_y);
    int distance = map_routing_distance(dst_grid_offset);
    if (distance <= 0 || distance >= 998) {
        return 0;
    }

    int num_tiles = 0;
    int last_direction = -1;
    int x = dst_x;
    int y = dst_y;
    int grid_offset = dst_grid_offset;
    while (distance > 1) {
        int current_rand = rand;
        distance = map_routing_distance(grid_offset);
        if (is_flotsam) {
            current_rand = map_random_get(grid_offset) & 3;
        }
        int direction = -1;
        for (int d = 0; d < 8; d++) {
            if (d != last_direction) {
                int next_offset = grid_offset + map_grid_direction_delta(d);
                int next_distance = map_routing_distance(next_offset);
                if (next_distance) {
                    if (next_distance < distance) {
                        distance = next_distance;
                        direction = d;
                    } else if (next_distance == distance && rand == current_rand) {
                        // allow flotsam to wander
                        distance = next_distance;
                        direction = d;
                    }
                }
            }
        }
        if (direction == -1) {
            return 0;
        }
        adjust_tile_in_direction(direction, &x, &y, &grid_offset);
        int forward_direction = (direction + 4) % 8;
        direction_path[num_tiles++] = forward_direction;
        last_direction = forward_direction;
        if (num_tiles >= MAX_PATH) {
            return 0;
        }
    }
    for (int i = 0; i < num_tiles; i++) {
        path[i] = direction_path[num_tiles - i - 1];
    }
    return num_tiles;
}

void map_routing_update_all(void)
{
    map_routing_update_land();
    map_routing_update_water();
    map_routing_update_walls();
}

void map_routing_update_land(void)
{
    map_routing_update_land_citizen();
    memset(terrain_land_noncitizen.items, -1, GRID_SIZE * GRID_SIZE * sizeof(int8_t));
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (terrain_grid.items[grid_offset] & TERRAIN_GATEHOUSE) {
                terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_4_GATEHOUSE;
            } else if (terrain_grid.items[grid_offset] & TERRAIN_ROAD) {
                terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_0_PASSABLE;
            } else if (terrain_grid.items[grid_offset] & (TERRAIN_GARDEN | TERRAIN_ACCESS_RAMP | TERRAIN_RUBBLE)) {
                terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_2_CLEARABLE;
            } else if (terrain_grid.items[grid_offset] & TERRAIN_BUILDING) {
                switch (all_buildings[map_building_at(grid_offset)].type) {
                    case BUILDING_WAREHOUSE:
                    case BUILDING_FORT_GROUND:
                        terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_0_PASSABLE;
                        break;
                    case BUILDING_BURNING_RUIN:
                    case BUILDING_NATIVE_HUT:
                    case BUILDING_NATIVE_MEETING:
                    case BUILDING_NATIVE_CROPS:
                        terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_N1_BLOCKED;
                        break;
                    case BUILDING_FORT_LEGIONARIES:
                    case BUILDING_FORT_JAVELIN:
                    case BUILDING_FORT_MOUNTED:
                        terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_5_FORT;
                        break;
                    case BUILDING_GRANARY:
                        switch (map_property_multi_tile_xy(grid_offset)) {
                            case EDGE_X1Y0:
                            case EDGE_X0Y1:
                            case EDGE_X1Y1:
                            case EDGE_X2Y1:
                            case EDGE_X1Y2:
                                terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_0_PASSABLE;
                                break;
                        }
                        break;
                    default:
                        terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_1_BUILDING;
                        break;
                }
            } else if (terrain_grid.items[grid_offset] & TERRAIN_AQUEDUCT) {
                terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_2_CLEARABLE;
            } else if (terrain_grid.items[grid_offset] & TERRAIN_WALL) {
                terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_3_WALL;
            } else if (terrain_grid.items[grid_offset] & TERRAIN_NOT_CLEAR) {
                terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_N1_BLOCKED;
            } else {
                terrain_land_noncitizen.items[grid_offset] = NONCITIZEN_0_PASSABLE;
            }
        }
    }
}

void map_routing_update_land_citizen(void)
{
    memset(terrain_land_citizen.items, -1, GRID_SIZE * GRID_SIZE * sizeof(int8_t));
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (terrain_grid.items[grid_offset] & TERRAIN_ROAD) {
                terrain_land_citizen.items[grid_offset] = CITIZEN_0_ROAD;
            } else if (terrain_grid.items[grid_offset] & (TERRAIN_RUBBLE | TERRAIN_ACCESS_RAMP | TERRAIN_GARDEN)) {
                terrain_land_citizen.items[grid_offset] = CITIZEN_2_PASSABLE_TERRAIN;
            } else if (terrain_grid.items[grid_offset] & (TERRAIN_BUILDING | TERRAIN_GATEHOUSE)) {
                if (!map_building_at(grid_offset)) {
                    // shouldn't happen
                    terrain_land_noncitizen.items[grid_offset] = CITIZEN_4_CLEAR_TERRAIN; // BUG: should be citizen?
                    terrain_grid.items[grid_offset] &= ~TERRAIN_BUILDING;
                    map_image_set(grid_offset, (map_random_get(grid_offset) & 7) + image_group(GROUP_TERRAIN_GRASS_1));
                    map_property_mark_draw_tile(grid_offset);
                    map_property_set_multi_tile_size(grid_offset, 1);
                    continue;
                }
                struct building_t *b = &all_buildings[map_building_at(grid_offset)];
                switch (b->type) {
                    case BUILDING_WAREHOUSE:
                    case BUILDING_GATEHOUSE:
                        terrain_land_citizen.items[grid_offset] = CITIZEN_0_ROAD;
                        break;
                    case BUILDING_FORT_GROUND:
                        terrain_land_citizen.items[grid_offset] = CITIZEN_2_PASSABLE_TERRAIN;
                        break;
                    case BUILDING_TRIUMPHAL_ARCH:
                        if (b->subtype.orientation == 3) {
                            switch (map_property_multi_tile_xy(grid_offset)) {
                                case EDGE_X0Y1:
                                case EDGE_X1Y1:
                                case EDGE_X2Y1:
                                    terrain_land_citizen.items[grid_offset] = CITIZEN_0_ROAD;
                                    break;
                            }
                        } else {
                            switch (map_property_multi_tile_xy(grid_offset)) {
                                case EDGE_X1Y0:
                                case EDGE_X1Y1:
                                case EDGE_X1Y2:
                                    terrain_land_citizen.items[grid_offset] = CITIZEN_0_ROAD;
                                    break;
                            }
                        }
                        break;
                    case BUILDING_GRANARY:
                        switch (map_property_multi_tile_xy(grid_offset)) {
                            case EDGE_X1Y0:
                            case EDGE_X0Y1:
                            case EDGE_X1Y1:
                            case EDGE_X2Y1:
                            case EDGE_X1Y2:
                                terrain_land_citizen.items[grid_offset] = CITIZEN_0_ROAD;
                                break;
                        }
                        break;
                    case BUILDING_RESERVOIR:
                        switch (map_property_multi_tile_xy(grid_offset)) {
                            case EDGE_X1Y0:
                            case EDGE_X0Y1:
                            case EDGE_X2Y1:
                            case EDGE_X1Y2:
                                terrain_land_citizen.items[grid_offset] = CITIZEN_N4_RESERVOIR_CONNECTOR; // aqueduct connect points
                                break;
                        }
                        break;
                    default:
                        terrain_land_citizen.items[grid_offset] = CITIZEN_N1_BLOCKED;
                        break;
                }
            } else if (terrain_grid.items[grid_offset] & TERRAIN_AQUEDUCT) {
                int image_id = map_image_at(grid_offset) - image_group(GROUP_BUILDING_AQUEDUCT);
                if (image_id <= 3) {
                    terrain_land_citizen.items[grid_offset] = CITIZEN_N3_AQUEDUCT;
                } else if (image_id <= 7) {
                    terrain_land_citizen.items[grid_offset] = CITIZEN_N1_BLOCKED;
                } else if (image_id <= 9) {
                    terrain_land_citizen.items[grid_offset] = CITIZEN_N3_AQUEDUCT;
                } else if (image_id <= 14) {
                    terrain_land_citizen.items[grid_offset] = CITIZEN_N1_BLOCKED;
                } else if (image_id <= 18) {
                    terrain_land_citizen.items[grid_offset] = CITIZEN_N3_AQUEDUCT;
                } else if (image_id <= 22) {
                    terrain_land_citizen.items[grid_offset] = CITIZEN_N1_BLOCKED;
                } else if (image_id <= 24) {
                    terrain_land_citizen.items[grid_offset] = CITIZEN_N3_AQUEDUCT;
                } else {
                    terrain_land_citizen.items[grid_offset] = CITIZEN_N1_BLOCKED;
                }
            } else if (terrain_grid.items[grid_offset] & TERRAIN_NOT_CLEAR) {
                terrain_land_citizen.items[grid_offset] = CITIZEN_N1_BLOCKED;
            } else {
                terrain_land_citizen.items[grid_offset] = CITIZEN_4_CLEAR_TERRAIN;
            }
        }
    }
}

void map_routing_update_water(void)
{
    memset(terrain_water.items, -1, GRID_SIZE * GRID_SIZE * sizeof(int8_t));
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_terrain_is(grid_offset, TERRAIN_WATER)
            && map_terrain_is(grid_offset + map_grid_delta(0, -1), TERRAIN_WATER)
            && map_terrain_is(grid_offset + map_grid_delta(-1, 0), TERRAIN_WATER)
            && map_terrain_is(grid_offset + map_grid_delta(1, 0), TERRAIN_WATER)
            && map_terrain_is(grid_offset + map_grid_delta(0, 1), TERRAIN_WATER)) {
                if (x > 0 && x < map_data.width - 1 &&
                    y > 0 && y < map_data.height - 1) {
                    switch (map_sprite_bridge_at(grid_offset)) {
                        case 5:
                        case 6: // low bridge middle section
                            terrain_water.items[grid_offset] = WATER_N3_LOW_BRIDGE;
                            break;
                        case 13: // ship bridge pillar
                            terrain_water.items[grid_offset] = WATER_N1_BLOCKED;
                            break;
                        default:
                            terrain_water.items[grid_offset] = WATER_0_PASSABLE;
                            break;
                    }
                } else {
                    terrain_water.items[grid_offset] = WATER_N2_MAP_EDGE;
                }
            } else {
                terrain_water.items[grid_offset] = WATER_N1_BLOCKED;
            }
        }
    }
}

static int is_wall_tile(int grid_offset)
{
    return map_terrain_is(grid_offset, TERRAIN_WALL_OR_GATEHOUSE) ? 1 : 0;
}

void map_routing_update_walls(void)
{
    memset(terrain_walls.items, -1, GRID_SIZE * GRID_SIZE * sizeof(int8_t));
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_terrain_is(grid_offset, TERRAIN_WALL)) {
                int adjacent_wall_tiles = 0;
                switch (city_view_orientation()) {
                    case DIR_0_TOP:
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(0, 1));
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(1, 1));
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(1, 0));
                        break;
                    case DIR_2_RIGHT:
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(0, 1));
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(-1, 1));
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(-1, 0));
                        break;
                    case DIR_4_BOTTOM:
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(0, -1));
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(-1, -1));
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(-1, 0));
                        break;
                    case DIR_6_LEFT:
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(0, -1));
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(1, -1));
                        adjacent_wall_tiles += is_wall_tile(grid_offset + map_grid_delta(1, 0));
                        break;
                }
                if (adjacent_wall_tiles == 3) {
                    terrain_walls.items[grid_offset] = WALL_0_PASSABLE;
                } else {
                    terrain_walls.items[grid_offset] = WALL_N1_BLOCKED;
                }
            } else if (map_terrain_is(grid_offset, TERRAIN_GATEHOUSE)) {
                terrain_walls.items[grid_offset] = WALL_0_PASSABLE;
            } else {
                terrain_walls.items[grid_offset] = WALL_N1_BLOCKED;
            }
        }
    }
}

int map_routing_is_wall_passable(int grid_offset)
{
    return terrain_walls.items[grid_offset] == WALL_0_PASSABLE;
}

int map_routing_citizen_is_passable(int grid_offset)
{
    return terrain_land_citizen.items[grid_offset] == CITIZEN_0_ROAD ||
        terrain_land_citizen.items[grid_offset] == CITIZEN_2_PASSABLE_TERRAIN;
}

void clear_distances(void)
{
    memset(routing_distance.items, 0, GRID_SIZE * GRID_SIZE * sizeof(int16_t));
}

static void enqueue(int next_offset, int dist)
{
    routing_distance.items[next_offset] = dist;
    routing_queue.items[routing_queue.tail++] = next_offset;
    if (routing_queue.tail >= MAX_QUEUE_ROUTING) {
        routing_queue.tail = 0;
    }
}

static int valid_offset(int grid_offset)
{
    return map_grid_is_valid_offset(grid_offset) && routing_distance.items[grid_offset] == 0;
}

static void route_queue(int source, int dest, void (*callback)(int next_offset, int dist))
{
    clear_distances();
    routing_queue.head = routing_queue.tail = 0;
    enqueue(source, 1);
    while (routing_queue.head != routing_queue.tail) {
        int offset = routing_queue.items[routing_queue.head];
        if (offset == dest) {
            break;
        }
        int dist = 1 + routing_distance.items[offset];
        for (int i = 0; i < 4; i++) {
            if (valid_offset(offset + ROUTE_OFFSETS[i])) {
                callback(offset + ROUTE_OFFSETS[i], dist);
            }
        }
        if (++routing_queue.head >= MAX_QUEUE_ROUTING) {
            routing_queue.head = 0;
        }
    }
}

void route_queue_dir8(int source, void (*callback)(int, int))
{
    clear_distances();
    routing_queue.head = routing_queue.tail = 0;
    enqueue(source, 1);
    int tiles = 0;
    while (routing_queue.head != routing_queue.tail) {
        if (++tiles > GUARD) {
            break;
        }
        int offset = routing_queue.items[routing_queue.head];
        int dist = 1 + routing_distance.items[offset];
        for (int i = 0; i < 8; i++) {
            if (valid_offset(offset + ROUTE_OFFSETS[i])) {
                callback(offset + ROUTE_OFFSETS[i], dist);
            }
        }
        if (++routing_queue.head >= MAX_QUEUE_ROUTING) {
            routing_queue.head = 0;
        }
    }
}

static void callback_calc_distance(int next_offset, int dist)
{
    if (terrain_land_citizen.items[next_offset] >= CITIZEN_0_ROAD) {
        enqueue(next_offset, dist);
    }
}

void map_routing_calculate_distances(int x, int y)
{
    ++stats.total_routes_calculated;
    route_queue(map_grid_offset(x, y), -1, callback_calc_distance);
}

static void callback_calc_distance_water_boat(int next_offset, int dist)
{
    if (terrain_water.items[next_offset] != WATER_N1_BLOCKED &&
        terrain_water.items[next_offset] != WATER_N3_LOW_BRIDGE) {
        enqueue(next_offset, dist);
        if (terrain_water.items[next_offset] == WATER_N2_MAP_EDGE) {
            routing_distance.items[next_offset] += 4;
        }
    }
}

void map_routing_calculate_distances_water_boat(int x, int y)
{
    int grid_offset = map_grid_offset(x, y);
    if (terrain_water.items[grid_offset] == WATER_N1_BLOCKED) {
        clear_distances();
    } else {
        clear_distances();
        memset(water_drag.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
        routing_queue.head = routing_queue.tail = 0;
        enqueue(grid_offset, 1);
        int tiles = 0;
        while (routing_queue.head != routing_queue.tail) {
            int offset = routing_queue.items[routing_queue.head];
            if (++tiles > GUARD) {
                break;
            }
            int drag = terrain_water.items[offset] == WATER_N2_MAP_EDGE ? 4 : 0;
            if (drag && water_drag.items[offset]++ < drag) {
                routing_queue.items[routing_queue.tail++] = offset;
                if (routing_queue.tail >= MAX_QUEUE_ROUTING) {
                    routing_queue.tail = 0;
                }
            } else {
                int dist = 1 + routing_distance.items[offset];
                for (int i = 0; i < 4; i++) {
                    if (valid_offset(offset + ROUTE_OFFSETS[i])) {
                        callback_calc_distance_water_boat(offset + ROUTE_OFFSETS[i], dist);
                    }
                }
            }
            if (++routing_queue.head >= MAX_QUEUE_ROUTING) {
                routing_queue.head = 0;
            }
        }
    }
}

void callback_calc_distance_water_flotsam(int next_offset, int dist)
{
    if (terrain_water.items[next_offset] != WATER_N1_BLOCKED) {
        enqueue(next_offset, dist);
    }
}

static void callback_calc_distance_build_wall(int next_offset, int dist)
{
    if (terrain_land_citizen.items[next_offset] == CITIZEN_4_CLEAR_TERRAIN) {
        enqueue(next_offset, dist);
    }
}

static void callback_calc_distance_build_road(int next_offset, int dist)
{
    int blocked = 0;
    switch (terrain_land_citizen.items[next_offset]) {
        case CITIZEN_N3_AQUEDUCT:
            if (!map_can_place_road_under_aqueduct(next_offset)) {
                routing_distance.items[next_offset] = -1;
                blocked = 1;
            }
            break;
        case CITIZEN_2_PASSABLE_TERRAIN: // rubble, garden, access ramp
        case CITIZEN_N1_BLOCKED: // non-empty land
            blocked = 1;
            break;
        default:
            if (map_terrain_is(next_offset, TERRAIN_BUILDING)) {
                blocked = 1;
            }
            break;
    }
    if (!blocked) {
        enqueue(next_offset, dist);
    }
}

static void callback_calc_distance_build_aqueduct(int next_offset, int dist)
{
    int blocked = 0;
    switch (terrain_land_citizen.items[next_offset]) {
        case CITIZEN_N3_AQUEDUCT:
        case CITIZEN_2_PASSABLE_TERRAIN: // rubble, garden, access ramp
        case CITIZEN_N1_BLOCKED: // non-empty land
            blocked = 1;
            break;
        default:
            if (map_terrain_is(next_offset, TERRAIN_BUILDING)) {
                if (terrain_land_citizen.items[next_offset] != CITIZEN_N4_RESERVOIR_CONNECTOR) {
                    blocked = 1;
                }
            }
            break;
    }
    if (map_terrain_is(next_offset, TERRAIN_ROAD) && !map_can_place_aqueduct_on_road(next_offset)) {
        routing_distance.items[next_offset] = -1;
        blocked = 1;
    }
    if (!blocked) {
        enqueue(next_offset, dist);
    }
}

static int map_can_place_initial_road_or_aqueduct(int grid_offset, int is_aqueduct)
{
    if (terrain_land_citizen.items[grid_offset] == CITIZEN_N1_BLOCKED) {
        // not open land, can only if:
        // - aqueduct should be placed, and:
        // - land is a reservoir building OR an aqueduct
        if (!is_aqueduct) {
            return 0;
        }
        if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT)) {
            return 1;
        }
        if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
            if (all_buildings[map_building_at(grid_offset)].type == BUILDING_RESERVOIR) {
                return 1;
            }
        }
        return 0;
    } else if (terrain_land_citizen.items[grid_offset] == CITIZEN_2_PASSABLE_TERRAIN) {
        // rubble, access ramp, garden
        return 0;
    } else if (terrain_land_citizen.items[grid_offset] == CITIZEN_N3_AQUEDUCT) {
        if (is_aqueduct) {
            return 0;
        }
        if (map_can_place_road_under_aqueduct(grid_offset)) {
            return 1;
        }
        return 0;
    } else {
        return 1;
    }
}

int map_routing_calculate_distances_for_building(int type, int x, int y)
{
    if (type == ROUTED_BUILDING_WALL) {
        route_queue(map_grid_offset(x, y), -1, callback_calc_distance_build_wall);
        return 1;
    }
    clear_distances();
    int source_offset = map_grid_offset(x, y);
    if (!map_can_place_initial_road_or_aqueduct(source_offset, type != ROUTED_BUILDING_ROAD)) {
        return 0;
    }
    if (map_terrain_is(source_offset, TERRAIN_ROAD) &&
        type != ROUTED_BUILDING_ROAD && !map_can_place_aqueduct_on_road(source_offset)) {
        return 0;
    }
    ++stats.total_routes_calculated;
    if (type == ROUTED_BUILDING_ROAD) {
        route_queue(source_offset, -1, callback_calc_distance_build_road);
    } else {
        route_queue(source_offset, -1, callback_calc_distance_build_aqueduct);
    }
    return 1;
}

static int callback_delete_wall_aqueduct(int next_offset, int dist)
{
    if (terrain_land_citizen.items[next_offset] < CITIZEN_0_ROAD) {
        if (map_terrain_is(next_offset, TERRAIN_AQUEDUCT | TERRAIN_WALL)) {
            terrain_grid.items[next_offset] &= ~TERRAIN_CLEARABLE;
            return UNTIL_STOP;
        }
    } else {
        enqueue(next_offset, dist);
    }
    return UNTIL_CONTINUE;
}

void map_routing_delete_first_wall_or_aqueduct(int x, int y)
{
    ++stats.total_routes_calculated;
    clear_distances();
    routing_queue.head = routing_queue.tail = 0;
    enqueue(map_grid_offset(x, y), 1);
    while (routing_queue.head != routing_queue.tail) {
        int offset = routing_queue.items[routing_queue.head];
        int dist = 1 + routing_distance.items[offset];
        for (int i = 0; i < 4; i++) {
            if (valid_offset(offset + ROUTE_OFFSETS[i])) {
                if (callback_delete_wall_aqueduct(offset + ROUTE_OFFSETS[i], dist) == UNTIL_STOP) {
                    break;
                }
            }
        }
        if (++routing_queue.head >= MAX_QUEUE_ROUTING) {
            routing_queue.head = 0;
        }
    }
}

static void callback_travel_citizen_land(int next_offset, int dist)
{
    if (terrain_land_citizen.items[next_offset] >= 0) {
        enqueue(next_offset, dist);
    }
}

int map_routing_citizen_can_travel_over_land(int src_x, int src_y, int dst_x, int dst_y)
{
    int src_offset = map_grid_offset(src_x, src_y);
    int dst_offset = map_grid_offset(dst_x, dst_y);
    ++stats.total_routes_calculated;
    route_queue(src_offset, dst_offset, callback_travel_citizen_land);
    return routing_distance.items[dst_offset] != 0;
}

static void callback_travel_citizen_road_garden(int next_offset, int dist)
{
    if (terrain_land_citizen.items[next_offset] >= CITIZEN_0_ROAD &&
        terrain_land_citizen.items[next_offset] <= CITIZEN_2_PASSABLE_TERRAIN) {
        enqueue(next_offset, dist);
    }
}

int map_routing_citizen_can_travel_over_road_garden(int src_x, int src_y, int dst_x, int dst_y)
{
    int src_offset = map_grid_offset(src_x, src_y);
    int dst_offset = map_grid_offset(dst_x, dst_y);
    ++stats.total_routes_calculated;
    route_queue(src_offset, dst_offset, callback_travel_citizen_road_garden);
    return routing_distance.items[dst_offset] != 0;
}

static void callback_travel_walls(int next_offset, int dist)
{
    if (terrain_walls.items[next_offset] >= WALL_0_PASSABLE &&
        terrain_walls.items[next_offset] <= 2) {
        enqueue(next_offset, dist);
    }
}

int map_routing_can_travel_over_walls(int src_x, int src_y, int dst_x, int dst_y)
{
    int src_offset = map_grid_offset(src_x, src_y);
    int dst_offset = map_grid_offset(dst_x, dst_y);
    ++stats.total_routes_calculated;
    route_queue(src_offset, dst_offset, callback_travel_walls);
    return routing_distance.items[dst_offset] != 0;
}

static void callback_travel_noncitizen_land_through_building(int next_offset, int dist)
{
    if (terrain_land_noncitizen.items[next_offset] == NONCITIZEN_0_PASSABLE ||
        terrain_land_noncitizen.items[next_offset] == NONCITIZEN_2_CLEARABLE ||
        (terrain_land_noncitizen.items[next_offset] == NONCITIZEN_1_BUILDING &&
            map_building_at(next_offset) == route_through_building_id)) {
        enqueue(next_offset, dist);
    }
}

static void callback_travel_noncitizen_land(int next_offset, int dist)
{
    if (terrain_land_noncitizen.items[next_offset] >= NONCITIZEN_0_PASSABLE &&
        terrain_land_noncitizen.items[next_offset] <= NONCITIZEN_5_FORT) {
        enqueue(next_offset, dist);
    }
}

int map_routing_noncitizen_can_travel_over_land(int src_x, int src_y, int dst_x, int dst_y, int only_through_building_id, int max_tiles)
{
    int src_offset = map_grid_offset(src_x, src_y);
    int dst_offset = map_grid_offset(dst_x, dst_y);
    ++stats.total_routes_calculated;
    ++stats.enemy_routes_calculated;
    if (only_through_building_id) {
        route_through_building_id = only_through_building_id;
        route_queue(src_offset, dst_offset, callback_travel_noncitizen_land_through_building);
    } else {
        clear_distances();
        routing_queue.head = routing_queue.tail = 0;
        enqueue(src_offset, 1);
        int tiles = 0;
        while (routing_queue.head != routing_queue.tail) {
            int offset = routing_queue.items[routing_queue.head];
            if (offset == dst_offset) break;
            if (++tiles > max_tiles) break;
            int dist = 1 + routing_distance.items[offset];
            for (int i = 0; i < 4; i++) {
                if (valid_offset(offset + ROUTE_OFFSETS[i])) {
                    callback_travel_noncitizen_land(offset + ROUTE_OFFSETS[i], dist);
                }
            }
            if (++routing_queue.head >= MAX_QUEUE_ROUTING) {
                routing_queue.head = 0;
            }
        }
    }
    return routing_distance.items[dst_offset] != 0;
}

static void callback_travel_noncitizen_through_everything(int next_offset, int dist)
{
    if (terrain_land_noncitizen.items[next_offset] >= NONCITIZEN_0_PASSABLE) {
        enqueue(next_offset, dist);
    }
}

int map_routing_noncitizen_can_travel_through_everything(int src_x, int src_y, int dst_x, int dst_y)
{
    int src_offset = map_grid_offset(src_x, src_y);
    int dst_offset = map_grid_offset(dst_x, dst_y);
    ++stats.total_routes_calculated;
    route_queue(src_offset, dst_offset, callback_travel_noncitizen_through_everything);
    return routing_distance.items[dst_offset] != 0;
}

int map_routing_distance(int grid_offset)
{
    return routing_distance.items[grid_offset];
}

void map_routing_save_state(struct buffer_t *buf)
{
    buffer_write_i32(buf, stats.enemy_routes_calculated);
    buffer_write_i32(buf, stats.total_routes_calculated);
}

void map_routing_load_state(struct buffer_t *buf)
{
    stats.enemy_routes_calculated = buffer_read_i32(buf);
    stats.total_routes_calculated = buffer_read_i32(buf);
}

int map_sprite_animation_at(int grid_offset)
{
    return sprite.items[grid_offset];
}

void map_sprite_animation_set(int grid_offset, int value)
{
    sprite.items[grid_offset] = value;
}

int map_sprite_bridge_at(int grid_offset)
{
    return sprite.items[grid_offset];
}

void map_sprite_bridge_set(int grid_offset, int value)
{
    sprite.items[grid_offset] = value;
}

void map_sprite_clear_tile(int grid_offset)
{
    sprite.items[grid_offset] = 0;
}

void map_sprite_clear(void)
{
    memset(sprite.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

void map_sprite_backup(void)
{
    memcpy(sprite_backup.items, sprite.items, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

void map_sprite_restore(void)
{
    memcpy(sprite.items, sprite_backup.items, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
}

void map_sprite_save_state(struct buffer_t *buf, struct buffer_t *backup)
{
    buffer_write_raw(buf, sprite.items, GRID_SIZE * GRID_SIZE);
    buffer_write_raw(backup, sprite_backup.items, GRID_SIZE * GRID_SIZE);
}

void map_sprite_load_state(struct buffer_t *buf, struct buffer_t *backup)
{
    buffer_read_raw(buf, sprite.items, GRID_SIZE * GRID_SIZE);
    buffer_read_raw(backup, sprite_backup.items, GRID_SIZE * GRID_SIZE);
}

int map_terrain_is(int grid_offset, int terrain)
{
    return map_grid_is_valid_offset(grid_offset) && terrain_grid.items[grid_offset] & terrain;
}

int map_terrain_count_directly_adjacent_with_type(int grid_offset, int terrain)
{
    int count = 0;
    if (map_terrain_is(grid_offset + map_grid_delta(0, -1), terrain)) {
        count++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(1, 0), terrain)) {
        count++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(0, 1), terrain)) {
        count++;
    }
    if (map_terrain_is(grid_offset + map_grid_delta(-1, 0), terrain)) {
        count++;
    }
    return count;
}

int map_terrain_has_adjacent_x_with_type(int grid_offset, int terrain)
{
    if (map_terrain_is(grid_offset + map_grid_delta(0, -1), terrain) ||
        map_terrain_is(grid_offset + map_grid_delta(0, 1), terrain)) {
        return 1;
    }
    return 0;
}

int map_terrain_has_adjacent_y_with_type(int grid_offset, int terrain)
{
    if (map_terrain_is(grid_offset + map_grid_delta(-1, 0), terrain) ||
        map_terrain_is(grid_offset + map_grid_delta(1, 0), terrain)) {
        return 1;
    }
    return 0;
}

int map_terrain_exists_tile_in_area_with_type(int x, int y, int size, int terrain)
{
    for (int yy = y; yy < y + size; yy++) {
        for (int xx = x; xx < x + size; xx++) {
            if (map_grid_is_inside(xx, yy, 1) && terrain_grid.items[map_grid_offset(xx, yy)] & terrain) {
                return 1;
            }
        }
    }
    return 0;
}

int map_terrain_exists_tile_in_radius_with_type(int x, int y, int size, int radius, int terrain)
{
    int x_min, y_min, x_max, y_max;
    map_grid_get_area(x, y, size, radius, &x_min, &y_min, &x_max, &y_max);

    for (int yy = y_min; yy <= y_max; yy++) {
        for (int xx = x_min; xx <= x_max; xx++) {
            if (map_terrain_is(map_grid_offset(xx, yy), terrain)) {
                return 1;
            }
        }
    }
    return 0;
}

int map_terrain_exist_multiple_tiles_in_radius_with_type(int x, int y, int size, int radius, int terrain, int count)
{
    int x_min, y_min, x_max, y_max;
    map_grid_get_area(x, y, size, radius, &x_min, &y_min, &x_max, &y_max);

    int tiles_found = 0;
    for (int yy = y_min; yy <= y_max; yy++) {
        for (int xx = x_min; xx <= x_max; xx++) {
            if (map_terrain_is(map_grid_offset(xx, yy), terrain)) {
                tiles_found++;
                if (tiles_found >= count) {
                    return 1;
                }
            }
        }
    }

    return 0;
}

int map_terrain_all_tiles_in_radius_are(int x, int y, int size, int radius, int terrain)
{
    int x_min, y_min, x_max, y_max;
    map_grid_get_area(x, y, size, radius, &x_min, &y_min, &x_max, &y_max);

    for (int yy = y_min; yy <= y_max; yy++) {
        for (int xx = x_min; xx <= x_max; xx++) {
            if (!map_terrain_is(map_grid_offset(xx, yy), terrain)) {
                return 0;
            }
        }
    }
    return 1;
}

int map_terrain_is_adjacent_to_open_water(int x, int y, int size)
{
    int base_offset = map_grid_offset(x, y);
    for (const int *tile_delta = map_grid_adjacent_offsets(size); *tile_delta; tile_delta++) {
        if (map_terrain_is(base_offset + *tile_delta, TERRAIN_WATER) &&
            map_routing_distance(base_offset + *tile_delta) > 0) {
            return 1;
        }
    }
    return 0;
}

int map_terrain_get_adjacent_road_or_clear_land(int x, int y, int size, int *x_tile, int *y_tile)
{
    int base_offset = map_grid_offset(x, y);
    for (const int *tile_delta = map_grid_adjacent_offsets(size); *tile_delta; tile_delta++) {
        int grid_offset = base_offset + *tile_delta;
        if (map_terrain_is(grid_offset, TERRAIN_ROAD) ||
            !map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
            *x_tile = map_grid_offset_to_x(grid_offset);
            *y_tile = map_grid_offset_to_y(grid_offset);
            return 1;
        }
    }
    return 0;
}

static void add_road(int grid_offset)
{
    if (!map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
        terrain_grid.items[grid_offset] |= TERRAIN_ROAD;
    }
}

void map_terrain_add_gatehouse_roads(int x, int y, int orientation)
{
    // roads under gatehouse
    terrain_grid.items[map_grid_offset(x, y)] |= TERRAIN_ROAD;
    terrain_grid.items[map_grid_offset(x + 1, y)] |= TERRAIN_ROAD;
    terrain_grid.items[map_grid_offset(x, y + 1)] |= TERRAIN_ROAD;
    terrain_grid.items[map_grid_offset(x + 1, y + 1)] |= TERRAIN_ROAD;

    // free roads before/after gate
    if (orientation == 1) {
        add_road(map_grid_offset(x, y - 1));
        add_road(map_grid_offset(x + 1, y - 1));
        add_road(map_grid_offset(x, y + 2));
        add_road(map_grid_offset(x + 1, y + 2));
    } else if (orientation == 2) {
        add_road(map_grid_offset(x - 1, y));
        add_road(map_grid_offset(x - 1, y + 1));
        add_road(map_grid_offset(x + 2, y));
        add_road(map_grid_offset(x + 2, y + 1));
    }
}

void map_terrain_add_triumphal_arch_roads(int x, int y, int orientation)
{
    if (orientation == 1) {
        // road in the middle
        terrain_grid.items[map_grid_offset(x + 1, y)] |= TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 1, y + 1)] |= TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 1, y + 2)] |= TERRAIN_ROAD;
        // no roads on other tiles
        terrain_grid.items[map_grid_offset(x, y)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x, y + 1)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x, y + 2)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y + 1)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y + 2)] &= ~TERRAIN_ROAD;
    } else if (orientation == 2) {
        // road in the middle
        terrain_grid.items[map_grid_offset(x, y + 1)] |= TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 1, y + 1)] |= TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y + 1)] |= TERRAIN_ROAD;
        // no roads on other tiles
        terrain_grid.items[map_grid_offset(x, y)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 1, y)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x, y + 2)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 1, y + 2)] &= ~TERRAIN_ROAD;
        terrain_grid.items[map_grid_offset(x + 2, y + 2)] &= ~TERRAIN_ROAD;
    }
}

void map_terrain_backup(void)
{
    memcpy(terrain_grid_backup.items, terrain_grid.items, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
}

void map_terrain_restore(void)
{
    memcpy(terrain_grid.items, terrain_grid_backup.items, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
}

void map_terrain_save_state(struct buffer_t *buf)
{
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        buffer_write_u16(buf, terrain_grid.items[i]);
    }
    buffer_write_raw(buf, terrain_elevation.items, GRID_SIZE * GRID_SIZE);
}

void map_terrain_load_state(struct buffer_t *buf)
{
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        terrain_grid.items[i] = buffer_read_u16(buf);
    }
    buffer_read_raw(buf, terrain_elevation.items, GRID_SIZE * GRID_SIZE);
}

int is_clear(int x, int y, int size, int disallowed_terrain, int check_image)
{
    if (!map_grid_is_inside(x, y, size)) {
        return 0;
    }
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            if (map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR & disallowed_terrain)) {
                return 0;
            } else if (map_has_figure_at(grid_offset)) {
                return 0;
            } else if (check_image && map_image_at(grid_offset)) {
                return 0;
            }
        }
    }
    return 1;
}

void foreach_region_tile(int x_min, int y_min, int x_max, int y_max, void (*callback)(int x, int y, int grid_offset))
{
    map_grid_bound_area(&x_min, &y_min, &x_max, &y_max);
    int grid_offset = map_grid_offset(x_min, y_min);
    for (int yy = y_min; yy <= y_max; yy++) {
        for (int xx = x_min; xx <= x_max; xx++) {
            callback(xx, yy, grid_offset);
            ++grid_offset;
        }
        grid_offset += GRID_SIZE - (x_max - x_min + 1);
    }
}

void map_tiles_update_all_gardens(void)
{
    // clear garden image
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_terrain_is(grid_offset, TERRAIN_GARDEN)
            && !map_terrain_is(grid_offset, TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP)) {
                map_image_set(grid_offset, 0);
                map_property_set_multi_tile_size(grid_offset, 1);
                map_property_mark_draw_tile(grid_offset);
            }
        }
    }
    // set garden image
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_terrain_is(grid_offset, TERRAIN_GARDEN)
                && !map_terrain_is(grid_offset, TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP)) {
                if (!map_image_at(grid_offset)) {
                    int image_id = image_group(GROUP_TERRAIN_GARDEN);
                    int all_terrain_in_area_is_garden = 1;
                    if (map_grid_is_inside(x, y, 2)) {
                        for (int dy = 0; dy < 2; dy++) {
                            for (int dx = 0; dx < 2; dx++) {
                                int grid_offset2 = map_grid_offset(x + dx, y + dy);
                                if ((terrain_grid.items[grid_offset2] & TERRAIN_NOT_CLEAR) != TERRAIN_GARDEN) {
                                    all_terrain_in_area_is_garden = 0;
                                    break;
                                }
                                if (map_image_at(grid_offset2) != 0) {
                                    all_terrain_in_area_is_garden = 0;
                                    break;
                                }
                            }
                        }
                    }
                    if (all_terrain_in_area_is_garden) {
                        switch (map_random_get(grid_offset) & 3) {
                            case 0: case 1:
                                image_id += 6;
                                break;
                            case 2:
                                image_id += 5;
                                break;
                            case 3:
                                image_id += 4;
                                break;
                        }
                        map_building_tiles_add(0, x, y, 2, image_id, TERRAIN_GARDEN);
                    } else {
                        if (y & 1) {
                            switch (x & 3) {
                                case 0: case 2:
                                    image_id += 2;
                                    break;
                                case 1: case 3:
                                    image_id += 3;
                                    break;
                            }
                        } else {
                            switch (x & 3) {
                                case 1: case 3:
                                    image_id += 1;
                                    break;
                            }
                        }
                        map_image_set(grid_offset, image_id);
                    }
                }
            }
        }
    }
}

void map_tiles_determine_gardens(void)
{
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            int base_image = image_group(GROUP_TERRAIN_GARDEN);
            int image_id = map_image_at(grid_offset);
            if (image_id >= base_image && image_id <= base_image + 6) {
                terrain_grid.items[grid_offset] |= TERRAIN_GARDEN;
                map_property_clear_constructing(grid_offset);
                map_aqueduct_set(grid_offset, 0);
            }
        }
    }
}

static int is_tile_plaza(int grid_offset)
{
    if (map_terrain_is(grid_offset, TERRAIN_ROAD) &&
        map_property_is_plaza_or_earthquake(grid_offset) &&
        !map_terrain_is(grid_offset, TERRAIN_WATER | TERRAIN_BUILDING) &&
        !map_image_at(grid_offset)) {
        return 1;
    }
    return 0;
}

void map_tiles_update_all_plazas(void)
{
    // remove plaza below building
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_terrain_is(grid_offset, TERRAIN_ROAD)
                && map_property_is_plaza_or_earthquake(grid_offset)) {
                if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
                    map_property_clear_plaza_or_earthquake(grid_offset);
                }
            }
        }
    }
    // clear plaza image
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_terrain_is(grid_offset, TERRAIN_ROAD)
                && map_property_is_plaza_or_earthquake(grid_offset)) {
                map_image_set(grid_offset, 0);
                map_property_set_multi_tile_size(grid_offset, 1);
                map_property_mark_draw_tile(grid_offset);
            }
        }
    }
    // set plaza image
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_terrain_is(grid_offset, TERRAIN_ROAD)
                && map_property_is_plaza_or_earthquake(grid_offset)
                && !map_image_at(grid_offset)) {
                int image_id = image_group(GROUP_TERRAIN_PLAZA);
                if (is_tile_plaza(grid_offset + map_grid_delta(1, 0))
                && is_tile_plaza(grid_offset + map_grid_delta(0, 1))
                && is_tile_plaza(grid_offset + map_grid_delta(1, 1))) {
                    if (map_random_get(grid_offset) & 1) {
                        image_id += 7;
                    } else {
                        image_id += 6;
                    }
                    map_building_tiles_add(0, x, y, 2, image_id, TERRAIN_ROAD);
                } else {
                    // single tile plaza
                    switch ((x & 1) + (y & 1)) {
                        case 2: image_id += 1; break;
                        case 1: image_id += 2; break;
                    }
                    map_image_set(grid_offset, image_id);
                }
            }
        }
    }
}

static int get_gatehouse_building_id(int grid_offset)
{
    if (map_terrain_is(grid_offset, TERRAIN_GATEHOUSE)) {
        return map_building_at(grid_offset);
    }
    return 0;
}

static int get_gatehouse_position(int grid_offset, int direction, int building_id)
{
    int result = 0;
    if (direction == DIR_0_TOP) {
        if (map_terrain_is(grid_offset + map_grid_delta(1, -1), TERRAIN_GATEHOUSE) &&
                map_building_at(grid_offset + map_grid_delta(1, -1)) == building_id) {
            result = 1;
            if (!map_terrain_is(grid_offset + map_grid_delta(1, 0), TERRAIN_WALL)) {
                result = 0;
            }
            if (map_terrain_is(grid_offset + map_grid_delta(-1, 0), TERRAIN_WALL) &&
                map_terrain_is(grid_offset + map_grid_delta(-1, 1), TERRAIN_WALL)) {
                result = 2;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(0, 1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(1, 1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
        } else if (map_terrain_is(grid_offset + map_grid_delta(-1, -1), TERRAIN_GATEHOUSE) &&
                map_building_at(grid_offset + map_grid_delta(-1, -1)) == building_id) {
            result = 3;
            if (!map_terrain_is(grid_offset + map_grid_delta(-1, 0), TERRAIN_WALL)) {
                result = 0;
            }
            if (map_terrain_is(grid_offset + map_grid_delta(1, 0), TERRAIN_WALL) &&
                map_terrain_is(grid_offset + map_grid_delta(1, 1), TERRAIN_WALL)) {
                result = 4;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(0, 1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(-1, 1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
        }
    } else if (direction == DIR_6_LEFT) {
        if (map_terrain_is(grid_offset + map_grid_delta(-1, 1), TERRAIN_GATEHOUSE) &&
                map_building_at(grid_offset + map_grid_delta(-1, 1)) == building_id) {
            result = 1;
            if (!map_terrain_is(grid_offset + map_grid_delta(0, 1), TERRAIN_WALL)) {
                result = 0;
            }
            if (map_terrain_is(grid_offset + map_grid_delta(0, -1), TERRAIN_WALL) &&
                map_terrain_is(grid_offset + map_grid_delta(1, -1), TERRAIN_WALL)) {
                result = 2;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(1, 0), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(1, 1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
        } else if (map_terrain_is(grid_offset + map_grid_delta(-1, -1), TERRAIN_GATEHOUSE) &&
                map_building_at(grid_offset + map_grid_delta(-1, -1)) == building_id) {
            result = 3;
            if (!map_terrain_is(grid_offset + map_grid_delta(0, -1), TERRAIN_WALL)) {
                result = 0;
            }
            if (map_terrain_is(grid_offset + map_grid_delta(0, 1), TERRAIN_WALL) &&
                map_terrain_is(grid_offset + map_grid_delta(1, 1), TERRAIN_WALL)) {
                result = 4;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(1, 0), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(1, -1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
        }
    } else if (direction == DIR_4_BOTTOM) {
        if (map_terrain_is(grid_offset + map_grid_delta(1, 1), TERRAIN_GATEHOUSE) &&
                map_building_at(grid_offset + map_grid_delta(1, 1)) == building_id) {
            result = 1;
            if (!map_terrain_is(grid_offset + map_grid_delta(1, 0), TERRAIN_WALL)) {
                result = 0;
            }
            if (map_terrain_is(grid_offset + map_grid_delta(-1, 0), TERRAIN_WALL) &&
                map_terrain_is(grid_offset + map_grid_delta(-1, -1), TERRAIN_WALL)) {
                result = 2;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(0, -1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(1, -1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
        } else if (map_terrain_is(grid_offset + map_grid_delta(-1, 1), TERRAIN_GATEHOUSE) &&
                map_building_at(grid_offset + map_grid_delta(-1, 1)) == building_id) {
            result = 3;
            if (!map_terrain_is(grid_offset + map_grid_delta(-1, 0), TERRAIN_WALL)) {
                result = 0;
            }
            if (map_terrain_is(grid_offset + map_grid_delta(1, 0), TERRAIN_WALL) &&
                map_terrain_is(grid_offset + map_grid_delta(1, -1), TERRAIN_WALL)) {
                result = 4;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(0, -1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(-1, -1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
        }
    } else if (direction == DIR_2_RIGHT) {
        if (map_terrain_is(grid_offset + map_grid_delta(1, 1), TERRAIN_GATEHOUSE) &&
                map_building_at(grid_offset + map_grid_delta(1, 1)) == building_id) {
            result = 1;
            if (!map_terrain_is(grid_offset + map_grid_delta(0, 1), TERRAIN_WALL)) {
                result = 0;
            }
            if (map_terrain_is(grid_offset + map_grid_delta(0, -1), TERRAIN_WALL) &&
                map_terrain_is(grid_offset + map_grid_delta(-1, -1), TERRAIN_WALL)) {
                result = 2;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(-1, 0), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(-1, 1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
        } else if (map_terrain_is(grid_offset + map_grid_delta(1, -1), TERRAIN_GATEHOUSE) &&
                map_building_at(grid_offset + map_grid_delta(1, -1)) == building_id) {
            result = 3;
            if (!map_terrain_is(grid_offset + map_grid_delta(0, -1), TERRAIN_WALL)) {
                result = 0;
            }
            if (map_terrain_is(grid_offset + map_grid_delta(0, 1), TERRAIN_WALL) &&
                map_terrain_is(grid_offset + map_grid_delta(-1, 1), TERRAIN_WALL)) {
                result = 4;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(-1, 0), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
            if (!map_terrain_is(grid_offset + map_grid_delta(-1, -1), TERRAIN_WALL_OR_GATEHOUSE)) {
                result = 0;
            }
        }
    }
    return result;
}

void set_wall_image(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset)
{
    if (!map_terrain_is(grid_offset, TERRAIN_WALL) ||
        map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        return;
    }
    int tiles[MAX_TILES];
    fill_matches(grid_offset, TERRAIN_WALL, 0, 1, tiles);
    const struct terrain_image_t *img = get_image(CONTEXT_WALL, tiles);
    map_image_set(grid_offset, image_group(GROUP_BUILDING_WALL) + img->group_offset + img->item_offset);
    map_property_set_multi_tile_size(grid_offset, 1);
    map_property_mark_draw_tile(grid_offset);
    if (map_terrain_count_directly_adjacent_with_type(grid_offset, TERRAIN_GATEHOUSE) > 0) {
        for (int i = 0; i < MAX_TILES; i += 2) {
            tiles[i] = map_terrain_is(grid_offset + map_grid_direction_delta(i), TERRAIN_WALL_OR_GATEHOUSE) ? 1 : 0;
        }
        img = get_image(CONTEXT_WALL_GATEHOUSE, tiles);
        if (img->is_valid) {
            map_image_set(grid_offset, image_group(GROUP_BUILDING_WALL) +
                          img->group_offset + img->item_offset);
        } else {
            int gatehouse_up = get_gatehouse_building_id(grid_offset + map_grid_delta(0, -1));
            int gatehouse_left = get_gatehouse_building_id(grid_offset + map_grid_delta(-1, 0));
            int gatehouse_down = get_gatehouse_building_id(grid_offset + map_grid_delta(0, 1));
            int gatehouse_right = get_gatehouse_building_id(grid_offset + map_grid_delta(1, 0));
            int image_offset = 0;
            int map_orientation = city_view_orientation();
            if (map_orientation == DIR_0_TOP) {
                if (gatehouse_up && !gatehouse_left) {
                    int pos = get_gatehouse_position(grid_offset, DIR_0_TOP, gatehouse_up);
                    if (pos > 0) {
                        if (pos <= 2) {
                            image_offset = 29;
                        } else if (pos == 3) {
                            image_offset = 31;
                        } else {
                            image_offset = 33;
                        }
                    }
                } else if (gatehouse_left && !gatehouse_up) {
                    int pos = get_gatehouse_position(grid_offset, DIR_6_LEFT, gatehouse_left);
                    if (pos > 0) {
                        if (pos <= 2) {
                            image_offset = 30;
                        } else if (pos == 3) {
                            image_offset = 32;
                        } else {
                            image_offset = 33;
                        }
                    }
                }
            } else if (map_orientation == DIR_2_RIGHT) {
                if (gatehouse_up && !gatehouse_right) {
                    int pos = get_gatehouse_position(grid_offset, DIR_0_TOP, gatehouse_up);
                    if (pos > 0) {
                        if (pos == 1) {
                            image_offset = 32;
                        } else if (pos == 2) {
                            image_offset = 33;
                        } else {
                            image_offset = 30;
                        }
                    }
                } else if (gatehouse_right && !gatehouse_up) {
                    int pos = get_gatehouse_position(grid_offset, DIR_2_RIGHT, gatehouse_right);
                    if (pos > 0) {
                        if (pos <= 2) {
                            image_offset = 29;
                        } else if (pos == 3) {
                            image_offset = 31;
                        } else {
                            image_offset = 33;
                        }
                    }
                }
            } else if (map_orientation == DIR_4_BOTTOM) {
                if (gatehouse_down && !gatehouse_right) {
                    int pos = get_gatehouse_position(grid_offset, DIR_4_BOTTOM, gatehouse_down);
                    if (pos > 0) {
                        if (pos == 1) {
                            image_offset = 31;
                        } else if (pos == 2) {
                            image_offset = 33;
                        } else {
                            image_offset = 29;
                        }
                    }
                } else if (gatehouse_right && !gatehouse_down) {
                    int pos = get_gatehouse_position(grid_offset, DIR_2_RIGHT, gatehouse_right);
                    if (pos > 0) {
                        if (pos == 1) {
                            image_offset = 32;
                        } else if (pos == 2) {
                            image_offset = 33;
                        } else {
                            image_offset = 30;
                        }
                    }
                }
            } else if (map_orientation == DIR_6_LEFT) {
                if (gatehouse_down && !gatehouse_left) {
                    int pos = get_gatehouse_position(grid_offset, DIR_4_BOTTOM, gatehouse_down);
                    if (pos > 0) {
                        if (pos <= 2) {
                            image_offset = 30;
                        } else if (pos == 3) {
                            image_offset = 32;
                        } else {
                            image_offset = 33;
                        }
                    }
                } else if (gatehouse_left && !gatehouse_down) {
                    int pos = get_gatehouse_position(grid_offset, DIR_6_LEFT, gatehouse_left);
                    if (pos > 0) {
                        if (pos == 1) {
                            image_offset = 31;
                        } else if (pos == 2) {
                            image_offset = 33;
                        } else {
                            image_offset = 29;
                        }
                    }
                }
            }
            if (image_offset) {
                map_image_set(grid_offset, image_group(GROUP_BUILDING_WALL) + image_offset);
            }
        }
    }
}

void map_tiles_update_all_walls(void)
{
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            set_wall_image(x, y, grid_offset);
        }
    }
}

int map_tiles_set_wall(int x, int y)
{
    int grid_offset = map_grid_offset(x, y);
    int tile_set = 0;
    if (!map_terrain_is(grid_offset, TERRAIN_WALL)) {
        tile_set = 1;
    }
    terrain_grid.items[grid_offset] = TERRAIN_WALL;
    map_property_clear_constructing(grid_offset);

    foreach_region_tile(x - 1, y - 1, x + 1, y + 1, set_wall_image);
    return tile_set;
}

int map_tiles_is_paved_road(int grid_offset)
{
    int desirability = map_desirability_get(grid_offset);
    if (desirability > 4) {
        return 1;
    }
    if (desirability > 0 && map_terrain_is(grid_offset, TERRAIN_FOUNTAIN_RANGE)) {
        return 1;
    }
    return 0;
}

static void set_aqueduct_image(int grid_offset, int is_road, const struct terrain_image_t *img)
{
    int group_offset = img->group_offset;
    if (is_road) {
        if (!img->aqueduct_offset || (group_offset != 2 && group_offset != 3)) {
            if (map_terrain_is(grid_offset + map_grid_delta(0, -1), TERRAIN_ROAD)) {
                group_offset = 3;
            } else {
                group_offset = 2;
            }
        }
        if (map_tiles_is_paved_road(grid_offset)) {
            group_offset -= 2;
        } else {
            group_offset += 6;
        }
    }
    int image_aqueduct = image_group(GROUP_BUILDING_AQUEDUCT);
    int water_offset;
    int image_id = map_image_at(grid_offset);
    if (image_id >= image_aqueduct && image_id < image_aqueduct + 15) {
        water_offset = 0;
    } else {
        water_offset = 15;
    }
    map_image_set(grid_offset, image_aqueduct + water_offset + group_offset);
    map_property_set_multi_tile_size(grid_offset, 1);
    map_property_mark_draw_tile(grid_offset);
}

void set_road_image(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset)
{
    if (!map_terrain_is(grid_offset, TERRAIN_ROAD) ||
        map_terrain_is(grid_offset, TERRAIN_WATER | TERRAIN_BUILDING)) {
        return;
    }
    if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT)) {
        set_aqueduct_image(grid_offset, 1, map_image_context_get_aqueduct(grid_offset, 0));
        return;
    }
    if (map_property_is_plaza_or_earthquake(grid_offset)) {
        return;
    }
    if (map_tiles_is_paved_road(grid_offset)) {
        int tiles[MAX_TILES];
        set_tiles_road(grid_offset, tiles);
        const struct terrain_image_t *img = get_image(CONTEXT_PAVED_ROAD, tiles);
        map_image_set(grid_offset, image_group(GROUP_TERRAIN_ROAD) + img->group_offset + img->item_offset);
    } else {
        int tiles[MAX_TILES];
        set_tiles_road(grid_offset, tiles);
        const struct terrain_image_t *img = get_image(CONTEXT_DIRT_ROAD, tiles);
        map_image_set(grid_offset, image_group(GROUP_TERRAIN_ROAD) + img->group_offset + img->item_offset + 49);
    }
    map_property_set_multi_tile_size(grid_offset, 1);
    map_property_mark_draw_tile(grid_offset);
}

void map_tiles_update_all_roads(void)
{
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            set_road_image(x, y, grid_offset);
        }
    }
}

int map_tiles_set_road(int x, int y)
{
    int grid_offset = map_grid_offset(x, y);
    int tile_set = 0;
    if (!map_terrain_is(grid_offset, TERRAIN_ROAD)) {
        tile_set = 1;
    }
    terrain_grid.items[grid_offset] |= TERRAIN_ROAD;
    map_property_clear_constructing(grid_offset);

    foreach_region_tile(x - 1, y - 1, x + 1, y + 1, set_road_image);
    return tile_set;
}

static void clear_empty_land_image(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset)
{
    if (!map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR | TERRAIN_MEADOW)) {
        map_image_set(grid_offset, 0);
        map_property_set_multi_tile_size(grid_offset, 1);
        map_property_mark_draw_tile(grid_offset);
    }
}

static void set_empty_land_image(int x, int y, int size, int image_id)
{
    if (!map_grid_is_inside(x, y, size)) {
        return;
    }
    int index = 0;
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            terrain_grid.items[grid_offset] &= ~TERRAIN_CLEARABLE;
            map_building_set(grid_offset, 0);
            map_property_clear_constructing(grid_offset);
            map_property_set_multi_tile_size(grid_offset, 1);
            map_property_mark_draw_tile(grid_offset);
            map_image_set(grid_offset, image_id + index);
            index++;
        }
    }
}

static void set_empty_land_pass1(int x, int y, int grid_offset)
{
    if (!map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR) && !map_image_at(grid_offset) &&
        !(map_random_get(grid_offset) & 0xf0)) {
        int image_id;
        if (map_property_is_alternate_terrain(grid_offset)) {
            image_id = image_group(GROUP_TERRAIN_GRASS_2);
        } else {
            image_id = image_group(GROUP_TERRAIN_GRASS_1);
        }
        set_empty_land_image(x, y, 1, image_id + (map_random_get(grid_offset) & 7));
    }
}

static void set_empty_land_pass2(int x, int y, int grid_offset)
{
    if (!map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR) && !map_image_at(grid_offset)) {
        int image_id;
        if (map_property_is_alternate_terrain(grid_offset)) {
            image_id = image_group(GROUP_TERRAIN_GRASS_2);
        } else {
            image_id = image_group(GROUP_TERRAIN_GRASS_1);
        }
        if (is_clear(x, y, 4, TERRAIN_ALL, 1)) {
            set_empty_land_image(x, y, 4, image_id + 42);
        } else if (is_clear(x, y, 3, TERRAIN_ALL, 1)) {
            set_empty_land_image(x, y, 3, image_id + 24 + 9 * (map_random_get(grid_offset) & 1));
        } else if (is_clear(x, y, 2, TERRAIN_ALL, 1)) {
            set_empty_land_image(x, y, 2, image_id + 8 + 4 * (map_random_get(grid_offset) & 3));
        } else {
            set_empty_land_image(x, y, 1, image_id + (map_random_get(grid_offset) & 7));
        }
    }
}

void map_tiles_update_all_empty_land(void)
{
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            clear_empty_land_image(x, y, grid_offset);
        }
    }
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            set_empty_land_pass1(x, y, grid_offset);
        }
    }
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            set_empty_land_pass2(x, y, grid_offset);
        }
    }
}

void map_tiles_update_region_empty_land(int x_min, int y_min, int x_max, int y_max)
{
    foreach_region_tile(x_min, y_min, x_max, y_max, clear_empty_land_image);
    foreach_region_tile(x_min, y_min, x_max, y_max, set_empty_land_pass1);
    foreach_region_tile(x_min, y_min, x_max, y_max, set_empty_land_pass2);
}

static void set_meadow_image(int x, int y, int grid_offset)
{
    if (map_terrain_is(grid_offset, TERRAIN_MEADOW) && !map_terrain_is(grid_offset, FORBIDDEN_TERRAIN_MEADOW)) {
        int random = map_random_get(grid_offset) & 3;
        int image_id = image_group(GROUP_TERRAIN_MEADOW);
        if (map_terrain_all_tiles_in_radius_are(x, y, 1, 2, TERRAIN_MEADOW)) {
            map_image_set(grid_offset, image_id + random + 8);
        } else if (map_terrain_all_tiles_in_radius_are(x, y, 1, 1, TERRAIN_MEADOW)) {
            map_image_set(grid_offset, image_id + random + 4);
        } else {
            map_image_set(grid_offset, image_id + random);
        }
        map_property_set_multi_tile_size(grid_offset, 1);
        map_property_mark_draw_tile(grid_offset);
        map_aqueduct_set(grid_offset, 0);
    }
}

void update_meadow_tile(int x, int y, int grid_offset)
{
    if (map_terrain_is(grid_offset, TERRAIN_MEADOW) && !map_terrain_is(grid_offset, FORBIDDEN_TERRAIN_MEADOW)) {
        foreach_region_tile(x - 1, y - 1, x + 1, y + 1, set_meadow_image);
    }
}

void update_water_tile(int x, int y, int grid_offset)
{
    if (map_terrain_is(grid_offset, TERRAIN_WATER) && !map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        foreach_region_tile(x - 1, y - 1, x + 1, y + 1, set_water_image);
    }
}

void map_tiles_update_all_water(void)
{
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            update_water_tile(x, y, grid_offset);
        }
    }
}

void update_aqueduct_tile(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset)
{
    if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT) && map_aqueduct_at(grid_offset) <= 15) {
        const struct terrain_image_t *img = map_image_context_get_aqueduct(grid_offset, aqueduct_include_construction);
        int is_road = map_terrain_is(grid_offset, TERRAIN_ROAD);
        if (is_road) {
            map_property_clear_plaza_or_earthquake(grid_offset);
        }
        set_aqueduct_image(grid_offset, is_road, img);
        map_aqueduct_set(grid_offset, img->aqueduct_offset);
    }
}

void map_tiles_update_all_aqueducts(int include_construction)
{
    aqueduct_include_construction = include_construction;
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            update_aqueduct_tile(x, y, grid_offset);
        }
    }
    aqueduct_include_construction = 0;
}

void set_earthquake_image(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset)
{
    if (map_terrain_is(grid_offset, TERRAIN_ROCK) && map_property_is_plaza_or_earthquake(grid_offset)) {
        int tiles[MAX_TILES];
        for (int i = 0; i < MAX_TILES; i++) {
            int offset = grid_offset + map_grid_direction_delta(i);
            tiles[i] = (map_terrain_is(offset, TERRAIN_ROCK) &&
                map_property_is_plaza_or_earthquake(grid_offset)) ? 1 : 0;
        }
        const struct terrain_image_t *img = get_image(CONTEXT_EARTHQUAKE, tiles);
        if (img->is_valid) {
            map_image_set(grid_offset,
                image_group(GROUP_TERRAIN_EARTHQUAKE) + img->group_offset + img->item_offset);
        } else {
            map_image_set(grid_offset, image_group(GROUP_TERRAIN_EARTHQUAKE));
        }
        map_property_set_multi_tile_size(grid_offset, 1);
        map_property_mark_draw_tile(grid_offset);
    }
}

void map_tiles_update_all_earthquake(void)
{
    for (int y = 0, grid_offset = map_data.start_offset; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_terrain_is(grid_offset, TERRAIN_ROCK) && map_property_is_plaza_or_earthquake(grid_offset)) {
                terrain_grid.items[grid_offset] |= TERRAIN_ROCK;
                map_property_mark_plaza_or_earthquake(grid_offset);
                foreach_region_tile(x - 1, y - 1, x + 1, y + 1, set_earthquake_image);
            }
        }
    }
}

void set_rubble_image(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset)
{
    if (map_terrain_is(grid_offset, TERRAIN_RUBBLE) && !map_terrain_is(grid_offset, FORBIDDEN_TERRAIN_RUBBLE)) {
        map_image_set(grid_offset, image_group(GROUP_TERRAIN_RUBBLE) + (map_random_get(grid_offset) & 7));
        map_property_set_multi_tile_size(grid_offset, 1);
        map_property_mark_draw_tile(grid_offset);
        map_aqueduct_set(grid_offset, 0);
    }
}

static void clear_access_ramp_image(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset)
{
    if (map_terrain_is(grid_offset, TERRAIN_ACCESS_RAMP)) {
        map_image_set(grid_offset, 0);
    }
}

static void set_elevation_image(int x, int y, int grid_offset)
{
    if (map_terrain_is(grid_offset, TERRAIN_ACCESS_RAMP) && !map_image_at(grid_offset)) {
        int image_offset = -1;
        if (map_grid_is_inside(x, y, 1)) {
            static const int offsets[4][6] = {
                {OFFSET(0,1), OFFSET(1,1), OFFSET(0,0), OFFSET(1,0), OFFSET(0,2), OFFSET(1,2)},
                {OFFSET(0,0), OFFSET(0,1), OFFSET(1,0), OFFSET(1,1), OFFSET(-1,0), OFFSET(-1,1)},
                {OFFSET(0,0), OFFSET(1,0), OFFSET(0,1), OFFSET(1,1), OFFSET(0,-1), OFFSET(1,-1)},
                {OFFSET(1,0), OFFSET(1,1), OFFSET(0,0), OFFSET(0,1), OFFSET(2,0), OFFSET(2,1)},
            };
            int base_offset = map_grid_offset(x, y);
            for (int dir = 0; dir < 4; dir++) {
                int right_tiles = 0;
                int height = -1;
                for (int i = 0; i < 6; i++) {
                    int grid_offset2 = base_offset + offsets[dir][i];
                    if (i < 2) { // 2nd row
                        if (map_terrain_is(grid_offset2, TERRAIN_ELEVATION)) {
                            right_tiles++;
                        }
                        height = terrain_elevation.items[grid_offset2];
                    } else if (i < 4) { // 1st row
                        if (map_terrain_is(grid_offset2, TERRAIN_ACCESS_RAMP) &&
                            terrain_elevation.items[grid_offset2] < height) {
                            right_tiles++;
                        }
                    } else { // higher row beyond access ramp
                        if (map_terrain_is(grid_offset2, TERRAIN_ELEVATION)) {
                            if (terrain_elevation.items[grid_offset2] != height) {
                                right_tiles++;
                            }
                        } else if (terrain_elevation.items[grid_offset2] >= height) {
                            right_tiles++;
                        }
                    }
                }
                if (right_tiles == 6) {
                    image_offset = dir;
                    break;
                }
            }
            if (image_offset > -1) {
                switch (city_view_orientation()) {
                    case DIR_0_TOP: break;
                    case DIR_6_LEFT: image_offset += 1; break;
                    case DIR_4_BOTTOM: image_offset += 2; break;
                    case DIR_2_RIGHT: image_offset += 3; break;
                }
                if (image_offset >= 4) {
                    image_offset -= 4;
                }
            }
        }
        if (image_offset < 0) {
            // invalid map: remove access ramp
            terrain_grid.items[grid_offset] &= ~TERRAIN_ACCESS_RAMP;
            map_property_set_multi_tile_size(grid_offset, 1);
            map_property_mark_draw_tile(grid_offset);
            if (terrain_elevation.items[grid_offset]) {
                terrain_grid.items[grid_offset] |= TERRAIN_ELEVATION;
            } else {
                terrain_grid.items[grid_offset] &= ~TERRAIN_ELEVATION;
                map_image_set(grid_offset,
                    image_group(GROUP_TERRAIN_GRASS_1) + (map_random_get(grid_offset) & 7));
            }
        } else {
            map_building_tiles_add(0, x, y, 2,
                image_group(GROUP_TERRAIN_ACCESS_RAMP) + image_offset, TERRAIN_ACCESS_RAMP);
        }
    }
    if (terrain_elevation.items[grid_offset]
    && !map_terrain_is(grid_offset, TERRAIN_ACCESS_RAMP)
    && !map_terrain_is(grid_offset, TERRAIN_WATER)
    && !map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        int tiles[MAX_TILES];
        for (int i = 0; i < MAX_TILES; i++) {
            tiles[i] = terrain_elevation.items[grid_offset + map_grid_direction_delta(i)] >= terrain_elevation.items[grid_offset] ? 1 : 0;
        }
        const struct terrain_image_t *img = get_image(CONTEXT_ELEVATION, tiles);
        if (img->group_offset == 44) {
            terrain_grid.items[grid_offset] &= ~TERRAIN_ELEVATION;
            map_property_set_multi_tile_xy(grid_offset, 0, 0, 1);
        } else {
            map_property_set_multi_tile_xy(grid_offset, 0, 0, 1);
            terrain_grid.items[grid_offset] |= TERRAIN_ELEVATION;
            map_image_set(grid_offset, image_group(GROUP_TERRAIN_ELEVATION) + img->group_offset + img->item_offset);
        }
    }
}

void map_tiles_update_all_elevation(void)
{
    int width = map_data.width - 2;
    int height = map_data.height - 2;
    foreach_region_tile(0, 0, width, height, clear_access_ramp_image);
    foreach_region_tile(0, 0, width, height, set_elevation_image);
}

void map_tiles_add_entry_exit_flags(void)
{
    int entry_orientation;
    if (scenario.entry_point.x == 0) {
        entry_orientation = DIR_2_RIGHT;
    } else if (scenario.entry_point.x == map_data.width - 1) {
        entry_orientation = DIR_6_LEFT;
    } else if (scenario.entry_point.y == 0) {
        entry_orientation = DIR_0_TOP;
    } else if (scenario.entry_point.y == map_data.height - 1) {
        entry_orientation = DIR_4_BOTTOM;
    } else {
        entry_orientation = -1;
    }
    if (entry_orientation >= 0) {
        int grid_offset = map_grid_offset(scenario.entry_point.x, scenario.entry_point.y);
        terrain_grid.items[grid_offset] |= TERRAIN_ROCK;
        int orientation = (city_view_orientation() + entry_orientation) % 8;
        map_image_set(grid_offset, image_group(GROUP_TERRAIN_ENTRY_EXIT_FLAGS) + orientation / 2);
    }

    int exit_orientation;
    if (scenario.exit_point.x == 0) {
        exit_orientation = DIR_2_RIGHT;
    } else if (scenario.exit_point.x == map_data.width - 1) {
        exit_orientation = DIR_6_LEFT;
    } else if (scenario.exit_point.y == 0) {
        exit_orientation = DIR_0_TOP;
    } else if (scenario.exit_point.y == map_data.height - 1) {
        exit_orientation = DIR_4_BOTTOM;
    } else {
        exit_orientation = -1;
    }
    if (exit_orientation >= 0) {
        int grid_offset = map_grid_offset(scenario.exit_point.x, scenario.exit_point.y);
        terrain_grid.items[grid_offset] |= TERRAIN_ROCK;
        int orientation = (city_view_orientation() + exit_orientation) % 8;
        map_image_set(grid_offset, image_group(GROUP_TERRAIN_ENTRY_EXIT_FLAGS) + 4 + orientation / 2);
    }
}

static void map_terrain_add_with_radius(int x, int y, int size, int radius, int terrain)
{
    int x_min, y_min, x_max, y_max;
    map_grid_get_area(x, y, size, radius, &x_min, &y_min, &x_max, &y_max);

    for (int yy = y_min; yy <= y_max; yy++) {
        for (int xx = x_min; xx <= x_max; xx++) {
            terrain_grid.items[map_grid_offset(xx, yy)] |= terrain;
        }
    }
}

void map_water_supply_update_reservoir_fountain(void)
{
    for (int i = 0; i < GRID_SIZE * GRID_SIZE; i++) {
        terrain_grid.items[i] &= ~(TERRAIN_FOUNTAIN_RANGE | TERRAIN_RESERVOIR_RANGE);
    }
    // reservoirs
    int image_without_water = image_group(GROUP_BUILDING_AQUEDUCT_NO_WATER);
    int grid_offset = map_data.start_offset;
    for (int y = 0; y < map_data.height; y++, grid_offset += map_data.border_size) {
        for (int x = 0; x < map_data.width; x++, grid_offset++) {
            if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT)) {
                map_aqueduct_set(grid_offset, 0);
                int image_id = map_image_at(grid_offset);
                if (image_id < image_without_water) {
                    map_image_set(grid_offset, image_id + 15);
                }
            }
        }
    }
    building_list_large_clear(1);
    // mark reservoirs next to water
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->type == BUILDING_RESERVOIR) {
            building_list_large_add(i);
            if (map_terrain_exists_tile_in_area_with_type(b->x - 1, b->y - 1, 5, TERRAIN_WATER)) {
                b->has_water_access = 2;
            } else {
                b->has_water_access = 0;
            }
        }
    }
    int total_reservoirs = building_list_large_size();
    const int *reservoirs = building_list_large_items();
    // fill reservoirs from full ones
    int changed = 1;
    static const int CONNECTOR_OFFSETS[] = { OFFSET(1,-1), OFFSET(3,1), OFFSET(1,3), OFFSET(-1,1) };
    while (changed == 1) {
        changed = 0;
        for (int i = 0; i < total_reservoirs; i++) {
            struct building_t *b = &all_buildings[reservoirs[i]];
            if (b->has_water_access == 2) {
                b->has_water_access = 1;
                changed = 1;
                for (int d = 0; d < 4; d++) {
                    int grid_offset2 = b->grid_offset + CONNECTOR_OFFSETS[d];
                    if (!map_terrain_is(grid_offset2, TERRAIN_AQUEDUCT)) {
                        return;
                    }
                    memset(&water_supply_queue, 0, sizeof(water_supply_queue));
                    int guard = 0;
                    int next_offset;
                    do {
                        if (++guard >= GRID_SIZE * GRID_SIZE) {
                            break;
                        }
                        map_aqueduct_set(grid_offset2, 1);
                        int image_id = map_image_at(grid_offset2);
                        if (image_id >= image_without_water) {
                            map_image_set(grid_offset2, image_id - 15);
                        }
                        next_offset = -1;
                        for (int j = 0; j < 4; j++) {
                            int new_offset = grid_offset2 + ADJACENT_OFFSETS[j];
                            struct building_t *bb = &all_buildings[map_building_at(new_offset)];
                            if (bb->id && bb->type == BUILDING_RESERVOIR) {
                                // check if aqueduct connects to reservoir --> doesn't connect to corner
                                int xy = map_property_multi_tile_xy(new_offset);
                                if (xy != EDGE_X0Y0 && xy != EDGE_X2Y0 && xy != EDGE_X0Y2 && xy != EDGE_X2Y2) {
                                    if (!bb->has_water_access) {
                                        bb->has_water_access = 2;
                                    }
                                }
                            } else if (map_terrain_is(new_offset, TERRAIN_AQUEDUCT)) {
                                if (!map_aqueduct_at(new_offset)) {
                                    if (next_offset == -1) {
                                        next_offset = new_offset;
                                    } else {
                                        water_supply_queue.items[water_supply_queue.tail++] = new_offset;
                                        if (water_supply_queue.tail >= MAX_QUEUE) {
                                            water_supply_queue.tail = 0;
                                        }
                                    }
                                }
                            }
                        }
                        if (next_offset == -1) {
                            if (water_supply_queue.head == water_supply_queue.tail) {
                                return;
                            }
                            next_offset = water_supply_queue.items[water_supply_queue.head++];
                            if (water_supply_queue.head >= MAX_QUEUE) {
                                water_supply_queue.head = 0;
                            }
                        }
                        grid_offset2 = next_offset;
                    } while (next_offset > -1);

                }
            }
        }
    }
    // mark reservoir ranges
    for (int i = 0; i < total_reservoirs; i++) {
        struct building_t *b = &all_buildings[reservoirs[i]];
        if (b->has_water_access) {
            map_terrain_add_with_radius(b->x, b->y, 3, 10, TERRAIN_RESERVOIR_RANGE);
        }
    }
    // fountains
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_FOUNTAIN) {
            continue;
        }
        int des = map_desirability_get(b->grid_offset);
        int image_id;
        if (des > 60) {
            image_id = image_group(GROUP_BUILDING_FOUNTAIN_4);
        } else if (des > 40) {
            image_id = image_group(GROUP_BUILDING_FOUNTAIN_3);
        } else if (des > 20) {
            image_id = image_group(GROUP_BUILDING_FOUNTAIN_2);
        } else {
            image_id = image_group(GROUP_BUILDING_FOUNTAIN_1);
        }
        map_building_tiles_add(i, b->x, b->y, 1, image_id, TERRAIN_BUILDING);
        if (map_terrain_is(b->grid_offset, TERRAIN_RESERVOIR_RANGE) && b->num_workers) {
            b->has_water_access = 1;
            map_terrain_add_with_radius(b->x, b->y, 1,
                scenario.climate == CLIMATE_DESERT ? 3 : 4,
                TERRAIN_FOUNTAIN_RANGE);
        } else {
            b->has_water_access = 0;
        }
    }
    // wells (to show range in water overlay)
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_WELL) {
            continue;
        }
        map_terrain_add_with_radius(b->x, b->y, 1, 2, TERRAIN_FOUNTAIN_RANGE);
    }
}

void map_water_add_building(int building_id, int x, int y, int size, int image_id)
{
    if (!map_grid_is_inside(x, y, size)) {
        return;
    }
    struct map_point_t leftmost;
    switch (city_view_orientation()) {
        case DIR_0_TOP:
            leftmost.x = 0;
            leftmost.y = size - 1;
            break;
        case DIR_2_RIGHT:
            leftmost.x = leftmost.y = 0;
            break;
        case DIR_4_BOTTOM:
            leftmost.x = size - 1;
            leftmost.y = 0;
            break;
        case DIR_6_LEFT:
            leftmost.x = leftmost.y = size - 1;
            break;
        default:
            return;
    }
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) {
            int grid_offset = map_grid_offset(x + dx, y + dy);
            terrain_grid.items[grid_offset] |= TERRAIN_BUILDING;
            if (!map_terrain_is(grid_offset, TERRAIN_WATER)) {
                terrain_grid.items[grid_offset] &= ~TERRAIN_CLEARABLE;
                terrain_grid.items[grid_offset] |= TERRAIN_BUILDING;
            }
            map_building_set(grid_offset, building_id);
            map_property_clear_constructing(grid_offset);
            map_property_set_multi_tile_size(grid_offset, size);
            map_image_set(grid_offset, image_id);
            map_property_set_multi_tile_xy(grid_offset, dx, dy,
                dx == leftmost.x && dy == leftmost.y);
        }
    }
}
