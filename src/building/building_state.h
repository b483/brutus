#ifndef BUILDING_BUILDING_STATE_H
#define BUILDING_BUILDING_STATE_H

#include "building/building.h"
#include "core/buffer.h"

void building_state_save_to_buffer(struct buffer_t *buf, const struct building_t *b);

void building_state_load_from_buffer(struct buffer_t *buf, struct building_t *b);

#endif // BUILDING_BUILDING_STATE_H
