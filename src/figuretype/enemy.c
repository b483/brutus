#include "enemy.h"

#include "city/data_private.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/formation.h"
#include "figure/formation_enemy.h"
#include "figure/image.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "figuretype/missile.h"
#include "map/figure.h"
#include "scenario/data.h"
#include "sound/effect.h"
#include "sound/speech.h"

static void shoot_enemy_missile(figure *f, struct formation_t *m)
{
    f->wait_ticks_missile++;
    map_point tile = { 0, 0 };
    if (f->wait_ticks_missile > f->missile_delay) {
        f->wait_ticks_missile = 0;
        if (set_missile_target(f, &tile, 1) || set_missile_target(f, &tile, 0)) {
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
            m->missile_fired = 6;
        }
        if (f->missile_type == FIGURE_ARROW && f->attack_image_offset < 5) {
            sound_effect_play(SOUND_EFFECT_ARROW);
        }
        f->attack_image_offset++;
        if (f->attack_image_offset > 100) {
            f->attack_image_offset = 0;
        }
    }
}

static void enemy_initial(figure *f, struct formation_t *m)
{
    map_figure_update(f);
    f->image_offset = 0;
    figure_route_remove(f);
    f->wait_ticks--;
    if (f->wait_ticks <= 0) {
        if (f->is_ghost && f->index_in_formation == 0) {
            if (m->layout == FORMATION_ENEMY_MOB) {
                sound_speech_play_file("wavs/drums.wav");
            } else {
                sound_speech_play_file("wavs/horn1.wav");
            }
        }
        f->is_ghost = 0;
        if (m->recent_fight) {
            f->action_state = FIGURE_ACTION_ENEMY_ENGAGED;
        } else {
            f->destination_x = m->destination_x + formation_layout_position_x(m->layout, f->index_in_formation);
            f->destination_y = m->destination_y + formation_layout_position_y(m->layout, f->index_in_formation);
            if (calc_general_direction(f->x, f->y, f->destination_x, f->destination_y) < 8) {
                f->action_state = FIGURE_ACTION_ENEMY_MARCHING;
            }
        }
    }
    if (f->max_range) {
        // ranged enemies
        shoot_enemy_missile(f, m);
    }
}

static void enemy_marching(figure *f, const struct formation_t *m)
{
    f->wait_ticks--;
    if (f->wait_ticks <= 0) {
        f->wait_ticks = 50;
        f->destination_x = m->destination_x + formation_layout_position_x(m->layout, f->index_in_formation);
        f->destination_y = m->destination_y + formation_layout_position_y(m->layout, f->index_in_formation);
        if (calc_general_direction(f->x, f->y, f->destination_x, f->destination_y) == DIR_FIGURE_AT_DESTINATION) {
            f->action_state = FIGURE_ACTION_ENEMY_INITIAL;
            return;
        }
        f->destination_building_id = m->destination_building_id;
        figure_route_remove(f);
    }
    figure_movement_move_ticks(f, f->speed_multiplier);
    if (f->direction == DIR_FIGURE_AT_DESTINATION ||
        f->direction == DIR_FIGURE_REROUTE ||
        f->direction == DIR_FIGURE_LOST) {
        f->action_state = FIGURE_ACTION_ENEMY_INITIAL;
    }
}

static void engage_enemy(figure *f, struct formation_t *m)
{
    if (!m->recent_fight) {
        f->action_state = FIGURE_ACTION_ENEMY_INITIAL;
    }
    if (f->max_range) {
        shoot_enemy_missile(f, m);
    } else {
        figure *target = melee_unit__set_closest_target(f);
        if (target) {
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                f->action_state = FIGURE_ACTION_ENEMY_INITIAL;
                f->target_figure_id = 0;
            }
        } else {
            f->action_state = FIGURE_ACTION_ENEMY_INITIAL;
            f->wait_ticks = 50;
        }
    }
}

static void enemy_action(figure *f, struct formation_t *m)
{
    city_data.figure.enemies++;
    f->terrain_usage = TERRAIN_USAGE_ENEMY;

    switch (f->action_state) {
        case FIGURE_ACTION_FLEEING:
            f->destination_x = f->source_x;
            f->destination_y = f->source_y;
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                f->direction == DIR_FIGURE_REROUTE ||
                f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
        case FIGURE_ACTION_ENEMY_INITIAL:
            enemy_initial(f, m);
            break;
        case FIGURE_ACTION_ENEMY_MARCHING:
            enemy_marching(f, m);
            break;
        case FIGURE_ACTION_ENEMY_ENGAGED:
            engage_enemy(f, m);
            break;
    }
}

static int get_direction(figure *f)
{
    int dir;
    if (f->action_state == FIGURE_ACTION_ATTACK) {
        dir = f->attack_direction;
    } else if (f->direction < 8) {
        dir = f->direction;
    } else {
        dir = f->previous_tile_direction;
    }
    return figure_image_normalize_direction(dir);
}

static int get_missile_direction(figure *f, const struct formation_t *m)
{
    int dir;
    if (f->action_state == FIGURE_ACTION_ATTACK) {
        dir = f->attack_direction;
    } else if (m->missile_fired || f->direction < 8) {
        dir = f->direction;
    } else {
        dir = f->previous_tile_direction;
    }
    return figure_image_normalize_direction(dir);
}

void figure_enemy_ranged_spear_1_action(figure *f)
{
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_missile_direction(f, &formations[f->formation_id]);

    switch (formations[f->formation_id].enemy_img_group) {
        case ENEMY_TYPE_PERGAMUM:
        case ENEMY_TYPE_PHOENICIAN:
        case ENEMY_TYPE_ETRUSCAN:
        case ENEMY_TYPE_GREEK:
            break;
        default:
            return;
    }
    if (f->action_state == FIGURE_ACTION_ATTACK) {
        if (f->attack_image_offset >= 12) {
            f->image_id = 745 + dir + 8 * ((f->attack_image_offset - 12) / 2);
        } else {
            f->image_id = 745 + dir;
        }
    } else if (f->action_state == FIGURE_ACTION_ENEMY_INITIAL) {
        f->image_id = 697 + dir + 8 * figure_image_missile_launcher_offset(f);
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = 793 + figure_image_corpse_offset(f);
    } else if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 745 + dir + 8 * (f->image_offset / 2);
    } else {
        f->image_id = 601 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_sword_1_action(figure *f)
{
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_direction(f);

    switch (formations[f->formation_id].enemy_img_group) {
        case ENEMY_TYPE_PERGAMUM:
        case ENEMY_TYPE_PHOENICIAN:
        case ENEMY_TYPE_EGYPTIAN:
            break;
        default:
            return;
    }
    if (f->action_state == FIGURE_ACTION_ATTACK) {
        if (f->attack_image_offset >= 12) {
            f->image_id = 545 + dir + 8 * ((f->attack_image_offset - 12) / 2);
        } else {
            f->image_id = 545 + dir;
        }
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = 593 + figure_image_corpse_offset(f);
    } else if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 545 + dir + 8 * (f->image_offset / 2);
    } else {
        f->image_id = 449 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_sword_2_action(figure *f)
{
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_direction(f);

    switch (formations[f->formation_id].enemy_img_group) {
        case ENEMY_TYPE_ETRUSCAN:
        case ENEMY_TYPE_GREEK:
        case ENEMY_TYPE_CARTHAGINIAN:
            break;
        default:
            return;
    }
    if (f->action_state == FIGURE_ACTION_ATTACK) {
        if (f->attack_image_offset >= 12) {
            f->image_id = 545 + dir + 8 * ((f->attack_image_offset - 12) / 2);
        } else {
            f->image_id = 545 + dir;
        }
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = 593 + figure_image_corpse_offset(f);
    } else if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 545 + dir + 8 * (f->image_offset / 2);
    } else {
        f->image_id = 449 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_camel_action(figure *f)
{
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_missile_direction(f, &formations[f->formation_id]);

    if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 601 + dir + 8 * f->image_offset;
    } else if (f->action_state == FIGURE_ACTION_ATTACK) {
        f->image_id = 601 + dir;
    } else if (f->action_state == FIGURE_ACTION_ENEMY_INITIAL) {
        f->image_id = 697 + dir + 8 * figure_image_missile_launcher_offset(f);
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = 745 + figure_image_corpse_offset(f);
    } else {
        f->image_id = 601 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_elephant_action(figure *f)
{
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_direction(f);

    if (f->direction == DIR_FIGURE_ATTACK || f->action_state == FIGURE_ACTION_ATTACK) {
        f->image_id = 601 + dir + 8 * f->image_offset;
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = 705 + figure_image_corpse_offset(f);
    } else {
        f->image_id = 601 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_chariot_action(figure *f)
{
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_direction(f);

    if (f->direction == DIR_FIGURE_ATTACK || f->action_state == FIGURE_ACTION_ATTACK) {
        f->image_id = 697 + dir + 8 * (f->image_offset / 2);
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = 745 + figure_image_corpse_offset(f);
    } else {
        f->image_id = 601 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_fast_sword_action(figure *f)
{
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_direction(f);

    int attack_id, corpse_id, normal_id;
    if (formations[f->formation_id].enemy_img_group == ENEMY_TYPE_BARBARIAN) {
        attack_id = 393;
        corpse_id = 441;
        normal_id = 297;
    } else if (formations[f->formation_id].enemy_img_group == ENEMY_TYPE_NUMIDIAN) {
        attack_id = 593;
        corpse_id = 641;
        normal_id = 449;
    } else if (formations[f->formation_id].enemy_img_group == ENEMY_TYPE_GOTH) {
        attack_id = 545;
        corpse_id = 593;
        normal_id = 449;
    } else {
        return;
    }
    if (f->action_state == FIGURE_ACTION_ATTACK) {
        if (f->attack_image_offset >= 12) {
            f->image_id = attack_id + dir + 8 * ((f->attack_image_offset - 12) / 2);
        } else {
            f->image_id = attack_id + dir;
        }
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = corpse_id + figure_image_corpse_offset(f);
    } else if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = attack_id + dir + 8 * (f->image_offset / 2);
    } else {
        f->image_id = normal_id + dir + 8 * f->image_offset;
    }
}

void figure_enemy_sword_3_action(figure *f)
{
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_direction(f);

    if (formations[f->formation_id].enemy_img_group != ENEMY_TYPE_GAUL && formations[f->formation_id].enemy_img_group != ENEMY_TYPE_CELT) {
        return;
    }
    if (f->action_state == FIGURE_ACTION_ATTACK) {
        if (f->attack_image_offset >= 12) {
            f->image_id = 545 + dir + 8 * ((f->attack_image_offset - 12) / 2);
        } else {
            f->image_id = 545 + dir;
        }
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = 593 + figure_image_corpse_offset(f);
    } else if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 545 + dir + 8 * (f->image_offset / 2);
    } else {
        f->image_id = 449 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_ranged_spear_2_action(figure *f)
{
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_missile_direction(f, &formations[f->formation_id]);

    if (formations[f->formation_id].enemy_img_group != ENEMY_TYPE_NUMIDIAN) {
        return;
    }
    if (f->action_state == FIGURE_ACTION_ATTACK) {
        if (f->attack_image_offset >= 12) {
            f->image_id = 593 + dir + 8 * ((f->attack_image_offset - 12) / 2);
        } else {
            f->image_id = 593 + dir;
        }
    } else if (f->action_state == FIGURE_ACTION_ENEMY_INITIAL) {
        f->image_id = 545 + dir + 8 * figure_image_missile_launcher_offset(f);
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = 641 + figure_image_corpse_offset(f);
    } else if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 593 + dir + 8 * (f->image_offset / 2);
    } else {
        f->image_id = 449 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_mounted_archer_action(figure *f)
{
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_missile_direction(f, &formations[f->formation_id]);

    if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 601 + dir + 8 * f->image_offset;
    } else if (f->action_state == FIGURE_ACTION_ATTACK) {
        f->image_id = 601 + dir;
    } else if (f->action_state == FIGURE_ACTION_ENEMY_INITIAL) {
        f->image_id = 697 + dir + 8 * figure_image_missile_launcher_offset(f);
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = 745 + figure_image_corpse_offset(f);
    } else {
        f->image_id = 601 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_axe_action(figure *f)
{
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_direction(f);

    if (formations[f->formation_id].enemy_img_group != ENEMY_TYPE_GAUL) {
        return;
    }
    if (f->action_state == FIGURE_ACTION_ATTACK) {
        if (f->attack_image_offset >= 12) {
            f->image_id = 697 + dir + 8 * ((f->attack_image_offset - 12) / 2);
        } else {
            f->image_id = 697 + dir;
        }
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = 745 + figure_image_corpse_offset(f);
    } else if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 697 + dir + 8 * (f->image_offset / 2);
    } else {
        f->image_id = 601 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_gladiator_action(figure *f)
{
    f->terrain_usage = TERRAIN_USAGE_ANY;
    f->use_cross_country = 0;
    figure_image_increase_offset(f, 12);
    if (scenario.gladiator_revolt.state == EVENT_FINISHED) {
        // end of gladiator revolt: kill gladiators
        if (f->action_state != FIGURE_ACTION_CORPSE) {
            f->action_state = FIGURE_ACTION_CORPSE;
            f->wait_ticks = 0;
            f->direction = 0;
            clear_targeting_on_unit_death(f);
            refresh_formation_figure_indexes(f);
        }
    }
    switch (f->action_state) {
        case FIGURE_ACTION_ATTACK:
            figure_image_increase_offset(f, 16);
            break;
        case FIGURE_ACTION_NATIVE_CREATED:
            f->image_offset = 0;
            f->wait_ticks++;
            if (f->wait_ticks > 10 + (f->id & 3)) {
                f->wait_ticks = 0;
                f->action_state = FIGURE_ACTION_NATIVE_ATTACKING;
                int x_tile, y_tile;
                int building_id = formation_rioter_get_target_building(&x_tile, &y_tile);
                if (building_id) {
                    f->destination_x = x_tile;
                    f->destination_y = y_tile;
                    f->destination_building_id = building_id;
                    figure_route_remove(f);
                } else {
                    f->state = FIGURE_STATE_DEAD;
                }
            }
            break;
        case FIGURE_ACTION_NATIVE_ATTACKING:
            city_data.figure.attacking_natives = 10;
            f->terrain_usage = TERRAIN_USAGE_ENEMY;
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                f->direction == DIR_FIGURE_REROUTE ||
                f->direction == DIR_FIGURE_LOST) {
                f->action_state = FIGURE_ACTION_NATIVE_CREATED;
            }
            break;
    }
    int dir;
    if (f->action_state == FIGURE_ACTION_ATTACK || f->direction == DIR_FIGURE_ATTACK) {
        dir = f->attack_direction;
    } else if (f->direction < 8) {
        dir = f->direction;
    } else {
        dir = f->previous_tile_direction;
    }
    dir = figure_image_normalize_direction(dir);

    if (f->action_state == FIGURE_ACTION_ATTACK || f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = image_group(GROUP_FIGURE_GLADIATOR) + dir + 104 + 8 * (f->image_offset / 2);
    } else if (f->action_state == FIGURE_ACTION_CORPSE) {
        f->image_id = image_group(GROUP_FIGURE_GLADIATOR) + 96 + figure_image_corpse_offset(f);
    } else {
        f->image_id = image_group(GROUP_FIGURE_GLADIATOR) + dir + 8 * f->image_offset;
    }
}

void figure_enemy_caesar_legionary_action(figure *f)
{
    city_data.figure.imperial_soldiers++;
    figure_image_increase_offset(f, 12);
    f->cart_image_id = 0;
    enemy_action(f, &formations[f->formation_id]);

    int dir = get_direction(f);
    int img_group_base_id = image_group(GROUP_FIGURE_CAESAR_LEGIONARY);

    if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = img_group_base_id + dir + 8 * ((f->attack_image_offset - 12) / 2);
    }
    switch (f->action_state) {
        case FIGURE_ACTION_ATTACK:
            if (f->attack_image_offset >= 12) {
                f->image_id = img_group_base_id + dir +
                    8 * ((f->attack_image_offset - 12) / 2);
            } else {
                f->image_id = img_group_base_id + dir;
            }
            break;
        default:
            if (f->figure_is_halted && formations[f->formation_id].missile_attack_timeout) {
                f->image_id = img_group_base_id + 144 + dir + 8 * f->image_offset;
            } else {
                f->image_id = img_group_base_id + 48 + dir + 8 * f->image_offset;
            }
            break;
    }
}
