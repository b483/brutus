#include "emperor.h"

#include "city/data_private.h"
#include "city/finance.h"
#include "city/message.h"
#include "city/ratings.h"
#include "core/calc.h"
#include "figure/formation_enemy.h"
#include "game/time.h"
#include "scenario/data.h"
#include "scenario/editor_events.h"

static const int SALARY_FOR_RANK[11] = { 1, 2, 3, 5, 8, 13, 21, 34, 55, 89, 144 };

void city_emperor_init_scenario(void)
{
    city_data.ratings.favor = scenario.initial_favor;
    city_data.emperor.personal_savings = scenario.initial_personal_savings;
    city_data.emperor.player_rank = scenario.player_rank;
    city_emperor_set_salary_rank(city_data.emperor.player_rank);
}

void update_debt_state(void)
{
    if (city_data.finance.treasury >= 0) {
        city_data.emperor.months_in_debt = -1;
        return;
    }
    if (city_data.emperor.debt_state == 0) {
        // provide bailout
        int rescue_loan = scenario.rescue_loan;
        city_finance_process_donation(rescue_loan);
        city_finance_calculate_totals();

        city_data.emperor.debt_state = 1;
        city_data.emperor.months_in_debt = 0;
        city_message_post(1, MESSAGE_CITY_IN_DEBT, 0, 0);
        city_ratings_reduce_prosperity_after_bailout();
    } else if (city_data.emperor.debt_state == 1) {
        city_data.emperor.debt_state = 2;
        city_data.emperor.months_in_debt = 0;
        city_message_post(1, MESSAGE_CITY_IN_DEBT_AGAIN, 0, 0);
        city_ratings_change_favor(-5);
    } else if (city_data.emperor.debt_state == 2) {
        if (city_data.emperor.months_in_debt == -1) {
            city_message_post(1, MESSAGE_CITY_IN_DEBT_AGAIN, 0, 0);
            city_data.emperor.months_in_debt = 0;
        }
        if (game_time_day() == 0) {
            city_data.emperor.months_in_debt++;
        }
        if (city_data.emperor.months_in_debt >= 12) {
            city_data.emperor.debt_state = 3;
            city_data.emperor.months_in_debt = 0;
            if (!city_data.figure.imperial_soldiers) {
                city_message_post(1, MESSAGE_CITY_STILL_IN_DEBT, 0, 0);
                city_ratings_change_favor(-10);
            }
        }
    } else if (city_data.emperor.debt_state == 3) {
        if (city_data.emperor.months_in_debt == -1) {
            city_message_post(1, MESSAGE_CITY_STILL_IN_DEBT, 0, 0);
            city_data.emperor.months_in_debt = 0;
        }
        if (game_time_day() == 0) {
            city_data.emperor.months_in_debt++;
        }
        if (city_data.emperor.months_in_debt >= 12) {
            city_data.emperor.debt_state = 4;
            city_data.emperor.months_in_debt = 0;
            if (!city_data.figure.imperial_soldiers) {
                if (city_data.ratings.favor > 10) {
                    city_data.ratings.favor = 10;
                }
            }
        }
    }
}

void process_caesar_invasion(void)
{
    if (city_data.figure.imperial_soldiers && !city_data.emperor.invasion.from_editor) {
        // caesar invasion in progress
        city_data.emperor.invasion.duration_day_countdown--;
        if (city_data.ratings.favor >= 35 && city_data.emperor.invasion.duration_day_countdown < 176) {
            // pause legions
            for (int i = 0; i < MAX_ENEMY_FORMATIONS; i++) {
                if (enemy_formations[i].in_use == 1 && enemy_formations[i].figure_type == FIGURE_ENEMY_CAESAR_LEGIONARY) {
                    enemy_formations[i].wait_ticks_movement = 0;
                }
            }
        } else if (city_data.ratings.favor >= 22) {
            if (city_data.emperor.invasion.duration_day_countdown > 0) {
                // retreat
                for (int i = 0; i < MAX_ENEMY_FORMATIONS; i++) {
                    if (enemy_formations[i].in_use && enemy_formations[i].figure_type == FIGURE_ENEMY_CAESAR_LEGIONARY) {
                        enemy_formations[i].morale = 0;
                    }
                }
                if (!city_data.emperor.invasion.retreat_message_shown) {
                    city_data.emperor.invasion.retreat_message_shown = 1;
                    city_message_post(1, MESSAGE_CAESAR_ARMY_RETREAT, 0, 0);
                }
            } else if (city_data.emperor.invasion.duration_day_countdown == 0) {
                // a year has passed (11 months), siege goes on
                city_message_post(1, MESSAGE_CAESAR_ARMY_CONTINUE, 0, 0);
            }
        }
    } else if (city_data.emperor.invasion.soldiers_killed
        && city_data.emperor.invasion.soldiers_killed >= city_data.emperor.invasion.size) {
        // player defeated caesar army
        city_data.emperor.invasion.size = 0;
        city_data.emperor.invasion.soldiers_killed = 0;
        if (!city_data.emperor.invasion.from_editor) {
            if (city_data.ratings.favor < 35) {
                city_ratings_change_favor(10);
                if (city_data.emperor.invasion.count < 2) {
                    city_message_post(1, MESSAGE_CAESAR_RESPECT_1, 0, 0);
                } else if (city_data.emperor.invasion.count < 3) {
                    city_message_post(1, MESSAGE_CAESAR_RESPECT_2, 0, 0);
                } else {
                    city_message_post(1, MESSAGE_CAESAR_RESPECT_3, 0, 0);
                }
            }
        }
        if (city_data.emperor.invasion.from_editor) {
            city_data.emperor.invasion.from_editor = 0;
        }
    } else if (city_data.emperor.invasion.days_until_invasion <= 0) {
        if (city_data.ratings.favor <= 10) {
            // warn player that caesar is angry and will invade in a year
            city_data.emperor.invasion.warnings_given++;
            city_data.emperor.invasion.days_until_invasion = 192;
            if (city_data.emperor.invasion.warnings_given <= 1) {
                city_message_post(1, MESSAGE_CAESAR_WRATH, 0, 0);
            }
        }
    } else {
        city_data.emperor.invasion.days_until_invasion--;
        if (city_data.emperor.invasion.days_until_invasion == 0) {
            // invade!
            int size;
            if (city_data.emperor.invasion.count == 0) {
                size = 32;
            } else if (city_data.emperor.invasion.count == 1) {
                size = 64;
            } else if (city_data.emperor.invasion.count == 2) {
                size = 96;
            } else {
                size = 160;
            }
            if (scenario_invasion_start_from_caesar(size)) {
                city_data.emperor.invasion.count++;
                city_data.emperor.invasion.duration_day_countdown = 192;
                city_data.emperor.invasion.retreat_message_shown = 0;
                city_data.emperor.invasion.size = size;
                city_data.emperor.invasion.soldiers_killed = 0;
            }
        }
    }
}

void city_emperor_send_gift(void)
{
    int size = city_data.emperor.selected_gift_size;
    if (size < GIFT_MODEST || size > GIFT_LAVISH) {
        return;
    }
    int cost = city_data.emperor.gifts[size].cost;

    if (cost > city_data.emperor.personal_savings) {
        return;
    }

    if (city_data.emperor.gift_overdose_penalty <= 0) {
        city_data.emperor.gift_overdose_penalty = 1;
        if (size == GIFT_MODEST) {
            city_ratings_change_favor(3);
        } else if (size == GIFT_GENEROUS) {
            city_ratings_change_favor(5);
        } else if (size == GIFT_LAVISH) {
            city_ratings_change_favor(10);
        }
    } else if (city_data.emperor.gift_overdose_penalty == 1) {
        city_data.emperor.gift_overdose_penalty = 2;
        if (size == GIFT_MODEST) {
            city_ratings_change_favor(1);
        } else if (size == GIFT_GENEROUS) {
            city_ratings_change_favor(3);
        } else if (size == GIFT_LAVISH) {
            city_ratings_change_favor(5);
        }
    } else if (city_data.emperor.gift_overdose_penalty == 2) {
        city_data.emperor.gift_overdose_penalty = 3;
        if (size == GIFT_MODEST) {
            city_ratings_change_favor(0);
        } else if (size == GIFT_GENEROUS) {
            city_ratings_change_favor(1);
        } else if (size == GIFT_LAVISH) {
            city_ratings_change_favor(3);
        }
    } else if (city_data.emperor.gift_overdose_penalty == 3) {
        city_data.emperor.gift_overdose_penalty = 4;
        if (size == GIFT_MODEST) {
            city_ratings_change_favor(0);
        } else if (size == GIFT_GENEROUS) {
            city_ratings_change_favor(0);
        } else if (size == GIFT_LAVISH) {
            city_ratings_change_favor(1);
        }
    }

    city_data.emperor.months_since_gift = 0;
    // rotate gift type
    city_data.emperor.gifts[size].id++;
    if (city_data.emperor.gifts[size].id >= 4) {
        city_data.emperor.gifts[size].id = 0;
    }

    city_data.emperor.personal_savings -= cost;
}

int city_emperor_salary_for_rank(int rank)
{
    return SALARY_FOR_RANK[rank];
}

void city_emperor_set_salary_rank(int player_rank)
{
    city_data.emperor.salary_rank = player_rank;
    city_data.emperor.salary_amount = SALARY_FOR_RANK[player_rank];
}

