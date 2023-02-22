#include "soldier.h"

#include "city/data_private.h"
#include "city/map.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/formation.h"
#include "figure/formation_layout.h"
#include "figure/image.h"
#include "figure/movement.h"
#include "figure/properties.h"
#include "figure/route.h"
#include "figuretype/missile.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/point.h"

static const map_point ALTERNATIVE_POINTS[] = { {-1, -6},
    {0, -1}, {1, -1}, {1, 0}, {1, 1}, {0, 1}, {-1, 1}, {-1, 0}, {-1, -1},
    {0, -2}, {1, -2}, {2, -2}, {2, -1}, {2, 0}, {2, 1}, {2, 2}, {1, 2},
    {0, 2}, {-1, 2}, {-2, 2}, {-2, 1}, {-2, 0}, {-2, -1}, {-2, -2}, {-1, -2},
    {0, -3}, {1, -3}, {2, -3}, {3, -3}, {3, -2}, {3, -1}, {3, 0}, {3, 1},
    {3, 2}, {3, 3}, {2, 3}, {1, 3}, {0, 3}, {-1, 3}, {-2, 3}, {-3, 3},
    {-3, 2}, {-3, 1}, {-3, 0}, {-3, -1}, {-3, -2}, {-3, -3}, {-2, -3}, {-1, -3},
    {0, -4}, {1, -4}, {2, -4}, {3, -4}, {4, -4}, {4, -3}, {4, -2}, {4, -1},
    {4, 0}, {4, 1}, {4, 2}, {4, 3}, {4, 4}, {3, 4}, {2, 4}, {1, 4},
    {0, 4}, {-1, 4}, {-2, 4}, {-3, 4}, {-4, 4}, {-4, 3}, {-4, 2}, {-4, 1},
    {-4, 0}, {-4, -1}, {-4, -2}, {-4, -3}, {-4, -4}, {-3, -4}, {-2, -4}, {-1, -4},
    {0, -5}, {1, -5}, {2, -5}, {3, -5}, {4, -5}, {5, -5}, {5, -4}, {5, -3},
    {5, -2}, {5, -1}, {5, 0}, {5, 1}, {5, 2}, {5, 3}, {5, 4}, {5, 5},
    {4, 5}, {3, 5}, {2, 5}, {1, 5}, {0, 5}, {-1, 5}, {-2, 5}, {-3, 5},
    {-4, 5}, {-5, 5}, {-5, 4}, {-5, 3}, {-5, 2}, {-5, 1}, {-5, 0}, {-5, -1},
    {-5, -2}, {-5, -3}, {-5, -4}, {-5, -5}, {-4, -5}, {-3, -5}, {-2, -5}, {-1, -5},
    {0, -6}, {1, -6}, {2, -6}, {3, -6}, {4, -6}, {5, -6}, {6, -6}, {6, -5},
    {6, -4}, {6, -3}, {6, -2}, {6, -1}, {6, 0}, {6, 1}, {6, 2}, {6, 3},
    {6, 4}, {6, 5}, {6, 6}, {5, 6}, {4, 6}, {3, 6}, {2, 6}, {1, 6},
    {0, 6}, {-1, 6}, {-2, 6}, {-3, 6}, {-4, 6}, {-5, 6}, {-6, 6}, {-6, 5},
    {-6, 4}, {-6, 3}, {-6, 2}, {-6, 1}, {-6, 0}, {-6, -1}, {-6, -2}, {-6, -3},
    {-6, -4}, {-6, -5}, {-6, -6}, {-5, -6}, {-4, -6}, {-3, -6}, {-2, -6}, {-1, -6},
};

void figure_military_standard_action(figure *f)
{
    f->terrain_usage = TERRAIN_USAGE_ANY;
    figure_image_increase_offset(f, 16);
    map_figure_delete(f);
    if (formations[f->formation_id].is_at_fort) {
        f->x = formations[f->formation_id].x;
        f->y = formations[f->formation_id].y;
    } else {
        f->x = formations[f->formation_id].standard_x;
        f->y = formations[f->formation_id].standard_y;
    }
    f->grid_offset = map_grid_offset(f->x, f->y);
    f->cross_country_x = 15 * f->x + 7;
    f->cross_country_y = 15 * f->y + 7;
    map_figure_add(f);

    f->image_id = image_group(GROUP_FIGURE_FORT_STANDARD_POLE) + 20 - formations[f->formation_id].morale / 5;
    if (formations[f->formation_id].figure_type == FIGURE_FORT_LEGIONARY) {
        if (formations[f->formation_id].is_halted) {
            f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + 8;
        } else {
            f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + f->image_offset / 2;
        }
    } else if (formations[f->formation_id].figure_type == FIGURE_FORT_MOUNTED) {
        if (formations[f->formation_id].is_halted) {
            f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + 26;
        } else {
            f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + 18 + f->image_offset / 2;
        }
    } else {
        if (formations[f->formation_id].is_halted) {
            f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + 17;
        } else {
            f->cart_image_id = image_group(GROUP_FIGURE_FORT_FLAGS) + 9 + f->image_offset / 2;
        }
    }
}

static void javelin_launch_missile(figure *f)
{
    map_point tile = { -1, -1 };
    f->wait_ticks_missile++;
    if (f->wait_ticks_missile > figure_properties_for_type(f->type)->missile_delay) {
        f->wait_ticks_missile = 0;
        if (get_missile_target(f, 10, &tile)) {
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
            figure_create_missile(f->id, f->x, f->y, tile.x, tile.y, FIGURE_JAVELIN);
            formations[f->formation_id].missile_fired = 6;
        }
        f->attack_image_offset++;
        if (f->attack_image_offset > 100) {
            f->attack_image_offset = 0;
        }
    }
}

static void mop_up_enemies(figure *f)
{
    figure *target = set_closest_eligible_target(f);
    if (target) {
        figure_movement_move_ticks(f, f->speed_multiplier);
        if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
            f->action_state = FIGURE_ACTION_84_SOLDIER_AT_STANDARD;
            f->target_figure_id = 0;
        }
    } else {
        f->action_state = FIGURE_ACTION_84_SOLDIER_AT_STANDARD;
        f->image_offset = 0;
    }
}

static void update_image(figure *f, const struct formation_t *m)
{
    int dir;
    if (f->action_state == FIGURE_ACTION_150_ATTACK) {
        dir = f->attack_direction;
    } else if (m->missile_fired) {
        dir = f->direction;
    } else if (f->action_state == FIGURE_ACTION_84_SOLDIER_AT_STANDARD) {
        dir = m->direction;
    } else if (f->direction < 8) {
        dir = f->direction;
    } else {
        dir = f->previous_tile_direction;
    }
    dir = figure_image_normalize_direction(dir);
    if (f->type == FIGURE_FORT_JAVELIN) {
        int image_id = image_group(GROUP_BUILDING_FORT_JAVELIN);
        if (f->action_state == FIGURE_ACTION_150_ATTACK) {
            if (f->attack_image_offset < 12) {
                f->image_id = image_id + 96 + dir;
            } else {
                f->image_id = image_id + 96 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            }
        } else if (f->action_state == FIGURE_ACTION_149_CORPSE) {
            f->image_id = image_id + 144 + figure_image_corpse_offset(f);
        } else if (f->action_state == FIGURE_ACTION_84_SOLDIER_AT_STANDARD) {
            f->image_id = image_id + 96 + dir +
                8 * figure_image_missile_launcher_offset(f);
        } else {
            f->image_id = image_id + dir + 8 * f->image_offset;
        }
    } else if (f->type == FIGURE_FORT_MOUNTED) {
        int image_id = image_group(GROUP_FIGURE_FORT_MOUNTED);
        if (f->action_state == FIGURE_ACTION_150_ATTACK) {
            if (f->attack_image_offset < 12) {
                f->image_id = image_id + 96 + dir;
            } else {
                f->image_id = image_id + 96 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            }
        } else if (f->action_state == FIGURE_ACTION_149_CORPSE) {
            f->image_id = image_id + 144 + figure_image_corpse_offset(f);
        } else {
            f->image_id = image_id + dir + 8 * f->image_offset;
        }
    } else if (f->type == FIGURE_FORT_LEGIONARY) {
        int image_id = image_group(GROUP_BUILDING_FORT_LEGIONARY);
        if (f->action_state == FIGURE_ACTION_150_ATTACK) {
            if (f->attack_image_offset < 12) {
                f->image_id = image_id + 96 + dir;
            } else {
                f->image_id = image_id + 96 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            }
        } else if (f->action_state == FIGURE_ACTION_149_CORPSE) {
            f->image_id = image_id + 152 + figure_image_corpse_offset(f);
        } else if (f->action_state == FIGURE_ACTION_84_SOLDIER_AT_STANDARD) {
            if (m->is_halted && m->layout == FORMATION_TORTOISE && m->missile_attack_timeout) {
                f->image_id = image_id + dir + 144;
            } else {
                f->image_id = image_id + dir;
            }
        } else {
            f->image_id = image_id + dir + 8 * f->image_offset;
        }
    }
}

void figure_soldier_action(figure *f)
{
    city_data.figure.soldiers++;
    f->terrain_usage = TERRAIN_USAGE_ANY;
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    if (!formations[f->formation_id].in_use) {
        f->action_state = FIGURE_ACTION_149_CORPSE;
    }
    if (f->type == FIGURE_FORT_MOUNTED) {
        f->speed_multiplier = 3;
    } else if (f->type == FIGURE_FORT_JAVELIN) {
        f->speed_multiplier = 2;
    } else {
        f->speed_multiplier = 1;
    }
    int layout = formations[f->formation_id].layout;
    if (f->formation_at_rest || f->action_state == FIGURE_ACTION_81_SOLDIER_GOING_TO_FORT) {
        layout = FORMATION_AT_REST;
    }
    f->formation_position_x.soldier = formations[f->formation_id].x + formation_layout_position_x(layout, f->index_in_formation);
    f->formation_position_y.soldier = formations[f->formation_id].y + formation_layout_position_y(layout, f->index_in_formation);

    switch (f->action_state) {
        case FIGURE_ACTION_150_ATTACK:
            figure_combat_handle_attack(f);
            break;
        case FIGURE_ACTION_149_CORPSE:
            figure_combat_handle_corpse(f);
            break;
        case FIGURE_ACTION_80_SOLDIER_AT_REST:
            map_figure_update(f);
            f->wait_ticks = 0;
            f->formation_at_rest = 1;
            f->image_offset = 0;
            if (f->x != f->formation_position_x.soldier || f->y != f->formation_position_y.soldier) {
                f->action_state = FIGURE_ACTION_81_SOLDIER_GOING_TO_FORT;
            }
            break;
        case FIGURE_ACTION_81_SOLDIER_GOING_TO_FORT:
        case FIGURE_ACTION_148_FLEEING:
            f->wait_ticks = 0;
            f->formation_at_rest = 1;
            f->destination_x = f->formation_position_x.soldier;
            f->destination_y = f->formation_position_y.soldier;
            f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_80_SOLDIER_AT_REST;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
        case FIGURE_ACTION_82_SOLDIER_RETURNING_TO_BARRACKS:
            f->formation_at_rest = 1;
            f->destination_x = f->source_x;
            f->destination_y = f->source_y;
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION || f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_83_SOLDIER_GOING_TO_STANDARD:
            f->formation_at_rest = 0;
            f->destination_x = formations[f->formation_id].standard_x + formation_layout_position_x(formations[f->formation_id].layout, f->index_in_formation);
            f->destination_y = formations[f->formation_id].standard_y + formation_layout_position_y(formations[f->formation_id].layout, f->index_in_formation);
            if (f->alternative_location_index) {
                f->destination_x += ALTERNATIVE_POINTS[f->alternative_location_index].x;
                f->destination_y += ALTERNATIVE_POINTS[f->alternative_location_index].y;
            }
            f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_84_SOLDIER_AT_STANDARD;
                f->image_offset = 0;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->alternative_location_index++;
                if (f->alternative_location_index > 168) {
                    f->state = FIGURE_STATE_DEAD;
                }
                f->image_offset = 0;
            }
            break;
        case FIGURE_ACTION_84_SOLDIER_AT_STANDARD:
            f->formation_at_rest = 0;
            f->image_offset = 0;
            map_figure_update(f);
            f->destination_x = formations[f->formation_id].standard_x + formation_layout_position_x(formations[f->formation_id].layout, f->index_in_formation);
            f->destination_y = formations[f->formation_id].standard_y + formation_layout_position_y(formations[f->formation_id].layout, f->index_in_formation);
            if (f->alternative_location_index) {
                f->destination_x += ALTERNATIVE_POINTS[f->alternative_location_index].x;
                f->destination_y += ALTERNATIVE_POINTS[f->alternative_location_index].y;
            }
            if (f->x != f->destination_x || f->y != f->destination_y) {
                if (formations[f->formation_id].missile_fired <= 0 && formations[f->formation_id].recent_fight <= 0 && formations[f->formation_id].missile_attack_timeout <= 0) {
                    f->action_state = FIGURE_ACTION_83_SOLDIER_GOING_TO_STANDARD;
                    f->alternative_location_index = 0;
                }
            }
            if (f->action_state != FIGURE_ACTION_83_SOLDIER_GOING_TO_STANDARD) {
                if (f->type == FIGURE_FORT_JAVELIN) {
                    javelin_launch_missile(f);
                } else if (f->type == FIGURE_FORT_LEGIONARY) {
                    // attack adjacent enemy
                    for (int i = 0; i < 8 && f->action_state != FIGURE_ACTION_150_ATTACK; i++) {
                        figure_combat_attack_figure_at(f, f->grid_offset + map_grid_direction_delta(i));
                    }
                }
            }
            break;
        case FIGURE_ACTION_85_SOLDIER_GOING_TO_MILITARY_ACADEMY:
            formations[f->formation_id].has_military_training = 1;
            f->formation_at_rest = 1;
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_81_SOLDIER_GOING_TO_FORT;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
        case FIGURE_ACTION_86_SOLDIER_MOPPING_UP:
            f->formation_at_rest = 0;
            mop_up_enemies(f);
            break;
        case FIGURE_ACTION_87_SOLDIER_GOING_TO_DISTANT_BATTLE:
        {
            f->formation_at_rest = 0;
            f->destination_x = city_data.map.exit_point.x;
            f->destination_y = city_data.map.exit_point.y;
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_89_SOLDIER_AT_DISTANT_BATTLE;
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
        }
        case FIGURE_ACTION_88_SOLDIER_RETURNING_FROM_DISTANT_BATTLE:
            f->is_ghost = 0;
            f->wait_ticks = 0;
            f->formation_at_rest = 1;
            f->destination_x = f->formation_position_x.soldier;
            f->destination_y = f->formation_position_y.soldier;
            f->destination_grid_offset = map_grid_offset(f->destination_x, f->destination_y);
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_80_SOLDIER_AT_REST;
            } else if (f->direction == DIR_FIGURE_REROUTE) {
                figure_route_remove(f);
            } else if (f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
        case FIGURE_ACTION_89_SOLDIER_AT_DISTANT_BATTLE:
            f->is_ghost = 1;
            f->formation_at_rest = 1;
            break;
    }

    update_image(f, &formations[f->formation_id]);
}
