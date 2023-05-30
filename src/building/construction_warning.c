#include "construction_warning.h"

#include "building/building.h"
#include "building/count.h"
#include "city/constants.h"
#include "city/data_private.h"
#include "city/population.h"
#include "city/resource.h"
#include "city/warning.h"
#include "core/calc.h"
#include "empire/object.h"
#include "map/grid.h"
#include "map/road_access.h"
#include "map/terrain.h"
#include "scenario/data.h"

static int has_warning = 0;

void building_construction_warning_reset(void)
{
    has_warning = 0;
}

static void show(warning_type warning)
{
    city_warning_show(warning);
    has_warning = 1;
}

static void check_road_access(int type, int x, int y, int size)
{
    switch (type) {
        case BUILDING_SMALL_STATUE:
        case BUILDING_MEDIUM_STATUE:
        case BUILDING_LARGE_STATUE:
        case BUILDING_FOUNTAIN:
        case BUILDING_WELL:
        case BUILDING_RESERVOIR:
        case BUILDING_GATEHOUSE:
        case BUILDING_TRIUMPHAL_ARCH:
        case BUILDING_HOUSE_VACANT_LOT:
        case BUILDING_FORT_LEGIONARIES:
        case BUILDING_FORT_JAVELIN:
        case BUILDING_FORT_MOUNTED:
            return;
    }

    int has_road = 0;
    if (map_has_road_access(x, y, size, 0)) {
        has_road = 1;
    } else if (type == BUILDING_WAREHOUSE && map_has_road_access(x, y, size, 0)) {
        has_road = 1;
    } else if (type == BUILDING_HIPPODROME && map_has_road_access_hippodrome(x, y, 0)) {
        has_road = 1;
    }
    if (!has_road) {
        show(WARNING_ROAD_ACCESS_NEEDED);
    }
}

static void check_water(int type, int x, int y)
{
    if (!has_warning) {
        if (type == BUILDING_FOUNTAIN || type == BUILDING_BATHHOUSE) {
            int grid_offset = map_grid_offset(x, y);
            int has_water = 0;
            if (map_terrain_is(grid_offset, TERRAIN_RESERVOIR_RANGE)) {
                has_water = 1;
            } else if (type == BUILDING_BATHHOUSE) {
                if (map_terrain_is(grid_offset + map_grid_delta(1, 0), TERRAIN_RESERVOIR_RANGE) ||
                    map_terrain_is(grid_offset + map_grid_delta(0, 1), TERRAIN_RESERVOIR_RANGE) ||
                    map_terrain_is(grid_offset + map_grid_delta(1, 1), TERRAIN_RESERVOIR_RANGE)) {
                    has_water = 1;
                }
            }
            if (!has_water) {
                show(WARNING_WATER_PIPE_ACCESS_NEEDED);
            }
        }
    }
}

static void check_workers(int type)
{
    if (!has_warning && type != BUILDING_WELL && !building_is_fort(type)) {
        if (building_properties[type].n_laborers > 0 && city_data.labor.workers_needed >= 10) {
            show(WARNING_WORKERS_NEEDED);
        }
    }
}

static void check_market(int type)
{
    if (!has_warning && type == BUILDING_GRANARY) {
        if (building_count_active(BUILDING_MARKET) <= 0) {
            show(WARNING_BUILD_MARKET);
        }
    }
}

static void check_barracks(int type)
{
    if (!has_warning) {
        if (building_is_fort(type) && building_count_active(BUILDING_BARRACKS) <= 0) {
            show(WARNING_BUILD_BARRACKS);
        }
    }
}

static void check_weapons_access(int type)
{
    if (!has_warning && type == BUILDING_BARRACKS) {
        if (city_data.resource.stored_in_warehouses[RESOURCE_WEAPONS] <= 0) {
            show(WARNING_WEAPONS_NEEDED);
        }
    }
}

static int map_terrain_is_adjacent_to_wall(int x, int y, int size)
{
    int base_offset = map_grid_offset(x, y);
    for (const int *tile_delta = map_grid_adjacent_offsets(size); *tile_delta; tile_delta++) {
        if (map_terrain_is(base_offset + *tile_delta, TERRAIN_WALL)) {
            return 1;
        }
    }
    return 0;
}

static void check_wall(int type, int x, int y, int size)
{
    if (!has_warning && type == BUILDING_TOWER) {
        if (!map_terrain_is_adjacent_to_wall(x, y, size)) {
            show(WARNING_SENTRIES_NEED_WALL);
        }
    }
}

static void check_actor_access(int type)
{
    if (!has_warning && type == BUILDING_THEATER) {
        if (building_count_active(BUILDING_ACTOR_COLONY) <= 0) {
            show(WARNING_BUILD_ACTOR_COLONY);
        }
    }
}

static void check_gladiator_access(int type)
{
    if (!has_warning && type == BUILDING_AMPHITHEATER) {
        if (building_count_active(BUILDING_GLADIATOR_SCHOOL) <= 0) {
            show(WARNING_BUILD_GLADIATOR_SCHOOL);
        }
    }
}

static void check_lion_access(int type)
{
    if (!has_warning && type == BUILDING_COLOSSEUM) {
        if (building_count_active(BUILDING_LION_HOUSE) <= 0) {
            show(WARNING_BUILD_LION_HOUSE);
        }
    }
}

static void check_charioteer_access(int type)
{
    if (!has_warning && type == BUILDING_HIPPODROME) {
        if (building_count_active(BUILDING_CHARIOT_MAKER) <= 0) {
            show(WARNING_BUILD_CHARIOT_MAKER);
        }
    }
}

static int empire_offers_resource(int resource)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use
            && empire_objects[i].city_type == EMPIRE_CITY_TRADE
            && empire_objects[i].resource_sell_limit[resource]) {
            return 1;
        }
    }
    return 0;
}

void building_construction_warning_check_all(int type, int x, int y, int size)
{
    building_construction_warning_check_food_stocks(type);
    check_workers(type);
    check_market(type);
    check_actor_access(type);
    check_gladiator_access(type);
    check_lion_access(type);
    check_charioteer_access(type);

    check_barracks(type);
    check_weapons_access(type);

    check_wall(type, x, y, size);
    check_water(type, x, y);
    check_road_access(type, x, y, size);

    // check raw resources availability
    int raw_resource = 0;
    int finished_good = 0;
    warning_type warning_resource_needed = 0;
    warning_type warning_resource_building = 0;
    switch (type) {
        case BUILDING_OIL_WORKSHOP:
            raw_resource = RESOURCE_OLIVES;
            finished_good = RESOURCE_OIL;
            warning_resource_needed = WARNING_OLIVES_NEEDED;
            warning_resource_building = WARNING_BUILD_OLIVE_FARM;
            break;
        case BUILDING_WINE_WORKSHOP:
            raw_resource = RESOURCE_VINES;
            finished_good = RESOURCE_WINE;
            warning_resource_needed = WARNING_VINES_NEEDED;
            warning_resource_building = WARNING_BUILD_VINES_FARM;
            break;
        case BUILDING_WEAPONS_WORKSHOP:
            raw_resource = RESOURCE_IRON;
            finished_good = RESOURCE_WEAPONS;
            warning_resource_needed = WARNING_IRON_NEEDED;
            warning_resource_building = WARNING_BUILD_IRON_MINE;
            break;
        case BUILDING_FURNITURE_WORKSHOP:
            raw_resource = RESOURCE_TIMBER;
            finished_good = RESOURCE_FURNITURE;
            warning_resource_needed = WARNING_TIMBER_NEEDED;
            warning_resource_building = WARNING_BUILD_TIMBER_YARD;
            break;
        case BUILDING_POTTERY_WORKSHOP:
            raw_resource = RESOURCE_CLAY;
            finished_good = RESOURCE_POTTERY;
            warning_resource_needed = WARNING_CLAY_NEEDED;
            warning_resource_building = WARNING_BUILD_CLAY_PIT;
            break;
        default:
            return;
    }
    if (building_count_industry_active(raw_resource) <= 0) {
        if (city_data.resource.stored_in_warehouses[finished_good] <= 0 && city_data.resource.stored_in_warehouses[raw_resource] <= 0) {
            show(warning_resource_needed);
            if (our_city_can_produce_resource(raw_resource)) {
                show(warning_resource_building);
            } else if (empire_offers_resource(raw_resource) && !resource_import_trade_route_open(raw_resource)) {
                show(WARNING_OPEN_TRADE_TO_IMPORT);
            } else if (city_data.resource.trade_status[raw_resource] != TRADE_STATUS_IMPORT) {
                show(WARNING_TRADE_IMPORT_RESOURCE);
            }
        }
    }
}

void building_construction_warning_check_food_stocks(int type)
{
    if (!has_warning && type == BUILDING_HOUSE_VACANT_LOT) {
        if (city_data.population.population >= 200 && !scenario.rome_supplies_wheat) {
            if (city_resource_food_percentage_produced() <= 95) {
                show(WARNING_MORE_FOOD_NEEDED);
            }
        }
    }
}

void building_construction_warning_check_reservoir(int type)
{
    if (!has_warning && type == BUILDING_RESERVOIR) {
        if (building_count_active(BUILDING_RESERVOIR)) {
            show(WARNING_CONNECT_TO_RESERVOIR);
        } else {
            show(WARNING_PLACE_RESERVOIR_NEXT_TO_WATER);
        }
    }
}
