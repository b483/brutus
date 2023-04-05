#include "building.h"

#include "building/building_state.h"
#include "building/destruction.h"
#include "building/model.h"
#include "building/properties.h"
#include "building/storage.h"
#include "city/buildings.h"
#include "city/data_private.h"
#include "city/population.h"
#include "city/warning.h"
#include "core/calc.h"
#include "figure/figure.h"
#include "figure/formation_legion.h"
#include "figure/route.h"
#include "game/resource.h"
#include "game/undo.h"
#include "map/building_tiles.h"
#include "map/desirability.h"
#include "map/elevation.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/random.h"
#include "map/road_access.h"
#include "map/routing_terrain.h"
#include "map/terrain.h"
#include "map/tiles.h"

#include <string.h>

static building all_buildings[MAX_BUILDINGS];

static struct {
    int highest_id_in_use;
    int highest_id_ever;
    int created_sequence;
    int incorrect_houses;
    int unfixable_houses;
} extra = { 0, 0, 0, 0, 0 };

building *building_get(int id)
{
    return &all_buildings[id];
}

building *building_main(building *b)
{
    for (int guard = 0; guard < 9; guard++) {
        if (b->prev_part_building_id <= 0) {
            return b;
        }
        b = &all_buildings[b->prev_part_building_id];
    }
    return &all_buildings[0];
}

building *building_next(building *b)
{
    return &all_buildings[b->next_part_building_id];
}

building *building_create(building_type type, int x, int y)
{
    building *b = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        if (all_buildings[i].state == BUILDING_STATE_UNUSED && !game_undo_contains_building(i)) {
            b = &all_buildings[i];
            break;
        }
    }
    if (!b) {
        city_warning_show(WARNING_DATA_LIMIT_REACHED);
        return &all_buildings[0];
    }

    const building_properties *props = building_properties_for_type(type);

    memset(&(b->data), 0, sizeof(b->data));

    b->state = BUILDING_STATE_CREATED;
    b->type = type;
    b->size = props->size;
    b->created_sequence = extra.created_sequence++;
    b->sentiment.house_happiness = 50;
    b->distance_from_entry = 0;

    // house size
    b->house_size = 0;
    if (type >= BUILDING_HOUSE_SMALL_TENT && type <= BUILDING_HOUSE_MEDIUM_INSULA) {
        b->house_size = 1;
    } else if (type >= BUILDING_HOUSE_LARGE_INSULA && type <= BUILDING_HOUSE_MEDIUM_VILLA) {
        b->house_size = 2;
    } else if (type >= BUILDING_HOUSE_LARGE_VILLA && type <= BUILDING_HOUSE_MEDIUM_PALACE) {
        b->house_size = 3;
    } else if (type >= BUILDING_HOUSE_LARGE_PALACE && type <= BUILDING_HOUSE_LUXURY_PALACE) {
        b->house_size = 4;
    }

    // subtype
    if (building_is_house(type)) {
        b->subtype.house_level = type - BUILDING_HOUSE_VACANT_LOT;
    } else {
        b->subtype.house_level = 0;
    }

    // input/output resources
    switch (type) {
        case BUILDING_WHEAT_FARM:
            b->output_resource_id = RESOURCE_WHEAT;
            break;
        case BUILDING_VEGETABLE_FARM:
            b->output_resource_id = RESOURCE_VEGETABLES;
            break;
        case BUILDING_FRUIT_FARM:
            b->output_resource_id = RESOURCE_FRUIT;
            break;
        case BUILDING_OLIVE_FARM:
            b->output_resource_id = RESOURCE_OLIVES;
            break;
        case BUILDING_VINES_FARM:
            b->output_resource_id = RESOURCE_VINES;
            break;
        case BUILDING_PIG_FARM:
            b->output_resource_id = RESOURCE_MEAT;
            break;
        case BUILDING_MARBLE_QUARRY:
            b->output_resource_id = RESOURCE_MARBLE;
            break;
        case BUILDING_IRON_MINE:
            b->output_resource_id = RESOURCE_IRON;
            break;
        case BUILDING_TIMBER_YARD:
            b->output_resource_id = RESOURCE_TIMBER;
            break;
        case BUILDING_CLAY_PIT:
            b->output_resource_id = RESOURCE_CLAY;
            break;
        case BUILDING_WINE_WORKSHOP:
            b->output_resource_id = RESOURCE_WINE;
            b->subtype.workshop_type = WORKSHOP_VINES_TO_WINE;
            break;
        case BUILDING_OIL_WORKSHOP:
            b->output_resource_id = RESOURCE_OIL;
            b->subtype.workshop_type = WORKSHOP_OLIVES_TO_OIL;
            break;
        case BUILDING_WEAPONS_WORKSHOP:
            b->output_resource_id = RESOURCE_WEAPONS;
            b->subtype.workshop_type = WORKSHOP_IRON_TO_WEAPONS;
            break;
        case BUILDING_FURNITURE_WORKSHOP:
            b->output_resource_id = RESOURCE_FURNITURE;
            b->subtype.workshop_type = WORKSHOP_TIMBER_TO_FURNITURE;
            break;
        case BUILDING_POTTERY_WORKSHOP:
            b->output_resource_id = RESOURCE_POTTERY;
            b->subtype.workshop_type = WORKSHOP_CLAY_TO_POTTERY;
            break;
        default:
            b->output_resource_id = RESOURCE_NONE;
            break;
    }

    if (type == BUILDING_GRANARY) {
        b->data.granary.resource_stored[RESOURCE_NONE] = 2400;
    }

    b->x = x;
    b->y = y;
    b->grid_offset = map_grid_offset(x, y);
    b->house_figure_generation_delay = map_random_get(b->grid_offset) & 0x7f;
    b->figure_roam_direction = b->house_figure_generation_delay & 6;
    b->fire_proof = props->fire_proof;

    return b;
}

static void building_delete(building *b)
{
    building_clear_related_data(b);
    int id = b->id;
    memset(b, 0, sizeof(building));
    b->id = id;
}

void building_clear_related_data(building *b)
{
    if (b->storage_id) {
        building_storage_delete(b->storage_id);
        b->storage_id = 0;
    }
    if (b->type == BUILDING_SENATE_UPGRADED) {
        city_buildings_remove_senate(b);
    }
    if (b->type == BUILDING_DOCK) {
        city_data.building.working_docks--;
    }
    if (b->type == BUILDING_BARRACKS) {
        city_buildings_remove_barracks(b);
    }
    if (b->type == BUILDING_FORT) {
        if (b->formation_id > 0) {
            if (formations[b->formation_id].in_use) {
                for (int i = 0; i < formations[b->formation_id].num_figures; i++) {
                    figure *f = figure_get(formations[b->formation_id].figures[i]);
                    map_point nearest_barracks_road_tile = { 0 };
                    set_destination__closest_building_of_type(b->id, BUILDING_BARRACKS, &nearest_barracks_road_tile);
                    figure_route_remove(f);
                    if (nearest_barracks_road_tile.x) {
                        f->destination_x = nearest_barracks_road_tile.x;
                        f->destination_y = nearest_barracks_road_tile.y;
                    } else {
                        f->destination_x = city_data.map.exit_point.x;
                        f->destination_y = city_data.map.exit_point.y;
                    }
                    f->action_state = FIGURE_ACTION_SOLDIER_RETURNING_TO_BARRACKS;
                }
                map_figure_delete(figure_get(formations[b->formation_id].legion_standard__figure_id));
                formation_clear(b->formation_id);
                formation_calculate_legion_totals();
            }
        }
    }
    if (b->type == BUILDING_HIPPODROME) {
        city_data.building.hippodrome_placed = 0;
    }
}

void building_update_state(void)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_CREATED) {
            b->state = BUILDING_STATE_IN_USE;
        }
        if (b->state != BUILDING_STATE_IN_USE || !b->house_size) {
            if (b->state == BUILDING_STATE_UNDO || b->state == BUILDING_STATE_DELETED_BY_PLAYER) {
                if (b->type == BUILDING_TOWER || b->type == BUILDING_GATEHOUSE) {
                    map_tiles_update_all_walls();
                    map_tiles_update_all_roads();
                } else if (b->type == BUILDING_RESERVOIR) {
                    map_tiles_update_all_aqueducts(0);
                } else if (b->type == BUILDING_GRANARY) {
                    map_tiles_update_all_roads();
                }
                map_building_tiles_remove(i, b->x, b->y);
                map_routing_update_land();
                building_delete(b);
            } else if (b->state == BUILDING_STATE_RUBBLE) {
                if (b->house_size) {
                    city_population_remove_home_removed(b->house_population);
                }
                building_delete(b);
            } else if (b->state == BUILDING_STATE_DELETED_BY_GAME) {
                building_delete(b);
            }
        }
        if (b->type == BUILDING_TIMBER_YARD && !map_terrain_exist_multiple_tiles_in_radius_with_type(b->x, b->y, 2, 1, TERRAIN_TREE | TERRAIN_SHRUB, 3)) {
            building_destroy_by_plague(b);
        }
    }
}

void building_update_desirability(void)
{
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        b->desirability = map_desirability_get_max(b->x, b->y, b->size);
    }
}

int building_is_house(building_type type)
{
    return type >= BUILDING_HOUSE_VACANT_LOT && type <= BUILDING_HOUSE_LUXURY_PALACE;
}

int building_is_fort(building_type type)
{
    return type == BUILDING_FORT_LEGIONARIES ||
        type == BUILDING_FORT_JAVELIN ||
        type == BUILDING_FORT_MOUNTED;
}

int building_get_highest_id(void)
{
    return extra.highest_id_in_use;
}

void building_update_highest_id(void)
{
    extra.highest_id_in_use = 0;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        if (all_buildings[i].state != BUILDING_STATE_UNUSED) {
            extra.highest_id_in_use = i;
        }
    }
    if (extra.highest_id_in_use > extra.highest_id_ever) {
        extra.highest_id_ever = extra.highest_id_in_use;
    }
}

void set_destination__closest_building_of_type(int closest_to__building_id, int closest_building_of_type__type, map_point *closest_building_of_type__road_tile)
{
    int min_distance = 10000;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        building *b = building_get(i);
        if (b->state == BUILDING_STATE_IN_USE && b->type == closest_building_of_type__type && b->num_workers >= model_get_building(closest_building_of_type__type)->laborers) {
            building *closest_to__building = building_get(closest_to__building_id);
            int dist = calc_maximum_distance(closest_to__building->x, closest_to__building->y, b->x, b->y);
            if (dist < min_distance) {
                if (map_has_road_access(b->x, b->y, b->size, closest_building_of_type__road_tile)) {
                    min_distance = dist;
                }
            }
        }
    }
}

void building_totals_add_corrupted_house(int unfixable)
{
    extra.incorrect_houses++;
    if (unfixable) {
        extra.unfixable_houses++;
    }
}

void building_clear_all(void)
{
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        memset(&all_buildings[i], 0, sizeof(building));
        all_buildings[i].id = i;
    }
    extra.highest_id_in_use = 0;
    extra.highest_id_ever = 0;
    extra.created_sequence = 0;
    extra.incorrect_houses = 0;
    extra.unfixable_houses = 0;
}

void building_save_state(buffer *buf, buffer *highest_id, buffer *highest_id_ever,
                         buffer *sequence, buffer *corrupt_houses)
{
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        building_state_save_to_buffer(buf, &all_buildings[i]);
    }
    buffer_write_i32(highest_id, extra.highest_id_in_use);
    buffer_write_i32(highest_id_ever, extra.highest_id_ever);
    buffer_write_i32(sequence, extra.created_sequence);

    buffer_write_i32(corrupt_houses, extra.incorrect_houses);
    buffer_write_i32(corrupt_houses, extra.unfixable_houses);
}

void building_load_state(buffer *buf, buffer *highest_id, buffer *highest_id_ever,
                         buffer *sequence, buffer *corrupt_houses)
{
    for (int i = 0; i < MAX_BUILDINGS; i++) {
        building_state_load_from_buffer(buf, &all_buildings[i]);
        all_buildings[i].id = i;
    }
    extra.highest_id_in_use = buffer_read_i32(highest_id);
    extra.highest_id_ever = buffer_read_i32(highest_id_ever);
    extra.created_sequence = buffer_read_i32(sequence);

    extra.incorrect_houses = buffer_read_i32(corrupt_houses);
    extra.unfixable_houses = buffer_read_i32(corrupt_houses);
}
