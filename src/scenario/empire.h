#ifndef SCENARIO_EMPIRE_H
#define SCENARIO_EMPIRE_H

int scenario_empire_id(void);

// set empire expansion date via year offset from scenario start date
void scenario_empire_set_expansion_year_offset(int expansion_year);

// get empire expansion date via year offset from scenario start date
int scenario_empire_get_expansion_year_offset(void);

int scenario_empire_is_expanded(void);

void scenario_empire_process_expansion(void);

#endif // SCENARIO_EMPIRE_H
