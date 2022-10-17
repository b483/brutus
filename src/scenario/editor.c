#include "editor.h"

#include "core/lang.h"
#include "core/string.h"
#include "map/grid.h"
#include "scenario/data.h"
#include "scenario/property.h"

#include <string.h>

static const struct {
    int width;
    int height;
} MAP_SIZES[] = {
    {40, 40},
    {60, 60},
    {80, 80},
    {100, 100},
    {120, 120},
    {160, 160}
};

static void init_point(map_point *point)
{
    point->x = -1;
    point->y = -1;
}

void scenario_editor_create(int map_size)
{
    memset(&scenario, 0, sizeof(scenario));

    scenario.map.width = MAP_SIZES[map_size].width;
    scenario.map.height = MAP_SIZES[map_size].height;
    scenario.map.grid_border_size = GRID_SIZE - scenario.map.width;
    scenario.map.grid_start = (GRID_SIZE - scenario.map.height) / 2 * GRID_SIZE + (GRID_SIZE - scenario.map.width) / 2;

    // Map description/briefing
    string_copy(lang_get_string(44, 38), scenario.briefing, MAX_BRIEFING);

    // Brief description
    string_copy(lang_get_string(44, 37), scenario.brief_description, MAX_BRIEF_DESCRIPTION);

    // Starting conditions
    scenario.start_year = -500;
    scenario.initial_favor = 40;
    scenario.initial_funds = 1000;
    scenario.rescue_loan = 500;
    scenario.initial_personal_savings = 0;
    scenario.rome_supplies_wheat = 0;
    scenario.win_criteria.milestone25_year = 10;
    scenario.win_criteria.milestone50_year = 20;
    scenario.win_criteria.milestone75_year = 30;

    // Requests
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].year = 1;
        scenario.requests[i].amount = 1;
        scenario.requests[i].years_deadline = 5;
        scenario.requests[i].favor = 8;
    }

    // Invasions
    for (int i = 0; i < MAX_INVASIONS; i++) {
        scenario.invasions[i].year = 1;
        scenario.invasions[i].from = 8;
    }

    // Buildings allowed
    for (int i = 0; i < MAX_ALLOWED_BUILDINGS; i++) {
        scenario.allowed_buildings[i] = 1;
    }

    // Win criteria
    scenario.win_criteria.culture.enabled = 1;
    scenario.win_criteria.culture.goal = 10;
    scenario.win_criteria.prosperity.enabled = 1;
    scenario.win_criteria.prosperity.goal = 10;
    scenario.win_criteria.peace.enabled = 1;
    scenario.win_criteria.peace.goal = 10;
    scenario.win_criteria.favor.enabled = 1;
    scenario.win_criteria.favor.goal = 10;
    scenario.win_criteria.population.enabled = 0;
    scenario.win_criteria.population.goal = 0;
    scenario.win_criteria.time_limit.enabled = 0;
    scenario.win_criteria.time_limit.years = 0;
    scenario.win_criteria.survival_time.enabled = 0;
    scenario.win_criteria.survival_time.years = 0;

    scenario.earthquake.severity = 0;
    scenario.earthquake.year = 0;

    // Price changes
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        scenario.price_changes[i].year = 1;
        scenario.price_changes[i].amount = 1;
    }

    // Demand changes
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        scenario.demand_changes[i].year = 1;
    }

    init_point(&scenario.earthquake_point);
    for (int i = 0; i < MAX_INVASION_POINTS; i++) {
        init_point(&scenario.invasion_points[i]);
    }
    init_point(&scenario.entry_point);
    init_point(&scenario.exit_point);
    init_point(&scenario.river_entry_point);
    init_point(&scenario.river_exit_point);
    for (int i = 0; i < MAX_FISH_POINTS; i++) {
        init_point(&scenario.fishing_points[i]);
    }
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        init_point(&scenario.herd_points[i]);
    }

    scenario.is_saved = 1;
}

void scenario_editor_set_native_images(int image_hut, int image_meeting, int image_crops)
{
    scenario.native_images.hut = image_hut;
    scenario.native_images.meeting = image_meeting;
    scenario.native_images.crops = image_crops;
}

void scenario_editor_sort_requests(void)
{
    for (int i = 0; i < MAX_REQUESTS; i++) {
        for (int j = MAX_REQUESTS - 1; j > 0; j--) {
            request_t *current = &scenario.requests[j];
            request_t *prev = &scenario.requests[j - 1];
            if (current->resource) {
                // if no previous request scheduled, move current back until first; if previous request is later than current, swap
                if (!prev->resource || prev->year > current->year || (prev->year == current->year && prev->month > current->month)) {
                    request_t tmp = *current;
                    *current = *prev;
                    *prev = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

void scenario_editor_sort_invasions(void)
{
    for (int i = 0; i < MAX_INVASIONS; i++) {
        for (int j = MAX_INVASIONS - 1; j > 0; j--) {
            invasion_t *current = &scenario.invasions[j];
            invasion_t *prev = &scenario.invasions[j - 1];
            if (current->type) {
                // if no previous invasion scheduled, move current back until first; if previous invasion is later than current, swap
                if (!prev->type || prev->year > current->year || (prev->year == current->year && prev->month > current->month)) {
                    invasion_t tmp = *current;
                    *current = *prev;
                    *prev = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

void scenario_editor_sort_price_changes(void)
{
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        for (int j = MAX_PRICE_CHANGES - 1; j > 0; j--) {
            price_change_t *current = &scenario.price_changes[j];
            price_change_t *prev = &scenario.price_changes[j - 1];
            if (current->resource) {
                // if no previous price change scheduled, move current back until first; if previous price change is later than current, swap
                if (!prev->resource || prev->year > current->year || (prev->year == current->year && prev->month > current->month)) {
                    price_change_t tmp = *current;
                    *current = *prev;
                    *prev = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

void scenario_editor_sort_demand_changes(void)
{
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        for (int j = MAX_DEMAND_CHANGES - 1; j > 0; j--) {
            demand_change_t *current = &scenario.demand_changes[j];
            demand_change_t *prev = &scenario.demand_changes[j - 1];
            if (current->resource && current->route_id) {
                // if no previous demand change scheduled, move current back until first; if previous demand change is later than current, swap
                if (!prev->resource || !prev->route_id || prev->year > current->year || (prev->year == current->year && prev->month > current->month)) {
                    demand_change_t tmp = *current;
                    *current = *prev;
                    *prev = tmp;
                }
            }
        }
    }
    scenario.is_saved = 0;
}

void scenario_editor_cycle_image(int forward)
{
    if (forward) {
        scenario.image_id++;
    } else {
        scenario.image_id--;
    }
    if (scenario.image_id < 0) {
        scenario.image_id = 15;
    }
    if (scenario.image_id > 15) {
        scenario.image_id = 0;
    }
    scenario.is_saved = 0;
}

void scenario_editor_cycle_climate(void)
{
    switch (scenario.climate) {
        case CLIMATE_CENTRAL:
            scenario.climate = CLIMATE_NORTHERN;
            break;
        case CLIMATE_NORTHERN:
            scenario.climate = CLIMATE_DESERT;
            break;
        case CLIMATE_DESERT:
        default:
            scenario.climate = CLIMATE_CENTRAL;
            break;
    }
    scenario.is_saved = 0;
}

void scenario_editor_update_briefing(const uint8_t *new_briefing)
{
    if (!string_equals(scenario.briefing, new_briefing)) {
        string_copy(new_briefing, scenario.briefing, MAX_BRIEFING);
        scenario.is_saved = 0;
    }
}

void scenario_editor_update_brief_description(const uint8_t *new_description)
{
    if (!string_equals(scenario.brief_description, new_description)) {
        string_copy(new_description, scenario.brief_description, MAX_BRIEF_DESCRIPTION);
        scenario.is_saved = 0;
    }
}

void scenario_editor_set_enemy(int enemy_id)
{
    scenario.enemy_id = enemy_id;
    scenario.is_saved = 0;
}

void scenario_editor_change_empire(int change)
{
    scenario.empire.id += change;
    if (scenario.empire.id < 0) {
        scenario.empire.id = 39;
    } else if (scenario.empire.id >= 40) {
        scenario.empire.id = 0;
    }
    scenario.is_saved = 0;
}

int scenario_editor_is_building_allowed(int id)
{
    return scenario.allowed_buildings[id];
}

void scenario_editor_toggle_building_allowed(int id)
{
    scenario.allowed_buildings[id] = scenario.allowed_buildings[id] ? 0 : 1;
    scenario.is_saved = 0;
}

void scenario_editor_set_player_rank(int rank)
{
    scenario.player_rank = rank;
    scenario.is_saved = 0;
}

void scenario_editor_set_initial_favor(int amount)
{
    scenario.initial_favor = amount;
    scenario.is_saved = 0;
}

void scenario_editor_set_initial_funds(int amount)
{
    scenario.initial_funds = amount;
    scenario.is_saved = 0;
}

void scenario_editor_set_rescue_loan(int amount)
{
    scenario.rescue_loan = amount;
    scenario.is_saved = 0;
}

void scenario_editor_set_initial_personal_savings(int amount)
{
    scenario.initial_personal_savings = amount;
    scenario.is_saved = 0;
}

void scenario_editor_toggle_rome_supplies_wheat(void)
{
    scenario.rome_supplies_wheat = !scenario.rome_supplies_wheat;
    scenario.is_saved = 0;
}

void scenario_editor_toggle_flotsam(void)
{
    scenario.flotsam_enabled = !scenario.flotsam_enabled;
    scenario.is_saved = 0;
}

int scenario_editor_milestone_year(int milestone_percentage)
{
    switch (milestone_percentage) {
        case 25:
            return scenario.win_criteria.milestone25_year;
        case 50:
            return scenario.win_criteria.milestone50_year;
        case 75:
            return scenario.win_criteria.milestone75_year;
        default:
            return 0;
    }
}

void scenario_editor_set_milestone_year(int milestone_percentage, int year)
{
    switch (milestone_percentage) {
        case 25:
            scenario.win_criteria.milestone25_year = year;
            break;
        case 50:
            scenario.win_criteria.milestone50_year = year;
            break;
        case 75:
            scenario.win_criteria.milestone75_year = year;
            break;
        default:
            return;
    }
    scenario.is_saved = 0;
}

void scenario_editor_set_start_year(int year)
{
    scenario.start_year = year;
    scenario.is_saved = 0;
}

void scenario_editor_toggle_open_play(void)
{
    scenario.is_open_play = !scenario.is_open_play;
    if (scenario.is_open_play) {
        scenario.open_play_scenario_id = 12; // fix it to 12: first unused entry
    }
    scenario.is_saved = 0;
}

void scenario_editor_toggle_culture(void)
{
    scenario.win_criteria.culture.enabled = !scenario.win_criteria.culture.enabled;
    scenario.is_saved = 0;
}

void scenario_editor_set_culture(int goal)
{
    scenario.win_criteria.culture.goal = goal;
    scenario.is_saved = 0;
}

void scenario_editor_toggle_prosperity(void)
{
    scenario.win_criteria.prosperity.enabled = !scenario.win_criteria.prosperity.enabled;
    scenario.is_saved = 0;
}

void scenario_editor_set_prosperity(int goal)
{
    scenario.win_criteria.prosperity.goal = goal;
    scenario.is_saved = 0;
}

void scenario_editor_toggle_peace(void)
{
    scenario.win_criteria.peace.enabled = !scenario.win_criteria.peace.enabled;
    scenario.is_saved = 0;
}

void scenario_editor_set_peace(int goal)
{
    scenario.win_criteria.peace.goal = goal;
    scenario.is_saved = 0;
}

void scenario_editor_toggle_favor(void)
{
    scenario.win_criteria.favor.enabled = !scenario.win_criteria.favor.enabled;
    scenario.is_saved = 0;
}

void scenario_editor_set_favor(int goal)
{
    scenario.win_criteria.favor.goal = goal;
    scenario.is_saved = 0;
}

void scenario_editor_toggle_population(void)
{
    scenario.win_criteria.population.enabled = !scenario.win_criteria.population.enabled;
    scenario.is_saved = 0;
}

void scenario_editor_set_population(int goal)
{
    scenario.win_criteria.population.goal = goal;
    scenario.is_saved = 0;
}

void scenario_editor_toggle_time_limit(void)
{
    scenario.win_criteria.time_limit.enabled = !scenario.win_criteria.time_limit.enabled;
    if (scenario.win_criteria.time_limit.enabled) {
        scenario.win_criteria.survival_time.enabled = 0;
    }
    scenario.is_saved = 0;
}

void scenario_editor_set_time_limit(int years)
{
    scenario.win_criteria.time_limit.years = years;
    scenario.is_saved = 0;
}

void scenario_editor_toggle_survival_time(void)
{
    scenario.win_criteria.survival_time.enabled = !scenario.win_criteria.survival_time.enabled;
    if (scenario.win_criteria.survival_time.enabled) {
        scenario.win_criteria.time_limit.enabled = 0;
    }
    scenario.is_saved = 0;
}

void scenario_editor_set_survival_time(int years)
{
    scenario.win_criteria.survival_time.years = years;
    scenario.is_saved = 0;
}
