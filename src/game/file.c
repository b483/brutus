#include "file.h"

#include "building/construction.h"
#include "building/granary.h"
#include "building/maintenance.h"
#include "building/menu.h"
#include "building/storage.h"
#include "city/data.h"
#include "city/emperor.h"
#include "city/map.h"
#include "city/message.h"
#include "city/military.h"
#include "city/victory.h"
#include "city/view.h"
#include "core/config.h"
#include "core/encoding.h"
#include "core/file.h"
#include "core/image.h"
#include "core/io.h"
#include "core/lang.h"
#include "core/string.h"
#include "empire/empire.h"
#include "empire/trade_prices.h"
#include "figure/enemy_army.h"
#include "figure/formation.h"
#include "figure/name.h"
#include "figure/route.h"
#include "figure/trader.h"
#include "figuretype/animal.h"
#include "figuretype/water.h"
#include "game/animation.h"
#include "game/file_io.h"
#include "game/settings.h"
#include "game/state.h"
#include "game/time.h"
#include "game/undo.h"
#include "map/aqueduct.h"
#include "map/bookmark.h"
#include "map/building.h"
#include "map/desirability.h"
#include "map/elevation.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/image.h"
#include "map/image_context.h"
#include "map/natives.h"
#include "map/orientation.h"
#include "map/property.h"
#include "map/random.h"
#include "map/road_network.h"
#include "map/routing_terrain.h"
#include "map/soldier_strength.h"
#include "map/sprite.h"
#include "map/terrain.h"
#include "map/tiles.h"
#include "scenario/editor_events.h"
#include "scenario/map.h"
#include "scenario/data.h"
#include "scenario/scenario.h"
#include "sound/city.h"
#include "sound/music.h"

#include <string.h>


static void clear_scenario_data(void)
{
    // clear data
    city_victory_reset();
    building_construction_clear_type();
    city_data_init();
    city_message_init_scenario();
    game_state_init();
    game_animation_init();
    sound_city_init();
    building_menu_disable_all();
    building_clear_all();
    building_storage_clear_all();
    figure_init_scenario();
    enemy_armies_clear();
    figure_name_init();
    formations_clear();
    figure_route_clear_all();

    game_time_init(2098);

    // clear grids
    map_image_clear();
    map_building_clear();
    map_terrain_clear();
    map_aqueduct_clear();
    map_grid_clear_u16(map_figures.items);
    map_property_clear();
    map_sprite_clear();
    map_random_clear();
    map_desirability_clear();
    map_elevation_clear();
    map_soldier_strength_clear();
    map_road_network_clear();

    map_image_context_init();
    map_random_init();
}

static void initialize_scenario_data(const uint8_t *scenario_name)
{
    string_copy(scenario_name, scenario.scenario_name, MAX_SCENARIO_NAME);
    scenario_map_init();

    // initialize grids
    map_tiles_update_all_elevation();
    map_tiles_update_all_earthquake();
    map_tiles_update_all_rocks();
    map_tiles_add_entry_exit_flags();
    map_tiles_update_all_empty_land();
    map_tiles_update_all_meadow();
    map_tiles_update_all_roads();
    map_tiles_update_all_plazas();
    map_tiles_update_all_walls();
    map_tiles_update_all_aqueducts(0);

    map_natives_init();

    city_view_init();

    figure_create_fishing_points();
    figure_create_herds();
    figure_create_flotsam();

    map_routing_update_all();

    scenario_map_init_entry_exit();

    city_map_set_entry_point(scenario.entry_point.x, scenario.entry_point.y);
    city_map_set_exit_point(scenario.exit_point.x, scenario.exit_point.y);

    game_time_init(scenario.start_year);

    // set up events
    load_custom_messages();

    empire_init_scenario();
    traders_clear();
    city_military_determine_distant_battle_city();
    building_menu_update();
    image_load_climate(scenario.climate, 0, 0);

    city_data_init_scenario();
    game_state_unpause();
}

static void initialize_saved_game(void)
{
    scenario.empire.distant_battle_roman_travel_months = empire_object_init_distant_battle_travel_months(EMPIRE_OBJECT_ROMAN_ARMY);
    scenario.empire.distant_battle_enemy_travel_months = empire_object_init_distant_battle_travel_months(EMPIRE_OBJECT_ENEMY_ARMY);

    scenario_map_init();

    city_view_init();

    map_routing_update_all();

    map_orientation_update_buildings();
    figure_route_clean();
    map_road_network_update();
    building_maintenance_check_rome_access();
    building_granaries_calculate_stocks();
    building_menu_update();
    city_message_init_problem_areas();

    sound_city_init();

    building_construction_clear_type();
    game_undo_disable();
    game_state_reset_overlay();

    image_load_climate(scenario.climate, 0, 0);
    city_military_determine_distant_battle_city();
    map_tiles_determine_gardens();

    city_message_clear_scroll();

    game_state_unpause();
}

int game_file_start_scenario(const char *scenario_selected)
{
    // assume scenario can be passed in with or without .map extension
    char scenario_file[FILE_NAME_MAX];
    strncpy(scenario_file, scenario_selected, FILE_NAME_MAX - 1);
    uint8_t scenario_name[FILE_NAME_MAX];
    encoding_from_utf8(scenario_selected, scenario_name, FILE_NAME_MAX);
    if (file_has_extension(scenario_file, "map")) {
        file_remove_extension(scenario_name);
    } else {
        file_append_extension(scenario_file, "map");
    }
    if (!file_exists(MAPS_DIR_PATH, scenario_file)) {
        return 0;
    }

    map_bookmarks_clear();
    clear_scenario_data();

    if (!game_file_load_scenario_data(scenario_file)) {
        return 0;
    }
    trade_prices_reset();

    initialize_scenario_data(scenario_name);
    set_player_name_from_config();
    city_emperor_init_scenario();
    building_menu_update();
    city_message_init_scenario();
    return 1;
}

int game_file_load_scenario_data(const char *scenario_file)
{
    if (!game_file_io_read_scenario(MAPS_DIR_PATH, scenario_file)) {
        return 0;
    }
    city_view_reset_orientation();
    return 1;
}

int game_file_load_saved_game(const char *dir, const char *filename)
{
    if (!game_file_io_read_saved_game(dir, filename, 0)) {
        return 0;
    }
    initialize_saved_game();
    building_storage_reset_building_ids();

    sound_music_update(1);
    return 1;
}

