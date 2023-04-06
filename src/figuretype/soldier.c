#include "soldier.h"

#include "city/data_private.h"
#include "city/map.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/formation.h"
#include "figure/formation_legion.h"
#include "figure/image.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "figuretype/missile.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/point.h"
#include "sound/effect.h"

#include <stdlib.h>

void figure_military_standard_action(struct figure_t *f)
{
    f->terrain_usage = TERRAIN_USAGE_ANY;
    figure_image_increase_offset(f, 16);

    f->image_id = image_group(GROUP_FIGURE_FORT_STANDARD_POLE) + 20 - formations[f->formation_id].morale / 5;
    if (formations[f->formation_id].figure_type == FIGURE_FORT_LEGIONARY) {
        f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + f->image_offset / 2;
    } else if (formations[f->formation_id].figure_type == FIGURE_FORT_MOUNTED) {
        f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + 18 + f->image_offset / 2;
    } else {
        f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + 9 + f->image_offset / 2;
    }
}

static void update_image(struct figure_t *f, const struct formation_t *m)
{
    int dir;
    if (f->action_state == FIGURE_ACTION_ATTACK) {
        dir = f->attack_direction;
    } else if (m->missile_fired) {
        dir = f->direction;
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
        if (f->action_state == FIGURE_ACTION_ATTACK) {
            if (f->attack_image_offset < 12) {
                f->image_id = image_id + 96 + dir;
            } else {
                f->image_id = image_id + 96 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            }
        } else if (f->action_state == FIGURE_ACTION_CORPSE) {
            f->image_id = image_id + 144 + figure_image_corpse_offset(f);
        } else if (f->action_state == FIGURE_ACTION_SOLDIER_AT_STANDARD) {
            f->image_id = image_id + 96 + dir +
                8 * figure_image_missile_launcher_offset(f);
        } else {
            f->image_id = image_id + dir + 8 * f->image_offset;
        }
    } else if (f->type == FIGURE_FORT_MOUNTED) {
        int image_id = image_group(GROUP_FIGURE_FORT_MOUNTED);
        if (f->action_state == FIGURE_ACTION_ATTACK) {
            if (f->attack_image_offset < 12) {
                f->image_id = image_id + 96 + dir;
            } else {
                f->image_id = image_id + 96 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            }
        } else if (f->action_state == FIGURE_ACTION_CORPSE) {
            f->image_id = image_id + 144 + figure_image_corpse_offset(f);
        } else {
            f->image_id = image_id + dir + 8 * f->image_offset;
        }
    } else if (f->type == FIGURE_FORT_LEGIONARY) {
        int image_id = image_group(GROUP_BUILDING_FORT_LEGIONARY);
        if (f->action_state == FIGURE_ACTION_ATTACK) {
            if (f->attack_image_offset < 12) {
                f->image_id = image_id + 96 + dir;
            } else {
                f->image_id = image_id + 96 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            }
        } else if (f->action_state == FIGURE_ACTION_CORPSE) {
            f->image_id = image_id + 152 + figure_image_corpse_offset(f);
        } else if (f->action_state == FIGURE_ACTION_SOLDIER_AT_STANDARD) {
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

void figure_soldier_action(struct figure_t *f)
{
    city_data.figure.soldiers++;
    f->terrain_usage = TERRAIN_USAGE_ANY;
    figure_image_increase_offset(f, 12);
    struct formation_t *legion_formation = &formations[f->formation_id];

    switch (f->action_state) {
        case FIGURE_ACTION_SOLDIER_AT_REST:
            map_figure_update(f);
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_SOLDIER_GOING_TO_FORT:
        case FIGURE_ACTION_FLEEING:
            f->destination_x = legion_formation->x + formation_layout_position_x(FORMATION_AT_REST, f->index_in_formation);
            f->destination_y = legion_formation->y + formation_layout_position_y(FORMATION_AT_REST, f->index_in_formation);
            f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_SOLDIER_AT_REST;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
        case FIGURE_ACTION_SOLDIER_RETURNING_TO_BARRACKS:
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_SOLDIER_GOING_TO_STANDARD:
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                f->action_state = FIGURE_ACTION_SOLDIER_AT_STANDARD;
                f->image_offset = 0;
                if (f->type == FIGURE_FORT_LEGIONARY && rand() % 100 == 1) {
                    sound_effect_play(SOUND_EFFECT_FORMATION_SHIELD);
                }
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_SOLDIER_AT_STANDARD:
            f->image_offset = 0;
            map_figure_update(f);
            if (f->type == FIGURE_FORT_JAVELIN) {
                map_point tile = { -1, -1 };
                f->wait_ticks_missile++;
                if (f->wait_ticks_missile > f->missile_delay) {
                    f->wait_ticks_missile = 0;
                    int target_acquired = 0;
                    if (f->is_military_trained) {
                        target_acquired = set_missile_target(f, &tile, 1) || set_missile_target(f, &tile, 0);
                    } else {
                        target_acquired = set_missile_target(f, &tile, 0);
                    }
                    if (target_acquired) {
                        f->attack_image_offset = 1;
                        f->direction = calc_missile_shooter_direction(f->x, f->y, tile.x, tile.y);
                    } else {
                        f->attack_image_offset = 0;
                    }
                }
                if (f->attack_image_offset) {
                    if (f->attack_image_offset == 1) {
                        if (tile.x == -1 || tile.y == -1) {
                            map_point_get_last_result(&tile);
                        }
                        figure_create_missile(f, &tile, f->missile_type);
                        formations[f->formation_id].missile_fired = 6;
                    }
                    f->attack_image_offset++;
                    if (f->attack_image_offset > 100) {
                        f->attack_image_offset = 0;
                    }
                }
            } else if (f->type == FIGURE_FORT_LEGIONARY) {
                // attack adjacent enemy
                for (int i = 0; i < 8 && f->action_state != FIGURE_ACTION_ATTACK; i++) {
                    figure_combat_attack_figure_at(f, f->grid_offset + map_grid_direction_delta(i));
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
                if (legion_formation->is_at_rest) {
                    f->action_state = FIGURE_ACTION_SOLDIER_GOING_TO_FORT;
                } else {
                    deploy_legion_unit_to_formation_location(f, legion_formation);
                }
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
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
                deploy_legion_unit_to_formation_location(f, legion_formation);
            }
            break;
        case FIGURE_ACTION_SOLDIER_GOING_TO_DISTANT_BATTLE:
        {
            f->destination_x = city_data.map.exit_point.x;
            f->destination_y = city_data.map.exit_point.y;
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->is_ghost = 1;
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
        }
        case FIGURE_ACTION_SOLDIER_RETURNING_FROM_DISTANT_BATTLE:
            f->is_ghost = 0;
            f->destination_x = legion_formation->x + formation_layout_position_x(FORMATION_AT_REST, f->index_in_formation);
            f->destination_y = legion_formation->y + formation_layout_position_y(FORMATION_AT_REST, f->index_in_formation);
            f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_SOLDIER_AT_REST;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
    }

    update_image(f, &formations[f->formation_id]);
}
