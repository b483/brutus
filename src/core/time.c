#include "core/time.h"

static uint32_t current_time;

uint32_t time_get_millis(void)
{
    return current_time;
}

void time_set_millis(uint32_t millis)
{
    current_time = millis;
}
