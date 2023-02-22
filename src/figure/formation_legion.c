#include "formation_legion.h"

#include "city/data_private.h"
#include "city/military.h"
#include "city/warning.h"
#include "core/calc.h"
#include "figure/enemy_army.h"
#include "figure/figure.h"
#include "figure/route.h"
#include "map/building.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/routing.h"
#include "scenario/data.h"

int formation_legion_create_for_fort(building *fort)
{
    formation_calculate_legion_totals();

    // create legion formation
    struct formation_t *m = create_formation_type(fort->subtype.fort_figure_type);
    m->faction_id = 1;
    m->is_legion = 1;
    m->legion_id = m->id - 1;
    m->layout = FORMATION_DOUBLE_LINE_1;
    m->morale = 50;
    m->max_figures = MAX_FORMATION_FIGURES;
    m->x = m->standard_x = m->x_home = fort->x + 3;
    m->y = m->standard_y = m->y_home = fort->y - 1;
    m->building_id = fort->id;
    m->is_at_fort = 1;

    figure *standard = figure_create(FIGURE_FORT_STANDARD, 0, 0, DIR_0_TOP);
    standard->building_id = fort->id;
    standard->formation_id = m->id;
    m->standard_figure_id = standard->id;

    return m->id;
}

void formation_legion_restore_layout(struct formation_t *m)
{
    if (m->layout == FORMATION_MOP_UP) {
        m->layout = m->prev.layout;
    }
}

static int prepare_to_move(struct formation_t *m)
{
    if (m->months_very_low_morale || m->months_low_morale > 1) {
        return 0;
    }
    if (m->months_low_morale == 1) {
        formation_change_morale(m, 10); // yay, we can move!
    }
    return 1;
}

void formation_legion_move_to(struct formation_t *m, int x, int y)
{
    map_routing_calculate_distances(m->x_home, m->y_home);
    if (map_routing_distance(map_grid_offset(x, y)) <= 0) {
        return; // unable to route there
    }
    if (x == m->x_home && y == m->y_home) {
        return; // use formation_legion_return_home
    }
    if (m->cursed_by_mars) {
        return;
    }
    m->standard_x = x;
    m->standard_y = y;
    m->is_at_fort = 0;

    if (m->morale <= 20) {
        city_warning_show(WARNING_LEGION_MORALE_TOO_LOW);
    }
    for (int i = 0; i < MAX_FORMATION_FIGURES && m->figures[i]; i++) {
        figure *f = figure_get(m->figures[i]);
        if (f->action_state == FIGURE_ACTION_149_CORPSE ||
            f->action_state == FIGURE_ACTION_150_ATTACK) {
            continue;
        }
        if (prepare_to_move(m)) {
            f->alternative_location_index = 0;
            f->action_state = FIGURE_ACTION_83_SOLDIER_GOING_TO_STANDARD;
            figure_route_remove(f);
        }
    }
}

void formation_legion_return_home(struct formation_t *m)
{
    map_routing_calculate_distances(m->x_home, m->y_home);
    if (map_routing_distance(map_grid_offset(m->x, m->y)) <= 0) {
        return; // unable to route home
    }
    if (m->cursed_by_mars) {
        return;
    }
    m->is_at_fort = 1;
    formation_legion_restore_layout(m);
    for (int i = 0; i < MAX_FORMATION_FIGURES && m->figures[i]; i++) {
        figure *f = figure_get(m->figures[i]);
        if (f->action_state == FIGURE_ACTION_149_CORPSE ||
            f->action_state == FIGURE_ACTION_150_ATTACK) {
            continue;
        }
        if (prepare_to_move(m)) {
            f->action_state = FIGURE_ACTION_81_SOLDIER_GOING_TO_FORT;
            figure_route_remove(f);
        }
    }
}

static int is_legion(figure *f)
{
    if (figure_is_legion(f) || f->type == FIGURE_FORT_STANDARD) {
        return f->formation_id;
    }
    return 0;
}

int formation_legion_at_grid_offset(int grid_offset)
{
    return map_figure_foreach_until(grid_offset, is_legion);
}

int formation_legion_at_building(int grid_offset)
{
    int building_id = map_building_at(grid_offset);
    if (building_id > 0) {
        building *b = building_get(building_id);
        if (b->state == BUILDING_STATE_IN_USE && (b->type == BUILDING_FORT || b->type == BUILDING_FORT_GROUND)) {
            return b->formation_id;
        }
    }
    return 0;
}

void formation_legion_decrease_damage(void)
{
    for (int i = 1; i < MAX_FIGURES; i++) {
        figure *f = figure_get(i);
        if (f->state == FIGURE_STATE_ALIVE && figure_is_legion(f)) {
            if (f->action_state == FIGURE_ACTION_80_SOLDIER_AT_REST) {
                if (f->damage) {
                    f->damage--;
                }
            }
        }
    }
}
