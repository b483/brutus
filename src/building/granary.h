#ifndef BUILDING_GRANARY_H
#define BUILDING_GRANARY_H

#include "building/building.h"
#include "map/point.h"

enum {
    GRANARY_TASK_NONE = -1,
    GRANARY_TASK_GETTING = 0
};

int building_granary_add_resource(struct building_t *granary, int resource, int is_produced);

int building_granary_remove_resource(struct building_t *granary, int resource, int amount);

int building_granary_remove_for_getting_deliveryman(struct building_t *src, struct building_t *dst, int *resource);

int building_granary_determine_worker_task(struct building_t *granary);

void building_granaries_calculate_stocks(void);

int building_granary_for_storing(int x, int y, int resource, int distance_from_entry, int road_network_id,
                                 int force_on_stockpile, int *understaffed, struct map_point_t *dst);

int building_getting_granary_for_storing(int x, int y, int resource, int distance_from_entry, int road_network_id,
                                         struct map_point_t *dst);

int building_granary_for_getting(struct building_t *src, struct map_point_t *dst);

void building_granary_bless(void);

void building_granary_warehouse_curse(int big);

#endif // BUILDING_GRANARY_H
