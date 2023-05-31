#ifndef BUILDING_BARRACKS_H
#define BUILDING_BARRACKS_H

#include "building/building.h"

int building_get_barracks_for_weapon(int resource, int road_network_id, struct map_point_t *dst);

void building_barracks_create_soldier(struct building_t *barracks, int x, int y);

int building_barracks_create_tower_sentry(struct building_t *barracks, int x, int y);

void building_barracks_request_tower_sentry(void);

void building_barracks_decay_tower_sentry_request(void);

int building_barracks_has_tower_sentry_request(void);

void building_barracks_save_state(struct buffer_t *buf);

void building_barracks_load_state(struct buffer_t *buf);

#endif // BUILDING_BARRACKS_H
