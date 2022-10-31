#include "core/string.h"
#include "game/settings.h"
#include "scenario.h"
#include "scenario/data.h"

struct scenario_t scenario;
struct scenario_settings scenario_settings;

void scenario_save_state(buffer *buf)
{
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

    // Brief description
    buffer_write_raw(buf, scenario.brief_description, MAX_BRIEF_DESCRIPTION);
    buffer_write_i16(buf, scenario.brief_description_image_id);

    // Scenario description
    buffer_write_raw(buf, scenario.briefing, MAX_BRIEFING);

    // Terrain set
    buffer_write_u8(buf, scenario.climate);

    // Starting conditions
    buffer_write_i16(buf, scenario.player_rank);
    buffer_write_i16(buf, scenario.start_year);
    buffer_write_u8(buf, scenario.initial_favor);
    buffer_write_i32(buf, scenario.initial_funds);
    buffer_write_i32(buf, scenario.rescue_loan);
    buffer_write_i32(buf, scenario.initial_personal_savings);
    buffer_write_i32(buf, scenario.rome_supplies_wheat);
    buffer_write_u8(buf, scenario.flotsam_enabled);

    // Win criteria
    buffer_write_i16(buf, scenario.is_open_play);
    buffer_write_i32(buf, scenario.population_win_criteria.enabled);
    buffer_write_i32(buf, scenario.population_win_criteria.goal);
    buffer_write_u8(buf, scenario.culture_win_criteria.enabled);
    buffer_write_i32(buf, scenario.culture_win_criteria.goal);
    buffer_write_u8(buf, scenario.prosperity_win_criteria.enabled);
    buffer_write_i32(buf, scenario.prosperity_win_criteria.goal);
    buffer_write_u8(buf, scenario.peace_win_criteria.enabled);
    buffer_write_i32(buf, scenario.peace_win_criteria.goal);
    buffer_write_u8(buf, scenario.favor_win_criteria.enabled);
    buffer_write_i32(buf, scenario.favor_win_criteria.goal);
    buffer_write_i32(buf, scenario.time_limit_win_criteria.enabled);
    buffer_write_i32(buf, scenario.time_limit_win_criteria.years);
    buffer_write_i32(buf, scenario.survival_time_win_criteria.enabled);
    buffer_write_i32(buf, scenario.survival_time_win_criteria.years);
    buffer_write_i32(buf, scenario.milestone25_year);
    buffer_write_i32(buf, scenario.milestone50_year);
    buffer_write_i32(buf, scenario.milestone75_year);

    // Buildings allowed
    for (int i = 0; i < MAX_ALLOWED_BUILDINGS; i++) {
        buffer_write_i16(buf, scenario.allowed_buildings[i]);
    }

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

    // Buildings
    buffer_write_i32(buf, scenario.native_images.hut);
    buffer_write_i32(buf, scenario.native_images.meeting);
    buffer_write_i32(buf, scenario.native_images.crops);
    buffer_write_i32(buf, scenario.native_images.vacant_lots);

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

    // Brief description
    buffer_read_raw(buf, scenario.brief_description, MAX_BRIEF_DESCRIPTION);
    scenario.brief_description_image_id = buffer_read_i16(buf);

    // Scenario description
    buffer_read_raw(buf, scenario.briefing, MAX_BRIEFING);

    // Terrain set
    scenario.climate = buffer_read_u8(buf);

    // Starting conditions
    scenario.player_rank = buffer_read_i16(buf);
    scenario.start_year = buffer_read_i16(buf);
    scenario.initial_favor = buffer_read_u8(buf);
    scenario.initial_funds = buffer_read_i32(buf);
    scenario.rescue_loan = buffer_read_i32(buf);
    scenario.initial_personal_savings = buffer_read_i32(buf);
    scenario.rome_supplies_wheat = buffer_read_i32(buf);
    scenario.flotsam_enabled = buffer_read_u8(buf);

    // Win criteria
    scenario.is_open_play = buffer_read_i16(buf);
    scenario.population_win_criteria.enabled = buffer_read_i32(buf);
    scenario.population_win_criteria.goal = buffer_read_i32(buf);
    scenario.culture_win_criteria.enabled = buffer_read_u8(buf);
    scenario.culture_win_criteria.goal = buffer_read_i32(buf);
    scenario.prosperity_win_criteria.enabled = buffer_read_u8(buf);
    scenario.prosperity_win_criteria.goal = buffer_read_i32(buf);
    scenario.peace_win_criteria.enabled = buffer_read_u8(buf);
    scenario.peace_win_criteria.goal = buffer_read_i32(buf);
    scenario.favor_win_criteria.enabled = buffer_read_u8(buf);
    scenario.favor_win_criteria.goal = buffer_read_i32(buf);
    scenario.time_limit_win_criteria.enabled = buffer_read_i32(buf);
    scenario.time_limit_win_criteria.years = buffer_read_i32(buf);
    scenario.survival_time_win_criteria.enabled = buffer_read_i32(buf);
    scenario.survival_time_win_criteria.years = buffer_read_i32(buf);
    scenario.milestone25_year = buffer_read_i32(buf);
    scenario.milestone50_year = buffer_read_i32(buf);
    scenario.milestone75_year = buffer_read_i32(buf);

    // Buildings allowed
    for (int i = 0; i < MAX_ALLOWED_BUILDINGS; i++) {
        scenario.allowed_buildings[i] = buffer_read_i16(buf);
    }

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
    scenario.native_images.vacant_lots = buffer_read_i32(buf);

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

void scenario_settings_set_player_name(const uint8_t *name)
{
    string_copy(name, scenario_settings.player_name, MAX_PLAYER_NAME);
}

void scenario_settings_save_state(buffer *player_name)
{
    buffer_write_raw(player_name, scenario_settings.player_name, MAX_PLAYER_NAME);
}

void scenario_settings_load_state(buffer *player_name)
{
    buffer_read_raw(player_name, scenario_settings.player_name, MAX_PLAYER_NAME);
}
