#include "property.h"

#include "core/string.h"
#include "scenario/data.h"


const uint8_t *scenario_name(void)
{
    return scenario.scenario_name;
}

void scenario_set_name(const uint8_t *name)
{
    string_copy(name, scenario.scenario_name, MAX_SCENARIO_NAME);
}

const uint8_t *scenario_player_name(void)
{
    return scenario.settings.player_name;
}

void scenario_set_player_name(const uint8_t *name)
{
    string_copy(name, scenario.settings.player_name, MAX_PLAYER_NAME);
}

int scenario_is_open_play(void)
{
    return scenario.is_open_play;
}

int scenario_open_play_id(void)
{
    return scenario.open_play_scenario_id;
}

scenario_climate scenario_property_climate(void)
{
    return scenario.climate;
}

int scenario_property_start_year(void)
{
    return scenario.start_year;
}

int scenario_property_rome_supplies_wheat(void)
{
    return scenario.rome_supplies_wheat;
}

int scenario_property_enemy(void)
{
    return scenario.enemy_id;
}

int scenario_property_player_rank(void)
{
    return scenario.player_rank;
}

int scenario_image_id(void)
{
    return scenario.image_id;
}

const uint8_t *scenario_briefing(void)
{
    return scenario.briefing;
}

const uint8_t *scenario_brief_description(void)
{
    return scenario.brief_description;
}

int scenario_initial_favor(void)
{
    return scenario.initial_favor;
}

int scenario_initial_funds(void)
{
    return scenario.initial_funds;
}

int scenario_rescue_loan(void)
{
    return scenario.rescue_loan;
}

int scenario_initial_personal_savings(void)
{
    return scenario.initial_personal_savings;
}