#include "game/settings.h"
#include "scenario.h"
#include "scenario/data.h"

struct scenario_t scenario;
struct scenario_settings scenario_settings;

int scenario_is_saved(void)
{
    return scenario.is_saved;
}

void scenario_save_state(buffer *buf)
{
    buffer_write_u8(buf, scenario.open_play_scenario_id);
    buffer_write_i16(buf, scenario.image_id);

    buffer_write_i32(buf, scenario.map.width);
    buffer_write_i32(buf, scenario.map.height);
    buffer_write_i32(buf, scenario.map.grid_start);
    buffer_write_i32(buf, scenario.map.grid_border_size);

    buffer_write_i16(buf, scenario.empire.id);
    buffer_write_i32(buf, scenario.empire.is_expanded);
    buffer_write_i32(buf, scenario.empire.expansion_year);
    buffer_write_u8(buf, scenario.empire.distant_battle_roman_travel_months);
    buffer_write_u8(buf, scenario.empire.distant_battle_enemy_travel_months);

    // Map name
    buffer_write_raw(buf, scenario.scenario_name, MAX_SCENARIO_NAME);

    // Map description
    buffer_write_raw(buf, scenario.briefing, MAX_BRIEFING);

    // Brief description
    buffer_write_raw(buf, scenario.brief_description, MAX_BRIEF_DESCRIPTION);

    // Starting conditions
    buffer_write_i16(buf, scenario.player_rank);
    buffer_write_i16(buf, scenario.start_year);
    buffer_write_u8(buf, scenario.initial_favor);
    buffer_write_i32(buf, scenario.initial_funds);
    buffer_write_i32(buf, scenario.rescue_loan);
    buffer_write_i32(buf, scenario.initial_personal_savings);
    buffer_write_i32(buf, scenario.rome_supplies_wheat);
    buffer_write_u8(buf, scenario.flotsam_enabled);
    buffer_write_i32(buf, scenario.win_criteria.milestone25_year);
    buffer_write_i32(buf, scenario.win_criteria.milestone50_year);
    buffer_write_i32(buf, scenario.win_criteria.milestone75_year);

    // Terrain set
    buffer_write_u8(buf, scenario.climate);

    // Requests
    for (int i = 0; i < MAX_REQUESTS; i++) {
        buffer_write_i16(buf, scenario.requests[i].year);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        buffer_write_u8(buf, scenario.requests[i].month);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        buffer_write_u32(buf, scenario.requests[i].amount);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        buffer_write_i16(buf, scenario.requests[i].resource);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        buffer_write_i16(buf, scenario.requests[i].years_deadline);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        buffer_write_u8(buf, scenario.requests[i].favor);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        buffer_write_u8(buf, scenario.requests[i].state);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        buffer_write_u8(buf, scenario.requests[i].visible);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        buffer_write_u8(buf, scenario.requests[i].months_to_comply);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        buffer_write_u8(buf, scenario.requests[i].can_comply_dialog_shown);
    }

    // Enemy
    buffer_write_i16(buf, scenario.enemy_id);

    // Invasions
    for (int i = 0; i < MAX_INVASIONS; i++) {
        buffer_write_i16(buf, scenario.invasions[i].year);
    }
    for (int i = 0; i < MAX_INVASIONS; i++) {
        buffer_write_u8(buf, scenario.invasions[i].month);
    }
    for (int i = 0; i < MAX_INVASIONS; i++) {
        buffer_write_i16(buf, scenario.invasions[i].amount);
    }
    for (int i = 0; i < MAX_INVASIONS; i++) {
        buffer_write_i16(buf, scenario.invasions[i].type);
    }
    for (int i = 0; i < MAX_INVASIONS; i++) {
        buffer_write_i16(buf, scenario.invasions[i].from);
    }
    for (int i = 0; i < MAX_INVASIONS; i++) {
        buffer_write_i16(buf, scenario.invasions[i].attack_type);
    }

    // Buildings allowed
    for (int i = 0; i < MAX_ALLOWED_BUILDINGS; i++) {
        buffer_write_i16(buf, scenario.allowed_buildings[i]);
    }

    // Win criteria
    buffer_write_i16(buf, scenario.is_open_play);
    buffer_write_u8(buf, scenario.win_criteria.culture.enabled);
    buffer_write_i32(buf, scenario.win_criteria.culture.goal);
    buffer_write_u8(buf, scenario.win_criteria.prosperity.enabled);
    buffer_write_i32(buf, scenario.win_criteria.prosperity.goal);
    buffer_write_u8(buf, scenario.win_criteria.peace.enabled);
    buffer_write_i32(buf, scenario.win_criteria.peace.goal);
    buffer_write_u8(buf, scenario.win_criteria.favor.enabled);
    buffer_write_i32(buf, scenario.win_criteria.favor.goal);
    buffer_write_i32(buf, scenario.win_criteria.time_limit.enabled);
    buffer_write_i32(buf, scenario.win_criteria.time_limit.years);
    buffer_write_i32(buf, scenario.win_criteria.survival_time.enabled);
    buffer_write_i32(buf, scenario.win_criteria.survival_time.years);
    buffer_write_i32(buf, scenario.win_criteria.population.enabled);
    buffer_write_i32(buf, scenario.win_criteria.population.goal);

    // Special events
    buffer_write_i32(buf, scenario.earthquake.severity);
    buffer_write_i32(buf, scenario.earthquake.year);
    buffer_write_i32(buf, scenario.gladiator_revolt.enabled);
    buffer_write_i32(buf, scenario.gladiator_revolt.year);
    buffer_write_i32(buf, scenario.emperor_change.enabled);
    buffer_write_i32(buf, scenario.emperor_change.year);
    // random events
    buffer_write_i32(buf, scenario.random_events.sea_trade_problem);
    buffer_write_i32(buf, scenario.random_events.land_trade_problem);
    buffer_write_i32(buf, scenario.random_events.raise_wages);
    buffer_write_i32(buf, scenario.random_events.lower_wages);
    buffer_write_i32(buf, scenario.random_events.contaminated_water);

    // Price changes
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        buffer_write_i16(buf, scenario.price_changes[i].year);
    }
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        buffer_write_u8(buf, scenario.price_changes[i].month);
    }
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        buffer_write_u8(buf, scenario.price_changes[i].resource);
    }
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        buffer_write_u8(buf, scenario.price_changes[i].is_rise);
    }
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        buffer_write_u8(buf, scenario.price_changes[i].amount);
    }

    // Demand changes
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        buffer_write_i16(buf, scenario.demand_changes[i].year);
    }
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        buffer_write_u8(buf, scenario.demand_changes[i].month);
    }
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        buffer_write_u8(buf, scenario.demand_changes[i].resource);
    }
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        buffer_write_u8(buf, scenario.demand_changes[i].route_id);
    }
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        buffer_write_u8(buf, scenario.demand_changes[i].is_rise);
    }

    // Earthquake points
    buffer_write_i16(buf, scenario.earthquake_point.x);
    buffer_write_i16(buf, scenario.earthquake_point.y);

    // Invasion points
    for (int i = 0; i < MAX_INVASION_POINTS; i++) {
        buffer_write_i16(buf, scenario.invasion_points[i].x);
    }
    for (int i = 0; i < MAX_INVASION_POINTS; i++) {
        buffer_write_i16(buf, scenario.invasion_points[i].y);
    }

    // Entry/exit points
    buffer_write_i16(buf, scenario.entry_point.x);
    buffer_write_i16(buf, scenario.entry_point.y);
    buffer_write_i16(buf, scenario.exit_point.x);
    buffer_write_i16(buf, scenario.exit_point.y);

    // River entry/exit points
    buffer_write_i16(buf, scenario.river_entry_point.x);
    buffer_write_i16(buf, scenario.river_entry_point.y);
    buffer_write_i16(buf, scenario.river_exit_point.x);
    buffer_write_i16(buf, scenario.river_exit_point.y);

    // Native buildings
    buffer_write_i32(buf, scenario.native_images.hut);
    buffer_write_i32(buf, scenario.native_images.meeting);
    buffer_write_i32(buf, scenario.native_images.crops);

    // Fishing points
    for (int i = 0; i < MAX_FISH_POINTS; i++) {
        buffer_write_i16(buf, scenario.fishing_points[i].x);
    }
    for (int i = 0; i < MAX_FISH_POINTS; i++) {
        buffer_write_i16(buf, scenario.fishing_points[i].y);
    }

    // Herd points
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        buffer_write_i16(buf, scenario.herd_points[i].x);
    }
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        buffer_write_i16(buf, scenario.herd_points[i].y);
    }

    scenario.is_saved = 1;
}

void scenario_load_state(buffer *buf)
{
    scenario.open_play_scenario_id = buffer_read_u8(buf);
    scenario.image_id = buffer_read_i16(buf);

    scenario.map.width = buffer_read_i32(buf);
    scenario.map.height = buffer_read_i32(buf);
    scenario.map.grid_start = buffer_read_i32(buf);
    scenario.map.grid_border_size = buffer_read_i32(buf);

    scenario.empire.id = buffer_read_i16(buf);
    scenario.empire.is_expanded = buffer_read_i32(buf);
    scenario.empire.expansion_year = buffer_read_i32(buf);
    scenario.empire.distant_battle_roman_travel_months = buffer_read_u8(buf);
    scenario.empire.distant_battle_enemy_travel_months = buffer_read_u8(buf);

    // Map name
    buffer_read_raw(buf, scenario.scenario_name, MAX_SCENARIO_NAME);

    // Map description
    buffer_read_raw(buf, scenario.briefing, MAX_BRIEFING);

    // Brief description
    buffer_read_raw(buf, scenario.brief_description, MAX_BRIEF_DESCRIPTION);

    // Starting conditions
    scenario.player_rank = buffer_read_i16(buf);
    scenario.start_year = buffer_read_i16(buf);
    scenario.initial_favor = buffer_read_u8(buf);
    scenario.initial_funds = buffer_read_i32(buf);
    scenario.rescue_loan = buffer_read_i32(buf);
    scenario.initial_personal_savings = buffer_read_i32(buf);
    scenario.rome_supplies_wheat = buffer_read_i32(buf);
    scenario.flotsam_enabled = buffer_read_u8(buf);
    scenario.win_criteria.milestone25_year = buffer_read_i32(buf);
    scenario.win_criteria.milestone50_year = buffer_read_i32(buf);
    scenario.win_criteria.milestone75_year = buffer_read_i32(buf);

    // Terrain set
    scenario.climate = buffer_read_u8(buf);

    // Requests
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].year = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].month = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].amount = buffer_read_u32(buf);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].resource = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].years_deadline = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].favor = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].state = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].visible = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].months_to_comply = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].can_comply_dialog_shown = buffer_read_u8(buf);
    }

    // Enemy
    scenario.enemy_id = buffer_read_i16(buf);

    // Invasions
    for (int i = 0; i < MAX_INVASIONS; i++) {
        scenario.invasions[i].year = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_INVASIONS; i++) {
        scenario.invasions[i].month = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_INVASIONS; i++) {
        scenario.invasions[i].amount = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_INVASIONS; i++) {
        scenario.invasions[i].type = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_INVASIONS; i++) {
        scenario.invasions[i].from = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_INVASIONS; i++) {
        scenario.invasions[i].attack_type = buffer_read_i16(buf);
    }

    // Buildings allowed
    for (int i = 0; i < MAX_ALLOWED_BUILDINGS; i++) {
        scenario.allowed_buildings[i] = buffer_read_i16(buf);
    }

    // Win criteria
    scenario.is_open_play = buffer_read_i16(buf);
    scenario.win_criteria.culture.enabled = buffer_read_u8(buf);
    scenario.win_criteria.culture.goal = buffer_read_i32(buf);
    scenario.win_criteria.prosperity.enabled = buffer_read_u8(buf);
    scenario.win_criteria.prosperity.goal = buffer_read_i32(buf);
    scenario.win_criteria.peace.enabled = buffer_read_u8(buf);
    scenario.win_criteria.peace.goal = buffer_read_i32(buf);
    scenario.win_criteria.favor.enabled = buffer_read_u8(buf);
    scenario.win_criteria.favor.goal = buffer_read_i32(buf);
    scenario.win_criteria.time_limit.enabled = buffer_read_i32(buf);
    scenario.win_criteria.time_limit.years = buffer_read_i32(buf);
    scenario.win_criteria.survival_time.enabled = buffer_read_i32(buf);
    scenario.win_criteria.survival_time.years = buffer_read_i32(buf);
    scenario.win_criteria.population.enabled = buffer_read_i32(buf);
    scenario.win_criteria.population.goal = buffer_read_i32(buf);

    // Special events
    scenario.earthquake.severity = buffer_read_i32(buf);
    scenario.earthquake.year = buffer_read_i32(buf);
    scenario.gladiator_revolt.enabled = buffer_read_i32(buf);
    scenario.gladiator_revolt.year = buffer_read_i32(buf);
    scenario.emperor_change.enabled = buffer_read_i32(buf);
    scenario.emperor_change.year = buffer_read_i32(buf);
    // random events
    scenario.random_events.sea_trade_problem = buffer_read_i32(buf);
    scenario.random_events.land_trade_problem = buffer_read_i32(buf);
    scenario.random_events.raise_wages = buffer_read_i32(buf);
    scenario.random_events.lower_wages = buffer_read_i32(buf);
    scenario.random_events.contaminated_water = buffer_read_i32(buf);

    // Price changes
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        scenario.price_changes[i].year = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        scenario.price_changes[i].month = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        scenario.price_changes[i].resource = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        scenario.price_changes[i].is_rise = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        scenario.price_changes[i].amount = buffer_read_u8(buf);
    }

    // Demand changes
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        scenario.demand_changes[i].year = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        scenario.demand_changes[i].month = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        scenario.demand_changes[i].resource = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        scenario.demand_changes[i].route_id = buffer_read_u8(buf);
    }
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        scenario.demand_changes[i].is_rise = buffer_read_u8(buf);
    }

    // Earthquake points
    scenario.earthquake_point.x = buffer_read_i16(buf);
    scenario.earthquake_point.y = buffer_read_i16(buf);

    // Invasion points
    for (int i = 0; i < MAX_INVASION_POINTS; i++) {
        scenario.invasion_points[i].x = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_INVASION_POINTS; i++) {
        scenario.invasion_points[i].y = buffer_read_i16(buf);
    }

    // Entry/exit points
    scenario.entry_point.x = buffer_read_i16(buf);
    scenario.entry_point.y = buffer_read_i16(buf);
    scenario.exit_point.x = buffer_read_i16(buf);
    scenario.exit_point.y = buffer_read_i16(buf);

    // River entry/exit points
    scenario.river_entry_point.x = buffer_read_i16(buf);
    scenario.river_entry_point.y = buffer_read_i16(buf);
    scenario.river_exit_point.x = buffer_read_i16(buf);
    scenario.river_exit_point.y = buffer_read_i16(buf);

    // Native buildings
    scenario.native_images.hut = buffer_read_i32(buf);
    scenario.native_images.meeting = buffer_read_i32(buf);
    scenario.native_images.crops = buffer_read_i32(buf);

    // Fishing points
    for (int i = 0; i < MAX_FISH_POINTS; i++) {
        scenario.fishing_points[i].x = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_FISH_POINTS; i++) {
        scenario.fishing_points[i].y = buffer_read_i16(buf);
    }

    // Herd points
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        scenario.herd_points[i].x = buffer_read_i16(buf);
    }
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        scenario.herd_points[i].y = buffer_read_i16(buf);
    }

    scenario.is_saved = 1;
}

void scenario_settings_save_state(buffer *player_name)
{
    buffer_write_raw(player_name, scenario_settings.player_name, MAX_PLAYER_NAME);
}

void scenario_settings_load_state(buffer *player_name)
{
    buffer_read_raw(player_name, scenario_settings.player_name, MAX_PLAYER_NAME);
}
