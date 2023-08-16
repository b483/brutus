#include "construction.h"

#include "building/building.h"
#include "building/construction_clear.h"
#include "building/construction_routed.h"
#include "building/construction_warning.h"
#include "building/count.h"
#include "building/storage.h"
#include "building/warehouse.h"
#include "city/buildings.h"
#include "city/data.h"
#include "city/finance.h"
#include "city/resource.h"
#include "city/view.h"
#include "city/warning.h"
#include "core/calc.h"
#include "core/config.h"
#include "core/image.h"
#include "core/random.h"
#include "core/time.h"
#include "figure/formation_herd.h"
#include "figure/formation_legion.h"
#include "game/undo.h"
#include "map/aqueduct.h"
#include "map/bridge.h"
#include "map/building.h"
#include "map/building_tiles.h"
#include "map/grid.h"
#include "map/image.h"
#include "map/orientation.h"
#include "map/property.h"
#include "map/routing.h"
#include "map/routing_terrain.h"
#include "map/terrain.h"
#include "map/tiles.h"
#include "map/water.h"
#include "scenario/scenario.h"
#include "sound/sound.h"
#include "window/build_menu.h"

static struct {
    int type;
    int in_progress;
    struct map_tile_t start;
    struct map_tile_t end;
    int cost_preview;
    struct {
        int meadow;
        int rock;
        int tree;
        int water;
        int wall;
    } required_terrain;
    int road_orientation;
    uint32_t road_last_update;
    int draw_as_constructing;
    int start_offset_x_view;
    int start_offset_y_view;
} data;

static int last_items_cleared;

static void mark_construction(int x, int y, int size, int terrain, int absolute_xy)
{
    if (map_building_tiles_mark_construction(x, y, size, terrain, absolute_xy)) {
        data.draw_as_constructing = 1;
    }
}

static int place_houses(int measure_only, int x_start, int y_start, int x_end, int y_end)
{
    int x_min, x_max, y_min, y_max;
    map_grid_start_end_to_area(x_start, y_start, x_end, y_end, &x_min, &y_min, &x_max, &y_max);

    int needs_road_warning = 0;
    int items_placed = 0;
    game_undo_restore_building_state();
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
                continue;
            }
            if (measure_only) {
                map_property_mark_constructing(grid_offset);
                items_placed++;
            } else {
                struct building_t *b = building_create(BUILDING_HOUSE_VACANT_LOT, x, y);
                game_undo_add_building(b);
                if (b->id > 0) {
                    items_placed++;
                    map_building_tiles_add(b->id, x, y, 1,
                        image_group(GROUP_BUILDING_HOUSE_VACANT_LOT), TERRAIN_BUILDING);
                    if (!map_terrain_exists_tile_in_radius_with_type(x, y, 1, 2, TERRAIN_ROAD)) {
                        needs_road_warning = 1;
                    }
                }
            }
        }
    }
    if (!measure_only) {
        building_construction_warning_check_food_stocks(BUILDING_HOUSE_VACANT_LOT);
        if (needs_road_warning) {
            city_warning_show(WARNING_HOUSE_TOO_FAR_FROM_ROAD);
        }
        map_routing_update_land();
        window_invalidate();
    }
    return items_placed;
}

static int place_plaza(int x_start, int y_start, int x_end, int y_end)
{
    int x_min, y_min, x_max, y_max;
    map_grid_start_end_to_area(x_start, y_start, x_end, y_end, &x_min, &y_min, &x_max, &y_max);
    game_undo_restore_map(1);

    int items_placed = 0;
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (map_terrain_is(grid_offset, TERRAIN_ROAD) &&
                !map_terrain_is(grid_offset, TERRAIN_WATER | TERRAIN_BUILDING | TERRAIN_AQUEDUCT)) {
                if (!map_property_is_plaza_or_earthquake(grid_offset)) {
                    items_placed++;
                }
                map_image_set(grid_offset, 0);
                map_property_mark_plaza_or_earthquake(grid_offset);
                map_property_set_multi_tile_size(grid_offset, 1);
                map_property_mark_draw_tile(grid_offset);
            }
        }
    }
    map_tiles_update_all_plazas();
    return items_placed;
}

static int place_garden(int x_start, int y_start, int x_end, int y_end)
{
    game_undo_restore_map(1);

    int x_min, y_min, x_max, y_max;
    map_grid_start_end_to_area(x_start, y_start, x_end, y_end, &x_min, &y_min, &x_max, &y_max);

    int items_placed = 0;
    for (int y = y_min; y <= y_max; y++) {
        for (int x = x_min; x <= x_max; x++) {
            int grid_offset = map_grid_offset(x, y);
            if (!map_terrain_is(grid_offset, TERRAIN_NOT_CLEAR)) {
                items_placed++;
                terrain_grid.items[grid_offset] |= TERRAIN_GARDEN;
            }
        }
    }
    map_tiles_update_all_gardens();
    return items_placed;
}

void building_construction_set_cost(int cost)
{
    data.cost_preview = cost;
}

void building_construction_set_type(int type)
{
    data.type = type;
    data.in_progress = 0;
    data.start.x = 0;
    data.start.y = 0;
    data.end.x = 0;
    data.end.y = 0;
    data.cost_preview = 0;

    if (type != BUILDING_NONE) {
        data.required_terrain.wall = 0;
        data.required_terrain.water = 0;
        data.required_terrain.tree = 0;
        data.required_terrain.rock = 0;
        data.required_terrain.meadow = 0;
        data.road_orientation = 0;
        data.road_last_update = time_get_millis();
        data.start.grid_offset = 0;

        switch (type) {
            case BUILDING_WHEAT_FARM:
            case BUILDING_VEGETABLE_FARM:
            case BUILDING_FRUIT_FARM:
            case BUILDING_OLIVE_FARM:
            case BUILDING_VINES_FARM:
            case BUILDING_PIG_FARM:
                data.required_terrain.meadow = 1;
                break;
            case BUILDING_MARBLE_QUARRY:
            case BUILDING_IRON_MINE:
                data.required_terrain.rock = 1;
                break;
            case BUILDING_TIMBER_YARD:
                data.required_terrain.tree = 1;
                break;
            case BUILDING_CLAY_PIT:
                data.required_terrain.water = 1;
                break;
            case BUILDING_GATEHOUSE:
            case BUILDING_TRIUMPHAL_ARCH:
                data.road_orientation = 1;
                break;
            case BUILDING_TOWER:
                data.required_terrain.wall = 1;
                break;
            default:
                break;
        }
    }
}

void building_construction_clear_type(void)
{
    data.cost_preview = 0;
    data.type = BUILDING_NONE;
}

int building_construction_type(void)
{
    return data.type;
}

int building_construction_cost(void)
{
    return data.cost_preview;
}

int building_construction_size(int *x, int *y)
{
    if (!building_construction_is_updatable() || !data.in_progress
        || (data.type != BUILDING_CLEAR_LAND && !data.cost_preview)) {
        return 0;
    }
    int size_x = data.end.x - data.start.x;
    int size_y = data.end.y - data.start.y;
    if (size_x < 0) {
        size_x = -size_x;
    }
    if (size_y < 0) {
        size_y = -size_y;
    }
    size_x++;
    size_y++;
    *x = size_x;
    *y = size_y;
    return 1;
}

int building_construction_in_progress(void)
{
    return data.in_progress;
}

void building_construction_start(int x, int y, int grid_offset)
{
    data.start.grid_offset = grid_offset;
    data.start.x = data.end.x = x;
    data.start.y = data.end.y = y;

    if (game_undo_start_build(data.type)) {
        data.in_progress = 1;
        int can_start = 1;
        switch (data.type) {
            case BUILDING_ROAD:
                can_start = map_routing_calculate_distances_for_building(
                    ROUTED_BUILDING_ROAD, data.start.x, data.start.y);
                break;
            case BUILDING_AQUEDUCT:
                can_start = map_routing_calculate_distances_for_building(
                    ROUTED_BUILDING_AQUEDUCT, data.start.x, data.start.y);
                break;
            case BUILDING_WALL:
                can_start = map_routing_calculate_distances_for_building(
                    ROUTED_BUILDING_WALL, data.start.x, data.start.y);
                break;
            default:
                break;
        }
        if (!can_start) {
            building_construction_cancel();
        }
    }
}

int building_construction_is_updatable(void)
{
    switch (data.type) {
        case BUILDING_CLEAR_LAND:
        case BUILDING_ROAD:
        case BUILDING_AQUEDUCT:
        case BUILDING_WALL:
        case BUILDING_PLAZA:
        case BUILDING_GARDENS:
        case BUILDING_HOUSE_VACANT_LOT:
            return 1;
        default:
            return 0;
    }
}

void building_construction_cancel(void)
{
    map_property_clear_constructing_and_deleted();
    if (data.in_progress && building_construction_is_updatable()) {
        game_undo_restore_map(1);
        data.in_progress = 0;
        data.cost_preview = 0;
    } else {
        building_construction_clear_type();
    }
}

void building_construction_update(int x, int y, int grid_offset)
{
    if (grid_offset) {
        data.end.x = x;
        data.end.y = y;
        data.end.grid_offset = grid_offset;
    } else {
        x = data.end.x;
        y = data.end.y;
    }
    if (!data.type) {
        data.cost_preview = 0;
        return;
    }
    map_property_clear_constructing_and_deleted();
    int current_cost = building_properties[data.type].cost;

    if (data.type == BUILDING_CLEAR_LAND) {
        int items_placed = last_items_cleared = building_construction_clear_land(1, data.start.x, data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (data.type == BUILDING_WALL) {
        int items_placed = building_construction_place_wall(1, data.start.x, data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (data.type == BUILDING_ROAD) {
        int items_placed = building_construction_place_road(1, data.start.x, data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (data.type == BUILDING_PLAZA) {
        int items_placed = place_plaza(data.start.x, data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (data.type == BUILDING_GARDENS) {
        int items_placed = place_garden(data.start.x, data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (data.type == BUILDING_LOW_BRIDGE || data.type == BUILDING_SHIP_BRIDGE) {
        int length = map_bridge_building_length();
        if (length > 1) current_cost *= length;
    } else if (data.type == BUILDING_AQUEDUCT) {
        building_construction_place_aqueduct(data.start.x, data.start.y, x, y, &current_cost);
        map_tiles_update_all_aqueducts(0);
    } else if (data.type == BUILDING_HOUSE_VACANT_LOT) {
        int items_placed = place_houses(1, data.start.x, data.start.y, x, y);
        if (items_placed >= 0) current_cost *= items_placed;
    } else if (data.type == BUILDING_GATEHOUSE) {
        mark_construction(x, y, 2, ~TERRAIN_ROAD, 0);
    } else if (data.type == BUILDING_TRIUMPHAL_ARCH) {
        mark_construction(x, y, 3, ~TERRAIN_ROAD, 0);
    } else if (data.type == BUILDING_WAREHOUSE) {
        mark_construction(x, y, 3, TERRAIN_ALL, 0);
    } else if (building_is_fort(data.type)) {
        if (city_data.military.total_legions < MAX_LEGIONS) {
            const int offsets_x[] = { 3, 4, 4, 3 };
            const int offsets_y[] = { -1, -1, 0, 0 };
            int orient_index = city_view_orientation() / 2;
            int x_offset = offsets_x[orient_index];
            int y_offset = offsets_y[orient_index];
            if (map_building_tiles_are_clear(x, y, 3, TERRAIN_ALL) &&
                map_building_tiles_are_clear(x + x_offset, y + y_offset, 4, TERRAIN_ALL)) {
                mark_construction(x, y, 3, TERRAIN_ALL, 0);
                mark_construction(x + x_offset, y + y_offset, 4, TERRAIN_ALL, 0);
            }
        }
    } else if (data.type == BUILDING_HIPPODROME) {
        if (map_building_tiles_are_clear(x, y, 5, TERRAIN_ALL) &&
            map_building_tiles_are_clear(x + 5, y, 5, TERRAIN_ALL) &&
            map_building_tiles_are_clear(x + 10, y, 5, TERRAIN_ALL) &&
            !city_data.building.hippodrome_placed) {
            mark_construction(x, y, 5, TERRAIN_ALL, 0);
            mark_construction(x + 5, y, 5, TERRAIN_ALL, 0);
            mark_construction(x + 10, y, 5, TERRAIN_ALL, 0);
        }
    } else if (data.type == BUILDING_SHIPYARD || data.type == BUILDING_WHARF) {
        if (!map_water_determine_orientation_size2(x, y, 1, 0, 0)) {
            data.draw_as_constructing = 1;
        }
    } else if (data.type == BUILDING_DOCK) {
        if (!map_water_determine_orientation_size3(x, y, 1, 0, 0)) {
            data.draw_as_constructing = 1;
        }
    } else if (data.required_terrain.meadow || data.required_terrain.rock || data.required_terrain.tree ||
            data.required_terrain.water || data.required_terrain.wall) {
        // never mark as constructing
    } else {
        if (!(data.type == BUILDING_SENATE && city_data.building.senate_placed) &&
            !(data.type == BUILDING_BARRACKS && building_count_total(BUILDING_BARRACKS) > 0)) {
            mark_construction(x, y, building_properties[data.type].size, TERRAIN_ALL, 0);
        }
    }
    if (!city_finance_can_afford(current_cost)) {
        map_property_clear_constructing_and_deleted();
        building_construction_cancel();
        data.cost_preview = 0;
        city_warning_show(WARNING_OUT_OF_MONEY);
        return;
    }
    data.cost_preview = current_cost;
}

static void add_fort(struct building_t *fort)
{
    fort->prev_part_building_id = 0;
    map_building_tiles_add(fort->id, fort->x, fort->y, fort->size, image_group(GROUP_BUILDING_FORT), TERRAIN_BUILDING);
    if (fort->type == BUILDING_FORT_LEGIONARIES) {
        fort->subtype.fort_figure_type = FIGURE_FORT_LEGIONARY;
    } else if (fort->type == BUILDING_FORT_JAVELIN) {
        fort->subtype.fort_figure_type = FIGURE_FORT_JAVELIN;
    } else if (fort->type == BUILDING_FORT_MOUNTED) {
        fort->subtype.fort_figure_type = FIGURE_FORT_MOUNTED;
    }

    fort->formation_id = create_legion_formation_for_fort(fort);
    // create parade ground
    struct building_t *ground = building_create(BUILDING_FORT_GROUND, fort->x + 3, fort->y - 1);
    game_undo_add_building(ground);
    ground->formation_id = fort->formation_id;
    ground->prev_part_building_id = fort->id;
    fort->next_part_building_id = ground->id;
    ground->next_part_building_id = 0;
    map_building_tiles_add(ground->id, fort->x + 3, fort->y - 1, 4, image_group(GROUP_BUILDING_FORT) + 1, TERRAIN_BUILDING);
}

static void add_hippodrome(struct building_t *b)
{
    int image1 = image_group(GROUP_BUILDING_HIPPODROME_1);
    int image2 = image_group(GROUP_BUILDING_HIPPODROME_2);
    city_data.building.hippodrome_placed = 1;

    int orientation = city_view_orientation();
    struct building_t *part1 = b;
    if (orientation == DIR_0_TOP || orientation == DIR_4_BOTTOM) {
        part1->subtype.orientation = 0;
    } else {
        part1->subtype.orientation = 3;
    }
    part1->prev_part_building_id = 0;
    int image_id;
    switch (orientation) {
        case DIR_0_TOP:
            image_id = image2;
            break;
        case DIR_2_RIGHT:
            image_id = image1 + 4;
            break;
        case DIR_4_BOTTOM:
            image_id = image2 + 4;
            break;
        case DIR_6_LEFT:
            image_id = image1;
            break;
        default:
            return;
    }
    map_building_tiles_add(b->id, b->x, b->y, b->size, image_id, TERRAIN_BUILDING);

    struct building_t *part2 = building_create(BUILDING_HIPPODROME, b->x + 5, b->y);
    game_undo_add_building(part2);
    if (orientation == DIR_0_TOP || orientation == DIR_4_BOTTOM) {
        part2->subtype.orientation = 1;
    } else {
        part2->subtype.orientation = 4;
    }
    part2->prev_part_building_id = part1->id;
    part1->next_part_building_id = part2->id;
    part2->next_part_building_id = 0;
    switch (orientation) {
        case DIR_0_TOP:
        case DIR_4_BOTTOM:
            image_id = image2 + 2;
            break;
        case DIR_2_RIGHT:
        case DIR_6_LEFT:
            image_id = image1 + 2;
            break;
    }
    map_building_tiles_add(part2->id, b->x + 5, b->y, b->size, image_id, TERRAIN_BUILDING);

    struct building_t *part3 = building_create(BUILDING_HIPPODROME, b->x + 10, b->y);
    game_undo_add_building(part3);
    if (orientation == DIR_0_TOP || orientation == DIR_4_BOTTOM) {
        part3->subtype.orientation = 2;
    } else {
        part3->subtype.orientation = 5;
    }
    part3->prev_part_building_id = part2->id;
    part2->next_part_building_id = part3->id;
    part3->next_part_building_id = 0;
    switch (orientation) {
        case DIR_0_TOP:
            image_id = image2 + 4;
            break;
        case DIR_2_RIGHT:
            image_id = image1;
            break;
        case DIR_4_BOTTOM:
            image_id = image2;
            break;
        case DIR_6_LEFT:
            image_id = image1 + 4;
            break;
    }
    map_building_tiles_add(part3->id, b->x + 10, b->y, b->size, image_id, TERRAIN_BUILDING);
}

static struct building_t *add_warehouse_space(int x, int y, struct building_t *prev)
{
    struct building_t *b = building_create(BUILDING_WAREHOUSE_SPACE, x, y);
    game_undo_add_building(b);
    b->prev_part_building_id = prev->id;
    prev->next_part_building_id = b->id;
    map_building_tiles_add(b->id, x, y, 1, EMPTY_WAREHOUSE_IMG_ID, TERRAIN_BUILDING);
    return b;
}

static void add_warehouse(struct building_t *b)
{
    b->storage_id = building_storage_create();
    b->prev_part_building_id = 0;
    map_building_tiles_add(b->id, b->x, b->y, 1, image_group(GROUP_BUILDING_WAREHOUSE), TERRAIN_BUILDING);

    struct building_t *prev = b;
    prev = add_warehouse_space(b->x + 1, b->y, prev);
    prev = add_warehouse_space(b->x + 2, b->y, prev);
    prev = add_warehouse_space(b->x, b->y + 1, prev);
    prev = add_warehouse_space(b->x + 1, b->y + 1, prev);
    prev = add_warehouse_space(b->x + 2, b->y + 1, prev);
    prev = add_warehouse_space(b->x, b->y + 2, prev);
    prev = add_warehouse_space(b->x + 1, b->y + 2, prev);
    prev = add_warehouse_space(b->x + 2, b->y + 2, prev);
    prev->next_part_building_id = 0;
}

static void map_terrain_remove_with_radius(int x, int y, int size, int radius, int terrain)
{
    int x_min, y_min, x_max, y_max;
    map_grid_get_area(x, y, size, radius, &x_min, &y_min, &x_max, &y_max);

    for (int yy = y_min; yy <= y_max; yy++) {
        for (int xx = x_min; xx <= x_max; xx++) {
            terrain_grid.items[map_grid_offset(xx, yy)] &= ~terrain;
        }
    }
}

static void add_to_map(struct building_t *b, int orientation, int waterside_orientation_abs, int waterside_orientation_rel)
{
    switch (b->type) {
        case BUILDING_HOUSE_LARGE_TENT:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_TENT) + 2, TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_SMALL_SHACK:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_SHACK), TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_LARGE_SHACK:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_SHACK) + 2, TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_SMALL_HOVEL:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_HOVEL), TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_LARGE_HOVEL:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_HOVEL) + 2, TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_SMALL_CASA:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_CASA), TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_LARGE_CASA:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_CASA) + 2, TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_SMALL_INSULA:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_INSULA_1), TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_MEDIUM_INSULA:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_INSULA_1) + 2, TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_LARGE_INSULA:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_INSULA_2), TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_GRAND_INSULA:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_INSULA_2) + 2, TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_SMALL_VILLA:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_VILLA_1), TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_MEDIUM_VILLA:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_VILLA_1) + 2, TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_LARGE_VILLA:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_VILLA_2), TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_GRAND_VILLA:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_VILLA_2) + 1, TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_SMALL_PALACE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_PALACE_1), TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_MEDIUM_PALACE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_PALACE_1) + 1, TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_LARGE_PALACE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_PALACE_2), TERRAIN_BUILDING);
            break;
        case BUILDING_HOUSE_LUXURY_PALACE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOUSE_PALACE_2) + 1, TERRAIN_BUILDING);
            break;
        case BUILDING_AMPHITHEATER:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_AMPHITHEATER), TERRAIN_BUILDING);
            break;
        case BUILDING_THEATER:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_THEATER), TERRAIN_BUILDING);
            break;
        case BUILDING_COLOSSEUM:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_COLOSSEUM), TERRAIN_BUILDING);
            break;
        case BUILDING_GLADIATOR_SCHOOL:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_GLADIATOR_SCHOOL), TERRAIN_BUILDING);
            break;
        case BUILDING_LION_HOUSE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_LION_HOUSE), TERRAIN_BUILDING);
            break;
        case BUILDING_ACTOR_COLONY:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_ACTOR_COLONY), TERRAIN_BUILDING);
            break;
        case BUILDING_CHARIOT_MAKER:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_CHARIOT_MAKER), TERRAIN_BUILDING);
            break;
        case BUILDING_SMALL_STATUE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_STATUE), TERRAIN_BUILDING);
            break;
        case BUILDING_MEDIUM_STATUE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_STATUE) + 1, TERRAIN_BUILDING);
            break;
        case BUILDING_LARGE_STATUE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_STATUE) + 2, TERRAIN_BUILDING);
            break;
        case BUILDING_DOCTOR:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_DOCTOR), TERRAIN_BUILDING);
            break;
        case BUILDING_HOSPITAL:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_HOSPITAL), TERRAIN_BUILDING);
            break;
        case BUILDING_BATHHOUSE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_BATHHOUSE_NO_WATER), TERRAIN_BUILDING);
            break;
        case BUILDING_BARBER:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_BARBER), TERRAIN_BUILDING);
            break;
        case BUILDING_SCHOOL:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_SCHOOL), TERRAIN_BUILDING);
            break;
        case BUILDING_ACADEMY:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_ACADEMY), TERRAIN_BUILDING);
            break;
        case BUILDING_LIBRARY:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_LIBRARY), TERRAIN_BUILDING);
            break;
        case BUILDING_PREFECTURE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_PREFECTURE), TERRAIN_BUILDING);
            break;
        case BUILDING_WHEAT_FARM:
            map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS), 0);
            break;
        case BUILDING_VEGETABLE_FARM:
            map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS) + 5, 0);
            break;
        case BUILDING_FRUIT_FARM:
            map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS) + 10, 0);
            break;
        case BUILDING_OLIVE_FARM:
            map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS) + 15, 0);
            break;
        case BUILDING_VINES_FARM:
            map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS) + 20, 0);
            break;
        case BUILDING_PIG_FARM:
            map_building_tiles_add_farm(b->id, b->x, b->y, image_group(GROUP_BUILDING_FARM_CROPS) + 25, 0);
            break;
        case BUILDING_MARBLE_QUARRY:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_MARBLE_QUARRY), TERRAIN_BUILDING);
            break;
        case BUILDING_IRON_MINE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_IRON_MINE), TERRAIN_BUILDING);
            break;
        case BUILDING_TIMBER_YARD:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TIMBER_YARD), TERRAIN_BUILDING);
            break;
        case BUILDING_CLAY_PIT:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_CLAY_PIT), TERRAIN_BUILDING);
            break;
        case BUILDING_WINE_WORKSHOP:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_WINE_WORKSHOP), TERRAIN_BUILDING);
            break;
        case BUILDING_OIL_WORKSHOP:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_OIL_WORKSHOP), TERRAIN_BUILDING);
            break;
        case BUILDING_WEAPONS_WORKSHOP:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_WEAPONS_WORKSHOP), TERRAIN_BUILDING);
            break;
        case BUILDING_FURNITURE_WORKSHOP:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_FURNITURE_WORKSHOP), TERRAIN_BUILDING);
            break;
        case BUILDING_POTTERY_WORKSHOP:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_POTTERY_WORKSHOP), TERRAIN_BUILDING);
            break;
        case BUILDING_GRANARY:
            b->storage_id = building_storage_create();
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_GRANARY), TERRAIN_BUILDING);
            map_tiles_update_area_roads(b->x, b->y, 5);
            break;
        case BUILDING_MARKET:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_MARKET), TERRAIN_BUILDING);
            break;
        case BUILDING_GOVERNORS_HOUSE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_GOVERNORS_HOUSE), TERRAIN_BUILDING);
            break;
        case BUILDING_GOVERNORS_VILLA:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_GOVERNORS_VILLA), TERRAIN_BUILDING);
            break;
        case BUILDING_GOVERNORS_PALACE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_GOVERNORS_PALACE), TERRAIN_BUILDING);
            break;
        case BUILDING_MISSION_POST:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_MISSION_POST), TERRAIN_BUILDING);
            break;
        case BUILDING_ENGINEERS_POST:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_ENGINEERS_POST), TERRAIN_BUILDING);
            break;
        case BUILDING_FORUM:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_FORUM), TERRAIN_BUILDING);
            break;
        case BUILDING_RESERVOIR:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_RESERVOIR), TERRAIN_BUILDING);
            break;
        case BUILDING_FOUNTAIN:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_FOUNTAIN_1), TERRAIN_BUILDING);
            break;
        case BUILDING_WELL:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_WELL), TERRAIN_BUILDING);
            break;
        case BUILDING_MILITARY_ACADEMY:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_MILITARY_ACADEMY), TERRAIN_BUILDING);
            break;
        case BUILDING_SMALL_TEMPLE_CERES:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_CERES), TERRAIN_BUILDING);
            break;
        case BUILDING_SMALL_TEMPLE_NEPTUNE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_NEPTUNE), TERRAIN_BUILDING);
            break;
        case BUILDING_SMALL_TEMPLE_MERCURY:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_MERCURY), TERRAIN_BUILDING);
            break;
        case BUILDING_SMALL_TEMPLE_MARS:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_MARS), TERRAIN_BUILDING);
            break;
        case BUILDING_SMALL_TEMPLE_VENUS:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_VENUS), TERRAIN_BUILDING);
            break;
        case BUILDING_LARGE_TEMPLE_CERES:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_CERES) + 1, TERRAIN_BUILDING);
            break;
        case BUILDING_LARGE_TEMPLE_NEPTUNE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_NEPTUNE) + 1, TERRAIN_BUILDING);
            break;
        case BUILDING_LARGE_TEMPLE_MERCURY:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_MERCURY) + 1, TERRAIN_BUILDING);
            break;
        case BUILDING_LARGE_TEMPLE_MARS:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_MARS) + 1, TERRAIN_BUILDING);
            break;
        case BUILDING_LARGE_TEMPLE_VENUS:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TEMPLE_VENUS) + 1, TERRAIN_BUILDING);
            break;
        case BUILDING_ORACLE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_ORACLE), TERRAIN_BUILDING);
            break;
        case BUILDING_SHIPYARD:
            b->data.industry.orientation = waterside_orientation_abs;
            map_water_add_building(b->id, b->x, b->y, 2,
                image_group(GROUP_BUILDING_SHIPYARD) + waterside_orientation_rel);
            break;
        case BUILDING_WHARF:
            b->data.industry.orientation = waterside_orientation_abs;
            map_water_add_building(b->id, b->x, b->y, 2,
                image_group(GROUP_BUILDING_WHARF) + waterside_orientation_rel);
            break;
        case BUILDING_DOCK:
            city_data.building.working_docks++;
            b->data.dock.orientation = waterside_orientation_abs;
            {
                int image_id;
                switch (waterside_orientation_rel) {
                    case 0: image_id = image_group(GROUP_BUILDING_DOCK_1); break;
                    case 1: image_id = image_group(GROUP_BUILDING_DOCK_2); break;
                    case 2: image_id = image_group(GROUP_BUILDING_DOCK_3); break;
                    default:image_id = image_group(GROUP_BUILDING_DOCK_4); break;
                }
                map_water_add_building(b->id, b->x, b->y, b->size, image_id);
            }
            break;
        case BUILDING_TOWER:
            map_terrain_remove_with_radius(b->x, b->y, 2, 0, TERRAIN_WALL);
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TOWER),
                TERRAIN_BUILDING | TERRAIN_GATEHOUSE);
            map_tiles_update_area_walls(b->x, b->y, 5);
            break;
        case BUILDING_GATEHOUSE:
            map_building_tiles_add(b->id, b->x, b->y, b->size,
                image_group(GROUP_BUILDING_TOWER) + orientation, TERRAIN_BUILDING | TERRAIN_GATEHOUSE);
            b->subtype.orientation = orientation;
            map_orientation_update_buildings();
            map_terrain_add_gatehouse_roads(b->x, b->y, orientation);
            map_tiles_update_area_roads(b->x, b->y, 5);
            map_tiles_update_all_plazas();
            map_tiles_update_area_walls(b->x, b->y, 5);
            break;
        case BUILDING_TRIUMPHAL_ARCH:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_TRIUMPHAL_ARCH) + orientation - 1, TERRAIN_BUILDING);
            b->subtype.orientation = orientation;
            map_orientation_update_buildings();
            map_terrain_add_triumphal_arch_roads(b->x, b->y, orientation);
            map_tiles_update_area_roads(b->x, b->y, 5);
            map_tiles_update_all_plazas();
            city_data.building.triumphal_arches_available--;
            if (!city_data.building.triumphal_arches_available) { // none left
                build_menus[MENU_ADMINISTRATION].menu_items[10].building_id = 0;
                // disable menu if this was the only enabled item
                int menu_enabled = 0;
                for (int j = 0; j < MAX_ITEMS_PER_BUILD_MENU; j++) {
                    if (build_menus[MENU_ADMINISTRATION].menu_items[j].building_id) {
                        menu_enabled = 1;
                        break;
                    }
                }
                if (!menu_enabled) {
                    build_menus[MENU_ADMINISTRATION].is_enabled = 0;
                }
            }
            building_construction_clear_type();
            break;
        case BUILDING_SENATE:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_SENATE), TERRAIN_BUILDING);
            city_buildings_add_senate(b);
            break;
        case BUILDING_BARRACKS:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_BARRACKS), TERRAIN_BUILDING);
            city_buildings_add_barracks(b);
            break;
        case BUILDING_WAREHOUSE:
            add_warehouse(b);
            break;
        case BUILDING_HIPPODROME:
            add_hippodrome(b);
            break;
        case BUILDING_FORT_LEGIONARIES:
        case BUILDING_FORT_JAVELIN:
        case BUILDING_FORT_MOUNTED:
            add_fort(b);
            break;
        case BUILDING_NATIVE_HUT:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_NATIVE) + (random_byte() & 1), TERRAIN_BUILDING);
            break;
        case BUILDING_NATIVE_MEETING:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_NATIVE) + 2, TERRAIN_BUILDING);
            break;
        case BUILDING_NATIVE_CROPS:
            map_building_tiles_add(b->id, b->x, b->y, b->size, image_group(GROUP_BUILDING_FARM_CROPS), TERRAIN_BUILDING);
            break;
    }
    map_routing_update_land();
    map_routing_update_walls();
}

static int building_construction_place_building(int type, int x, int y)
{
    int terrain_mask = TERRAIN_ALL;
    if (type == BUILDING_GATEHOUSE || type == BUILDING_TRIUMPHAL_ARCH) {
        terrain_mask = ~TERRAIN_ROAD;
    } else if (type == BUILDING_TOWER) {
        terrain_mask = ~TERRAIN_WALL;
    }
    int size = building_properties[type].size;
    if (type == BUILDING_WAREHOUSE) {
        size = 3;
    }
    int building_orientation = 0;
    if (type == BUILDING_GATEHOUSE) {
        building_orientation = map_orientation_for_gatehouse(x, y);
    } else if (type == BUILDING_TRIUMPHAL_ARCH) {
        building_orientation = map_orientation_for_triumphal_arch(x, y);
    }
    switch (city_view_orientation()) {
        case DIR_2_RIGHT: x = x - size + 1; break;
        case DIR_4_BOTTOM: x = x - size + 1; y = y - size + 1; break;
        case DIR_6_LEFT: y = y - size + 1; break;
    }
    // extra checks
    if (type == BUILDING_GATEHOUSE) {
        if (!map_tiles_are_clear(x, y, size, terrain_mask)) {
            city_warning_show(WARNING_CLEAR_LAND_NEEDED);
            return 0;
        }
        if (!building_orientation) {
            if (building_construction_road_orientation() == 1) {
                building_orientation = 1;
            } else {
                building_orientation = 2;
            }
        }
    }
    if (type == BUILDING_TRIUMPHAL_ARCH) {
        if (!map_tiles_are_clear(x, y, size, terrain_mask)) {
            city_warning_show(WARNING_CLEAR_LAND_NEEDED);
            return 0;
        }
        if (!building_orientation) {
            if (building_construction_road_orientation() == 1) {
                building_orientation = 1;
            } else {
                building_orientation = 3;
            }
        }
    }
    int waterside_orientation_abs = 0, waterside_orientation_rel = 0;
    if (type == BUILDING_SHIPYARD || type == BUILDING_WHARF) {
        if (map_water_determine_orientation_size2(
            x, y, 0, &waterside_orientation_abs, &waterside_orientation_rel)) {
            city_warning_show(WARNING_SHORE_NEEDED);
            return 0;
        }
    } else if (type == BUILDING_DOCK) {
        if (map_water_determine_orientation_size3(
            x, y, 0, &waterside_orientation_abs, &waterside_orientation_rel)) {
            city_warning_show(WARNING_SHORE_NEEDED);
            return 0;
        }
        map_routing_calculate_distances_water_boat(scenario.river_entry_point.x, scenario.river_entry_point.y);
        if (!map_terrain_is_adjacent_to_open_water(x, y, 3)) {
            city_warning_show(WARNING_DOCK_OPEN_WATER_NEEDED);
            return 0;
        }
    } else {
        if (!map_tiles_are_clear(x, y, size, terrain_mask)) {
            city_warning_show(WARNING_CLEAR_LAND_NEEDED);
            return 0;
        }
        int warning_id;
        if (!check_building_terrain_requirements(x, y, &warning_id)) {
            city_warning_show(warning_id);
            return 0;
        }
    }
    if (building_is_fort(type)) {
        if (!map_tiles_are_clear(x + 3, y - 1, 4, terrain_mask)) {
            city_warning_show(WARNING_CLEAR_LAND_NEEDED);
            return 0;
        }
        if (city_data.military.total_legions >= MAX_LEGIONS) {
            city_warning_show(WARNING_MAX_LEGIONS_REACHED);
            return 0;
        }
    }
    if (type == BUILDING_HIPPODROME) {
        if (city_data.building.hippodrome_placed) {
            city_warning_show(WARNING_ONE_BUILDING_OF_TYPE);
            return 0;
        }
        if (!map_tiles_are_clear(x + 5, y, 5, terrain_mask) ||
            !map_tiles_are_clear(x + 10, y, 5, terrain_mask)) {
            city_warning_show(WARNING_CLEAR_LAND_NEEDED);
            return 0;
        }
    }
    if (type == BUILDING_SENATE && city_data.building.senate_placed) {
        city_warning_show(WARNING_ONE_BUILDING_OF_TYPE);
        return 0;
    }
    if (type == BUILDING_BARRACKS && building_count_total(BUILDING_BARRACKS) > 0) {
        city_warning_show(WARNING_ONE_BUILDING_OF_TYPE);
        return 0;
    }
    building_construction_warning_check_all(type, x, y, size);

    // phew, checks done!
    struct building_t *b;
    b = building_create(type, x, y);
    game_undo_add_building(b);
    if (b->id <= 0) {
        return 0;
    }
    add_to_map(b, building_orientation, waterside_orientation_abs, waterside_orientation_rel);
    return 1;
}

static int get_nearby_enemy_type(int x_start, int y_start, int x_end, int y_end)
{
    for (int i = 1; i < MAX_FIGURES; i++) {
        struct figure_t *f = &figures[i];
        if (figure_is_alive(f) && (figure_properties[f->type].is_enemy_unit || figure_properties[f->type].is_caesar_legion_unit || f->type == FIGURE_WOLF)) {
            int dx = (f->x > x_start) ? (f->x - x_start) : (x_start - f->x);
            int dy = (f->y > y_start) ? (f->y - y_start) : (y_start - f->y);
            if (dx <= 12 && dy <= 12) {
                if (f->type == FIGURE_WOLF) {
                    return 1;
                } else {
                    return 2;
                }
            }
            dx = (f->x > x_end) ? (f->x - x_end) : (x_end - f->x);
            dy = (f->y > y_end) ? (f->y - y_end) : (y_end - f->y);
            if (dx <= 12 && dy <= 12) {
                if (f->type == FIGURE_WOLF) {
                    return 1;
                } else {
                    return 2;
                }
            }
        }
    }
    return 0;
}

void building_construction_place(void)
{
    data.cost_preview = 0;
    data.in_progress = 0;
    int x_start = data.start.x;
    int y_start = data.start.y;
    int x_end = data.end.x;
    int y_end = data.end.y;
    building_construction_warning_reset();
    if (!data.type) {
        return;
    }
    if (data.type >= BUILDING_LARGE_TEMPLE_CERES && data.type <= BUILDING_LARGE_TEMPLE_VENUS
        && city_data.resource.stored_in_warehouses[RESOURCE_MARBLE] < 2) {
        map_property_clear_constructing_and_deleted();
        city_warning_show(WARNING_MARBLE_NEEDED_LARGE_TEMPLE);
        return;
    }
    if (data.type == BUILDING_ORACLE && city_data.resource.stored_in_warehouses[RESOURCE_MARBLE] < 2) {
        map_property_clear_constructing_and_deleted();
        city_warning_show(WARNING_MARBLE_NEEDED_ORACLE);
        return;
    }
    int enemy_type = get_nearby_enemy_type(x_start, y_start, x_end, y_end);
    if (data.type != BUILDING_CLEAR_LAND && enemy_type) {
        if (data.type == BUILDING_WALL || data.type == BUILDING_ROAD || data.type == BUILDING_AQUEDUCT) {
            game_undo_restore_map(0);
        } else if (data.type == BUILDING_PLAZA || data.type == BUILDING_GARDENS) {
            game_undo_restore_map(1);
        } else if (data.type == BUILDING_LOW_BRIDGE || data.type == BUILDING_SHIP_BRIDGE) {
            map_bridge_reset_building_length();
        } else {
            map_property_clear_constructing_and_deleted();
        }
        if (enemy_type == 1) {
            play_sound_effect(SOUND_EFFECT_WOLF_ATTACK_2);
        } else {
            city_warning_show(WARNING_ENEMY_NEARBY);
        }
        return;
    }

    int placement_cost = building_properties[data.type].cost;
    if (data.type == BUILDING_CLEAR_LAND) {
        // BUG in original (keep this behaviour): if confirmation has to be asked (bridge/fort),
        // the previous cost is deducted from treasury and if user chooses 'no', they still pay for removal.
        // If we don't do it this way, the user doesn't pay for the removal at all since we don't come back
        // here when the user says yes.
        int items_placed = building_construction_clear_land(0, x_start, y_start, x_end, y_end);
        if (items_placed < 0) {
            items_placed = last_items_cleared;
        }
        placement_cost *= items_placed;
        map_property_clear_constructing_and_deleted();
    } else if (data.type == BUILDING_WALL) {
        placement_cost *= building_construction_place_wall(0, x_start, y_start, x_end, y_end);
    } else if (data.type == BUILDING_ROAD) {
        placement_cost *= building_construction_place_road(0, x_start, y_start, x_end, y_end);
    } else if (data.type == BUILDING_PLAZA) {
        placement_cost *= place_plaza(x_start, y_start, x_end, y_end);
    } else if (data.type == BUILDING_GARDENS) {
        placement_cost *= place_garden(x_start, y_start, x_end, y_end);
        map_routing_update_land();
    } else if (data.type == BUILDING_LOW_BRIDGE) {
        int length = map_bridge_add(x_end, y_end, 0);
        if (length <= 1) {
            city_warning_show(WARNING_SHORE_NEEDED);
            return;
        }
        placement_cost *= length;
    } else if (data.type == BUILDING_SHIP_BRIDGE) {
        int length = map_bridge_add(x_end, y_end, 1);
        if (length <= 1) {
            city_warning_show(WARNING_SHORE_NEEDED);
            return;
        }
        placement_cost *= length;
    } else if (data.type == BUILDING_AQUEDUCT) {
        int cost;
        if (!building_construction_place_aqueduct(x_start, y_start, x_end, y_end, &cost)) {
            city_warning_show(WARNING_CLEAR_LAND_NEEDED);
            return;
        }
        placement_cost = cost;
        map_tiles_update_all_aqueducts(0);
        map_routing_update_land();
    } else if (data.type == BUILDING_HOUSE_VACANT_LOT) {
        placement_cost *= place_houses(0, x_start, y_start, x_end, y_end);
    } else if (!building_construction_place_building(data.type, x_end, y_end)) {
        return;
    }
    if ((data.type >= BUILDING_LARGE_TEMPLE_CERES && data.type <= BUILDING_LARGE_TEMPLE_VENUS) || data.type == BUILDING_ORACLE) {
        building_warehouses_remove_resource(RESOURCE_MARBLE, 2);
    }
    if (data.type >= BUILDING_SMALL_TEMPLE_CERES && data.type <= BUILDING_SMALL_TEMPLE_VENUS) {
        data.type++;
        if (data.type > BUILDING_SMALL_TEMPLE_VENUS) {
            data.type = BUILDING_SMALL_TEMPLE_CERES;
        }
    }
    if (data.type >= BUILDING_LARGE_TEMPLE_CERES && data.type <= BUILDING_LARGE_TEMPLE_VENUS) {
        data.type++;
        if (data.type > BUILDING_LARGE_TEMPLE_VENUS) {
            data.type = BUILDING_LARGE_TEMPLE_CERES;
        }
    }
    city_finance_process_construction(placement_cost);
    // move herds away
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        if (herd_formations[i].in_use && herd_formations[i].figure_type != FIGURE_WOLF) {
            if (calc_maximum_distance(x_end, y_end, herd_formations[i].destination_x, herd_formations[i].destination_y) <= 6) {
                // force new roaming destination search
                herd_formations[i].wait_ticks_movement = SHEEP_HERD_ROAM_DELAY; // largest roam delay
            }
        }
    }

    if (data.type != BUILDING_TRIUMPHAL_ARCH) {
        game_undo_finish_build(placement_cost);
    }
}

static void set_warning(int *warning_id, int warning)
{
    if (warning_id) {
        *warning_id = warning;
    }
}

int check_building_terrain_requirements(int x, int y, int *warning_id)
{
    if (data.required_terrain.meadow) {
        if (!map_terrain_exists_tile_in_radius_with_type(x, y, 3, 1, TERRAIN_MEADOW)) {
            set_warning(warning_id, WARNING_MEADOW_NEEDED);
            return 0;
        }
    } else if (data.required_terrain.rock) {
        if (!map_terrain_exists_tile_in_radius_with_type(x, y, 2, 1, TERRAIN_ELEVATION)) {
            set_warning(warning_id, WARNING_ROCK_NEEDED);
            return 0;
        }
    } else if (data.required_terrain.tree) {
        if (!map_terrain_exist_multiple_tiles_in_radius_with_type(x, y, 2, 1, TERRAIN_TREE | TERRAIN_SHRUB, 3)) {
            set_warning(warning_id, WARNING_TREE_NEEDED);
            return 0;
        }
    } else if (data.required_terrain.water) {
        if (!map_terrain_exists_tile_in_radius_with_type(x, y, 2, 3, TERRAIN_WATER)) {
            set_warning(warning_id, WARNING_WATER_NEEDED);
            return 0;
        }
    } else if (data.required_terrain.wall) {
        if (!map_terrain_all_tiles_in_radius_are(x, y, 2, 0, TERRAIN_WALL)) {
            set_warning(warning_id, WARNING_WALL_NEEDED);
            return 0;
        }
    }
    return 1;
}

void building_construction_update_road_orientation(void)
{
    if (data.road_orientation > 0) {
        if (time_get_millis() - data.road_last_update > 1500) {
            data.road_last_update = time_get_millis();
            data.road_orientation = data.road_orientation == 1 ? 2 : 1;
        }
    }
}

int building_construction_road_orientation(void)
{
    return data.road_orientation;
}

void building_construction_record_view_position(int view_x, int view_y, int grid_offset)
{
    if (grid_offset == data.start.grid_offset) {
        data.start_offset_x_view = view_x;
        data.start_offset_y_view = view_y;
    }
}

void building_construction_get_view_position(int *view_x, int *view_y)
{
    *view_x = data.start_offset_x_view;
    *view_y = data.start_offset_y_view;
}

int building_construction_get_start_grid_offset(void)
{
    return data.start.grid_offset;
}

void building_construction_reset_draw_as_constructing(void)
{
    data.draw_as_constructing = 0;
}

int building_construction_draw_as_constructing(void)
{
    return data.draw_as_constructing;
}
