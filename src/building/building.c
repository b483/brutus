#include "building.h"

#include "building/building_state.h"
#include "building/destruction.h"
#include "building/storage.h"
#include "city/buildings.h"
#include "city/data_private.h"
#include "city/population.h"
#include "city/warning.h"
#include "core/calc.h"
#include "figure/figure.h"
#include "figure/formation_legion.h"
#include "figure/route.h"
#include "game/resource.h"
#include "game/undo.h"
#include "map/building_tiles.h"
#include "map/desirability.h"
#include "map/elevation.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/random.h"
#include "map/road_access.h"
#include "map/routing_terrain.h"
#include "map/terrain.h"
#include "map/tiles.h"

#include <string.h>

static building all_buildings[MAX_BUILDINGS];

static struct {
    int highest_id_in_use;
    int highest_id_ever;
    int created_sequence;
    int incorrect_houses;
    int unfixable_houses;
} extra = { 0, 0, 0, 0, 0 };

struct building_properties_t building_properties[BUILDING_TYPE_MAX] = {
    // size | fireprf | img_grp | img_off | cost | des_value | des_step | des_step_siz | des_range | labrs
    {  0,     0,        0,        0,        0,     0,          0,         0,             0,          0}, // BUILDING_NONE
    {  1,     0,        0,        0,        10,    0,          0,         0,             0,          0}, // BUILDING_HOUSE_VACANT_LOT
    {  1,     0,        0,        0,        0,    -3,          1,         1,             3,          0}, // BUILDING_HOUSE_SMALL_TENT
    {  1,     0,        0,        0,        0,    -3,          1,         1,             3,          0}, // BUILDING_HOUSE_LARGE_TENT
    {  1,     0,        0,        0,        0,    -2,          1,         1,             2,          0}, // BUILDING_HOUSE_SMALL_SHACK
    {  1,     0,        0,        0,        0,    -2,          1,         1,             2,          0}, // BUILDING_HOUSE_LARGE_SHACK
    {  1,     0,        0,        0,        0,    -2,          1,         1,             2,          0}, // BUILDING_HOUSE_SMALL_HOVEL
    {  1,     0,        0,        0,        0,    -2,          1,         1,             2,          0}, // BUILDING_HOUSE_LARGE_HOVEL
    {  1,     0,        0,        0,        0,    -1,          1,         1,             1,          0}, // BUILDING_HOUSE_SMALL_CASA
    {  1,     0,        0,        0,        0,    -1,          1,         1,             1,          0}, // BUILDING_HOUSE_LARGE_CASA
    {  1,     0,        0,        0,        0,     0,          1,         1,             1,          0}, // BUILDING_HOUSE_SMALL_INSULA
    {  1,     0,        0,        0,        0,     0,          1,         1,             1,          0}, // BUILDING_HOUSE_MEDIUM_INSULA
    {  2,     0,        0,        0,        0,     0,          0,         0,             0,          0}, // BUILDING_HOUSE_LARGE_INSULA
    {  2,     0,        0,        0,        0,     0,          0,         0,             0,          0}, // BUILDING_HOUSE_GRAND_INSULA
    {  2,     0,        0,        0,        0,     1,          2,        -1,             2,          0}, // BUILDING_HOUSE_SMALL_VILLA
    {  2,     0,        0,        0,        0,     1,          2,        -1,             2,          0}, // BUILDING_HOUSE_MEDIUM_VILLA
    {  3,     0,        0,        0,        0,     2,          2,        -2,             2,          0}, // BUILDING_HOUSE_LARGE_VILLA
    {  3,     0,        0,        0,        0,     2,          2,        -2,             2,          0}, // BUILDING_HOUSE_GRAND_VILLA
    {  3,     0,        0,        0,        0,     3,          2,        -1,             6,          0}, // BUILDING_HOUSE_SMALL_PALACE
    {  3,     0,        0,        0,        0,     3,          2,        -1,             6,          0}, // BUILDING_HOUSE_MEDIUM_PALACE
    {  4,     0,        0,        0,        0,     4,          2,        -1,             6,          0}, // BUILDING_HOUSE_LARGE_PALACE
    {  4,     0,        0,        0,        0,     4,          2,        -1,             6,          0}, // BUILDING_HOUSE_LUXURY_PALACE
    {  0,     0,        0,        0,        2,     0,          0,         0,             0,          0}, // BUILDING_CLEAR_LAND
    {  1,     0,        112,      0,        4,     0,          0,         0,             0,          0}, // BUILDING_ROAD
    {  3,     1,        25,       0,        80,   -6,          1,         2,             3,          0}, // BUILDING_RESERVOIR
    {  1,     0,        19,       0,        8,    -2,          1,         1,             2,          0}, // BUILDING_AQUEDUCT
    {  1,     1,        54,       0,        15,    0,          0,         0,             0,          4}, // BUILDING_FOUNTAIN
    {  1,     1,        23,       0,        5,     0,          0,         0,             0,          0}, // BUILDING_WELL
    {  1,     0,        67,       0,        25,    2,          1,        -1,             2,          2}, // BUILDING_BARBER
    {  2,     0,        185,      0,        50,    4,          1,        -1,             4,          10}, // BUILDING_BATHHOUSE
    {  1,     0,        68,       0,        30,    0,          0,         0,             0,          5}, // BUILDING_DOCTOR
    {  3,     0,        70,       0,        300,  -1,          2,         1,             2,          30}, // BUILDING_HOSPITAL
    {  0,     0,        0,        0,        0,     0,          0,         0,             0,          0}, // BUILDING_MENU_SMALL_TEMPLES
    {  2,     0,        71,       0,        50,    4,          2,        -1,             6,          2}, // BUILDING_SMALL_TEMPLE_CERES
    {  2,     0,        71,       0,        50,    4,          2,        -1,             6,          2}, // BUILDING_SMALL_TEMPLE_NEPTUNE
    {  2,     0,        71,       0,        50,    4,          2,        -1,             6,          2}, // BUILDING_SMALL_TEMPLE_MERCURY
    {  2,     0,        71,       0,        50,    4,          2,        -1,             6,          2}, // BUILDING_SMALL_TEMPLE_MARS
    {  2,     0,        71,       0,        50,    4,          2,        -1,             6,          2}, // BUILDING_SMALL_TEMPLE_VENUS
    {  0,     0,        0,        0,        0,     0,          0,         0,             0,          0}, // BUILDING_MENU_LARGE_TEMPLES
    {  3,     0,        71,       1,        150,   8,          2,        -1,             8,          5}, // BUILDING_LARGE_TEMPLE_CERES
    {  3,     0,        71,       1,        150,   8,          2,        -1,             8,          5}, // BUILDING_LARGE_TEMPLE_NEPTUNE
    {  3,     0,        71,       1,        150,   8,          2,        -1,             8,          5}, // BUILDING_LARGE_TEMPLE_MERCURY
    {  3,     0,        71,       1,        150,   8,          2,        -1,             8,          5}, // BUILDING_LARGE_TEMPLE_MARS
    {  3,     0,        71,       1,        150,   8,          2,        -1,             8,          5}, // BUILDING_LARGE_TEMPLE_VENUS
    {  2,     0,        76,       0,        200,   8,          2,        -1,             6,          0}, // BUILDING_ORACLE
    {  2,     0,        41,       0,        50,   -2,          1,         1,             2,          10}, // BUILDING_SCHOOL
    {  3,     0,        43,       0,        100,   4,          1,         1,             4,          30}, // BUILDING_ACADEMY
    {  2,     0,        42,       0,        75,    4,          1,        -1,             4,          20}, // BUILDING_LIBRARY
    {  2,     1,        184,      0,        100,  -3,          1,         1,             2,          20}, // BUILDING_MISSION_POST
    {  2,     0,        46,       0,        50,    2,          1,        -1,             2,          8}, // BUILDING_THEATER
    {  3,     0,        45,       0,        100,   4,          1,        -1,             4,          12}, // BUILDING_AMPHITHEATER
    {  5,     0,        48,       0,        500,  -3,          1,         1,             3,          25}, // BUILDING_COLOSSEUM
    {  5,     0,        213,      0,        2500, -3,          1,         1,             3,          150}, // BUILDING_HIPPODROME
    {  3,     0,        49,       0,        75,   -3,          1,         1,             3,          8}, // BUILDING_GLADIATOR_SCHOOL
    {  3,     0,        50,       0,        75,   -3,          1,         1,             3,          8}, // BUILDING_LION_HOUSE
    {  3,     0,        51,       0,        50,    2,          1,        -1,             2,          5}, // BUILDING_ACTOR_COLONY
    {  3,     0,        52,       0,        75,   -3,          1,         1,             3,          8}, // BUILDING_CHARIOT_MAKER
    {  2,     0,        63,       0,        75,    3,          2,        -1,             2,          6}, // BUILDING_FORUM
    {  5,     0,        62,       0,        400,   14,         2,        -1,             8,          30}, // BUILDING_SENATE
    {  3,     0,        85,       0,        150,   12,         2,        -2,             3,          0}, // BUILDING_GOVERNORS_HOUSE
    {  4,     0,        86,       0,        400,   20,         2,        -3,             4,          0}, // BUILDING_GOVERNORS_VILLA
    {  5,     0,        87,       0,        700,   28,         2,        -4,             5,          0}, // BUILDING_GOVERNORS_PALACE
    {  1,     1,        61,       0,        12,    3,          1,        -1,             3,          0}, // BUILDING_SMALL_STATUE
    {  2,     1,        61,       1,        60,    10,         1,        -2,             4,          0}, // BUILDING_MEDIUM_STATUE
    {  3,     1,        61,       2,        150,   14,         2,        -2,             5,          0}, // BUILDING_LARGE_STATUE
    {  3,     1,        205,      0,        0,     18,         2,         3,             5,          0}, // BUILDING_TRIUMPHAL_ARCH
    {  1,     1,        59,       0,        12,    3,          1,        -1,             3,          0}, // BUILDING_GARDENS
    {  1,     1,        58,       0,        15,    4,          1,        -2,             2,          0}, // BUILDING_PLAZA
    {  1,     1,        81,       0,        30,    0,          1,         1,             1,          5}, // BUILDING_ENGINEERS_POST
    {  1,     1,        0,        0,        40,    0,          0,         0,             0,          0}, // BUILDING_LOW_BRIDGE
    {  1,     1,        0,        0,        100,   0,          0,         0,             0,          0}, // BUILDING_SHIP_BRIDGE
    {  2,     0,        77,       0,        100,  -8,          2,         2,             3,          10}, // BUILDING_SHIPYARD
    {  3,     0,        78,       0,        100,  -8,          2,         2,             3,          12}, // BUILDING_DOCK
    {  2,     0,        79,       0,        60,   -8,          2,         2,             3,          6}, // BUILDING_WHARF
    {  1,     0,        24,       26,       12,    0,          0,         0,             0,          0}, // BUILDING_WALL
    {  2,     1,        17,       0,        150,  -8,          1,         2,             3,          6}, // BUILDING_TOWER
    {  2,     1,        17,       1,        100,  -4,          1,         1,             3,          0}, // BUILDING_GATEHOUSE
    {  1,     0,        64,       0,        30,   -2,          1,         1,             2,          6}, // BUILDING_PREFECTURE
    {  3,     1,        66,       0,        250,  -20,         2,         2,             8,          0}, // BUILDING_FORT
    {  3,     1,        66,       0,        1000, -20,         2,         2,             8,          0}, // BUILDING_FORT_LEGIONARIES
    {  3,     1,        66,       0,        1000, -20,         2,         2,             8,          0}, // BUILDING_FORT_JAVELIN
    {  3,     1,        66,       0,        1000, -20,         2,         2,             8,          0}, // BUILDING_FORT_MOUNTED
    {  3,     0,        201,      0,        1000, -3,          1,         1,             3,          20}, // BUILDING_MILITARY_ACADEMY
    {  3,     0,        166,      0,        150,  -6,          1,         1,             3,          10}, // BUILDING_BARRACKS
    {  0,     0,        0,        0,        0,     0,          0,         0,             0,          0}, // BUILDING_MENU_FARMS
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10}, // BUILDING_WHEAT_FARM
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10}, // BUILDING_VEGETABLE_FARM
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10}, // BUILDING_FRUIT_FARM
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10}, // BUILDING_OLIVE_FARM
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10}, // BUILDING_VINES_FARM
    {  3,     0,        37,       0,        40,   -2,          1,         1,             2,          10}, // BUILDING_PIG_FARM
    {  0,     0,        0,        0,        0,     0,          0,         0,             0,          0}, // BUILDING_MENU_RAW_MATERIALS
    {  2,     0,        40,       0,        40,   -3,          1,         1,             3,          10}, // BUILDING_CLAY_PIT
    {  2,     0,        38,       0,        50,   -6,          1,         1,             4,          10}, // BUILDING_MARBLE_QUARRY
    {  2,     0,        39,       0,        50,   -6,          1,         1,             4,          10}, // BUILDING_IRON_MINE
    {  2,     0,        65,       0,        40,   -4,          1,         1,             3,          10}, // BUILDING_TIMBER_YARD
    {  0,     0,        0,        0,        0,     0,          0,         0,             0,          0}, // BUILDING_MENU_WORKSHOPS
    {  2,     0,        44,       0,        45,   -1,          1,         1,             1,          10}, // BUILDING_WINE_WORKSHOP
    {  2,     0,        122,      0,        50,   -4,          1,         1,             2,          10}, // BUILDING_OIL_WORKSHOP
    {  2,     0,        123,      0,        50,   -4,          1,         1,             2,          10}, // BUILDING_WEAPONS_WORKSHOP
    {  2,     0,        124,      0,        40,   -4,          1,         1,             2,          10}, // BUILDING_FURNITURE_WORKSHOP
    {  2,     0,        125,      0,        40,   -4,          1,         1,             2,          10}, // BUILDING_POTTERY_WORKSHOP
    {  2,     0,        22,       0,        40,   -2,          1,         1,             6,          5}, // BUILDING_MARKET
    {  3,     0,        99,       0,        100,  -4,          1,         2,             2,          6}, // BUILDING_GRANARY
    {  1,     1,        82,       0,        70,   -5,          2,         2,             3,          6}, // BUILDING_WAREHOUSE
    {  1,     1,        82,       0,        0,     0,          0,         0,             0,          0}, // BUILDING_WAREHOUSE_SPACE
    {  1,     1,        183,      0,        50,    0,          0,         0,             0,          0}, // BUILDING_NATIVE_HUT
    {  2,     1,        183,      2,        50,    0,          0,         0,             0,          0}, // BUILDING_NATIVE_MEETING
    {  1,     1,        100,      0,        0,     0,          0,         0,             0,          0}, // BUILDING_NATIVE_CROPS
    {  4,     1,        66,       1,        0,     0,          0,         0,             0,          0}, // BUILDING_FORT_GROUND
    {  1,     1,        0,        0,        0,    -1,          1,         1,             2,          0}, // BUILDING_BURNING_RUIN
};

struct house_properties_t house_properties[20] = {
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

building *building_get(int id)
{
    return &all_buildings[id];
}

building *building_main(building *b)
{
    for (int guard = 0; guard < 9; guard++) {
        if (b->prev_part_building_id <= 0) {
            return b;
        }
        b = &all_buildings[b->prev_part_building_id];
    }
    return &all_buildings[0];
}

building *building_next(building *b)
{
    return &all_buildings[b->next_part_building_id];
}

building *building_create(building_type type, int x, int y)
{
    building *b = 0;
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
    b->distance_from_entry = 0;

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

static void building_delete(building *b)
{
    building_clear_related_data(b);
    int id = b->id;
    memset(b, 0, sizeof(building));
    b->id = id;
}

void building_clear_related_data(building *b)
{
    if (b->storage_id) {
        building_storage_delete(b->storage_id);
        b->storage_id = 0;
    }
    if (b->type == BUILDING_SENATE) {
        city_buildings_remove_senate(b);
    }
    if (b->type == BUILDING_DOCK) {
        city_data.building.working_docks--;
    }
    if (b->type == BUILDING_BARRACKS) {
        city_buildings_remove_barracks(b);
    }
    if (b->type == BUILDING_FORT) {
        if (b->formation_id > 0) {
            if (formations[b->formation_id].in_use) {
                for (int i = 0; i < formations[b->formation_id].num_figures; i++) {
                    struct figure_t *f = &figures[formations[b->formation_id].figures[i]];
                    map_point nearest_barracks_road_tile = { 0 };
                    set_destination__closest_building_of_type(b->id, BUILDING_BARRACKS, &nearest_barracks_road_tile);
                    figure_route_remove(f);
                    if (nearest_barracks_road_tile.x) {
                        f->destination_x = nearest_barracks_road_tile.x;
                        f->destination_y = nearest_barracks_road_tile.y;
                    } else {
                        f->destination_x = city_data.map.exit_point.x;
                        f->destination_y = city_data.map.exit_point.y;
                    }
                    f->action_state = FIGURE_ACTION_SOLDIER_RETURNING_TO_BARRACKS;
                }
                map_figure_delete(&figures[formations[b->formation_id].legion_standard__figure_id]);
                formation_clear(b->formation_id);
                formation_calculate_legion_totals();
            }
        }
    }
    if (b->type == BUILDING_HIPPODROME) {
        city_data.building.hippodrome_placed = 0;
    }
}

void building_update_state(void)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = &all_buildings[i];
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
            building_destroy_by_plague(b);
        }
    }
}

void building_update_desirability(void)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        b->desirability = map_desirability_get_max(b->x, b->y, b->size);
    }
}

int building_is_house(building_type type)
{
    return type >= BUILDING_HOUSE_VACANT_LOT && type <= BUILDING_HOUSE_LUXURY_PALACE;
}

int building_is_fort(building_type type)
{
    return type == BUILDING_FORT_LEGIONARIES ||
        type == BUILDING_FORT_JAVELIN ||
        type == BUILDING_FORT_MOUNTED;
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

void set_destination__closest_building_of_type(int closest_to__building_id, int closest_building_of_type__type, map_point *closest_building_of_type__road_tile)
{
    int min_distance = 10000;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = building_get(i);
        if (b->state == BUILDING_STATE_IN_USE && b->type == closest_building_of_type__type && b->num_workers >= building_properties[closest_building_of_type__type].laborers) {
            building *closest_to__building = building_get(closest_to__building_id);
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
        memset(&all_buildings[i], 0, sizeof(building));
        all_buildings[i].id = i;
    }
    extra.highest_id_in_use = 0;
    extra.highest_id_ever = 0;
    extra.created_sequence = 0;
    extra.incorrect_houses = 0;
    extra.unfixable_houses = 0;
}

void building_save_state(buffer *buf, buffer *highest_id, buffer *highest_id_ever,
                         buffer *sequence, buffer *corrupt_houses)
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

void building_load_state(buffer *buf, buffer *highest_id, buffer *highest_id_ever,
                         buffer *sequence, buffer *corrupt_houses)
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
