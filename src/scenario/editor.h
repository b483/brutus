#ifndef SCENARIO_EDITOR_H
#define SCENARIO_EDITOR_H

#include <stdint.h>

void scenario_editor_create(int map_size);

void scenario_editor_set_native_images(int image_hut, int image_meeting, int image_crops);

void scenario_editor_sort_requests(void);

void scenario_editor_sort_invasions(void);

void scenario_editor_sort_price_changes(void);

void scenario_editor_sort_demand_changes(void);

void scenario_editor_cycle_image(int forward);

void scenario_editor_cycle_climate(void);

void scenario_editor_update_briefing(const uint8_t *new_briefing);

void scenario_editor_update_brief_description(const uint8_t *new_description);

void scenario_editor_set_enemy(int enemy_id);

void scenario_editor_change_empire(int change);

int scenario_editor_is_building_allowed(int id);

void scenario_editor_toggle_building_allowed(int id);

void scenario_editor_set_player_rank(int rank);

void scenario_editor_set_initial_favor(int amount);

void scenario_editor_set_initial_funds(int amount);
void scenario_editor_set_rescue_loan(int amount);
void scenario_editor_set_initial_personal_savings(int amount);

void scenario_editor_toggle_rome_supplies_wheat(void);

void scenario_editor_toggle_flotsam(void);

int scenario_editor_milestone_year(int milestone_percentage);
void scenario_editor_set_milestone_year(int milestone_percentage, int year);

void scenario_editor_set_start_year(int year);

void scenario_editor_toggle_open_play(void);

void scenario_editor_toggle_culture(void);
void scenario_editor_set_culture(int goal);
void scenario_editor_toggle_prosperity(void);
void scenario_editor_set_prosperity(int goal);
void scenario_editor_toggle_peace(void);
void scenario_editor_set_peace(int goal);
void scenario_editor_toggle_favor(void);
void scenario_editor_set_favor(int goal);

void scenario_editor_toggle_population(void);
void scenario_editor_set_population(int goal);

void scenario_editor_toggle_time_limit(void);
void scenario_editor_set_time_limit(int years);
void scenario_editor_toggle_survival_time(void);
void scenario_editor_set_survival_time(int years);

#endif // SCENARIO_EDITOR_H
