#include "criteria.h"

#include "scenario/data.h"

static int max_game_year;

int scenario_criteria_milestone_year(int percentage)
{
    switch (percentage) {
        case 25:
            return scenario.start_year + scenario.milestone25_year;
        case 50:
            return scenario.start_year + scenario.milestone50_year;
        case 75:
            return scenario.start_year + scenario.milestone75_year;
        default:
            return 0;
    }
}

void scenario_criteria_init_max_year(void)
{
    if (scenario.time_limit_win_criteria.enabled) {
        max_game_year = scenario.start_year + scenario.time_limit_win_criteria.years;
    } else if (scenario.survival_time_win_criteria.enabled) {
        max_game_year = scenario.start_year + scenario.survival_time_win_criteria.years;
    } else {
        max_game_year = 1000000 + scenario.start_year;
    }
}

int scenario_criteria_max_year(void)
{
    return max_game_year;
}

void scenario_criteria_save_state(buffer *buf)
{
    buffer_write_i32(buf, max_game_year);
}

void scenario_criteria_load_state(buffer *buf)
{
    max_game_year = buffer_read_i32(buf);
}
