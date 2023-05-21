#include "figure/figure.h"

#include "building/building.h"
#include "city/data_private.h"
#include "city/emperor.h"
#include "core/image.h"
#include "core/image_group.h"
#include "core/random.h"
#include "empire/object.h"
#include "figure/formation.h"
#include "figure/name.h"
#include "figure/route.h"
#include "figure/trader.h"
#include "map/figure.h"
#include "map/grid.h"

#include <string.h>

struct figure_t figures[MAX_FIGURES];

struct figure_properties_t figure_properties[FIGURE_TYPE_MAX] = {
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  0,   0,   0},  // FIGURE_NONE = 0,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  0,   0},  // FIGURE_IMMIGRANT = 1,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  0,   0},  // FIGURE_EMIGRANT = 2,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  0,   0},  // FIGURE_HOMELESS = 3,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  10,  128, 0},  // FIGURE_PATRICIAN = 4,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  0,   0},  // FIGURE_CART_PUSHER = 5,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  384, 0},  // FIGURE_LABOR_SEEKER = 6,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  384, 0},  // FIGURE_BARBER = 7,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  384, 0},  // FIGURE_BATHHOUSE_WORKER = 8,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  384, 0},  // FIGURE_DOCTOR = 9,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  384, 0},  // FIGURE_SURGEON = 10,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  384, 0},  // FIGURE_PRIEST = 11,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  10,  96,  0},  // FIGURE_SCHOOL_CHILD = 12,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  384, 0},  // FIGURE_TEACHER = 13,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20,  384, 0},  // FIGURE_LIBRARIAN = 14,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20, 192,  0},  // FIGURE_MISSIONARY = 15,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  2, 512,   0},  // FIGURE_ACTOR = 16,
{0, 1, 0, 0, 0, 0, 0, 0, 0, 9,  2, 0,   0, 0,   0,              0,  100, 512, 0},  // FIGURE_GLADIATOR = 17,
{0, 1, 0, 0, 0, 0, 0, 0, 0, 15, 0, 0,   0, 0,   0,              0,  100, 512, 0},  // FIGURE_LION_TAMER = 18,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20, 512,  0},  // FIGURE_CHARIOTEER = 19,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  0, 0,     0},  // FIGURE_HIPPODROME_HORSES = 20,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20, 512,  0},  // FIGURE_TAX_COLLECTOR = 21,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20, 640,  0},  // FIGURE_ENGINEER = 22,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  0, 0,     1},  // FIGURE_FISHING_BOAT = 23,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  0, 0,     0},  // FIGURE_FISH_GULLS = 24,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  0, 0,     1},  // FIGURE_SHIPWRECK = 25,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20, 0,    0},  // FIGURE_DOCKER = 26,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  0, 0,     2},  // FIGURE_FLOTSAM = 27,
{0, 1, 0, 0, 0, 0, 0, 0, 0, 0,  0, 8,   0, 200, FIGURE_BOLT,    15, 0, 0,     0},  // FIGURE_BALLISTA = 28,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 200, 0, 0,   0,              0,  0, 0,     0},  // FIGURE_BOLT = 29,
{0, 1, 0, 0, 0, 0, 0, 0, 0, 6,  0, 6,   0, 50,  FIGURE_JAVELIN, 12, 50, 0,    0},  // FIGURE_TOWER_SENTRY = 30,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 15,  0, 0,   0,              0,  0, 0,     0},  // FIGURE_JAVELIN = 31,
{0, 1, 0, 0, 0, 0, 0, 0, 0, 5,  0, 0,   0, 0,   0,              0,  50, 640,  0},  // FIGURE_PREFECT = 32,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  0, 0,     0},  // FIGURE_FORT_STANDARD = 33,
{0, 0, 1, 0, 0, 0, 0, 0, 0, 4,  0, 4,   0, 100, FIGURE_JAVELIN, 10, 70, 0,    0},  // FIGURE_FORT_JAVELIN = 34,
{0, 0, 1, 0, 0, 0, 0, 0, 0, 6,  0, 0,   0, 0,   0,              0,  110, 0,   0},  // FIGURE_FORT_MOUNTED = 35,
{0, 0, 1, 0, 0, 0, 0, 0, 0, 10, 3, 0,   6, 0,   0,              0,  150, 0,   0},  // FIGURE_FORT_LEGIONARY = 36,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20, 800,  0},  // FIGURE_MARKET_BUYER = 37,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20, 384,  0},  // FIGURE_MARKET_TRADER = 38,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20, 0,    0},  // FIGURE_DELIVERY_BOY = 39,
{1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20, 0,    0},  // FIGURE_WAREHOUSEMAN = 40,
{0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  30, 0,    0},  // FIGURE_PROTESTER = 41,
{0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  30, 0,    0},  // FIGURE_CRIMINAL = 42,
{0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  30, 480,  0},  // FIGURE_RIOTER = 43,
{0, 0, 0, 0, 1, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  10, 0,    0},  // FIGURE_TRADE_CARAVAN = 44,
{0, 0, 0, 0, 1, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  10, 0,    0},  // FIGURE_TRADE_CARAVAN_DONKEY = 45,
{0, 0, 0, 0, 1, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  0, 0,     1},  // FIGURE_TRADE_SHIP = 46,
{0, 0, 0, 0, 0, 1, 0, 0, 0, 6,  0, 0,   0, 0,   0,              0,  40, 800,  0},  // FIGURE_INDIGENOUS_NATIVE = 47,
{0, 0, 0, 0, 0, 1, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  40, 0,    0},  // FIGURE_NATIVE_TRADER = 48,
{0, 0, 0, 0, 0, 0, 1, 0, 0, 8,  0, 0,   0, 0,   0,              0,  80, 0,    0},  // FIGURE_WOLF = 49,
{0, 0, 0, 0, 0, 0, 1, 0, 0, 0,  0, 0,   0, 0,   0,              0,  10, 0,    0},  // FIGURE_SHEEP = 50,
{0, 0, 0, 0, 0, 0, 1, 0, 0, 0,  0, 0,   0, 0,   0,              0,  20, 0,    0},  // FIGURE_ZEBRA = 51,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 9,  2, 0,   0, 0,   0,              0,  100, 0,   0},  // FIGURE_ENEMY_GLADIATOR = 52,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 7,  1, 0,   0, 0,   0,              0,  90, 0,    0},  // FIGURE_ENEMY_BARBARIAN_SWORDSMAN = 53,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 12, 2, 0,   2, 0,   0,              0,  120, 0,   0},  // FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN = 54,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 20, 5, 0,   8, 70,  FIGURE_ARROW,   15, 200, 0,   0},  // FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT = 55,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 10, 1, 0,   2, 0,   0,              0,  110, 0,   0},  // FIGURE_ENEMY_BRITON_SWORDSMAN = 56,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 12, 4, 0,   4, 0,   0,              0,  120, 0,   0},  // FIGURE_ENEMY_BRITON_CHARIOT = 57,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 10, 1, 0,   2, 0,   0,              0,  110, 0,   0},  // FIGURE_ENEMY_CELT_SWORDSMAN = 58,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 12, 4, 0,   4, 0,   0,              0,  120, 0,   0},  // FIGURE_ENEMY_CELT_CHARIOT = 59,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 10, 1, 0,   2, 0,   0,              0,  110, 0,   0},  // FIGURE_ENEMY_PICT_SWORDSMAN = 60,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 12, 4, 0,   4, 0,   0,              0,  120, 0,   0},  // FIGURE_ENEMY_PICT_CHARIOT = 61,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 7,  0, 0,   0, 0,   0,              0,  90, 0,    0},  // FIGURE_ENEMY_EGYPTIAN_SWORDSMAN = 62,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 7,  1, 0,   0, 70,  FIGURE_ARROW,   15, 120, 0,   0},  // FIGURE_ENEMY_EGYPTIAN_CAMEL = 63,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 12, 2, 0,   2, 0,   0,              0,  120, 0,   0},  // FIGURE_ENEMY_ETRUSCAN_SWORDSMAN = 64,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 5,  0, 0,   0, 70,  FIGURE_JAVELIN, 10, 70, 0,    0},  // FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER = 65,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 12, 2, 0,   2, 0,   0,              0,  120, 0,   0},  // FIGURE_ENEMY_SAMNITE_SWORDSMAN = 66,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 5,  0, 0,   0, 70,  FIGURE_JAVELIN, 10, 70, 0,    0},  // FIGURE_ENEMY_SAMNITE_SPEAR_THROWER = 67,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 10, 1, 0,   2, 0,   0,              0,  110, 0,   0},  // FIGURE_ENEMY_GAUL_SWORDSMAN = 68,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 15, 2, 0,   3, 0,   0,              0,  120, 0,   0},  // FIGURE_ENEMY_GAUL_AXEMAN = 69,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 10, 1, 0,   2, 0,   0,              0,  110, 0,   0},  // FIGURE_ENEMY_HELVETIUS_SWORDSMAN = 70,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 15, 2, 0,   3, 0,   0,              0,  120, 0,   0},  // FIGURE_ENEMY_HELVETIUS_AXEMAN = 71,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 7,  1, 0,   0, 0,   0,              0,  90, 0,    0},  // FIGURE_ENEMY_HUN_SWORDSMAN = 72,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 6,  1, 0,   0, 70,  FIGURE_ARROW,   15, 100, 0,   0},  // FIGURE_ENEMY_HUN_MOUNTED_ARCHER = 73,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 7,  1, 0,   0, 0,   0,              0,  90, 0,    0},  // FIGURE_ENEMY_GOTH_SWORDSMAN = 74,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 6,  1, 0,   0, 70,  FIGURE_ARROW,   15, 100, 0,   0},  // FIGURE_ENEMY_GOTH_MOUNTED_ARCHER = 75,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 7,  1, 0,   0, 0,   0,              0,  90, 0,    0},  // FIGURE_ENEMY_VISIGOTH_SWORDSMAN = 76,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 6,  1, 0,   0, 70,  FIGURE_ARROW,   15, 100, 0,   0},  // FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER = 77,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 12, 2, 0,   2, 0,   0,              0,  120, 0,   0},  // FIGURE_ENEMY_GREEK_SWORDSMAN = 78,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 5,  0, 0,   0, 70,  FIGURE_JAVELIN, 10, 70, 0,    0},  // FIGURE_ENEMY_GREEK_SPEAR_THROWER = 79,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 12, 2, 0,   2, 0,   0,              0,  120, 0,   0},  // FIGURE_ENEMY_MACEDONIAN_SWORDSMAN = 80,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 5,  0, 0,   0, 70,  FIGURE_JAVELIN, 10, 70, 0,    0},  // FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER = 81,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 7,  1, 0,   0, 0,   0,              0,  90, 0,    0},  // FIGURE_ENEMY_NUMIDIAN_SWORDSMAN = 82,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 5,  0, 0,   0, 100, FIGURE_JAVELIN, 10, 70, 0,    0},  // FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER = 83,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 7,  0, 0,   0, 0,   0,              0,  90, 0,    0},  // FIGURE_ENEMY_PERGAMUM_SWORDSMAN = 84,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 5,  0, 0,   0, 70,  FIGURE_ARROW,   15, 70, 0,    0},  // FIGURE_ENEMY_PERGAMUM_ARCHER = 85,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 7,  0, 0,   0, 0,   0,              0,  90, 0,    0},  // FIGURE_ENEMY_IBERIAN_SWORDSMAN = 86,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 5,  0, 0,   0, 70,  FIGURE_JAVELIN, 10, 70, 0,    0},  // FIGURE_ENEMY_IBERIAN_SPEAR_THROWER = 87,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 7,  0, 0,   0, 0,   0,              0,  90, 0,    0},  // FIGURE_ENEMY_JUDEAN_SWORDSMAN = 88,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 5,  0, 0,   0, 70,  FIGURE_JAVELIN, 10, 70, 0,    0},  // FIGURE_ENEMY_JUDEAN_SPEAR_THROWER = 89,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 7,  0, 0,   0, 0,   0,              0,  90, 0,    0},  // FIGURE_ENEMY_SELEUCID_SWORDSMAN = 90,
{0, 0, 0, 0, 0, 0, 0, 1, 0, 5,  0, 0,   0, 70,  FIGURE_JAVELIN, 10, 70, 0,    0},  // FIGURE_ENEMY_SELEUCID_SPEAR_THROWER = 91,
{0, 0, 0, 0, 0, 0, 0, 0, 1, 4,  0, 0,   0, 100, FIGURE_JAVELIN, 10, 90, 0,    0},  // FIGURE_ENEMY_CAESAR_JAVELIN = 92,
{0, 0, 0, 0, 0, 0, 0, 0, 1, 8,  0, 0,   0, 0,   0,              0,  100, 0,   0},  // FIGURE_ENEMY_CAESAR_MOUNTED = 93,
{0, 0, 0, 0, 0, 0, 0, 0, 1, 10, 3, 0,   6, 0,   0,              0,  150, 0,   0},  // FIGURE_ENEMY_CAESAR_LEGIONARY = 94,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 5,   0, 0,   0,              0,  0, 0,     0},  // FIGURE_ARROW = 95,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  0, 0,     0},  // FIGURE_MAP_FLAG = 96,
{0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0,   0, 0,   0,              0,  0, 0,     0},  // FIGURE_EXPLOSION = 97,
};

static const int CORPSE_IMAGE_OFFSETS[128] = {
    0, 1, 2, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
    7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7
};

struct figure_t *figure_create(int type, int x, int y, direction_type dir)
{
    int id = 0;
    for (int i = 1; i < MAX_FIGURES; i++) {
        if (!figures[i].state) {
            id = i;
            break;
        }
    }
    if (!id) {
        return &figures[0];
    }

    struct figure_t *f = &figures[id];
    f->state = FIGURE_STATE_ALIVE;
    f->type = type;
    f->use_cross_country = 0;
    f->speed_multiplier = 1;
    f->direction = dir;
    f->source_x = f->destination_x = f->previous_tile_x = f->x = x;
    f->source_y = f->destination_y = f->previous_tile_y = f->y = y;
    f->grid_offset = map_grid_offset(x, y);
    f->cross_country_x = 15 * x;
    f->cross_country_y = 15 * y;
    f->progress_on_tile = 15;
    f->phrase_sequence_city = f->phrase_sequence_exact = random_byte() & 3;
    f->name_id = get_figure_name_id(f);
    map_figure_add(f);
    if (type == FIGURE_TRADE_CARAVAN || type == FIGURE_TRADE_SHIP) {
        f->trader_id = trader_create();
    }

    return f;
}

void figure_delete(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    switch (f->type) {
        case FIGURE_LABOR_SEEKER:
        case FIGURE_MARKET_BUYER:
            if (f->building_id) {
                b->figure_id2 = 0;
            }
            break;
        case FIGURE_BALLISTA:
            b->figure_id4 = 0;
            break;
        case FIGURE_DOCKER:
            for (int i = 0; i < 3; i++) {
                if (b->data.dock.docker_ids[i] == f->id) {
                    b->data.dock.docker_ids[i] = 0;
                }
            }
            break;
        case FIGURE_ENEMY_CAESAR_LEGIONARY:
            city_data.emperor.invasion.soldiers_killed++;
            break;
        case FIGURE_EXPLOSION:
        case FIGURE_FORT_STANDARD:
        case FIGURE_ARROW:
        case FIGURE_JAVELIN:
        case FIGURE_BOLT:
        case FIGURE_FISH_GULLS:
        case FIGURE_SHEEP:
        case FIGURE_WOLF:
        case FIGURE_ZEBRA:
        case FIGURE_DELIVERY_BOY:
        case FIGURE_PATRICIAN:
            // nothing to do here
            break;
        default:
            if (f->building_id) {
                b->figure_id = 0;
            }
            break;
    }
    if (f->empire_city_id) {
        // remove trader
        for (int i = 0; i < 3; i++) {
            if (empire_objects[f->empire_city_id].trader_figure_ids[i] == f->id) {
                empire_objects[f->empire_city_id].trader_figure_ids[i] = 0;
            }
        }
    }
    if (f->immigrant_building_id) {
        b->immigrant_figure_id = 0;
    }
    figure_route_remove(f);
    map_figure_delete(f);

    int figure_id = f->id;
    memset(f, 0, sizeof(struct figure_t));
    f->id = figure_id;
}

int figure_is_dead(const struct figure_t *f)
{
    return f->state != FIGURE_STATE_ALIVE || f->action_state == FIGURE_ACTION_CORPSE;
}

void figure_handle_corpse(struct figure_t *f)
{
    if (f->wait_ticks < 0) {
        f->wait_ticks = 0;
    }
    f->wait_ticks++;
    if (f->wait_ticks >= 128) {
        f->wait_ticks = 127;
        f->state = FIGURE_STATE_DEAD;
        return;
    }
    switch (f->type) {
        case FIGURE_IMMIGRANT:
        case FIGURE_EMIGRANT:
        case FIGURE_CHARIOTEER: // corpse images missing for charioteer, assign migrant ones
        case FIGURE_TRADE_CARAVAN:
        case FIGURE_TRADE_CARAVAN_DONKEY:
            f->image_id = image_group(GROUP_FIGURE_MIGRANT) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;  // corpse images missing for trade caravan, assign migrant ones
            break;
        case FIGURE_HOMELESS:
            f->image_id = image_group(GROUP_FIGURE_HOMELESS) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_PATRICIAN:
            f->image_id = image_group(GROUP_FIGURE_PATRICIAN) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_CART_PUSHER:
        case FIGURE_DOCKER:
        case FIGURE_WAREHOUSEMAN:
        case FIGURE_NATIVE_TRADER:
            f->image_id = image_group(GROUP_FIGURE_CARTPUSHER) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            f->cart_image_id = 0;
            break;
        case FIGURE_LABOR_SEEKER:
            f->image_id = image_group(GROUP_FIGURE_LABOR_SEEKER) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_BARBER:
            f->image_id = image_group(GROUP_FIGURE_BARBER) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_BATHHOUSE_WORKER:
            f->image_id = image_group(GROUP_FIGURE_BATHHOUSE_WORKER) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_DOCTOR:
        case FIGURE_SURGEON:
            f->image_id = image_group(GROUP_FIGURE_DOCTOR_SURGEON) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_PRIEST:
            f->image_id = image_group(GROUP_FIGURE_PRIEST) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_SCHOOL_CHILD:
            f->image_id = image_group(GROUP_FIGURE_SCHOOL_CHILD) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_TEACHER:
        case FIGURE_LIBRARIAN:
            f->image_id = image_group(GROUP_FIGURE_TEACHER_LIBRARIAN) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_MISSIONARY:
            f->image_id = image_group(GROUP_FIGURE_MISSIONARY) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_ACTOR:
            f->image_id = image_group(GROUP_FIGURE_ACTOR) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_GLADIATOR:
        case FIGURE_ENEMY_GLADIATOR:
            f->image_id = image_group(GROUP_FIGURE_GLADIATOR) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_LION_TAMER:
            f->image_id = image_group(GROUP_FIGURE_LION_TAMER) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            f->cart_image_id = 0;
            break;
        case FIGURE_TAX_COLLECTOR:
            f->image_id = image_group(GROUP_FIGURE_TAX_COLLECTOR) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_ENGINEER:
            f->image_id = image_group(GROUP_FIGURE_ENGINEER) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_TOWER_SENTRY:
            f->image_id = image_group(GROUP_FIGURE_TOWER_SENTRY) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 136;
            break;
        case FIGURE_PREFECT:
            f->image_id = image_group(GROUP_FIGURE_PREFECT) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_FORT_JAVELIN:
            f->image_id = image_group(GROUP_BUILDING_FORT_JAVELIN) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 144;
            break;
        case FIGURE_FORT_MOUNTED:
            f->image_id = image_group(GROUP_FIGURE_FORT_MOUNTED) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 144;
            break;
        case FIGURE_FORT_LEGIONARY:
            f->image_id = image_group(GROUP_BUILDING_FORT_LEGIONARY) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 152;
            break;
        case FIGURE_MARKET_BUYER:
        case FIGURE_MARKET_TRADER:
            f->image_id = image_group(GROUP_FIGURE_MARKET_LADY) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_DELIVERY_BOY:
            f->image_id = image_group(GROUP_FIGURE_DELIVERY_BOY) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_PROTESTER:
        case FIGURE_CRIMINAL:
        case FIGURE_RIOTER:
            f->image_id = image_group(GROUP_FIGURE_CRIMINAL) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_INDIGENOUS_NATIVE:
        case FIGURE_ENEMY_BARBARIAN_SWORDSMAN:
            f->image_id = 441 + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2];
            break;
        case FIGURE_WOLF:
            f->image_id = image_group(GROUP_FIGURE_WOLF) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_SHEEP:
            f->image_id = image_group(GROUP_FIGURE_SHEEP) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 104;
            break;
        case FIGURE_ZEBRA:
            f->image_id = image_group(GROUP_FIGURE_ZEBRA) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 96;
            break;
        case FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN:
        case FIGURE_ENEMY_BRITON_SWORDSMAN:
        case FIGURE_ENEMY_CELT_SWORDSMAN:
        case FIGURE_ENEMY_PICT_SWORDSMAN:
        case FIGURE_ENEMY_EGYPTIAN_SWORDSMAN:
        case FIGURE_ENEMY_ETRUSCAN_SWORDSMAN:
        case FIGURE_ENEMY_SAMNITE_SWORDSMAN:
        case FIGURE_ENEMY_GAUL_SWORDSMAN:
        case FIGURE_ENEMY_HELVETIUS_SWORDSMAN:
        case FIGURE_ENEMY_HUN_SWORDSMAN:
        case FIGURE_ENEMY_GOTH_SWORDSMAN:
        case FIGURE_ENEMY_VISIGOTH_SWORDSMAN:
        case FIGURE_ENEMY_GREEK_SWORDSMAN:
        case FIGURE_ENEMY_MACEDONIAN_SWORDSMAN:
        case FIGURE_ENEMY_IBERIAN_SWORDSMAN:
        case FIGURE_ENEMY_PERGAMUM_SWORDSMAN:
        case FIGURE_ENEMY_JUDEAN_SWORDSMAN:
        case FIGURE_ENEMY_SELEUCID_SWORDSMAN:
            f->image_id = 593 + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2];
            break;
        case FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT:
            f->image_id = 705 + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2];
            break;
        case FIGURE_ENEMY_BRITON_CHARIOT:
        case FIGURE_ENEMY_CELT_CHARIOT:
        case FIGURE_ENEMY_PICT_CHARIOT:
        case FIGURE_ENEMY_EGYPTIAN_CAMEL:
        case FIGURE_ENEMY_GAUL_AXEMAN:
        case FIGURE_ENEMY_HELVETIUS_AXEMAN:
        case FIGURE_ENEMY_HUN_MOUNTED_ARCHER:
        case FIGURE_ENEMY_GOTH_MOUNTED_ARCHER:
        case FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER:
            f->image_id = 745 + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2];
            break;
        case FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER:
        case FIGURE_ENEMY_SAMNITE_SPEAR_THROWER:
        case FIGURE_ENEMY_GREEK_SPEAR_THROWER:
        case FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER:
        case FIGURE_ENEMY_PERGAMUM_ARCHER:
        case FIGURE_ENEMY_IBERIAN_SPEAR_THROWER:
        case FIGURE_ENEMY_JUDEAN_SPEAR_THROWER:
        case FIGURE_ENEMY_SELEUCID_SPEAR_THROWER:
            f->image_id = 793 + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2];
            break;
        case FIGURE_ENEMY_NUMIDIAN_SWORDSMAN:
        case FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER:
            f->image_id = 641 + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2];
            break;
        case FIGURE_ENEMY_CAESAR_JAVELIN:
        case FIGURE_ENEMY_CAESAR_MOUNTED:
        case FIGURE_ENEMY_CAESAR_LEGIONARY:
            f->image_id = image_group(GROUP_FIGURE_CAESAR_LEGIONARY) + CORPSE_IMAGE_OFFSETS[f->wait_ticks / 2] + 152;
            break;
    }
}

int city_figures_total_invading_enemies(void)
{
    return city_data.figure.imperial_soldiers + city_data.figure.enemies;
}

void figure_init_scenario(void)
{
    for (int i = 0; i < MAX_FIGURES; i++) {
        memset(&figures[i], 0, sizeof(struct figure_t));
        figures[i].id = i;
    }
}

static void figure_save(buffer *buf, const struct figure_t *f)
{
    buffer_write_u8(buf, f->is_targetable);
    buffer_write_u8(buf, f->type);
    buffer_write_u8(buf, f->state);
    buffer_write_u8(buf, f->action_state);
    buffer_write_u8(buf, f->action_state_before_attack);
    buffer_write_u8(buf, f->formation_id);
    buffer_write_u8(buf, f->index_in_formation);
    buffer_write_u8(buf, f->damage);
    buffer_write_u8(buf, f->is_military_trained);
    buffer_write_u8(buf, f->mounted_charge_ticks);
    buffer_write_u8(buf, f->mounted_charge_ticks_max);
    buffer_write_u16(buf, f->target_figure_id);
    for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
        buffer_write_u16(buf, f->melee_targeter_ids[i]);
    }
    for (int i = 0; i < MAX_MELEE_COMBATANTS_PER_UNIT; i++) {
        buffer_write_u16(buf, f->melee_combatant_ids[i]);
    }
    buffer_write_u8(buf, f->num_melee_combatants);
    for (int i = 0; i < MAX_RANGED_TARGETERS_PER_UNIT; i++) {
        buffer_write_u16(buf, f->ranged_targeter_ids[i]);
    }
    buffer_write_u8(buf, f->prefect_recent_guard_duty);
    buffer_write_i8(buf, f->attack_direction);
    buffer_write_u8(buf, f->source_x);
    buffer_write_u8(buf, f->source_y);
    buffer_write_u16(buf, f->routing_path_id);
    buffer_write_u16(buf, f->routing_path_current_tile);
    buffer_write_u16(buf, f->routing_path_length);
    buffer_write_u8(buf, f->terrain_usage);
    buffer_write_u8(buf, f->speed_multiplier);
    buffer_write_i8(buf, f->previous_tile_direction);
    buffer_write_u8(buf, f->previous_tile_x);
    buffer_write_u8(buf, f->previous_tile_y);
    buffer_write_i8(buf, f->direction);
    buffer_write_u8(buf, f->progress_on_tile);
    buffer_write_u8(buf, f->x);
    buffer_write_u8(buf, f->y);
    buffer_write_u16(buf, f->grid_offset);
    buffer_write_u8(buf, f->destination_x);
    buffer_write_u8(buf, f->destination_y);
    buffer_write_u16(buf, f->destination_grid_offset);
    buffer_write_u16(buf, f->destination_building_id);
    buffer_write_u8(buf, f->figure_is_halted);
    buffer_write_u8(buf, f->use_cross_country);
    buffer_write_u8(buf, f->cc_direction);
    buffer_write_u16(buf, f->cross_country_x);
    buffer_write_u16(buf, f->cross_country_y);
    buffer_write_i16(buf, f->cc_delta_x);
    buffer_write_i16(buf, f->cc_delta_y);
    buffer_write_i16(buf, f->cc_delta_xy);
    buffer_write_u16(buf, f->cc_destination_x);
    buffer_write_u16(buf, f->cc_destination_y);
    buffer_write_u8(buf, f->missile_offset);
    buffer_write_u16(buf, f->roam_length);
    buffer_write_u8(buf, f->roam_choose_destination);
    buffer_write_u8(buf, f->roam_random_counter);
    buffer_write_i8(buf, f->roam_turn_direction);
    buffer_write_i8(buf, f->roam_ticks_until_next_turn);
    buffer_write_u8(buf, f->in_building_wait_ticks);
    buffer_write_u8(buf, f->height_adjusted_ticks);
    buffer_write_u8(buf, f->current_height);
    buffer_write_u8(buf, f->target_height);
    buffer_write_u16(buf, f->next_figure_id_on_same_tile);
    buffer_write_u16(buf, f->image_id);
    buffer_write_u8(buf, f->image_offset);
    buffer_write_u8(buf, f->attack_image_offset);
    buffer_write_u8(buf, f->is_shooting);
    buffer_write_u16(buf, f->cart_image_id);
    buffer_write_i8(buf, f->x_offset_cart);
    buffer_write_i8(buf, f->y_offset_cart);
    buffer_write_u8(buf, f->enemy_image_group);
    buffer_write_i16(buf, f->wait_ticks);
    buffer_write_u8(buf, f->wait_ticks_missile);
    buffer_write_u16(buf, f->name_id);
    buffer_write_u8(buf, f->is_ghost);
    buffer_write_u16(buf, f->building_id);
    buffer_write_u16(buf, f->immigrant_building_id);
    buffer_write_u8(buf, f->migrant_num_people);
    buffer_write_u8(buf, f->min_max_seen);
    buffer_write_u8(buf, f->phrase_sequence_exact);
    buffer_write_i8(buf, f->phrase_id);
    buffer_write_u8(buf, f->phrase_sequence_city);
    buffer_write_u8(buf, f->empire_city_id);
    buffer_write_u8(buf, f->resource_id);
    buffer_write_u8(buf, f->collecting_item_id);
    buffer_write_u8(buf, f->trader_id);
    buffer_write_u16(buf, f->leading_figure_id);
    buffer_write_u8(buf, f->trader_amount_bought);
    buffer_write_u8(buf, f->loads_sold_or_carrying);
    buffer_write_u8(buf, f->trade_ship_failed_dock_attempts);
    buffer_write_u8(buf, f->flotsam_visible);
}

static void figure_load(buffer *buf, struct figure_t *f)
{
    f->is_targetable = buffer_read_u8(buf);
    f->type = buffer_read_u8(buf);
    f->state = buffer_read_u8(buf);
    f->action_state = buffer_read_u8(buf);
    f->action_state_before_attack = buffer_read_u8(buf);
    f->formation_id = buffer_read_u8(buf);
    f->index_in_formation = buffer_read_u8(buf);
    f->damage = buffer_read_u8(buf);
    f->is_military_trained = buffer_read_u8(buf);
    f->mounted_charge_ticks = buffer_read_u8(buf);
    f->mounted_charge_ticks_max = buffer_read_u8(buf);
    f->target_figure_id = buffer_read_u16(buf);
    for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
        f->melee_targeter_ids[i] = buffer_read_u16(buf);
    }
    for (int i = 0; i < MAX_MELEE_COMBATANTS_PER_UNIT; i++) {
        f->melee_combatant_ids[i] = buffer_read_u16(buf);
    }
    f->num_melee_combatants = buffer_read_u8(buf);
    for (int i = 0; i < MAX_RANGED_TARGETERS_PER_UNIT; i++) {
        f->ranged_targeter_ids[i] = buffer_read_u16(buf);
    }
    f->prefect_recent_guard_duty = buffer_read_u8(buf);
    f->attack_direction = buffer_read_i8(buf);
    f->source_x = buffer_read_u8(buf);
    f->source_y = buffer_read_u8(buf);
    f->routing_path_id = buffer_read_u16(buf);
    f->routing_path_current_tile = buffer_read_u16(buf);
    f->routing_path_length = buffer_read_u16(buf);
    f->terrain_usage = buffer_read_u8(buf);
    f->speed_multiplier = buffer_read_u8(buf);
    f->previous_tile_direction = buffer_read_i8(buf);
    f->previous_tile_x = buffer_read_u8(buf);
    f->previous_tile_y = buffer_read_u8(buf);
    f->direction = buffer_read_i8(buf);
    f->progress_on_tile = buffer_read_u8(buf);
    f->x = buffer_read_u8(buf);
    f->y = buffer_read_u8(buf);
    f->grid_offset = buffer_read_u16(buf);
    f->destination_x = buffer_read_u8(buf);
    f->destination_y = buffer_read_u8(buf);
    f->destination_grid_offset = buffer_read_u16(buf);
    f->destination_building_id = buffer_read_u16(buf);
    f->figure_is_halted = buffer_read_u8(buf);
    f->use_cross_country = buffer_read_u8(buf);
    f->cc_direction = buffer_read_u8(buf);
    f->cross_country_x = buffer_read_u16(buf);
    f->cross_country_y = buffer_read_u16(buf);
    f->cc_delta_x = buffer_read_i16(buf);
    f->cc_delta_y = buffer_read_i16(buf);
    f->cc_delta_xy = buffer_read_i16(buf);
    f->cc_destination_x = buffer_read_u16(buf);
    f->cc_destination_y = buffer_read_u16(buf);
    f->missile_offset = buffer_read_u8(buf);
    f->roam_length = buffer_read_u16(buf);
    f->roam_choose_destination = buffer_read_u8(buf);
    f->roam_random_counter = buffer_read_u8(buf);
    f->roam_turn_direction = buffer_read_i8(buf);
    f->roam_ticks_until_next_turn = buffer_read_i8(buf);
    f->in_building_wait_ticks = buffer_read_u8(buf);
    f->height_adjusted_ticks = buffer_read_u8(buf);
    f->current_height = buffer_read_u8(buf);
    f->target_height = buffer_read_u8(buf);
    f->next_figure_id_on_same_tile = buffer_read_u16(buf);
    f->image_id = buffer_read_u16(buf);
    f->image_offset = buffer_read_u8(buf);
    f->attack_image_offset = buffer_read_u8(buf);
    f->is_shooting = buffer_read_u8(buf);
    f->cart_image_id = buffer_read_u16(buf);
    f->x_offset_cart = buffer_read_i8(buf);
    f->y_offset_cart = buffer_read_i8(buf);
    f->enemy_image_group = buffer_read_u8(buf);
    f->wait_ticks = buffer_read_i16(buf);
    f->wait_ticks_missile = buffer_read_u8(buf);
    f->name_id = buffer_read_u16(buf);
    f->is_ghost = buffer_read_u8(buf);
    f->building_id = buffer_read_u16(buf);
    f->immigrant_building_id = buffer_read_u16(buf);
    f->migrant_num_people = buffer_read_u8(buf);
    f->min_max_seen = buffer_read_u8(buf);
    f->phrase_sequence_exact = buffer_read_u8(buf);
    f->phrase_id = buffer_read_i8(buf);
    f->phrase_sequence_city = buffer_read_u8(buf);
    f->empire_city_id = buffer_read_u8(buf);
    f->resource_id = buffer_read_u8(buf);
    f->collecting_item_id = buffer_read_u8(buf);
    f->trader_id = buffer_read_u8(buf);
    f->leading_figure_id = buffer_read_u16(buf);
    f->trader_amount_bought = buffer_read_u8(buf);
    f->loads_sold_or_carrying = buffer_read_u8(buf);
    f->trade_ship_failed_dock_attempts = buffer_read_u8(buf);
    f->flotsam_visible = buffer_read_u8(buf);
}

void figure_save_state(buffer *list)
{
    for (int i = 0; i < MAX_FIGURES; i++) {
        figure_save(list, &figures[i]);
    }
}

void figure_load_state(buffer *list)
{
    for (int i = 0; i < MAX_FIGURES; i++) {
        figure_load(list, &figures[i]);
        figures[i].id = i;
    }
}
