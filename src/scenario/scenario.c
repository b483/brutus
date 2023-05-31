#include "scenario.h"

#include "core/lang.h"
#include "core/string.h"
#include "game/settings.h"
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

struct scenario_t scenario;
struct scenario_settings_t scenario_settings;

static void init_point(struct map_point_t *point)
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

    // Win criteria
    scenario.culture_win_criteria.enabled = 1;
    scenario.culture_win_criteria.goal = 10;
    scenario.prosperity_win_criteria.enabled = 1;
    scenario.prosperity_win_criteria.goal = 10;
    scenario.peace_win_criteria.enabled = 1;
    scenario.peace_win_criteria.goal = 10;
    scenario.favor_win_criteria.enabled = 1;
    scenario.favor_win_criteria.goal = 10;

    // Buildings allowed
    for (int i = 1; i < BUILDING_TYPE_MAX; i++) {
        scenario.allowed_buildings[i] = 1;
    }

    // Special events
    scenario.gladiator_revolt.year = 1;

    // Requests
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].year = 1;
        scenario.requests[i].amount = 1;
        scenario.requests[i].years_deadline = 5;
        scenario.requests[i].months_to_comply = 60;
        scenario.requests[i].favor = 8;
    }

    // Custom messages
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.editor_custom_messages[i].year = 1;
    }

    // Earthquakes
    for (int i = 0; i < MAX_EARTHQUAKES; i++) {
        scenario.earthquakes[i].year = 1;
        for (int j = 0; j < MAX_EARTHQUAKE_BRANCHES; j++) {
            init_point(&scenario.earthquakes[i].branch_coordinates[j]);
        }
    }

    // Invasions
    for (int i = 0; i < MAX_INVASIONS; i++) {
        scenario.invasions[i].year_offset = 1;
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

    for (int i = 0; i < MAX_EARTHQUAKE_POINTS; i++) {
        init_point(&scenario.earthquake_points[i]);
    }
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

void scenario_save_state(struct buffer_t *buf)
{
    buffer_write_u16(buf, scenario.map.width);
    buffer_write_u16(buf, scenario.map.height);
    buffer_write_u16(buf, scenario.map.grid_start);
    buffer_write_u16(buf, scenario.map.grid_border_size);

    buffer_write_u8(buf, scenario.empire.id);
    buffer_write_u8(buf, scenario.empire.is_expanded);
    buffer_write_u16(buf, scenario.empire.expansion_year);
    buffer_write_u8(buf, scenario.empire.distant_battle_roman_travel_months);
    buffer_write_u8(buf, scenario.empire.distant_battle_enemy_travel_months);

    // Map name
    buffer_write_raw(buf, scenario.scenario_name, MAX_SCENARIO_NAME);

    // Brief description
    buffer_write_raw(buf, scenario.brief_description, MAX_BRIEF_DESCRIPTION);
    buffer_write_u8(buf, scenario.brief_description_image_id);

    // Scenario description
    buffer_write_raw(buf, scenario.briefing, MAX_BRIEFING);

    // Terrain set
    buffer_write_u8(buf, scenario.climate);

    // Starting conditions
    buffer_write_u8(buf, scenario.player_rank);
    buffer_write_i16(buf, scenario.start_year);
    buffer_write_u8(buf, scenario.initial_favor);
    buffer_write_u32(buf, scenario.initial_funds);
    buffer_write_u32(buf, scenario.rescue_loan);
    buffer_write_u32(buf, scenario.initial_personal_savings);
    buffer_write_u8(buf, scenario.rome_supplies_wheat);
    buffer_write_u8(buf, scenario.flotsam_enabled);

    // Win criteria
    buffer_write_u8(buf, scenario.population_win_criteria.enabled);
    buffer_write_u32(buf, scenario.population_win_criteria.goal);
    buffer_write_u8(buf, scenario.culture_win_criteria.enabled);
    buffer_write_u16(buf, scenario.culture_win_criteria.goal);
    buffer_write_u8(buf, scenario.prosperity_win_criteria.enabled);
    buffer_write_u16(buf, scenario.prosperity_win_criteria.goal);
    buffer_write_u8(buf, scenario.peace_win_criteria.enabled);
    buffer_write_u16(buf, scenario.peace_win_criteria.goal);
    buffer_write_u8(buf, scenario.favor_win_criteria.enabled);
    buffer_write_u16(buf, scenario.favor_win_criteria.goal);
    buffer_write_u8(buf, scenario.time_limit_win_criteria.enabled);
    buffer_write_u16(buf, scenario.time_limit_win_criteria.years);
    buffer_write_u8(buf, scenario.survival_time_win_criteria.enabled);
    buffer_write_u16(buf, scenario.survival_time_win_criteria.years);

    // Buildings allowed
    for (int i = 0; i < BUILDING_TYPE_MAX; i++) {
        buffer_write_u8(buf, scenario.allowed_buildings[i]);
    }

    // Special events
    buffer_write_u8(buf, scenario.gladiator_revolt.state);
    buffer_write_u8(buf, scenario.gladiator_revolt.month);
    buffer_write_u16(buf, scenario.gladiator_revolt.year);
    // random events
    buffer_write_u8(buf, scenario.random_events.sea_trade_problem);
    buffer_write_u8(buf, scenario.random_events.land_trade_problem);
    buffer_write_u8(buf, scenario.random_events.raise_wages);
    buffer_write_u8(buf, scenario.random_events.lower_wages);
    buffer_write_u8(buf, scenario.random_events.contaminated_water);

    // Requests
    for (int i = 0; i < MAX_REQUESTS; i++) {
        buffer_write_u16(buf, scenario.requests[i].year);
        buffer_write_u8(buf, scenario.requests[i].month);
        buffer_write_u16(buf, scenario.requests[i].amount);
        buffer_write_u8(buf, scenario.requests[i].resource);
        buffer_write_u16(buf, scenario.requests[i].years_deadline);
        buffer_write_u8(buf, scenario.requests[i].favor);
        buffer_write_u8(buf, scenario.requests[i].state);
        buffer_write_u8(buf, scenario.requests[i].visible);
        buffer_write_u16(buf, scenario.requests[i].months_to_comply);
        buffer_write_u8(buf, scenario.requests[i].can_comply_dialog_shown);
    }

    // Custom messages
    for (int i = 0; i < MAX_EDITOR_CUSTOM_MESSAGES; i++) {
        buffer_write_u16(buf, scenario.editor_custom_messages[i].year);
        buffer_write_u8(buf, scenario.editor_custom_messages[i].month);
        buffer_write_u8(buf, scenario.editor_custom_messages[i].urgent);
        buffer_write_u8(buf, scenario.editor_custom_messages[i].enabled);
        buffer_write_raw(buf, scenario.editor_custom_messages[i].title, MAX_CUSTOM_MESSAGE_TITLE);
        buffer_write_raw(buf, scenario.editor_custom_messages[i].text, MAX_CUSTOM_MESSAGE_TEXT);
        buffer_write_raw(buf, scenario.editor_custom_messages[i].video_file, MAX_CUSTOM_MESSAGE_VIDEO_TEXT);
    }

    // Earthquakes
    for (int i = 0; i < MAX_EARTHQUAKES; i++) {
        // link earthquake events to corresponding map points; leave a gap at the epicenter to prevent total map block-off and allow for more interesting building patterns
        scenario.earthquakes[i].branch_coordinates[0].x = scenario.earthquake_points[scenario.earthquakes[i].point].x;
        scenario.earthquakes[i].branch_coordinates[0].y = scenario.earthquake_points[scenario.earthquakes[i].point].y - 2;
        scenario.earthquakes[i].branch_coordinates[1].x = scenario.earthquake_points[scenario.earthquakes[i].point].x + 2;
        scenario.earthquakes[i].branch_coordinates[1].y = scenario.earthquake_points[scenario.earthquakes[i].point].y;
        scenario.earthquakes[i].branch_coordinates[2].x = scenario.earthquake_points[scenario.earthquakes[i].point].x;
        scenario.earthquakes[i].branch_coordinates[2].y = scenario.earthquake_points[scenario.earthquakes[i].point].y + 2;
        scenario.earthquakes[i].branch_coordinates[3].x = scenario.earthquake_points[scenario.earthquakes[i].point].x - 2;
        scenario.earthquakes[i].branch_coordinates[3].y = scenario.earthquake_points[scenario.earthquakes[i].point].y;
        buffer_write_u8(buf, scenario.earthquakes[i].state);
        buffer_write_u8(buf, scenario.earthquakes[i].severity);
        buffer_write_u8(buf, scenario.earthquakes[i].month);
        buffer_write_u16(buf, scenario.earthquakes[i].year);
        buffer_write_u16(buf, scenario.earthquakes[i].duration);
        buffer_write_u16(buf, scenario.earthquakes[i].max_duration);
        buffer_write_u8(buf, scenario.earthquakes[i].delay);
        buffer_write_u8(buf, scenario.earthquakes[i].max_delay);
        buffer_write_u8(buf, scenario.earthquakes[i].point);
        for (int j = 0; j < MAX_EARTHQUAKE_BRANCHES; j++) {
            buffer_write_i32(buf, scenario.earthquakes[i].branch_coordinates[j].x);
            buffer_write_i32(buf, scenario.earthquakes[i].branch_coordinates[j].y);
        }
    }

    // Invasions
    for (int i = 0; i < MAX_INVASIONS; i++) {
        buffer_write_u16(buf, scenario.invasions[i].year_offset);
        buffer_write_u8(buf, scenario.invasions[i].month);
        buffer_write_u16(buf, scenario.invasions[i].amount);
        buffer_write_u8(buf, scenario.invasions[i].type);
        buffer_write_u8(buf, scenario.invasions[i].enemy_type);
        buffer_write_u8(buf, scenario.invasions[i].from);
        buffer_write_u8(buf, scenario.invasions[i].target_type);
    }

    buffer_write_u8(buf, scenario.invasion_upcoming);

    // Price changes
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        buffer_write_u16(buf, scenario.price_changes[i].year);
        buffer_write_u8(buf, scenario.price_changes[i].month);
        buffer_write_u8(buf, scenario.price_changes[i].resource);
        buffer_write_u8(buf, scenario.price_changes[i].is_rise);
        buffer_write_u8(buf, scenario.price_changes[i].amount);
    }

    // Demand changes
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        buffer_write_u16(buf, scenario.demand_changes[i].year);
        buffer_write_u8(buf, scenario.demand_changes[i].month);
        buffer_write_u8(buf, scenario.demand_changes[i].resource);
        buffer_write_u8(buf, scenario.demand_changes[i].trade_city_id);
        buffer_write_u8(buf, scenario.demand_changes[i].is_rise);
    }

    // Earthquake points
    for (int i = 0; i < MAX_EARTHQUAKE_POINTS; i++) {
        buffer_write_i16(buf, scenario.earthquake_points[i].x);
        buffer_write_i16(buf, scenario.earthquake_points[i].y);
    }

    // Invasion points
    for (int i = 0; i < MAX_INVASION_POINTS; i++) {
        buffer_write_i16(buf, scenario.invasion_points[i].x);
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
    buffer_write_u32(buf, scenario.native_images.hut);
    buffer_write_u32(buf, scenario.native_images.meeting);
    buffer_write_u32(buf, scenario.native_images.crops);
    buffer_write_u32(buf, scenario.native_images.vacant_lots);

    // Fishing points
    for (int i = 0; i < MAX_FISH_POINTS; i++) {
        buffer_write_i16(buf, scenario.fishing_points[i].x);
        buffer_write_i16(buf, scenario.fishing_points[i].y);
    }

    // Herd points
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        buffer_write_i16(buf, scenario.herd_points[i].x);
        buffer_write_i16(buf, scenario.herd_points[i].y);
    }

    scenario.is_saved = 1;
}

void scenario_load_state(struct buffer_t *buf)
{
    scenario.map.width = buffer_read_u16(buf);
    scenario.map.height = buffer_read_u16(buf);
    scenario.map.grid_start = buffer_read_u16(buf);
    scenario.map.grid_border_size = buffer_read_u16(buf);

    scenario.empire.id = buffer_read_u8(buf);
    scenario.empire.is_expanded = buffer_read_u8(buf);
    scenario.empire.expansion_year = buffer_read_u16(buf);
    scenario.empire.distant_battle_roman_travel_months = buffer_read_u8(buf);
    scenario.empire.distant_battle_enemy_travel_months = buffer_read_u8(buf);

    // Map name
    buffer_read_raw(buf, scenario.scenario_name, MAX_SCENARIO_NAME);

    // Brief description
    buffer_read_raw(buf, scenario.brief_description, MAX_BRIEF_DESCRIPTION);
    scenario.brief_description_image_id = buffer_read_u8(buf);

    // Scenario description
    buffer_read_raw(buf, scenario.briefing, MAX_BRIEFING);

    // Terrain set
    scenario.climate = buffer_read_u8(buf);

    // Starting conditions
    scenario.player_rank = buffer_read_u8(buf);
    scenario.start_year = buffer_read_i16(buf);
    scenario.initial_favor = buffer_read_u8(buf);
    scenario.initial_funds = buffer_read_u32(buf);
    scenario.rescue_loan = buffer_read_u32(buf);
    scenario.initial_personal_savings = buffer_read_u32(buf);
    scenario.rome_supplies_wheat = buffer_read_u8(buf);
    scenario.flotsam_enabled = buffer_read_u8(buf);

    // Win criteria
    scenario.population_win_criteria.enabled = buffer_read_u8(buf);
    scenario.population_win_criteria.goal = buffer_read_u32(buf);
    scenario.culture_win_criteria.enabled = buffer_read_u8(buf);
    scenario.culture_win_criteria.goal = buffer_read_u16(buf);
    scenario.prosperity_win_criteria.enabled = buffer_read_u8(buf);
    scenario.prosperity_win_criteria.goal = buffer_read_u16(buf);
    scenario.peace_win_criteria.enabled = buffer_read_u8(buf);
    scenario.peace_win_criteria.goal = buffer_read_u16(buf);
    scenario.favor_win_criteria.enabled = buffer_read_u8(buf);
    scenario.favor_win_criteria.goal = buffer_read_u16(buf);
    scenario.time_limit_win_criteria.enabled = buffer_read_u8(buf);
    scenario.time_limit_win_criteria.years = buffer_read_u16(buf);
    scenario.survival_time_win_criteria.enabled = buffer_read_u8(buf);
    scenario.survival_time_win_criteria.years = buffer_read_u16(buf);

    // Buildings allowed
    for (int i = 0; i < BUILDING_TYPE_MAX; i++) {
        scenario.allowed_buildings[i] = buffer_read_u8(buf);
    }

    // Special events
    scenario.gladiator_revolt.state = buffer_read_u8(buf);
    scenario.gladiator_revolt.month = buffer_read_u8(buf);
    scenario.gladiator_revolt.year = buffer_read_u16(buf);
    // random events
    scenario.random_events.sea_trade_problem = buffer_read_u8(buf);
    scenario.random_events.land_trade_problem = buffer_read_u8(buf);
    scenario.random_events.raise_wages = buffer_read_u8(buf);
    scenario.random_events.lower_wages = buffer_read_u8(buf);
    scenario.random_events.contaminated_water = buffer_read_u8(buf);

    // Requests
    for (int i = 0; i < MAX_REQUESTS; i++) {
        scenario.requests[i].year = buffer_read_u16(buf);
        scenario.requests[i].month = buffer_read_u8(buf);
        scenario.requests[i].amount = buffer_read_u16(buf);
        scenario.requests[i].resource = buffer_read_u8(buf);
        scenario.requests[i].years_deadline = buffer_read_u16(buf);
        scenario.requests[i].favor = buffer_read_u8(buf);
        scenario.requests[i].state = buffer_read_u8(buf);
        scenario.requests[i].visible = buffer_read_u8(buf);
        scenario.requests[i].months_to_comply = buffer_read_u16(buf);
        scenario.requests[i].can_comply_dialog_shown = buffer_read_u8(buf);
    }

    // Custom messages
    for (int i = 0; i < MAX_EDITOR_CUSTOM_MESSAGES; i++) {
        scenario.editor_custom_messages[i].year = buffer_read_u16(buf);
        scenario.editor_custom_messages[i].month = buffer_read_u8(buf);
        scenario.editor_custom_messages[i].urgent = buffer_read_u8(buf);
        scenario.editor_custom_messages[i].enabled = buffer_read_u8(buf);
        buffer_read_raw(buf, scenario.editor_custom_messages[i].title, MAX_CUSTOM_MESSAGE_TITLE);
        buffer_read_raw(buf, scenario.editor_custom_messages[i].text, MAX_CUSTOM_MESSAGE_TEXT);
        buffer_read_raw(buf, scenario.editor_custom_messages[i].video_file, MAX_CUSTOM_MESSAGE_VIDEO_TEXT);
    }

    // Earthquakes
    for (int i = 0; i < MAX_EARTHQUAKES; i++) {
        scenario.earthquakes[i].state = buffer_read_u8(buf);
        scenario.earthquakes[i].severity = buffer_read_u8(buf);
        scenario.earthquakes[i].month = buffer_read_u8(buf);
        scenario.earthquakes[i].year = buffer_read_u16(buf);
        scenario.earthquakes[i].duration = buffer_read_u16(buf);
        scenario.earthquakes[i].max_duration = buffer_read_u16(buf);
        scenario.earthquakes[i].delay = buffer_read_u8(buf);
        scenario.earthquakes[i].max_delay = buffer_read_u8(buf);
        scenario.earthquakes[i].point = buffer_read_u8(buf);
        for (int j = 0; j < MAX_EARTHQUAKE_BRANCHES; j++) {
            scenario.earthquakes[i].branch_coordinates[j].x = buffer_read_i32(buf);
            scenario.earthquakes[i].branch_coordinates[j].y = buffer_read_i32(buf);
        }

    }

    // Invasions
    for (int i = 0; i < MAX_INVASIONS; i++) {
        scenario.invasions[i].year_offset = buffer_read_u16(buf);
        scenario.invasions[i].month = buffer_read_u8(buf);
        scenario.invasions[i].amount = buffer_read_u16(buf);
        scenario.invasions[i].type = buffer_read_u8(buf);
        scenario.invasions[i].enemy_type = buffer_read_u8(buf);
        scenario.invasions[i].from = buffer_read_u8(buf);
        scenario.invasions[i].target_type = buffer_read_u8(buf);
    }

    scenario.invasion_upcoming = buffer_read_u8(buf);

    // Price changes
    for (int i = 0; i < MAX_PRICE_CHANGES; i++) {
        scenario.price_changes[i].year = buffer_read_u16(buf);
        scenario.price_changes[i].month = buffer_read_u8(buf);
        scenario.price_changes[i].resource = buffer_read_u8(buf);
        scenario.price_changes[i].is_rise = buffer_read_u8(buf);
        scenario.price_changes[i].amount = buffer_read_u8(buf);
    }

    // Demand changes
    for (int i = 0; i < MAX_DEMAND_CHANGES; i++) {
        scenario.demand_changes[i].year = buffer_read_u16(buf);
        scenario.demand_changes[i].month = buffer_read_u8(buf);
        scenario.demand_changes[i].resource = buffer_read_u8(buf);
        scenario.demand_changes[i].trade_city_id = buffer_read_u8(buf);
        scenario.demand_changes[i].is_rise = buffer_read_u8(buf);
    }

    // Earthquake points
    for (int i = 0; i < MAX_EARTHQUAKE_POINTS; i++) {
        scenario.earthquake_points[i].x = buffer_read_i16(buf);
        scenario.earthquake_points[i].y = buffer_read_i16(buf);
    }

    // Invasion points
    for (int i = 0; i < MAX_INVASION_POINTS; i++) {
        scenario.invasion_points[i].x = buffer_read_i16(buf);
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
    scenario.native_images.hut = buffer_read_u32(buf);
    scenario.native_images.meeting = buffer_read_u32(buf);
    scenario.native_images.crops = buffer_read_u32(buf);
    scenario.native_images.vacant_lots = buffer_read_u32(buf);

    // Fishing points
    for (int i = 0; i < MAX_FISH_POINTS; i++) {
        scenario.fishing_points[i].x = buffer_read_i16(buf);
        scenario.fishing_points[i].y = buffer_read_i16(buf);
    }

    // Herd points
    for (int i = 0; i < MAX_HERD_POINTS; i++) {
        scenario.herd_points[i].x = buffer_read_i16(buf);
        scenario.herd_points[i].y = buffer_read_i16(buf);
    }

    scenario.is_saved = 1;
}

void scenario_settings_set_player_name(const uint8_t *name)
{
    string_copy(name, scenario_settings.player_name, MAX_PLAYER_NAME);
}

void scenario_settings_save_state(struct buffer_t *player_name)
{
    buffer_write_raw(player_name, scenario_settings.player_name, MAX_PLAYER_NAME);
}

void scenario_settings_load_state(struct buffer_t *player_name)
{
    buffer_read_raw(player_name, scenario_settings.player_name, MAX_PLAYER_NAME);
}
