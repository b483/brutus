#include "file.h"

#include "city/data.h"
#include "city/emperor.h"
#include "city/message.h"
#include "city/military.h"
#include "city/victory.h"
#include "city/view.h"
#include "core/config.h"
#include "core/file.h"
#include "core/image.h"
#include "core/lang.h"
#include "core/string.h"
#include "empire/empire.h"
#include "figure/formation.h"
#include "figure/formation_herd.h"
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
#include "map/map.h"
#include "scenario/scenario.h"
#include "scenario/scenario.h"
#include "sound/sound.h"
#include "window/build_menu.h"

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
    initialize_city_sounds();
    building_clear_all();
    building_storage_clear_all();
    figure_init_scenario();
    figure_name_init();
    reset_all_formations();
    figure_route_clear_all();

    game_time_init(2098);

    // clear grids
    map_image_clear();
    map_building_clear();
    memset(terrain_grid.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
    map_aqueduct_clear();
    memset(map_figures.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint16_t));
    map_property_clear();
    map_sprite_clear();
    map_random_clear();
    map_desirability_clear();
    memset(terrain_elevation.items, 0, GRID_SIZE * GRID_SIZE * sizeof(uint8_t));
    map_road_network_clear();

    map_image_context_init();
    map_random_init();
}

static void initialize_scenario_data(const char *scenario_name)
{
    string_copy(scenario_name, scenario.scenario_name, MAX_SCENARIO_NAME);
    map_grid_init(scenario.map.width, scenario.map.height, scenario.map.grid_start, scenario.map.grid_border_size);

    // initialize grids
    map_tiles_update_all_elevation();
    map_tiles_update_all_earthquake();
    map_tiles_add_entry_exit_flags();
    map_tiles_update_all_empty_land();
    map_tiles_update_all_roads();
    map_tiles_update_all_plazas();
    map_tiles_update_all_walls();
    map_tiles_update_all_aqueducts(0);

    map_natives_init();

    city_view_init();

    figure_create_fishing_points();
    create_herds();
    figure_create_flotsam();

    map_routing_update_all();

    // init entry/exit
    if (scenario.entry_point.x == -1 || scenario.entry_point.y == -1) {
        scenario.entry_point.x = scenario.map.width - 1;
        scenario.entry_point.y = scenario.map.height / 2;
    }
    if (scenario.exit_point.x == -1 || scenario.exit_point.y == -1) {
        scenario.exit_point.x = scenario.entry_point.x;
        scenario.exit_point.y = scenario.entry_point.y;
    }

    game_time_init(scenario.start_year);

    // set up events
    load_custom_messages();

    empire_init_scenario();
    traders_clear();
    city_military_determine_distant_battle_city();
    image_load_climate(scenario.climate, 0, 0);

    city_data_init_scenario();
    game_state_unpause();
}

static void initialize_saved_game(void)
{
    scenario.empire.distant_battle_roman_travel_months = empire_object_init_distant_battle_travel_months(EMPIRE_OBJECT_ROMAN_ARMY);
    scenario.empire.distant_battle_enemy_travel_months = empire_object_init_distant_battle_travel_months(EMPIRE_OBJECT_ENEMY_ARMY);

    map_grid_init(scenario.map.width, scenario.map.height, scenario.map.grid_start, scenario.map.grid_border_size);

    city_view_init();

    map_routing_update_all();

    map_orientation_update_buildings();
    figure_route_clean();
    map_road_network_update();
    building_granaries_calculate_stocks();
    map_building_menu_items();
    city_message_init_problem_areas();

    initialize_city_sounds();

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
    string_copy(scenario_selected, scenario_file, FILE_NAME_MAX - 1);
    if (!file_has_extension(scenario_file, "map")) {
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

    // reset trade prices
    for (int i = 0; i < RESOURCE_TYPES_MAX; i++) {
        trade_prices[i] = DEFAULT_PRICES[i];
    }

    if (file_has_extension(scenario_file, "map")) {
        file_remove_extension(scenario_file);
    }
    initialize_scenario_data(scenario_file); // requires map name only w/o ext
    set_player_name_from_config();
    city_emperor_init_scenario();
    map_building_menu_items();
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

    update_music(1);
    return 1;
}

