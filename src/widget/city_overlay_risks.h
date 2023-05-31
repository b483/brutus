#ifndef WIGET_CITY_OVERLAY_RISKS_H
#define WIGET_CITY_OVERLAY_RISKS_H

#include "city_overlay.h"

void city_overlay_problems_prepare_building(struct building_t *b);

const struct city_overlay_t *city_overlay_for_fire(void);

const struct city_overlay_t *city_overlay_for_damage(void);

const struct city_overlay_t *city_overlay_for_crime(void);

const struct city_overlay_t *city_overlay_for_problems(void);

const struct city_overlay_t *city_overlay_for_native(void);

#endif // WIGET_CITY_OVERLAY_RISKS_H
