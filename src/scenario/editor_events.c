#include "editor_events.h"

#include "building/count.h"
#include "building/destruction.h"
#include "building/warehouse.h"
#include "city/data.h"
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

static const struct {
    int pct_type[MAX_ENEMY_TYPES_PER_ARMY];
    int figure_types[MAX_ENEMY_TYPES_PER_ARMY];
    int formation_layout;
    int enemy_img_group;
} ENEMY_PROPERTIES[] = {
    {{100, 0, 0}, {FIGURE_ENEMY_BARBARIAN_SWORDSMAN, 0, 0}, FORMATION_ENEMY_MOB, ENEMY_IMG_TYPE_BARBARIAN}, // barbarian
    {{90, 10, 0}, {FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN, FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT, 0}, FORMATION_ENEMY_WIDE_COLUMN, ENEMY_IMG_TYPE_CARTHAGINIAN}, // carthaginian
    {{80, 20, 0}, {FIGURE_ENEMY_BRITON_SWORDSMAN, FIGURE_ENEMY_BRITON_CHARIOT, 0}, FORMATION_ENEMY_MOB, ENEMY_IMG_TYPE_CELT}, // briton
    {{80, 20, 0}, {FIGURE_ENEMY_CELT_SWORDSMAN, FIGURE_ENEMY_CELT_CHARIOT, 0}, FORMATION_ENEMY_MOB, ENEMY_IMG_TYPE_CELT}, // celt
    {{80, 20, 0}, {FIGURE_ENEMY_PICT_SWORDSMAN, FIGURE_ENEMY_PICT_CHARIOT, 0}, FORMATION_ENEMY_MOB, ENEMY_IMG_TYPE_CELT}, // pict
    {{80, 20, 0}, {FIGURE_ENEMY_EGYPTIAN_SWORDSMAN, FIGURE_ENEMY_EGYPTIAN_CAMEL, 0}, FORMATION_ENEMY_WIDE_COLUMN, ENEMY_IMG_TYPE_EGYPTIAN}, // egyptian
    {{50, 50, 0}, {FIGURE_ENEMY_ETRUSCAN_SWORDSMAN, FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER, 0}, FORMATION_DOUBLE_LINE_1, ENEMY_IMG_TYPE_ETRUSCAN}, // etruscan
    {{50, 50, 0}, {FIGURE_ENEMY_SAMNITE_SWORDSMAN, FIGURE_ENEMY_SAMNITE_SPEAR_THROWER, 0}, FORMATION_DOUBLE_LINE_1, ENEMY_IMG_TYPE_ETRUSCAN}, // samnite
    {{50, 50, 0}, {FIGURE_ENEMY_GAUL_SWORDSMAN, FIGURE_ENEMY_GAUL_AXEMAN, 0}, FORMATION_ENEMY_MOB, ENEMY_IMG_TYPE_GAUL}, // gaul
    {{50, 50, 0}, {FIGURE_ENEMY_HELVETIUS_SWORDSMAN, FIGURE_ENEMY_HELVETIUS_AXEMAN, 0}, FORMATION_ENEMY_MOB, ENEMY_IMG_TYPE_GAUL}, // helvetius
    {{50, 50, 0}, {FIGURE_ENEMY_HUN_SWORDSMAN, FIGURE_ENEMY_HUN_MOUNTED_ARCHER, 0}, FORMATION_ENEMY_MOB, ENEMY_IMG_TYPE_GOTH}, // hun
    {{50, 50, 0}, {FIGURE_ENEMY_GOTH_SWORDSMAN, FIGURE_ENEMY_GOTH_MOUNTED_ARCHER, 0}, FORMATION_ENEMY_MOB, ENEMY_IMG_TYPE_GOTH}, // goth
    {{50, 50, 0}, {FIGURE_ENEMY_VISIGOTH_SWORDSMAN, FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER, 0}, FORMATION_ENEMY_MOB, ENEMY_IMG_TYPE_GOTH}, // visigoth
    {{80, 20, 0}, {FIGURE_ENEMY_GREEK_SWORDSMAN, FIGURE_ENEMY_GREEK_SPEAR_THROWER, 0}, FORMATION_DOUBLE_LINE_1, ENEMY_IMG_TYPE_GREEK}, // greek
    {{80, 20, 0}, {FIGURE_ENEMY_MACEDONIAN_SWORDSMAN, FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER, 0}, FORMATION_DOUBLE_LINE_1, ENEMY_IMG_TYPE_GREEK}, // macedonian
    {{40, 60, 0}, {FIGURE_ENEMY_NUMIDIAN_SWORDSMAN, FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER, 0}, FORMATION_ENEMY_MOB, ENEMY_IMG_TYPE_NORTH_AFRICAN}, // numidian
    {{30, 70, 0}, {FIGURE_ENEMY_PERGAMUM_SWORDSMAN, FIGURE_ENEMY_PERGAMUM_ARCHER, 0}, FORMATION_TORTOISE, ENEMY_IMG_TYPE_PERSIAN}, // pergamum
    {{50, 50, 0}, {FIGURE_ENEMY_IBERIAN_SWORDSMAN, FIGURE_ENEMY_IBERIAN_SPEAR_THROWER, 0}, FORMATION_DOUBLE_LINE_1, ENEMY_IMG_TYPE_PHOENICIAN}, // iberian
    {{50, 50, 0}, {FIGURE_ENEMY_JUDEAN_SWORDSMAN, FIGURE_ENEMY_JUDEAN_SPEAR_THROWER, 0}, FORMATION_DOUBLE_LINE_1, ENEMY_IMG_TYPE_PHOENICIAN}, // judean
    {{50, 50, 0}, {FIGURE_ENEMY_SELEUCID_SWORDSMAN, FIGURE_ENEMY_SELEUCID_SPEAR_THROWER, 0}, FORMATION_DOUBLE_LINE_1, ENEMY_IMG_TYPE_PHOENICIAN}, // seleucid
    {{100, 0, 0}, {FIGURE_ENEMY_CAESAR_LEGIONARY, 0, 0}, FORMATION_TORTOISE, ENEMY_IMG_TYPE_CAESAR} // caesar
};

static int LAYOUT_OFFSETS_FROM_MAP_EDGE[][2] = {
    {4, 9}, // FORMATION_TORTOISE
    {2, 5}, // FORMATION_DOUBLE_LINE_1
    {2, 5}, // FORMATION_DOUBLE_LINE_2
    {0, 0}, // FORMATION_SINGLE_LINE_1 (not used by enemies)
    {0, 0}, // FORMATION_SINGLE_LINE_2 (not used by enemies)
    {0, 0}, // FORMATION_MOP_UP (not used by enemies)
    {0, 0}, // FORMATION_AT_REST (not used by enemies)
    {4, 11}, // FORMATION_ENEMY_MOB
    {5, 12}, // FORMATION_ENEMY_WIDE_COLUMN
};

static int LAYOUT_SPACING_BETWEEN_FORMATIONS[] = {
    3, // FORMATION_TORTOISE
    5, // FORMATION_DOUBLE_LINE_1
    5, // FORMATION_DOUBLE_LINE_2
    0, // FORMATION_SINGLE_LINE_1 (not used by enemies)
    0, // FORMATION_SINGLE_LINE_2 (not used by enemies)
    0, // FORMATION_MOP_UP (not used by enemies)
    0, // FORMATION_AT_REST (not used by enemies)
    4, // FORMATION_ENEMY_MOB
    4, // FORMATION_ENEMY_WIDE_COLUMN
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
        building_destroy_by_collapse(&all_buildings[building_id]);
        int ruin_id = map_building_at(grid_offset);
        if (ruin_id) {
            all_buildings[ruin_id].state = BUILDING_STATE_DELETED_BY_GAME;
            map_building_set(grid_offset, 0);
        }
    }
    terrain_grid.items[grid_offset] = 0;
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
    for (int i = 0; i < MAX_EARTHQUAKES; i++) {
        if (!scenario.earthquakes[i].state || scenario.earthquakes[i].branch_coordinates[0].x == -1 || scenario.earthquakes[i].branch_coordinates[0].y == -1) {
            return;
        }
        if (scenario.earthquakes[i].state == EVENT_NOT_STARTED) {
            if (scenario.start_year + scenario.earthquakes[i].year == game_time_year() && scenario.earthquakes[i].month == game_time_month()) {
                scenario.earthquakes[i].state = EVENT_IN_PROGRESS;
                city_message_post(1, MESSAGE_EARTHQUAKE, 0, map_grid_offset(scenario.earthquakes[i].branch_coordinates[0].x, scenario.earthquakes[i].branch_coordinates[1].y));
            }
        } else if (scenario.earthquakes[i].state == EVENT_IN_PROGRESS) {
            scenario.earthquakes[i].delay++;
            if (scenario.earthquakes[i].delay >= scenario.earthquakes[i].max_delay) {
                scenario.earthquakes[i].delay = 0;
                scenario.earthquakes[i].duration++;
                if (scenario.earthquakes[i].duration >= scenario.earthquakes[i].max_duration) {
                    scenario.earthquakes[i].state = EVENT_FINISHED;
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
                int x = calc_bound(scenario.earthquakes[i].branch_coordinates[index].x + dx, 0, scenario.map.width - 1);
                int y = calc_bound(scenario.earthquakes[i].branch_coordinates[index].y + dy, 0, scenario.map.height - 1);
                if (!map_terrain_is(map_grid_offset(x, y), TERRAIN_ELEVATION | TERRAIN_ROCK | TERRAIN_WATER)) {
                    scenario.earthquakes[i].branch_coordinates[index].x = x;
                    scenario.earthquakes[i].branch_coordinates[index].y = y;
                    advance_earthquake_to_tile(x, y);
                }
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

static void create_enemy_formation(int figure_type, int enemy_type, int x, int y, int figures_amount_to_spawn, int orientation, int formation_index, int enemy_attack_priority)
{
    for (int i = 0; i < MAX_ENEMY_FORMATIONS; i++) {
        if (!enemy_formations[i].in_use) {
            struct formation_t *m = &enemy_formations[i];
            m->in_use = 1;
            m->figure_type = figure_type;
            m->layout = ENEMY_PROPERTIES[enemy_type].formation_layout;
            if (ENEMY_PROPERTIES[enemy_type].formation_layout == FORMATION_DOUBLE_LINE_1) {
                if (orientation == DIR_2_RIGHT || orientation == DIR_6_LEFT) {
                    m->layout = FORMATION_DOUBLE_LINE_2;
                }
            }
            m->max_figures = figures_amount_to_spawn;
            m->attack_priority = enemy_attack_priority;
            switch (enemy_type) {
                case ENEMY_TYPE_BARBARIAN:
                case ENEMY_TYPE_BRITON:
                case ENEMY_TYPE_CELT:
                case ENEMY_TYPE_PICT:
                case ENEMY_TYPE_GAUL:
                case ENEMY_TYPE_HELVETIUS:
                case ENEMY_TYPE_HUN:
                case ENEMY_TYPE_GOTH:
                case ENEMY_TYPE_VISIGOTH:
                case ENEMY_TYPE_NUMIDIAN:
                    m->max_morale = 80;
                    break;
                case ENEMY_TYPE_CARTHAGINIAN:
                case ENEMY_TYPE_GREEK:
                case ENEMY_TYPE_MACEDONIAN:
                    m->max_morale = 90;
                    break;
                case ENEMY_TYPE_PERGAMUM:
                case ENEMY_TYPE_IBERIAN:
                case ENEMY_TYPE_JUDEAN:
                case ENEMY_TYPE_SELEUCID:
                case ENEMY_TYPE_EGYPTIAN:
                case ENEMY_TYPE_ETRUSCAN:
                case ENEMY_TYPE_SAMNITE:
                    m->max_morale = 70;
                    break;
                case ENEMY_TYPE_CAESAR:
                    m->max_morale = 100;
                    break;
            }
            m->morale = m->max_morale;
            if (orientation == DIR_0_TOP) {
                if (formation_index % 2) {
                    m->destination_x = x;
                    m->destination_y = y - LAYOUT_OFFSETS_FROM_MAP_EDGE[m->layout][1];
                } else {
                    m->destination_x = x;
                    m->destination_y = y - LAYOUT_OFFSETS_FROM_MAP_EDGE[m->layout][0];
                }
            } else if (orientation == DIR_4_BOTTOM) {
                if (formation_index % 2) {
                    m->destination_x = x;
                    m->destination_y = y + LAYOUT_OFFSETS_FROM_MAP_EDGE[m->layout][1];
                } else {
                    m->destination_x = x;
                    m->destination_y = y + LAYOUT_OFFSETS_FROM_MAP_EDGE[m->layout][0];
                }
            } else if (orientation == DIR_2_RIGHT) {
                if (formation_index % 2) {
                    m->destination_x = x + LAYOUT_OFFSETS_FROM_MAP_EDGE[m->layout][1];
                    m->destination_y = y;
                } else {
                    m->destination_x = x + LAYOUT_OFFSETS_FROM_MAP_EDGE[m->layout][0];
                    m->destination_y = y;
                }
            } else if (orientation == DIR_6_LEFT) {
                if (formation_index % 2) {
                    m->destination_x = x - LAYOUT_OFFSETS_FROM_MAP_EDGE[m->layout][1];
                    m->destination_y = y;
                } else {
                    m->destination_x = x - LAYOUT_OFFSETS_FROM_MAP_EDGE[m->layout][0];
                    m->destination_y = y;
                }
            }

            for (int fig = 0; fig < figures_amount_to_spawn; fig++) {
                struct figure_t *f = figure_create(figure_type, x, y, orientation);
                f->is_targetable = 1;
                f->action_state = FIGURE_ACTION_ENEMY_SPAWNING;
                f->formation_id = m->id;
                f->enemy_image_group = ENEMY_PROPERTIES[enemy_type].enemy_img_group;
                if (formation_index) {
                    f->wait_ticks = 50 * formation_index + 15 * fig + 300;
                } else { // first formation is "scouting party"
                    f->wait_ticks = 15 * fig + 10;
                }

                f->terrain_usage = TERRAIN_USAGE_ENEMY;
                f->is_invisible = 1;
                switch (figure_type) {
                    case FIGURE_ENEMY_BARBARIAN_SWORDSMAN:
                    case FIGURE_ENEMY_HUN_SWORDSMAN:
                    case FIGURE_ENEMY_GOTH_SWORDSMAN:
                    case FIGURE_ENEMY_VISIGOTH_SWORDSMAN:
                    case FIGURE_ENEMY_NUMIDIAN_SWORDSMAN:
                    case FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER:
                        f->speed_multiplier = 2;
                        break;
                    case FIGURE_ENEMY_BRITON_CHARIOT:
                    case FIGURE_ENEMY_CELT_CHARIOT:
                    case FIGURE_ENEMY_PICT_CHARIOT:
                        f->speed_multiplier = 3;
                        f->mounted_charge_ticks = 8;
                        f->mounted_charge_ticks_max = 8;
                        break;
                    case FIGURE_ENEMY_HUN_MOUNTED_ARCHER:
                    case FIGURE_ENEMY_GOTH_MOUNTED_ARCHER:
                    case FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER:
                        f->speed_multiplier = 3;
                        break;
                    default:
                        break;
                }
                add_figure_to_formation(f, m);
                if (enemy_type == ENEMY_TYPE_CAESAR) {
                    city_data.figure.imperial_soldiers++;
                } else {
                    city_data.figure.enemies++;
                }
            }
            break;
        }
    }
}

static int terrain_at_offset_clear(int grid_offset)
{
    if (map_terrain_is(grid_offset, TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_TREE | TERRAIN_ELEVATION)
    || (map_terrain_is(grid_offset, TERRAIN_WATER) && !map_terrain_is(grid_offset, TERRAIN_ROAD))) {
        return 0;
    } else {
        if (map_terrain_is(grid_offset, TERRAIN_BUILDING | TERRAIN_AQUEDUCT | TERRAIN_GATEHOUSE | TERRAIN_WALL)) {
            building_destroy_by_enemy(map_grid_offset_to_x(grid_offset), map_grid_offset_to_y(grid_offset), grid_offset);
        }
        return 1;
    }
}

static int find_closest_clear_y_offset(int x, int y)
{
    for (int j = y; j < scenario.map.height; j++) {
        int grid_offset = map_grid_offset(x, j);
        if (terrain_at_offset_clear(grid_offset)) {
            return j;
        } else {
            continue;
        }
    }
    for (int j = y; j < scenario.map.height; j--) {
        int grid_offset = map_grid_offset(x, j);
        if (terrain_at_offset_clear(grid_offset)) {
            return j;
        } else {
            continue;
        }
    }
    return -1;
}

static int find_closest_clear_x_offset(int x, int y)
{
    for (int j = x; j < scenario.map.width; j++) {
        int grid_offset = map_grid_offset(j, y);
        if (terrain_at_offset_clear(grid_offset)) {
            return j;
        } else {
            continue;
        }
    }
    for (int j = x; j < scenario.map.width; j--) {
        int grid_offset = map_grid_offset(j, y);
        if (terrain_at_offset_clear(grid_offset)) {
            return j;
        } else {
            continue;
        }
    }
    return -1;
}

int start_invasion(int enemy_type, int amount, int invasion_point, int enemy_attack_priority)
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

    // determine orientation
    int orientation = DIR_0_TOP;
    if (y == 0) {
        orientation = DIR_4_BOTTOM;
    } else if (x == 0) {
        orientation = DIR_2_RIGHT;
    } else if (x >= scenario.map.width - 1) {
        orientation = DIR_6_LEFT;
    }

    int enemy_count_per_type[MAX_ENEMY_TYPES_PER_ARMY];
    for (int i = 0; i < MAX_ENEMY_TYPES_PER_ARMY; i++) {
        enemy_count_per_type[i] = calc_adjust_with_percentage(amount, ENEMY_PROPERTIES[enemy_type].pct_type[i]);
    }
    enemy_count_per_type[0] += amount - (enemy_count_per_type[0] + enemy_count_per_type[1] + enemy_count_per_type[2]); // assign leftovers to first type

    int formation_index = 0;
    for (int i = 0; i < MAX_ENEMY_TYPES_PER_ARMY; i++) {
        while (enemy_count_per_type[i]) {
            int figures_amount_to_spawn = enemy_count_per_type[i] >= MAX_FORMATION_FIGURES ? MAX_FORMATION_FIGURES : enemy_count_per_type[i];
            // determine spawn point for each formation
            if (orientation == DIR_0_TOP || orientation == DIR_4_BOTTOM) {
                int x_offseted = -1;
                if (x >= scenario.map.width / 2) {
                    if (formation_index % 2) {
                        x_offseted = find_closest_clear_x_offset(x - LAYOUT_SPACING_BETWEEN_FORMATIONS[ENEMY_PROPERTIES[enemy_type].formation_layout] * formation_index, y);
                    } else {
                        x_offseted = find_closest_clear_x_offset(x - LAYOUT_SPACING_BETWEEN_FORMATIONS[ENEMY_PROPERTIES[enemy_type].formation_layout] * (formation_index + 1), y);
                    }
                } else {
                    if (formation_index % 2) {
                        x_offseted = find_closest_clear_x_offset(x + LAYOUT_SPACING_BETWEEN_FORMATIONS[ENEMY_PROPERTIES[enemy_type].formation_layout] * formation_index, y);
                    } else {
                        x_offseted = find_closest_clear_x_offset(x + LAYOUT_SPACING_BETWEEN_FORMATIONS[ENEMY_PROPERTIES[enemy_type].formation_layout] * (formation_index + 1), y);
                    }
                }
                if (x_offseted > -1) {
                    create_enemy_formation(ENEMY_PROPERTIES[enemy_type].figure_types[i], enemy_type, x_offseted, y, figures_amount_to_spawn, orientation, formation_index, enemy_attack_priority);
                    enemy_count_per_type[i] -= figures_amount_to_spawn;
                    formation_index++;
                }
            } else { // left/right
                int y_offseted = -1;
                if (y >= scenario.map.height / 2) {
                    if (formation_index % 2) {
                        y_offseted = find_closest_clear_y_offset(x, y - LAYOUT_SPACING_BETWEEN_FORMATIONS[ENEMY_PROPERTIES[enemy_type].formation_layout] * formation_index);
                    } else {
                        y_offseted = find_closest_clear_y_offset(x, y - LAYOUT_SPACING_BETWEEN_FORMATIONS[ENEMY_PROPERTIES[enemy_type].formation_layout] * (formation_index + 1));
                    }
                } else {
                    if (formation_index % 2) {
                        y_offseted = find_closest_clear_y_offset(x, y + LAYOUT_SPACING_BETWEEN_FORMATIONS[ENEMY_PROPERTIES[enemy_type].formation_layout] * formation_index);
                    } else {
                        y_offseted = find_closest_clear_y_offset(x, y + LAYOUT_SPACING_BETWEEN_FORMATIONS[ENEMY_PROPERTIES[enemy_type].formation_layout] * (formation_index + 1));
                    }
                }
                if (y_offseted > -1) {
                    create_enemy_formation(ENEMY_PROPERTIES[enemy_type].figure_types[i], enemy_type, x, y_offseted, figures_amount_to_spawn, orientation, formation_index, enemy_attack_priority);
                    enemy_count_per_type[i] -= figures_amount_to_spawn;
                    formation_index++;
                }
            }
        }
    }

    return map_grid_offset(x, y);
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
                    scenario.invasions[i].enemy_type,
                    scenario.invasions[i].amount,
                    scenario.invasions[i].from,
                    scenario.invasions[i].target_type);
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
                    scenario.invasions[i].amount,
                    scenario.invasions[i].from,
                    scenario.invasions[i].target_type);
                if (grid_offset > 0) {
                    city_data.emperor.invasion.from_editor = 1;
                    city_message_post(1, MESSAGE_CAESAR_ARMY_ATTACK, 0, grid_offset);
                }
            }
            // local uprisings
            if (scenario.invasions[i].type == INVASION_TYPE_LOCAL_UPRISING) {
                int grid_offset = start_invasion(
                    ENEMY_TYPE_BARBARIAN,
                    scenario.invasions[i].amount,
                    scenario.invasions[i].from,
                    scenario.invasions[i].target_type);
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
    int grid_offset = start_invasion(ENEMY_TYPE_CAESAR, size, 0, FORMATION_ATTACK_BEST_BUILDINGS);
    if (grid_offset > 0) {
        city_message_post(1, MESSAGE_CAESAR_ARMY_ATTACK, 0, grid_offset);
        return 1;
    }
    return 0;
}

void scenario_invasion_start_from_cheat(void)
{
    city_message_post(1, MESSAGE_ENEMY_ARMY_ATTACK, 0, start_invasion(rand() % (ENEMY_TYPE_MAX_COUNT + 1), 160, MAX_INVASION_POINTS, FORMATION_ATTACK_RANDOM));
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

static void increase_resource_buy_limit(struct empire_object_t *trade_city, int resource)
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

static void decrease_resource_buy_limit(struct empire_object_t *trade_city, int resource)
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

static void increase_resource_sell_limit(struct empire_object_t *trade_city, int resource)
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

static void decrease_resource_sell_limit(struct empire_object_t *trade_city, int resource)
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