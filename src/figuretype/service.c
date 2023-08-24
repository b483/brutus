#include "service.h"

#include "building/building.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "game/game.h"
#include "map/map.h"

static void roamer_action(struct figure_t *f, int num_ticks)
{
    switch (f->action_state) {
        case FIGURE_ACTION_ROAMING:
            f->is_invisible = 0;
            f->roam_length++;
            if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                int x, y;
                struct building_t *b = &all_buildings[f->building_id];
                if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x, &y)) {
                    f->action_state = FIGURE_ACTION_ROAMER_RETURNING;
                    f->destination_x = x;
                    f->destination_y = y;
                    figure_route_remove(f);
                    f->roam_length = 0;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            figure_movement_roam_ticks(f, num_ticks);
            break;
        case FIGURE_ACTION_ROAMER_RETURNING:
            figure_movement_move_ticks(f, num_ticks);
            if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
    }
}

static void culture_action(struct figure_t *f, int group)
{
    struct building_t *b = &all_buildings[f->building_id];
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
        figure_delete(f);
        return;
    }
    figure_image_increase_offset(f, 12);
    roamer_action(f, 1);
    f->image_id = image_group(group) + figure_image_direction(f) + 8 * f->image_offset;
}

void figure_priest_action(struct figure_t *f)
{
    culture_action(f, GROUP_FIGURE_PRIEST);
}

void figure_school_child_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    if (b->state != BUILDING_STATE_IN_USE || b->type != BUILDING_SCHOOL) {
        figure_delete(f);
        return;
    }
    figure_image_increase_offset(f, 12);
    switch (f->action_state) {
        case FIGURE_ACTION_ROAMING:
            f->is_invisible = 0;
            f->roam_length++;
            if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                figure_delete(f);
                return;
            }
            figure_movement_roam_ticks(f, 2);
            break;
    }
    f->image_id = image_group(GROUP_FIGURE_SCHOOL_CHILD) + figure_image_direction(f) + 8 * f->image_offset;
}

void figure_teacher_action(struct figure_t *f)
{
    culture_action(f, GROUP_FIGURE_TEACHER_LIBRARIAN);
}

void figure_librarian_action(struct figure_t *f)
{
    culture_action(f, GROUP_FIGURE_TEACHER_LIBRARIAN);
}

void figure_barber_action(struct figure_t *f)
{
    culture_action(f, GROUP_FIGURE_BARBER);
}

void figure_bathhouse_worker_action(struct figure_t *f)
{
    culture_action(f, GROUP_FIGURE_BATHHOUSE_WORKER);
}

void figure_doctor_action(struct figure_t *f)
{
    culture_action(f, GROUP_FIGURE_DOCTOR_SURGEON);
}

void figure_missionary_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
        figure_delete(f);
        return;
    }
    roamer_action(f, 1);
    figure_image_increase_offset(f, 12);
    f->image_id = image_group(GROUP_FIGURE_MISSIONARY) + figure_image_direction(f) + 8 * f->image_offset;
}

void figure_patrician_action(struct figure_t *f)
{
    if (all_buildings[f->building_id].state != BUILDING_STATE_IN_USE) {
        figure_delete(f);
        return;
    }
    roamer_action(f, 1);
    figure_image_increase_offset(f, 12);
    f->image_id = image_group(GROUP_FIGURE_PATRICIAN) + figure_image_direction(f) + 8 * f->image_offset;
}

void figure_labor_seeker_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id2 != f->id) {
        figure_delete(f);
        return;
    }
    roamer_action(f, 1);
    figure_image_increase_offset(f, 12);
    f->image_id = image_group(GROUP_FIGURE_LABOR_SEEKER) + figure_image_direction(f) + 8 * f->image_offset;
}

void figure_market_trader_action(struct figure_t *f)
{
    struct building_t *market = &all_buildings[f->building_id];
    if (market->state != BUILDING_STATE_IN_USE || market->figure_id != f->id) {
        figure_delete(f);
        return;
    }
    if (f->action_state == FIGURE_ACTION_ROAMING) {
        // force return on out of stock
        int max_stock = 0;
        if (market->id > 0 && market->type == BUILDING_MARKET) {
            for (int i = INVENTORY_OIL; i <= INVENTORY_FURNITURE; i++) {
                int stock = market->data.market.inventory[i];
                if (stock > max_stock) {
                    max_stock = stock;
                }
            }
        }
        int stock = building_market_get_max_food_stock(market) + max_stock;
        if (f->roam_length >= 96 && stock <= 0) {
            f->roam_length = figure_properties[f->type].max_roam_length;
        }
    }
    roamer_action(f, 1);
    figure_image_increase_offset(f, 12);
    f->image_id = image_group(GROUP_FIGURE_MARKET_LADY) + figure_image_direction(f) + 8 * f->image_offset;
}

void figure_tax_collector_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];

    f->use_cross_country = 0;
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
        figure_delete(f);
        return;
    }

    switch (f->action_state) {
        case FIGURE_ACTION_TAX_COLLECTOR_CREATED:
            f->is_invisible = 1;
            f->image_offset = 0;
            f->wait_ticks--;
            if (f->wait_ticks <= 0) {
                int x_road, y_road;
                if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                    f->action_state = FIGURE_ACTION_TAX_COLLECTOR_ENTERING_EXITING;
                    figure_movement_set_cross_country_destination(f, x_road, y_road);
                    f->roam_length = 0;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            break;
        case FIGURE_ACTION_TAX_COLLECTOR_ENTERING_EXITING:
            f->use_cross_country = 1;
            f->is_invisible = 1;
            if (figure_movement_move_ticks_cross_country(f, 1) == 1) {
                if (map_building_at(f->grid_offset) == f->building_id) {
                    // returned to own building
                    figure_delete(f);
                    return;
                } else {
                    f->action_state = FIGURE_ACTION_TAX_COLLECTOR_ROAMING;
                    figure_movement_init_roaming(f);
                    f->roam_length = 0;
                }
            }
            break;
        case FIGURE_ACTION_TAX_COLLECTOR_ROAMING:
            f->is_invisible = 0;
            f->roam_length++;
            if (f->roam_length >= figure_properties[f->type].max_roam_length) {
                int x_road, y_road;
                if (map_closest_road_within_radius(b->x, b->y, b->size, 2, &x_road, &y_road)) {
                    f->action_state = FIGURE_ACTION_TAX_COLLECTOR_RETURNING;
                    f->destination_x = x_road;
                    f->destination_y = y_road;
                } else {
                    figure_delete(f);
                    return;
                }
            }
            figure_movement_roam_ticks(f, 1);
            break;
        case FIGURE_ACTION_TAX_COLLECTOR_RETURNING:
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_TAX_COLLECTOR_ENTERING_EXITING;
                figure_movement_set_cross_country_destination(f, b->x, b->y);
                f->roam_length = 0;
            } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
    }
    figure_image_increase_offset(f, 12);
    f->image_id = image_group(GROUP_FIGURE_TAX_COLLECTOR) + figure_image_direction(f) + 8 * f->image_offset;
}
