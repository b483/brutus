#include "map_editor.h"

#include "city/view.h"
#include "core/time.h"
#include "editor/tool.h"
#include "game/game.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/panel.h"
#include "input/scroll.h"
#include "map/figure.h"
#include "map/grid.h"
#include "map/image.h"
#include "map/point.h"
#include "map/property.h"
#include "scenario/scenario.h"
#include "sound/city.h"
#include "sound/effect.h"
#include "widget/city_figure.h"
#include "widget/map_editor_tool.h"
#include "window/editor/map.h"
#include "window/popup_dialog.h"

static struct {
    map_tile current_tile;
    int selected_grid_offset;
    int new_start_grid_offset;
    int capture_input;
} data;

static struct {
    time_millis last_water_animation_time;
    int advance_water_animation;

    int image_id_water_first;
    int image_id_water_last;
} draw_context;

static void init_draw_context(void)
{
    draw_context.advance_water_animation = 0;
    time_millis now = time_get_millis();
    if (now - draw_context.last_water_animation_time > 60) {
        draw_context.last_water_animation_time = now;
        draw_context.advance_water_animation = 1;
    }
    draw_context.image_id_water_first = image_group(GROUP_TERRAIN_WATER);
    draw_context.image_id_water_last = 5 + draw_context.image_id_water_first;
}

static void draw_footprint(int x, int y, int grid_offset)
{
    if (grid_offset < 0) {
        // Outside map: draw black tile
        image_draw_isometric_footprint_from_draw_tile(image_group(GROUP_TERRAIN_BLACK), x, y, 0);
    } else if (map_property_is_draw_tile(grid_offset)) {
        // Valid grid_offset and leftmost tile -> draw
        color_t color_mask = 0;
        int image_id = map_image_at(grid_offset);
        if (draw_context.advance_water_animation &&
            image_id >= draw_context.image_id_water_first &&
            image_id <= draw_context.image_id_water_last) {
            image_id++;
            if (image_id > draw_context.image_id_water_last) {
                image_id = draw_context.image_id_water_first;
            }
            map_image_set(grid_offset, image_id);
        }
        image_draw_isometric_footprint_from_draw_tile(image_id, x, y, color_mask);
    }
}

static void draw_top(int x, int y, int grid_offset)
{
    if (!map_property_is_draw_tile(grid_offset)) {
        return;
    }
    int image_id = map_image_at(grid_offset);
    color_t color_mask = 0;
    image_draw_isometric_top_from_draw_tile(image_id, x, y, color_mask);
}

static void draw_flags(int x, int y, int grid_offset)
{
    int figure_id = map_figure_at(grid_offset);
    while (figure_id) {
        figure *f = figure_get(figure_id);
        if (!f->is_ghost) {
            city_draw_figure(f, x, y, 0);
        }
        figure_id = f->next_figure_id_on_same_tile;
    }
}

static void set_city_clip_rectangle(void)
{
    int x, y, width, height;
    city_view_get_viewport(&x, &y, &width, &height);
    graphics_set_clip_rectangle(x, y, width, height);
}

void widget_map_editor_draw(void)
{
    set_city_clip_rectangle();

    init_draw_context();
    city_view_foreach_map_tile(draw_footprint);
    city_view_foreach_valid_map_tile(draw_flags, draw_top, 0);
    map_editor_tool_draw(&data.current_tile);

    graphics_reset_clip_rectangle();
}

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

static void scroll_map(const mouse *m)
{
    pixel_offset delta;
    if (scroll_get_delta(m, &delta, SCROLL_TYPE_CITY)) {
        city_view_scroll(delta.x, delta.y);
        sound_city_decay_views();
    }
}

static int input_coords_in_map(int x, int y)
{
    int x_offset, y_offset, width, height;
    city_view_get_viewport(&x_offset, &y_offset, &width, &height);

    x -= x_offset;
    y -= y_offset;

    return (x >= 0 && x < width &&y >= 0 && y < height);
}

static void confirm_editor_exit_to_main_menu(void)
{
    game_exit_editor();
}

void request_exit_editor(void)
{
    if (scenario_is_saved()) {
        game_exit_editor();
    } else {
        window_popup_dialog_show(POPUP_DIALOG_EDITOR_QUIT_WITHOUT_SAVING, confirm_editor_exit_to_main_menu, 1);
    }
}

void widget_map_editor_handle_input(const mouse *m, const hotkeys *h)
{
    scroll_map(m);

    if (m->right.went_down && input_coords_in_map(m->x, m->y) && !editor_tool_is_active()) {
        scroll_drag_start();
    }
    if (m->right.went_up) {
        if (!editor_tool_is_active()) {
            int has_scrolled = scroll_drag_end();
            if (!has_scrolled) {
                editor_tool_deactivate();
            }
        } else {
            editor_tool_deactivate();
        }
    }

    if (h->escape_pressed) {
        if (editor_tool_is_active()) {
            editor_tool_deactivate();
        } else {
            request_exit_editor();
        }
        return;
    }

    map_tile *tile = &data.current_tile;
    update_city_view_coords(m->x, m->y, tile);

    if (tile->grid_offset) {
        if (m->left.went_down) {
            if (!editor_tool_is_in_use()) {
                editor_tool_start_use(tile);
            }
            editor_tool_update_use(tile);
        } else if (m->left.is_down || editor_tool_is_in_use()) {
            editor_tool_update_use(tile);
        }
    }
    if (m->left.went_up && editor_tool_is_in_use()) {
        editor_tool_end_use(tile);
        sound_effect_play(SOUND_EFFECT_BUILD);
    }
}

void widget_map_editor_clear_current_tile(void)
{
    data.selected_grid_offset = 0;
    data.current_tile.grid_offset = 0;
}
