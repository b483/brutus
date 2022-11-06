#include "editor_events.h"

#include "city/message.h"
#include "game/time.h"
#include "scenario/data.h"

void scenario_editor_earthquake_cycle_severity(void)
{
    scenario.earthquake.severity++;
    if (scenario.earthquake.severity > EARTHQUAKE_LARGE) {
        scenario.earthquake.severity = EARTHQUAKE_NONE;
    }
    scenario.is_saved = 0;
}

void scenario_editor_earthquake_set_year(int year)
{
    scenario.earthquake.year = year;
    scenario.is_saved = 0;
}

void scenario_editor_gladiator_revolt_toggle_enabled(void)
{
    scenario.gladiator_revolt.enabled = !scenario.gladiator_revolt.enabled;
    scenario.is_saved = 0;
}

void scenario_editor_gladiator_revolt_set_year(int year)
{
    scenario.gladiator_revolt.year = year;
    scenario.is_saved = 0;
}

void scenario_editor_emperor_change_toggle_enabled(void)
{
    scenario.emperor_change.enabled = !scenario.emperor_change.enabled;
    scenario.is_saved = 0;
}

void scenario_editor_emperor_change_set_year(int year)
{
    scenario.emperor_change.year = year;
    scenario.is_saved = 0;
}

void scenario_editor_sea_trade_problem_toggle_enabled(void)
{
    scenario.random_events.sea_trade_problem = !scenario.random_events.sea_trade_problem;
    scenario.is_saved = 0;
}

void scenario_editor_land_trade_problem_toggle_enabled(void)
{
    scenario.random_events.land_trade_problem = !scenario.random_events.land_trade_problem;
    scenario.is_saved = 0;
}

void scenario_editor_raise_wages_toggle_enabled(void)
{
    scenario.random_events.raise_wages = !scenario.random_events.raise_wages;
    scenario.is_saved = 0;
}

void scenario_editor_lower_wages_toggle_enabled(void)
{
    scenario.random_events.lower_wages = !scenario.random_events.lower_wages;
    scenario.is_saved = 0;
}

void scenario_editor_contaminated_water_toggle_enabled(void)
{
    scenario.random_events.contaminated_water = !scenario.random_events.contaminated_water;
    scenario.is_saved = 0;
}

void scenario_custom_messages_process(void)
{
    for (int i = 0; i < MAX_EDITOR_CUSTOM_MESSAGES; i++) {
        if (!scenario.editor_custom_messages[i].enabled) {
            continue;
        }
        if (game_time_year() == scenario.editor_custom_messages[i].year + scenario.start_year
            && game_time_month() == scenario.editor_custom_messages[i].month) {
            city_message_post(1, 123 + i, 0, 0);
        }
    }
}