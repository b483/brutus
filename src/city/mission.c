#include "mission.h"

#include "city/data_private.h"

void city_mission_reset_save_start(void)
{
    city_data.mission.start_saved_game_written = 0;
}

int city_mission_should_save_start(void)
{
    if (!city_data.mission.start_saved_game_written) {
        city_data.mission.start_saved_game_written = 1;
        return 1;
    } else {
        return 0;
    }
}
