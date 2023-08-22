#include "missile.h"

#include "city/view.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/formation_enemy.h"
#include "figure/formation_herd.h"
#include "figure/formation_legion.h"
#include "figure/movement.h"
#include "figure/sound.h"
#include "map/map.h"
#include "sound/sound.h"

static const int CLOUD_TILE_OFFSETS[] = { 0, 0, 0, 1, 1, 2 };

static const int CLOUD_CC_OFFSETS[] = { 0, 7, 14, 7, 14, 7 };

static const int CLOUD_SPEED[] = {
    1, 2, 1, 3, 2, 1, 3, 2, 1, 1, 2, 1, 2, 1, 3, 1
};

static const struct map_point_t CLOUD_DIRECTION[] = {
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
        struct figure_t *f = figure_create(FIGURE_EXPLOSION,
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

void figure_create_missile(struct figure_t *shooter, struct map_point_t *target_tile, int type)
{
    struct figure_t *missile = figure_create(type, shooter->x, shooter->y, DIR_0_TOP);
    if (missile->id) {
        missile->missile_offset = type == FIGURE_BOLT ? 60 : 10;
        missile->building_id = shooter->id;
        missile->destination_x = target_tile->x;
        missile->destination_y = target_tile->y;
        figure_movement_set_cross_country_direction(missile, missile->cross_country_x, missile->cross_country_y, 15 * target_tile->x, 15 * target_tile->y, 1);
    }
}

static int get_target_on_tile(struct figure_t *projectile)
{
    struct figure_t *shooter = &figures[projectile->building_id];
    if (map_figures.items[projectile->grid_offset] > 0) {
        int figure_id = map_figures.items[projectile->grid_offset];
        while (figure_id) {
            struct figure_t *target = &figures[figure_id];
            if (figure_is_alive(target) && target->is_targetable) {
                if (figure_properties[shooter->type].is_friendly_armed_unit || figure_properties[shooter->type].is_player_legion_unit) {
                    if (is_valid_target_for_player_unit(target)) {
                        return target->id;
                    }
                } else if (figure_properties[shooter->type].is_enemy_unit) {
                    if (is_valid_target_for_enemy_unit(target)) {
                        return target->id;
                    }
                }
            }
            figure_id = target->next_figure_id_on_same_tile;
        }
    }
    return 0;
}

void figure_explosion_cloud_action(struct figure_t *f)
{
    f->use_cross_country = 1;
    f->progress_on_tile++;
    if (f->progress_on_tile > 44) {
        figure_delete(f);
        return;
    }
    figure_movement_move_ticks_cross_country(f, f->speed_multiplier);
    if (f->progress_on_tile < 48) {
        f->image_id = image_group(GROUP_FIGURE_EXPLOSION) + CLOUD_IMAGE_OFFSETS[f->progress_on_tile / 2];
    } else {
        f->image_id = image_group(GROUP_FIGURE_EXPLOSION) + 7;
    }
}

static void missile_hit_target(struct figure_t *projectile, struct figure_t *target)
{
    struct figure_t *shooter = &figures[projectile->building_id];
    int damage_inflicted = (figure_properties[shooter->type].missile_attack_value + figure_properties[projectile->type].missile_attack_value) - figure_properties[target->type].missile_defense_value;
    if (damage_inflicted < 0) {
        damage_inflicted = 0;
    }
    if (projectile->type != FIGURE_BOLT
        && ((target->type == FIGURE_FORT_LEGIONARY && legion_formations[target->formation_id].layout == FORMATION_TORTOISE)
            || (target->type == FIGURE_ENEMY_CAESAR_LEGIONARY && enemy_formations[target->formation_id].layout == FORMATION_TORTOISE))
            && target->figure_is_halted) {
        damage_inflicted = 1;
    }
    int target_damage = damage_inflicted + target->damage;
    if (target_damage <= figure_properties[target->type].max_damage) {
        target->damage = target_damage;
    } else { // kill target
        target->damage = figure_properties[target->type].max_damage + 1;
        target->is_corpse = 1;
        target->is_targetable = 0;
        target->wait_ticks = 0;
        figure_play_die_sound(target);
        if (figure_properties[target->type].is_player_legion_unit) {
            update_formation_morale_after_death(&legion_formations[target->formation_id]);
        } else {
            update_formation_morale_after_death(&enemy_formations[target->formation_id]);
        }
        clear_targeting_on_unit_death(target);
    }
    if (figure_properties[target->type].is_player_legion_unit) {
        legion_formations[target->formation_id].missile_attack_timeout = 6;
    } else if (figure_properties[target->type].is_herd_animal) {
        herd_formations[target->formation_id].missile_attack_timeout = 6;
    } else if (figure_properties[target->type].is_enemy_unit || figure_properties[target->type].is_caesar_legion_unit) {
        enemy_formations[target->formation_id].missile_attack_timeout = 6;
    }
    // clear targeting
    shooter->target_figure_id = 0;
    figure__remove_ranged_targeter_from_list(target, shooter);
    figure_delete(projectile);
}

void figure_arrow_action(struct figure_t *projectile)
{
    projectile->use_cross_country = 1;
    projectile->progress_on_tile++;
    int should_die = figure_movement_move_ticks_cross_country(projectile, 8);
    int target_id = get_target_on_tile(projectile);
    if (target_id) {
        struct figure_t *target = &figures[target_id];
        missile_hit_target(projectile, target);
        play_sound_effect(SOUND_EFFECT_ARROW_HIT);
    }
    int dir = (16 + projectile->direction - 2 * city_view_orientation()) % 16;
    projectile->image_id = image_group(GROUP_FIGURE_MISSILE) + 16 + dir;
    if (projectile->progress_on_tile > 120) {
        figure_delete(projectile);
    }
    if (should_die || target_id) {
        figure_delete(projectile);
    }
}

void figure_javelin_action(struct figure_t *projectile)
{
    projectile->use_cross_country = 1;
    projectile->progress_on_tile++;
    int should_die = figure_movement_move_ticks_cross_country(projectile, 4);
    int target_id = get_target_on_tile(projectile);
    if (target_id) {
        struct figure_t *target = &figures[target_id];
        missile_hit_target(projectile, target);
        play_sound_effect(SOUND_EFFECT_JAVELIN);
    }
    int dir = (16 + projectile->direction - 2 * city_view_orientation()) % 16;
    projectile->image_id = image_group(GROUP_FIGURE_MISSILE) + dir;
    if (should_die || target_id) {
        figure_delete(projectile);
    }
    if (projectile->progress_on_tile > 120) {
        figure_delete(projectile);
    }
}

void figure_bolt_action(struct figure_t *projectile)
{
    projectile->use_cross_country = 1;
    projectile->progress_on_tile++;
    int should_die = figure_movement_move_ticks_cross_country(projectile, 10);
    int target_id = get_target_on_tile(projectile);
    if (target_id) {
        struct figure_t *target = &figures[target_id];
        missile_hit_target(projectile, target);
        play_sound_effect(SOUND_EFFECT_BALLISTA_HIT_PERSON);
    }
    int dir = (16 + projectile->direction - 2 * city_view_orientation()) % 16;
    projectile->image_id = image_group(GROUP_FIGURE_MISSILE) + 32 + dir;
    if (projectile->progress_on_tile > 120) {
        figure_delete(projectile);
    }
    if (should_die || target_id) {
        play_sound_effect(SOUND_EFFECT_BALLISTA_HIT_GROUND);
        figure_delete(projectile);
    }
}
