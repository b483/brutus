#include "military.h"

#include "building/menu.h"
#include "city/buildings.h"
#include "city/data_private.h"
#include "city/message.h"
#include "city/ratings.h"
#include "core/calc.h"
#include "empire/object.h"
#include "figure/figure.h"
#include "figure/formation.h"
#include "figure/formation_legion.h"
#include "scenario/data.h"
#include "scenario/editor_events.h"

void city_military_determine_distant_battle_city(void)
{
    for (int i = 0; i < MAX_OBJECTS; i++) {
        if (empire_objects[i].in_use && empire_objects[i].city_type == EMPIRE_CITY_VULNERABLE_ROMAN) {
            city_data.distant_battle.city = i;
        }
    }
}

int city_military_distant_battle_roman_army_is_traveling(void)
{
    return city_data.distant_battle.roman_months_to_travel_forth > 0 ||
        city_data.distant_battle.roman_months_to_travel_back > 0;
}

int city_military_has_distant_battle(void)
{
    return city_data.distant_battle.months_until_battle > 0 ||
        city_data.distant_battle.roman_months_to_travel_back > 0 ||
        city_data.distant_battle.roman_months_to_travel_forth > 0 ||
        city_data.distant_battle.city_foreign_months_left > 0;
}

void city_military_init_distant_battle(int enemy_strength)
{
    city_data.distant_battle.enemy_months_traveled = 1;
    city_data.distant_battle.roman_months_traveled = 1;
    city_data.distant_battle.months_until_battle = 24;
    city_data.distant_battle.enemy_strength = enemy_strength;
    city_data.distant_battle.total_count++;
    city_data.distant_battle.roman_months_to_travel_back = 0;
    city_data.distant_battle.roman_months_to_travel_forth = 0;
}

static void update_time_traveled(void)
{
    if (city_data.distant_battle.months_until_battle < scenario.empire.distant_battle_enemy_travel_months) {
        city_data.distant_battle.enemy_months_traveled =
            scenario.empire.distant_battle_enemy_travel_months - city_data.distant_battle.months_until_battle + 1;
    } else {
        city_data.distant_battle.enemy_months_traveled = 1;
    }
    if (city_data.distant_battle.roman_months_to_travel_forth >= 1) {
        if (scenario.empire.distant_battle_roman_travel_months - city_data.distant_battle.roman_months_traveled >
            scenario.empire.distant_battle_enemy_travel_months - city_data.distant_battle.enemy_months_traveled) {
            city_data.distant_battle.roman_months_to_travel_forth -= 2;
        } else {
            city_data.distant_battle.roman_months_to_travel_forth--;
        }
        if (city_data.distant_battle.roman_months_to_travel_forth <= 1) {
            city_data.distant_battle.roman_months_to_travel_forth = 1;
        }
        city_data.distant_battle.roman_months_traveled =
            scenario.empire.distant_battle_roman_travel_months - city_data.distant_battle.roman_months_to_travel_forth + 1;
        if (city_data.distant_battle.roman_months_traveled < 1) {
            city_data.distant_battle.roman_months_traveled = 1;
        }
        if (city_data.distant_battle.roman_months_traveled > scenario.empire.distant_battle_roman_travel_months) {
            city_data.distant_battle.roman_months_traveled = scenario.empire.distant_battle_roman_travel_months;
        }
    }
}

static void set_city_foreign(void)
{
    if (city_data.distant_battle.city) {
        empire_objects[city_data.distant_battle.city].city_type = EMPIRE_CITY_DISTANT_FOREIGN;
    }
    city_data.distant_battle.city_foreign_months_left = 24;
}

static int player_has_won(void)
{
    int won;
    int pct_loss;
    if (city_data.distant_battle.roman_strength < city_data.distant_battle.enemy_strength) {
        won = 0;
        pct_loss = 100;
    } else {
        won = 1;
        int pct_advantage = calc_percentage(
            city_data.distant_battle.roman_strength - city_data.distant_battle.enemy_strength,
            city_data.distant_battle.roman_strength);
        if (pct_advantage < 10) {
            pct_loss = 70;
        } else if (pct_advantage < 25) {
            pct_loss = 50;
        } else if (pct_advantage < 50) {
            pct_loss = 25;
        } else if (pct_advantage < 75) {
            pct_loss = 15;
        } else if (pct_advantage < 100) {
            pct_loss = 10;
        } else if (pct_advantage < 150) {
            pct_loss = 5;
        } else {
            pct_loss = 0;
        }
    }
    // apply legion losses
    for (int i = 0; i < MAX_LEGIONS; i++) {
        if (legion_formations[i].in_use && legion_formations[i].in_distant_battle) {
            struct formation_t *m = &legion_formations[i];
            m->morale = calc_bound(m->morale - 75, 0, m->max_morale);
            int soldiers_total = 0;
            for (int fig = 0; fig < m->num_figures; fig++) {
                if (m->figures[fig] > 0) {
                    struct figure_t *f = &figures[m->figures[fig]];
                    if (figure_is_alive(f)) {
                        soldiers_total++;
                    }
                }
            }
            int soldiers_to_kill = calc_adjust_with_percentage(soldiers_total, pct_loss);
            if (soldiers_to_kill >= soldiers_total) {
                m->in_distant_battle = 0;
            }
            for (int fig = 0; fig < m->num_figures; fig++) {
                if (m->figures[fig] > 0) {
                    struct figure_t *f = &figures[m->figures[fig]];
                    if (figure_is_alive(f)) {
                        if (soldiers_to_kill) {
                            soldiers_to_kill--;
                            figure_delete(f);
                        }
                    }
                }
            }
        }
    }
    return won;
}

static void fight_distant_battle(void)
{
    if (city_data.distant_battle.roman_months_to_travel_forth <= 0) {
        city_message_post(1, MESSAGE_DISTANT_BATTLE_LOST_NO_TROOPS, 0, 0);
        city_ratings_change_favor(-50);
        set_city_foreign();
    } else if (city_data.distant_battle.roman_months_to_travel_forth > 2) {
        city_message_post(1, MESSAGE_DISTANT_BATTLE_LOST_TOO_LATE, 0, 0);
        city_ratings_change_favor(-25);
        set_city_foreign();
        city_data.distant_battle.roman_months_to_travel_back = city_data.distant_battle.roman_months_traveled;
    } else if (!player_has_won()) {
        city_message_post(1, MESSAGE_DISTANT_BATTLE_LOST_TOO_WEAK, 0, 0);
        city_ratings_change_favor(-10);
        set_city_foreign();
        city_data.distant_battle.roman_months_traveled = 0;
        // no return: all soldiers killed
    } else {
        if (scenario.allowed_buildings[BUILDING_TRIUMPHAL_ARCH]) {
            city_message_post(1, MESSAGE_DISTANT_BATTLE_WON, 0, 0);
            city_data.building.triumphal_arches_available++;
            building_menu_update();
        } else {
            city_message_post(1, MESSAGE_DISTANT_BATTLE_WON_TRIUMPHAL_ARCH_DISABLED, 0, 0);
        }
        city_ratings_change_favor(25);
        city_data.distant_battle.won_count++;
        city_data.distant_battle.city_foreign_months_left = 0;
        city_data.distant_battle.roman_months_to_travel_back = city_data.distant_battle.roman_months_traveled;
    }
    city_data.distant_battle.months_until_battle = 0;
    city_data.distant_battle.enemy_months_traveled = 0;
    city_data.distant_battle.roman_months_to_travel_forth = 0;
}

static void update_aftermath(void)
{
    if (city_data.distant_battle.roman_months_to_travel_back > 0) {
        city_data.distant_battle.roman_months_to_travel_back--;
        city_data.distant_battle.roman_months_traveled = city_data.distant_battle.roman_months_to_travel_back;
        if (city_data.distant_battle.roman_months_to_travel_back <= 0) {
            if (city_data.distant_battle.city_foreign_months_left) {
                // soldiers return - not in time
                city_message_post(1, MESSAGE_TROOPS_RETURN_FAILED, 0, city_data.map.exit_point.grid_offset);
            } else {
                // victorious
                city_message_post(1, MESSAGE_TROOPS_RETURN_VICTORIOUS, 0, city_data.map.exit_point.grid_offset);
            }
            city_data.distant_battle.roman_months_traveled = 0;
            // return soldiers
            for (int i = 0; i < MAX_LEGIONS; i++) {
                if (legion_formations[i].in_use && legion_formations[i].in_distant_battle) {
                    legion_formations[i].in_distant_battle = 0;
                    for (int fig = 0; fig < legion_formations[i].num_figures; fig++) {
                        if (legion_formations[i].figures[fig] > 0) {
                            struct figure_t *f = &figures[legion_formations[i].figures[fig]];
                            if (figure_is_alive(f)) {
                                f->action_state = FIGURE_ACTION_SOLDIER_RETURNING_FROM_DISTANT_BATTLE;
                            }
                        }
                    }
                }
            }
        }
    } else if (city_data.distant_battle.city_foreign_months_left > 0) {
        city_data.distant_battle.city_foreign_months_left--;
        if (city_data.distant_battle.city_foreign_months_left <= 0) {
            city_message_post(1, MESSAGE_DISTANT_BATTLE_CITY_RETAKEN, 0, 0);
            if (city_data.distant_battle.city) {
                empire_objects[city_data.distant_battle.city].city_type = EMPIRE_CITY_VULNERABLE_ROMAN;
            }
        }
    }
}

void city_military_process_distant_battle(void)
{
    if (city_data.distant_battle.months_until_battle > 0) {
        --city_data.distant_battle.months_until_battle;
        if (city_data.distant_battle.months_until_battle > 0) {
            update_time_traveled();
        } else {
            fight_distant_battle();
        }
    } else {
        update_aftermath();
    }
}
