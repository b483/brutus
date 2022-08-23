#include "city.h"

#include "building/construction.h"
#include "building/properties.h"
#include "city/finance.h"
#include "city/view.h"
#include "city/warning.h"
#include "core/direction.h"
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
#include "scenario/property.h"
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
        if (cost <= city_finance_treasury()) {
            // Color blind friendly
            color = scenario_property_climate() == CLIMATE_DESERT ? COLOR_FONT_ORANGE : COLOR_FONT_ORANGE_LIGHT;
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

static int handle_legion_click(const map_tile *tile)
{
    if (tile->grid_offset) {
        int formation_id = formation_legion_at_grid_offset(tile->grid_offset);
        if (formation_id > 0 && !formation_get(formation_id)->in_distant_battle) {
            window_city_military_show(formation_id);
            return 1;
        }
    }
    return 0;
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

static void build_end(void)
{
    if (building_construction_in_progress()) {
        if (building_construction_type() != BUILDING_NONE) {
            sound_effect_play(SOUND_EFFECT_BUILD);
        }
        building_construction_place();
        widget_minimap_invalidate();
    }
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

    return (x >= 0 && x < width &&y >= 0 && y < height);
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
        if (handle_legion_click(tile)) {
            return;
        }
        if (!building_construction_in_progress()) {
            build_start(tile);
        }
        build_move(tile);
    } else if (m->left.is_down || building_construction_in_progress()) {
        build_move(tile);
    }
    if (m->left.went_up) {
        build_end();
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


static void confirm_exit_to_main_menu(int accepted)
{
    if (accepted) {
        building_construction_clear_type();
        game_undo_disable();
        game_state_reset_overlay();
        window_main_menu_show(1);
    } else {
        window_city_return();
    }
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

static void military_map_click(int legion_formation_id, const map_tile *tile)
{
    if (!tile->grid_offset) {
        return;
    }
    formation *m = formation_get(legion_formation_id);
    if (m->in_distant_battle || m->cursed_by_mars) {
        return;
    }
    int other_formation_id = formation_legion_at_building(tile->grid_offset);
    if (other_formation_id && other_formation_id == legion_formation_id) {
        formation_legion_return_home(m);
    } else {
        formation_legion_move_to(m, tile->x, tile->y);
        sound_speech_play_file("wavs/cohort5.wav");
    }
    window_city_show();
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
            military_map_click(legion_formation_id, tile);
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
    if (overlay == OVERLAY_NONE && building_id && building_get(building_id)->type == BUILDING_SENATE_UPGRADED) {
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
