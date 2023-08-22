#ifndef MAP_MAP_H
#define MAP_MAP_H

#include "core/buffer.h"
#include "figure/figure.h"

#define OFFSET(x,y) (x + GRID_SIZE * y)

enum {
    GRID_SIZE = 162
};

enum {
    TERRAIN_SHRUB = 1,
    TERRAIN_ROCK = 2,
    TERRAIN_WATER = 4,
    TERRAIN_BUILDING = 8,
    TERRAIN_TREE = 16,
    TERRAIN_GARDEN = 32,
    TERRAIN_ROAD = 64,
    TERRAIN_RESERVOIR_RANGE = 128,
    TERRAIN_AQUEDUCT = 256,
    TERRAIN_ELEVATION = 512,
    TERRAIN_ACCESS_RAMP = 1024,
    TERRAIN_MEADOW = 2048,
    TERRAIN_RUBBLE = 4096,
    TERRAIN_FOUNTAIN_RANGE = 8192,
    TERRAIN_WALL = 16384,
    TERRAIN_GATEHOUSE = 32768,
    // combined
    TERRAIN_WALL_OR_GATEHOUSE = TERRAIN_WALL | TERRAIN_GATEHOUSE,
    TERRAIN_NOT_CLEAR = TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_BUILDING | TERRAIN_TREE | TERRAIN_GARDEN | TERRAIN_ROAD | TERRAIN_AQUEDUCT | TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP | TERRAIN_RUBBLE | TERRAIN_WALL | TERRAIN_GATEHOUSE,
    TERRAIN_CLEARABLE = TERRAIN_SHRUB | TERRAIN_BUILDING | TERRAIN_TREE | TERRAIN_GARDEN | TERRAIN_ROAD | TERRAIN_AQUEDUCT | TERRAIN_RUBBLE | TERRAIN_WALL | TERRAIN_GATEHOUSE,
    TERRAIN_IMPASSABLE = TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_BUILDING | TERRAIN_TREE | TERRAIN_ELEVATION | TERRAIN_WALL | TERRAIN_GATEHOUSE,
    TERRAIN_ALL = 65535
};

enum {
    ROUTED_BUILDING_ROAD = 0,
    ROUTED_BUILDING_WALL = 1,
    ROUTED_BUILDING_AQUEDUCT = 2,
    ROUTED_BUILDING_AQUEDUCT_WITHOUT_GRAPHIC = 4,
};

enum {
    CITIZEN_0_ROAD = 0,
    CITIZEN_2_PASSABLE_TERRAIN = 2,
    CITIZEN_4_CLEAR_TERRAIN = 4,
    CITIZEN_N1_BLOCKED = -1,
    CITIZEN_N3_AQUEDUCT = -3,
    CITIZEN_N4_RESERVOIR_CONNECTOR = -4,

    NONCITIZEN_0_PASSABLE = 0,
    NONCITIZEN_1_BUILDING = 1,
    NONCITIZEN_2_CLEARABLE = 2,
    NONCITIZEN_3_WALL = 3,
    NONCITIZEN_4_GATEHOUSE = 4,
    NONCITIZEN_5_FORT = 5,
    NONCITIZEN_N1_BLOCKED = -1,

    WATER_0_PASSABLE = 0,
    WATER_N1_BLOCKED = -1,
    WATER_N2_MAP_EDGE = -2,
    WATER_N3_LOW_BRIDGE = -3,

    WALL_0_PASSABLE = 0,
    WALL_N1_BLOCKED = -1,
};

enum {
    EDGE_X0Y0 = 0,
    EDGE_X1Y0 = 1,
    EDGE_X2Y0 = 2,
    EDGE_X0Y1 = 8,
    EDGE_X1Y1 = 9,
    EDGE_X2Y1 = 10,
    EDGE_X0Y2 = 16,
    EDGE_X1Y2 = 17,
    EDGE_X2Y2 = 18
};

struct map_point_t {
    int x;
    int y;
};

struct map_tile_t {
    int x;
    int y;
    int grid_offset;
};

extern struct grid_u16_t terrain_grid;

extern struct grid_u8_t terrain_elevation;

extern struct grid_i8_t terrain_land_citizen;
extern struct grid_i8_t terrain_land_noncitizen;
extern struct grid_i8_t terrain_water;
extern struct grid_i8_t terrain_walls;

struct terrain_image_t {
    int is_valid;
    int group_offset;
    int item_offset;
    int aqueduct_offset;
};

struct grid_u8_t {
    uint8_t items[GRID_SIZE * GRID_SIZE];
};

struct grid_i8_t {
    int8_t items[GRID_SIZE * GRID_SIZE];
};

struct grid_u16_t {
    uint16_t items[GRID_SIZE * GRID_SIZE];
};

struct grid_i16_t {
    int16_t items[GRID_SIZE * GRID_SIZE];
};

extern struct map_data_t {
    int width;
    int height;
    int start_offset;
    int border_size;
} map_data;

int map_aqueduct_at(int grid_offset);

void map_aqueduct_set(int grid_offset, int value);

/**
 * Removes aqueduct and updates adjacent aqueduct tiles
 * @param grid_offset Offset
 */
void map_aqueduct_remove(int grid_offset);

void map_aqueduct_clear(void);

void map_aqueduct_backup(void);

void map_aqueduct_restore(void);

void map_aqueduct_save_state(struct buffer_t *buf, struct buffer_t *backup);

void map_aqueduct_load_state(struct buffer_t *buf, struct buffer_t *backup);

void map_bookmarks_clear(void);

void map_bookmark_save(int number);

int map_bookmark_go_to(int number);

void map_bookmark_save_state(struct buffer_t *buf);

void map_bookmark_load_state(struct buffer_t *buf);

int map_bridge_building_length(void);

void map_bridge_reset_building_length(void);

int map_bridge_calculate_length_direction(int x, int y, int *length, int *direction);

int map_bridge_get_sprite_id(int index, int length, int direction, int is_ship_bridge);
/**
 * Adds a bridge to the terrain
 * @param x Map X
 * @param y Map Y
 * @param is_ship_bridge Whether this is a ship bridge or low bridge
 * @return Length of the bridge
 */
int map_bridge_add(int x, int y, int is_ship_bridge);

void map_bridge_remove(int grid_offset, int mark_deleted);

int map_bridge_count_figures(int grid_offset);

int map_is_bridge(int grid_offset);

void map_building_tiles_add(int building_id, int x, int y, int size, int image_id, int terrain);

void map_building_tiles_add_farm(int building_id, int x, int y, int crop_image_id, int progress);

void map_building_tiles_remove(int building_id, int x, int y);

void map_building_tiles_set_rubble(int building_id, int x, int y, int size);

void map_building_tiles_mark_deleting(int grid_offset);

int map_building_tiles_mark_construction(int x, int y, int size, int terrain, int absolute_xy);

int map_building_tiles_are_clear(int x, int y, int size, int terrain);

/**
 * Returns the building at the given offset
 * @param grid_offset Map offset
 * @return Building ID of building at offset, 0 means no building
 */
int map_building_at(int grid_offset);

void map_building_set(int grid_offset, int building_id);

/**
 * Increases building damage by 1
 * @param grid_offset Map offset
 * @return New damage amount
 */
int map_building_damage_increase(int grid_offset);

void map_building_damage_clear(int grid_offset);

int map_rubble_building_type(int grid_offset);

void map_set_rubble_building_type(int grid_offset, int type);

/**
 * Clears the maps related to buildings
 */
void map_building_clear(void);

void map_building_save_state(struct buffer_t *buildings, struct buffer_t *damage);

void map_building_load_state(struct buffer_t *buildings, struct buffer_t *damage);

void map_desirability_clear(void);

void map_desirability_update(void);

int map_desirability_get(int grid_offset);

int map_desirability_get_max(int x, int y, int size);

void map_desirability_save_state(struct buffer_t *buf);

void map_desirability_load_state(struct buffer_t *buf);

extern struct grid_u16_t map_figures;

/**
 * Returns the first figure at the given offset
 * @param grid_offset Map offset
 * @return Figure ID of first figure at offset
 */
int map_figure_at(int grid_offset);

/**
 * Returns whether there is a figure at the given offset
 * @param grid_offset Map offset
 * @return True if there is a figure, otherwise false
 */
int map_has_figure_at(int grid_offset);

void map_figure_add(struct figure_t *f);

void map_figure_update(struct figure_t *f);

void map_figure_delete(struct figure_t *f);

void map_figure_save_state(struct buffer_t *buf);

void map_figure_load_state(struct buffer_t *buf);

void map_grid_init(int width, int height, int start_offset, int border_size);

int map_grid_is_valid_offset(int grid_offset);

int map_grid_offset(int x, int y);

int map_grid_offset_to_x(int grid_offset);

int map_grid_offset_to_y(int grid_offset);

int map_grid_delta(int x, int y);

int map_grid_direction_delta(int direction);

void map_grid_size(int *width, int *height);

void map_grid_bound(int *x, int *y);

void map_grid_bound_area(int *x_min, int *y_min, int *x_max, int *y_max);

void map_grid_get_area(int x, int y, int size, int radius, int *x_min, int *y_min, int *x_max, int *y_max);

int map_grid_is_inside(int x, int y, int size);

const int *map_grid_adjacent_offsets(int size);

void map_image_context_init(void);
void map_image_context_reset_water(void);
void map_image_context_reset_elevation(void);

const struct terrain_image_t *map_image_context_get_aqueduct(int grid_offset, int include_construction);

int map_image_at(int grid_offset);

void map_image_set(int grid_offset, int image_id);

void map_image_backup(void);

void map_image_restore(void);

void map_image_restore_at(int grid_offset);

void map_image_clear(void);
void map_image_init_edges(void);

void map_image_save_state(struct buffer_t *buf);

void map_image_load_state(struct buffer_t *buf);

void map_natives_init(void);
void map_natives_init_editor(void);

void map_natives_check_land(void);

void map_orientation_change(int counter_clockwise);

int map_orientation_for_gatehouse(int x, int y);

int map_orientation_for_triumphal_arch(int x, int y);

void map_orientation_update_buildings(void);

int map_property_is_draw_tile(int grid_offset);
void map_property_mark_draw_tile(int grid_offset);
void map_property_clear_draw_tile(int grid_offset);

int map_property_is_native_land(int grid_offset);
void map_property_mark_native_land(int grid_offset);
void map_property_clear_all_native_land(void);

int map_property_multi_tile_xy(int grid_offset);
int map_property_multi_tile_x(int grid_offset);
int map_property_multi_tile_y(int grid_offset);
int map_property_is_multi_tile_xy(int grid_offset, int x, int y);
void map_property_set_multi_tile_xy(int grid_offset, int x, int y, int is_draw_tile);
void map_property_clear_multi_tile_xy(int grid_offset);

int map_property_multi_tile_size(int grid_offset);
void map_property_set_multi_tile_size(int grid_offset, int size);

void map_property_init_alternate_terrain(void);
int map_property_is_alternate_terrain(int grid_offset);

int map_property_is_plaza_or_earthquake(int grid_offset);
void map_property_mark_plaza_or_earthquake(int grid_offset);
void map_property_clear_plaza_or_earthquake(int grid_offset);

int map_property_is_constructing(int grid_offset);
void map_property_mark_constructing(int grid_offset);
void map_property_clear_constructing(int grid_offset);

int map_property_is_deleted(int grid_offset);
void map_property_mark_deleted(int grid_offset);
void map_property_clear_deleted(int grid_offset);

void map_property_clear_constructing_and_deleted(void);

void map_property_clear(void);

void map_property_backup(void);
void map_property_restore(void);

void map_property_save_state(struct buffer_t *bitfields, struct buffer_t *edge);
void map_property_load_state(struct buffer_t *bitfields, struct buffer_t *edge);

void map_random_clear(void);

void map_random_init(void);

int map_random_get(int grid_offset);

void map_random_save_state(struct buffer_t *buf);

void map_random_load_state(struct buffer_t *buf);

int map_has_road_access(int x, int y, int size, struct map_point_t *road);

int map_has_road_access_hippodrome(int x, int y, struct map_point_t *road);

int map_has_road_access_granary(int x, int y, struct map_point_t *road);

int map_closest_road_within_radius(int x, int y, int size, int radius, int *x_road, int *y_road);

int map_closest_reachable_road_within_radius(int x, int y, int size, int radius, int *x_road, int *y_road);

int map_road_to_largest_network(int x, int y, int size, int *x_road, int *y_road);

int map_road_to_largest_network_hippodrome(int x, int y, int *x_road, int *y_road);

int map_can_place_road_under_aqueduct(int grid_offset);

int map_get_aqueduct_with_road_image(int grid_offset);

void map_road_network_clear(void);

int map_road_network_get(int grid_offset);

void map_road_network_update(void);

int map_routing_get_path(uint8_t *path, int src_x, int src_y, int dst_x, int dst_y, int num_directions);

int map_routing_get_path_on_water(uint8_t *path, int dst_x, int dst_y, int is_flotsam);

int map_routing_get_closest_tile_within_range(int src_x, int src_y, int dst_x, int dst_y, int num_directions, int range, int *out_x, int *out_y);

void map_routing_update_all(void);
void map_routing_update_land(void);
void map_routing_update_land_citizen(void);
void map_routing_update_water(void);
void map_routing_update_walls(void);

int map_routing_is_wall_passable(int grid_offset);

int map_routing_citizen_is_passable(int grid_offset);

void clear_distances(void);

void route_queue_dir8(int source, void (*callback)(int, int));

void map_routing_calculate_distances(int x, int y);
void map_routing_calculate_distances_water_boat(int x, int y);
void callback_calc_distance_water_flotsam(int next_offset, int dist);

int map_routing_calculate_distances_for_building(int type, int x, int y);

void map_routing_delete_first_wall_or_aqueduct(int x, int y);

int map_routing_distance(int grid_offset);

int map_routing_citizen_can_travel_over_land(int src_x, int src_y, int dst_x, int dst_y);
int map_routing_citizen_can_travel_over_road_garden(int src_x, int src_y, int dst_x, int dst_y);
int map_routing_can_travel_over_walls(int src_x, int src_y, int dst_x, int dst_y);

int map_routing_noncitizen_can_travel_over_land(int src_x, int src_y, int dst_x, int dst_y, int only_through_building_id, int max_tiles);
int map_routing_noncitizen_can_travel_through_everything(int src_x, int src_y, int dst_x, int dst_y);

void map_routing_save_state(struct buffer_t *buf);

void map_routing_load_state(struct buffer_t *buf);

int map_sprite_animation_at(int grid_offset);

void map_sprite_animation_set(int grid_offset, int value);

int map_sprite_bridge_at(int grid_offset);

void map_sprite_bridge_set(int grid_offset, int value);

void map_sprite_clear_tile(int grid_offset);

void map_sprite_clear(void);

void map_sprite_backup(void);

void map_sprite_restore(void);

void map_sprite_save_state(struct buffer_t *buf, struct buffer_t *backup);

void map_sprite_load_state(struct buffer_t *buf, struct buffer_t *backup);

int map_terrain_is(int grid_offset, int terrain);

int map_terrain_count_directly_adjacent_with_type(int grid_offset, int terrain);

int map_terrain_has_adjacent_x_with_type(int grid_offset, int terrain);

int map_terrain_has_adjacent_y_with_type(int grid_offset, int terrain);

int map_terrain_exists_tile_in_area_with_type(int x, int y, int size, int terrain);

int map_terrain_exists_tile_in_radius_with_type(int x, int y, int size, int radius, int terrain);

int map_terrain_exist_multiple_tiles_in_radius_with_type(int x, int y, int size, int radius, int terrain, int count);

int map_terrain_all_tiles_in_radius_are(int x, int y, int size, int radius, int terrain);

int map_terrain_is_adjacent_to_open_water(int x, int y, int size);

int map_terrain_get_adjacent_road_or_clear_land(int x, int y, int size, int *x_tile, int *y_tile);

void map_terrain_add_gatehouse_roads(int x, int y, int orientation);
void map_terrain_add_triumphal_arch_roads(int x, int y, int orientation);

void map_terrain_backup(void);

void map_terrain_restore(void);

void map_terrain_save_state(struct buffer_t *buf);

void map_terrain_load_state(struct buffer_t *buf);

void map_tiles_update_all_gardens(void);
void map_tiles_determine_gardens(void);

void map_tiles_update_all_plazas(void);

void set_wall_image(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset);

void map_tiles_update_all_walls(void);

int map_tiles_set_wall(int x, int y);

int map_tiles_is_paved_road(int grid_offset);

void set_road_image(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset);
void map_tiles_update_all_roads(void);

int map_tiles_set_road(int x, int y);

void map_tiles_update_all_empty_land(void);
void map_tiles_update_region_empty_land(int x_min, int y_min, int x_max, int y_max);

void update_meadow_tile(int x, int y, int grid_offset);

void update_water_tile(int x, int y, int grid_offset);

void map_tiles_update_all_water(void);

void update_aqueduct_tile(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset);

void map_tiles_update_all_aqueducts(int include_construction);

void set_earthquake_image(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset);

void map_tiles_update_all_earthquake(void);
void set_rubble_image(__attribute__((unused)) int x, __attribute__((unused)) int y, int grid_offset);

void map_tiles_update_all_elevation(void);

int is_clear(int x, int y, int size, int disallowed_terrain, int check_image);

void foreach_region_tile(int x_min, int y_min, int x_max, int y_max, void (*callback)(int x, int y, int grid_offset));

void map_tiles_add_entry_exit_flags(void);

void map_water_supply_update_reservoir_fountain(void);

void map_water_add_building(int building_id, int x, int y, int size, int image_id);

#endif