#include "file_editor.h"

#include "building/construction.h"
#include "building/storage.h"
#include "city/data.h"
#include "city/message.h"
#include "city/victory.h"
#include "city/view.h"
#include "core/image.h"
#include "core/image_group_editor.h"
#include "empire/empire.h"
#include "empire/object.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "figure/name.h"
#include "figure/route.h"
#include "figure/trader.h"
#include "figuretype/editor.h"
#include "figuretype/water.h"
#include "game/animation.h"
#include "game/file_io.h"
#include "game/state.h"
#include "game/time.h"
#include "map/aqueduct.h"
#include "map/building.h"
#include "map/desirability.h"
#include "map/figure.h"
#include "map/image.h"
#include "map/image_context.h"
#include "map/natives.h"
#include "map/property.h"
#include "map/random.h"
#include "map/road_network.h"
#include "map/routing_terrain.h"
#include "map/sprite.h"
#include "map/terrain.h"
#include "map/tiles.h"
#include "scenario/data.h"
#include "scenario/editor_events.h"
#include "scenario/map.h"
#include "scenario/scenario.h"
#include "sound/city.h"
#include "sound/music.h"

void game_file_editor_clear_data(void)
{
    city_victory_reset();
    building_construction_clear_type();
    city_data_init();
    city_data_init_scenario();
    city_message_init_scenario();
    game_state_init();
    game_animation_init();
    sound_city_init();
    building_clear_all();
    building_storage_clear_all();
    figure_init_scenario();
    figure_name_init();
    reset_all_formations();
    figure_route_clear_all();
    traders_clear();
    game_time_init(2098);
}

static void map_terrain_init_outside_map(void)
{
    int map_width, map_height;
    map_grid_size(&map_width, &map_height);
    int y_start = (GRID_SIZE - map_height) / 2;
    int x_start = (GRID_SIZE - map_width) / 2;
    for (int y = 0; y < GRID_SIZE; y++) {
        int y_outside_map = y < y_start || y >= y_start + map_height;
        for (int x = 0; x < GRID_SIZE; x++) {
            if (y_outside_map || x < x_start || x >= x_start + map_width) {
                terrain_grid.items[x + GRID_SIZE * y] = TERRAIN_SHRUB | TERRAIN_WATER;
            }
        }
    }
}

static void clear_map_data(void)
{
    map_image_clear();
    map_building_clear();
    map_terrain_clear();
    map_aqueduct_clear();
    map_grid_clear_u16(map_figures.items);
    map_property_clear();
    map_sprite_clear();
    map_random_clear();
    map_desirability_clear();
    map_grid_clear_u8(terrain_elevation.items);
    map_road_network_clear();

    map_image_context_init();
    map_terrain_init_outside_map();
    map_random_init();
    map_property_init_alternate_terrain();
}

static void create_blank_map(int size)
{
    scenario_editor_create(size);
    scenario_map_init();
    clear_map_data();
    map_image_init_edges();
    city_view_set_camera(76, 152);
    city_view_reset_orientation();
}

static void prepare_map_for_editing(int map_is_new)
{
    image_load_climate(scenario.climate, 1, 0);

    if (map_is_new) {
        empire_load(0);
        empire_object_our_city_set_resources_sell();
    }

    figure_init_scenario();
    figure_create_editor_flags();
    figure_create_flotsam();

    map_tiles_update_all_elevation();
    map_tiles_update_all_earthquake();
    map_tiles_update_all_empty_land();
    map_tiles_update_all_roads();
    map_tiles_update_all_plazas();
    map_tiles_update_all_walls();
    map_tiles_update_all_aqueducts(0);
    map_natives_init_editor();
    map_routing_update_all();

    city_view_init();
    game_state_unpause();
}

void game_file_editor_create_scenario(int size)
{
    create_blank_map(size);
    prepare_map_for_editing(1);
}

int game_file_editor_load_scenario(const char *dir, const char *scenario_file)
{
    clear_map_data();
    if (!game_file_io_read_scenario(dir, scenario_file)) {
        return 0;
    }
    scenario_map_init();

    prepare_map_for_editing(0);
    return 1;
}

int game_file_editor_write_scenario(const char *dir, const char *scenario_file)
{
    scenario.native_images.hut = image_group(GROUP_EDITOR_BUILDING_NATIVE);
    scenario.native_images.meeting = image_group(GROUP_EDITOR_BUILDING_NATIVE) + 2;
    scenario.native_images.crops = image_group(GROUP_EDITOR_BUILDING_CROPS);
    scenario.empire.distant_battle_roman_travel_months = empire_object_init_distant_battle_travel_months(EMPIRE_OBJECT_ROMAN_ARMY);
    scenario.empire.distant_battle_enemy_travel_months = empire_object_init_distant_battle_travel_months(EMPIRE_OBJECT_ENEMY_ARMY);

    return game_file_io_write_scenario(dir, scenario_file);
}
