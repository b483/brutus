#ifndef BUILDING_BUILDING_STATE_H
#define BUILDING_BUILDING_STATE_H

#include "building/building.h"
#include "core/buffer.h"

void building_state_save_to_buffer(buffer *buf, const struct building_t *b);

void building_state_load_from_buffer(buffer *buf, struct building_t *b);

#endif // BUILDING_BUILDING_STATE_H
