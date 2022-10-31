#ifndef SCENARIO_CRITERIA_H
#define SCENARIO_CRITERIA_H

#include "core/buffer.h"

int scenario_criteria_milestone_year(int percentage);

void scenario_criteria_init_max_year(void);
int scenario_criteria_max_year(void);

void scenario_criteria_save_state(buffer *buf);

void scenario_criteria_load_state(buffer *buf);

#endif // SCENARIO_CRITERIA_H
