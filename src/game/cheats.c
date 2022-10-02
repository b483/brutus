#include "cheats.h"

#include "building/type.h"
#include "city/finance.h"
#include "city/victory.h"
#include "graphics/window.h"
#include "scenario/invasion.h"
#include "window/building_info.h"


void game_cheat_money(void)
{
    city_finance_process_cheat();
    window_invalidate();
}

void game_cheat_invasion(void)
{
    scenario_invasion_start_from_cheat();
}

void game_cheat_victory(void)
{
    city_victory_force_win();
}
