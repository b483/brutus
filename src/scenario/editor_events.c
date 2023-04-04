#include "editor_events.h"

#include "building/count.h"
#include "building/destruction.h"
#include "building/warehouse.h"
#include "city/data_private.h"
#include "city/health.h"
#include "city/message.h"
#include "city/military.h"
#include "city/population.h"
#include "city/ratings.h"
#include "city/trade.h"
#include "core/calc.h"
#include "core/image.h"
#include "core/random.h"
#include "empire/object.h"
#include "empire/trade_prices.h"
#include "figure/figure.h"
#include "figure/formation_enemy.h"
#include "figure/name.h"
#include "figuretype/missile.h"
#include "game/time.h"
#include "map/building.h"
#include "map/grid.h"
#include "map/routing_terrain.h"
#include "map/terrain.h"
#include "map/tiles.h"
#include "scenario/data.h"
#include "scenario/map.h"
#include "sound/effect.h"

#include <stdlib.h>

#define MAX_ENEMY_TYPES_PER_ARMY 3

enum {
    EVENT_ROME_RAISES_WAGES = 1,
    EVENT_ROME_LOWERS_WAGES = 2,
    EVENT_LAND_TRADE_DISRUPTED = 3,
    EVENT_LAND_SEA_DISRUPTED = 4,
    EVENT_CONTAMINATED_WATER = 5
};

static const int RANDOM_EVENT_PROBABILITY[128] = {
    0, 0, 1, 0, 0, 0, 4, 0, 0, 0, 0, 3, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 3, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 2, 0, 0, 0, 0, 0, 5, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 4, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 2, 0, 0, 4, 0, 0,
    0, 0, 3, 0, 0, 4, 0, 0, 0, 0, 0, 0, 0, 0, 5, 0
};

static const int ENEMY_ID_TO_ENEMY_TYPE[ENEMY_TYPE_MAX_COUNT] = {
    ENEMY_TYPE_BARBARIAN,
    ENEMY_TYPE_CARTHAGINIAN,
    ENEMY_TYPE_CELT,
    ENEMY_TYPE_CELT,
    ENEMY_TYPE_CELT,
    ENEMY_TYPE_EGYPTIAN,
    ENEMY_TYPE_ETRUSCAN,
    ENEMY_TYPE_ETRUSCAN,
    ENEMY_TYPE_GAUL,
    ENEMY_TYPE_GAUL,
    ENEMY_TYPE_GOTH,
    ENEMY_TYPE_GOTH,
    ENEMY_TYPE_GOTH,
    ENEMY_TYPE_GREEK,
    ENEMY_TYPE_GREEK,
    ENEMY_TYPE_NUMIDIAN,
    ENEMY_TYPE_PERGAMUM,
    ENEMY_TYPE_PHOENICIAN,
    ENEMY_TYPE_PHOENICIAN,
    ENEMY_TYPE_PHOENICIAN
};

static const struct {
    int pct_type[MAX_ENEMY_TYPES_PER_ARMY];
    int figure_types[MAX_ENEMY_TYPES_PER_ARMY];
    int formation_layout;
} ENEMY_PROPERTIES[12] = {
    {{100, 0, 0}, {FIGURE_ENEMY_FAST_SWORD, 0, 0}, FORMATION_ENEMY_MOB}, // barbarian
    {{90, 10, 0}, {FIGURE_ENEMY_SWORD_2, FIGURE_ENEMY_ELEPHANT, 0}, FORMATION_ENEMY_WIDE_COLUMN}, // carthaginian
    {{80, 20, 0}, {FIGURE_ENEMY_SWORD_3, FIGURE_ENEMY_CHARIOT, 0}, FORMATION_ENEMY_MOB}, // celt
    {{80, 20, 0}, {FIGURE_ENEMY_SWORD_1, FIGURE_ENEMY_CAMEL, 0}, FORMATION_ENEMY_WIDE_COLUMN}, // egyptian
    {{50, 50, 0}, {FIGURE_ENEMY_SWORD_2, FIGURE_ENEMY_RANGED_SPEAR_1, 0}, FORMATION_ENEMY_DOUBLE_LINE}, // etruscan
    {{50, 50, 0}, {FIGURE_ENEMY_SWORD_3, FIGURE_ENEMY_AXE, 0}, FORMATION_ENEMY_MOB}, // gaul
    {{50, 50, 0}, {FIGURE_ENEMY_FAST_SWORD, FIGURE_ENEMY_MOUNTED_ARCHER, 0}, FORMATION_ENEMY_MOB}, // goth
    {{80, 20, 0}, {FIGURE_ENEMY_SWORD_2, FIGURE_ENEMY_RANGED_SPEAR_1, 0}, FORMATION_ENEMY_DOUBLE_LINE}, // greek
    {{40, 60, 0}, {FIGURE_ENEMY_FAST_SWORD, FIGURE_ENEMY_RANGED_SPEAR_2, 0}, FORMATION_ENEMY_MOB}, // numidian
    {{30, 70, 0}, {FIGURE_ENEMY_SWORD_1, FIGURE_ENEMY_RANGED_SPEAR_1, 0}, FORMATION_TORTOISE}, // pergamum
    {{50, 50, 0}, {FIGURE_ENEMY_SWORD_1, FIGURE_ENEMY_RANGED_SPEAR_1, 0}, FORMATION_ENEMY_DOUBLE_LINE}, // phoenician
    {{100, 0, 0}, {FIGURE_ENEMY_CAESAR_LEGIONARY, 0, 0}, FORMATION_TORTOISE} // caesar
};

void scenario_empire_process_expansion(void)
{
    if (scenario.empire.is_expanded || scenario.empire.expansion_year <= 0) {
        return;
    }
    if (game_time_year() < scenario.empire.expansion_year + scenario.start_year) {
        return;
    }

    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (!empire_objects[i].in_use || empire_objects[i].type != EMPIRE_OBJECT_CITY) {
            continue;
        }
        if (empire_objects[i].city_type == EMPIRE_CITY_FUTURE_TRADE) {
            empire_objects[i].city_type = EMPIRE_CITY_TRADE;
            empire_objects[i].expanded.image_id = image_group(GROUP_EMPIRE_CITY_TRADE);
        } else if (empire_objects[i].city_type == EMPIRE_CITY_FUTURE_ROMAN) {
            empire_objects[i].city_type = EMPIRE_CITY_DISTANT_ROMAN;
            empire_objects[i].expanded.image_id = image_group(GROUP_EMPIRE_CITY_DISTANT_ROMAN);
        } else {
            continue;
        }
    }
    scenario.empire.is_expanded = 1;
    city_message_post(1, MESSAGE_EMPIRE_HAS_EXPANDED, 0, 0);
}

int scenario_building_allowed(int building_type)
{
    switch (building_type) {
        case BUILDING_HOUSE_VACANT_LOT:
            return scenario.allowed_buildings[ALLOWED_BUILDING_HOUSE_VACANT_LOT];
        case BUILDING_CLEAR_LAND:
            return scenario.allowed_buildings[ALLOWED_BUILDING_CLEAR_TERRAIN];
        case BUILDING_ROAD:
            return scenario.allowed_buildings[ALLOWED_BUILDING_ROAD];
        case BUILDING_RESERVOIR:
            return scenario.allowed_buildings[ALLOWED_BUILDING_RESERVOIR];
        case BUILDING_AQUEDUCT:
            return scenario.allowed_buildings[ALLOWED_BUILDING_AQUEDUCT];
        case BUILDING_FOUNTAIN:
            return scenario.allowed_buildings[ALLOWED_BUILDING_FOUNTAIN];
        case BUILDING_WELL:
            return scenario.allowed_buildings[ALLOWED_BUILDING_WELL];
        case BUILDING_BARBER:
            return scenario.allowed_buildings[ALLOWED_BUILDING_BARBER];
        case BUILDING_BATHHOUSE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_BATHHOUSE];
        case BUILDING_DOCTOR:
            return scenario.allowed_buildings[ALLOWED_BUILDING_DOCTOR];
        case BUILDING_HOSPITAL:
            return scenario.allowed_buildings[ALLOWED_BUILDING_HOSPITAL];
        case BUILDING_SMALL_TEMPLE_CERES:
            return scenario.allowed_buildings[ALLOWED_BUILDING_SMALL_TEMPLE_CERES];
        case BUILDING_SMALL_TEMPLE_NEPTUNE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_SMALL_TEMPLE_NEPTUNE];
        case BUILDING_SMALL_TEMPLE_MERCURY:
            return scenario.allowed_buildings[ALLOWED_BUILDING_SMALL_TEMPLE_MERCURY];
        case BUILDING_SMALL_TEMPLE_MARS:
            return scenario.allowed_buildings[ALLOWED_BUILDING_SMALL_TEMPLE_MARS];
        case BUILDING_SMALL_TEMPLE_VENUS:
            return scenario.allowed_buildings[ALLOWED_BUILDING_SMALL_TEMPLE_VENUS];
        case BUILDING_LARGE_TEMPLE_CERES:
            return scenario.allowed_buildings[ALLOWED_BUILDING_LARGE_TEMPLE_CERES];
        case BUILDING_LARGE_TEMPLE_NEPTUNE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_LARGE_TEMPLE_NEPTUNE];
        case BUILDING_LARGE_TEMPLE_MERCURY:
            return scenario.allowed_buildings[ALLOWED_BUILDING_LARGE_TEMPLE_MERCURY];
        case BUILDING_LARGE_TEMPLE_MARS:
            return scenario.allowed_buildings[ALLOWED_BUILDING_LARGE_TEMPLE_MARS];
        case BUILDING_LARGE_TEMPLE_VENUS:
            return scenario.allowed_buildings[ALLOWED_BUILDING_LARGE_TEMPLE_VENUS];
        case BUILDING_ORACLE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_ORACLE];
        case BUILDING_SCHOOL:
            return scenario.allowed_buildings[ALLOWED_BUILDING_SCHOOL];
        case BUILDING_ACADEMY:
            return scenario.allowed_buildings[ALLOWED_BUILDING_ACADEMY];
        case BUILDING_LIBRARY:
            return scenario.allowed_buildings[ALLOWED_BUILDING_LIBRARY];
        case BUILDING_MISSION_POST:
            return scenario.allowed_buildings[ALLOWED_BUILDING_MISSION_POST];
        case BUILDING_THEATER:
            return scenario.allowed_buildings[ALLOWED_BUILDING_THEATER];
        case BUILDING_AMPHITHEATER:
            return scenario.allowed_buildings[ALLOWED_BUILDING_AMPHITHEATER];
        case BUILDING_COLOSSEUM:
            return scenario.allowed_buildings[ALLOWED_BUILDING_COLOSSEUM];
        case BUILDING_HIPPODROME:
            return scenario.allowed_buildings[ALLOWED_BUILDING_HIPPODROME];
        case BUILDING_GLADIATOR_SCHOOL:
            return scenario.allowed_buildings[ALLOWED_BUILDING_GLADIATOR_SCHOOL];
        case BUILDING_LION_HOUSE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_LION_HOUSE];
        case BUILDING_ACTOR_COLONY:
            return scenario.allowed_buildings[ALLOWED_BUILDING_ACTOR_COLONY];
        case BUILDING_CHARIOT_MAKER:
            return scenario.allowed_buildings[ALLOWED_BUILDING_CHARIOT_MAKER];
        case BUILDING_FORUM:
            return scenario.allowed_buildings[ALLOWED_BUILDING_FORUM];
        case BUILDING_SENATE_UPGRADED:
            return scenario.allowed_buildings[ALLOWED_BUILDING_SENATE_UPGRADED];
        case BUILDING_GOVERNORS_HOUSE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_GOVERNORS_HOUSE];
        case BUILDING_GOVERNORS_VILLA:
            return scenario.allowed_buildings[ALLOWED_BUILDING_GOVERNORS_VILLA];
        case BUILDING_GOVERNORS_PALACE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_GOVERNORS_PALACE];
        case BUILDING_SMALL_STATUE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_SMALL_STATUE];
        case BUILDING_MEDIUM_STATUE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_MEDIUM_STATUE];
        case BUILDING_LARGE_STATUE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_LARGE_STATUE];
        case BUILDING_TRIUMPHAL_ARCH:
            return scenario.allowed_buildings[ALLOWED_BUILDING_TRIUMPHAL_ARCH];
        case BUILDING_GARDENS:
            return scenario.allowed_buildings[ALLOWED_BUILDING_GARDENS];
        case BUILDING_PLAZA:
            return scenario.allowed_buildings[ALLOWED_BUILDING_PLAZA];
        case BUILDING_ENGINEERS_POST:
            return scenario.allowed_buildings[ALLOWED_BUILDING_ENGINEERS_POST];
        case BUILDING_LOW_BRIDGE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_LOW_BRIDGE];
        case BUILDING_SHIP_BRIDGE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_SHIP_BRIDGE];
        case BUILDING_SHIPYARD:
            return scenario.allowed_buildings[ALLOWED_BUILDING_SHIPYARD];
        case BUILDING_WHARF:
            return scenario.allowed_buildings[ALLOWED_BUILDING_WHARF];
        case BUILDING_DOCK:
            return scenario.allowed_buildings[ALLOWED_BUILDING_DOCK];
        case BUILDING_WALL:
            return scenario.allowed_buildings[ALLOWED_BUILDING_WALL];
        case BUILDING_TOWER:
            return scenario.allowed_buildings[ALLOWED_BUILDING_TOWER];
        case BUILDING_GATEHOUSE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_GATEHOUSE];
        case BUILDING_PREFECTURE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_PREFECTURE];
        case BUILDING_FORT_LEGIONARIES:
            return scenario.allowed_buildings[ALLOWED_BUILDING_FORT_LEGIONARIES];
        case BUILDING_FORT_JAVELIN:
            return scenario.allowed_buildings[ALLOWED_BUILDING_FORT_JAVELIN];
        case BUILDING_FORT_MOUNTED:
            return scenario.allowed_buildings[ALLOWED_BUILDING_FORT_MOUNTED];
        case BUILDING_MILITARY_ACADEMY:
            return scenario.allowed_buildings[ALLOWED_BUILDING_MILITARY_ACADEMY];
        case BUILDING_BARRACKS:
            return scenario.allowed_buildings[ALLOWED_BUILDING_BARRACKS];
        case BUILDING_WHEAT_FARM:
            return scenario.allowed_buildings[ALLOWED_BUILDING_WHEAT_FARM];
        case BUILDING_VEGETABLE_FARM:
            return scenario.allowed_buildings[ALLOWED_BUILDING_VEGETABLE_FARM];
        case BUILDING_FRUIT_FARM:
            return scenario.allowed_buildings[ALLOWED_BUILDING_FRUIT_FARM];
        case BUILDING_OLIVE_FARM:
            return scenario.allowed_buildings[ALLOWED_BUILDING_OLIVE_FARM];
        case BUILDING_VINES_FARM:
            return scenario.allowed_buildings[ALLOWED_BUILDING_VINES_FARM];
        case BUILDING_PIG_FARM:
            return scenario.allowed_buildings[ALLOWED_BUILDING_PIG_FARM];
        case BUILDING_CLAY_PIT:
            return scenario.allowed_buildings[ALLOWED_BUILDING_CLAY_PIT];
        case BUILDING_MARBLE_QUARRY:
            return scenario.allowed_buildings[ALLOWED_BUILDING_MARBLE_QUARRY];
        case BUILDING_IRON_MINE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_IRON_MINE];
        case BUILDING_TIMBER_YARD:
            return scenario.allowed_buildings[ALLOWED_BUILDING_TIMBER_YARD];
        case BUILDING_WINE_WORKSHOP:
            return scenario.allowed_buildings[ALLOWED_BUILDING_WINE_WORKSHOP];
        case BUILDING_OIL_WORKSHOP:
            return scenario.allowed_buildings[ALLOWED_BUILDING_OIL_WORKSHOP];
        case BUILDING_WEAPONS_WORKSHOP:
            return scenario.allowed_buildings[ALLOWED_BUILDING_WEAPONS_WORKSHOP];
        case BUILDING_FURNITURE_WORKSHOP:
            return scenario.allowed_buildings[ALLOWED_BUILDING_FURNITURE_WORKSHOP];
        case BUILDING_POTTERY_WORKSHOP:
            return scenario.allowed_buildings[ALLOWED_BUILDING_POTTERY_WORKSHOP];
        case BUILDING_MARKET:
            return scenario.allowed_buildings[ALLOWED_BUILDING_MARKET];
        case BUILDING_GRANARY:
            return scenario.allowed_buildings[ALLOWED_BUILDING_GRANARY];
        case BUILDING_WAREHOUSE:
            return scenario.allowed_buildings[ALLOWED_BUILDING_WAREHOUSE];
    }
    return 1;
}

void scenario_gladiator_revolt_process(void)
{
    if (!scenario.gladiator_revolt.state) {
        return;
    }
    if (scenario.gladiator_revolt.state == EVENT_NOT_STARTED) {
        if (game_time_year() == scenario.start_year + scenario.gladiator_revolt.year && game_time_month() == scenario.gladiator_revolt.month) {
            if (building_count_active(BUILDING_GLADIATOR_SCHOOL) > 0) {
                scenario.gladiator_revolt.state = EVENT_IN_PROGRESS;
                city_message_post(1, MESSAGE_GLADIATOR_REVOLT, 0, 0);
            } else {
                scenario.gladiator_revolt.state = EVENT_FINISHED;
            }
        }
    } else if (scenario.gladiator_revolt.state == EVENT_IN_PROGRESS) {
        if (scenario.gladiator_revolt.month + 3 == game_time_month()) {
            scenario.gladiator_revolt.state = EVENT_FINISHED;
            city_message_post(1, MESSAGE_GLADIATOR_REVOLT_FINISHED, 0, 0);
        }
    }
}

static void advance_earthquake_to_tile(int x, int y)
{
    int grid_offset = map_grid_offset(x, y);
    int building_id = map_building_at(grid_offset);
    if (building_id) {
        building_destroy_by_collapse(building_get(building_id));
        int ruin_id = map_building_at(grid_offset);
        if (ruin_id) {
            building_get(ruin_id)->state = BUILDING_STATE_DELETED_BY_GAME;
            map_building_set(grid_offset, 0);
        }
    }
    map_terrain_set(grid_offset, 0);
    map_tiles_set_earthquake(x, y);
    map_tiles_update_all_gardens();
    map_tiles_update_all_roads();
    map_tiles_update_all_plazas();

    map_routing_update_land();
    map_routing_update_walls();

    sound_effect_play(SOUND_EFFECT_EXPLOSION);
    figure_create_explosion_cloud(x, y, 1);
}

void scenario_earthquake_process(void)
{
    if (!scenario.earthquake.state || scenario.earthquake.branch_coordinates[0].x == -1 || scenario.earthquake.branch_coordinates[0].y == -1) {
        return;
    }
    if (scenario.earthquake.state == EVENT_NOT_STARTED) {
        if (scenario.start_year + scenario.earthquake.year == game_time_year() && scenario.earthquake.month == game_time_month()) {
            scenario.earthquake.state = EVENT_IN_PROGRESS;
            city_message_post(1, MESSAGE_EARTHQUAKE, 0, map_grid_offset(scenario.earthquake.branch_coordinates[0].x, scenario.earthquake.branch_coordinates[1].y));
        }
    } else if (scenario.earthquake.state == EVENT_IN_PROGRESS) {
        scenario.earthquake.delay++;
        if (scenario.earthquake.delay >= scenario.earthquake.max_delay) {
            scenario.earthquake.delay = 0;
            scenario.earthquake.duration++;
            if (scenario.earthquake.duration >= scenario.earthquake.max_duration) {
                scenario.earthquake.state = EVENT_FINISHED;
            }
            int index = rand() % 4;
            int dx = 0;
            int dy = 0;
            switch (index) {
                case 0:
                    // ~north
                    dx = rand() % 3 - 1;
                    dy = dx ? 0 : -1;
                    break;
                case 1:
                    // ~east
                    dy = rand() % 3 - 1;
                    dx = dy ? 0 : 1;
                    break;
                case 2:
                    // ~south
                    dx = rand() % 3 - 1;
                    dy = dx ? 0 : 1;
                    break;
                case 3:
                    // ~west
                    dy = rand() % 3 - 1;
                    dx = dy ? 0 : -1;
                    break;
                default:
                    break;
            }
            int x = calc_bound(scenario.earthquake.branch_coordinates[index].x + dx, 0, scenario.map.width - 1);
            int y = calc_bound(scenario.earthquake.branch_coordinates[index].y + dy, 0, scenario.map.height - 1);
            if (!map_terrain_is(map_grid_offset(x, y), TERRAIN_ELEVATION | TERRAIN_ROCK | TERRAIN_WATER)) {
                scenario.earthquake.branch_coordinates[index].x = x;
                scenario.earthquake.branch_coordinates[index].y = y;
                advance_earthquake_to_tile(x, y);
            }
        }
    }
}

void scenario_random_event_process(void)
{
    int event = RANDOM_EVENT_PROBABILITY[random_byte()];
    switch (event) {
        case EVENT_ROME_RAISES_WAGES:
            if (scenario.random_events.raise_wages) {
                if (city_labor_raise_wages_rome()) {
                    city_message_post(1, MESSAGE_ROME_RAISES_WAGES, 0, 0);
                }
            }
            break;
        case EVENT_ROME_LOWERS_WAGES:
            if (scenario.random_events.lower_wages) {
                if (city_labor_lower_wages_rome()) {
                    city_message_post(1, MESSAGE_ROME_LOWERS_WAGES, 0, 0);
                }
            }
            break;
        case EVENT_LAND_TRADE_DISRUPTED:
            if (scenario.random_events.land_trade_problem) {
                if (city_data.trade.num_land_routes) {
                    city_data.trade.land_trade_problem_duration = 48;
                    if (scenario.climate == CLIMATE_DESERT) {
                        city_message_post(1, MESSAGE_LAND_TRADE_DISRUPTED_SANDSTORMS, 0, 0);
                    } else {
                        city_message_post(1, MESSAGE_LAND_TRADE_DISRUPTED_LANDSLIDES, 0, 0);
                    }
                }
            }
            break;
        case EVENT_LAND_SEA_DISRUPTED:
            if (scenario.random_events.sea_trade_problem) {
                if (city_data.trade.num_sea_routes) {
                    city_data.trade.sea_trade_problem_duration = 48;
                    city_message_post(1, MESSAGE_SEA_TRADE_DISRUPTED, 0, 0);
                }
            }
            break;
        case EVENT_CONTAMINATED_WATER:
            if (scenario.random_events.contaminated_water) {
                if (city_data.population.population > 200) {
                    int change;
                    int health_rate = city_health();
                    if (health_rate > 80) {
                        change = -50;
                    } else if (health_rate > 60) {
                        change = -40;
                    } else {
                        change = -25;
                    }
                    city_health_change(change);
                    city_message_post(1, MESSAGE_CONTAMINATED_WATER, 0, 0);
                }
            }
            break;
    }
}

void scenario_request_process(void)
{
    for (int i = 0; i < MAX_REQUESTS; i++) {
        if (!scenario.requests[i].resource || scenario.requests[i].state > REQUEST_STATE_DISPATCHED_LATE) {
            continue;
        }
        if (scenario.requests[i].state == REQUEST_STATE_DISPATCHED || scenario.requests[i].state == REQUEST_STATE_DISPATCHED_LATE) {
            --scenario.requests[i].months_to_comply;
            if (scenario.requests[i].months_to_comply <= 0) {
                if (scenario.requests[i].state == REQUEST_STATE_DISPATCHED) {
                    city_message_post(1, MESSAGE_REQUEST_RECEIVED, i, 0);
                    city_ratings_change_favor(scenario.requests[i].favor);
                } else {
                    city_message_post(1, MESSAGE_REQUEST_RECEIVED_LATE, i, 0);
                    city_ratings_change_favor(scenario.requests[i].favor / 2);
                }
                scenario.requests[i].state = REQUEST_STATE_RECEIVED;
                scenario.requests[i].visible = 0;
            }
        } else {
            // normal or overdue
            if (scenario.requests[i].visible) {
                --scenario.requests[i].months_to_comply;
                if (scenario.requests[i].state == REQUEST_STATE_NORMAL) {
                    if (scenario.requests[i].months_to_comply == 12) {
                        // reminder
                        city_message_post(1, MESSAGE_REQUEST_REMINDER, i, 0);
                    } else if (scenario.requests[i].months_to_comply <= 0) {
                        city_message_post(1, MESSAGE_REQUEST_REFUSED, i, 0);
                        scenario.requests[i].state = REQUEST_STATE_OVERDUE;
                        scenario.requests[i].months_to_comply = 24;
                        city_ratings_reduce_favor_missed_request(3);
                    }
                } else if (scenario.requests[i].state == REQUEST_STATE_OVERDUE) {
                    if (scenario.requests[i].months_to_comply <= 0) {
                        city_message_post(1, MESSAGE_REQUEST_REFUSED_OVERDUE, i, 0);
                        scenario.requests[i].state = REQUEST_STATE_IGNORED;
                        scenario.requests[i].visible = 0;
                        city_ratings_reduce_favor_missed_request(5);
                    }
                }
                if (!scenario.requests[i].can_comply_dialog_shown &&
                    city_data.resource.stored_in_warehouses[scenario.requests[i].resource] >= scenario.requests[i].amount) {
                    scenario.requests[i].can_comply_dialog_shown = 1;
                    city_message_post(1, MESSAGE_REQUEST_CAN_COMPLY, i, 0);
                }
            } else {
                // request is not visible
                int year = scenario.start_year;
                if (game_time_year() == year + scenario.requests[i].year &&
                    game_time_month() == scenario.requests[i].month) {
                    scenario.requests[i].visible = 1;
                    if (city_data.resource.stored_in_warehouses[scenario.requests[i].resource] >= scenario.requests[i].amount) {
                        scenario.requests[i].can_comply_dialog_shown = 1;
                    }
                    if (scenario.requests[i].resource == RESOURCE_DENARII) {
                        city_message_post(1, MESSAGE_CAESAR_REQUESTS_MONEY, i, 0);
                    } else if (scenario.requests[i].resource == RESOURCE_TROOPS) {
                        city_message_post(1, MESSAGE_CAESAR_REQUESTS_ARMY, i, 0);
                    } else {
                        city_message_post(1, MESSAGE_CAESAR_REQUESTS_GOODS, i, 0);
                    }
                }
            }
        }
    }
}

void scenario_request_dispatch(int id)
{
    if (scenario.requests[id].state == REQUEST_STATE_NORMAL) {
        scenario.requests[id].state = REQUEST_STATE_DISPATCHED;
    } else {
        scenario.requests[id].state = REQUEST_STATE_DISPATCHED_LATE;
    }
    random_generate_next();
    scenario.requests[id].months_to_comply = (random_byte() & 3) + 1;
    scenario.requests[id].visible = 0;
    if (scenario.requests[id].resource == RESOURCE_DENARII) {
        city_finance_process_sundry(scenario.requests[id].amount);
    } else if (scenario.requests[id].resource == RESOURCE_TROOPS) {
        city_population_remove_for_troop_request(scenario.requests[id].amount);
        building_warehouses_remove_resource(RESOURCE_WEAPONS, scenario.requests[id].amount);
    } else {
        building_warehouses_remove_resource(scenario.requests[id].resource, scenario.requests[id].amount);
    }
}

void scenario_custom_messages_process(void)
{
    for (int i = 0; i < MAX_EDITOR_CUSTOM_MESSAGES; i++) {
        if (!scenario.editor_custom_messages[i].enabled) {
            continue;
        }
        if (game_time_year() == scenario.editor_custom_messages[i].year + scenario.start_year
            && game_time_month() == scenario.editor_custom_messages[i].month) {
            city_message_post(1, MESSAGE_EDITOR_1 + i, 0, 0);
        }
    }
}

static void create_enemy_squad(int figure_type, int enemy_type, int enemy_type_detailed, int x, int y, int figures_amount, int orientation, int spawn_delay_offset, int enemy_attack_priority, int invasion_id)
{
    struct formation_t *m = formation_create_enemy(figure_type, figures_amount, x, y, ENEMY_PROPERTIES[enemy_type].formation_layout, orientation, enemy_type, enemy_attack_priority, invasion_id);
    if (m) {
        for (int fig = 0; fig < figures_amount; fig++) {
            figure *f = figure_create(figure_type, x, y, orientation);
            f->action_state = FIGURE_ACTION_ENEMY_INITIAL;
            f->wait_ticks = 40 * spawn_delay_offset + 10 * fig + 10;
            f->formation_id = m->id;
            f->name = figure_name_get(figure_type, enemy_type);
            f->enemy_image_type = enemy_type;
            if (f->type == FIGURE_ENEMY_RANGED_SPEAR_1 && f->enemy_image_type == ENEMY_TYPE_PERGAMUM) {
                f->missile_type = FIGURE_ARROW;
                f->max_range = 15;
            }
            f->enemy_image_type_detailed = enemy_type_detailed;
            f->is_ghost = 1;
            add_figure_to_formation(f, m);
        }
    }
}

int start_invasion(int enemy_type, int enemy_type_detailed, int amount, int invasion_point, int enemy_attack_priority, int invasion_id)
{
    if (amount <= 0) {
        return -1;
    }

    int x, y;
    // determine invasion point
    if (enemy_type == ENEMY_TYPE_CAESAR) {
        x = scenario.entry_point.x;
        y = scenario.entry_point.y;
    } else {
        int invasion_flags_placed = 0;
        for (int i = 0; i < MAX_INVASION_POINTS; i++) {
            if (scenario.invasion_points[i].x != -1) {
                invasion_flags_placed++;
            }
        }
        if (invasion_point == MAX_INVASION_POINTS) { // random
            if (invasion_flags_placed) {
                // one or more, not necessarily sequential
                int rnd_point = rand() % MAX_INVASION_POINTS;
                x = scenario.invasion_points[rnd_point].x;
                y = scenario.invasion_points[rnd_point].y;
                while (x == -1) {
                    if (rnd_point < MAX_INVASION_POINTS - 1) {
                        rnd_point++;
                    } else {
                        rnd_point = 0;
                    }
                    x = scenario.invasion_points[rnd_point].x;
                    y = scenario.invasion_points[rnd_point].y;
                }
            } else {
                // no invasion flags placed
                x = scenario.exit_point.x;
                y = scenario.exit_point.y;
            }
        } else {
            x = scenario.invasion_points[invasion_point].x;
            y = scenario.invasion_points[invasion_point].y;
        }
    }

    // check terrain
    int grid_offset = map_grid_offset(x, y);
    if (map_terrain_is(grid_offset, TERRAIN_ELEVATION | TERRAIN_ROCK | TERRAIN_SHRUB)) {
        return -1;
    }
    if (map_terrain_is(grid_offset, TERRAIN_WATER)) {
        if (!map_terrain_is(grid_offset, TERRAIN_ROAD)) { // bridge
            return -1;
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_BUILDING | TERRAIN_AQUEDUCT | TERRAIN_GATEHOUSE | TERRAIN_WALL)) {
        building_destroy_by_enemy(x, y, grid_offset);
    }

    // determine orientation
    int orientation;
    if (y == 0) {
        orientation = DIR_4_BOTTOM;
    } else if (y >= scenario.map.height - 1) {
        orientation = DIR_0_TOP;
    } else if (x == 0) {
        orientation = DIR_2_RIGHT;
    } else if (x >= scenario.map.width - 1) {
        orientation = DIR_6_LEFT;
    } else {
        orientation = DIR_4_BOTTOM;
    }

    int enemy_count_per_type[MAX_ENEMY_TYPES_PER_ARMY];
    for (int i = 0; i < MAX_ENEMY_TYPES_PER_ARMY; i++) {
        enemy_count_per_type[i] = calc_adjust_with_percentage(amount, ENEMY_PROPERTIES[enemy_type].pct_type[i]);
    }
    enemy_count_per_type[0] += amount - (enemy_count_per_type[0] + enemy_count_per_type[1] + enemy_count_per_type[2]); // assign leftovers to first type

    int spawn_delay_offset = 0;
    for (int i = 0; i < MAX_ENEMY_TYPES_PER_ARMY; i++) {
        int figure_type = ENEMY_PROPERTIES[enemy_type].figure_types[i];
        while (enemy_count_per_type[i]) {
            if (enemy_count_per_type[i] >= MAX_FORMATION_FIGURES) {
                create_enemy_squad(figure_type, enemy_type, enemy_type_detailed, x, y, MAX_FORMATION_FIGURES, orientation, spawn_delay_offset, enemy_attack_priority, invasion_id);
                enemy_count_per_type[i] -= MAX_FORMATION_FIGURES;
                spawn_delay_offset++;
            } else {
                create_enemy_squad(figure_type, enemy_type, enemy_type_detailed, x, y, enemy_count_per_type[i], orientation, spawn_delay_offset, enemy_attack_priority, invasion_id);
                enemy_count_per_type[i] = 0;
            }
        }
    }

    return grid_offset;
}

void scenario_invasion_process(void)
{
    // handle warnings
    for (int i = 0; i < MAX_INVASIONS; i++) {
        for (int j = 3; j > 0; j--) {
            if (scenario.invasions[i].type == INVASION_TYPE_ENEMY_ARMY
            && (game_time_year() == scenario.start_year + scenario.invasions[i].year_offset - j && game_time_month() == scenario.invasions[i].month)) {
                scenario.invasion_upcoming = 1;
                city_message_post(0, MESSAGE_ENEMIES_AT_THE_DOOR + 1 - j, 0, 0);
            }
        }

    }

    // trigger invasions
    for (int i = 0; i < MAX_INVASIONS; i++) {
        if (game_time_year() == scenario.start_year + scenario.invasions[i].year_offset && game_time_month() == scenario.invasions[i].month) {
            // enemy army
            if (scenario.invasions[i].type == INVASION_TYPE_ENEMY_ARMY) {
                int grid_offset = start_invasion(
                    ENEMY_ID_TO_ENEMY_TYPE[scenario.invasions[i].enemy_type],
                    scenario.invasions[i].enemy_type,
                    scenario.invasions[i].amount,
                    scenario.invasions[i].from,
                    scenario.invasions[i].target_type,
                    i);
                if (grid_offset > 0) {
                    if (scenario.invasions[i].enemy_type) {
                        city_message_post(1, MESSAGE_ENEMY_ARMY_ATTACK, 0, grid_offset);
                    } else {
                        city_message_post(1, MESSAGE_BARBARIAN_ATTACK, 0, grid_offset);
                    }
                }
            }
            // editor scheduled invasion by Caesar
            if (scenario.invasions[i].type == INVASION_TYPE_CAESAR) {
                int grid_offset = start_invasion(
                    ENEMY_TYPE_CAESAR,
                    scenario.invasions[i].enemy_type,
                    scenario.invasions[i].amount,
                    scenario.invasions[i].from,
                    scenario.invasions[i].target_type,
                    i);
                if (grid_offset > 0) {
                    city_message_post(1, MESSAGE_CAESAR_ARMY_ATTACK, 0, grid_offset);
                }
                city_data.emperor.invasion.from_editor = 1;
                city_data.emperor.invasion.size = scenario.invasions[i].amount;
            }
            // local uprisings
            if (scenario.invasions[i].type == INVASION_TYPE_LOCAL_UPRISING) {
                int grid_offset = start_invasion(
                    ENEMY_TYPE_BARBARIAN,
                    ENEMY_TYPE_BARBARIAN,
                    scenario.invasions[i].amount,
                    scenario.invasions[i].from,
                    scenario.invasions[i].target_type,
                    i);
                if (grid_offset > 0) {
                    city_message_post(1, MESSAGE_LOCAL_UPRISING, 0, grid_offset);
                }
            }
            scenario.invasion_upcoming = 0;
        }
    }
}

int scenario_invasion_start_from_caesar(int size)
{
    int grid_offset = start_invasion(ENEMY_TYPE_CAESAR, ENEMY_TYPE_CAESAR, size, 0, FORMATION_ATTACK_BEST_BUILDINGS, 24);
    if (grid_offset > 0) {
        city_message_post(1, MESSAGE_CAESAR_ARMY_ATTACK, 0, grid_offset);
        return 1;
    }
    return 0;
}

void scenario_invasion_start_from_cheat(void)
{
    int enemy_id = rand() % ENEMY_TYPE_MAX_COUNT;
    start_invasion(ENEMY_ID_TO_ENEMY_TYPE[enemy_id], enemy_id, 160, MAX_INVASION_POINTS, FORMATION_ATTACK_FOOD_CHAIN, 23);
}

void scenario_distant_battle_process(void)
{
    for (int i = 0; i < MAX_INVASIONS; i++) {
        if (scenario.invasions[i].type == INVASION_TYPE_DISTANT_BATTLE &&
            game_time_year() == scenario.invasions[i].year_offset + scenario.start_year &&
            game_time_month() == scenario.invasions[i].month &&
            scenario.empire.distant_battle_enemy_travel_months > 4 &&
            scenario.empire.distant_battle_roman_travel_months > 4 &&
            !city_military_has_distant_battle()) {

            city_message_post(1, MESSAGE_CAESAR_REQUESTS_ARMY, 0, 0);
            city_military_init_distant_battle(scenario.invasions[i].amount);
            return;
        }
    }

    city_military_process_distant_battle();
}

void scenario_price_change_process(void)
{
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        if (!scenario.price_changes[i].resource) {
            continue;
        }
        if (game_time_year() != scenario.price_changes[i].year + scenario.start_year ||
            game_time_month() != scenario.price_changes[i].month) {
            continue;
        }
        if (scenario.price_changes[i].is_rise) {
            if (trade_price_change(scenario.price_changes[i].resource, scenario.price_changes[i].amount)) {
                city_message_post(1, MESSAGE_PRICE_INCREASED, scenario.price_changes[i].amount, scenario.price_changes[i].resource);
            }
        } else {
            if (trade_price_change(scenario.price_changes[i].resource, -scenario.price_changes[i].amount)) {
                city_message_post(1, MESSAGE_PRICE_DECREASED, scenario.price_changes[i].amount, scenario.price_changes[i].resource);
            }
        }
    }
}

static void increase_resource_buy_limit(struct empire_object_t *trade_city, resource_type resource)
{
    switch (trade_city->resource_buy_limit[resource]) {
        case 0:
            trade_city->resource_buy_limit[resource] = 15;
            city_message_post(1, MESSAGE_INCREASED_TRADING, trade_city->id, resource);
            break;
        case 15:
            trade_city->resource_buy_limit[resource] = 25;
            city_message_post(1, MESSAGE_INCREASED_TRADING, trade_city->id, resource);
            break;
        case 25:
            trade_city->resource_buy_limit[resource] = 40;
            city_message_post(1, MESSAGE_INCREASED_TRADING, trade_city->id, resource);
            break;
        default:
            break;
    }
}

static void decrease_resource_buy_limit(struct empire_object_t *trade_city, resource_type resource)
{
    switch (trade_city->resource_buy_limit[resource]) {
        case 40:
            trade_city->resource_buy_limit[resource] = 25;
            city_message_post(1, MESSAGE_DECREASED_TRADING, trade_city->id, resource);
            break;
        case 25:
            trade_city->resource_buy_limit[resource] = 15;
            city_message_post(1, MESSAGE_DECREASED_TRADING, trade_city->id, resource);
            break;
        case 15:
            trade_city->resource_buy_limit[resource] = 0;
            city_message_post(1, MESSAGE_TRADE_STOPPED, trade_city->id, resource);
            break;
        default:
            break;
    }
}

static void increase_resource_sell_limit(struct empire_object_t *trade_city, resource_type resource)
{
    switch (trade_city->resource_sell_limit[resource]) {
        case 0:
            trade_city->resource_sell_limit[resource] = 15;
            city_message_post(1, MESSAGE_INCREASED_TRADING, trade_city->id, resource);
            break;
        case 15:
            trade_city->resource_sell_limit[resource] = 25;
            city_message_post(1, MESSAGE_INCREASED_TRADING, trade_city->id, resource);
            break;
        case 25:
            trade_city->resource_sell_limit[resource] = 40;
            city_message_post(1, MESSAGE_INCREASED_TRADING, trade_city->id, resource);
            break;
        default:
            break;
    }
}

static void decrease_resource_sell_limit(struct empire_object_t *trade_city, resource_type resource)
{
    switch (trade_city->resource_sell_limit[resource]) {
        case 40:
            trade_city->resource_sell_limit[resource] = 25;
            city_message_post(1, MESSAGE_DECREASED_TRADING, trade_city->id, resource);
            break;
        case 25:
            trade_city->resource_sell_limit[resource] = 15;
            city_message_post(1, MESSAGE_DECREASED_TRADING, trade_city->id, resource);
            break;
        case 15:
            trade_city->resource_sell_limit[resource] = 0;
            city_message_post(1, MESSAGE_TRADE_STOPPED, trade_city->id, resource);
            break;
        default:
            break;
    }
}

void scenario_demand_change_process(void)
{
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        if (scenario.demand_changes[i].resource && scenario.demand_changes[i].trade_city_id
            && game_time_year() == scenario.demand_changes[i].year + scenario.start_year
            && game_time_month() == scenario.demand_changes[i].month) {
            struct empire_object_t *trade_city = &empire_objects[scenario.demand_changes[i].trade_city_id];
            if (trade_city->trade_route_open) {
                if (scenario.demand_changes[i].is_rise) {
                    if (trade_city->resource_buy_limit[scenario.demand_changes[i].resource]) {
                        increase_resource_buy_limit(trade_city, scenario.demand_changes[i].resource);
                    } else if (trade_city->resource_sell_limit[scenario.demand_changes[i].resource]) {
                        increase_resource_sell_limit(trade_city, scenario.demand_changes[i].resource);
                    }
                } else {
                    if (trade_city->resource_buy_limit[scenario.demand_changes[i].resource]) {
                        decrease_resource_buy_limit(trade_city, scenario.demand_changes[i].resource);
                    }
                    if (trade_city->resource_sell_limit[scenario.demand_changes[i].resource]) {
                        decrease_resource_sell_limit(trade_city, scenario.demand_changes[i].resource);
                    }
                }
            }
        }
    }
}