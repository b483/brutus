#include "combat.h"

#include "city/data_private.h"
#include "core/calc.h"
#include "figure/formation.h"
#include "figure/movement.h"
#include "figure/properties.h"
#include "figure/route.h"
#include "figure/sound.h"
#include "map/figure.h"
#include "sound/effect.h"

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

    const figure_properties *props = figure_properties_for_type(f->type);
    const figure_properties *opponent_props = figure_properties_for_type(opponent->type);
    if (opponent_props->category == FIGURE_CATEGORY_CITIZEN || opponent_props->category == FIGURE_CATEGORY_CRIMINAL) {
        f->attack_image_offset = 12;
    } else {
        f->attack_image_offset = 0;
    }
    int figure_attack = props->attack_value;
    int opponent_defense = opponent_props->defense_value;

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

    int max_damage = opponent_props->max_damage;
    int net_attack = figure_attack - opponent_defense;
    if (net_attack < 0) {
        net_attack = 0;
    }
    opponent->damage += net_attack;
    if (opponent->damage <= max_damage) {
        figure_play_hit_sound(f->type);
    } else {
        opponent->action_state = FIGURE_ACTION_149_CORPSE;
        opponent->wait_ticks = 0;
        figure_play_die_sound(opponent);
        formation_update_morale_after_death(opponent_formation);
    }
}

void figure_combat_handle_attack(figure *f)
{
    figure_movement_advance_attack(f);

    // remove dead opponent from figure's targeter and melee attacker lists
    figure *opponent = figure_get(f->primary_melee_combatant_id);
    if (opponent->action_state == FIGURE_ACTION_149_CORPSE) {
        for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
            if (f->targeter_ids[i] == opponent->id) {
                f->targeter_ids[i] = 0;
                f->num_melee_targeters--;
                break;
            }
        }
        for (int i = 0; i < MAX_MELEE_COMBATANTS_PER_UNIT; i++) {
            if (f->melee_combatant_ids[i] == opponent->id) {
                f->melee_combatant_ids[i] = 0;
                f->num_melee_combatants--;
                break;
            }
        }
    }

    if (f->num_melee_combatants) {
        f->attack_image_offset++;
        if (f->attack_image_offset >= 24) {
            // select next target from list
            for (int i = 0; i < MAX_MELEE_COMBATANTS_PER_UNIT; i++) {
                if (f->melee_combatant_ids[i]) {
                    f->primary_melee_combatant_id = f->melee_combatant_ids[i];
                    f->target_figure_id = f->melee_combatant_ids[i];
                    break;
                }
            }
            hit_opponent(f);
        }
    } else {
        f->action_state = f->action_state_before_attack;
        f->target_figure_id = 0;
        f->primary_melee_combatant_id = 0;
        figure_route_remove(f);
    }
    return;
}

static int set_targeter_for_target(figure *targeter, figure *target)
{
    for (int i = 0; i < MAX_MELEE_TARGETERS_PER_UNIT; i++) {
        if (target->targeter_ids[i] == targeter->id) {
            return 1;
        } else if (!target->targeter_ids[i]) {
            target->targeter_ids[i] = targeter->id;
            target->num_melee_targeters++;
            return 1;
        };
    }
    return 0;
}

figure *set_closest_eligible_target(figure *f)
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
        if (figure_is_dead(potential_target)) {
            continue;
        }
        int potential_target_distance = calc_maximum_distance(f->x, f->y, potential_target->x, potential_target->y);
        if (potential_target_distance < closest_target_distance) {
            // skip potential enemy target if its targeter list is full, unless the current targeter is already in
            // this prevents swarming while also keeping target eligible
            if (potential_target->num_melee_targeters >= MAX_MELEE_TARGETERS_PER_UNIT) {
                for (int j = 0; j < MAX_MELEE_TARGETERS_PER_UNIT; j++) {
                    if (potential_target->targeter_ids[j] == f->id) {
                        closest_target_distance = potential_target_distance;
                        closest_eligible_target = potential_target;
                        break;
                    }
                }
                continue;
            }
            int targeter_figure_category = figure_properties_for_type(f->type)->category;
            int potential_target_figure_category = figure_properties_for_type(potential_target->type)->category;
            if (targeter_figure_category == FIGURE_CATEGORY_ARMED) {
                if (potential_target_figure_category == FIGURE_CATEGORY_HOSTILE
                || potential_target_figure_category == FIGURE_CRIMINAL
                || (potential_target->type == FIGURE_INDIGENOUS_NATIVE && potential_target->action_state == FIGURE_ACTION_159_NATIVE_ATTACKING)) {
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                }
            } else if (f->type == FIGURE_WOLF) {
                switch (potential_target->type) {
                    case FIGURE_EXPLOSION:
                    case FIGURE_FORT_STANDARD:
                    case FIGURE_TRADE_SHIP:
                    case FIGURE_FISHING_BOAT:
                    case FIGURE_MAP_FLAG:
                    case FIGURE_FLOTSAM:
                    case FIGURE_SHIPWRECK:
                    case FIGURE_TOWER_SENTRY:
                    case FIGURE_ARROW:
                    case FIGURE_JAVELIN:
                    case FIGURE_BOLT:
                    case FIGURE_BALLISTA:
                    case FIGURE_CREATURE:
                    case FIGURE_WOLF:
                        continue;
                }
                closest_target_distance = potential_target_distance;
                closest_eligible_target = potential_target;
            } else {
                // targeter is enemy unit
                if ((potential_target_figure_category == FIGURE_CATEGORY_ARMED && potential_target->type != FIGURE_TOWER_SENTRY)
                || potential_target->type == FIGURE_WOLF
                || potential_target->type == FIGURE_NATIVE_TRADER
                || (potential_target->type == FIGURE_INDIGENOUS_NATIVE && potential_target->action_state != FIGURE_ACTION_159_NATIVE_ATTACKING)
                ) {
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                }
            }
        }
    }
    // set target and destination for figure 
    if (closest_eligible_target) {
        f->target_figure_id = closest_eligible_target->id;
        f->destination_x = closest_eligible_target->x;
        f->destination_y = closest_eligible_target->y;
        figure_route_remove(f);
        set_targeter_for_target(f, closest_eligible_target);
    }

    return closest_eligible_target;
}

int get_missile_target(figure *shooter, int max_range, map_point *tile)
{
    int closest_target_distance = max_range;
    figure *closest_eligible_target = 0;
    for (int i = 1; i < MAX_FIGURES; i++) {
        figure *potential_target = figure_get(i);
        if (figure_is_dead(potential_target)) {
            continue;
        }
        int potential_target_distance = calc_maximum_distance(shooter->x, shooter->y, potential_target->x, potential_target->y);
        if (potential_target_distance < closest_target_distance && figure_movement_can_launch_cross_country_missile(shooter->x, shooter->y, potential_target->x, potential_target->y)) {
            int potential_target_figure_category = figure_properties_for_type(potential_target->type)->category;
            if (shooter->is_friendly) {
                if (potential_target_figure_category == FIGURE_CATEGORY_HOSTILE
                || potential_target_figure_category == FIGURE_CATEGORY_CRIMINAL
                || (potential_target->type == FIGURE_INDIGENOUS_NATIVE && potential_target->action_state == FIGURE_ACTION_159_NATIVE_ATTACKING)) {
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;
                }
            } else {
                // shooter is enemy unit
                switch (potential_target->type) {
                    case FIGURE_EXPLOSION:
                    case FIGURE_FORT_STANDARD:
                    case FIGURE_MAP_FLAG:
                    case FIGURE_FLOTSAM:
                    case FIGURE_INDIGENOUS_NATIVE:
                    case FIGURE_ARROW:
                    case FIGURE_JAVELIN:
                    case FIGURE_BOLT:
                    case FIGURE_BALLISTA:
                    case FIGURE_CREATURE:
                    case FIGURE_FISH_GULLS:
                    case FIGURE_SHIPWRECK:
                    case FIGURE_SHEEP:
                    case FIGURE_ZEBRA:
                    case FIGURE_SPEAR:
                        continue;
                }
                if ((potential_target_figure_category == FIGURE_CATEGORY_ARMED && potential_target->type != FIGURE_TOWER_SENTRY)
                || potential_target->type == FIGURE_WOLF
                || potential_target_figure_category == FIGURE_CATEGORY_CITIZEN
                // target only trader natives so as to not get stuck in place shooting while fighters respawn
                || potential_target->type == FIGURE_NATIVE_TRADER) {
                    // skip (closer) civilian/native trader if defender or wolf an eligible target
                    if ((potential_target_figure_category == FIGURE_CATEGORY_CITIZEN || potential_target->type == FIGURE_NATIVE_TRADER)
                    && closest_eligible_target
                    && (figure_properties_for_type(closest_eligible_target->type)->category == FIGURE_CATEGORY_ARMED || closest_eligible_target->type == FIGURE_WOLF)) {
                        continue;
                    }
                    closest_target_distance = potential_target_distance;
                    closest_eligible_target = potential_target;

                }
            }
        }
    }
    if (closest_eligible_target) {
        map_point_store_result(closest_eligible_target->x, closest_eligible_target->y, tile);
        return closest_eligible_target->id;
    }
    return 0;
}

void figure_combat_attack_figure_at(figure *f, int grid_offset)
{
    int figure_category = figure_properties_for_type(f->type)->category;
    if (figure_category <= FIGURE_CATEGORY_INACTIVE || figure_category >= FIGURE_CATEGORY_CRIMINAL || f->action_state == FIGURE_ACTION_150_ATTACK) {
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

        int opponent_category = figure_properties_for_type(opponent->type)->category;
        int attack = 0;
        if (opponent->state != FIGURE_STATE_ALIVE) {
            attack = 0;
        } else if (opponent->action_state == FIGURE_ACTION_149_CORPSE) {
            attack = 0;
        } else if (figure_category == FIGURE_CATEGORY_ARMED && opponent_category == FIGURE_CATEGORY_HOSTILE) {
            attack = 1;
        } else if (figure_category == FIGURE_CATEGORY_ARMED && opponent_category == FIGURE_CATEGORY_CRIMINAL) {
            attack = 1;
        } else if (figure_category == FIGURE_CATEGORY_ARMED && opponent_category == FIGURE_CATEGORY_ANIMAL) {
            attack = 1;
        } else if (figure_category == FIGURE_CATEGORY_ARMED && opponent_category == FIGURE_CATEGORY_NATIVE) {
            if (opponent->action_state == FIGURE_ACTION_159_NATIVE_ATTACKING) {
                attack = 1;
            }
        } else if (figure_category == FIGURE_CATEGORY_HOSTILE && opponent_category == FIGURE_CATEGORY_CITIZEN) {
            attack = 1;
        } else if (figure_category == FIGURE_CATEGORY_HOSTILE && opponent_category == FIGURE_CATEGORY_ARMED) {
            attack = 1;
        } else if (figure_category == FIGURE_CATEGORY_HOSTILE && opponent_category == FIGURE_CATEGORY_CRIMINAL) {
            attack = 1;
        } else if (figure_category == FIGURE_CATEGORY_HOSTILE && opponent_category == FIGURE_CATEGORY_NATIVE) {
            // wolves attack natives
            if (f->type == FIGURE_WOLF) {
                attack = 1;
            }
            // other hostiles (i.e. invaders) only fight with natives that aren't attacking the player
            else if (opponent->action_state != FIGURE_ACTION_159_NATIVE_ATTACKING) {
                attack = 1;
            }
        } else if (figure_category == FIGURE_CATEGORY_HOSTILE && opponent_category == FIGURE_CATEGORY_ANIMAL) {
            attack = 1;
        } else if (figure_category == FIGURE_CATEGORY_HOSTILE && opponent_category == FIGURE_CATEGORY_HOSTILE) {
            // wolves attack every hostile unit not a wolf
            if (f->type == FIGURE_WOLF && opponent->type != FIGURE_WOLF) {
                attack = 1;
            } else if
                // invaders and caesar's legions attack each other
                ((!figure_is_caesar_legion(f) && figure_is_caesar_legion(opponent))
                || (figure_is_caesar_legion(f) && !figure_is_caesar_legion(opponent))) {
                attack = 1;
            }
        }

        if (opponent->action_state == FIGURE_ACTION_150_ATTACK && opponent->num_melee_combatants >= MAX_MELEE_COMBATANTS_PER_UNIT) {
            attack = 0;
        }

        if (attack) {
            f->action_state_before_attack = f->action_state;
            f->action_state = FIGURE_ACTION_150_ATTACK;

            // if attacker's target list full but opponent not in it, overwrite first entry
            if (!set_targeter_for_target(opponent, f)) {
                f->targeter_ids[0] = opponent->id;
            }

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

            set_targeter_for_target(f, opponent);

            return;
        }
        primary_melee_combatant_id = opponent->next_figure_id_on_same_tile;
    }
}
