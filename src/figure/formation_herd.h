#ifndef FIGURE_FORMATION_HERD_H
#define FIGURE_FORMATION_HERD_H

#include "figure/formation.h"
#include "scenario/scenario.h"

#define WOLF_PACK_SIZE 8
#define MAX_WOLF_ROAM_DISTANCE 16
#define WOLF_PACK_ROAM_DELAY 12

#define SHEEP_HERD_SIZE 10
#define MAX_SHEEP_ROAM_DISTANCE 8
#define SHEEP_HERD_ROAM_DELAY 24

#define ZEBRA_HERD_SIZE 12
#define MAX_ZEBRA_ROAM_DISTANCE 20
#define ZEBRA_HERD_ROAM_DELAY 6

extern struct formation_t herd_formations[MAX_HERD_POINTS];

extern const int HERD_FORMATION_LAYOUT_POSITION_X_OFFSETS[MAX_FORMATION_FIGURES];
extern const int HERD_FORMATION_LAYOUT_POSITION_Y_OFFSETS[MAX_FORMATION_FIGURES];

void create_herds(void);

void update_herd_formations(void);

void herd_formations_save_state(struct buffer_t *buf);
void herd_formations_load_state(struct buffer_t *buf);

#endif // FIGURE_FORMATION_HERD_H
