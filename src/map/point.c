#include "map/point.h"

static map_point last = {0, 0};

void map_point_store_result(int x, int y, map_point *point)
{
    point->x = last.x = x;
    point->y = last.y = y;
}
