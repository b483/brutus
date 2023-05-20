#include "tick.h"

#include "building/count.h"
#include "building/dock.h"
#include "building/figure.h"
#include "building/granary.h"
#include "building/house_evolution.h"
#include "building/house_population.h"
#include "building/house_service.h"
#include "building/industry.h"
#include "building/maintenance.h"
#include "building/warehouse.h"
#include "city/culture.h"
#include "city/data_private.h"
#include "city/emperor.h"
#include "city/finance.h"
#include "city/gods.h"
#include "city/health.h"
#include "city/labor.h"
#include "city/message.h"
#include "city/migration.h"
#include "city/population.h"
#include "city/ratings.h"
#include "city/resource.h"
#include "city/sentiment.h"
#include "city/trade.h"
#include "city/victory.h"
#include "core/file.h"
#include "core/random.h"
#include "editor/editor.h"
#include "empire/object.h"
#include "figure/combat.h"
#include "figure/formation_legion.h"
#include "figuretype/animal.h"
#include "figuretype/cartpusher.h"
#include "figuretype/crime.h"
#include "figuretype/docker.h"
#include "figuretype/editor.h"
#include "figuretype/enemy.h"
#include "figuretype/entertainer.h"
#include "figuretype/maintenance.h"
#include "figuretype/market.h"
#include "figuretype/migrant.h"
#include "figuretype/missile.h"
#include "figuretype/native.h"
#include "figuretype/service.h"
#include "figuretype/soldier.h"
#include "figuretype/trader.h"
#include "figuretype/wall.h"
#include "figuretype/water.h"
#include "game/file_io.h"
#include "game/settings.h"
#include "game/time.h"
#include "game/undo.h"
#include "map/desirability.h"
#include "map/natives.h"
#include "map/road_network.h"
#include "map/routing_terrain.h"
#include "map/tiles.h"
#include "map/water_supply.h"
#include "scenario/editor_events.h"
#include "sound/music.h"
#include "widget/minimap.h"

static void advance_year(void)
{
    game_undo_disable();
    game_time_advance_year();
    scenario_empire_process_expansion();
    city_population_request_yearly_update();
    city_finance_handle_year_change();

    // reset yearly trade amounts
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use && empire_objects[i].trade_route_open) {
            for (int r = RESOURCE_MIN; r < RESOURCE_MAX; r++) {
                empire_objects[i].resource_bought[r] = 0;
                empire_objects[i].resource_sold[r] = 0;
            }
        }
    }

    building_maintenance_update_fire_direction();
    city_ratings_update(1);
    city_data.religion.neptune_double_trade_active = 0;
}

static void advance_month(void)
{
    city_migration_reset_newcomers();
    city_health_update();
    scenario_random_event_process();
    city_finance_handle_month_change();
    city_resource_consume_food();
    city_victory_update_months_to_govern();
    update_legion_morale_monthly();
    city_message_decrease_delays();

    map_tiles_update_all_roads();
    map_tiles_update_all_water();
    map_routing_update_land_citizen();
    city_message_sort_and_compact();

    if (game_time_advance_month()) {
        advance_year();
    } else {
        city_ratings_update(0);
    }
    scenario_custom_messages_process();
    scenario_gladiator_revolt_process();
    scenario_request_process();
    scenario_price_change_process();
    scenario_demand_change_process();
    scenario_invasion_process();
    scenario_distant_battle_process();
    // record monthly population
    city_data.population.monthly.values[city_data.population.monthly.next_index++] = city_data.population.population;
    if (city_data.population.monthly.next_index >= 2400) {
        city_data.population.monthly.next_index = 0;
    }
    ++city_data.population.monthly.count;
    city_festival_update();
    if (setting_monthly_autosave()) {
        game_file_io_write_saved_game(SAVES_DIR_PATH, "autosave.sav");
    }
}

static void advance_day(void)
{
    if (game_time_advance_day()) {
        advance_month();
    }
    if (game_time_day() == 0 || game_time_day() == 8) {
        city_sentiment_update();
    }
}

static void advance_tick(void)
{
    // NB: these ticks are noop:
    // 0, 9, 11, 13, 14, 15, 26, 29, 41, 42, 47
    switch (game_time_tick()) {
        case 1: city_gods_calculate_moods(1); break;
        case 2: sound_music_update(0); break;
        case 3: widget_minimap_invalidate(); break;
        case 4:
            update_debt_state();
            process_caesar_invasion();
            break;
        case 5: formation_update_all(); break;
        case 6: map_natives_check_land(); break;
        case 7: map_road_network_update(); break;
        case 8: building_granaries_calculate_stocks(); break;
        case 10: building_update_highest_id(); break;
        case 12: house_service_decay_houses_covered(); break;
        case 16: city_resource_calculate_warehouse_stocks(); break;
        case 17: city_resource_calculate_food_stocks_and_supply_wheat(); break;
        case 18: city_resource_calculate_workshop_stocks(); break;
        case 19: building_dock_update_open_water_access(); break;
        case 20: building_industry_update_production(); break;
        case 21: building_maintenance_check_rome_access(); break;
        case 22: house_population_update_room(); break;
        case 23: house_population_update_migration(); break;
        case 24: house_population_evict_overcrowded(); break;
        case 25: city_labor_update(); break;
        case 27: map_water_supply_update_reservoir_fountain(); break;
        case 28: map_water_supply_update_houses(); break;
        case 30: widget_minimap_invalidate(); break;
        case 31: building_figure_generate(); break;
        case 32: city_trade_update(); break;
        case 33: building_count_update(); city_culture_update_coverage(); break;
        case 34: distribute_treasury(); break;
        case 35: house_service_decay_culture(); break;
        case 36: house_service_calculate_culture_aggregates(); break;
        case 37: map_desirability_update(); break;
        case 38: building_update_desirability(); break;
        case 39: building_house_process_evolve_and_consume_goods(); break;
        case 40: building_update_state(); break;
        case 43: building_maintenance_update_burning_ruins(); break;
        case 44: building_maintenance_check_fire_collapse(); break;
        case 45: figure_generate_criminals(); break;
        case 46: building_industry_update_wheat_production(); break;
        case 48: house_service_decay_tax_collector(); break;
        case 49: city_culture_calculate(); break;
    }
    if (game_time_advance_tick()) {
        advance_day();
    }
}

void game_tick_run(void)
{
    if (editor_is_active()) {
        random_generate_next(); // update random to randomize native huts
        for (int i = 1; i < MAX_FIGURES; i++) {
            struct figure_t *f = &figures[i];
            if (f->state == FIGURE_STATE_ALIVE && f->type == FIGURE_MAP_FLAG) {
                figure_editor_flag_action(f);
            } else if (f->state == FIGURE_STATE_DEAD) {
                figure_delete(f);
            }
        }
        return;
    }
    random_generate_next();
    game_undo_reduce_time_available();
    advance_tick();
    city_data.entertainment.hippodrome_has_race = 0;
    for (int i = 1; i < MAX_FIGURES; i++) {
        struct figure_t *f = &figures[i];
        if (f->action_state == FIGURE_ACTION_CORPSE) {
            figure_handle_corpse(f);
        } else if (f->action_state == FIGURE_ACTION_ATTACK) {
            figure_combat_handle_attack(f);
        }
        if (f->state == FIGURE_STATE_ALIVE) {
            switch (f->type) {
                case FIGURE_NONE:
                case FIGURE_IMMIGRANT:
                    figure_immigrant_action(f);
                    break;
                case FIGURE_EMIGRANT:
                    figure_emigrant_action(f);
                    break;
                case FIGURE_HOMELESS:
                    figure_homeless_action(f);
                    break;
                case FIGURE_PATRICIAN:
                    figure_patrician_action(f);
                    break;
                case FIGURE_CART_PUSHER:
                    figure_cartpusher_action(f);
                    break;
                case FIGURE_LABOR_SEEKER:
                    figure_labor_seeker_action(f);
                    break;
                case FIGURE_BARBER:
                    figure_barber_action(f);
                    break;
                case FIGURE_BATHHOUSE_WORKER:
                    figure_bathhouse_worker_action(f);
                    break;
                case FIGURE_DOCTOR:
                case FIGURE_SURGEON:
                    figure_doctor_action(f);
                    break;
                case FIGURE_PRIEST:
                    figure_priest_action(f);
                    break;
                case FIGURE_SCHOOL_CHILD:
                    figure_school_child_action(f);
                    break;
                case FIGURE_TEACHER:
                    figure_teacher_action(f);
                    break;
                case FIGURE_LIBRARIAN:
                    figure_librarian_action(f);
                    break;
                case FIGURE_MISSIONARY:
                    figure_missionary_action(f);
                    break;
                case FIGURE_ACTOR:
                case FIGURE_GLADIATOR:
                case FIGURE_LION_TAMER:
                case FIGURE_CHARIOTEER:
                    figure_entertainer_action(f);
                    break;
                case FIGURE_HIPPODROME_HORSES:
                    figure_hippodrome_horse_action(f);
                    break;
                case FIGURE_TAX_COLLECTOR:
                    figure_tax_collector_action(f);
                    break;
                case FIGURE_ENGINEER:
                    figure_engineer_action(f);
                    break;
                case FIGURE_FISHING_BOAT:
                    figure_fishing_boat_action(f);
                    break;
                case FIGURE_FISH_GULLS:
                    figure_seagulls_action(f);
                    break;
                case FIGURE_SHIPWRECK:
                    figure_shipwreck_action(f);
                    break;
                case FIGURE_DOCKER:
                    figure_docker_action(f);
                    break;
                case FIGURE_FLOTSAM:
                    figure_flotsam_action(f);
                    break;
                case FIGURE_BALLISTA:
                    figure_ballista_action(f);
                    break;
                case FIGURE_BOLT:
                    figure_bolt_action(f);
                    break;
                case FIGURE_TOWER_SENTRY:
                    figure_tower_sentry_action(f);
                    break;
                case FIGURE_JAVELIN:
                    figure_javelin_action(f);
                    break;
                case FIGURE_PREFECT:
                    figure_prefect_action(f);
                    break;
                case FIGURE_FORT_STANDARD:
                    figure_military_standard_action(f);
                    break;
                case FIGURE_FORT_JAVELIN:
                case FIGURE_FORT_MOUNTED:
                case FIGURE_FORT_LEGIONARY:
                    figure_soldier_action(f);
                    break;
                case FIGURE_MARKET_BUYER:
                    figure_market_buyer_action(f);
                    break;
                case FIGURE_MARKET_TRADER:
                    figure_market_trader_action(f);
                    break;
                case FIGURE_DELIVERY_BOY:
                    figure_delivery_boy_action(f);
                    break;
                case FIGURE_WAREHOUSEMAN:
                    figure_warehouseman_action(f);
                    break;
                case FIGURE_PROTESTER:
                    figure_protestor_action(f);
                    break;
                case FIGURE_CRIMINAL:
                    figure_criminal_action(f);
                    break;
                case FIGURE_RIOTER:
                    figure_rioter_action(f);
                    break;
                case FIGURE_TRADE_CARAVAN:
                    figure_trade_caravan_action(f);
                    break;
                case FIGURE_TRADE_CARAVAN_DONKEY:
                    figure_trade_caravan_donkey_action(f);
                    break;
                case FIGURE_TRADE_SHIP:
                    figure_trade_ship_action(f);
                    break;
                case FIGURE_INDIGENOUS_NATIVE:
                    figure_indigenous_native_action(f);
                    break;
                case FIGURE_NATIVE_TRADER:
                    figure_native_trader_action(f);
                    break;
                case FIGURE_WOLF:
                    figure_wolf_action(f);
                    break;
                case FIGURE_SHEEP:
                    figure_sheep_action(f);
                    break;
                case FIGURE_ZEBRA:
                    figure_zebra_action(f);
                    break;
                case FIGURE_ENEMY_GLADIATOR:
                    figure_enemy_gladiator_action(f);
                    break;
                case FIGURE_ENEMY_BARBARIAN_SWORDSMAN:
                case FIGURE_ENEMY_HUN_SWORDSMAN:
                case FIGURE_ENEMY_GOTH_SWORDSMAN:
                case FIGURE_ENEMY_VISIGOTH_SWORDSMAN:
                case FIGURE_ENEMY_NUMIDIAN_SWORDSMAN:
                    figure_enemy_fast_swordsman_action(f);
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
                case FIGURE_ENEMY_GREEK_SWORDSMAN:
                case FIGURE_ENEMY_MACEDONIAN_SWORDSMAN:
                case FIGURE_ENEMY_PERGAMUM_SWORDSMAN:
                case FIGURE_ENEMY_IBERIAN_SWORDSMAN:
                case FIGURE_ENEMY_JUDEAN_SWORDSMAN:
                case FIGURE_ENEMY_SELEUCID_SWORDSMAN:
                    figure_enemy_swordsman_action(f);
                    break;
                case FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT:
                    figure_enemy_elephant_action(f);
                    break;
                case FIGURE_ENEMY_BRITON_CHARIOT:
                case FIGURE_ENEMY_CELT_CHARIOT:
                case FIGURE_ENEMY_PICT_CHARIOT:
                    figure_enemy_chariot_action(f);
                    break;
                case FIGURE_ENEMY_EGYPTIAN_CAMEL:
                    figure_enemy_camel_action(f);
                    break;
                case FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER:
                    figure_enemy_light_ranged_spearman_action(f);
                    break;
                case FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER:
                case FIGURE_ENEMY_SAMNITE_SPEAR_THROWER:
                case FIGURE_ENEMY_GREEK_SPEAR_THROWER:
                case FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER:
                case FIGURE_ENEMY_PERGAMUM_ARCHER:
                case FIGURE_ENEMY_IBERIAN_SPEAR_THROWER:
                case FIGURE_ENEMY_JUDEAN_SPEAR_THROWER:
                case FIGURE_ENEMY_SELEUCID_SPEAR_THROWER:
                    figure_enemy_heavy_ranged_spearman_action(f);
                    break;
                case FIGURE_ENEMY_GAUL_AXEMAN:
                case FIGURE_ENEMY_HELVETIUS_AXEMAN:
                    figure_enemy_axeman_action(f);
                    break;
                case FIGURE_ENEMY_HUN_MOUNTED_ARCHER:
                case FIGURE_ENEMY_GOTH_MOUNTED_ARCHER:
                case FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER:
                    figure_enemy_mounted_archer_action(f);
                    break;
                case FIGURE_ENEMY_CAESAR_JAVELIN:
                case FIGURE_ENEMY_CAESAR_MOUNTED:
                case FIGURE_ENEMY_CAESAR_LEGIONARY:
                    figure_enemy_caesar_legionary_action(f);
                    break;
                case FIGURE_ARROW:
                    figure_arrow_action(f);
                    break;
                case FIGURE_MAP_FLAG:
                    figure_editor_flag_action(f);
                    break;
                case FIGURE_EXPLOSION:
                    figure_explosion_cloud_action(f);
                    break;
                default:
                    break;
            }
        } else if (f->state == FIGURE_STATE_DEAD) {
            figure_delete(f);
        }
    }
    scenario_earthquake_process();
    city_victory_check();
}
