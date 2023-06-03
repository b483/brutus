#include "tool.h"

#include "building/construction_routed.h"
#include "city/view.h"
#include "core/image.h"
#include "core/image_group_editor.h"
#include "core/random.h"
#include "figuretype/water.h"
#include "game/undo.h"
#include "graphics/window.h"
#include "map/building_tiles.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/image.h"
#include "map/image_context.h"
#include "map/property.h"
#include "map/random.h"
#include "map/routing.h"
#include "map/routing_terrain.h"
#include "map/tiles.h"
#include "map/terrain.h"
#include "scenario/data.h"
#include "scenario/editor_events.h"
#include "city/warning.h"
#include "widget/map_editor_tool.h"
#include "widget/minimap.h"

#define TERRAIN_NOT_DISPLACEABLE TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_BUILDING | TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP

#define OFFSET(x,y) (x + GRID_SIZE * y)

static const int TILE_GRID_OFFSETS[] = { 0, GRID_SIZE, 1, GRID_SIZE + 1 };

static const int ACCESS_RAMP_TILE_OFFSETS_BY_ORIENTATION[4][6] = {
    {OFFSET(0,1), OFFSET(1,1), OFFSET(0,2), OFFSET(1,2), OFFSET(0,0), OFFSET(1,0)},
    {OFFSET(0,0), OFFSET(0,1), OFFSET(-1,0), OFFSET(-1,1), OFFSET(1,0), OFFSET(1,1)},
    {OFFSET(0,0), OFFSET(1,0), OFFSET(0,-1), OFFSET(1,-1), OFFSET(0,1), OFFSET(1,1)},
    {OFFSET(1,0), OFFSET(1,1), OFFSET(2,0), OFFSET(2,1), OFFSET(0,0), OFFSET(0,1)},
};

static struct {
    int active;
    int type;
    int id;
    int brush_size;
    int build_in_progress;
    int start_elevation;
    struct map_tile_t start_tile;
} data = { 0, TOOL_GRASS, 0, 2, 0, 0, {0} };

int editor_tool_type(void)
{
    return data.type;
}

int editor_tool_is_active(void)
{
    return data.active;
}

void editor_tool_deactivate(void)
{
    if (editor_tool_is_updatable() && data.build_in_progress) {
        game_undo_restore_map(1);
        data.build_in_progress = 0;
    } else {
        data.active = 0;
    }
}

void editor_tool_set_with_id(int type, int id)
{
    data.active = 1;
    data.type = type;
    data.id = id;
}

int editor_tool_brush_size(void)
{
    return data.brush_size;
}

void editor_tool_set_brush_size(int size)
{
    data.brush_size = size;
}

int editor_tool_is_updatable(void)
{
    return data.type == TOOL_ROAD;
}

int editor_tool_is_in_use(void)
{
    return data.build_in_progress;
}

void editor_tool_start_use(const struct map_tile_t *tile)
{
    if (!data.active) {
        return;
    }
    data.build_in_progress = 1;
    data.start_elevation = terrain_elevation.items[tile->grid_offset];
    data.start_tile = *tile;
    if (data.type == TOOL_ROAD) {
        game_undo_start_build(BUILDING_ROAD);
        map_routing_update_land();
    }
}

void draw_brush(int x, int y)
{
    draw_flat_tile(x, y, COLOR_MASK_GREEN);
    int tiles_remaining = data.brush_size;
    while (tiles_remaining) {
        for (int i = 1; i <= tiles_remaining; i++) {
            draw_flat_tile(x + HALF_TILE_WIDTH_PIXELS * (i + (data.brush_size - tiles_remaining)), y - HALF_TILE_HEIGHT_PIXELS * (i - (data.brush_size - tiles_remaining)), COLOR_MASK_GREEN);
            draw_flat_tile(x + HALF_TILE_WIDTH_PIXELS * (i - (data.brush_size - tiles_remaining)), y + HALF_TILE_HEIGHT_PIXELS * (i + (data.brush_size - tiles_remaining)), COLOR_MASK_GREEN);
            draw_flat_tile(x - HALF_TILE_WIDTH_PIXELS * (i + (data.brush_size - tiles_remaining)), y + HALF_TILE_HEIGHT_PIXELS * (i - (data.brush_size - tiles_remaining)), COLOR_MASK_GREEN);
            draw_flat_tile(x - HALF_TILE_WIDTH_PIXELS * (i - (data.brush_size - tiles_remaining)), y - HALF_TILE_HEIGHT_PIXELS * (i + (data.brush_size - tiles_remaining)), COLOR_MASK_GREEN);
        }
        tiles_remaining--;
    }
}

static void place_rock_at(int x, int y, int grid_offset, int size)
{
    int x_size_adjusted = size > 1 ? size - 1 : size; // use x - 1 for medium/large rocks; technically causes image overlap but looks better
    // check if all needed tiles are clear and within the map grid
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < size; dx++) { // separate full x-size check to avoid protrusion beyond the map grid
            if (!map_grid_is_inside(x + dx, y - dy, 1)) {
                return;
            }
        }
        for (int dx = 0; dx < x_size_adjusted; dx++) {
            if (map_terrain_is(map_grid_offset(x + dx, y - dy), TERRAIN_NOT_DISPLACEABLE)) {
                return;
            }
        }
    }
    // set terrain rock for all covered tiles
    for (int dy = 0; dy < size; dy++) {
        for (int dx = 0; dx < x_size_adjusted; dx++) {
            terrain_grid.items[map_grid_offset(x + dx, y - dy)] = TERRAIN_ROCK;
        }
    }
    // set multitile rock image
    int image_id;
    if (size == 1) { // small rock
        image_id = map_random_get(grid_offset) & 7;
    } else if (size == 2) { // medium rock
        image_id = 8 + (map_random_get(grid_offset) & 3);
    } else { // large rock
        image_id = 12 + (map_random_get(grid_offset) & 1);
    }
    if (map_terrain_exists_tile_in_radius_with_type(x, y, size, 4, TERRAIN_ELEVATION)) {
        image_id += image_group(GROUP_TERRAIN_ELEVATION_ROCK);
    } else {
        image_id += image_group(GROUP_TERRAIN_ROCK);
    }
    map_image_set(grid_offset, image_id);
}

static void add_terrain_at(int x, int y)
{
    if (!map_grid_is_inside(x, y, 1)) {
        return;
    }
    int grid_offset = map_grid_offset(x, y);
    int shrub_rnd = map_random_get(grid_offset) & 7;
    int elevation = terrain_elevation.items[grid_offset];
    switch (data.type) {
        case TOOL_GRASS:
            if (!map_terrain_is(grid_offset, TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP)) {
                if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
                    map_building_tiles_remove(0, x, y);
                }
                terrain_grid.items[grid_offset] = 0;
            }
            break;
        case TOOL_SMALL_SHRUB:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_SHRUB;
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_SHRUB) + shrub_rnd);
            }
            break;
        case TOOL_MEDIUM_SHRUB:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_SHRUB;
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_SHRUB) + shrub_rnd + 8);
            }
            break;
        case TOOL_LARGE_SHRUB:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_SHRUB;
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_SHRUB) + shrub_rnd + 16);
            }
            break;
        case TOOL_LARGEST_SHRUB:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_SHRUB;
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_SHRUB) + shrub_rnd + 24);
            }
            break;
        case TOOL_SMALL_ROCK:
            place_rock_at(x, y, grid_offset, 1);
            break;
        case TOOL_MEDIUM_ROCK:
            place_rock_at(x, y, grid_offset, 2);
            break;
        case TOOL_LARGE_ROCK:
            place_rock_at(x, y, grid_offset, 3);
            break;
        case TOOL_WATER:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_WATER;
            }
            break;
        case TOOL_TREES:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_TREE;
                map_image_set(grid_offset, image_group(GROUP_TERRAIN_TREE) + (map_random_get(grid_offset) & 7));
            }
            break;
        case TOOL_MEADOW:
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_DISPLACEABLE)) {
                terrain_grid.items[grid_offset] = TERRAIN_MEADOW;
                // dupl with set_meadow_image
                int random = map_random_get(grid_offset) & 3;
                int image_id = image_group(GROUP_TERRAIN_MEADOW);
                if (map_terrain_all_tiles_in_radius_are(x, y, 1, 2, TERRAIN_MEADOW)) {
                    map_image_set(grid_offset, image_id + random + 8);
                } else if (map_terrain_all_tiles_in_radius_are(x, y, 1, 1, TERRAIN_MEADOW)) {
                    map_image_set(grid_offset, image_id + random + 4);
                } else {
                    map_image_set(grid_offset, image_id + random);
                }
            }
            break;
        case TOOL_RAISE_LAND:
            if (elevation < 5 && elevation == data.start_elevation) {
                map_property_set_multi_tile_size(grid_offset, 1);
                terrain_elevation.items[grid_offset] = elevation + 1;
                terrain_grid.items[grid_offset] = TERRAIN_ELEVATION;
            }
            break;
        case TOOL_LOWER_LAND:
            if (elevation == data.start_elevation) {
                if (elevation == 1) {
                    terrain_elevation.items[grid_offset] = 0;
                    terrain_grid.items[grid_offset] = 0;
                } else if (elevation) {
                    terrain_elevation.items[grid_offset] = elevation - 1;
                    terrain_grid.items[grid_offset] = TERRAIN_ELEVATION;
                }
            }
            break;
        default:
            break;
    }
}

void editor_tool_update_use(const struct map_tile_t *tile)
{
    if (!data.build_in_progress) {
        return;
    }
    if (data.type == TOOL_ROAD) {
        building_construction_place_road(1, data.start_tile.x, data.start_tile.y, tile->x, tile->y);
        return;
    }
    switch (data.type) {
        case TOOL_GRASS:
        case TOOL_SMALL_SHRUB:
        case TOOL_MEDIUM_SHRUB:
        case TOOL_LARGE_SHRUB:
        case TOOL_LARGEST_SHRUB:
        case TOOL_WATER:
        case TOOL_TREES:
        case TOOL_SMALL_ROCK:
        case TOOL_MEDIUM_ROCK:
        case TOOL_LARGE_ROCK:
        case TOOL_MEADOW:
        case TOOL_RAISE_LAND:
        case TOOL_LOWER_LAND:
            break;
        default:
            return;
    }

    add_terrain_at(tile->x, tile->y);
    int tiles_remaining = data.brush_size;
    while (tiles_remaining) {
        for (int i = 1; i <= tiles_remaining; i++) {
            add_terrain_at(tile->x + (data.brush_size - tiles_remaining), tile->y - i); // top to right
            add_terrain_at(tile->x + i, tile->y + (data.brush_size - tiles_remaining)); // right to bottom
            add_terrain_at(tile->x - (data.brush_size - tiles_remaining), tile->y + i); // bottom to left
            add_terrain_at(tile->x - i, tile->y - (data.brush_size - tiles_remaining)); // left to top
        }
        tiles_remaining--;
    }

    int x_min = tile->x - data.brush_size;
    int x_max = tile->x + data.brush_size;
    int y_min = tile->y - data.brush_size;
    int y_max = tile->y + data.brush_size;
    map_image_context_reset_water();
    map_tiles_update_region_water(x_min, y_min, x_max, y_max);
    switch (data.type) {
        case TOOL_GRASS:
            map_tiles_update_region_water(x_min, y_min, x_max, y_max);
            map_tiles_update_region_empty_land(x_min, y_min, x_max, y_max);
            break;
        case TOOL_RAISE_LAND:
        case TOOL_LOWER_LAND:
            map_image_context_reset_water();
            map_image_context_reset_elevation();
            map_tiles_update_all_elevation();
            map_tiles_update_region_water(x_min, y_min, x_max, y_max);
            map_tiles_update_region_empty_land(x_min, y_min, x_max, y_max);
            break;
        default:
            break;
    }

    scenario.is_saved = 0;
    widget_minimap_invalidate();
}

static void place_building(const struct map_tile_t *tile)
{
    int image_id;
    int size;
    int type;
    switch (data.type) {
        case TOOL_NATIVE_HUT:
            type = BUILDING_NATIVE_HUT;
            image_id = image_group(GROUP_EDITOR_BUILDING_NATIVE) + (random_byte() & 1);
            size = 1;
            break;
        case TOOL_NATIVE_CENTER:
            type = BUILDING_NATIVE_MEETING;
            image_id = image_group(GROUP_EDITOR_BUILDING_NATIVE) + 2;
            size = 2;
            break;
        case TOOL_NATIVE_FIELD:
            type = BUILDING_NATIVE_CROPS;
            image_id = image_group(GROUP_EDITOR_BUILDING_CROPS);
            size = 1;
            break;
        case TOOL_HOUSE_VACANT_LOT:
            type = BUILDING_HOUSE_VACANT_LOT;
            image_id = image_group(GROUP_EDITOR_BUILDING_NATIVE) - 4;
            size = 1;
            break;
        default:
            return;
    }

    if (editor_tool_can_place_building(tile, size * size, 0)) {
        struct building_t *b = building_create(type, tile->x, tile->y);
        map_building_tiles_add(b->id, tile->x, tile->y, size, image_id, TERRAIN_BUILDING);
        scenario.is_saved = 0;
    } else {
        city_warning_show(WARNING_EDITOR_CANNOT_PLACE);
    }
}

static void update_terrain_after_elevation_changes(void)
{
    map_elevation_remove_cliffs();

    map_image_context_reset_water();
    map_image_context_reset_elevation();
    map_tiles_update_all_elevation();
    map_tiles_update_all_empty_land();

    scenario.is_saved = 0;
}

static void place_access_ramp(const struct map_tile_t *tile)
{
    int orientation = 0;
    if (editor_tool_can_place_access_ramp(tile, &orientation)) {
        int terrain_mask = ~(TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_BUILDING | TERRAIN_GARDEN | TERRAIN_AQUEDUCT);
        for (int dy = 0; dy < 2; dy++) {
            for (int dx = 0; dx < 2; dx++) {
                int grid_offset = tile->grid_offset + map_grid_delta(dx, dy);
                terrain_grid.items[grid_offset] = terrain_grid.items[grid_offset] & terrain_mask;
            }
        }
        map_building_tiles_add(0, tile->x, tile->y, 2,
            image_group(GROUP_TERRAIN_ACCESS_RAMP) + orientation, TERRAIN_ACCESS_RAMP);

        update_terrain_after_elevation_changes();
        scenario.is_saved = 0;
    } else {
        city_warning_show(WARNING_EDITOR_CANNOT_PLACE);
    }
}

void editor_tool_end_use(const struct map_tile_t *tile)
{
    if (!data.build_in_progress) {
        return;
    }
    data.build_in_progress = 0;
    if (!tile->grid_offset) {
        return;
    }

    int warning = 0;
    switch (data.type) {
        case TOOL_ENTRY_POINT:
            if (editor_tool_can_place_flag(data.type, tile, &warning)) {
                scenario.entry_point.x = tile->x;
                scenario.entry_point.y = tile->y;
            }
            break;
        case TOOL_EXIT_POINT:
            if (editor_tool_can_place_flag(data.type, tile, &warning)) {
                scenario.exit_point.x = tile->x;
                scenario.exit_point.y = tile->y;
            }
            break;
        case TOOL_RIVER_ENTRY_POINT:
            if (editor_tool_can_place_flag(data.type, tile, &warning)) {
                scenario.river_entry_point.x = tile->x;
                scenario.river_entry_point.y = tile->y;
                figure_create_flotsam();
                map_routing_update_water();
            }
            break;
        case TOOL_RIVER_EXIT_POINT:
            if (editor_tool_can_place_flag(data.type, tile, &warning)) {
                scenario.river_exit_point.x = tile->x;
                scenario.river_exit_point.y = tile->y;
                figure_create_flotsam();
                map_routing_update_water();
            }
            break;
        case TOOL_EARTHQUAKE_POINT:
            if (editor_tool_can_place_flag(data.type, tile, &warning)) {
                scenario.earthquake_points[data.id].x = tile->x;
                scenario.earthquake_points[data.id].y = tile->y;
                window_request_refresh();
            }
            break;
        case TOOL_INVASION_POINT:
            if (editor_tool_can_place_flag(data.type, tile, &warning)) {
                scenario.invasion_points[data.id].x = tile->x;
                scenario.invasion_points[data.id].y = tile->y;
            }
            break;
        case TOOL_FISHING_POINT:
            if (editor_tool_can_place_flag(data.type, tile, &warning)) {
                scenario.fishing_points[data.id].x = tile->x;
                scenario.fishing_points[data.id].y = tile->y;
            }
            break;
        case TOOL_HERD_POINT:
            if (editor_tool_can_place_flag(data.type, tile, &warning)) {
                scenario.herd_points[data.id].x = tile->x;
                scenario.herd_points[data.id].y = tile->y;
            }
            break;
        case TOOL_NATIVE_CENTER:
        case TOOL_NATIVE_FIELD:
        case TOOL_NATIVE_HUT:
        case TOOL_HOUSE_VACANT_LOT:
            place_building(tile);
            break;
        case TOOL_RAISE_LAND:
        case TOOL_LOWER_LAND:
            update_terrain_after_elevation_changes();
            break;
        case TOOL_ACCESS_RAMP:
            place_access_ramp(tile);
            break;
        case TOOL_ROAD:
            building_construction_place_road(0, data.start_tile.x, data.start_tile.y, tile->x, tile->y);
            break;
        default:
            break;
    }
    scenario.is_saved = 0;
    if (warning) {
        city_warning_show(warning);
    }
}

static int is_clear_terrain(const struct map_tile_t *tile, int *warning)
{
    int result = !map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR ^ TERRAIN_ROAD);
    if (!result && warning) {
        *warning = WARNING_EDITOR_CANNOT_PLACE;
    }
    return result;
}

static int is_edge(const struct map_tile_t *tile, int *warning)
{
    int result = tile->x == 0 || tile->y == 0 || tile->x == map_data.width - 1 || tile->y == map_data.height - 1;
    if (!result && warning) {
        *warning = WARNING_EDITOR_NEED_MAP_EDGE;
    }
    return result;
}

static int is_water(const struct map_tile_t *tile, int *warning)
{
    int result = map_terrain_is(tile->grid_offset, TERRAIN_WATER);
    if (!result && warning) {
        *warning = WARNING_EDITOR_NEED_OPEN_WATER;
    }
    return result;
}

static int is_deep_water(const struct map_tile_t *tile, int *warning)
{
    int result = map_terrain_is(tile->grid_offset, TERRAIN_WATER) &&
        map_terrain_count_directly_adjacent_with_type(tile->grid_offset, TERRAIN_WATER) == 4;
    if (!result && warning) {
        *warning = WARNING_EDITOR_NEED_OPEN_WATER;
    }
    return result;
}

int editor_tool_can_place_flag(int type, const struct map_tile_t *tile, int *warning)
{
    switch (type) {
        case TOOL_ENTRY_POINT:
        case TOOL_EXIT_POINT:
        case TOOL_INVASION_POINT:
            return is_clear_terrain(tile, warning) && is_edge(tile, warning);

        case TOOL_EARTHQUAKE_POINT:
        case TOOL_HERD_POINT:
            return is_clear_terrain(tile, warning);

        case TOOL_FISHING_POINT:
            return is_water(tile, warning);

        case TOOL_RIVER_ENTRY_POINT:
        case TOOL_RIVER_EXIT_POINT:
            return is_edge(tile, warning) && is_deep_water(tile, warning);

        default:
            return 0;
    }
}

int editor_tool_can_place_access_ramp(const struct map_tile_t *tile, int *orientation_index)
{
    if (!map_grid_is_inside(tile->x, tile->y, 2)) {
        return 0;
    }
    for (int orientation = 0; orientation < 4; orientation++) {
        int right_tiles = 0;
        int wrong_tiles = 0;
        int top_elevation = 0;
        for (int index = 0; index < 6; index++) {
            int tile_offset = tile->grid_offset + ACCESS_RAMP_TILE_OFFSETS_BY_ORIENTATION[orientation][index];
            int elevation = terrain_elevation.items[tile_offset];
            if (index < 2) {
                if (map_terrain_is(tile_offset, TERRAIN_ELEVATION)) {
                    right_tiles++;
                } else {
                    wrong_tiles++;
                }
                top_elevation = elevation;
            } else if (index < 4) {
                if (map_terrain_is(tile_offset, TERRAIN_ELEVATION)) {
                    if (elevation == top_elevation) {
                        wrong_tiles++;
                    } else {
                        right_tiles++;
                    }
                } else if (elevation >= top_elevation) {
                    right_tiles++;
                } else {
                    wrong_tiles++;
                }
            } else {
                if (map_terrain_is(tile_offset, TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP)) {
                    wrong_tiles++;
                } else if (elevation >= top_elevation) {
                    wrong_tiles++;
                } else {
                    right_tiles++;
                }
            }
        }
        if (right_tiles == 6) {
            if (orientation_index) {
                *orientation_index = orientation;
            }
            return 1;
        }
    }
    return 0;
}

int editor_tool_can_place_building(const struct map_tile_t *tile, int num_tiles, int *blocked_tiles)
{
    int blocked = 0;
    for (int i = 0; i < num_tiles; i++) {
        int tile_offset = tile->grid_offset + TILE_GRID_OFFSETS[i];
        if ((terrain_grid.items[tile_offset] & TERRAIN_NOT_CLEAR) || map_has_figure_at(tile_offset)) {
            blocked = 1;
            if (blocked_tiles) blocked_tiles[i] = 1;
        } else {
            if (blocked_tiles) blocked_tiles[i] = 0;
        }
    }
    return !blocked;
}