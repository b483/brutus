#ifndef BUILDING_WAREHOUSE_H
#define BUILDING_WAREHOUSE_H

#include "building/building.h"
#include "map/point.h"

enum {
    WAREHOUSE_ROOM = 0,
    WAREHOUSE_FULL = 1,
    WAREHOUSE_SOME_ROOM = 2
};

enum {
    WAREHOUSE_TASK_NONE = -1,
    WAREHOUSE_TASK_GETTING = 0,
    WAREHOUSE_TASK_DELIVERING = 1
};

int building_warehouse_get_space_info(struct building_t *warehouse);

int building_warehouse_get_amount(struct building_t *warehouse, int resource);

int building_warehouse_add_resource(struct building_t *b, int resource);

int building_warehouse_remove_resource(struct building_t *warehouse, int resource, int amount);

void building_warehouse_remove_resource_curse(struct building_t *warehouse, int amount);

void building_warehouse_space_set_image(struct building_t *space, int resource);

void building_warehouse_space_add_import(struct building_t *space, int resource);

void building_warehouse_space_remove_export(struct building_t *space, int resource);

void building_warehouses_add_resource(int resource, int amount);

int building_warehouses_remove_resource(int resource, int amount);

int building_warehouse_for_storing(int src_building_id, int x, int y, int resource,
                                   int distance_from_entry, int road_network_id, int *understaffed,
                                   struct map_point_t *dst);

int building_warehouse_for_getting(struct building_t *src, int resource, struct map_point_t *dst);

int building_warehouse_determine_worker_task(struct building_t *warehouse, int *resource);

#endif // BUILDING_WAREHOUSE_H
