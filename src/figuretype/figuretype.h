#ifndef FIGURETYPE_ENTERTAINER_H
#define FIGURETYPE_ENTERTAINER_H

#include "figure/figure.h"
#include "map/map.h"

enum {
    TRADE_SHIP_NONE = 0,
    TRADE_SHIP_BUYING = 1,
    TRADE_SHIP_SELLING = 2,
};

enum {
    MAP_FLAG_ENTRY = 1,
    MAP_FLAG_EXIT = 2,
    MAP_FLAG_EARTHQUAKE_MIN = 3,
    MAP_FLAG_EARTHQUAKE_MAX = 12,
    MAP_FLAG_INVASION_MIN = 13,
    MAP_FLAG_INVASION_MAX = 20,
    MAP_FLAG_RIVER_ENTRY = 21,
    MAP_FLAG_RIVER_EXIT = 22,
    MAP_FLAG_FISHING_MIN = 23,
    MAP_FLAG_FISHING_MAX = 30,
    MAP_FLAG_HERD_MIN = 31,
    MAP_FLAG_HERD_MAX = 38,
    MAP_FLAG_MIN = 1,
    MAP_FLAG_MAX = 39,
};

void figure_hippodrome_horse_action(struct figure_t *f);

void figure_hippodrome_horse_reroute(void);

void figure_enemy_light_swordsman_action(struct figure_t *f);

void figure_enemy_heavy_swordsman_action(struct figure_t *f);

void figure_create_homeless(int x, int y, int num_people);

void figure_create_explosion_cloud(int x, int y, int size);

void figure_create_missile(struct figure_t *shooter, struct map_point_t *target_tile, int type);

int figure_trade_caravan_can_buy(struct figure_t *trader, int warehouse_id, int city_id);

int figure_trade_caravan_can_sell(struct figure_t *trader, int warehouse_id, int city_id);

void figure_tower_sentry_action(struct figure_t *f);

void figure_tower_sentry_reroute(void);

void figure_create_flotsam(void);

#endif