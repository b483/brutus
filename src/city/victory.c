#include "victory.h"

#include "building/construction.h"
#include "city/data_private.h"
#include "city/figures.h"
#include "city/finance.h"
#include "city/message.h"
#include "core/config.h"
#include "game/time.h"
#include "scenario/data.h"
#include "sound/music.h"
#include "window/mission_end.h"
#include "window/victory_dialog.h"

static struct {
    int state;
    int force_win;
} data;

void city_victory_reset(void)
{
    data.state = VICTORY_STATE_NONE;
    data.force_win = 0;
}

void city_victory_force_win(void)
{
    data.force_win = 1;
}

int city_victory_state(void)
{
    return data.state;
}

static int determine_victory_state(void)
{
    int state = VICTORY_STATE_WON;
    int has_criteria = 0;

    if (scenario.culture_win_criteria.enabled) {
        has_criteria = 1;
        if (city_data.ratings.culture < scenario.culture_win_criteria.goal) {
            state = VICTORY_STATE_NONE;
        }
    }
    if (scenario.prosperity_win_criteria.enabled) {
        has_criteria = 1;
        if (city_data.ratings.prosperity < scenario.prosperity_win_criteria.goal) {
            state = VICTORY_STATE_NONE;
        }
    }
    if (scenario.peace_win_criteria.enabled) {
        has_criteria = 1;
        if (city_data.ratings.peace < scenario.peace_win_criteria.goal) {
            state = VICTORY_STATE_NONE;
        }
    }
    if (scenario.favor_win_criteria.enabled) {
        has_criteria = 1;
        if (city_data.ratings.favor < scenario.favor_win_criteria.goal) {
            state = VICTORY_STATE_NONE;
        }
    }
    if (scenario.population_win_criteria.enabled) {
        has_criteria = 1;
        if (city_data.population.population < scenario.population_win_criteria.goal) {
            state = VICTORY_STATE_NONE;
        }
    }

    if (!has_criteria) {
        state = VICTORY_STATE_NONE;
    }
    // Bug: the survival time only works if no other criteria have been set
    if (!has_criteria) {
        if (scenario.time_limit_win_criteria.enabled || scenario.survival_time_win_criteria.enabled) {
            has_criteria = 1;
        }
    }

    if (scenario.time_limit_win_criteria.enabled && (game_time_year() >= scenario.start_year + scenario.time_limit_win_criteria.years)) {
        state = VICTORY_STATE_LOST;
    } else if (scenario.survival_time_win_criteria.enabled && (game_time_year() >= scenario.start_year + scenario.survival_time_win_criteria.years)) {
        state = VICTORY_STATE_WON;
    }

    if (city_figures_total_invading_enemies() > 2 + city_data.figure.soldiers) {
        if (city_data.population.population < city_data.population.highest_ever / 4) {
            state = VICTORY_STATE_LOST;
        }
    }
    if (city_figures_total_invading_enemies() > 0) {
        if (city_data.population.population <= 0) {
            state = VICTORY_STATE_LOST;
        }
    }
    if (!has_criteria) {
        state = VICTORY_STATE_NONE;
    }
    return state;
}

void city_victory_check(void)
{
    if (scenario.is_open_play) {
        return;
    }
    data.state = determine_victory_state();

    if (city_data.mission.has_won) {
        data.state = city_data.mission.continue_months_left <= 0 ? VICTORY_STATE_WON : VICTORY_STATE_NONE;
    }
    if (data.force_win) {
        data.state = VICTORY_STATE_WON;
    }
    if (data.state != VICTORY_STATE_NONE) {
        building_construction_clear_type();
        if (data.state == VICTORY_STATE_LOST) {
            if (city_data.mission.fired_message_shown) {
                window_mission_end_show_fired();
            } else {
                city_data.mission.fired_message_shown = 1;
                city_message_post(1, MESSAGE_FIRED, 0, 0);
            }
            data.force_win = 0;
        } else if (data.state == VICTORY_STATE_WON) {
            sound_music_stop();
            if (city_data.mission.victory_message_shown) {
                window_mission_end_show_won();
                data.force_win = 0;
            } else {
                city_data.mission.victory_message_shown = 1;
                window_victory_dialog_show();
            }
        }
    }
}

void city_victory_update_months_to_govern(void)
{
    if (city_data.mission.has_won) {
        city_data.mission.continue_months_left--;
    }
}

void city_victory_continue_governing(int months)
{
    city_data.mission.has_won = 1;
    city_data.mission.continue_months_left += months;
    city_data.mission.continue_months_chosen = months;
    city_data.emperor.salary_rank = 0;
    city_data.emperor.salary_amount = 0;
    city_finance_update_salary();
}

void city_victory_stop_governing(void)
{
    city_data.mission.has_won = 0;
    city_data.mission.continue_months_left = 0;
    city_data.mission.continue_months_chosen = 0;
}
