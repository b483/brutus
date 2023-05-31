#include "migrant.h"

#include "building/house.h"
#include "city/data.h"
#include "city/map.h"
#include "city/population.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "map/road_access.h"

void figure_create_immigrant(struct building_t *house, int num_people)
{
    struct figure_t *f = figure_create(FIGURE_IMMIGRANT, city_data.map.entry_point.x, city_data.map.entry_point.y, DIR_0_TOP);
    f->action_state = FIGURE_ACTION_IMMIGRANT_CREATED;
    f->is_targetable = 1;
    f->terrain_usage = TERRAIN_USAGE_ANY;
    f->immigrant_building_id = house->id;
    f->wait_ticks = 10 + (house->house_figure_generation_delay & 0x7f);
    f->migrant_num_people = num_people;
    house->immigrant_figure_id = f->id;
}

void figure_create_emigrant(struct building_t *house, int num_people)
{
    city_population_remove(num_people);
    if (num_people < house->house_population) {
        house->house_population -= num_people;
    } else {
        house->house_population = 0;
        building_house_change_to_vacant_lot(house);
    }
    struct figure_t *f = figure_create(FIGURE_EMIGRANT, house->x, house->y, DIR_0_TOP);
    f->action_state = FIGURE_ACTION_EMIGRANT_CREATED;
    f->is_targetable = 1;
    f->terrain_usage = TERRAIN_USAGE_ANY;
    f->wait_ticks = 0;
    f->migrant_num_people = num_people;
}

void figure_create_homeless(int x, int y, int num_people)
{
    struct figure_t *f = figure_create(FIGURE_HOMELESS, x, y, DIR_0_TOP);
    f->action_state = FIGURE_ACTION_HOMELESS_CREATED;
    f->is_targetable = 1;
    f->terrain_usage = TERRAIN_USAGE_PREFER_ROADS;
    f->wait_ticks = 0;
    f->migrant_num_people = num_people;
    city_population_remove_homeless(num_people);
}

static void update_migrant_dir_and_image(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    f->image_id = image_group(GROUP_FIGURE_MIGRANT) + figure_image_direction(f) + 8 * f->image_offset;
    if (f->action_state == FIGURE_ACTION_IMMIGRANT_ARRIVING || f->action_state == FIGURE_ACTION_EMIGRANT_LEAVING) {
        int dir = figure_image_direction(f);
        f->cart_image_id = image_group(GROUP_FIGURE_MIGRANT_CART) + dir;
        figure_image_set_cart_offset(f, (dir + 4) % 8);
    }
}

static int closest_house_with_room(int x, int y)
{
    int min_dist = 1000;
    int min_building_id = 0;
    int max_id = building_get_highest_id();
    for (int i = 1; i <= max_id; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state == BUILDING_STATE_IN_USE && b->house_size
            && b->distance_from_entry > 0 && b->house_population_room > 0) {
            if (!b->immigrant_figure_id) {
                int dist = calc_maximum_distance(x, y, b->x, b->y);
                if (dist < min_dist) {
                    min_dist = dist;
                    min_building_id = i;
                }
            }
        }
    }
    return min_building_id;

}

void figure_immigrant_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->immigrant_building_id];

    f->cart_image_id = 0;
    if (b->state != BUILDING_STATE_IN_USE || b->immigrant_figure_id != f->id || !b->house_size) {
        figure_delete(f);
        return;
    }

    switch (f->action_state) {
        case FIGURE_ACTION_IMMIGRANT_CREATED:
            f->is_invisible = 1;
            f->wait_ticks--;
            if (f->wait_ticks <= 0) {
                int x_road, y_road;
                if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                    f->action_state = FIGURE_ACTION_IMMIGRANT_ARRIVING;
                    f->destination_x = x_road;
                    f->destination_y = y_road;
                    f->roam_length = 0;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            break;
        case FIGURE_ACTION_IMMIGRANT_ARRIVING:
            f->is_invisible = 0;
            figure_movement_move_ticks(f, 1);
            switch (f->direction) {
                case DIR_FIGURE_AT_DESTINATION:
                    f->action_state = FIGURE_ACTION_IMMIGRANT_ENTERING_HOUSE;
                    figure_movement_set_cross_country_destination(f, b->x, b->y);
                    f->roam_length = 0;
                    break;
                case DIR_FIGURE_REROUTE:
                    figure_route_remove(f);
                    break;
                case DIR_FIGURE_LOST:
                    b->immigrant_figure_id = 0;
                    b->distance_from_entry = 0;
                    figure_delete(f);
                    return;
            }
            break;
        case FIGURE_ACTION_IMMIGRANT_ENTERING_HOUSE:
            f->use_cross_country = 1;
            f->is_invisible = 1;
            if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                int max_people = house_properties[b->subtype.house_level].max_people;
                if (b->house_is_merged) {
                    max_people *= 4;
                }
                int room = max_people - b->house_population;
                if (room < 0) {
                    room = 0;
                }
                if (room < f->migrant_num_people) {
                    f->migrant_num_people = room;
                }
                if (!b->house_population) {
                    building_house_change_to(b, BUILDING_HOUSE_SMALL_TENT);
                }
                b->house_population += f->migrant_num_people;
                b->house_population_room = max_people - b->house_population;
                city_population_add(f->migrant_num_people);
                b->immigrant_figure_id = 0;
                figure_delete(f);
                return;
            }
            f->is_invisible = f->in_building_wait_ticks ? 1 : 0;
            break;
    }
    update_migrant_dir_and_image(f);
}

void figure_emigrant_action(struct figure_t *f)
{
    f->cart_image_id = 0;

    switch (f->action_state) {
        case FIGURE_ACTION_EMIGRANT_CREATED:
            f->is_invisible = 1;
            f->wait_ticks++;
            if (f->wait_ticks >= 5) {
                int x_road, y_road;
                if (!map_closest_road_within_radius(f->x, f->y, 1, 5, &x_road, &y_road)) {
                    figure_delete(f);
                    return;
                }
                f->action_state = FIGURE_ACTION_EMIGRANT_EXITING_HOUSE;
                figure_movement_set_cross_country_destination(f, x_road, y_road);
                f->roam_length = 0;
            }
            break;
        case FIGURE_ACTION_EMIGRANT_EXITING_HOUSE:
            f->use_cross_country = 1;
            f->is_invisible = 1;
            if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                f->action_state = FIGURE_ACTION_EMIGRANT_LEAVING;
                f->destination_x = city_data.map.entry_point.x;
                f->destination_y = city_data.map.entry_point.y;
                f->roam_length = 0;
                f->progress_on_tile = 15;
            }
            f->is_invisible = f->in_building_wait_ticks ? 1 : 0;
            break;
        case FIGURE_ACTION_EMIGRANT_LEAVING:
            f->use_cross_country = 0;
            f->is_invisible = 0;
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                f->direction == DIR_FIGURE_REROUTE ||
                f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
    }
    update_migrant_dir_and_image(f);
}

void figure_homeless_action(struct figure_t *f)
{
    switch (f->action_state) {
        case FIGURE_ACTION_HOMELESS_CREATED:
            f->image_offset = 0;
            f->wait_ticks++;
            if (f->wait_ticks > 51) {
                int building_id = closest_house_with_room(f->x, f->y);
                if (building_id) {
                    struct building_t *b = &all_buildings[building_id];
                    int x_road, y_road;
                    if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                        b->immigrant_figure_id = f->id;
                        f->immigrant_building_id = building_id;
                        f->action_state = FIGURE_ACTION_HOMELESS_GOING_TO_HOUSE;
                        f->destination_x = x_road;
                        f->destination_y = y_road;
                        f->roam_length = 0;
                    } else {
                        figure_delete(f);
                        return;
                    }
                } else {
                    f->action_state = FIGURE_ACTION_HOMELESS_LEAVING;
                    f->destination_x = city_data.map.exit_point.x;
                    f->destination_y = city_data.map.exit_point.y;
                    f->roam_length = 0;
                    f->wait_ticks = 0;
                }
            }
            break;
        case FIGURE_ACTION_HOMELESS_GOING_TO_HOUSE:
            f->is_invisible = 0;
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                all_buildings[f->immigrant_building_id].immigrant_figure_id = 0;
                figure_delete(f);
                return;
            } else if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                struct building_t *b = &all_buildings[f->immigrant_building_id];
                f->action_state = FIGURE_ACTION_HOMELESS_ENTERING_HOUSE;
                figure_movement_set_cross_country_destination(f, b->x, b->y);
                f->roam_length = 0;
            }
            break;
        case FIGURE_ACTION_HOMELESS_ENTERING_HOUSE:
            f->use_cross_country = 1;
            f->is_invisible = 1;
            if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                struct building_t *b = &all_buildings[f->immigrant_building_id];
                if (f->immigrant_building_id && building_is_house(b->type)) {
                    int max_people = house_properties[b->subtype.house_level].max_people;
                    if (b->house_is_merged) {
                        max_people *= 4;
                    }
                    int room = max_people - b->house_population;
                    if (room < 0) {
                        room = 0;
                    }
                    if (room < f->migrant_num_people) {
                        f->migrant_num_people = room;
                    }
                    if (!b->house_population) {
                        building_house_change_to(b, BUILDING_HOUSE_SMALL_TENT);
                    }
                    b->house_population += f->migrant_num_people;
                    b->house_population_room = max_people - b->house_population;
                    city_population_add_homeless(f->migrant_num_people);
                    b->immigrant_figure_id = 0;
                }
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_HOMELESS_LEAVING:
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            }
            f->wait_ticks++;
            if (f->wait_ticks > 30) {
                f->wait_ticks = 0;
                int building_id = closest_house_with_room(f->x, f->y);
                if (building_id > 0) {
                    struct building_t *b = &all_buildings[building_id];
                    int x_road, y_road;
                    if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                        b->immigrant_figure_id = f->id;
                        f->immigrant_building_id = building_id;
                        f->action_state = FIGURE_ACTION_HOMELESS_GOING_TO_HOUSE;
                        f->destination_x = x_road;
                        f->destination_y = y_road;
                        f->roam_length = 0;
                        figure_route_remove(f);
                    }
                }
            }
            break;
    }
    figure_image_increase_offset(f, 12);
    f->image_id = image_group(GROUP_FIGURE_HOMELESS) + figure_image_direction(f) + 8 * f->image_offset;
}
