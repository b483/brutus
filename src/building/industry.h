#ifndef BUILDING_INDUSTRY_H
#define BUILDING_INDUSTRY_H

#include "building/building.h"

int building_is_farm(int type);
int building_is_workshop(int type);

void building_industry_update_production(void);
void building_industry_update_wheat_production(void);

int building_industry_has_produced_resource(struct building_t *b);
void building_industry_start_new_production(struct building_t *b);

void building_bless_farms(void);
void building_curse_farms(int big_curse);

void building_workshop_add_raw_material(struct building_t *b);

int building_get_workshop_for_raw_material(
    int x, int y, int resource, int distance_from_entry, int road_network_id, struct map_point_t *dst);
int building_get_workshop_for_raw_material_with_room(
    int x, int y, int resource, int distance_from_entry, int road_network_id, struct map_point_t *dst);

#endif // BUILDING_INDUSTRY_H
