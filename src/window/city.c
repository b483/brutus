#include "city.h"

#include "building/clone.h"
#include "building/construction.h"
#include "building/menu.h"
#include "city/data_private.h"
#include "city/message.h"
#include "city/victory.h"
#include "city/view.h"
#include "city/warning.h"
#include "core/config.h"
#include "figure/formation.h"
#include "figure/formation_legion.h"
#include "game/file.h"
#include "game/orientation.h"
#include "game/settings.h"
#include "game/state.h"
#include "game/time.h"
#include "game/undo.h"
#include "graphics/graphics.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/screen.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "map/bookmark.h"
#include "map/grid.h"
#include "scenario/data.h"
#include "scenario/editor_events.h"
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

static uint8_t pause_string[] = "Game paused";

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
        text_draw_centered(pause_string, x_offset, 58, 448, FONT_NORMAL_BLACK, COLOR_BLACK);
    }
}

static void draw_time_left(void)
{
    if (scenario.time_limit_win_criteria.enabled && !city_data.mission.has_won) {
        int remaining_months;
        if (scenario.start_year + scenario.time_limit_win_criteria.years <= game_time_year() + 1) {
            remaining_months = 12 - game_time_month();
        } else {
            remaining_months = 12 - game_time_month() + 12 * (scenario.start_year + scenario.time_limit_win_criteria.years - game_time_year() - 1);
        }
        label_draw(1, 25, 15, 1);
        int width = lang_text_draw(6, 2, 6, 29, FONT_NORMAL_BLACK);
        text_draw_number(remaining_months, ' ', 0, 6 + width, 29, FONT_NORMAL_BLACK);
    } else if (scenario.survival_time_win_criteria.enabled && !city_data.mission.has_won) {
        int remaining_months;
        if (scenario.start_year + scenario.survival_time_win_criteria.years <= game_time_year() + 1) {
            remaining_months = 12 - game_time_month();
        } else {
            remaining_months = 12 - game_time_month() + 12 * (scenario.start_year + scenario.survival_time_win_criteria.years - game_time_year() - 1);
        }
        label_draw(1, 25, 15, 1);
        int width = lang_text_draw(6, 3, 6, 29, FONT_NORMAL_BLACK);
        text_draw_number(remaining_months, ' ', 0, 6 + width, 29, FONT_NORMAL_BLACK);
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
    int n_legions = formation_get_num_legions();
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
        current_selected_legion_formation_id = get_legion_formation_by_index(current_selected_legion_index);
        if (formations[current_selected_legion_formation_id].in_distant_battle || !formations[current_selected_legion_formation_id].num_figures) {
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

static void toggle_pause(void)
{
    game_state_toggle_paused();
    city_warning_clear_all();
}

static void set_construction_building_type(building_type type)
{
    if (type == BUILDING_CLEAR_LAND) {
        building_construction_cancel();
        building_construction_set_type(type);
        window_request_refresh();
    } else if (scenario.allowed_buildings[type] && building_menu_is_enabled(type)) {
        building_construction_cancel();
        building_construction_set_type(type);
        window_request_refresh();
    }
}

static void replay_map_confirmed(void)
{
    if (game_file_start_scenario((char *) scenario.scenario_name)) {
        window_city_show();
    }
}

void replay_map(void)
{
    building_construction_clear_type();
    window_popup_dialog_show_confirmation(1, 2, replay_map_confirmed);
}

static void cycle_buildings(void)
{
    int last_building_type_selected = building_construction_type();

    if (last_building_type_selected < BUILDING_RESERVOIR) {
        last_building_type_selected = BUILDING_RESERVOIR;
    } else {
        last_building_type_selected++;
    }
    while (last_building_type_selected <= BUILDING_WAREHOUSE) {
        if (last_building_type_selected == BUILDING_MENU_SMALL_TEMPLES
        || last_building_type_selected == BUILDING_MENU_LARGE_TEMPLES
        || last_building_type_selected == BUILDING_FORT
        || last_building_type_selected == BUILDING_MENU_FARMS
        || last_building_type_selected == BUILDING_MENU_RAW_MATERIALS
        || last_building_type_selected == BUILDING_MENU_WORKSHOPS) {
            last_building_type_selected++;
        }
        if (last_building_type_selected == BUILDING_TRIUMPHAL_ARCH) {
            if (city_data.building.triumphal_arches_available <= city_data.building.triumphal_arches_placed) {
                last_building_type_selected++;
            }
        }
        if (scenario.allowed_buildings[last_building_type_selected]) {
            set_construction_building_type(last_building_type_selected);
            break;
        }
        last_building_type_selected++;
    }
}

static void cycle_buildings_reverse(void)
{
    int last_building_type_selected = building_construction_type();

    if (last_building_type_selected < BUILDING_RESERVOIR) {
        last_building_type_selected = BUILDING_WAREHOUSE;
    } else {
        last_building_type_selected--;
    }
    while (last_building_type_selected >= BUILDING_RESERVOIR) {
        if (last_building_type_selected == BUILDING_MENU_SMALL_TEMPLES
        || last_building_type_selected == BUILDING_MENU_LARGE_TEMPLES
        || last_building_type_selected == BUILDING_FORT
        || last_building_type_selected == BUILDING_MENU_FARMS
        || last_building_type_selected == BUILDING_MENU_RAW_MATERIALS
        || last_building_type_selected == BUILDING_MENU_WORKSHOPS) {
            last_building_type_selected--;
        }
        if (last_building_type_selected == BUILDING_TRIUMPHAL_ARCH) {
            if (city_data.building.triumphal_arches_available <= city_data.building.triumphal_arches_placed) {
                last_building_type_selected--;
            }
        }
        if (scenario.allowed_buildings[last_building_type_selected]) {
            set_construction_building_type(last_building_type_selected);
            break;
        }
        last_building_type_selected--;
    }
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
        for (int i = 1; i < MAX_FORMATIONS; i++) {
            if (formations[i].in_use && formations[i].is_legion && !formations[i].in_distant_battle && !formations[i].is_at_rest) {
                formation_legion_return_home(&formations[i]);
            }
        }
    }
    if (h->show_last_advisor) {
        window_advisors_show(setting_last_advisor());
    }
    if (h->show_empire_map) {
        window_empire_show();
    }
    if (h->show_messages) {
        window_message_list_show();
    }
    if (h->go_to_problem) {
        button_go_to_problem(0, 0);
    }
    if (h->clone_building) {
        building_type type = building_clone_type_from_grid_offset(widget_city_current_grid_offset());
        if (type) {
            set_construction_building_type(type);
        }
    }
    if (h->cycle_buildings) {
        cycle_buildings();
    }
    if (h->cycle_buildings_reverse) {
        cycle_buildings_reverse();
    }
    if (h->undo) {
        game_undo_perform();
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
        if (city_data.finance.treasury < 50000) {
            city_data.finance.treasury += 1000;
            city_data.finance.cheated_money += 1000;
        }
        window_invalidate();
    }
    if (h->cheat_invasion) {
        scenario_invasion_start_from_cheat();
    }
    if (h->cheat_victory) {
        city_victory_force_win();
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
