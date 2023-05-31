#ifndef WINDOW_BUILDING_UTILITY_H
#define WINDOW_BUILDING_UTILITY_H

#include "common.h"

void window_building_draw_engineers_post(struct building_info_context_t *c);

void window_building_draw_prefect(struct building_info_context_t *c);

void window_building_draw_burning_ruin(struct building_info_context_t *c);
void window_building_draw_rubble(struct building_info_context_t *c);

void window_building_draw_reservoir(struct building_info_context_t *c);
void window_building_draw_aqueduct(struct building_info_context_t *c);
void window_building_draw_fountain(struct building_info_context_t *c);
void window_building_draw_well(struct building_info_context_t *c);

void window_building_draw_mission_post(struct building_info_context_t *c);
void window_building_draw_native_hut(struct building_info_context_t *c);
void window_building_draw_native_meeting(struct building_info_context_t *c);
void window_building_draw_native_crops(struct building_info_context_t *c);

#endif // WINDOW_BUILDING_UTILITY_H
