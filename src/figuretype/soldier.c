#include "soldier.h"

#include "city/data.h"
#include "city/map.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/formation_legion.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "figuretype/missile.h"
#include "map/figure.h"
#include "map/grid.h"
#include "sound/effect.h"

#include <stdlib.h>

void figure_military_standard_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 16);

    f->image_id = image_group(GROUP_FIGURE_FORT_STANDARD_POLE) + 20 - legion_formations[f->formation_id].morale / 5;
    if (legion_formations[f->formation_id].figure_type == FIGURE_FORT_LEGIONARY) {
        f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + f->image_offset / 2;
    } else if (legion_formations[f->formation_id].figure_type == FIGURE_FORT_MOUNTED) {
        f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + 18 + f->image_offset / 2;
    } else {
        f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + 9 + f->image_offset / 2;
    }
}

static void update_soldier_image(struct figure_t *f, struct formation_t *m)
{
    int dir;
    if (f->is_shooting) {
        dir = f->direction;
        m->direction = f->direction;
    } else if (f->action_state == FIGURE_ACTION_SOLDIER_AT_STANDARD) {
        dir = m->direction;
    } else if (f->direction < 8) {
        dir = f->direction;
    } else {
        dir = f->previous_tile_direction;
    }
    dir = figure_image_normalize_direction(dir);
    if (f->type == FIGURE_FORT_JAVELIN) {
        int image_id = image_group(GROUP_BUILDING_FORT_JAVELIN);
        if (f->action_state == FIGURE_ACTION_SOLDIER_AT_STANDARD) {
            f->image_id = image_id + 96 + dir + 8 * MISSILE_LAUNCHER_OFFSETS[f->attack_image_offset / 2];
        } else {
            f->image_id = image_id + dir + 8 * f->image_offset;
        }
    } else if (f->type == FIGURE_FORT_MOUNTED) {
        int image_id = image_group(GROUP_FIGURE_FORT_MOUNTED);
        f->image_id = image_id + dir + 8 * f->image_offset;
    } else if (f->type == FIGURE_FORT_LEGIONARY) {
        int image_id = image_group(GROUP_BUILDING_FORT_LEGIONARY);
        if (f->action_state == FIGURE_ACTION_SOLDIER_AT_STANDARD) {
            if (f->figure_is_halted && m->layout == FORMATION_TORTOISE && m->missile_attack_timeout) {
                f->image_id = image_id + dir + 144;
            } else {
                f->image_id = image_id + dir;
            }
        } else {
            f->image_id = image_id + dir + 8 * f->image_offset;
        }
    }
}

static void update_legion_facing_direction(struct formation_t *m)
{
    if (m->layout == FORMATION_DOUBLE_LINE_1 || m->layout == FORMATION_SINGLE_LINE_1) {
        if (m->standard_y < m->prev_standard_y) {
            m->direction = DIR_0_TOP;
        } else if (m->standard_y > m->prev_standard_y) {
            m->direction = DIR_4_BOTTOM;
        }
    } else if (m->layout == FORMATION_DOUBLE_LINE_2 || m->layout == FORMATION_SINGLE_LINE_2) {
        if (m->standard_x < m->prev_standard_x) {
            m->direction = DIR_6_LEFT;
        } else if (m->standard_x > m->prev_standard_x) {
            m->direction = DIR_2_RIGHT;
        }
    } else if (m->layout == FORMATION_TORTOISE) {
        int dx = (m->standard_x < m->prev_standard_x) ? (m->prev_standard_x - m->standard_x) : (m->standard_x - m->prev_standard_x);
        int dy = (m->standard_y < m->prev_standard_y) ? (m->prev_standard_y - m->standard_y) : (m->standard_y - m->prev_standard_y);
        if (dx > dy) {
            if (m->standard_x < m->prev_standard_x) {
                m->direction = DIR_6_LEFT;
            } else if (m->standard_x > m->prev_standard_x) {
                m->direction = DIR_2_RIGHT;
            }
        } else {
            if (m->standard_y < m->prev_standard_y) {
                m->direction = DIR_0_TOP;
            } else if (m->standard_y > m->prev_standard_y) {
                m->direction = DIR_4_BOTTOM;
            }
        }
    }
    m->prev_standard_x = m->standard_x;
    m->prev_standard_y = m->standard_y;
}

void figure_soldier_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    struct formation_t *m = &legion_formations[f->formation_id];
    if (f->is_shooting) {
        f->attack_image_offset++;
        if (f->attack_image_offset > 100) {
            f->attack_image_offset = 0;
            f->is_shooting = 0;
        }
    } else {
        f->wait_ticks_missile++;
        if (f->wait_ticks_missile > 250) {
            f->wait_ticks_missile = 250;
        }
    }
    switch (f->action_state) {
        case FIGURE_ACTION_SOLDIER_AT_REST:
            map_figure_update(f);
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_SOLDIER_GOING_TO_FORT:
            rout_unit(f);
            break;
        case FIGURE_ACTION_SOLDIER_RETURNING_TO_BARRACKS:
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_SOLDIER_GOING_TO_STANDARD:
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                f->action_state = FIGURE_ACTION_SOLDIER_AT_STANDARD;
                if (f->type == FIGURE_FORT_LEGIONARY && rand() % 100 == 1) {
                    sound_effect_play(SOUND_EFFECT_FORMATION_SHIELD);
                }
                update_legion_facing_direction(m);
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_SOLDIER_AT_STANDARD:
            f->image_offset = 0;
            map_figure_update(f);
            if (f->type == FIGURE_FORT_JAVELIN) {
                struct map_point_t tile = { -1, -1 };
                if (f->wait_ticks_missile > figure_properties[f->type].missile_delay && set_missile_target(f, &tile)) {
                    f->is_shooting = 1;
                    f->wait_ticks_missile = 0;
                    f->attack_image_offset = 1;
                    f->direction = calc_missile_shooter_direction(f->x, f->y, tile.x, tile.y);
                    figure_create_missile(f, &tile, figure_properties[f->type].missile_type);
                } else {
                    if (!f->is_shooting) {
                        f->attack_image_offset = 0;
                    }
                }
            } else if (f->type == FIGURE_FORT_LEGIONARY) {
                // attack adjacent enemy
                for (int i = 0; i < 8; i++) {
                    melee_attack_figure_at_offset(f, f->grid_offset + map_grid_direction_delta(i));
                }
            }
            break;
        case FIGURE_ACTION_SOLDIER_GOING_TO_MILITARY_ACADEMY:
            f->is_military_trained = 1;
            if (f->type == FIGURE_FORT_MOUNTED) {
                f->mounted_charge_ticks = 20;
                f->mounted_charge_ticks_max = 20;
            }
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                if (m->is_at_rest) {
                    f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_FORT;
                } else {
                    deploy_legion_unit_to_formation_location(f, m);
                }
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
        case FIGURE_ACTION_SOLDIER_MOPPING_UP:
            struct figure_t *target = melee_unit__set_closest_target(f);
            if (target) {
                figure_movement_move_ticks(f, f->speed_multiplier);
                if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                    figure_route_remove(f);
                    f->action_state = FIGURE_ACTION_SOLDIER_AT_STANDARD;
                    f->target_figure_id = 0;
                }
            } else {
                f->image_offset = 0;
                deploy_legion_unit_to_formation_location(f, m);
            }
            break;
        case FIGURE_ACTION_SOLDIER_GOING_TO_DISTANT_BATTLE:
        {
            f->destination_x = city_data.map.exit_point.x;
            f->destination_y = city_data.map.exit_point.y;
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->is_invisible = 1;
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
        }
        case FIGURE_ACTION_SOLDIER_RETURNING_FROM_DISTANT_BATTLE:
            f->is_invisible = 0;
            f->destination_x = m->standard_x + formation_layout_position_x(FORMATION_AT_REST, f->index_in_formation);
            f->destination_y = m->standard_y + formation_layout_position_y(FORMATION_AT_REST, f->index_in_formation);
            f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_SOLDIER_AT_REST;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                figure_delete(f);
                return;
            }
            break;
    }
    update_soldier_image(f, m);
}
