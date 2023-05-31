#include "city_overlay_risks.h"

#include "building/industry.h"
#include "game/state.h"
#include "graphics/image.h"
#include "map/building.h"
#include "map/image.h"
#include "map/property.h"
#include "map/random.h"
#include "map/terrain.h"

static int is_problem_cartpusher(int figure_id)
{
    if (figure_id) {
        struct figure_t *fig = &figures[figure_id];
        return fig->action_state == FIGURE_ACTION_CARTPUSHER_INITIAL && fig->min_max_seen;
    } else {
        return 0;
    }
}

void city_overlay_problems_prepare_building(struct building_t *b)
{
    if (b->house_size) {
        return;
    }
    if (b->type == BUILDING_FOUNTAIN || b->type == BUILDING_BATHHOUSE) {
        if (!b->has_water_access) {
            b->show_on_problem_overlay = 1;
        }
    } else if (b->type >= BUILDING_WHEAT_FARM && b->type <= BUILDING_CLAY_PIT) {
        if (is_problem_cartpusher(b->figure_id)) {
            b->show_on_problem_overlay = 1;
        }
    } else if (building_is_workshop(b->type)) {
        if (is_problem_cartpusher(b->figure_id)) {
            b->show_on_problem_overlay = 1;
        } else if (b->loads_stored <= 0) {
            b->show_on_problem_overlay = 1;
        }
    }
}

static int show_building_fire_crime(const struct building_t *b)
{
    return b->type == BUILDING_PREFECTURE || b->type == BUILDING_BURNING_RUIN;
}

static int show_building_damage(const struct building_t *b)
{
    return b->type == BUILDING_ENGINEERS_POST;
}

static int show_building_problems(const struct building_t *b)
{
    return b->show_on_problem_overlay;
}

static int show_building_native(const struct building_t *b)
{
    return b->type == BUILDING_NATIVE_HUT || b->type == BUILDING_NATIVE_MEETING || b->type == BUILDING_MISSION_POST;
}

static int show_figure_fire(const struct figure_t *f)
{
    return f->type == FIGURE_PREFECT;
}

static int show_figure_damage(const struct figure_t *f)
{
    return f->type == FIGURE_ENGINEER;
}

static int show_figure_crime(const struct figure_t *f)
{
    return f->type == FIGURE_PREFECT || f->type == FIGURE_PROTESTER ||
        f->type == FIGURE_CRIMINAL || f->type == FIGURE_RIOTER;
}

static int show_figure_problems(const struct figure_t *f)
{
    if (f->type == FIGURE_LABOR_SEEKER) {
        return all_buildings[f->building_id].show_on_problem_overlay;
    } else if (f->type == FIGURE_CART_PUSHER) {
        return f->action_state == FIGURE_ACTION_CARTPUSHER_INITIAL || f->min_max_seen;
    } else {
        return 0;
    }
}

static int show_figure_native(const struct figure_t *f)
{
    return f->type == FIGURE_INDIGENOUS_NATIVE || f->type == FIGURE_MISSIONARY;
}

static int get_column_height_fire(const struct building_t *b)
{
    return b->fire_risk > 0 ? b->fire_risk / 10 : NO_COLUMN;
}

static int get_column_height_damage(const struct building_t *b)
{
    return b->damage_risk > 0 ? b->damage_risk / 20 : NO_COLUMN;
}

static int get_column_height_crime(const struct building_t *b)
{
    if (b->house_size) {
        int happiness = b->sentiment.house_happiness;
        if (happiness <= 0) {
            return 10;
        } else if (happiness <= 10) {
            return 8;
        } else if (happiness <= 20) {
            return 6;
        } else if (happiness <= 30) {
            return 4;
        } else if (happiness <= 40) {
            return 2;
        } else if (happiness < 50) {
            return 1;
        }
    }
    return NO_COLUMN;
}

static int get_column_height_none(__attribute__((unused)) const struct building_t *b)
{
    return NO_COLUMN;
}

const struct city_overlay_t *city_overlay_for_fire(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_FIRE,
        COLUMN_TYPE_RISK,
        show_building_fire_crime,
        show_figure_fire,
        get_column_height_fire,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_damage(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_DAMAGE,
        COLUMN_TYPE_RISK,
        show_building_damage,
        show_figure_damage,
        get_column_height_damage,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_crime(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_CRIME,
        COLUMN_TYPE_RISK,
        show_building_fire_crime,
        show_figure_crime,
        get_column_height_crime,
        0,
        0
    };
    return &overlay;
}

const struct city_overlay_t *city_overlay_for_problems(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_PROBLEMS,
        COLUMN_TYPE_RISK,
        show_building_problems,
        show_figure_problems,
        get_column_height_none,
        0,
        0
    };
    return &overlay;
}

static int terrain_on_native_overlay(void)
{
    return
        TERRAIN_SHRUB | TERRAIN_ROCK | TERRAIN_WATER | TERRAIN_TREE |
        TERRAIN_GARDEN | TERRAIN_ELEVATION | TERRAIN_ACCESS_RAMP | TERRAIN_RUBBLE;
}

static void draw_footprint_native(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }
    if (map_terrain_is(grid_offset, terrain_on_native_overlay())) {
        if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
            city_with_overlay_draw_building_footprint(x, y, grid_offset, 0);
        } else {
            image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, 0);
        }
    } else if (map_terrain_is(grid_offset, TERRAIN_AQUEDUCT | TERRAIN_WALL)) {
        // display grass
        int image_id = image_group(GROUP_TERRAIN_GRASS_1) + (map_random_get(grid_offset) & 7);
        image_draw_isometric_footprint_from_draw_tile(image_id, x, y, 0);
    } else if (map_terrain_is(grid_offset, TERRAIN_BUILDING)) {
        city_with_overlay_draw_building_footprint(x, y, grid_offset, 0);
    } else {
        if (map_property_is_native_land(grid_offset)) {
            image_draw_isometric_footprint_from_draw_tile(image_group(GROUP_TERRAIN_DESIRABILITY) + 1, x, y, 0);
        } else {
            image_draw_isometric_footprint_from_draw_tile(map_image_at(grid_offset), x, y, 0);
        }
    }
}

static void draw_top_native(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }
    if (map_terrain_is(grid_offset, terrain_on_native_overlay())) {
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

const struct city_overlay_t *city_overlay_for_native(void)
{
    static struct city_overlay_t overlay = {
        OVERLAY_NATIVE,
        COLUMN_TYPE_RISK,
        show_building_native,
        show_figure_native,
        get_column_height_none,
        draw_footprint_native,
        draw_top_native
    };
    return &overlay;
}
