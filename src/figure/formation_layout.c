#include "formation_layout.h"

#include "figure/formation.h"

static const int FORMATION_LAYOUT_POSITION_X[FORMATION_MAX][MAX_FORMATION_FIGURES] = {
    {0, 1, 0, 1, -1, -1, 0, 1, -1, 2, 2, 2, 0, 1, -1, 2}, // FORMATION_TORTOISE
    {0, 0, -1, 1, -1, 1, -2, -2, 2, 2, -3, -3, 3, 3, -4, -4}, // FORMATION_DOUBLE_LINE_1
    {0, 0, 0, 1, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, // FORMATION_DOUBLE_LINE_2
    {0, 2, -2, 1, -1, 3, -3, 4, -4, 5, 6, -5, -6, 7, 8, -7}, // FORMATION_SINGLE_LINE_1
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1}, // FORMATION_SINGLE_LINE_2
    {0, 1, 0, 1, 2, 2, 1, 0, 2, 3, 3, 3, 1, 2, 0, 3}, // FORMATION_MOP_UP
    {0, 1, 0, 1, 2, 2, 1, 0, 2, 3, 3, 3, 1, 2, 0, 3}, // FORMATION_AT_REST
    {0, -3, -1, 0, 2, 2, 3, 4, 2, 3, 0, -3, 2, -1, -3, 0}, // FORMATION_ENEMY_MOB
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // FORMATION_ENEMY_DOUBLE_LINE (FORMATION_DOUBLE_LINE_1 or 2 based on map orientation)
    {0, 2, 0, 2, -2, -2, 0, 2, -2, 4, 4, 4, 0, 2, -2, 4}, // FORMATION_ENEMY_WIDE_COLUMN
    {0, 2, -1, 1, 1, -1, 3, -2, 0, -4, -1, 0, 1, 4, 2, -5}, // FORMATION_HERD
};
static const int FORMATION_LAYOUT_POSITION_Y[FORMATION_MAX][MAX_FORMATION_FIGURES] = {
    {0, 0, 1, 1, 0, 1, -1, -1, -1, -1, 0, 1, 2, 2, 2, 2}, // FORMATION_TORTOISE
    {0, 1, 0, 0, 1, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1}, // FORMATION_DOUBLE_LINE_1
    {0, -1, 1, 0, -1, 1, -2, -2, 2, 2, -3, -3, 3, 3, -4, -4}, // FORMATION_DOUBLE_LINE_2
    {0, 0, 0, 1, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 1}, // FORMATION_SINGLE_LINE_1
    {0, -2, 2, -1, 1, -3, 3, -4, 4, -5, -6, 5, 6, -7, -8, 7}, // FORMATION_SINGLE_LINE_2
    {0, 0, 1, 1, 0, 1, 2, 2, 2, 0, 1, 2, 3, 3, 3, 3}, // FORMATION_MOP_UP
    {0, 0, 1, 1, 0, 1, 2, 2, 2, 0, 1, 2, 3, 3, 3, 3}, // FORMATION_AT_REST
    {0, -2, 0, 1, 0, 1, 1, 2, -2, -1, -3, 1, -1, 2, 2, -2}, // FORMATION_ENEMY_MOB
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}, // FORMATION_ENEMY_DOUBLE_LINE (FORMATION_DOUBLE_LINE_1 or 2 based on map orientation)
    {0, 0, 2, 2, 0, 2, -2, -2, -2, -2, 0, 2, 4, 4, 4, 4}, // FORMATION_ENEMY_WIDE_COLUMN
    {0, 1, -1, 1, 0, 1, 1, -1, 2, 0, 3, 5, 4, 0, 3, 2}, // FORMATION_HERD
};

int formation_layout_position_x(int layout, int index)
{
    return FORMATION_LAYOUT_POSITION_X[layout][index];
}

int formation_layout_position_y(int layout, int index)
{
    return FORMATION_LAYOUT_POSITION_Y[layout][index];
}
