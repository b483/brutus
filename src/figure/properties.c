#include "properties.h"

static const figure_properties properties[] = {
    //  cat  dmg  atk  def Mdef Matk Mfrq
        {0,   0,   0,   0,   0,   0,   0},  // FIGURE_NONE
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_IMMIGRANT
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_EMIGRANT
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_HOMELESS
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_CART_PUSHER
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_LABOR_SEEKER
        {0,   0,   0,   0,   0,   0,   0},  // FIGURE_EXPLOSION
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_TAX_COLLECTOR
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_ENGINEER
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_WAREHOUSEMAN
        {2,  50,   5,   0,   0,   0,   0},  // FIGURE_PREFECT
        {2,  80,   4,   0,   0,   4, 100},  // FIGURE_FORT_JAVELIN
        {2, 120,   8,   0,   0,   0,   0},  // FIGURE_FORT_MOUNTED
        {2, 150,  10,   0,   0,   0,   0},  // FIGURE_FORT_LEGIONARY
        {0,   0,   0,   0,   0,   0,   0},  // FIGURE_FORT_STANDARD
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_ACTOR
        {2, 100,   9,   2,   0,   0,   0},  // FIGURE_GLADIATOR
        {2, 100,  15,   0,   0,   0,   0},  // FIGURE_LION_TAMER
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_CHARIOTEER
        {1,  10,   0,   0,   0,   0,   0},  // FIGURE_TRADE_CARAVAN
        {0,   0,   0,   0,   0,   0,   0},  // FIGURE_TRADE_SHIP
        {1,  10,   0,   0,   0,   0,   0},  // FIGURE_TRADE_CARAVAN_DONKEY
        {4,  12,   0,   0,   0,   0,   0},  // FIGURE_PROTESTER
        {4,  12,   0,   0,   0,   0,   0},  // FIGURE_CRIMINAL
        {4,  12,   0,   0,   0,   0,   0},  // FIGURE_RIOTER
        {0,   0,   0,   0,   0,   0,   0},  // FIGURE_FISHING_BOAT
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_MARKET_TRADER
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_PRIEST
        {1,  10,   0,   0,   0,   0,   0},  // FIGURE_SCHOOL_CHILD
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_TEACHER
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_LIBRARIAN
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_BARBER
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_BATHHOUSE_WORKER
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_DOCTOR
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_SURGEON
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_WORKER
        {0,   0,   0,   0,   0,   0,   0},  // FIGURE_MAP_FLAG
        {0,   0,   0,   0,   0,   0,   0},  // FIGURE_FLOTSAM
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_DOCKER
        {1,  20,   0,   0,   0,   0,   0},  // FIGURE_MARKET_BUYER
        {1,  10,   0,   0,   0,   0,   0},  // FIGURE_PATRICIAN
        {5,  40,   6,   0,   0,   0,   0},  // FIGURE_INDIGENOUS_NATIVE
        {2,  50,   6,   0,   0,   6,  50},  // FIGURE_TOWER_SENTRY
        {3,  70,   5,   0,   0,   4,  70},  // FIGURE_ENEMY43_SPEAR
        {3,  90,   7,   0,   0,   0,   0},  // FIGURE_ENEMY44_SWORD
        {3, 120,  12,   2,   2,   0,   0},  // FIGURE_ENEMY45_SWORD
        {3, 120,   7,   1,   0,   5,  70},  // FIGURE_ENEMY46_CAMEL
        {3, 200,  20,   5,   8,   6,  70},  // FIGURE_ENEMY47_ELEPHANT
        {3, 120,  15,   4,   4,   0,   0},  // FIGURE_ENEMY48_CHARIOT
        {3,  90,   7,   1,   0,   0,   0},  // FIGURE_ENEMY49_FAST_SWORD
        {3, 110,  10,   1,   2,   0,   0},  // FIGURE_ENEMY50_SWORD
        {3,  70,   5,   0,   0,   3, 100},  // FIGURE_ENEMY51_SPEAR
        {3, 100,   6,   1,   0,   4,  70},  // FIGURE_ENEMY52_MOUNTED_ARCHER
        {3, 120,  15,   2,   3,   0,   0},  // FIGURE_ENEMY53_AXE
        {3, 100,   9,   2,   0,   0,   0},  // FIGURE_ENEMY54_GLADIATOR
        {3,  90,   4,   0,   0,   4, 100},  // FIGURE_ENEMY_CAESAR_JAVELIN
        {3, 100,   8,   0,   0,   0,   0},  // FIGURE_ENEMY_CAESAR_MOUNTED
        {3, 150,  13,   2,   0,   0,   0},  // FIGURE_ENEMY_CAESAR_LEGIONARY
        {5,  40,   0,   0,   0,   0,   0},  // FIGURE_NATIVE_TRADER
        {0, 100,   0,   0,   0,  12,   0},  // FIGURE_ARROW
        {0, 100,   0,   0,   0,  20,   0},  // FIGURE_JAVELIN
        {0, 100,   0,   0,   0, 200,   0},  // FIGURE_BOLT
        {0, 100,   0,   0,   0,   0, 200},  // FIGURE_BALLISTA
        {0, 100,   0,   0,   0,   0,   0},  // FIGURE_CREATURE
        {0, 100,   0,   0,   0,   0,   0},  // FIGURE_MISSIONARY
        {0, 100,   0,   0,   0,   0,   0},  // FIGURE_FISH_GULLS
        {0, 100,   0,   0,   0,   0,   0},  // FIGURE_DELIVERY_BOY
        {0, 100,   0,   0,   0,   0,   0},  // FIGURE_SHIPWRECK
        {6,  10,   0,   0,   0,   0,   0},  // FIGURE_SHEEP
        {3,  80,   8,   0,   0,   0,   0},  // FIGURE_WOLF
        {6,  20,   0,   0,   0,   0,   0},  // FIGURE_ZEBRA
        {0, 100,   0,   0,   0,  10,   0},  // FIGURE_SPEAR
        {0, 100,   0,   0,   0,   0,   0},  // FIGURE_HIPPODROME_HORSES
        {0, 100,   0,   0,   0,   0,   0},  // 
        {0, 100,   0,   0,   0,   0,   0},  // 
        {0, 100,   0,   0,   0,   0,   0},  // 
        {0, 100,   0,   0,   0,   0,   0},  // 
        {0, 100,   0,   0,   0,   0,   0},  // 
        {0, 100,   0,   0,   0,   0,   0},  // 
        {0, 100,   0,   0,   0,   0,   0}  // 
};

const figure_properties *figure_properties_for_type(figure_type type)
{
    return &properties[type];
}
