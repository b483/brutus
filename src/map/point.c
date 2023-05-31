#include "map/point.h"

static struct map_point_t last = {0, 0};

void map_point_store_result(int x, int y, struct map_point_t *point)
{
    point->x = last.x = x;
    point->y = last.y = y;
}
