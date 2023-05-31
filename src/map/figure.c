#include "figure.h"

struct grid_u16_t map_figures;

int map_has_figure_at(int grid_offset)
{
    return map_grid_is_valid_offset(grid_offset) && map_figures.items[grid_offset] > 0;
}

int map_figure_at(int grid_offset)
{
    return map_grid_is_valid_offset(grid_offset) ? map_figures.items[grid_offset] : 0;
}

void map_figure_add(struct figure_t *f)
{
    if (!map_grid_is_valid_offset(f->grid_offset)) {
        return;
    }
    f->next_figure_id_on_same_tile = 0;

    if (map_figures.items[f->grid_offset]) {
        struct figure_t *next = &figures[map_figures.items[f->grid_offset]];
        while (next->next_figure_id_on_same_tile) {
            next = &figures[next->next_figure_id_on_same_tile];
        }
        next->next_figure_id_on_same_tile = f->id;
    } else {
        map_figures.items[f->grid_offset] = f->id;
    }
}

void map_figure_update(struct figure_t *f)
{
    if (!map_grid_is_valid_offset(f->grid_offset)) {
        return;
    }
    struct figure_t *next = &figures[map_figures.items[f->grid_offset]];
    while (next->id) {
        if (next->id == f->id) {
            return;
        }
        next = &figures[next->next_figure_id_on_same_tile];
    }
}

void map_figure_delete(struct figure_t *f)
{
    if (!map_grid_is_valid_offset(f->grid_offset) || !map_figures.items[f->grid_offset]) {
        f->next_figure_id_on_same_tile = 0;
        return;
    }

    if (map_figures.items[f->grid_offset] == f->id) {
        map_figures.items[f->grid_offset] = f->next_figure_id_on_same_tile;
    } else {
        struct figure_t *prev = &figures[map_figures.items[f->grid_offset]];
        while (prev->id && prev->next_figure_id_on_same_tile != f->id) {
            prev = &figures[prev->next_figure_id_on_same_tile];
        }
        prev->next_figure_id_on_same_tile = f->next_figure_id_on_same_tile;
    }
    f->next_figure_id_on_same_tile = 0;
}

void map_figure_save_state(struct buffer_t *buf)
{
    map_grid_save_state_u16(map_figures.items, buf);
}

void map_figure_load_state(struct buffer_t *buf)
{
    map_grid_load_state_u16(map_figures.items, buf);
}
