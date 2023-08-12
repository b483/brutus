#include "combat.h"

#include "building/building.h"
#include "city/data.h"
#include "core/calc.h"
#include "core/image.h"
#include "figure/combat.h"
#include "figure/formation_enemy.h"
#include "figure/formation_legion.h"
#include "figure/movement.h"
#include "figure/route.h"
#include "figure/sound.h"
#include "figuretype/enemy.h"
#include "map/building.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/image.h"
#include "map/terrain.h"
#include "scenario/scenario.h"
#include "sound/effect.h"

#include <stdlib.h>

enum {
    ATTACK_NONE = 0,
    FRONTAL_ATTACK = 1,
    SIDE_ATTACK = 2,
    BACK_ATTACK = 3,
};

static int tile_obstructed(int grid_offset)
{
    int map_img_at_offset = map_image_at(grid_offset);

    // low elevation between targets does not obstruct
    if (terrain_elevation.items[grid_offset] > 1) {
        return 1;
    }
    // shrubs lower to the ground do not obstruct
    if (terrain_grid.items[grid_offset] & TERRAIN_SHRUB) {
        int shrub_first_img_id = image_group(GROUP_TERRAIN_SHRUB);
        if (((scenario.climate == CLIMATE_CENTRAL || scenario.climate == CLIMATE_NORTHERN) && map_img_at_offset >= shrub_first_img_id + 8)
        || (scenario.climate == CLIMATE_DESERT && map_img_at_offset >= shrub_first_img_id + 24)) {
            return 1;
        }
    }
    // rocks lower to the ground do not obstruct
    if (terrain_grid.items[grid_offset] & TERRAIN_ROCK) {
        int rock_first_img_id = image_group(GROUP_TERRAIN_ROCK);
        if (map_img_at_offset >= rock_first_img_id + 8) {
            return 1;
        }
    }
    if (terrain_grid.items[grid_offset] & (TERRAIN_TREE | TERRAIN_WALL | TERRAIN_GATEHOUSE)) { // "terrain contains any of"
        return 1;
    }
    if (terrain_grid.items[grid_offset] & TERRAIN_BUILDING) {
        struct building_t *b = &all_buildings[map_building_at(grid_offset)];
        // buildings with a low max height do not obstruct
        if (b->type != BUILDING_FORT_GROUND && b->type != BUILDING_CLAY_PIT && b->type != BUILDING_MARBLE_QUARRY) {
            return 1;
        }
    }

    return 0;
}

static int can_see_target(struct figure_t *observer, struct figure_t *target)
{
    int observer_elevation = terrain_elevation.items[observer->grid_offset];
    if (map_terrain_is(observer->grid_offset, TERRAIN_WALL_OR_GATEHOUSE)) {
        observer_elevation += 6;
    }

    int x_delta = abs(observer->x - target->x);
    int y_delta = abs(observer->y - target->y);
    double slope;
    int x_check = 0;
    int y_check = 0;
    int delta;
    if (x_delta > y_delta) {
        slope = (double) (target->y - observer->y) / (double) (target->x - observer->x);
        x_check = observer->x;
        delta = x_delta;
    } else {
        slope = (double) (target->x - observer->x) / (double) (target->y - observer->y);
        y_check = observer->y;
        delta = y_delta;
    }
    while (delta > 1) {
        delta--;
        if (x_delta > y_delta) {
            if (observer->x < target->x) {
                x_check++;
            } else {
                x_check--;
            }
            y_check = slope * (x_check - observer->x) + observer->y;
        } else {
            if (observer->y < target->y) {
                y_check++;
            } else {
                y_check--;
            }
            x_check = slope * (y_check - observer->y) + observer->x;
        }
        if (observer_elevation > terrain_elevation.items[target->grid_offset]) {
            observer_elevation--;
            continue;
        }
        if (tile_obstructed(map_grid_offset(x_check, y_check))) {
            return 0;
        }
    }

    return 1;
}

int is_valid_target_for_player_unit(struct figure_t *target)
{
    return figure_properties[target->type].is_criminal_unit
        || (figure_properties[target->type].is_native_unit && target->action_state == FIGURE_ACTION_NATIVE_ATTACKING)
        || target->type == FIGURE_WOLF
        || figure_properties[target->type].is_enemy_unit
        || figure_properties[target->type].is_caesar_legion_unit;
}

int is_valid_target_for_enemy_unit(struct figure_t *target)
{
    return figure_properties[target->type].is_unarmed_civilian_unit
        || figure_properties[target->type].is_friendly_armed_unit
        || figure_properties[target->type].is_player_legion_unit
        || figure_properties[target->type].is_criminal_unit
        || figure_properties[target->type].is_empire_trader
        || (figure_properties[target->type].is_native_unit && target->action_state != FIGURE_ACTION_NATIVE_ATTACKING)
        || target->type == FIGURE_WOLF
        || figure_properties[target->type].is_caesar_legion_unit;
}

static int is_valid_target_for_caesar_unit(struct figure_t *target)
{
    return figure_properties[target->type].is_friendly_armed_unit
        || figure_properties[target->type].is_player_legion_unit
        || figure_properties[target->type].is_criminal_unit
        || (figure_properties[target->type].is_native_unit && target->action_state != FIGURE_ACTION_NATIVE_ATTACKING)
        || target->type == FIGURE_WOLF
        || figure_properties[target->type].is_enemy_unit;
}

static int figure__targeted_by_melee_unit(struct figure_t *f, struct figure_t *melee_targeter)
{
    for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
        if (f->melee_targeter_ids[i] == melee_targeter->id) {
            return 1;
        }
    }
    return 0;
}

static void figure__remove_melee_targeter_from_list(struct figure_t *f, struct figure_t *melee_targeter)
{
    for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
        if (f->melee_targeter_ids[i] == melee_targeter->id) {
            f->melee_targeter_ids[i] = 0;
        }
    }
}

struct figure_t *melee_unit__set_closest_target(struct figure_t *f)
{
    struct figure_t *closest_eligible_target = 0;
    int closest_target_distance;
    switch (f->type) {
        case FIGURE_PREFECT:
            closest_target_distance = PREFECT_TARGET_ACQUISITION_RANGE;
            break;
        case FIGURE_WOLF:
            closest_target_distance = 5;
            break;
        default:
            closest_target_distance = 20;
            break;
    }
    for (int i = 1; i < MAX_FIGURES; i++) {
        struct figure_t *potential_target = &figures[i];
        if (!figure_is_alive(potential_target) || !potential_target->is_targetable) {
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
            if (figure_properties[f->type].is_friendly_armed_unit || figure_properties[f->type].is_player_legion_unit) {
                if (is_valid_target_for_player_unit(potential_target) && can_see_target(f, potential_target)) { // can_see_target is expensive, call it last (allows for short-circuiting)
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                    continue;
                }
            } else if (f->type == FIGURE_WOLF) {
                if (potential_target->type != FIGURE_WOLF && can_see_target(f, potential_target)) {
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                    continue;
                }
            } else if (figure_properties[f->type].is_enemy_unit) {
                if (is_valid_target_for_enemy_unit(potential_target) && can_see_target(f, potential_target)) {
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                    continue;
                }
            } else if (figure_properties[f->type].is_caesar_legion_unit) {
                if (is_valid_target_for_caesar_unit(potential_target) && can_see_target(f, potential_target)) {
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
            struct figure_t *previous_target = &figures[f->target_figure_id];
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

static void engage_in_melee_combat(struct figure_t *attacker, struct figure_t *opponent)
{
    attacker->engaged_in_combat = 1;
    // if ranged unit engages in melee combat, remove it from its (previous) target's ranged targeter list
    if (figure_properties[attacker->type].max_range && attacker->target_figure_id) {
        struct figure_t *target_of_ranged_unit = &figures[attacker->target_figure_id];
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

    if (!opponent->engaged_in_combat) {
        opponent->engaged_in_combat = 1;
        // if opponent ranged unit engaged in melee combat, remove it from its (previous) target's ranged targeter list
        if (figure_properties[opponent->type].max_range && opponent->target_figure_id) {
            struct figure_t *target_of_opponent_ranged_unit = &figures[opponent->target_figure_id];
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

void melee_attack_figure_at_offset(struct figure_t *attacker, int grid_offset)
{
    if (attacker->engaged_in_combat) {
        return;
    }
    int figure_id = map_figures.items[grid_offset];
    while (figure_id) {
        struct figure_t *opponent = &figures[figure_id];
        if (opponent->id != attacker->id
        && figure_is_alive(opponent)
        && opponent->is_targetable
        && opponent->num_melee_combatants < MAX_MELEE_COMBATANTS_PER_UNIT) {
            if (figure_properties[attacker->type].is_friendly_armed_unit || figure_properties[attacker->type].is_player_legion_unit) {
                if (is_valid_target_for_player_unit(opponent) || (opponent->type == FIGURE_SHEEP || opponent->type == FIGURE_ZEBRA)) {
                    engage_in_melee_combat(attacker, opponent);
                    return;
                }
            } else if (attacker->type == FIGURE_WOLF) {
                if (opponent->type != FIGURE_WOLF) {
                    engage_in_melee_combat(attacker, opponent);
                    return;
                }
            } else if (figure_properties[attacker->type].is_enemy_unit) {
                if (is_valid_target_for_enemy_unit(opponent)) {
                    engage_in_melee_combat(attacker, opponent);
                    return;
                }
            } else if (figure_properties[attacker->type].is_caesar_legion_unit) {
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

static void hit_opponent(struct figure_t *attacker, struct figure_t *opponent)
{
    if (figure_properties[opponent->type].is_unarmed_civilian_unit || figure_properties[opponent->type].is_criminal_unit) {
        attacker->attack_image_offset = 12;
    } else {
        attacker->attack_image_offset = 0;
    }
    int attacker_attack_value = figure_properties[attacker->type].melee_attack_value;
    int opponent_defense_value = figure_properties[opponent->type].melee_defense_value;

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
    if (opponent->damage <= figure_properties[opponent->type].max_damage) {
        figure_play_hit_sound(attacker->type);
    } else {
        opponent->is_corpse = 1;
        opponent->is_targetable = 0;
        opponent->wait_ticks = 0;
        figure_play_die_sound(opponent);
        if (figure_properties[opponent->type].is_player_legion_unit) {
            update_formation_morale_after_death(&legion_formations[opponent->formation_id]);
        } else {
            update_formation_morale_after_death(&enemy_formations[opponent->formation_id]);
        }
        clear_targeting_on_unit_death(opponent);
    }
}

static int unit_is_charging_opponent(struct figure_t *f, struct figure_t *opponent)
{
    return f->mounted_charge_ticks
        && !f->figure_is_halted
        && opponent->type != FIGURE_FORT_MOUNTED
        && opponent->type != FIGURE_ENEMY_EGYPTIAN_CAMEL
        && opponent->type != FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT
        && opponent->type != FIGURE_ENEMY_BRITON_CHARIOT && opponent->type != FIGURE_ENEMY_CELT_CHARIOT && opponent->type != FIGURE_ENEMY_PICT_CHARIOT
        && opponent->type != FIGURE_ENEMY_HUN_MOUNTED_ARCHER && opponent->type != FIGURE_ENEMY_GOTH_MOUNTED_ARCHER && opponent->type != FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER;
}

void figure_combat_handle_attack(struct figure_t *f)
{
    figure_movement_advance_attack(f);
    f->attack_image_offset++;

    if (f->target_figure_id) {
        struct figure_t *opponent = &figures[f->target_figure_id];
        if (f->attack_image_offset >= 24 || unit_is_charging_opponent(f, opponent)) {
            hit_opponent(f, opponent);
            if (unit_is_charging_opponent(f, opponent)) {
                sound_effect_play(SOUND_EFFECT_HORSE_MOVING);
                f->mounted_charge_ticks--;
            }
            if (figure_properties[opponent->type].is_player_legion_unit) {
                legion_formations[opponent->formation_id].recent_fight = 6;
            } else if (figure_properties[opponent->type].is_enemy_unit || figure_properties[opponent->type].is_caesar_legion_unit) {
                enemy_formations[opponent->formation_id].recent_fight = 6;
            }
        }
    } else {
        f->engaged_in_combat = 0;
        figure_route_remove(f);
    }

    figure_image_increase_offset(f, 12);
    int dir = get_direction(f);
    switch (f->type) {
        case FIGURE_GLADIATOR:
        case FIGURE_ENEMY_GLADIATOR:
            f->image_id = image_group(GROUP_FIGURE_GLADIATOR) + dir + 104 + 8 * (f->image_offset / 2);
            break;
        case FIGURE_LION_TAMER:
            f->image_id = image_group(GROUP_FIGURE_LION_TAMER) + dir + 8 * f->image_offset;
            break;
        case FIGURE_TOWER_SENTRY:
            if (f->attack_image_offset < 12) {
                f->image_id = image_group(GROUP_FIGURE_TOWER_SENTRY) + 96 + figure_image_direction(f);
            } else {
                f->image_id = image_group(GROUP_FIGURE_TOWER_SENTRY) + 96 + figure_image_direction(f) + 8 * ((f->attack_image_offset - 12) / 2);
                // some buffer images missing, img ids 6841+ are for corpse
                if (f->image_id >= 6841) {
                    f->image_id = f->image_id - 8;
                }
            }
            break;
        case FIGURE_PREFECT:
            if (f->attack_image_offset >= 12) {
                f->image_id = image_group(GROUP_FIGURE_PREFECT) + 104 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            } else {
                f->image_id = image_group(GROUP_FIGURE_PREFECT) + 104 + dir;
            }
            break;
        case FIGURE_FORT_JAVELIN:
            if (f->attack_image_offset < 12) {
                f->image_id = image_group(GROUP_BUILDING_FORT_JAVELIN) + 96 + dir;
            } else {
                f->image_id = image_group(GROUP_BUILDING_FORT_JAVELIN) + 96 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            }
            break;
        case FIGURE_FORT_MOUNTED:
            if (f->attack_image_offset < 12) {
                f->image_id = image_group(GROUP_FIGURE_FORT_MOUNTED) + 96 + dir;
            } else {
                f->image_id = image_group(GROUP_FIGURE_FORT_MOUNTED) + 96 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            }
            break;
        case FIGURE_FORT_LEGIONARY:
            if (f->attack_image_offset < 12) {
                f->image_id = image_group(GROUP_BUILDING_FORT_LEGIONARY) + 96 + f->attack_direction;
            } else {
                f->image_id = image_group(GROUP_BUILDING_FORT_LEGIONARY) + 96 + f->attack_direction + 8 * ((f->attack_image_offset - 12) / 2);
            }
            break;
        case FIGURE_INDIGENOUS_NATIVE:
        case FIGURE_ENEMY_BARBARIAN_SWORDSMAN:
            if (f->attack_image_offset >= 12) {
                f->image_id = 393 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            } else {
                f->image_id = 393 + dir;
            }
            break;
        case FIGURE_WOLF:
            f->image_id = image_group(GROUP_FIGURE_WOLF) + 104 + dir + 8 * (f->attack_image_offset / 4);
            break;
        case FIGURE_ENEMY_CARTHAGINIAN_SWORDSMAN:
        case FIGURE_ENEMY_BRITON_SWORDSMAN:
        case FIGURE_ENEMY_CELT_SWORDSMAN:
        case FIGURE_ENEMY_PICT_SWORDSMAN:
        case FIGURE_ENEMY_EGYPTIAN_SWORDSMAN:
        case FIGURE_ENEMY_ETRUSCAN_SWORDSMAN:
        case FIGURE_ENEMY_SAMNITE_SWORDSMAN:
        case FIGURE_ENEMY_GAUL_SWORDSMAN:
        case FIGURE_ENEMY_HELVETIUS_SWORDSMAN:
        case FIGURE_ENEMY_GREEK_SWORDSMAN:
        case FIGURE_ENEMY_MACEDONIAN_SWORDSMAN:
        case FIGURE_ENEMY_PERGAMUM_SWORDSMAN:
        case FIGURE_ENEMY_IBERIAN_SWORDSMAN:
        case FIGURE_ENEMY_JUDEAN_SWORDSMAN:
        case FIGURE_ENEMY_SELEUCID_SWORDSMAN:
            if (f->attack_image_offset >= 12) {
                f->image_id = 545 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            } else {
                f->image_id = 545 + dir;
            }
            break;
        case FIGURE_ENEMY_CARTHAGINIAN_ELEPHANT:
        case FIGURE_ENEMY_EGYPTIAN_CAMEL:
        case FIGURE_ENEMY_HUN_MOUNTED_ARCHER:
        case FIGURE_ENEMY_GOTH_MOUNTED_ARCHER:
        case FIGURE_ENEMY_VISIGOTH_MOUNTED_ARCHER:
            f->image_id = 601 + dir + 8 * f->image_offset;
            break;
        case FIGURE_ENEMY_BRITON_CHARIOT:
        case FIGURE_ENEMY_CELT_CHARIOT:
        case FIGURE_ENEMY_PICT_CHARIOT:
            f->image_id = 697 + dir + 8 * (f->image_offset / 2);
            break;
        case FIGURE_ENEMY_ETRUSCAN_SPEAR_THROWER:
        case FIGURE_ENEMY_SAMNITE_SPEAR_THROWER:
        case FIGURE_ENEMY_GREEK_SPEAR_THROWER:
        case FIGURE_ENEMY_MACEDONIAN_SPEAR_THROWER:
        case FIGURE_ENEMY_PERGAMUM_ARCHER:
        case FIGURE_ENEMY_IBERIAN_SPEAR_THROWER:
        case FIGURE_ENEMY_JUDEAN_SPEAR_THROWER:
        case FIGURE_ENEMY_SELEUCID_SPEAR_THROWER:
            if (f->attack_image_offset >= 12) {
                f->image_id = 745 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            } else {
                f->image_id = 745 + dir;
            }
            break;
        case FIGURE_ENEMY_GAUL_AXEMAN:
        case FIGURE_ENEMY_HELVETIUS_AXEMAN:
            if (f->attack_image_offset >= 12) {
                f->image_id = 697 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            } else {
                f->image_id = 697 + dir;
            }
            break;
        case FIGURE_ENEMY_HUN_SWORDSMAN:
        case FIGURE_ENEMY_GOTH_SWORDSMAN:
        case FIGURE_ENEMY_VISIGOTH_SWORDSMAN:
            if (f->attack_image_offset >= 12) {
                f->image_id = 545 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            } else {
                f->image_id = 545 + dir;
            }
            break;
        case FIGURE_ENEMY_NUMIDIAN_SWORDSMAN:
        case FIGURE_ENEMY_NUMIDIAN_SPEAR_THROWER:
            if (f->attack_image_offset >= 12) {
                f->image_id = 593 + dir + 8 * ((f->attack_image_offset - 12) / 2);
            } else {
                f->image_id = 593 + dir;
            }
            break;
        case FIGURE_ENEMY_CAESAR_LEGIONARY:
            if (f->attack_image_offset >= 12) {
                f->image_id = image_group(GROUP_FIGURE_CAESAR_LEGIONARY) + dir + 8 * ((f->attack_image_offset - 12) / 2);
            } else {
                f->image_id = image_group(GROUP_FIGURE_CAESAR_LEGIONARY) + dir;
            }
            break;
    }
    return;
}

static int figure__targeted_by_ranged_unit(struct figure_t *f, struct figure_t *ranged_targeter)
{
    for (int i = 0; i < MAX_RANGED_TARGETERS_PER_UNIT; i++) {
        if (f->ranged_targeter_ids[i] == ranged_targeter->id) {
            return 1;
        }
    }
    return 0;
}

void figure__remove_ranged_targeter_from_list(struct figure_t *f, struct figure_t *ranged_targeter)
{
    for (int i = 0; i < MAX_RANGED_TARGETERS_PER_UNIT; i++) {
        if (f->ranged_targeter_ids[i] == ranged_targeter->id) {
            f->ranged_targeter_ids[i] = 0;
        }
    }
}

static int target_ranged_targeter_list_full(struct figure_t *target)
{
    for (int j = 0; j < MAX_RANGED_TARGETERS_PER_UNIT; j++) {
        if (!target->ranged_targeter_ids[j]) {
            return 0;
        }
    }
    return 1;
}

static void update_ranged_targeting(struct figure_t *shooter, struct figure_t *target, struct map_point_t *tile)
{
    if (shooter->target_figure_id && shooter->target_figure_id != target->id) {
        // if switching targets, remove targeter from previous target's ranged targeters list
        struct figure_t *previous_target = &figures[shooter->target_figure_id];
        figure__remove_ranged_targeter_from_list(previous_target, shooter);
    }
    shooter->target_figure_id = target->id;
    if (!figure__targeted_by_ranged_unit(target, shooter)) {
        // update new target's ranged targeters list
        for (int i = 0; i < MAX_RANGED_TARGETERS_PER_UNIT; i++) {
            if (!target->ranged_targeter_ids[i]) {
                target->ranged_targeter_ids[i] = shooter->id;
                break;
            }
        };
    }
    tile->x = target->x;
    tile->y = target->y;
}

int set_missile_target(struct figure_t *shooter, struct map_point_t *tile)
{
    int closest_target_distance = figure_properties[shooter->type].max_range;
    struct figure_t *closest_eligible_target = 0;
    struct figure_t *closest_eligible_overhit_target = 0;
    for (int i = 1; i < MAX_FIGURES; i++) {
        struct figure_t *potential_target = &figures[i];
        if (!figure_is_alive(potential_target) || !potential_target->is_targetable) {
            continue;
        }
        int potential_target_distance = calc_maximum_distance(shooter->x, shooter->y, potential_target->x, potential_target->y);
        if ((potential_target_distance < closest_target_distance)) {
            // if potential target is the current target, it's eligible (could still end up being switched for a nearer one)
            if (figure__targeted_by_ranged_unit(potential_target, shooter)) {
                closest_target_distance = potential_target_distance;
                closest_eligible_target = potential_target;
                continue;
            }
            if (figure_properties[shooter->type].is_friendly_armed_unit || figure_properties[shooter->type].is_player_legion_unit) {
                if (is_valid_target_for_player_unit(potential_target) && can_see_target(shooter, potential_target)) {
                    if (shooter->is_military_trained && target_ranged_targeter_list_full(potential_target)) {
                        // prefer targets that aren't already aimed at to prevent overhit, but keep overhit targets as a second option
                        closest_eligible_overhit_target = potential_target;
                    } else {
                        closest_target_distance = potential_target_distance;
                        closest_eligible_target = potential_target;
                    }
                    continue;
                }
            } else if (figure_properties[shooter->type].is_enemy_unit) {
                // skip (closer) unarmed target if already targeting a dangerous foe
                if ((figure_properties[potential_target->type].is_unarmed_civilian_unit || figure_properties[potential_target->type].is_empire_trader || potential_target->type == FIGURE_NATIVE_TRADER)
                    && closest_eligible_target
                    && (figure_properties[closest_eligible_target->type].is_friendly_armed_unit || figure_properties[closest_eligible_target->type].is_player_legion_unit || closest_eligible_target->type == FIGURE_WOLF || figure_properties[closest_eligible_target->type].is_caesar_legion_unit)) {
                    continue;
                }
                if (is_valid_target_for_enemy_unit(potential_target) && can_see_target(shooter, potential_target)) {
                    if (target_ranged_targeter_list_full(potential_target)) {
                        // prefer targets that aren't already aimed at to prevent overhit, but keep overhit targets as a second option
                        closest_eligible_overhit_target = potential_target;
                    } else {
                        closest_target_distance = potential_target_distance;
                        closest_eligible_target = potential_target;
                    }
                    continue;
                }
            }
        }
    }
    if (closest_eligible_target) {
        update_ranged_targeting(shooter, closest_eligible_target, tile);
        return 1;
    } else if (closest_eligible_overhit_target) {
        update_ranged_targeting(shooter, closest_eligible_overhit_target, tile);
        return 1;
    }

    return 0;
}

void clear_targeting_on_unit_death(struct figure_t *dead_unit)
{
    // remove unit from its target's targeter lists
    struct figure_t *target = &figures[dead_unit->target_figure_id];
    figure__remove_melee_targeter_from_list(target, dead_unit);
    figure__remove_ranged_targeter_from_list(target, dead_unit);

    // reset target of all opponents targeting the unit; remove unit from melee combatant list of all opponents fighting it
    for (int i = 0; i < MAX_FIGURES; i++) {
        struct figure_t *opponent = &figures[i];
        if (figure_is_alive(opponent)) {
            if (opponent->target_figure_id == dead_unit->id) {
                opponent->target_figure_id = 0;
            }
            for (int j = 0; j < MAX_MELEE_COMBATANTS_PER_UNIT; j++) {
                if (opponent->melee_combatant_ids[j] == dead_unit->id) {
                    opponent->melee_combatant_ids[j] = 0;
                    opponent->num_melee_combatants--;
                }
            }
        }
    }
}
