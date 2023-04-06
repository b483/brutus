#ifndef FIGURETYPE_TRADER_H
#define FIGURETYPE_TRADER_H

#include "figure/figure.h"

enum {
    TRADE_SHIP_NONE = 0,
    TRADE_SHIP_BUYING = 1,
    TRADE_SHIP_SELLING = 2,
};

int figure_trade_caravan_can_buy(struct figure_t *trader, int warehouse_id, int city_id);

int figure_trade_caravan_can_sell(struct figure_t *trader, int warehouse_id, int city_id);

void figure_trade_caravan_action(struct figure_t *f);

void figure_trade_caravan_donkey_action(struct figure_t *f);

void figure_native_trader_action(struct figure_t *f);

int figure_trade_ship_is_trading(struct figure_t *ship);

void figure_trade_ship_action(struct figure_t *f);

#endif // FIGURETYPE_TRADER_H
