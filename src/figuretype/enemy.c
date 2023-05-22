#include "enemy.h"

#include "building/building.h"
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
#include "map/terrain.h"
#include "scenario/data.h"
#include "sound/effect.h"
#include "sound/speech.h"

static void shoot_enemy_missile(struct figure_t *f, map_point *tile)
{
    f->is_shooting = 1;
    f->attack_image_offset = 1;
    figure_create_missile(f, tile, figure_properties[f->type].missile_type);
    if (figure_properties[f->type].missile_type == FIGURE_ARROW) {
        sound_effect_play(SOUND_EFFECT_ARROW);
    }
    f->wait_ticks_missile = 0;
    // clear targeting
    figure__remove_ranged_targeter_from_list(&figures[f->target_figure_id], f);
    f->target_figure_id = 0;
}

static void rout_enemy_unit(struct figure_t *f)
{
    f->destination_x = f->source_x;
    f->destination_y = f->source_y;
    figure_movement_move_ticks(f, f->speed_multiplier);
    if (f->direction == DIR_FIGURE_AT_DESTINATION ||
        f->direction == DIR_FIGURE_REROUTE ||
        f->direction == DIR_FIGURE_LOST) {
        f->state = FIGURE_STATE_DEAD;
    }
}

static void spawn_enemy(struct figure_t *f, struct formation_t *m)
{
    if (f->wait_ticks) {
        f->wait_ticks--;
        if (!f->wait_ticks) {
            if (f->index_in_formation % 4 < 1) {
                if (m->layout == FORMATION_ENEMY_MOB) {
                    sound_speech_play_file("wavs/drums.wav");
                } else {
                    sound_speech_play_file("wavs/horn1.wav");
                }
            }
            f->is_ghost = 0;
            f->action_state = FIGURE_ACTION_ENEMY_ADVANCING;
        }
    }
}

static void melee_enemy_action(struct figure_t *f)
{
    struct formation_t *m = &enemy_formations[f->formation_id];
    switch (f->action_state) {
        case FIGURE_ACTION_FLEEING:
            rout_enemy_unit(f);
            break;
        case FIGURE_ACTION_ENEMY_SPAWNING:
            spawn_enemy(f, m);
            break;
        case FIGURE_ACTION_ENEMY_REGROUPING:
            map_figure_update(f);
            f->image_offset = 0;
            break;
        case FIGURE_ACTION_ENEMY_ADVANCING:
            f->destination_x = m->destination_x + formation_layout_position_x(m->layout, f->index_in_formation);
            f->destination_y = m->destination_y + formation_layout_position_y(m->layout, f->index_in_formation);
            figure_movement_move_ticks(f, f->speed_multiplier);
            if (f->direction == DIR_FIGURE_AT_DESTINATION
                || f->direction == DIR_FIGURE_REROUTE
                || f->direction == DIR_FIGURE_LOST) {
                figure_route_remove(f);
                f->action_state = FIGURE_ACTION_ENEMY_REGROUPING;
            }
            break;
        case FIGURE_ACTION_ENEMY_ENGAGED:
            struct figure_t *target_unit = melee_unit__set_closest_target(f);
            if (target_unit) {
                f->destination_x = target_unit->x;
                f->destination_y = target_unit->y;
                figure_movement_move_ticks(f, f->speed_multiplier);
            } else {
                figure_movement_move_ticks(f, f->speed_multiplier);
            }
            if (f->direction == DIR_FIGURE_AT_DESTINATION
                || f->direction == DIR_FIGURE_REROUTE
                || f->direction == DIR_FIGURE_LOST) {
                figure_route_remove(f);
                f->action_state = FIGURE_ACTION_ENEMY_REGROUPING;
            }
            break;
    }
}

static void ranged_enemy_action(struct figure_t *f)
{
    struct formation_t *m = &enemy_formations[f->formation_id];
    map_point tile = { -1, -1 };
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
        case FIGURE_ACTION_FLEEING:
            rout_enemy_unit(f);
            break;
        case FIGURE_ACTION_ENEMY_SPAWNING:
            spawn_enemy(f, m);
            break;
        case FIGURE_ACTION_ENEMY_REGROUPING:
            map_figure_update(f);
            f->image_offset = 0;
            if (f->wait_ticks_missile > figure_properties[f->type].missile_delay && set_missile_target(f, &tile)) {
                f->direction = calc_missile_shooter_direction(f->x, f->y, tile.x, tile.y);
                shoot_enemy_missile(f, &tile);
            }
            break;
        case FIGURE_ACTION_ENEMY_ADVANCING:
            f->destination_x = m->destination_x + formation_layout_position_x(m->layout, f->index_in_formation);
            f->destination_y = m->destination_y + formation_layout_position_y(m->layout, f->index_in_formation);
            figure_movement_move_ticks(f, f->speed_multiplier);
            if ((f->type == FIGURE_ENEMY_HUN_MOUNTED_ARCHER || f->type == FIGURE_ENEMY_GOTH_MOUNTED_ARCHER || f->type == FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER)
            && f->wait_ticks_missile > figure_properties[f->type].missile_delay
            && set_missile_target(f, &tile)) {
                shoot_enemy_missile(f, &tile);
            }
            if (f->direction == DIR_FIGURE_AT_DESTINATION
                || f->direction == DIR_FIGURE_REROUTE
                || f->direction == DIR_FIGURE_LOST) {
                figure_route_remove(f);
                f->action_state = FIGURE_ACTION_ENEMY_REGROUPING;
            }
            break;
        case FIGURE_ACTION_ENEMY_ENGAGED:
            if (f->target_figure_id && calc_maximum_distance(f->x, f->y, f->destination_x, f->destination_y) < figure_properties[f->type].max_range) {
                figure_route_remove(f);
                f->destination_x = f->x;
                f->destination_y = f->y;
                f->image_offset = 0;
                if (f->wait_ticks_missile > figure_properties[f->type].missile_delay && set_missile_target(f, &tile)) {
                    f->direction = calc_missile_shooter_direction(f->x, f->y, tile.x, tile.y);
                    shoot_enemy_missile(f, &tile);
                    break;
                }
            } else {
                figure_movement_move_ticks(f, f->speed_multiplier);
                if (f->direction == DIR_FIGURE_AT_DESTINATION
                    || f->direction == DIR_FIGURE_REROUTE
                    || f->direction == DIR_FIGURE_LOST) {
                    figure_route_remove(f);
                    f->action_state = FIGURE_ACTION_ENEMY_REGROUPING;
                }
            }
            break;
    }
}

int get_direction(struct figure_t *f)
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

void figure_enemy_heavy_ranged_spearman_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    ranged_enemy_action(f);
    int dir = get_direction(f);

    if (f->action_state == FIGURE_ACTION_ENEMY_REGROUPING) {
        f->image_id = 697 + dir + 8 * figure_image_missile_launcher_offset(f);
    } else if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 745 + dir + 8 * (f->image_offset / 2);
    } else {
        f->image_id = 601 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_camel_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    ranged_enemy_action(f);
    int dir = get_direction(f);

    if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = 601 + dir + 8 * f->image_offset;
    } else if (f->action_state == FIGURE_ACTION_ENEMY_REGROUPING) {
        f->image_id = 697 + dir + 8 * figure_image_missile_launcher_offset(f);
    } else {
        f->image_id = 601 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_elephant_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    ranged_enemy_action(f);

    f->image_id = 601 + get_direction(f) + 8 * f->image_offset;
}

void figure_enemy_chariot_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    melee_enemy_action(f);
    int dir = get_direction(f);

    f->image_id = 601 + dir + 8 * f->image_offset;
}

void figure_enemy_fast_swordsman_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    melee_enemy_action(f);
    int dir = get_direction(f);

    int image_id;
    if (f->enemy_image_group == ENEMY_IMG_TYPE_BARBARIAN) {
        image_id = 297;
    } else if (f->enemy_image_group == ENEMY_IMG_TYPE_NORTH_AFRICAN) {
        image_id = 449;
    } else if (f->enemy_image_group == ENEMY_IMG_TYPE_GOTH) {
        image_id = 449;
    } else {
        return;
    }
    f->image_id = image_id + dir + 8 * f->image_offset;
}

void figure_enemy_swordsman_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    melee_enemy_action(f);
    int dir = get_direction(f);

    f->image_id = 449 + dir + 8 * f->image_offset;
}

void figure_enemy_light_ranged_spearman_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    ranged_enemy_action(f);
    int dir = get_direction(f);

    if (f->action_state == FIGURE_ACTION_ENEMY_REGROUPING) {
        f->image_id = 545 + dir + 8 * figure_image_missile_launcher_offset(f);
    } else {
        f->image_id = 449 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_mounted_archer_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    ranged_enemy_action(f);
    int dir = get_direction(f);

    if (f->action_state == FIGURE_ACTION_ENEMY_REGROUPING) {
        f->image_id = 697 + dir + 8 * figure_image_missile_launcher_offset(f);
    } else {
        f->image_id = 601 + dir + 8 * f->image_offset;
    }
}

void figure_enemy_axeman_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    melee_enemy_action(f);
    int dir = get_direction(f);

    f->image_id = 601 + dir + 8 * f->image_offset;
}

void figure_enemy_gladiator_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    if (scenario.gladiator_revolt.state == EVENT_FINISHED) {
        // end of gladiator revolt: kill gladiators
        f->action_state = FIGURE_ACTION_CORPSE;
        f->is_targetable = 0;
        f->wait_ticks = 0;
        f->direction = 0;
        clear_targeting_on_unit_death(f);
    }
    switch (f->action_state) {
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
            f->terrain_usage = TERRAIN_USAGE_ENEMY;
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                f->direction == DIR_FIGURE_REROUTE ||
                f->direction == DIR_FIGURE_LOST) {
                f->action_state = FIGURE_ACTION_NATIVE_CREATED;
            }
            break;
    }
    int dir = get_direction(f);
    f->image_id = image_group(GROUP_FIGURE_GLADIATOR) + dir + 8 * f->image_offset;
}

void figure_enemy_caesar_legionary_action(struct figure_t *f)
{
    figure_image_increase_offset(f, 12);
    melee_enemy_action(f);
    int dir = get_direction(f);
    int img_group_base_id = image_group(GROUP_FIGURE_CAESAR_LEGIONARY);

    if (f->direction == DIR_FIGURE_ATTACK) {
        f->image_id = img_group_base_id + dir + 8 * ((f->attack_image_offset - 12) / 2);
    }
    if (f->figure_is_halted && enemy_formations[f->formation_id].missile_attack_timeout) {
        f->image_id = img_group_base_id + 144 + dir + 8 * f->image_offset;
    } else {
        f->image_id = img_group_base_id + 48 + dir + 8 * f->image_offset;
    }
}

static void set_target_building_for_native(struct figure_t *f)
{
    struct building_t *min_building = 0;
    int min_distance = 10000;
    for (int i = 1; i < MAX_BUILDINGS; i++) {
        struct building_t *b = &all_buildings[i];
        if (b->state != BUILDING_STATE_IN_USE) {
            continue;
        }
        switch (b->type) {
            case BUILDING_MISSION_POST:
            case BUILDING_NATIVE_HUT:
            case BUILDING_NATIVE_CROPS:
            case BUILDING_NATIVE_MEETING:
            case BUILDING_WAREHOUSE:
            case BUILDING_FORT:
                break;
            default:
            {
                int distance = calc_maximum_distance(city_data.building.main_native_meeting.x, city_data.building.main_native_meeting.y, b->x, b->y);
                if (distance < min_distance) {
                    min_building = b;
                    min_distance = distance;
                }
            }
        }
    }
    if (min_building) {
        f->destination_x = min_building->x;
        f->destination_y = min_building->y;
    }
}

void figure_indigenous_native_action(struct figure_t *f)
{
    struct building_t *b = &all_buildings[f->building_id];
    if (b->state != BUILDING_STATE_IN_USE || b->figure_id != f->id) {
        f->state = FIGURE_STATE_DEAD;
    }
    figure_image_increase_offset(f, 12);
    switch (f->action_state) {
        case FIGURE_ACTION_NATIVE_GOING_TO_MEETING_CENTER:
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION) {
                f->action_state = FIGURE_ACTION_NATIVE_RETURNING_FROM_MEETING;
                f->destination_x = f->source_x;
                f->destination_y = f->source_y;
            } else if (f->direction == DIR_FIGURE_REROUTE || f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
        case FIGURE_ACTION_NATIVE_RETURNING_FROM_MEETING:
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                f->direction == DIR_FIGURE_REROUTE ||
                f->direction == DIR_FIGURE_LOST) {
                f->state = FIGURE_STATE_DEAD;
            }
            break;
        case FIGURE_ACTION_NATIVE_CREATED:
            f->image_offset = 0;
            f->wait_ticks++;
            if (f->wait_ticks > 10 + (f->id & 3)) {
                f->wait_ticks = 0;
                if (city_data.military.native_attack_duration) {
                    f->action_state = FIGURE_ACTION_NATIVE_ATTACKING;
                    set_target_building_for_native(f);
                } else {
                    int x_tile, y_tile;
                    struct building_t *meeting = &all_buildings[b->subtype.native_meeting_center_id];
                    if (map_terrain_get_adjacent_road_or_clear_land(
                        meeting->x, meeting->y, meeting->size, &x_tile, &y_tile)) {
                        f->action_state = FIGURE_ACTION_NATIVE_GOING_TO_MEETING_CENTER;
                        f->destination_x = x_tile;
                        f->destination_y = y_tile;
                    }
                }
                figure_route_remove(f);
            }
            break;
        case FIGURE_ACTION_NATIVE_ATTACKING:
            f->terrain_usage = TERRAIN_USAGE_ENEMY;
            figure_movement_move_ticks(f, 1);
            if (f->direction == DIR_FIGURE_AT_DESTINATION ||
                f->direction == DIR_FIGURE_REROUTE ||
                f->direction == DIR_FIGURE_LOST) {
                f->action_state = FIGURE_ACTION_NATIVE_CREATED;
            }
            break;
    }
    int dir = get_direction(f);

    if (f->action_state == FIGURE_ACTION_NATIVE_ATTACKING) {
        f->image_id = 297 + dir + 8 * f->image_offset;
    } else {
        f->image_id = 201 + dir + 8 * f->image_offset;
    }
}