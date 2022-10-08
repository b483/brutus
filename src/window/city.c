#include "city.h"

#include "building/clone.h"
#include "building/construction.h"
#include "building/menu.h"
#include "city/message.h"
#include "city/victory.h"
#include "city/view.h"
#include "city/warning.h"
#include "core/config.h"
#include "figure/formation.h"
#include "figure/formation_legion.h"
#include "game/cheats.h"
#include "game/file.h"
#include "game/orientation.h"
#include "game/settings.h"
#include "game/state.h"
#include "game/time.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "map/bookmark.h"
#include "map/grid.h"
#include "scenario/building.h"
#include "scenario/criteria.h"
#include "scenario/property.h"
#include "widget/city.h"
#include "widget/city_with_overlay.h"
#include "widget/top_menu.h"
#include "widget/sidebar/city.h"
#include "window/advisors.h"
#include "window/empire.h"
#include "window/file_dialog.h"
#include "window/message_list.h"
#include "window/popup_dialog.h"

static int any_selected_legion_index = 0;
static int current_selected_legion_index = 0;

void window_city_draw_background(void)
{
    widget_sidebar_city_draw_background();
    widget_top_menu_draw(1);
}

static int center_in_city(int element_width_pixels)
{
    int x, y, width, height;
    city_view_get_viewport(&x, &y, &width, &height);
    int margin = (width - element_width_pixels) / 2;
    return x + margin;
}

static void draw_paused_banner(void)
{
    if (game_state_is_paused()) {
        int x_offset = center_in_city(448);
        outer_panel_draw(x_offset, 40, 28, 3);
        lang_text_draw_centered(13, 2, x_offset, 58, 448, FONT_NORMAL_BLACK);
    }
}

static void draw_time_left(void)
{
    if (scenario_criteria_time_limit_enabled() && !city_victory_has_won()) {
        int years;
        if (scenario_criteria_max_year() <= game_time_year() + 1) {
            years = 0;
        } else {
            years = scenario_criteria_max_year() - game_time_year() - 1;
        }
        int total_months = 12 - game_time_month() + 12 * years;
        label_draw(1, 25, 15, 1);
        int width = lang_text_draw(6, 2, 6, 29, FONT_NORMAL_BLACK);
        text_draw_number(total_months, '@', " ", 6 + width, 29, FONT_NORMAL_BLACK);
    } else if (scenario_criteria_survival_enabled() && !city_victory_has_won()) {
        int years;
        if (scenario_criteria_max_year() <= game_time_year() + 1) {
            years = 0;
        } else {
            years = scenario_criteria_max_year() - game_time_year() - 1;
        }
        int total_months = 12 - game_time_month() + 12 * years;
        label_draw(1, 25, 15, 1);
        int width = lang_text_draw(6, 3, 6, 29, FONT_NORMAL_BLACK);
        text_draw_number(total_months, '@', " ", 6 + width, 29, FONT_NORMAL_BLACK);
    }
}

static void draw_foreground(void)
{
    widget_top_menu_draw(0);
    widget_city_draw();
    widget_sidebar_city_draw_foreground();
    if (window_is(WINDOW_CITY) || window_is(WINDOW_CITY_MILITARY)) {
        draw_time_left();
        draw_paused_banner();
    }
    widget_city_draw_construction_cost_and_size();
    if (window_is(WINDOW_CITY)) {
        city_message_process_queue();
    }
}

static void show_overlay(int overlay)
{
    if (game_state_overlay() == overlay) {
        game_state_set_overlay(OVERLAY_NONE);
    } else {
        game_state_set_overlay(overlay);
    }
    city_with_overlay_update();
    window_invalidate();
}

static void cycle_legion(void)
{
    int n_legions = formation_get_num_legions_cached();
    // check if any legions (forts) exist
    if (!n_legions) {
        return;
    }

    // wrap around if last legion was selected previously or forts deleted
    if (current_selected_legion_index >= n_legions) {
        current_selected_legion_index = 0;
    }
    // legion indexes are 1-6
    current_selected_legion_index++;

    int current_selected_legion_formation_id;
    while (current_selected_legion_index <= n_legions) {
        // formation id needed to prevent mismatch with index if forts deleted
        current_selected_legion_formation_id = formation_for_legion(current_selected_legion_index);
        formation *m = formation_get(current_selected_legion_formation_id);
        if (m->in_distant_battle || !m->num_figures) {
            // wrap around if last legion can't be selected but any other had been available previously
            if ((current_selected_legion_index == n_legions) && any_selected_legion_index) {
                current_selected_legion_index = 1;
                // prevent infinite loop if e.g. all forts were re-built after a legion had already been selected, or all legions were vanquished
                any_selected_legion_index = 0;
            } else {
                current_selected_legion_index++;
            }
        } else {
            window_city_military_show(current_selected_legion_formation_id);
            if (!any_selected_legion_index) {
                any_selected_legion_index = current_selected_legion_index;
            }
            return;
        }
    }
}

static void return_legions_to_fort(void)
{
    int n_legions = formation_get_num_legions_cached();
    for (int i = 0; i < n_legions; i++) {
        formation *m = formation_get(formation_for_legion(i + 1));
        if (!m->in_distant_battle && !m->is_at_fort) {
            formation_legion_return_home(m);
        }
    }
}

static void toggle_pause(void)
{
    game_state_toggle_paused();
    city_warning_clear_all();
}

static void set_construction_building_type(building_type type)
{
    if (scenario_building_allowed(type) && building_menu_is_enabled(type)) {
        building_construction_cancel();
        building_construction_set_type(type);
        window_request_refresh();
    }
}

static void replay_map_confirmed(void)
{
    if (game_file_start_scenario((char *) scenario_get_name())) {
        window_city_show();
    }
}

void replay_map(void)
{
    building_construction_clear_type();
    window_popup_dialog_show_confirmation(1, 2, replay_map_confirmed);
}

static void handle_hotkeys(const hotkeys *h)
{
    if (h->load_file) {
        window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_LOAD);
    }
    if (h->save_file) {
        window_file_dialog_show(FILE_TYPE_SAVED_GAME, FILE_DIALOG_SAVE);
    }
    if (h->decrease_game_speed) {
        setting_decrease_game_speed();
    }
    if (h->increase_game_speed) {
        setting_increase_game_speed();
    }
    if (h->toggle_pause) {
        toggle_pause();
    }
    if (h->rotate_map_left) {
        game_orientation_rotate_left();
        window_invalidate();
    }
    if (h->rotate_map_right) {
        game_orientation_rotate_right();
        window_invalidate();
    }
    if (h->replay_map) {
        replay_map();
    }
    if (h->cycle_legion) {
        cycle_legion();
    }
    if (h->return_legions_to_fort) {
        return_legions_to_fort();
    }
    if (h->show_last_advisor) {
        window_advisors_show_advisor(setting_last_advisor());
    }
    if (h->show_empire_map) {
        window_empire_show();
    }
    if (h->show_messages) {
        window_message_list_show();
    }
    if (h->clone_building) {
        building_type type = building_clone_type_from_grid_offset(widget_city_current_grid_offset());
        if (type) {
            set_construction_building_type(type);
        }
    }
    if (h->building) {
        set_construction_building_type(h->building);
    }

    if (h->show_overlay) {
        show_overlay(h->show_overlay);
    }
    if (h->go_to_bookmark) {
        if (map_bookmark_go_to(h->go_to_bookmark - 1)) {
            window_invalidate();
        }
    }
    if (h->set_bookmark) {
        map_bookmark_save(h->set_bookmark - 1);
    }
    if (h->cheat_money) {
        game_cheat_money();
    }
    if (h->cheat_invasion) {
        game_cheat_invasion();
    }
    if (h->cheat_victory) {
        game_cheat_victory();
    }
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    handle_hotkeys(h);
    if (!building_construction_in_progress()) {
        if (widget_top_menu_handle_input(m, h)) {
            return;
        }
        if (widget_sidebar_city_handle_mouse(m)) {
            return;
        }
    }
    widget_city_handle_input(m, h);
}

static void handle_input_military(const mouse *m, const hotkeys *h)
{
    handle_hotkeys(h);
    if (widget_top_menu_handle_input(m, h)) {
        return;
    }
    widget_city_handle_input_military(m, h, formation_get_selected());
}

static void get_tooltip(tooltip_context *c)
{
    int text_id = widget_top_menu_get_tooltip_text(c);
    if (!text_id) {
        text_id = widget_sidebar_city_get_tooltip_text();
    }
    if (text_id) {
        c->type = TOOLTIP_BUTTON;
        c->text_id = text_id;
        return;
    }
    widget_city_get_tooltip(c);
}

void window_city_draw_all(void)
{
    window_city_draw_background();
    draw_foreground();
}

void window_city_show(void)
{
    if (formation_get_selected()) {
        formation_set_selected(0);
    }
    window_type window = {
        WINDOW_CITY,
        window_city_draw_background,
        draw_foreground,
        handle_input,
        get_tooltip
    };
    window_show(&window);
}

void window_city_military_show(int legion_formation_id)
{
    if (building_construction_type()) {
        building_construction_cancel();
        building_construction_clear_type();
    }
    formation_set_selected(legion_formation_id);
    window_type window = {
        WINDOW_CITY_MILITARY,
        window_city_draw_background,
        draw_foreground,
        handle_input_military,
        get_tooltip
    };
    window_show(&window);
}

void window_city_return(void)
{
    int formation_id = formation_get_selected();
    if (formation_id) {
        window_city_military_show(formation_id);
    } else {
        window_city_show();
    }
}
