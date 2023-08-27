#include "city_overlay_other.h"

#include "city/city_new.h"
#include "core/calc.h"
#include "core/config.h"
#include "core/image.h"
#include "game/game.h"
#include "graphics/graphics.h"
#include "map/map.h"

static int show_building_religion(const struct building_t *b)
{
    return
        b->type == BUILDING_ORACLE || b->type == BUILDING_SMALL_TEMPLE_CERES ||
        b->type == BUILDING_SMALL_TEMPLE_NEPTUNE || b->type == BUILDING_SMALL_TEMPLE_MERCURY ||
        b->type == BUILDING_SMALL_TEMPLE_MARS || b->type == BUILDING_SMALL_TEMPLE_VENUS ||
        b->type == BUILDING_LARGE_TEMPLE_CERES || b->type == BUILDING_LARGE_TEMPLE_NEPTUNE ||
        b->type == BUILDING_LARGE_TEMPLE_MERCURY || b->type == BUILDING_LARGE_TEMPLE_MARS ||
        b->type == BUILDING_LARGE_TEMPLE_VENUS;
}

static int show_building_food_stocks(const struct building_t *b)
{
    return b->type == BUILDING_MARKET || b->type == BUILDING_WHARF || b->type == BUILDING_GRANARY;
}

static int show_building_tax_income(const struct building_t *b)
{
    return b->type == BUILDING_FORUM || b->type == BUILDING_SENATE;
}

static int show_building_water(const struct building_t *b)
{
    return b->type == BUILDING_WELL || b->type == BUILDING_FOUNTAIN || b->type == BUILDING_RESERVOIR;
}

static int show_building_desirability(__attribute__((unused)) const struct building_t *b)
{
    return 0;
}

static int show_figure_religion(const struct figure_t *f)
{
    return f->type == FIGURE_PRIEST;
}

static int show_figure_food_stocks(const struct figure_t *f)
{
    if (f->type == FIGURE_MARKET_BUYER || f->type == FIGURE_MARKET_TRADER ||
        f->type == FIGURE_DELIVERY_BOY || f->type == FIGURE_FISHING_BOAT) {
        return 1;
    } else if (f->type == FIGURE_CART_PUSHER) {
        return resource_is_food(f->resource_id);
    }
    return 0;
}

static int show_figure_tax_income(const struct figure_t *f)
{
    return f->type == FIGURE_TAX_COLLECTOR;
}

static int show_figure_none(__attribute__((unused)) const struct figure_t *f)
{
    return 0;
}

static int get_column_height_religion(const struct building_t *b)
{
    return b->house_size && b->data.house.num_gods ? b->data.house.num_gods * 17 / 10 : NO_COLUMN;
}

static int get_column_height_food_stocks(const struct building_t *b)
{
    if (b->house_size && house_properties[b->subtype.house_level].food_types) {
        int pop = b->house_population;
        int stocks = 0;
        for (int i = INVENTORY_WHEAT; i <= INVENTORY_MEAT; i++) {
            stocks += b->data.house.inventory[i];
        }
        int pct_stocks = calc_percentage(stocks, pop);
        if (pct_stocks <= 0) {
            return 10;
        } else if (pct_stocks < 100) {
            return 5;
        } else if (pct_stocks <= 200) {
            return 1;
        }
    }
    return NO_COLUMN;
}

static int get_column_height_tax_income(const struct building_t *b)
{
    if (b->house_size) {
        int pct = calc_adjust_with_percentage(b->tax_income_or_storage / 2, city_data.finance.tax_percentage);
        if (pct > 0) {
            return pct / 25;
        }
    }
    return NO_COLUMN;
}

static int get_column_height_none(__attribute__((unused)) const struct building_t *b)
{
    return NO_COLUMN;
}

const struct city_overlay_t *city_overlay_for_religion(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_RELIGION,
        COLUMN_TYPE_ACCESS,
        show_building_religion,
        show_figure_religion,
        get_column_height_religion,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_food_stocks(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_FOOD_STOCKS,
        COLUMN_TYPE_RISK,
        show_building_food_stocks,
        show_figure_food_stocks,
        get_column_height_food_stocks,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_tax_income(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_TAX_INCOME,
        COLUMN_TYPE_ACCESS,
        show_building_tax_income,
        show_figure_tax_income,
        get_column_height_tax_income,
        0,
        0
    };
    return &overlay;
}

static int has_deleted_building(int grid_offset)
{
    if (!config_get(CONFIG_UI_VISUAL_FEEDBACK_ON_DELETE)) {
        return 0;
    }
    struct building_t *b = &all_buildings[map_building_at(grid_offset)];
    b = building_main(b);
    return b->id && (b->is_deleted || map_property_is_deleted(b->grid_offset));
}

static int terrain_on_water_overlay(void)
{
    return
        TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_TREE |
        TERRAIN_GARDEN | TERRAIN_ROAD | TERRAIN_AQUEDUCT | TERRAIN_ELEVATION |
        TERRAIN_ACCESS_RAMP | TERRAIN_RUBBLE;
}

static void draw_footprint_water(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }
    if (map_terrain_is(grid_offset, terrain_on_water_overlay())) {
        if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
            city_with_overlay_draw_building_footprint(x, y, grid_offset, 0);
        } else {
            image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, 0);
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_WALL)) {
        // display grass
        int image_id = image_group(GROUP_TERRAIN_GRASS_1) + (map_random_get(grid_offset) & 7);
        image_draw_isometric_footprint_from_draw_tile(image_id, x, y, 0);
    } else if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        struct building_t *b = &all_buildings[map_building_at(grid_offset)];
        int terrain = terrain_grid.items[grid_offset];
        if (b->id && (b->has_well_access || (b->house_size && b->has_water_access))) {
            terrain |= TERRAIN_FOUNTAIN_RANGE;
        }
        int image_offset;
        switch (terrain & (TERRAIN_RESERVOIR_RANGE | TERRAIN_FOUNTAIN_RANGE)) {
            case TERRAIN_RESERVOIR_RANGE | TERRAIN_FOUNTAIN_RANGE:
                image_offset = 24;
                break;
            case TERRAIN_RESERVOIR_RANGE:
                image_offset = 8;
                break;
            case TERRAIN_FOUNTAIN_RANGE:
                image_offset = 16;
                break;
            default:
                image_offset = 0;
                break;
        }
        city_with_overlay_draw_building_footprint(x, y, grid_offset, image_offset);
    } else {
        int image_id = image_group(GROUP_TERRAIN_OVERLAY);
        switch (terrain_grid.items[grid_offset] & (TERRAIN_RESERVOIR_RANGE | TERRAIN_FOUNTAIN_RANGE)) {
            case TERRAIN_RESERVOIR_RANGE | TERRAIN_FOUNTAIN_RANGE:
                image_id += 27;
                break;
            case TERRAIN_RESERVOIR_RANGE:
                image_id += 11;
                break;
            case TERRAIN_FOUNTAIN_RANGE:
                image_id += 19;
                break;
            default:
                image_id = map_image_at(grid_offset);
                break;
        }
        image_draw_isometric_footprint_from_draw_tile(image_id, x, y, 0);
    }
}

static void draw_top_water(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }
    if (map_terrain_is(grid_offset, terrain_on_water_overlay())) {
        if (!map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
            color_t color_mask = 0;
            if (map_property_is_deleted(grid_offset) && map_property_multi_tile_size(grid_offset) == 1) {
                color_mask = COLOR_MASK_RED;
            }
            image_draw_isometric_top_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
        }
    } else if (map_building_at(grid_offset)) {
        city_with_overlay_draw_building_top(x, y, grid_offset);
    }
}

const struct city_overlay_t *city_overlay_for_water(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_WATER,
        COLUMN_TYPE_ACCESS,
        show_building_water,
        show_figure_none,
        get_column_height_none,
        draw_footprint_water,
        draw_top_water
    };
    return &overlay;
}

static int terrain_on_desirability_overlay(void)
{
    return
        TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER |
        TERRAIN_TREE | TERRAIN_GARDEN | TERRAIN_ROAD |
        TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP | TERRAIN_RUBBLE;
}

static int get_desirability_image_offset(int desirability)
{
    if (desirability < -10) {
        return 0;
    } else if (desirability < -5) {
        return 1;
    } else if (desirability < 0) {
        return 2;
    } else if (desirability == 1) {
        return 3;
    } else if (desirability < 5) {
        return 4;
    } else if (desirability < 10) {
        return 5;
    } else if (desirability < 15) {
        return 6;
    } else if (desirability < 20) {
        return 7;
    } else if (desirability < 25) {
        return 8;
    } else {
        return 9;
    }
}

static void draw_footprint_desirability(int x, int y, int grid_offset)
{
    color_t color_mask = map_property_is_deleted(grid_offset) ? COLOR_MASK_RED : 0;
    if (map_terrain_is(grid_offset, terrain_on_desirability_overlay())
        && !map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        // display normal tile
        if (map_property_is_draw_tile(grid_offset)) {
            image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT | TERRAIN_WALL)) {
        // display empty land/grass
        int image_id = image_group(GROUP_TERRAIN_GRASS_1) + (map_random_get(grid_offset) & 7);
        image_draw_isometric_footprint_from_draw_tile(image_id, x, y, color_mask);
    } else if (map_terrain_is(grid_offset, TERRAIN_BUILDING) || map_desirability_get(grid_offset)) {
        if (has_deleted_building(grid_offset)) {
            color_mask = COLOR_MASK_RED;
        }
        int offset = get_desirability_image_offset(map_desirability_get(grid_offset));
        image_draw_isometric_footprint_from_draw_tile(
            image_group(GROUP_TERRAIN_DESIRABILITY) + offset, x, y, color_mask);
    } else {
        image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
    }
}

static void draw_top_desirability(int x, int y, int grid_offset)
{
    color_t color_mask = map_property_is_deleted(grid_offset) ? COLOR_MASK_RED : 0;
    if (map_terrain_is(grid_offset, terrain_on_desirability_overlay())
        && !map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        // display normal tile
        if (map_property_is_draw_tile(grid_offset)) {
            image_draw_isometric_top_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT | TERRAIN_WALL)) {
        // grass, no top needed
    } else if (map_terrain_is(grid_offset, TERRAIN_BUILDING) || map_desirability_get(grid_offset)) {
        if (has_deleted_building(grid_offset)) {
            color_mask = COLOR_MASK_RED;
        }
        int offset = get_desirability_image_offset(map_desirability_get(grid_offset));
        image_draw_isometric_top_from_draw_tile(image_group(GROUP_TERRAIN_DESIRABILITY) + offset, x, y, color_mask);
    } else {
        image_draw_isometric_top_from_draw_tile(map_image_at(grid_offset), x, y, color_mask);
    }
}

const struct city_overlay_t *city_overlay_for_desirability(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_DESIRABILITY,
        COLUMN_TYPE_ACCESS,
        show_building_desirability,
        show_figure_none,
        get_column_height_none,
        draw_footprint_desirability,
        draw_top_desirability
    };
    return &overlay;
}
