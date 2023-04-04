#include "combat.h"

#include "building/building.h"
#include "city/data_private.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/combat.h"
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

enum {
    ATTACK_NONE = 0,
    FRONTAL_ATTACK = 1,
    SIDE_ATTACK = 2,
    BACK_ATTACK = 3,
};

int is_valid_target_for_player_unit(figure *target)
{
    return target->is_criminal_unit
        || (target->is_native_unit && target->action_state == FIGURE_ACTION_NATIVE_ATTACKING)
        || target->is_herd_animal
        || target->is_enemy_unit
        || target->is_caesar_legion_unit;
}

int is_valid_target_for_enemy_unit(figure *target)
{
    return target->is_unarmed_civilian_unit
        || target->is_friendly_armed_unit
        || target->is_player_legion_unit
        || target->is_criminal_unit
        || target->is_empire_trader
        || (target->is_native_unit && target->action_state != FIGURE_ACTION_NATIVE_ATTACKING)
        || target->type == FIGURE_WOLF
        || target->is_caesar_legion_unit;
}

static int is_valid_target_for_caesar_unit(figure *target)
{
    return target->is_friendly_armed_unit
        || target->is_player_legion_unit
        || target->is_criminal_unit
        || (target->is_native_unit && target->action_state != FIGURE_ACTION_NATIVE_ATTACKING)
        || target->type == FIGURE_WOLF
        || target->is_enemy_unit;
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

static void figure__remove_melee_targeter_from_list(figure *f, figure *melee_targeter)
{
    for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
        if (f->melee_targeter_ids[i] == melee_targeter->id) {
            f->melee_targeter_ids[i] = 0;
        }
    }
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
            closest_target_distance = 30;
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
            if (f->is_friendly_armed_unit || f->is_player_legion_unit) {
                if (is_valid_target_for_player_unit(potential_target)) {
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

            } else if (f->is_enemy_unit) {
                if (is_valid_target_for_enemy_unit(potential_target)) {
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                    continue;
                }
            } else if (f->is_caesar_legion_unit) {
                if (is_valid_target_for_caesar_unit(potential_target)) {
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

static void engage_in_melee_combat(figure *attacker, figure *opponent)
{
    attacker->action_state_before_attack = attacker->action_state;
    attacker->action_state = FIGURE_ACTION_ATTACK;
    // if ranged unit engages in melee combat, remove it from its (previous) target's ranged targeter list
    if (attacker->max_range && attacker->target_figure_id) {
        figure *target_of_ranged_unit = figure_get(attacker->target_figure_id);
        figure__remove_ranged_targeter_from_list(target_of_ranged_unit, attacker);
    }
    attacker->target_figure_id = opponent->id;
    attacker->melee_combatant_ids[0] = opponent->id;
    attacker->num_melee_combatants++;
    attacker->attack_image_offset = 12;
    if (opponent->x != opponent->destination_x || opponent->y != opponent->destination_y) {
        attacker->attack_direction = calc_general_direction(attacker->previous_tile_x, attacker->previous_tile_y,
            opponent->previous_tile_x, opponent->previous_tile_y);
    } else {
        attacker->attack_direction = calc_general_direction(attacker->previous_tile_x, attacker->previous_tile_y,
            opponent->x, opponent->y);
    }
    if (attacker->attack_direction >= 8) {
        attacker->attack_direction = 0;
    }

    if (opponent->action_state != FIGURE_ACTION_ATTACK) {
        opponent->action_state_before_attack = opponent->action_state;
        opponent->action_state = FIGURE_ACTION_ATTACK;
        // if opponent ranged unit engaged in melee combat, remove it from its (previous) target's ranged targeter list
        if (opponent->max_range && opponent->target_figure_id) {
            figure *target_of_opponent_ranged_unit = figure_get(opponent->target_figure_id);
            figure__remove_ranged_targeter_from_list(target_of_opponent_ranged_unit, opponent);
        }
        opponent->target_figure_id = attacker->id;
        opponent->attack_image_offset = 0;
        opponent->attack_direction = (attacker->attack_direction + 4) % 8;
    }
    // add attacker to opponent's melee combatants list
    for (int i = 0; i < MAX_MELEE_COMBATANTS_PER_UNIT; i++) {
        if (!opponent->melee_combatant_ids[i]) {
            opponent->melee_combatant_ids[i] = attacker->id;
            break;
        }
    }
    opponent->num_melee_combatants++;
}

void figure_combat_attack_figure_at(figure *attacker, int grid_offset)
{
    if (attacker->action_state == FIGURE_ACTION_ATTACK) {
        return;
    }
    int figure_id = map_figures.items[grid_offset];
    while (figure_id) {
        figure *opponent = figure_get(figure_id);
        if (opponent->id != attacker->id
        && !figure_is_dead(opponent)
        && opponent->is_targetable
        && opponent->num_melee_combatants < MAX_MELEE_COMBATANTS_PER_UNIT) {
            if (attacker->is_friendly_armed_unit || attacker->is_player_legion_unit) {
                if (is_valid_target_for_player_unit(opponent)) {
                    engage_in_melee_combat(attacker, opponent);
                    return;
                }
            } else if (attacker->type == FIGURE_WOLF) {
                if (opponent->type != FIGURE_WOLF) {
                    engage_in_melee_combat(attacker, opponent);
                    return;
                }
            } else if (attacker->is_enemy_unit) {
                if (is_valid_target_for_enemy_unit(opponent)) {
                    engage_in_melee_combat(attacker, opponent);
                    return;
                }
            } else if (attacker->is_caesar_legion_unit) {
                if (is_valid_target_for_caesar_unit(opponent)) {
                    engage_in_melee_combat(attacker, opponent);
                    return;
                }
            }
        }
        figure_id = opponent->next_figure_id_on_same_tile;
    }
}

static int determine_attack_direction(int dir1, int dir2)
{
    int deg_abs_delta = abs(dir1 * 45 - dir2 * 45);
    switch (deg_abs_delta) {
        case 0:
        case 45:
        case 315:
            return BACK_ATTACK;
        case 90:
        case 270:
            return SIDE_ATTACK;
        default:
            return FRONTAL_ATTACK;
    }
}

static void hit_opponent(figure *attacker, figure *opponent)
{
    if (opponent->is_unarmed_civilian_unit || opponent->is_criminal_unit) {
        attacker->attack_image_offset = 12;
    } else {
        attacker->attack_image_offset = 0;
    }
    int attacker_attack_value = attacker->melee_attack_value;
    int opponent_defense_value = opponent->melee_defense_value;

    switch (determine_attack_direction(attacker->attack_direction, opponent->attack_direction)) {
        case SIDE_ATTACK:
            attacker_attack_value += 2;
            opponent_defense_value /= 2;
            break;
        case BACK_ATTACK:
            attacker_attack_value *= 2;
            opponent_defense_value = 0;
            sound_effect_play(SOUND_EFFECT_SWORD_SWING);
            break;
        default:
            break;
    }

    int net_attack = attacker_attack_value - opponent_defense_value;
    if (net_attack < 0) {
        net_attack = 0;
    }
    opponent->damage += net_attack;
    if (opponent->damage <= opponent->max_damage) {
        figure_play_hit_sound(attacker->type);
    } else {
        opponent->action_state = FIGURE_ACTION_CORPSE;
        opponent->wait_ticks = 0;
        figure_play_die_sound(opponent);
        formation_update_morale_after_death(&formations[opponent->formation_id]);
        clear_targeting_on_unit_death(opponent);
        refresh_formation_figure_indexes(opponent);
    }
}

static int unit_is_charging_opponent(figure *f, figure *opponent)
{
    return f->mounted_charge_ticks
        && !f->figure_is_halted
        && opponent->type != FIGURE_FORT_MOUNTED && opponent->type != FIGURE_ENEMY_CAMEL && opponent->type != FIGURE_ENEMY_ELEPHANT && opponent->type != FIGURE_ENEMY_CHARIOT && opponent->type != FIGURE_ENEMY_MOUNTED_ARCHER;
}

void figure_combat_handle_attack(figure *f)
{
    figure_movement_advance_attack(f);
    f->attack_image_offset++;

    if (f->target_figure_id) {
        figure *opponent = figure_get(f->target_figure_id);
        if (f->attack_image_offset >= 24 || unit_is_charging_opponent(f, opponent)) {
            hit_opponent(f, opponent);
            if (unit_is_charging_opponent(f, opponent)) {
                sound_effect_play(SOUND_EFFECT_HORSE_MOVING);
                f->mounted_charge_ticks--;
            }
        }
    } else {
        f->action_state = f->action_state_before_attack;
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

void figure__remove_ranged_targeter_from_list(figure *f, figure *ranged_targeter)
{
    for (int i = 0; i < MAX_RANGED_TARGETERS_PER_UNIT; i++) {
        if (f->ranged_targeter_ids[i] == ranged_targeter->id) {
            f->ranged_targeter_ids[i] = 0;
        }
    }
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
    if (map_terrain_at_offset & TERRAIN_SHRUB) {
        int shrub_first_img_id = image_group(GROUP_TERRAIN_SHRUB);
        if (((scenario.climate == CLIMATE_CENTRAL || scenario.climate == CLIMATE_NORTHERN) && map_img_at_offset >= shrub_first_img_id + 8)
        || (scenario.climate == CLIMATE_DESERT && map_img_at_offset >= shrub_first_img_id + 24)) {
            return 1;
        }
    }
    // rocks lower to the ground do not obstruct
    if (map_terrain_at_offset & TERRAIN_ROCK) {
        int rock_first_img_id = image_group(GROUP_TERRAIN_ROCK);
        if (map_img_at_offset >= rock_first_img_id + 8) {
            return 1;
        }
    }
    if (map_terrain_at_offset & (TERRAIN_TREE | TERRAIN_WALL | TERRAIN_GATEHOUSE)) { // "terrain contains any of"
        return 1;
    }
    if (map_terrain_at_offset & TERRAIN_BUILDING) {
        building *b = building_get(map_building_at(grid_offset));
        // buildings with a low max height do not obstruct
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
        if ((potential_target_distance < closest_target_distance) && missile_trajectory_clear(shooter, potential_target)) {
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
            if (shooter->is_friendly_armed_unit || shooter->is_player_legion_unit) {
                if (is_valid_target_for_player_unit(potential_target)) {
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                    continue;
                }
            } else if (shooter->is_enemy_unit) {
                if (potential_target->is_unarmed_civilian_unit
                || potential_target->is_friendly_armed_unit
                || potential_target->is_player_legion_unit
                || potential_target->is_criminal_unit
                || potential_target->is_empire_trader
                || potential_target->type == FIGURE_NATIVE_TRADER // don't target native fighters as to not get stuck in place while they respawn
                || potential_target->type == FIGURE_WOLF
                || potential_target->is_caesar_legion_unit) {
                    // skip (closer) unarmed target if already targeting a dangerous foe
                    if ((potential_target->is_unarmed_civilian_unit || potential_target->is_empire_trader || potential_target->type == FIGURE_NATIVE_TRADER)
                        && closest_eligible_target
                        && (closest_eligible_target->is_friendly_armed_unit || closest_eligible_target->is_player_legion_unit || closest_eligible_target->type == FIGURE_WOLF || closest_eligible_target->is_caesar_legion_unit)) {
                        continue;
                    }
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                    continue;
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

void clear_targeting_on_unit_death(figure *unit)
{
    // remove unit from its target's targeter lists
    figure *target = figure_get(unit->target_figure_id);
    figure__remove_melee_targeter_from_list(target, unit);
    figure__remove_ranged_targeter_from_list(target, unit);

    // reset target of all opponents targeting the unit; remove unit from melee combatant list of all opponents fighting it
    for (int i = 0; i < MAX_FIGURES; i++) {
        figure *opponent = figure_get(i);
        if (!figure_is_dead(opponent)) {
            if (opponent->target_figure_id == unit->id) {
                opponent->target_figure_id = 0;
            }
            for (int j = 0; j < MAX_MELEE_COMBATANTS_PER_UNIT; j++) {
                if (opponent->melee_combatant_ids[j] == unit->id) {
                    opponent->melee_combatant_ids[j] = 0;
                    opponent->num_melee_combatants--;
                }
            }
        }
    }
}