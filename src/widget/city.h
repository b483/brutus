#ifndef WIDGET_CITY_H
#define WIDGET_CITY_H

#include "input/input.h"
#include "map/map.h"

struct pixel_coordinate_t {
    int x;
    int y;
};

void widget_city_draw(void);
void widget_city_draw_for_figure(int figure_id, struct pixel_coordinate_t *coord);

void widget_city_draw_construction_cost_and_size(void);

void update_city_view_coords(int x, int y, struct map_tile_t *tile);

void scroll_map(const struct mouse_t *m);

int widget_city_has_input(void);
void request_exit_scenario(void);
void widget_city_handle_input(const struct mouse_t *m, const struct hotkeys_t *h);
void widget_city_handle_input_military(const struct mouse_t *m, const struct hotkeys_t *h, int legion_formation_id);

void widget_city_clear_current_tile(void);

int widget_city_current_grid_offset(void);

#endif // WIDGET_CITY_H
