#ifndef CITY_MILITARY_H
#define CITY_MILITARY_H

void city_military_determine_distant_battle_city(void);

int city_military_distant_battle_roman_army_is_traveling(void);

void city_military_init_distant_battle(int enemy_strength);
int city_military_has_distant_battle(void);

void city_military_process_distant_battle(void);

#endif // CITY_MILITARY_H
