#ifndef SCENARIO_EDITOR_EVENTS_H
#define SCENARIO_EDITOR_EVENTS_H

void scenario_editor_earthquake_cycle_severity(void);
void scenario_editor_earthquake_set_year(int year);

void scenario_editor_gladiator_revolt_toggle_enabled(void);
void scenario_editor_gladiator_revolt_set_year(int year);

void scenario_editor_emperor_change_toggle_enabled(void);
void scenario_editor_emperor_change_set_year(int year);

void scenario_editor_sea_trade_problem_toggle_enabled(void);

void scenario_editor_land_trade_problem_toggle_enabled(void);

void scenario_editor_raise_wages_toggle_enabled(void);

void scenario_editor_lower_wages_toggle_enabled(void);

void scenario_editor_contaminated_water_toggle_enabled(void);

void scenario_custom_messages_process(void);

#endif // SCENARIO_EDITOR_EVENTS_H
