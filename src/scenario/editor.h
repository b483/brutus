#ifndef SCENARIO_EDITOR_H
#define SCENARIO_EDITOR_H

#include <stdint.h>

void scenario_editor_create(int map_size);

void scenario_editor_sort_requests(void);

void scenario_editor_sort_custom_messages(void);

void scenario_editor_sort_invasions(void);

void scenario_editor_sort_price_changes(void);

void scenario_editor_sort_demand_changes(void);

#endif // SCENARIO_EDITOR_H
