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
#include "core/random.h"
#include "empire/object.h"
#include "empire/trade_prices.h"
#include "empire/trade_route.h"
#include "empire/type.h"
#include "figure/figure.h"
#include "figure/formation.h"
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

#include <string.h>

#define MAX_INVASION_WARNINGS 101

static struct {
    int state;
    int duration;
    int max_duration;
    int delay;
    int max_delay;
    struct {
        int x;
        int y;
    } expand[4];
} data_earthquake;

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

static const int ENEMY_ID_TO_ENEMY_TYPE[20] = {
    ENEMY_0_BARBARIAN,
    ENEMY_7_ETRUSCAN,
    ENEMY_7_ETRUSCAN,
    ENEMY_10_CARTHAGINIAN,
    ENEMY_8_GREEK,
    ENEMY_8_GREEK,
    ENEMY_9_EGYPTIAN,
    ENEMY_5_PERGAMUM,
    ENEMY_6_SELEUCID,
    ENEMY_3_CELT,
    ENEMY_3_CELT,
    ENEMY_3_CELT,
    ENEMY_2_GAUL,
    ENEMY_2_GAUL,
    ENEMY_4_GOTH,
    ENEMY_4_GOTH,
    ENEMY_4_GOTH,
    ENEMY_6_SELEUCID,
    ENEMY_1_NUMIDIAN,
    ENEMY_6_SELEUCID
};

static const struct {
    int pct_type1;
    int pct_type2;
    int pct_type3;
    int figure_types[3];
    int formation_layout;
} ENEMY_PROPERTIES[12] = {
    {100, 0, 0, {FIGURE_ENEMY49_FAST_SWORD, 0, 0}, FORMATION_ENEMY_MOB}, // barbarian
    {40, 60, 0, {FIGURE_ENEMY49_FAST_SWORD, FIGURE_ENEMY51_SPEAR, 0}, FORMATION_ENEMY_MOB}, // numidian
    {50, 50, 0, {FIGURE_ENEMY50_SWORD, FIGURE_ENEMY53_AXE, 0}, FORMATION_ENEMY_MOB}, // gaul
    {80, 20, 0, {FIGURE_ENEMY50_SWORD, FIGURE_ENEMY48_CHARIOT, 0}, FORMATION_ENEMY_MOB}, // celt
    {50, 50, 0, {FIGURE_ENEMY49_FAST_SWORD, FIGURE_ENEMY52_MOUNTED_ARCHER, 0}, FORMATION_ENEMY_MOB}, // goth
    {30, 70, 0, {FIGURE_ENEMY44_SWORD, FIGURE_ENEMY43_SPEAR, 0}, FORMATION_COLUMN}, // pergamum
    {50, 50, 0, {FIGURE_ENEMY44_SWORD, FIGURE_ENEMY43_SPEAR, 0}, FORMATION_ENEMY_DOUBLE_LINE}, // seleucid
    {50, 50, 0, {FIGURE_ENEMY45_SWORD, FIGURE_ENEMY43_SPEAR, 0}, FORMATION_ENEMY_DOUBLE_LINE}, // etruscan
    {80, 20, 0, {FIGURE_ENEMY45_SWORD, FIGURE_ENEMY43_SPEAR, 0}, FORMATION_ENEMY_DOUBLE_LINE}, // greek
    {80, 20, 0, {FIGURE_ENEMY44_SWORD, FIGURE_ENEMY46_CAMEL, 0}, FORMATION_ENEMY_WIDE_COLUMN}, // egyptian
    {90, 10, 0, {FIGURE_ENEMY45_SWORD, FIGURE_ENEMY47_ELEPHANT, 0}, FORMATION_ENEMY_WIDE_COLUMN}, // carthaginian
    {100, 0, 0, {FIGURE_ENEMY_CAESAR_LEGIONARY, 0, 0}, FORMATION_COLUMN} // caesar
};

typedef struct {
    int in_use;
    int handled;
    int invasion_path_id;
    int warning_years;
    int x;
    int y;
    int image_id;
    int empire_object_id;
    int year_notified;
    int month_notified;
    int months_to_go;
    int invasion_id;
} invasion_warning;

static struct {
    int last_internal_invasion_id;
    invasion_warning warnings[MAX_INVASION_WARNINGS];
} data_invasion;

void scenario_empire_process_expansion(void)
{
    if (scenario.empire.is_expanded || scenario.empire.expansion_year <= 0) {
        return;
    }
    if (game_time_year() < scenario.empire.expansion_year + scenario.start_year) {
        return;
    }

    empire_object_set_expanded();

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
        case BUILDING_DRAGGABLE_RESERVOIR:
            return scenario.allowed_buildings[ALLOWED_BUILDING_DRAGGABLE_RESERVOIR];
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

void scenario_earthquake_init(void)
{
    switch (scenario.earthquake.severity) {
        default:
            data_earthquake.max_duration = 0;
            data_earthquake.max_delay = 0;
            break;
        case EARTHQUAKE_SMALL:
            data_earthquake.max_duration = 25 + (random_byte() & 0x1f);
            data_earthquake.max_delay = 10;
            break;
        case EARTHQUAKE_MEDIUM:
            data_earthquake.max_duration = 100 + (random_byte() & 0x3f);
            data_earthquake.max_delay = 8;
            break;
        case EARTHQUAKE_LARGE:
            data_earthquake.max_duration = 250 + random_byte();
            data_earthquake.max_delay = 6;
            break;
    }
    data_earthquake.state = EVENT_NOT_STARTED;
    for (int i = 0; i < 4; i++) {
        data_earthquake.expand[i].x = scenario.earthquake_point.x;
        data_earthquake.expand[i].y = scenario.earthquake_point.y;
    }
}

static void advance_earthquake_to_tile(int x, int y)
{
    int grid_offset = map_grid_offset(x, y);
    int building_id = map_building_at(grid_offset);
    if (building_id) {
        building_destroy_by_fire(building_get(building_id));
        sound_effect_play(SOUND_EFFECT_EXPLOSION);
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

    figure_create_explosion_cloud(x, y, 1);
}

void scenario_earthquake_process(void)
{
    if (scenario.earthquake.severity == EARTHQUAKE_NONE
        || scenario.earthquake_point.x == -1 || scenario.earthquake_point.y == -1) {
        return;
    }
    if (data_earthquake.state == EVENT_NOT_STARTED) {
        if (scenario.start_year + scenario.earthquake.year == game_time_year() && scenario.earthquake.month == game_time_month()) {
            data_earthquake.state = EVENT_IN_PROGRESS;
            data_earthquake.duration = 0;
            data_earthquake.delay = 0;
            advance_earthquake_to_tile(data_earthquake.expand[0].x, data_earthquake.expand[0].y);
            city_message_post(1, MESSAGE_EARTHQUAKE, 0,
                map_grid_offset(data_earthquake.expand[0].x, data_earthquake.expand[0].y));
        }
    } else if (data_earthquake.state == EVENT_IN_PROGRESS) {
        data_earthquake.delay++;
        if (data_earthquake.delay >= data_earthquake.max_delay) {
            data_earthquake.delay = 0;
            data_earthquake.duration++;
            if (data_earthquake.duration >= data_earthquake.max_duration) {
                data_earthquake.state = EVENT_FINISHED;
            }
            int dx, dy, index;
            switch (random_byte() & 0xf) {
                case 0: index = 0; dx = 0; dy = -1; break;
                case 1: index = 1; dx = 1; dy = 0; break;
                case 2: index = 2; dx = 0; dy = 1; break;
                case 3: index = 3; dx = -1; dy = 0; break;
                case 4: index = 0; dx = 0; dy = -1; break;
                case 5: index = 0; dx = -1; dy = 0; break;
                case 6: index = 0; dx = 1; dy = 0; break;
                case 7: index = 1; dx = 1; dy = 0; break;
                case 8: index = 1; dx = 0; dy = -1; break;
                case 9: index = 1; dx = 0; dy = 1; break;
                case 10: index = 2; dx = 0; dy = 1; break;
                case 11: index = 2; dx = -1; dy = 0; break;
                case 12: index = 2; dx = 1; dy = 0; break;
                case 13: index = 3; dx = -1; dy = 0; break;
                case 14: index = 3; dx = 0; dy = -1; break;
                case 15: index = 3; dx = 0; dy = 1; break;
                default: return;
            }
            int x = calc_bound(data_earthquake.expand[index].x + dx, 0, scenario.map.width - 1);
            int y = calc_bound(data_earthquake.expand[index].y + dy, 0, scenario.map.height - 1);
            if (!map_terrain_is(map_grid_offset(x, y), TERRAIN_ELEVATION | TERRAIN_ROCK | TERRAIN_WATER)) {
                data_earthquake.expand[index].x = x;
                data_earthquake.expand[index].y = y;
                advance_earthquake_to_tile(x, y);
            }
        }
    }
}

int scenario_earthquake_is_in_progress(void)
{
    return data_earthquake.state == EVENT_IN_PROGRESS;
}

void scenario_earthquake_save_state(buffer *buf)
{
    buffer_write_i32(buf, data_earthquake.state);
    buffer_write_i32(buf, data_earthquake.duration);
    buffer_write_i32(buf, data_earthquake.max_duration);
    buffer_write_i32(buf, data_earthquake.max_delay);
    buffer_write_i32(buf, data_earthquake.delay);
    for (int i = 0; i < 4; i++) {
        buffer_write_i32(buf, data_earthquake.expand[i].x);
        buffer_write_i32(buf, data_earthquake.expand[i].y);
    }
}

void scenario_earthquake_load_state(buffer *buf)
{
    data_earthquake.state = buffer_read_i32(buf);
    data_earthquake.duration = buffer_read_i32(buf);
    data_earthquake.max_duration = buffer_read_i32(buf);
    data_earthquake.max_delay = buffer_read_i32(buf);
    data_earthquake.delay = buffer_read_i32(buf);
    for (int i = 0; i < 4; i++) {
        data_earthquake.expand[i].x = buffer_read_i32(buf);
        data_earthquake.expand[i].y = buffer_read_i32(buf);
    }
}

static void raise_wages(void)
{
    if (scenario.random_events.raise_wages) {
        if (city_labor_raise_wages_rome()) {
            city_message_post(1, MESSAGE_ROME_RAISES_WAGES, 0, 0);
        }
    }
}

static void lower_wages(void)
{
    if (scenario.random_events.lower_wages) {
        if (city_labor_lower_wages_rome()) {
            city_message_post(1, MESSAGE_ROME_LOWERS_WAGES, 0, 0);
        }
    }
}

static void disrupt_land_trade(void)
{
    if (scenario.random_events.land_trade_problem) {
        if (city_trade_has_land_trade_route()) {
            city_trade_start_land_trade_problems(48);
            if (scenario.climate == CLIMATE_DESERT) {
                city_message_post(1, MESSAGE_LAND_TRADE_DISRUPTED_SANDSTORMS, 0, 0);
            } else {
                city_message_post(1, MESSAGE_LAND_TRADE_DISRUPTED_LANDSLIDES, 0, 0);
            }
        }
    }
}

static void disrupt_sea_trade(void)
{
    if (scenario.random_events.sea_trade_problem) {
        if (city_trade_has_sea_trade_route()) {
            city_trade_start_sea_trade_problems(48);
            city_message_post(1, MESSAGE_SEA_TRADE_DISRUPTED, 0, 0);
        }
    }
}

static void contaminate_water(void)
{
    if (scenario.random_events.contaminated_water) {
        if (city_population() > 200) {
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
}

void scenario_random_event_process(void)
{
    int event = RANDOM_EVENT_PROBABILITY[random_byte()];
    switch (event) {
        case EVENT_ROME_RAISES_WAGES:
            raise_wages();
            break;
        case EVENT_ROME_LOWERS_WAGES:
            lower_wages();
            break;
        case EVENT_LAND_TRADE_DISRUPTED:
            disrupt_land_trade();
            break;
        case EVENT_LAND_SEA_DISRUPTED:
            disrupt_sea_trade();
            break;
        case EVENT_CONTAMINATED_WATER:
            contaminate_water();
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

void scenario_invasion_clear(void)
{
    memset(data_invasion.warnings, 0, MAX_INVASION_WARNINGS * sizeof(invasion_warning));
}

void scenario_invasion_init(void)
{
    scenario_invasion_clear();
    int path_current = 1;
    int path_max = empire_object_get_max_invasion_path();
    if (path_max == 0) {
        return;
    }
    invasion_warning *warning = &data_invasion.warnings[1];
    for (int i = 0; i < MAX_INVASIONS; i++) {
        random_generate_next();
        if (!scenario.invasions[i].type) {
            continue;
        }
        if (scenario.invasions[i].type == INVASION_TYPE_LOCAL_UPRISING ||
            scenario.invasions[i].type == INVASION_TYPE_DISTANT_BATTLE) {
            continue;
        }
        for (int year = 1; year < 8; year++) {
            const empire_object *obj = empire_object_get_battle_icon(path_current, year);
            if (!obj) {
                continue;
            }
            // don't overlap messages if enemy is near enough from the beginning
            if (scenario.invasions[i].year) {
                if (obj->invasion_years > scenario.invasions[i].year) {
                    continue;
                }
            } else { // handles first year (year 0) invasions
                if (obj->invasion_path_id == (warning - 1)->invasion_path_id) {
                    continue;
                }
            }
            warning->in_use = 1;
            warning->invasion_path_id = obj->invasion_path_id;
            warning->warning_years = obj->invasion_years;
            warning->x = obj->x;
            warning->y = obj->y;
            warning->image_id = obj->image_id;
            warning->invasion_id = i;
            warning->empire_object_id = obj->id;
            warning->month_notified = 0;
            warning->year_notified = 0;
            warning->months_to_go = 12 * scenario.invasions[i].year;
            warning->months_to_go += scenario.invasions[i].month;
            warning->months_to_go -= 12 * year;
            ++warning;
        }
        path_current++;
        if (path_current > path_max) {
            path_current = 1;
        }
    }
}

int scenario_invasion_exists_upcoming(void)
{
    for (int i = 0; i < MAX_INVASION_WARNINGS; i++) {
        if (data_invasion.warnings[i].in_use && data_invasion.warnings[i].handled) {
            return 1;
        }
    }
    return 0;
}

void scenario_invasion_foreach_warning(void (*callback)(int x, int y, int image_id))
{
    for (int i = 0; i < MAX_INVASION_WARNINGS; i++) {
        if (data_invasion.warnings[i].in_use && data_invasion.warnings[i].handled) {
            callback(data_invasion.warnings[i].x, data_invasion.warnings[i].y, data_invasion.warnings[i].image_id);
        }
    }
}

static void determine_formations(int num_soldiers, int *num_formations, int soldiers_per_formation[])
{
    if (num_soldiers > 0) {
        if (num_soldiers <= 16) {
            *num_formations = 1;
            soldiers_per_formation[0] = num_soldiers;
        } else if (num_soldiers <= 32) {
            *num_formations = 2;
            soldiers_per_formation[1] = num_soldiers / 2;
            soldiers_per_formation[0] = num_soldiers - num_soldiers / 2;
        } else {
            *num_formations = 3;
            soldiers_per_formation[2] = num_soldiers / 3;
            soldiers_per_formation[1] = num_soldiers / 3;
            soldiers_per_formation[0] = num_soldiers - 2 * (num_soldiers / 3);
        }
    }
}

static int start_invasion(int enemy_type, int amount, int invasion_point, int attack_type, int invasion_id)
{
    if (amount <= 0) {
        return -1;
    }
    int formations_per_type[3];
    int soldiers_per_formation[3][4];
    int x, y;
    int orientation;

    if (amount > 200) {
        amount = 200;
    }
    data_invasion.last_internal_invasion_id++;
    if (data_invasion.last_internal_invasion_id > 32000) {
        data_invasion.last_internal_invasion_id = 1;
    }
    // calculate soldiers per type
    int num_type1 = calc_adjust_with_percentage(amount, ENEMY_PROPERTIES[enemy_type].pct_type1);
    int num_type2 = calc_adjust_with_percentage(amount, ENEMY_PROPERTIES[enemy_type].pct_type2);
    int num_type3 = calc_adjust_with_percentage(amount, ENEMY_PROPERTIES[enemy_type].pct_type3);
    num_type1 += amount - (num_type1 + num_type2 + num_type3); // assign leftovers to type1

    for (int t = 0; t < 3; t++) {
        formations_per_type[t] = 0;
        for (int f = 0; f < 4; f++) {
            soldiers_per_formation[t][f] = 0;
        }
    }

    // calculate number of formations
    determine_formations(num_type1, &formations_per_type[0], soldiers_per_formation[0]);
    determine_formations(num_type2, &formations_per_type[1], soldiers_per_formation[1]);
    determine_formations(num_type3, &formations_per_type[2], soldiers_per_formation[2]);

    // determine invasion point
    if (enemy_type == ENEMY_11_CAESAR) {
        x = scenario.entry_point.x;
        y = scenario.entry_point.y;
    } else {
        int num_points = 0;
        for (int i = 0; i < MAX_INVASION_POINTS; i++) {
            if (scenario.invasion_points[i].x != -1) {
                num_points++;
            }
        }
        if (invasion_point == MAX_INVASION_POINTS) { // random
            if (num_points <= 2) {
                invasion_point = random_byte() & 1;
            } else if (num_points <= 4) {
                invasion_point = random_byte() & 3;
            } else {
                invasion_point = random_byte() & 7;
            }
        }
        if (num_points > 0) {
            while (scenario.invasion_points[invasion_point].x == -1) {
                invasion_point++;
                if (invasion_point >= MAX_INVASION_POINTS) {
                    invasion_point = 0;
                }
            }
        }
        x = scenario.invasion_points[invasion_point].x;
        y = scenario.invasion_points[invasion_point].y;
    }
    if (x == -1 || y == -1) {
        x = scenario.exit_point.x;
        y = scenario.exit_point.y;
    }
    // determine orientation
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
    // spawn the lot!
    int seq = 0;
    for (int type = 0; type < 3; type++) {
        if (formations_per_type[type] <= 0) {
            continue;
        }
        int figure_type = ENEMY_PROPERTIES[enemy_type].figure_types[type];
        for (int i = 0; i < formations_per_type[type]; i++) {
            int formation_id = formation_create_enemy(
                figure_type, x, y, ENEMY_PROPERTIES[enemy_type].formation_layout, orientation,
                enemy_type, attack_type, invasion_id, data_invasion.last_internal_invasion_id
            );
            if (formation_id <= 0) {
                continue;
            }
            for (int fig = 0; fig < soldiers_per_formation[type][i]; fig++) {
                figure *f = figure_create(figure_type, x, y, orientation);
                f->faction_id = 0;
                f->is_friendly = 0;
                f->action_state = FIGURE_ACTION_151_ENEMY_INITIAL;
                f->wait_ticks = 200 * seq + 10 * fig + 10;
                f->formation_id = formation_id;
                f->name = figure_name_get(figure_type, enemy_type);
                f->is_ghost = 1;
            }
            seq++;
        }
    }
    return grid_offset;
}

void scenario_invasion_process(void)
{
    int enemy_id = scenario.enemy_id;
    for (int i = 0; i < MAX_INVASION_WARNINGS; i++) {
        if (!data_invasion.warnings[i].in_use) {
            continue;
        }
        // update warnings
        invasion_warning *warning = &data_invasion.warnings[i];
        warning->months_to_go--;
        if (warning->months_to_go <= 0) {
            if (warning->handled != 1) {
                warning->handled = 1;
                warning->year_notified = game_time_year();
                warning->month_notified = game_time_month();
                if (warning->warning_years > 2) {
                    city_message_post(0, MESSAGE_DISTANT_BATTLE, 0, 0);
                } else if (warning->warning_years > 1) {
                    city_message_post(0, MESSAGE_ENEMIES_CLOSING, 0, 0);
                } else {
                    city_message_post(0, MESSAGE_ENEMIES_AT_THE_DOOR, 0, 0);
                }
            }
        }
        if (game_time_year() >= scenario.start_year + scenario.invasions[warning->invasion_id].year &&
            game_time_month() >= scenario.invasions[warning->invasion_id].month) {
            // invasion attack time has passed
            warning->in_use = 0;
            if (warning->warning_years > 1) {
                continue;
            }
            // enemy invasions
            if (scenario.invasions[warning->invasion_id].type == INVASION_TYPE_ENEMY_ARMY) {
                int grid_offset = start_invasion(
                    ENEMY_ID_TO_ENEMY_TYPE[enemy_id],
                    scenario.invasions[warning->invasion_id].amount,
                    scenario.invasions[warning->invasion_id].from,
                    scenario.invasions[warning->invasion_id].target_type,
                    warning->invasion_id);
                if (grid_offset > 0) {
                    if (ENEMY_ID_TO_ENEMY_TYPE[enemy_id] > 4) {
                        city_message_post(1, MESSAGE_ENEMY_ARMY_ATTACK, data_invasion.last_internal_invasion_id, grid_offset);
                    } else {
                        city_message_post(1, MESSAGE_BARBARIAN_ATTACK, data_invasion.last_internal_invasion_id, grid_offset);
                    }
                }
            }
            // editor scheduled invasion by Caesar
            if (scenario.invasions[warning->invasion_id].type == INVASION_TYPE_CAESAR) {
                int grid_offset = start_invasion(
                    ENEMY_11_CAESAR,
                    scenario.invasions[warning->invasion_id].amount,
                    scenario.invasions[warning->invasion_id].from,
                    scenario.invasions[warning->invasion_id].target_type,
                    warning->invasion_id);
                if (grid_offset > 0) {
                    city_message_post(1, MESSAGE_CAESAR_ARMY_ATTACK, data_invasion.last_internal_invasion_id, grid_offset);
                }
                city_data.emperor.invasion.from_editor = 1;
                city_data.emperor.invasion.size = scenario.invasions[warning->invasion_id].amount;
            }
        }
    }
    // local uprisings
    for (int i = 0; i < MAX_INVASIONS; i++) {
        if (scenario.invasions[i].type == INVASION_TYPE_LOCAL_UPRISING) {
            if (game_time_year() == scenario.start_year + scenario.invasions[i].year &&
                game_time_month() == scenario.invasions[i].month) {
                int grid_offset = start_invasion(
                    ENEMY_0_BARBARIAN,
                    scenario.invasions[i].amount,
                    scenario.invasions[i].from,
                    scenario.invasions[i].target_type,
                    i);
                if (grid_offset > 0) {
                    city_message_post(1, MESSAGE_LOCAL_UPRISING, data_invasion.last_internal_invasion_id, grid_offset);
                }
            }
        }
    }
}

void scenario_invasion_start_from_mars(void)
{
    int amount = 32;
    int grid_offset = start_invasion(ENEMY_0_BARBARIAN, amount, 8, FORMATION_ATTACK_FOOD_CHAIN, 23);
    if (grid_offset) {
        city_message_post(1, MESSAGE_LOCAL_UPRISING_MARS, data_invasion.last_internal_invasion_id, grid_offset);
    }
}

int scenario_invasion_start_from_caesar(int size)
{
    int grid_offset = start_invasion(ENEMY_11_CAESAR, size, 0, FORMATION_ATTACK_BEST_BUILDINGS, 24);
    if (grid_offset > 0) {
        city_message_post(1, MESSAGE_CAESAR_ARMY_ATTACK, data_invasion.last_internal_invasion_id, grid_offset);
        return 1;
    }
    return 0;
}

void scenario_invasion_start_from_cheat(void)
{
    int enemy_id = scenario.enemy_id;
    int grid_offset = start_invasion(ENEMY_ID_TO_ENEMY_TYPE[enemy_id], 200, 8, FORMATION_ATTACK_FOOD_CHAIN, 23);
    if (grid_offset) {
        if (ENEMY_ID_TO_ENEMY_TYPE[enemy_id] > 4) {
            city_message_post(1, MESSAGE_ENEMY_ARMY_ATTACK, data_invasion.last_internal_invasion_id, grid_offset);
        } else {
            city_message_post(1, MESSAGE_BARBARIAN_ATTACK, data_invasion.last_internal_invasion_id, grid_offset);
        }
    }
}

void scenario_invasion_save_state(buffer *invasion_id, buffer *warnings)
{
    buffer_write_u16(invasion_id, data_invasion.last_internal_invasion_id);

    for (int i = 0; i < MAX_INVASION_WARNINGS; i++) {
        const invasion_warning *w = &data_invasion.warnings[i];
        buffer_write_u8(warnings, w->in_use);
        buffer_write_u8(warnings, w->handled);
        buffer_write_u8(warnings, w->invasion_path_id);
        buffer_write_u8(warnings, w->warning_years);
        buffer_write_i16(warnings, w->x);
        buffer_write_i16(warnings, w->y);
        buffer_write_i16(warnings, w->image_id);
        buffer_write_i16(warnings, w->empire_object_id);
        buffer_write_i16(warnings, w->month_notified);
        buffer_write_i16(warnings, w->year_notified);
        buffer_write_i32(warnings, w->months_to_go);
        buffer_write_u8(warnings, w->invasion_id);
    }
}

void scenario_invasion_load_state(buffer *invasion_id, buffer *warnings)
{
    data_invasion.last_internal_invasion_id = buffer_read_u16(invasion_id);

    for (int i = 0; i < MAX_INVASION_WARNINGS; i++) {
        invasion_warning *w = &data_invasion.warnings[i];
        w->in_use = buffer_read_u8(warnings);
        w->handled = buffer_read_u8(warnings);
        w->invasion_path_id = buffer_read_u8(warnings);
        w->warning_years = buffer_read_u8(warnings);
        w->x = buffer_read_i16(warnings);
        w->y = buffer_read_i16(warnings);
        w->image_id = buffer_read_i16(warnings);
        w->empire_object_id = buffer_read_i16(warnings);
        w->month_notified = buffer_read_i16(warnings);
        w->year_notified = buffer_read_i16(warnings);
        w->months_to_go = buffer_read_i32(warnings);
        w->invasion_id = buffer_read_u8(warnings);
    }
}

void scenario_distant_battle_process(void)
{
    for (int i = 0; i < MAX_INVASIONS; i++) {
        if (scenario.invasions[i].type == INVASION_TYPE_DISTANT_BATTLE &&
            game_time_year() == scenario.invasions[i].year + scenario.start_year &&
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
