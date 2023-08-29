#ifndef FIGURETYPE_EDITOR_H
#define FIGURETYPE_EDITOR_H

#include "figure/figure.h"

enum {
    MAP_FLAG_ENTRY = 1,
    MAP_FLAG_EXIT = 2,
    MAP_FLAG_EARTHQUAKE_MIN = 3,
    MAP_FLAG_EARTHQUAKE_MAX = 12,
    MAP_FLAG_INVASION_MIN = 13,
    MAP_FLAG_INVASION_MAX = 20,
    MAP_FLAG_RIVER_ENTRY = 21,
    MAP_FLAG_RIVER_EXIT = 22,
    MAP_FLAG_FISHING_MIN = 23,
    MAP_FLAG_FISHING_MAX = 30,
    MAP_FLAG_HERD_MIN = 31,
    MAP_FLAG_HERD_MAX = 38,
    MAP_FLAG_MIN = 1,
    MAP_FLAG_MAX = 39,
};

void figure_create_editor_flags(void);

void figure_editor_flag_action(struct figure_t *f);

#endif // FIGURETYPE_EDITOR_H
