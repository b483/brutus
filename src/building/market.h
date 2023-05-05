#ifndef BUILDING_MARKET_H
#define BUILDING_MARKET_H

#include "building/building.h"

int building_market_get_max_food_stock(struct building_t *market);
int building_market_get_max_goods_stock(struct building_t *market);
int building_market_get_storage_destination(struct building_t *market);

#endif // BUILDING_MARKET_H
