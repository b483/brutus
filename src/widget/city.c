#include "city.h"

#include "building/construction.h"
#include "city/data_private.h"
#include "city/finance.h"
#include "city/view.h"
#include "city/warning.h"
#include "core/string.h"
#include "figure/formation_legion.h"
#include "game/settings.h"
#include "game/state.h"
#include "game/undo.h"
#include "graphics/button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "input/scroll.h"
#include "map/building.h"
#include "map/grid.h"
#include "map/routing.h"
#include "scenario/data.h"
#include "sound/city.h"
#include "sound/speech.h"
#include "sound/effect.h"
#include "widget/city_with_overlay.h"
#include "widget/city_without_overlay.h"
#include "widget/minimap.h"
#include "window/building_info.h"
#include "window/city.h"
#include "window/main_menu.h"
#include "window/popup_dialog.h"

static struct {
    map_tile current_tile;
    map_tile selected_tile;
    int new_start_grid_offset;
    int capture_input;
} data;

static void set_city_clip_rectangle(void)
{
    int x, y, width, height;
    city_view_get_viewport(&x, &y, &width, &height);
    graphics_set_clip_rectangle(x, y, width, height);
}

void widget_city_draw(void)
{
    set_city_clip_rectangle();

    if (game_state_overlay()) {
        city_with_overlay_draw(&data.current_tile);
    } else {
        city_without_overlay_draw(0, 0, &data.current_tile);
    }

    graphics_reset_clip_rectangle();
}

void widget_city_draw_for_figure(int figure_id, pixel_coordinate *coord)
{
    set_city_clip_rectangle();

    city_without_overlay_draw(figure_id, coord, &data.current_tile);

    graphics_reset_clip_rectangle();
}

void widget_city_draw_construction_cost_and_size(void)
{
    if (scroll_in_progress()) {
        return;
    }
    int size_x, size_y;
    int cost = building_construction_cost();
    int has_size = building_construction_size(&size_x, &size_y);
    if (!cost && !has_size) {
        return;
    }
    set_city_clip_rectangle();
    int x, y;
    city_view_get_selected_tile_pixels(&x, &y);
    if (cost) {
        color_t color;
        if (cost <= city_data.finance.treasury) {
            // Color blind friendly
            color = scenario.climate == CLIMATE_DESERT ? COLOR_FONT_ORANGE : COLOR_FONT_ORANGE_LIGHT;
        } else {
            color = COLOR_FONT_RED;
        }
        text_draw_number_colored(cost, '@', " ", x + 58 + 1, y + 1, FONT_NORMAL_PLAIN, COLOR_BLACK);
        text_draw_number_colored(cost, '@', " ", x + 58, y, FONT_NORMAL_PLAIN, color);
    }
    if (has_size) {
        int width = -text_get_width(string_from_ascii("  "), FONT_SMALL_PLAIN);
        width += text_draw_number_colored(size_x, '@', "x", x - 15 + 1, y + 25 + 1, FONT_SMALL_PLAIN, COLOR_BLACK);
        text_draw_number_colored(size_x, '@', "x", x - 15, y + 25, FONT_SMALL_PLAIN, COLOR_FONT_YELLOW);
        text_draw_number_colored(size_y, '@', " ", x - 15 + width + 1, y + 25 + 1, FONT_SMALL_PLAIN, COLOR_BLACK);
        text_draw_number_colored(size_y, '@', " ", x - 15 + width, y + 25, FONT_SMALL_PLAIN, COLOR_FONT_YELLOW);
    }
    graphics_reset_clip_rectangle();
}

static int is_pause_button(int x, int y)
{
    return x < 5 * BLOCK_SIZE && y >= 24 && y < 24 + 4 * BLOCK_SIZE;
}

static int is_cancel_construction_button(int x, int y)
{
    if (!building_construction_type()) {
        return 0;
    }
    int city_x, city_y, width, height;
    city_view_get_viewport(&city_x, &city_y, &width, &height);

    int touch_width = 5 * BLOCK_SIZE;
    int touch_height = 4 * BLOCK_SIZE;
    int x_offset = width - touch_width;
    int y_offset = 24;
    return x >= x_offset && x < x_offset + touch_width && y >= y_offset && y < y_offset + touch_height;
}

// INPUT HANDLING

static void update_city_view_coords(int x, int y, map_tile *tile)
{
    view_tile view;
    if (city_view_pixels_to_view_tile(x, y, &view)) {
        tile->grid_offset = city_view_tile_to_grid_offset(&view);
        city_view_set_selected_view_tile(&view);
        tile->x = map_grid_offset_to_x(tile->grid_offset);
        tile->y = map_grid_offset_to_y(tile->grid_offset);
    } else {
        tile->grid_offset = tile->x = tile->y = 0;
    }
}

static int handle_right_click_allow_building_info(const map_tile *tile)
{
    int allow = 1;
    if (!window_is(WINDOW_CITY)) {
        allow = 0;
    }
    window_city_show();

    if (!tile->grid_offset) {
        allow = 0;
    }
    if (allow && city_has_warnings()) {
        city_warning_clear_all();
        allow = 0;
    }
    return allow;
}

static void build_start(const map_tile *tile)
{
    if (tile->grid_offset) { // Allow building on paused
        building_construction_start(tile->x, tile->y, tile->grid_offset);
    }
}

static void build_move(const map_tile *tile)
{
    if (!building_construction_in_progress()) {
        return;
    }
    building_construction_update(tile->x, tile->y, tile->grid_offset);
}

static void scroll_map(const mouse *m)
{
    pixel_offset delta;
    if (scroll_get_delta(m, &delta, SCROLL_TYPE_CITY)) {
        city_view_scroll(delta.x, delta.y);
        sound_city_decay_views();
    }
}

static int input_coords_in_city(int x, int y)
{
    if (is_pause_button(x, y) || is_cancel_construction_button(x, y)) {
        return 0;
    }
    int x_offset, y_offset, width, height;
    city_view_get_viewport(&x_offset, &y_offset, &width, &height);

    x -= x_offset;
    y -= y_offset;

    return (x >= 0 && x < width && y >= 0 && y < height);
}

int widget_city_has_input(void)
{
    return data.capture_input;
}

static void handle_mouse(const mouse *m)
{
    map_tile *tile = &data.current_tile;
    update_city_view_coords(m->x, m->y, tile);
    building_construction_reset_draw_as_constructing();
    if (m->left.went_down) {
        // handle legion click
        if (tile->grid_offset) {
            int formation_id = formation_legion_at_grid_offset(tile->grid_offset);
            if (formation_id && !formations[formation_id].in_distant_battle) {
                window_city_military_show(formation_id);
                return;
            }
        }
        if (!building_construction_in_progress() && building_construction_type() != BUILDING_NONE) {
            build_start(tile);
        }
        build_move(tile);
    } else if (m->left.is_down || building_construction_in_progress()) {
        build_move(tile);
    }
    if (m->left.went_up) {
        if (building_construction_in_progress()) {
            if (building_construction_type() != BUILDING_NONE) {
                sound_effect_play(SOUND_EFFECT_BUILD);
            }
            building_construction_place();
            widget_minimap_invalidate();
        }
    }
    if (m->right.went_down && input_coords_in_city(m->x, m->y) && !building_construction_type()) {
        scroll_drag_start();
    }
    if (m->right.went_up) {
        if (!building_construction_type()) {
            int has_scrolled = scroll_drag_end();
            if (!has_scrolled && handle_right_click_allow_building_info(tile)) {
                window_building_info_show(tile->grid_offset);
            }
        } else {
            building_construction_cancel();
            window_request_refresh();
        }
    }
}


static void confirm_exit_to_main_menu(void)
{
    building_construction_clear_type();
    game_undo_disable();
    game_state_reset_overlay();
    window_main_menu_show(1);
}

void request_exit_scenario(void)
{
    window_popup_dialog_show(POPUP_DIALOG_QUIT, confirm_exit_to_main_menu, 1);
}

void widget_city_handle_input(const mouse *m, const hotkeys *h)
{
    scroll_map(m);

    handle_mouse(m);

    if (h->escape_pressed) {
        if (building_construction_type()) {
            building_construction_cancel();
            window_request_refresh();
        } else {
            request_exit_scenario();
        }
    }
}

void widget_city_handle_input_military(const mouse *m, const hotkeys *h, int legion_formation_id)
{
    map_tile *tile = &data.current_tile;
    update_city_view_coords(m->x, m->y, tile);
    if (!city_view_is_sidebar_collapsed() && widget_minimap_handle_mouse(m)) {
        return;
    }
    scroll_map(m);
    if (m->right.went_up || h->escape_pressed) {
        data.capture_input = 0;
        city_warning_clear_all();
        window_city_show();
    } else {
        update_city_view_coords(m->x, m->y, tile);
        if (m->left.went_down) {
            if (!tile->grid_offset) {
                return;
            }
            if (formations[legion_formation_id].in_distant_battle || formations[legion_formation_id].cursed_by_mars) {
                return;
            }
            // return legion home upon clicking on own fort/fort ground
            building *b = building_get(map_building_at(tile->grid_offset));
            if (b && b->state == BUILDING_STATE_IN_USE && (b->type == BUILDING_FORT || b->type == BUILDING_FORT_GROUND) && b->formation_id == legion_formation_id) {
                formation_legion_return_home(&formations[legion_formation_id]);
            } else { // move legion if route available
                map_routing_calculate_distances(formations[legion_formation_id].x_home, formations[legion_formation_id].y_home);
                if (map_routing_distance(tile->grid_offset)
                && !formations[legion_formation_id].cursed_by_mars
                && formations[legion_formation_id].morale > ROUT_MORALE_THRESHOLD
                && formations[legion_formation_id].num_figures) {
                    formation_legion_move_to(&formations[legion_formation_id], tile);
                } else if (formations[legion_formation_id].morale <= ROUT_MORALE_THRESHOLD) {
                    city_warning_show(WARNING_LEGION_MORALE_TOO_LOW);
                }
            }
            window_city_show();
        }
    }
}

int widget_city_current_grid_offset(void)
{
    return data.current_tile.grid_offset;
}

void widget_city_get_tooltip(tooltip_context *c)
{
    if (setting_tooltips() == TOOLTIPS_NONE) {
        return;
    }
    if (!window_is(WINDOW_CITY)) {
        return;
    }
    if (data.current_tile.grid_offset == 0) {
        return;
    }
    int grid_offset = data.current_tile.grid_offset;
    int building_id = map_building_at(grid_offset);
    int overlay = game_state_overlay();
    // regular tooltips
    if (overlay == OVERLAY_NONE && building_id && building_get(building_id)->type == BUILDING_SENATE) {
        c->type = TOOLTIP_SENATE;
        c->high_priority = 1;
        return;
    }
    // overlay tooltips
    if (overlay != OVERLAY_NONE) {
        c->text_group = 66;
        c->text_id = city_with_overlay_get_tooltip_text(c, grid_offset);
        if (c->text_id) {
            c->type = TOOLTIP_OVERLAY;
            c->high_priority = 1;
        }
    }
}

void widget_city_clear_current_tile(void)
{
    data.selected_tile.x = -1;
    data.selected_tile.y = -1;
    data.selected_tile.grid_offset = 0;
    data.current_tile.grid_offset = 0;
}
