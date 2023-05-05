#ifndef BUILDING_HOUSE_H
#define BUILDING_HOUSE_H

#include "building/building.h"

void building_house_change_to(struct building_t *house, building_type type);
void building_house_change_to_vacant_lot(struct building_t *house);

void building_house_merge(struct building_t *house);

int building_house_can_expand(struct building_t *house, int num_tiles);

void building_house_expand_to_large_insula(struct building_t *house);
void building_house_expand_to_large_villa(struct building_t *house);
void building_house_expand_to_large_palace(struct building_t *house);

void building_house_devolve_from_large_insula(struct building_t *house);
void building_house_devolve_from_large_villa(struct building_t *house);
void building_house_devolve_from_large_palace(struct building_t *house);

void building_house_check_for_corruption(struct building_t *house);

#endif // BUILDING_HOUSE_H
