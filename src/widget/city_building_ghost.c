#include "city_building_ghost.h"

#include "city/buildings.h"
#include "city/data.h"
#include "city/finance.h"
#include "city/view.h"
#include "core/config.h"
#include "core/image.h"
#include "figure/formation_legion.h"
#include "game/game.h"
#include "input/input.h"
#include "map/map.h"
#include "scenario/scenario.h"
#include "widget/city_bridge.h"

#define MAX_TILES 25

static const int X_VIEW_OFFSETS[MAX_TILES] = {
    0,
    -30, 30, 0,
    -60, 60, -30, 30, 0,
    -90, 90, -60, 60, -30, 30, 0,
    -120, 120, -90, 90, -60, 60, -30, 30, 0
};

static const int Y_VIEW_OFFSETS[MAX_TILES] = {
    0,
    15, 15, 30,
    30, 30, 45, 45, 60,
    45, 45, 60, 60, 75, 75, 90,
    60, 60, 75, 75, 90, 90, 105, 105, 120
};

static const int TILE_GRID_OFFSETS[4][MAX_TILES] = {
    {OFFSET(0,0),
    OFFSET(0,1), OFFSET(1,0), OFFSET(1,1),
    OFFSET(0,2), OFFSET(2,0), OFFSET(1,2), OFFSET(2,1), OFFSET(2,2),
    OFFSET(0,3), OFFSET(3,0), OFFSET(1,3), OFFSET(3,1), OFFSET(2,3), OFFSET(3,2), OFFSET(3,3),
    OFFSET(0,4), OFFSET(4,0), OFFSET(1,4), OFFSET(4,1), OFFSET(2,4), OFFSET(4,2),
        OFFSET(3,4), OFFSET(4,3), OFFSET(4,4)},
    {OFFSET(0,0),
    OFFSET(-1,0), OFFSET(0,1), OFFSET(-1,1),
    OFFSET(-2,0), OFFSET(0,2), OFFSET(-2,1), OFFSET(-1,2), OFFSET(-2,2),
    OFFSET(-3,0), OFFSET(0,3), OFFSET(-3,1), OFFSET(-1,3), OFFSET(-3,2), OFFSET(-2,3), OFFSET(-3,3),
    OFFSET(-4,0), OFFSET(0,4), OFFSET(-4,1), OFFSET(-1,4), OFFSET(-4,2), OFFSET(-2,4),
        OFFSET(-4,3), OFFSET(-3,4), OFFSET(-4,4)},
    {OFFSET(0,0),
    OFFSET(0,-1), OFFSET(-1,0), OFFSET(-1,-1),
    OFFSET(0,-2), OFFSET(-2,0), OFFSET(-1,-2), OFFSET(-2,-1), OFFSET(-2,-2),
    OFFSET(0,-3), OFFSET(-3,0), OFFSET(-1,-3), OFFSET(-3,-1), OFFSET(-2,-3), OFFSET(-3,-2), OFFSET(-3,-3),
    OFFSET(0,-4), OFFSET(-4,0), OFFSET(-1,-4), OFFSET(-4,-1), OFFSET(-2,-4), OFFSET(-4,-2),
        OFFSET(-3,-4), OFFSET(-4,-3), OFFSET(-4,-4)},
    {OFFSET(0,0),
    OFFSET(1,0), OFFSET(0,-1), OFFSET(1,-1),
    OFFSET(2,0), OFFSET(0,-2), OFFSET(2,-1), OFFSET(1,-2), OFFSET(2,-2),
    OFFSET(3,0), OFFSET(0,-3), OFFSET(3,-1), OFFSET(1,-3), OFFSET(3,-2), OFFSET(2,-3), OFFSET(3,-3),
    OFFSET(4,0), OFFSET(0,-4), OFFSET(4,-1), OFFSET(1,-4), OFFSET(4,-2), OFFSET(2,-4),
        OFFSET(4,-3), OFFSET(3,-4), OFFSET(4,-4)},
};

static const int FORT_GROUND_GRID_OFFSETS[4] = { OFFSET(3,-1), OFFSET(4,-1), OFFSET(4,0), OFFSET(3,0) };
static const int FORT_GROUND_X_VIEW_OFFSETS[4] = { 120, 90, -120, -90 };
static const int FORT_GROUND_Y_VIEW_OFFSETS[4] = { 30, -75, -60, 45 };

static const int HIPPODROME_X_VIEW_OFFSETS[4] = { 150, 150, -150, -150 };
static const int HIPPODROME_Y_VIEW_OFFSETS[4] = { 75, -75, -75, 75 };

static int building_preview_blocked(int grid_offset, int num_tiles, int *blocked_tiles, int type)
{
    int orientation_index = city_view_orientation() / 2;
    int blocked = 0;
    for (int i = 0; i < num_tiles; i++) {
        int tile_offset = grid_offset + TILE_GRID_OFFSETS[orientation_index][i];
        int forbidden_terrain = terrain_grid.items[tile_offset] & TERRAIN_NOT_CLEAR;
        if (type == BUILDING_GATEHOUSE || type == BUILDING_TRIUMPHAL_ARCH) {
            forbidden_terrain &= ~TERRAIN_ROAD;
        }
        if (type == BUILDING_TOWER) {
            forbidden_terrain &= ~TERRAIN_WALL;
        }
        if (forbidden_terrain || map_has_figure_at(tile_offset)) {
            blocked_tiles[i] = 1;
            blocked = 1;
        } else {
            blocked_tiles[i] = 0;
        }
    }
    return blocked;
}

static void draw_blocked_building_preview(int x, int y, int num_tiles, int *blocked_tiles, int completely_blocked)
{
    for (int i = 0; i < num_tiles; i++) {
        int x_offset = x + X_VIEW_OFFSETS[i];
        int y_offset = y + Y_VIEW_OFFSETS[i];
        if (completely_blocked || blocked_tiles[i]) {
            image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x_offset, y_offset, COLOR_MASK_RED);
        } else {
            image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x_offset, y_offset, COLOR_MASK_GREEN);
        }
    }
}

static void draw_building(int image_id, int x, int y)
{
    image_draw_isometric_footprint(image_id, x, y, COLOR_MASK_GREEN);
    image_draw_isometric_top(image_id, x, y, COLOR_MASK_GREEN);
}

static void draw_water_range_preview(int x, int y, int radius)
{
    int image_id = image_group(GROUP_TERRAIN_FLAT_TILE);
    for (int i = 1; i <= radius; i++) {
        for (int j = 0; j <= radius; j++) {
            image_draw_blend_alpha(image_id, x + HALF_TILE_WIDTH_PIXELS * (i + j), y - HALF_TILE_HEIGHT_PIXELS * (i - j), COLOR_MASK_BLUE);
            image_draw_blend_alpha(image_id, x + HALF_TILE_WIDTH_PIXELS * (i - j), y + HALF_TILE_HEIGHT_PIXELS * (i + j), COLOR_MASK_BLUE);
            image_draw_blend_alpha(image_id, x - HALF_TILE_WIDTH_PIXELS * (i + j), y + HALF_TILE_HEIGHT_PIXELS * (i - j), COLOR_MASK_BLUE);
            image_draw_blend_alpha(image_id, x - HALF_TILE_WIDTH_PIXELS * (i - j), y - HALF_TILE_HEIGHT_PIXELS * (i + j), COLOR_MASK_BLUE);
        }
    }
}

static int get_building_image_id(int map_x, int map_y, int type, struct building_properties_t *props)
{
    int image_id = image_group(props->image_group) + props->image_offset;
    if (type == BUILDING_GATEHOUSE) {
        int orientation = map_orientation_for_gatehouse(map_x, map_y);
        int image_offset;
        if (orientation == 2) {
            image_offset = 1;
        } else if (orientation == 1) {
            image_offset = 0;
        } else {
            image_offset = building_construction_road_orientation() == 2 ? 1 : 0;
        }
        int map_orientation = city_view_orientation();
        if (map_orientation == DIR_6_LEFT || map_orientation == DIR_2_RIGHT) {
            image_offset = 1 - image_offset;
        }
        image_id += image_offset;
    } else if (type == BUILDING_TRIUMPHAL_ARCH) {
        int orientation = map_orientation_for_triumphal_arch(map_x, map_y);
        int image_offset;
        if (orientation == 2) {
            image_offset = 2;
        } else if (orientation == 1) {
            image_offset = 0;
        } else {
            image_offset = building_construction_road_orientation() == 2 ? 2 : 0;
        }
        int map_orientation = city_view_orientation();
        if (map_orientation == DIR_6_LEFT || map_orientation == DIR_2_RIGHT) {
            image_offset = 2 - image_offset;
        }
        image_id += image_offset;
    }
    return image_id;
}

static void get_building_base_xy(int map_x, int map_y, int building_size, int *x, int *y)
{
    switch (city_view_orientation()) {
        case DIR_0_TOP:
            *x = map_x;
            *y = map_y;
            break;
        case DIR_2_RIGHT:
            *x = map_x - building_size + 1;
            *y = map_y;
            break;
        case DIR_4_BOTTOM:
            *x = map_x - building_size + 1;
            *y = map_y - building_size + 1;
            break;
        case DIR_6_LEFT:
            *x = map_x;
            *y = map_y - building_size + 1;
            break;
        default:
            *x = *y = 0;
    }
}

static void draw_bridge(const struct map_tile_t *tile, int x, int y, int type)
{
    int length, direction;
    int end_grid_offset = map_bridge_calculate_length_direction(tile->x, tile->y, &length, &direction);

    int dir = direction - city_view_orientation();
    if (dir < 0) {
        dir += 8;
    }
    int blocked = 0;
    if (type == BUILDING_SHIP_BRIDGE && length < 5) {
        blocked = 1;
    } else if (!end_grid_offset) {
        blocked = 1;
    }
    int x_delta, y_delta;
    switch (dir) {
        case DIR_0_TOP:
            x_delta = 29;
            y_delta = -15;
            break;
        case DIR_2_RIGHT:
            x_delta = 29;
            y_delta = 15;
            break;
        case DIR_4_BOTTOM:
            x_delta = -29;
            y_delta = 15;
            break;
        case DIR_6_LEFT:
            x_delta = -29;
            y_delta = -15;
            break;
        default:
            return;
    }
    if (blocked) {
        image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, length > 0 ? COLOR_MASK_GREEN : COLOR_MASK_RED);
        if (length > 1) {
            image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x + x_delta * (length - 1), y + y_delta * (length - 1), COLOR_MASK_RED);
        }
        building_construction_set_cost(0);
    } else {
        if (dir == DIR_0_TOP || dir == DIR_6_LEFT) {
            for (int i = length - 1; i >= 0; i--) {
                int sprite_id = map_bridge_get_sprite_id(i, length, dir, type == BUILDING_SHIP_BRIDGE);
                city_draw_bridge_tile(x + x_delta * i, y + y_delta * i, sprite_id, COLOR_MASK_GREEN);
            }
        } else {
            for (int i = 0; i < length; i++) {
                int sprite_id = map_bridge_get_sprite_id(i, length, dir, type == BUILDING_SHIP_BRIDGE);
                city_draw_bridge_tile(x + x_delta * i, y + y_delta * i, sprite_id, COLOR_MASK_GREEN);
            }
        }
        building_construction_set_cost(building_properties[type].cost * length);
    }
}

int city_building_ghost_mark_deleting(const struct map_tile_t *tile)
{
    if (!config_get(CONFIG_UI_VISUAL_FEEDBACK_ON_DELETE)) {
        return 0;
    }
    int construction_type = building_construction_type();
    if (!tile->grid_offset || building_construction_draw_as_constructing() ||
        scroll_in_progress() || construction_type != BUILDING_CLEAR_LAND) {
        return (construction_type == BUILDING_CLEAR_LAND);
    }
    if (!building_construction_in_progress()) {
        map_property_clear_constructing_and_deleted();
    }
    map_building_tiles_mark_deleting(tile->grid_offset);
    return 1;
}

static int is_road_tile_for_aqueduct(int grid_offset, int gate_orientation)
{
    int is_road = map_terrain_is(grid_offset, TERRAIN_ROAD) ? 1 : 0;
    if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        struct building_t *b = &all_buildings[map_building_at(grid_offset)];
        if (b->type == BUILDING_GATEHOUSE) {
            if (b->subtype.orientation == gate_orientation) {
                is_road = 1;
            }
        } else if (b->type == BUILDING_GRANARY) {
            if (terrain_land_citizen.items[grid_offset] == CITIZEN_0_ROAD) {
                is_road = 1;
            }
        }
    }
    return is_road;
}

void city_building_ghost_draw(const struct map_tile_t *tile)
{
    if (!tile->grid_offset || scroll_in_progress()) {
        return;
    }

    int type = building_construction_type();
    if (building_construction_draw_as_constructing() || type == BUILDING_NONE || type == BUILDING_CLEAR_LAND) {
        return;
    }

    int x, y;
    city_view_get_selected_tile_pixels(&x, &y);

    struct building_properties_t *building_props = &building_properties[type];
    int building_size = type == BUILDING_WAREHOUSE ? 3 : building_props->size;
    int num_tiles = building_size * building_size;
    int blocked_tiles[num_tiles];
    int orientation_index = city_view_orientation() / 2;

    if (!city_finance_can_afford(building_properties[type].cost)) {
        draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
        if (type == BUILDING_HIPPODROME) {
            int blocked_tiles2[MAX_TILES];
            int blocked_tiles3[MAX_TILES];
            draw_blocked_building_preview(x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles2, 1);
            draw_blocked_building_preview(x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles3, 1);
        } else if (building_is_fort(type)) {
            int num_fort_ground_tiles = building_properties[BUILDING_FORT_GROUND].size * building_properties[BUILDING_FORT_GROUND].size;
            int blocked_tiles_ground[MAX_TILES];
            draw_blocked_building_preview(x + FORT_GROUND_X_VIEW_OFFSETS[orientation_index], y + FORT_GROUND_Y_VIEW_OFFSETS[orientation_index], num_fort_ground_tiles, blocked_tiles_ground, 1);
        }
        return;
    }

    int x_dir_offset = 0;
    int y_dir_offset = 0;
    get_building_base_xy(tile->x, tile->y, building_size, &x_dir_offset, &y_dir_offset);
    if (!check_building_terrain_requirements(x_dir_offset, y_dir_offset, 0)) {
        draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
        return;
    }

    int image_id = get_building_image_id(tile->x, tile->y, type, building_props);

    switch (type) {
        case BUILDING_HOUSE_VACANT_LOT:
            if (map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            draw_building(image_group(GROUP_BUILDING_HOUSE_VACANT_LOT), x, y);
            return;
        case BUILDING_ROAD:
            if (map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            if (map_terrain_is(tile->grid_offset, TERRAIN_AQUEDUCT)) {
                if (map_can_place_road_under_aqueduct(tile->grid_offset)) {
                    draw_building(image_group(GROUP_BUILDING_AQUEDUCT) + map_get_aqueduct_with_road_image(tile->grid_offset), x, y);
                } else {
                    image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                }
            } else {
                if (!map_terrain_has_adjacent_x_with_type(tile->grid_offset, TERRAIN_ROAD) &&
                    map_terrain_has_adjacent_y_with_type(tile->grid_offset, TERRAIN_ROAD)) {
                    image_id++;
                }
                draw_building(image_id, x, y);
            }
            return;
        case BUILDING_RESERVOIR:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            // y + 30 to center mouse, 11 instead of 10 range to compensate for building size
            draw_water_range_preview(x, y + 30, 11);
            draw_building(image_id, x, y);
            if (map_terrain_exists_tile_in_area_with_type(tile->x - 1, tile->y - 1, 5, TERRAIN_WATER)) {
                const struct image_t *img = image_get(image_id);
                int x_water = x - 58 + img->sprite_offset_x - 2;
                int y_water = y + img->sprite_offset_y - (img->height - 90);
                image_draw_masked(image_id + 1, x_water, y_water, COLOR_MASK_GREEN);
            }
            return;
        case BUILDING_AQUEDUCT:
            if (map_terrain_is(tile->grid_offset, TERRAIN_ROAD)) {
                int map_is_straight_road_for_aqueduct = 0;
                int road_tiles_x =
                    is_road_tile_for_aqueduct(tile->grid_offset + map_grid_delta(1, 0), 2) +
                    is_road_tile_for_aqueduct(tile->grid_offset + map_grid_delta(-1, 0), 2);
                int road_tiles_y =
                    is_road_tile_for_aqueduct(tile->grid_offset + map_grid_delta(0, -1), 1) +
                    is_road_tile_for_aqueduct(tile->grid_offset + map_grid_delta(0, 1), 1);
                if ((road_tiles_x == 2 && road_tiles_y == 0)
                || (road_tiles_y == 2 && road_tiles_x == 0)) {
                    map_is_straight_road_for_aqueduct = 1;
                }
                if (!map_is_straight_road_for_aqueduct || map_property_is_plaza_or_earthquake(tile->grid_offset)) {
                    image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                    return;
                }
            } else if (map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            const struct terrain_image_t *terrain_img = map_image_context_get_aqueduct(tile->grid_offset, 1);
            if (map_terrain_is(tile->grid_offset, TERRAIN_ROAD)) {
                int group_offset = terrain_img->group_offset;
                if (!terrain_img->aqueduct_offset) {
                    if (map_terrain_is(tile->grid_offset + map_grid_delta(0, -1), TERRAIN_ROAD)) {
                        group_offset = 3;
                    } else {
                        group_offset = 2;
                    }
                }
                if (map_tiles_is_paved_road(tile->grid_offset)) {
                    image_id += group_offset + 13;
                } else {
                    image_id += group_offset + 21;
                }
            } else {
                image_id += terrain_img->group_offset + 15;
            }
            draw_building(image_id, x, y);
            return;
        case BUILDING_FOUNTAIN:
            if (map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            draw_water_range_preview(x, y, scenario.climate == CLIMATE_DESERT ? 3 : 4);
            draw_building(image_id, x, y);
            if (map_terrain_is(tile->grid_offset, TERRAIN_RESERVOIR_RANGE)) {
                const struct image_t *img = image_get(image_id);
                image_draw_masked(image_id + 1, x + img->sprite_offset_x, y + img->sprite_offset_y, COLOR_MASK_GREEN);
            }
            return;
        case BUILDING_WELL:
            if (map_terrain_is(tile->grid_offset, TERRAIN_NOT_CLEAR)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            draw_water_range_preview(x, y, 2);
            draw_building(image_id, x, y);
            return;
        case BUILDING_BATHHOUSE:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            for (int i = 0; i < num_tiles; i++) {
                if (map_terrain_is(tile->grid_offset + TILE_GRID_OFFSETS[orientation_index][i], TERRAIN_RESERVOIR_RANGE)) {
                    const struct image_t *img = image_get(image_id);
                    image_draw_masked(image_id - 1, x + img->sprite_offset_x - 7, y + img->sprite_offset_y + 6, COLOR_MASK_GREEN);
                    break;
                }
            }
            return;
        case BUILDING_HIPPODROME:
            int blocked_tiles2[25];
            int blocked_tiles3[25];
            if (city_data.building.hippodrome_placed) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                draw_blocked_building_preview(x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles2, 1);
                draw_blocked_building_preview(x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles3, 1);
                return;
            }
            int blocked1 = building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type);
            int blocked2 = building_preview_blocked(tile->grid_offset + map_grid_delta(5, 0), num_tiles, blocked_tiles2, type);
            int blocked3 = building_preview_blocked(tile->grid_offset + map_grid_delta(10, 0), num_tiles, blocked_tiles3, type);
            if (blocked1 || blocked2 || blocked3) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                draw_blocked_building_preview(x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles2, 0);
                draw_blocked_building_preview(x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index], num_tiles, blocked_tiles3, 0);
                return;
            }
            if (orientation_index == 0) {
                image_id = image_group(GROUP_BUILDING_HIPPODROME_2);
                // part 1, 2, 3
                draw_building(image_id, x, y);
                draw_building(image_id + 2, x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id + 4, x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
            } else if (orientation_index == 1) {
                image_id = image_group(GROUP_BUILDING_HIPPODROME_1);
                // part 3, 2, 1
                draw_building(image_id, x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id + 2, x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id + 4, x, y);
            } else if (orientation_index == 2) {
                image_id = image_group(GROUP_BUILDING_HIPPODROME_2);
                // part 1, 2, 3
                draw_building(image_id + 4, x, y);
                draw_building(image_id + 2, x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id, x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
            } else if (orientation_index == 3) {
                image_id = image_group(GROUP_BUILDING_HIPPODROME_1);
                // part 3, 2, 1
                draw_building(image_id + 4, x + 2 * HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + 2 * HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id + 2, x + HIPPODROME_X_VIEW_OFFSETS[orientation_index], y + HIPPODROME_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id, x, y);
            }
            return;
        case BUILDING_SENATE:
            if (city_data.building.senate_placed) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                return;
            }
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            return;
        case BUILDING_TRIUMPHAL_ARCH:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            const struct image_t *img = image_get(image_id + 1);
            if (image_id == image_group(GROUP_BUILDING_TRIUMPHAL_ARCH)) {
                image_draw_masked(image_id + 1, x + img->sprite_offset_x + 4, y + img->sprite_offset_y - 51, COLOR_MASK_GREEN);
            } else {
                image_draw_masked(image_id + 1, x + img->sprite_offset_x - 33, y + img->sprite_offset_y - 56, COLOR_MASK_GREEN);
            }
            return;
        case BUILDING_PLAZA:
            if (!map_terrain_is(tile->grid_offset, TERRAIN_ROAD)) {
                image_draw_blend(image_group(GROUP_TERRAIN_FLAT_TILE), x, y, COLOR_MASK_RED);
                return;
            }
            draw_building(image_id, x, y);
            return;
        case BUILDING_LOW_BRIDGE:
        case BUILDING_SHIP_BRIDGE:
            draw_bridge(tile, x, y, type);
            break;
        case BUILDING_SHIPYARD:
        case BUILDING_WHARF:
            int dir_absolute;
            int dir_relative;
            if (map_water_determine_orientation_size2(tile->x, tile->y, 1, &dir_absolute, &dir_relative)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                return;
            }
            image_id = image_group(building_props->image_group) + building_props->image_offset + dir_relative;
            draw_building(image_id, x, y);
            return;
        case BUILDING_DOCK:
            int dir_absolute_d;
            int dir_relative_d;
            if (map_water_determine_orientation_size3(tile->x, tile->y, 1, &dir_absolute_d, &dir_relative_d)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                return;
            }
            switch (dir_relative_d) {
                case 0: image_id = image_group(GROUP_BUILDING_DOCK_1); break;
                case 1: image_id = image_group(GROUP_BUILDING_DOCK_2); break;
                case 2: image_id = image_group(GROUP_BUILDING_DOCK_3); break;
                default:image_id = image_group(GROUP_BUILDING_DOCK_4); break;
            }
            draw_building(image_id, x, y);
            return;
        case BUILDING_GATEHOUSE:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            // update road required based on timer
            building_construction_update_road_orientation();
            draw_building(image_id, x, y);
            return;
        case BUILDING_FORT_LEGIONARIES:
        case BUILDING_FORT_JAVELIN:
        case BUILDING_FORT_MOUNTED:
            int num_fort_ground_tiles = building_properties[BUILDING_FORT_GROUND].size * building_properties[BUILDING_FORT_GROUND].size;
            int blocked_tiles_ground[MAX_TILES];
            if (city_data.military.total_legions >= MAX_LEGIONS) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                draw_blocked_building_preview(x + FORT_GROUND_X_VIEW_OFFSETS[orientation_index], y + FORT_GROUND_Y_VIEW_OFFSETS[orientation_index], num_fort_ground_tiles, blocked_tiles_ground, 1);
                return;
            }
            int blocked1_f = building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type);
            int blocked2_f = building_preview_blocked(tile->grid_offset + FORT_GROUND_GRID_OFFSETS[orientation_index], num_fort_ground_tiles, blocked_tiles_ground, type);
            if (blocked1_f || blocked2_f) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                draw_blocked_building_preview(x + FORT_GROUND_X_VIEW_OFFSETS[orientation_index], y + FORT_GROUND_Y_VIEW_OFFSETS[orientation_index], num_fort_ground_tiles, blocked_tiles_ground, 0);
                return;
            }
            if (orientation_index == 0 || orientation_index == 3) {
                draw_building(image_id, x, y);
                draw_building(image_id + 1, x + FORT_GROUND_X_VIEW_OFFSETS[orientation_index], y + FORT_GROUND_Y_VIEW_OFFSETS[orientation_index]);
            } else {
                draw_building(image_id + 1, x + FORT_GROUND_X_VIEW_OFFSETS[orientation_index], y + FORT_GROUND_Y_VIEW_OFFSETS[orientation_index]);
                draw_building(image_id, x, y);
            }
            return;
        case BUILDING_BARRACKS:
            if (building_count_total(BUILDING_BARRACKS)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 1);
                return;
            }
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            return;
        case BUILDING_WHEAT_FARM:
        case BUILDING_VEGETABLE_FARM:
        case BUILDING_FRUIT_FARM:
        case BUILDING_OLIVE_FARM:
        case BUILDING_VINES_FARM:
        case BUILDING_PIG_FARM:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            // fields
            for (int i = 4; i < 9; i++) {
                image_draw_isometric_footprint(image_id + 1, x + X_VIEW_OFFSETS[i], y + Y_VIEW_OFFSETS[i], COLOR_MASK_GREEN);
            }
            return;
        case BUILDING_GRANARY:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            image_draw_isometric_footprint(image_id, x, y, COLOR_MASK_GREEN);
            const struct image_t *img_granary = image_get(image_id + 1);
            image_draw_masked(image_id + 1, x + img_granary->sprite_offset_x - 32, y + img_granary->sprite_offset_y - 64, COLOR_MASK_GREEN);
            return;
        case BUILDING_WAREHOUSE:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            image_draw_masked(image_group(GROUP_BUILDING_WAREHOUSE) + 17, x - 4, y - 42, COLOR_MASK_GREEN);
            for (int i = 1; i < 9; i++) {
                draw_building(EMPTY_WAREHOUSE_IMG_ID, x + X_VIEW_OFFSETS[i], y + Y_VIEW_OFFSETS[i]);
            }
            return;
        default:
            if (building_preview_blocked(tile->grid_offset, num_tiles, blocked_tiles, type)) {
                draw_blocked_building_preview(x, y, num_tiles, blocked_tiles, 0);
                return;
            }
            draw_building(image_id, x, y);
            return;
    }
}