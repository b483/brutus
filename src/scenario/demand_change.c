#include "demand_change.h"

#include "city/message.h"
#include "core/random.h"
#include "empire/object.h"
#include "empire/trade_route.h"
#include "game/time.h"
#include "scenario/data.h"

void scenario_demand_change_process(void)
{
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        if (!scenario.demand_changes[i].resource || !scenario.demand_changes[i].route_id) {
            continue;
        }
        if (game_time_year() != scenario.demand_changes[i].year + scenario.start_year ||
            game_time_month() != scenario.demand_changes[i].month) {
            continue;
        }
        int route = scenario.demand_changes[i].route_id;
        int resource = scenario.demand_changes[i].resource;
        empire_object *object = empire_object_get_for_trade_route(route);
        if (scenario.demand_changes[i].is_rise) {
            if (trade_route_increase_limit(route, resource) && empire_object_trade_route_is_open(route)) {
                city_message_post(1, MESSAGE_INCREASED_TRADING, object->id, resource);
            }
        } else {
            if (trade_route_decrease_limit(route, resource) && empire_object_trade_route_is_open(route)) {
                if (trade_route_limit(route, resource) > 0) {
                    city_message_post(1, MESSAGE_DECREASED_TRADING, object->id, resource);
                } else {
                    city_message_post(1, MESSAGE_TRADE_STOPPED, object->id, resource);
                }
            }
        }
    }
}
