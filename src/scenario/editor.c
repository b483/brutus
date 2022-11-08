#include "editor.h"

#include "core/lang.h"
#include "core/string.h"
#include "map/grid.h"
#include "scenario/data.h"

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

    // Brief description
    string_copy(lang_get_string(44, 37), scenario.brief_description, MAX_BRIEF_DESCRIPTION);

    // Scenario description/briefing
    string_copy(lang_get_string(44, 38), scenario.briefing, MAX_BRIEFING);

    // Starting conditions
    scenario.start_year = -500;
    scenario.initial_favor = 40;
    scenario.initial_funds = 1000;
    scenario.rescue_loan = 500;
    scenario.initial_personal_savings = 0;
    scenario.rome_supplies_wheat = 0;
    scenario.milestone25_year = 10;
    scenario.milestone50_year = 20;
    scenario.milestone75_year = 30;

    // Win criteria
    scenario.culture_win_criteria.enabled = 1;
    scenario.culture_win_criteria.goal = 10;
    scenario.prosperity_win_criteria.enabled = 1;
    scenario.prosperity_win_criteria.goal = 10;
    scenario.peace_win_criteria.enabled = 1;
    scenario.peace_win_criteria.goal = 10;
    scenario.favor_win_criteria.enabled = 1;
    scenario.favor_win_criteria.goal = 10;
    scenario.population_win_criteria.enabled = 0;
    scenario.population_win_criteria.goal = 0;
    scenario.time_limit_win_criteria.enabled = 0;
    scenario.time_limit_win_criteria.years = 0;
    scenario.survival_time_win_criteria.enabled = 0;
    scenario.survival_time_win_criteria.years = 0;

    // Buildings allowed
    for (int i = 0; i < MAX_ALLOWED_BUILDINGS; i++) {
        scenario.allowed_buildings[i] = 1;
    }

    scenario.earthquake.severity = 0;
    scenario.earthquake.year = 0;

    // Requests
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].year = 1;
        scenario.requests[i].amount = 1;
        scenario.requests[i].years_deadline = 5;
        scenario.requests[i].favor = 8;
    }

    // Custom messages
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.editor_custom_messages[i].year = 1;
    }

    // Invasions
    for (int i = 0; i < MAX_INVASIONS; i++) {
        scenario.invasions[i].year = 1;
        scenario.invasions[i].from = 8;
    }

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

void scenario_editor_sort_custom_messages(void)
{
    for (int i = 0; i < MAX_EDITOR_CUSTOM_MESSAGES; i++) {
        for (int j = MAX_EDITOR_CUSTOM_MESSAGES - 1; j > 0; j--) {
            editor_custom_messages_t *current = &scenario.editor_custom_messages[j];
            editor_custom_messages_t *prev = &scenario.editor_custom_messages[j - 1];
            if (current->enabled) {
                // if no previous custom message scheduled, move current back until first; if previous custom message is later than current, swap
                if (!prev->enabled || prev->year > current->year || (prev->year == current->year && prev->month > current->month)) {
                    editor_custom_messages_t tmp = *current;
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