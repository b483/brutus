#include "missile.h"

#include "city/view.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/formation.h"
#include "figure/movement.h"
#include "figure/sound.h"
#include "map/figure.h"
#include "map/point.h"
#include "sound/effect.h"

static const int CLOUD_TILE_OFFSETS[] = { 0, 0, 0, 1, 1, 2 };

static const int CLOUD_CC_OFFSETS[] = { 0, 7, 14, 7, 14, 7 };

static const int CLOUD_SPEED[] = {
    1, 2, 1, 3, 2, 1, 3, 2, 1, 1, 2, 1, 2, 1, 3, 1
};

static const map_point CLOUD_DIRECTION[] = {
    {0, -6}, {-2, -5}, {-4, -4}, {-5, -2}, {-6, 0}, {-5, -2}, {-4, -4}, {-2, -5},
    {0, -6}, {-2, -5}, {-4, -4}, {-5, -2}, {-6, 0}, {-5, -2}, {-4, -4}, {-2, -5}
};

static const int CLOUD_IMAGE_OFFSETS[] = {
    0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2,
    2, 2, 2, 2, 3, 3, 3, 4, 4, 5, 6, 7
};

void figure_create_explosion_cloud(int x, int y, int size)
{
    int tile_offset = CLOUD_TILE_OFFSETS[size];
    int cc_offset = CLOUD_CC_OFFSETS[size];
    for (int i = 0; i < 16; i++) {
        figure *f = figure_create(FIGURE_EXPLOSION,
            x + tile_offset, y + tile_offset, DIR_0_TOP);
        if (f->id) {
            f->cross_country_x += cc_offset;
            f->cross_country_y += cc_offset;
            f->destination_x += CLOUD_DIRECTION[i].x;
            f->destination_y += CLOUD_DIRECTION[i].y;
            figure_movement_set_cross_country_direction(f,
                f->cross_country_x, f->cross_country_y,
                15 * f->destination_x + cc_offset,
                15 * f->destination_y + cc_offset, 0);
            f->speed_multiplier = CLOUD_SPEED[i];
        }
    }
}

void figure_create_missile(figure *shooter, map_point *target_tile, int type)
{
    figure *missile = figure_create(type, shooter->x, shooter->y, DIR_0_TOP);
    if (missile->id) {
        missile->missile_offset = type == FIGURE_BOLT ? 60 : 10;
        missile->building_id = shooter->id;
        missile->destination_x = target_tile->x;
        missile->destination_y = target_tile->y;
        figure_movement_set_cross_country_direction(missile, missile->cross_country_x, missile->cross_country_y, 15 * target_tile->x, 15 * target_tile->y, 1);
    }
}

static int get_target_on_tile(figure *projectile)
{
    figure *shooter = figure_get(projectile->building_id);
    if (map_figures.items[projectile->grid_offset] > 0) {
        int figure_id = map_figures.items[projectile->grid_offset];
        while (figure_id) {
            figure *f = figure_get(figure_id);
            if (f->action_state != FIGURE_ACTION_CORPSE && f->is_targetable) {
                if (shooter->is_friendly_armed_unit) {
                    if (f->is_enemy_unit
                    || (f->type == FIGURE_INDIGENOUS_NATIVE && f->action_state == FIGURE_ACTION_NATIVE_ATTACKING)
                    || f->is_herd_animal) {
                        return f->id;
                    }
                } else {
                    if (f->is_unarmed_civilian_unit || f->is_friendly_armed_unit || f->is_caesar_legion_unit || (f->is_native_unit && f->action_state != FIGURE_ACTION_NATIVE_ATTACKING) || f->type == FIGURE_WOLF) {
                        return f->id;
                    }
                }
            }
            figure_id = f->next_figure_id_on_same_tile;
        }
    }
    return 0;
}

void figure_explosion_cloud_action(figure *f)
{
    f->use_cross_country = 1;
    f->progress_on_tile++;
    if (f->progress_on_tile > 44) {
        f->state = FIGURE_STATE_DEAD;
    }
    figure_movement_move_ticks_cross_country(f, f->speed_multiplier);
    if (f->progress_on_tile < 48) {
        f->image_id = image_group(GROUP_FIGURE_EXPLOSION) +
            CLOUD_IMAGE_OFFSETS[f->progress_on_tile / 2];
    } else {
        f->image_id = image_group(GROUP_FIGURE_EXPLOSION) + 7;
    }
}

static void missile_hit_target(figure *projectile, figure *target)
{
    int damage_inflicted = projectile->missile_attack_value - target->missile_defense_value;
    if (damage_inflicted < 0) {
        damage_inflicted = 0;
    }
    if ((target->type == FIGURE_FORT_LEGIONARY || target->type == FIGURE_ENEMY_CAESAR_LEGIONARY)
        && formations[target->formation_id].is_halted
        && formations[target->formation_id].layout == FORMATION_TORTOISE) {
        damage_inflicted = 1;
    }
    int target_damage = damage_inflicted + target->damage;
    if (target_damage <= target->max_damage) {
        target->damage = target_damage;
    } else { // kill target
        target->damage = target->max_damage + 1;
        target->action_state = FIGURE_ACTION_CORPSE;
        target->wait_ticks = 0;
        figure_play_die_sound(target);
        formation_update_morale_after_death(&formations[target->formation_id]);
    }
    projectile->state = FIGURE_STATE_DEAD;
    // for missiles: building_id contains the figure who shot it
    figure *shooter = figure_get(projectile->building_id);
    int missile_formation = shooter->formation_id;
    formations[target->formation_id].missile_attack_timeout = 6;
    formations[target->formation_id].missile_attack_formation_id = missile_formation;
    // clear targeting
    shooter->target_figure_id = 0;
    figure__remove_ranged_targeter_from_list(target, shooter);
}

void figure_arrow_action(figure *projectile)
{
    projectile->use_cross_country = 1;
    projectile->progress_on_tile++;
    if (projectile->progress_on_tile > 120) {
        projectile->state = FIGURE_STATE_DEAD;
    }
    int should_die = figure_movement_move_ticks_cross_country(projectile, 4);
    int target_id = get_target_on_tile(projectile);
    if (target_id) {
        figure *target = figure_get(target_id);
        missile_hit_target(projectile, target);
        sound_effect_play(SOUND_EFFECT_ARROW_HIT);
    } else if (should_die) {
        projectile->state = FIGURE_STATE_DEAD;
    }
    int dir = (16 + projectile->direction - 2 * city_view_orientation()) % 16;
    projectile->image_id = image_group(GROUP_FIGURE_MISSILE) + 16 + dir;
}

void figure_javelin_action(figure *projectile)
{
    projectile->use_cross_country = 1;
    projectile->progress_on_tile++;
    if (projectile->progress_on_tile > 120) {
        projectile->state = FIGURE_STATE_DEAD;
    }
    int should_die = figure_movement_move_ticks_cross_country(projectile, 4);

    int target_id = get_target_on_tile(projectile);

    if (target_id) {
        figure *target = figure_get(target_id);
        missile_hit_target(projectile, target);
        sound_effect_play(SOUND_EFFECT_JAVELIN);
    } else if (should_die) {
        projectile->state = FIGURE_STATE_DEAD;
    }
    int dir = (16 + projectile->direction - 2 * city_view_orientation()) % 16;
    projectile->image_id = image_group(GROUP_FIGURE_MISSILE) + dir;
}

void figure_bolt_action(figure *projectile)
{
    projectile->use_cross_country = 1;
    projectile->progress_on_tile++;
    if (projectile->progress_on_tile > 120) {
        projectile->state = FIGURE_STATE_DEAD;
    }
    int should_die = figure_movement_move_ticks_cross_country(projectile, 4);
    int target_id = get_target_on_tile(projectile);
    if (target_id) {
        figure *target = figure_get(target_id);
        int damage_inflicted = projectile->missile_attack_value - target->missile_defense_value;
        if (damage_inflicted < 0) {
            damage_inflicted = 0;
        }
        int target_damage = damage_inflicted + target->damage;
        if (target_damage <= target->max_damage) {
            target->damage = target_damage;
        } else { // kill target
            target->damage = target->max_damage + 1;
            target->action_state = FIGURE_ACTION_CORPSE;
            target->wait_ticks = 0;
            figure_play_die_sound(target);
            formation_update_morale_after_death(&formations[target->formation_id]);
        }
        sound_effect_play(SOUND_EFFECT_BALLISTA_HIT_PERSON);
        projectile->state = FIGURE_STATE_DEAD;
    } else if (should_die) {
        projectile->state = FIGURE_STATE_DEAD;
        sound_effect_play(SOUND_EFFECT_BALLISTA_HIT_GROUND);
    }
    int dir = (16 + projectile->direction - 2 * city_view_orientation()) % 16;
    projectile->image_id = image_group(GROUP_FIGURE_MISSILE) + 32 + dir;
}
