#include "map_editor_tool.h"

#include "building/building.h"
#include "core/image_group_editor.h"
#include "editor/tool.h"
#include "graphics/image.h"
#include "input/scroll.h"
#include "map/terrain.h"
#include "scenario/data.h"

#define MAX_TILES 4

static const int X_VIEW_OFFSETS[MAX_TILES] = { 0, -30, 30, 0 };
static const int Y_VIEW_OFFSETS[MAX_TILES] = { 0, 15, 15, 30 };

void draw_flat_tile(int x, int y, color_t color_mask)
{
    if (color_mask == COLOR_MASK_GREEN && scenario.climate != CLIMATE_DESERT) {
        image_draw_blend_alpha(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, ALPHA_MASK_SEMI_TRANSPARENT | color_mask);
    } else {
        image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, color_mask);
    }
}

static void draw_partially_blocked(int x, int y, int num_tiles, int *blocked_tiles)
{
    for (int i = 0; i < num_tiles; i++) {
        int x_offset = x + X_VIEW_OFFSETS[i];
        int y_offset = y + Y_VIEW_OFFSETS[i];
        if (blocked_tiles[i]) {
            draw_flat_tile(x_offset, y_offset, COLOR_MASK_RED);
        } else {
            draw_flat_tile(x_offset, y_offset, COLOR_MASK_GREEN);
        }
    }
}

static void draw_building_image(int image_id, int x, int y)
{
    image_draw_isometric_footprint(image_id, x, y, COLOR_MASK_GREEN);
    image_draw_isometric_top(image_id, x, y, COLOR_MASK_GREEN);
}

static void draw_building(const struct map_tile_t *tile, int x_view, int y_view, int type)
{
    int num_tiles = building_properties[type].size * building_properties[type].size;
    int blocked_tiles[MAX_TILES];
    int blocked = !editor_tool_can_place_building(tile, num_tiles, blocked_tiles);

    if (blocked) {
        draw_partially_blocked(x_view, y_view, num_tiles, blocked_tiles);
    } else if (editor_tool_is_in_use()) {
        int image_id = image_group(GROUP_TERRAIN_OVERLAY);
        for (int i = 0; i < num_tiles; i++) {
            int x_offset = x_view + X_VIEW_OFFSETS[i];
            int y_offset = y_view + Y_VIEW_OFFSETS[i];
            image_draw_isometric_footprint(image_id, x_offset, y_offset, 0);
        }
    } else {
        int image_id;
        if (type == BUILDING_NATIVE_CROPS) {
            image_id = image_group(GROUP_EDITOR_BUILDING_CROPS);
        } else if (type == BUILDING_HOUSE_VACANT_LOT) {
            image_id = image_group(GROUP_EDITOR_BUILDING_NATIVE) - 4;
        } else {
            image_id = image_group(building_properties[type].image_group) + building_properties[type].image_offset;
        }
        draw_building_image(image_id, x_view, y_view);
    }
}

static void draw_road(const struct map_tile_t *tile, int x, int y)
{
    int blocked = 0;
    int image_id = 0;
    if (map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR)) {
        blocked = 1;
    } else {
        image_id = image_group(GROUP_TERRAIN_ROAD);
        if (!map_terrain_has_adjacent_x_with_type(tile->grid_offset, TERRAIN_ROAD) &&
            map_terrain_has_adjacent_y_with_type(tile->grid_offset, TERRAIN_ROAD)) {
            image_id++;
        }
    }
    if (blocked) {
        draw_flat_tile(x, y, COLOR_MASK_RED);
    } else {
        draw_building_image(image_id, x, y);
    }
}

static void draw_access_ramp(const struct map_tile_t *tile, int x, int y)
{
    int orientation;
    if (editor_tool_can_place_access_ramp(tile, &orientation)) {
        int image_id = image_group(GROUP_TERRAIN_ACCESS_RAMP) + orientation;
        draw_building_image(image_id, x, y);
    } else {
        int blocked[4] = { 1, 1, 1, 1 };
        draw_partially_blocked(x, y, 4, blocked);
    }
}

static void draw_map_flag(int x, int y, int is_ok)
{
    draw_flat_tile(x, y, is_ok ? COLOR_MASK_GREEN : COLOR_MASK_RED);
}

void map_editor_tool_draw(const struct map_tile_t *tile)
{
    if (!tile->grid_offset || scroll_in_progress() || !editor_tool_is_active()) {
        return;
    }

    int type = editor_tool_type();
    int x, y;
    city_view_get_selected_tile_pixels(&x, &y);
    switch (type) {
        case TOOL_NATIVE_CENTER:
            draw_building(tile, x, y, BUILDING_NATIVE_MEETING);
            break;
        case TOOL_NATIVE_HUT:
            draw_building(tile, x, y, BUILDING_NATIVE_HUT);
            break;
        case TOOL_NATIVE_FIELD:
            draw_building(tile, x, y, BUILDING_NATIVE_CROPS);
            break;
        case TOOL_HOUSE_VACANT_LOT:
            draw_building(tile, x, y, BUILDING_HOUSE_VACANT_LOT);
            break;
        case TOOL_EARTHQUAKE_POINT:
        case TOOL_ENTRY_POINT:
        case TOOL_EXIT_POINT:
        case TOOL_RIVER_ENTRY_POINT:
        case TOOL_RIVER_EXIT_POINT:
        case TOOL_INVASION_POINT:
        case TOOL_FISHING_POINT:
        case TOOL_HERD_POINT:
            draw_map_flag(x, y, editor_tool_can_place_flag(type, tile, 0));
            break;

        case TOOL_ACCESS_RAMP:
            draw_access_ramp(tile, x, y);
            break;

        case TOOL_GRASS:
        case TOOL_SMALL_SHRUB:
        case TOOL_MEDIUM_SHRUB:
        case TOOL_LARGE_SHRUB:
        case TOOL_LARGEST_SHRUB:
        case TOOL_TREES:
        case TOOL_WATER:
        case TOOL_RAISE_LAND:
        case TOOL_LOWER_LAND:
        case TOOL_SMALL_ROCK:
        case TOOL_MEDIUM_ROCK:
        case TOOL_LARGE_ROCK:
        case TOOL_MEADOW:
            draw_brush(x, y);
            break;

        case TOOL_ROAD:
            draw_road(tile, x, y);
            break;
    }
}
