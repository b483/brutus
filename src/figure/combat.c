#include "combat.h"

#include "building/building.h"
#include "city/data_private.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/formation.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "figure/sound.h"
#include "map/building.h"
#include "map/elevation.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/image.h"
#include "map/terrain.h"
#include "scenario/data.h"
#include "sound/effect.h"

#include <stdlib.h>

void figure_combat_handle_corpse(figure *f)
{
    if (f->wait_ticks < 0) {
        f->wait_ticks = 0;
    }
    f->wait_ticks++;
    if (f->wait_ticks >= 128) {
        f->wait_ticks = 127;
        f->state = FIGURE_STATE_DEAD;
    }
}

static int attack_is_same_direction(int dir1, int dir2)
{
    if (dir1 == dir2) {
        return 1;
    }
    int dir2_off = dir2 <= 0 ? 7 : dir2 - 1;
    if (dir1 == dir2_off) {
        return 1;
    }
    dir2_off = dir2 >= 7 ? 0 : dir2 + 1;
    if (dir1 == dir2_off) {
        return 1;
    }
    return 0;
}

static void hit_opponent(figure *f)
{
    const struct formation_t *m = &formations[f->formation_id];
    figure *opponent = figure_get(f->primary_melee_combatant_id);
    struct formation_t *opponent_formation = &formations[opponent->formation_id];

    if (opponent->is_unarmed_civilian_unit || opponent->is_criminal_unit) {
        f->attack_image_offset = 12;
    } else {
        f->attack_image_offset = 0;
    }
    int figure_attack = f->melee_attack_value;
    int opponent_defense = f->melee_defense_value;

    if (opponent->primary_melee_combatant_id != f->id && m->figure_type != FIGURE_FORT_LEGIONARY &&
            attack_is_same_direction(f->attack_direction, opponent->attack_direction)) {
        figure_attack += 4; // attack opponent on the (exposed) back
        sound_effect_play(SOUND_EFFECT_SWORD_SWING);
    }
    if (m->is_halted && m->figure_type == FIGURE_FORT_LEGIONARY &&
            attack_is_same_direction(f->attack_direction, m->direction)) {
        figure_attack += 4; // coordinated formation attack bonus
    }
    // defense modifiers
    if (opponent_formation->is_halted &&
            (opponent_formation->figure_type == FIGURE_FORT_LEGIONARY ||
                opponent_formation->figure_type == FIGURE_ENEMY_CAESAR_LEGIONARY)) {
        if (!attack_is_same_direction(opponent->attack_direction, opponent_formation->direction)) {
            opponent_defense -= 4; // opponent not attacking in coordinated formation
        } else if (opponent_formation->layout == FORMATION_TORTOISE) {
            opponent_defense += 7;
        } else if (opponent_formation->layout == FORMATION_DOUBLE_LINE_1 ||
                   opponent_formation->layout == FORMATION_DOUBLE_LINE_2) {
            opponent_defense += 4;
        }
    }

    int net_attack = figure_attack - opponent_defense;
    if (net_attack < 0) {
        net_attack = 0;
    }
    opponent->damage += net_attack;
    if (opponent->damage <= opponent->max_damage) {
        figure_play_hit_sound(f->type);
    } else {
        opponent->action_state = FIGURE_ACTION_149_CORPSE;
        opponent->wait_ticks = 0;
        figure_play_die_sound(opponent);
        formation_update_morale_after_death(opponent_formation);
    }
}

void figure__remove_ranged_targeter_from_list(figure *f, figure *ranged_targeter)
{
    for (int i = 0; i < MAX_RANGED_TARGETERS_PER_UNIT; i++) {
        if (f->ranged_targeter_ids[i] == ranged_targeter->id) {
            f->ranged_targeter_ids[i] = 0;
        }
    }
}

static void figure__remove_melee_targeter_from_list(figure *f, figure *melee_targeter)
{
    for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
        if (f->melee_targeter_ids[i] == melee_targeter->id) {
            f->melee_targeter_ids[i] = 0;
        }
    }
}

static int unit_is_charging_opponent(figure *f, figure *opponent)
{
    return f->mounted_charge_ticks
        && !formations[f->formation_id].is_halted
        && opponent->type != FIGURE_FORT_MOUNTED && opponent->type != FIGURE_ENEMY46_CAMEL && opponent->type != FIGURE_ENEMY47_ELEPHANT && opponent->type != FIGURE_ENEMY48_CHARIOT && opponent->type != FIGURE_ENEMY52_MOUNTED_ARCHER;
}

void figure_combat_handle_attack(figure *f)
{
    figure_movement_advance_attack(f);

    // synchronize melee target list to melee attacker list as they can diverge when units move around/engage and retarget
    for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
        f->melee_targeter_ids[i] = f->melee_combatant_ids[i];
    }

    // remove dead opponent from figure's melee targeter/attacker lists
    figure *opponent = figure_get(f->primary_melee_combatant_id);
    if (opponent->action_state == FIGURE_ACTION_149_CORPSE) {
        figure__remove_melee_targeter_from_list(f, opponent);
        for (int i = 0; i < MAX_MELEE_COMBATANTS_PER_UNIT; i++) {
            if (f->melee_combatant_ids[i] == opponent->id) {
                f->melee_combatant_ids[i] = 0;
                f->num_melee_combatants--;
            }
        }
        if (opponent->max_range) {
            // if opponent is a ranged unit [that was engaged in melee with figure], remove it from figure's ranged targeters list as well
            figure__remove_ranged_targeter_from_list(f, opponent);
        }
    }

    if (f->num_melee_combatants) {
        f->attack_image_offset++;
        if (f->attack_image_offset >= 24 || unit_is_charging_opponent(f, opponent)) {
            // select next target from list
            for (int i = 0; i < MAX_MELEE_COMBATANTS_PER_UNIT; i++) {
                if (f->melee_combatant_ids[i]) {
                    f->primary_melee_combatant_id = f->melee_combatant_ids[i];
                    f->target_figure_id = f->melee_combatant_ids[i];
                    break;
                }
            }
            hit_opponent(f);
            if (unit_is_charging_opponent(f, opponent)) {
                sound_effect_play(SOUND_EFFECT_HORSE_MOVING);
                f->mounted_charge_ticks--;
            }
        }
    } else {
        f->action_state = f->action_state_before_attack;
        f->target_figure_id = 0;
        f->primary_melee_combatant_id = 0;
        figure_route_remove(f);
    }
    return;
}

static int figure__targeted_by_ranged_unit(figure *f, figure *ranged_targeter)
{
    for (int i = 0; i < MAX_RANGED_TARGETERS_PER_UNIT; i++) {
        if (f->ranged_targeter_ids[i] == ranged_targeter->id) {
            return 1;
        }
    }
    return 0;
}

static int figure__targeted_by_melee_unit(figure *f, figure *melee_targeter)
{
    for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
        if (f->melee_targeter_ids[i] == melee_targeter->id) {
            return 1;
        }
    }
    return 0;
}

figure *melee_unit__set_closest_target(figure *f)
{
    figure *closest_eligible_target = 0;
    int closest_target_distance;
    switch (f->type) {
        case FIGURE_PREFECT:
            closest_target_distance = PREFECT_TARGET_ACQUISITION_RANGE;
            break;
        case FIGURE_WOLF:
            closest_target_distance = 5;
            break;
        default:
            closest_target_distance = 100;
            break;
    }
    for (int i = 1; i < MAX_FIGURES; i++) {
        figure *potential_target = figure_get(i);
        if (figure_is_dead(potential_target) || !potential_target->is_targetable) {
            continue;
        }
        int potential_target_distance = calc_maximum_distance(f->x, f->y, potential_target->x, potential_target->y);
        if (potential_target_distance < closest_target_distance) {
            // skip potential enemy target if its targeter list is full, unless the current targeter is already in it
            int potential_target_melee_targeter_list_full = 1;
            for (int j = 0; j < MAX_MELEE_TARGETERS_PER_UNIT; j++) {
                if (!potential_target->melee_targeter_ids[j]) {
                    potential_target_melee_targeter_list_full = 0;
                    break;
                }
            }
            if (potential_target_melee_targeter_list_full) {
                if (figure__targeted_by_melee_unit(potential_target, f)) {
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                    // the remaining criteria (friendly, enemy) have already been checked on previous targeting, continue to next potential target
                    continue;
                } else {
                    continue;
                }
            }
            if (f->is_friendly_armed_unit) {
                if (potential_target->is_enemy_unit
                || potential_target->type == FIGURE_WOLF
                || potential_target->is_criminal_unit
                || (potential_target->type == FIGURE_INDIGENOUS_NATIVE && potential_target->action_state == FIGURE_ACTION_159_NATIVE_ATTACKING)) {
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                    continue;
                }
            } else if (f->type == FIGURE_WOLF) {
                if (potential_target->type != FIGURE_WOLF) {
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                    continue;
                }

            } else {
                // targeter is enemy unit
                if ((potential_target->is_friendly_armed_unit)
                || potential_target->type == FIGURE_WOLF
                || ((f->is_caesar_legion_unit && !potential_target->is_caesar_legion_unit) || (potential_target->is_caesar_legion_unit && !f->is_caesar_legion_unit))
                || potential_target->type == FIGURE_NATIVE_TRADER
                || (potential_target->type == FIGURE_INDIGENOUS_NATIVE && potential_target->action_state != FIGURE_ACTION_159_NATIVE_ATTACKING)
                ) {
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                    continue;
                }
            }
        }
    }
    if (closest_eligible_target) {
        // set target and destination for figure
        if (f->target_figure_id && f->target_figure_id != closest_eligible_target->id) {
            // if switching targets, remove targeter from previous target's melee targeters list
            figure *previous_target = figure_get(f->target_figure_id);
            figure__remove_melee_targeter_from_list(previous_target, f);
        }
        f->target_figure_id = closest_eligible_target->id;
        f->destination_x = closest_eligible_target->x;
        f->destination_y = closest_eligible_target->y;
        figure_route_remove(f);
        if (!figure__targeted_by_melee_unit(closest_eligible_target, f)) {
            // update new target's melee targeter list
            for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
                if (!closest_eligible_target->melee_targeter_ids[i]) {
                    closest_eligible_target->melee_targeter_ids[i] = f->id;
                    break;
                }
            };
        }
    }
    return closest_eligible_target;
}

static int tile_obstructed(int grid_offset)
{
    int map_terrain_at_offset = map_terrain_get(grid_offset);
    int map_img_at_offset = map_image_at(grid_offset);

    // low elevation between targets does not obstruct
    if (map_elevation_at(grid_offset) > 1) {
        return 1;
    }
    // shrubs lower to the ground do not obstruct
    if (map_terrain_at_offset == TERRAIN_SHRUB) {
        int shrub_first_img_id = image_group(GROUP_TERRAIN_SHRUB);
        if (((scenario.climate == CLIMATE_CENTRAL || scenario.climate == CLIMATE_NORTHERN) && map_img_at_offset >= shrub_first_img_id + 8)
        || (scenario.climate == CLIMATE_DESERT && map_img_at_offset >= shrub_first_img_id + 24)) {
            return 1;
        }
    }
    // rocks lower to the ground do not obstruct
    if (map_terrain_at_offset == TERRAIN_ROCK) {
        int rock_first_img_id = image_group(GROUP_TERRAIN_ROCK);
        if (map_img_at_offset >= rock_first_img_id + 8) {
            return 1;
        }
    }
    if (map_terrain_at_offset == TERRAIN_TREE
    || map_terrain_at_offset == TERRAIN_WALL
    || map_terrain_at_offset == (TERRAIN_BUILDING | TERRAIN_ROAD | TERRAIN_GATEHOUSE) // gatehouse
    || map_terrain_at_offset == (TERRAIN_BUILDING | TERRAIN_GATEHOUSE) // tower
    ) {
        return 1;
    }
    if (map_terrain_at_offset == TERRAIN_BUILDING) {
        building *b = building_get(map_building_at(grid_offset));
        if (b->type != BUILDING_FORT_GROUND && b->type != BUILDING_CLAY_PIT && b->type != BUILDING_MARBLE_QUARRY) {
            return 1;
        }
    }

    return 0;
}

static int missile_trajectory_clear(figure *shooter, figure *target)
{
    int shooter_elevation = 0;
    if (map_elevation_at(shooter->grid_offset)) {
        shooter_elevation += map_elevation_at(shooter->grid_offset);
    }
    if (map_terrain_is(shooter->grid_offset, TERRAIN_WALL_OR_GATEHOUSE)) {
        shooter_elevation += 6;
    }
    int target_elevation = 0;
    if (map_elevation_at(target->grid_offset)) {
        target_elevation += map_elevation_at(target->grid_offset);
    }

    int x_delta = abs(shooter->x - target->x);
    int y_delta = abs(shooter->y - target->y);
    double slope;
    int x_check = 0;
    int y_check = 0;
    int delta;
    if (x_delta > y_delta) {
        slope = (double) (target->y - shooter->y) / (double) (target->x - shooter->x);
        x_check = shooter->x;
        delta = x_delta;
    } else {
        slope = (double) (target->x - shooter->x) / (double) (target->y - shooter->y);
        y_check = shooter->y;
        delta = y_delta;
    }
    while (delta > 1) {
        delta--;
        if (x_delta > y_delta) {
            if (shooter->x < target->x) {
                x_check++;
            } else {
                x_check--;
            }
            y_check = slope * (x_check - shooter->x) + shooter->y;
        } else {
            if (shooter->y < target->y) {
                y_check++;
            } else {
                y_check--;
            }
            x_check = slope * (y_check - shooter->y) + shooter->x;
        }
        if (shooter_elevation > target_elevation) {
            shooter_elevation--;
            continue;
        }
        if (tile_obstructed(map_grid_offset(x_check, y_check))) {
            return 0;
        }
    }

    return 1;
}

int set_missile_target(figure *shooter, map_point *tile, int limit_max_targeters)
{
    int closest_target_distance = shooter->max_range;
    figure *closest_eligible_target = 0;
    for (int i = 1; i < MAX_FIGURES; i++) {
        figure *potential_target = figure_get(i);
        if (figure_is_dead(potential_target) || !potential_target->is_targetable) {
            continue;
        }
        int potential_target_distance = calc_maximum_distance(shooter->x, shooter->y, potential_target->x, potential_target->y);
        if (potential_target_distance < closest_target_distance) {
            // if potential target is the current target, it's eligible (could still end up being switched for a nearer one)
            if (figure__targeted_by_ranged_unit(potential_target, shooter)) {
                closest_target_distance = potential_target_distance;
                closest_eligible_target = potential_target;
                continue;
            }
            if (limit_max_targeters) {
                // if ranged targeter list of potential enemy target is full, skip potential target
                int potential_target_ranged_targeter_list_full = 1;
                for (int j = 0; j < MAX_RANGED_TARGETERS_PER_UNIT; j++) {
                    if (!potential_target->ranged_targeter_ids[j]) {
                        potential_target_ranged_targeter_list_full = 0;
                        break;
                    }
                }
                if (potential_target_ranged_targeter_list_full) {
                    continue;
                }
            }
            if (shooter->is_friendly_armed_unit) {
                if (potential_target->is_enemy_unit
                || potential_target->type == FIGURE_WOLF
                || potential_target->is_criminal_unit
                || (potential_target->type == FIGURE_INDIGENOUS_NATIVE && potential_target->action_state == FIGURE_ACTION_159_NATIVE_ATTACKING)) {
                    if (missile_trajectory_clear(shooter, potential_target)) {
                        closest_target_distance = potential_target_distance;
                        closest_eligible_target = potential_target;
                        continue;
                    }

                }
            } else {
                // shooter is enemy unit
                if (potential_target->is_friendly_armed_unit
                || potential_target->type == FIGURE_WOLF
                || (potential_target->is_caesar_legion_unit && !shooter->is_caesar_legion_unit)
                || potential_target->is_unarmed_civilian_unit
                // target only trader natives so as to not get stuck in place shooting respawning fighters
                || potential_target->type == FIGURE_NATIVE_TRADER) {
                    // skip (closer) civilian/native trader if defender or wolf an eligible target
                    if ((potential_target->is_unarmed_civilian_unit || potential_target->type == FIGURE_NATIVE_TRADER)
                    && closest_eligible_target
                    && (closest_eligible_target->is_friendly_armed_unit || closest_eligible_target->type == FIGURE_WOLF)) {
                        continue;
                    }
                    if (missile_trajectory_clear(shooter, potential_target)) {
                        closest_target_distance = potential_target_distance;
                        closest_eligible_target = potential_target;
                        continue;
                    }
                }
            }
        }
    }
    if (closest_eligible_target) {
        if (shooter->target_figure_id && shooter->target_figure_id != closest_eligible_target->id) {
            // if switching targets, remove targeter from previous target's ranged targeters list
            figure *previous_target = figure_get(shooter->target_figure_id);
            figure__remove_ranged_targeter_from_list(previous_target, shooter);
        }
        shooter->target_figure_id = closest_eligible_target->id;
        if (!figure__targeted_by_ranged_unit(closest_eligible_target, shooter)) {
            // update new target's ranged targeters list
            for (int i = 0; i < MAX_RANGED_TARGETERS_PER_UNIT; i++) {
                if (!closest_eligible_target->ranged_targeter_ids[i]) {
                    closest_eligible_target->ranged_targeter_ids[i] = shooter->id;
                    break;
                }
            };
        }
        map_point_store_result(closest_eligible_target->x, closest_eligible_target->y, tile);
        return 1;
    }

    return 0;
}

void figure_combat_attack_figure_at(figure *f, int grid_offset)
{
    if (f->action_state == FIGURE_ACTION_150_ATTACK) {
        return;
    }
    int guard = 0;
    int primary_melee_combatant_id = map_figure_at(grid_offset);
    while (1) {
        if (++guard >= MAX_FIGURES || primary_melee_combatant_id <= 0) {
            break;
        }
        figure *opponent = figure_get(primary_melee_combatant_id);
        if (primary_melee_combatant_id == f->id) {
            primary_melee_combatant_id = opponent->next_figure_id_on_same_tile;
            continue;
        }

        int attack = 0;
        if (opponent->state != FIGURE_STATE_ALIVE) {
            attack = 0;
        } else if (opponent->action_state == FIGURE_ACTION_149_CORPSE) {
            attack = 0;
        } else if (f->is_friendly_armed_unit && (opponent->is_enemy_unit || opponent->type == FIGURE_WOLF)) {
            attack = 1;
        } else if (f->is_friendly_armed_unit && opponent->is_criminal_unit) {
            attack = 1;
        } else if (f->is_friendly_armed_unit && opponent->is_herd_animal) {
            attack = 1;
        } else if (f->is_friendly_armed_unit && opponent->is_native_unit) {
            if (opponent->action_state == FIGURE_ACTION_159_NATIVE_ATTACKING) {
                attack = 1;
            }
        } else if ((f->is_enemy_unit || f->type == FIGURE_WOLF) && opponent->is_unarmed_civilian_unit) {
            attack = 1;
        } else if ((f->is_enemy_unit || f->type == FIGURE_WOLF) && opponent->is_friendly_armed_unit) {
            attack = 1;
        } else if ((f->is_enemy_unit || f->type == FIGURE_WOLF) && opponent->is_criminal_unit) {
            attack = 1;
        } else if ((f->is_enemy_unit || f->type == FIGURE_WOLF) && opponent->is_native_unit) {
            // wolves attack natives
            if (f->type == FIGURE_WOLF) {
                attack = 1;
            }
            // other hostiles (i.e. invaders) only fight with natives that aren't attacking the player
            else if (opponent->action_state != FIGURE_ACTION_159_NATIVE_ATTACKING) {
                attack = 1;
            }
        } else if ((f->is_enemy_unit || f->type == FIGURE_WOLF) && opponent->is_herd_animal) {
            attack = 1;
        } else if ((f->is_enemy_unit || f->type == FIGURE_WOLF) && (opponent->is_enemy_unit || opponent->type == FIGURE_WOLF)) {
            // wolves attack every hostile unit not a wolf
            if (f->type == FIGURE_WOLF && opponent->type != FIGURE_WOLF) {
                attack = 1;
            } else if
                // invaders and caesar's legions attack each other
                ((!f->is_caesar_legion_unit && opponent->is_caesar_legion_unit)
                || (f->is_caesar_legion_unit && !opponent->is_caesar_legion_unit)) {
                attack = 1;
            }
        }

        if (opponent->action_state == FIGURE_ACTION_150_ATTACK && opponent->num_melee_combatants >= MAX_MELEE_COMBATANTS_PER_UNIT) {
            attack = 0;
        }

        if (attack) {
            f->action_state_before_attack = f->action_state;
            f->action_state = FIGURE_ACTION_150_ATTACK;
            // if ranged unit engages in melee combat, remove it from its (previous) target's ranged targeter list
            if (f->max_range && f->target_figure_id) {
                figure *target_of_ranged_unit = figure_get(f->target_figure_id);
                figure__remove_ranged_targeter_from_list(target_of_ranged_unit, f);
            }
            f->target_figure_id = primary_melee_combatant_id;
            f->primary_melee_combatant_id = primary_melee_combatant_id;
            f->melee_combatant_ids[0] = primary_melee_combatant_id;
            f->num_melee_combatants++;
            f->attack_image_offset = 12;
            if (opponent->x != opponent->destination_x || opponent->y != opponent->destination_y) {
                f->attack_direction = calc_general_direction(f->previous_tile_x, f->previous_tile_y,
                    opponent->previous_tile_x, opponent->previous_tile_y);
            } else {
                f->attack_direction = calc_general_direction(f->previous_tile_x, f->previous_tile_y,
                    opponent->x, opponent->y);
            }
            if (f->attack_direction >= 8) {
                f->attack_direction = 0;
            }

            if (opponent->action_state != FIGURE_ACTION_150_ATTACK) {
                opponent->action_state_before_attack = opponent->action_state;
                opponent->action_state = FIGURE_ACTION_150_ATTACK;
                // if opponent ranged unit engaged in melee combat, remove it from its (previous) target's ranged targeter list
                if (opponent->max_range && opponent->target_figure_id) {
                    figure *target_of_opponent_ranged_unit = figure_get(opponent->target_figure_id);
                    figure__remove_ranged_targeter_from_list(target_of_opponent_ranged_unit, opponent);
                }
                opponent->target_figure_id = f->id;
                opponent->primary_melee_combatant_id = f->id;
                opponent->attack_image_offset = 0;
                opponent->attack_direction = (f->attack_direction + 4) % 8;
            }
            // add attacker to opponent's melee combatants list
            for (int i = 0; i < MAX_MELEE_COMBATANTS_PER_UNIT; i++) {
                if (!opponent->melee_combatant_ids[i]) {
                    opponent->melee_combatant_ids[i] = f->id;
                    break;
                }
            }
            opponent->num_melee_combatants++;

            return;
        }
        primary_melee_combatant_id = opponent->next_figure_id_on_same_tile;
    }
}
