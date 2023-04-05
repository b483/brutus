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
#include "game/custom_strings.h"
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
        text_draw_centered(get_custom_string(TR_GAME_PAUSED), x_offset, 58, 448, FONT_NORMAL_BLACK, COLOR_BLACK);
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
    } else if (scenario_building_allowed(type) && building_menu_is_enabled(type)) {
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

static void cycle_buildings(int reverse)
{
    int building_cycling_index = building_construction_type();

    // TODO: simplify switches to -- ++ after fixing enum

    // prevent game from hanging in corner case where all buildings are disabled
    int retried = 0;

    if (reverse) {
    reverse_label:
        switch (building_cycling_index) {
            case BUILDING_WAREHOUSE:
                if (scenario_building_allowed(BUILDING_GRANARY)) {
                    building_cycling_index = BUILDING_GRANARY;
                    break;
                }
                /* fall through */
            case BUILDING_GRANARY:
                if (scenario_building_allowed(BUILDING_MARKET)) {
                    building_cycling_index = BUILDING_MARKET;
                    break;
                }
                /* fall through */
            case BUILDING_MARKET:
                if (scenario_building_allowed(BUILDING_POTTERY_WORKSHOP)) {
                    building_cycling_index = BUILDING_POTTERY_WORKSHOP;
                    break;
                }
                /* fall through */
            case BUILDING_POTTERY_WORKSHOP:
                if (scenario_building_allowed(BUILDING_FURNITURE_WORKSHOP)) {
                    building_cycling_index = BUILDING_FURNITURE_WORKSHOP;
                    break;
                }
                /* fall through */
            case BUILDING_FURNITURE_WORKSHOP:
                if (scenario_building_allowed(BUILDING_WEAPONS_WORKSHOP)) {
                    building_cycling_index = BUILDING_WEAPONS_WORKSHOP;
                    break;
                }
                /* fall through */
            case BUILDING_WEAPONS_WORKSHOP:
                if (scenario_building_allowed(BUILDING_OIL_WORKSHOP)) {
                    building_cycling_index = BUILDING_OIL_WORKSHOP;
                    break;
                }
                /* fall through */
            case BUILDING_OIL_WORKSHOP:
                if (scenario_building_allowed(BUILDING_WINE_WORKSHOP)) {
                    building_cycling_index = BUILDING_WINE_WORKSHOP;
                    break;
                }
                /* fall through */
            case BUILDING_WINE_WORKSHOP:
                if (scenario_building_allowed(BUILDING_TIMBER_YARD)) {
                    building_cycling_index = BUILDING_TIMBER_YARD;
                    break;
                }
                /* fall through */
            case BUILDING_TIMBER_YARD:
                if (scenario_building_allowed(BUILDING_IRON_MINE)) {
                    building_cycling_index = BUILDING_IRON_MINE;
                    break;
                }
                /* fall through */
            case BUILDING_IRON_MINE:
                if (scenario_building_allowed(BUILDING_MARBLE_QUARRY)) {
                    building_cycling_index = BUILDING_MARBLE_QUARRY;
                    break;
                }
                /* fall through */
            case BUILDING_MARBLE_QUARRY:
                if (scenario_building_allowed(BUILDING_CLAY_PIT)) {
                    building_cycling_index = BUILDING_CLAY_PIT;
                    break;
                }
                /* fall through */
            case BUILDING_CLAY_PIT:
                if (scenario_building_allowed(BUILDING_PIG_FARM)) {
                    building_cycling_index = BUILDING_PIG_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_PIG_FARM:
                if (scenario_building_allowed(BUILDING_VINES_FARM)) {
                    building_cycling_index = BUILDING_VINES_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_VINES_FARM:
                if (scenario_building_allowed(BUILDING_OLIVE_FARM)) {
                    building_cycling_index = BUILDING_OLIVE_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_OLIVE_FARM:
                if (scenario_building_allowed(BUILDING_FRUIT_FARM)) {
                    building_cycling_index = BUILDING_FRUIT_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_FRUIT_FARM:
                if (scenario_building_allowed(BUILDING_VEGETABLE_FARM)) {
                    building_cycling_index = BUILDING_VEGETABLE_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_VEGETABLE_FARM:
                if (scenario_building_allowed(BUILDING_WHEAT_FARM)) {
                    building_cycling_index = BUILDING_WHEAT_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_WHEAT_FARM:
                if (scenario_building_allowed(BUILDING_BARRACKS)) {
                    building_cycling_index = BUILDING_BARRACKS;
                    break;
                }
                /* fall through */
            case BUILDING_BARRACKS:
                if (scenario_building_allowed(BUILDING_MILITARY_ACADEMY)) {
                    building_cycling_index = BUILDING_MILITARY_ACADEMY;
                    break;
                }
                /* fall through */
            case BUILDING_MILITARY_ACADEMY:
                if (scenario_building_allowed(BUILDING_FORT_MOUNTED)) {
                    building_cycling_index = BUILDING_FORT_MOUNTED;
                    break;
                }
                /* fall through */
            case BUILDING_FORT_MOUNTED:
                if (scenario_building_allowed(BUILDING_FORT_JAVELIN)) {
                    building_cycling_index = BUILDING_FORT_JAVELIN;
                    break;
                }
                /* fall through */
            case BUILDING_FORT_JAVELIN:
                if (scenario_building_allowed(BUILDING_FORT_LEGIONARIES)) {
                    building_cycling_index = BUILDING_FORT_LEGIONARIES;
                    break;
                }
                /* fall through */
            case BUILDING_FORT_LEGIONARIES:
                if (scenario_building_allowed(BUILDING_PREFECTURE)) {
                    building_cycling_index = BUILDING_PREFECTURE;
                    break;
                }
                /* fall through */
            case BUILDING_PREFECTURE:
                if (scenario_building_allowed(BUILDING_GATEHOUSE)) {
                    building_cycling_index = BUILDING_GATEHOUSE;
                    break;
                }
                /* fall through */
            case BUILDING_GATEHOUSE:
                if (scenario_building_allowed(BUILDING_TOWER)) {
                    building_cycling_index = BUILDING_TOWER;
                    break;
                }
                /* fall through */
            case BUILDING_TOWER:
                if (scenario_building_allowed(BUILDING_WALL)) {
                    building_cycling_index = BUILDING_WALL;
                    break;
                }
                /* fall through */
            case BUILDING_WALL:
                if (scenario_building_allowed(BUILDING_WHARF)) {
                    building_cycling_index = BUILDING_WHARF;
                    break;
                }
                /* fall through */
            case BUILDING_WHARF:
                if (scenario_building_allowed(BUILDING_DOCK)) {
                    building_cycling_index = BUILDING_DOCK;
                    break;
                }
                /* fall through */
            case BUILDING_DOCK:
                if (scenario_building_allowed(BUILDING_SHIPYARD)) {
                    building_cycling_index = BUILDING_SHIPYARD;
                    break;
                }
                /* fall through */
            case BUILDING_SHIPYARD:
                if (scenario_building_allowed(BUILDING_SHIP_BRIDGE)) {
                    building_cycling_index = BUILDING_SHIP_BRIDGE;
                    break;
                }
                /* fall through */
            case BUILDING_SHIP_BRIDGE:
                if (scenario_building_allowed(BUILDING_LOW_BRIDGE)) {
                    building_cycling_index = BUILDING_LOW_BRIDGE;
                    break;
                }
                /* fall through */
            case BUILDING_LOW_BRIDGE:
                if (scenario_building_allowed(BUILDING_ENGINEERS_POST)) {
                    building_cycling_index = BUILDING_ENGINEERS_POST;
                    break;
                }
                /* fall through */
            case BUILDING_ENGINEERS_POST:
                if (scenario_building_allowed(BUILDING_PLAZA)) {
                    building_cycling_index = BUILDING_PLAZA;
                    break;
                }
                /* fall through */
            case BUILDING_PLAZA:
                if (scenario_building_allowed(BUILDING_GARDENS)) {
                    building_cycling_index = BUILDING_GARDENS;
                    break;
                }
                /* fall through */
            case BUILDING_GARDENS:
                if (scenario_building_allowed(BUILDING_LARGE_STATUE)) {
                    building_cycling_index = BUILDING_LARGE_STATUE;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_STATUE:
                if (scenario_building_allowed(BUILDING_MEDIUM_STATUE)) {
                    building_cycling_index = BUILDING_MEDIUM_STATUE;
                    break;
                }
                /* fall through */
            case BUILDING_MEDIUM_STATUE:
                if (scenario_building_allowed(BUILDING_SMALL_STATUE)) {
                    building_cycling_index = BUILDING_SMALL_STATUE;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_STATUE:
                if (scenario_building_allowed(BUILDING_GOVERNORS_PALACE)) {
                    building_cycling_index = BUILDING_GOVERNORS_PALACE;
                    break;
                }
                /* fall through */
            case BUILDING_GOVERNORS_PALACE:
                if (scenario_building_allowed(BUILDING_GOVERNORS_VILLA)) {
                    building_cycling_index = BUILDING_GOVERNORS_VILLA;
                    break;
                }
                /* fall through */
            case BUILDING_GOVERNORS_VILLA:
                if (scenario_building_allowed(BUILDING_GOVERNORS_HOUSE)) {
                    building_cycling_index = BUILDING_GOVERNORS_HOUSE;
                    break;
                }
                /* fall through */
            case BUILDING_GOVERNORS_HOUSE:
                if (scenario_building_allowed(BUILDING_SENATE_UPGRADED)) {
                    building_cycling_index = BUILDING_SENATE_UPGRADED;
                    break;
                }
                /* fall through */
            case BUILDING_SENATE_UPGRADED:
                if (scenario_building_allowed(BUILDING_FORUM)) {
                    building_cycling_index = BUILDING_FORUM;
                    break;
                }
                /* fall through */
            case BUILDING_FORUM:
                if (scenario_building_allowed(BUILDING_CHARIOT_MAKER)) {
                    building_cycling_index = BUILDING_CHARIOT_MAKER;
                    break;
                }
                /* fall through */
            case BUILDING_CHARIOT_MAKER:
                if (scenario_building_allowed(BUILDING_ACTOR_COLONY)) {
                    building_cycling_index = BUILDING_ACTOR_COLONY;
                    break;
                }
                /* fall through */
            case BUILDING_ACTOR_COLONY:
                if (scenario_building_allowed(BUILDING_LION_HOUSE)) {
                    building_cycling_index = BUILDING_LION_HOUSE;
                    break;
                }
                /* fall through */
            case BUILDING_LION_HOUSE:
                if (scenario_building_allowed(BUILDING_GLADIATOR_SCHOOL)) {
                    building_cycling_index = BUILDING_GLADIATOR_SCHOOL;
                    break;
                }
                /* fall through */
            case BUILDING_GLADIATOR_SCHOOL:
                if (scenario_building_allowed(BUILDING_HIPPODROME)) {
                    building_cycling_index = BUILDING_HIPPODROME;
                    break;
                }
                /* fall through */
            case BUILDING_HIPPODROME:
                if (scenario_building_allowed(BUILDING_COLOSSEUM)) {
                    building_cycling_index = BUILDING_COLOSSEUM;
                    break;
                }
                /* fall through */
            case BUILDING_COLOSSEUM:
                if (scenario_building_allowed(BUILDING_AMPHITHEATER)) {
                    building_cycling_index = BUILDING_AMPHITHEATER;
                    break;
                }
                /* fall through */
            case BUILDING_AMPHITHEATER:
                if (scenario_building_allowed(BUILDING_THEATER)) {
                    building_cycling_index = BUILDING_THEATER;
                    break;
                }
                /* fall through */
            case BUILDING_THEATER:
                if (scenario_building_allowed(BUILDING_MISSION_POST)) {
                    building_cycling_index = BUILDING_MISSION_POST;
                    break;
                }
                /* fall through */
            case BUILDING_MISSION_POST:
                if (scenario_building_allowed(BUILDING_LIBRARY)) {
                    building_cycling_index = BUILDING_LIBRARY;
                    break;
                }
                /* fall through */
            case BUILDING_LIBRARY:
                if (scenario_building_allowed(BUILDING_ACADEMY)) {
                    building_cycling_index = BUILDING_ACADEMY;
                    break;
                }
                /* fall through */
            case BUILDING_ACADEMY:
                if (scenario_building_allowed(BUILDING_SCHOOL)) {
                    building_cycling_index = BUILDING_SCHOOL;
                    break;
                }
                /* fall through */
            case BUILDING_SCHOOL:
                if (scenario_building_allowed(BUILDING_ORACLE)) {
                    building_cycling_index = BUILDING_ORACLE;
                    break;
                }
                /* fall through */
            case BUILDING_ORACLE:
                if (scenario_building_allowed(BUILDING_LARGE_TEMPLE_VENUS)) {
                    building_cycling_index = BUILDING_LARGE_TEMPLE_VENUS;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_TEMPLE_VENUS:
                if (scenario_building_allowed(BUILDING_LARGE_TEMPLE_MARS)) {
                    building_cycling_index = BUILDING_LARGE_TEMPLE_MARS;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_TEMPLE_MARS:
                if (scenario_building_allowed(BUILDING_LARGE_TEMPLE_MERCURY)) {
                    building_cycling_index = BUILDING_LARGE_TEMPLE_MERCURY;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_TEMPLE_MERCURY:
                if (scenario_building_allowed(BUILDING_LARGE_TEMPLE_NEPTUNE)) {
                    building_cycling_index = BUILDING_LARGE_TEMPLE_NEPTUNE;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_TEMPLE_NEPTUNE:
                if (scenario_building_allowed(BUILDING_LARGE_TEMPLE_CERES)) {
                    building_cycling_index = BUILDING_LARGE_TEMPLE_CERES;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_TEMPLE_CERES:
                if (scenario_building_allowed(BUILDING_SMALL_TEMPLE_VENUS)) {
                    building_cycling_index = BUILDING_SMALL_TEMPLE_VENUS;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_TEMPLE_VENUS:
                if (scenario_building_allowed(BUILDING_SMALL_TEMPLE_MARS)) {
                    building_cycling_index = BUILDING_SMALL_TEMPLE_MARS;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_TEMPLE_MARS:
                if (scenario_building_allowed(BUILDING_SMALL_TEMPLE_MERCURY)) {
                    building_cycling_index = BUILDING_SMALL_TEMPLE_MERCURY;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_TEMPLE_MERCURY:
                if (scenario_building_allowed(BUILDING_SMALL_TEMPLE_NEPTUNE)) {
                    building_cycling_index = BUILDING_SMALL_TEMPLE_NEPTUNE;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_TEMPLE_NEPTUNE:
                if (scenario_building_allowed(BUILDING_SMALL_TEMPLE_CERES)) {
                    building_cycling_index = BUILDING_SMALL_TEMPLE_CERES;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_TEMPLE_CERES:
                if (scenario_building_allowed(BUILDING_HOSPITAL)) {
                    building_cycling_index = BUILDING_HOSPITAL;
                    break;
                }
                /* fall through */
            case BUILDING_HOSPITAL:
                if (scenario_building_allowed(BUILDING_DOCTOR)) {
                    building_cycling_index = BUILDING_DOCTOR;
                    break;
                }
                /* fall through */
            case BUILDING_DOCTOR:
                if (scenario_building_allowed(BUILDING_BATHHOUSE)) {
                    building_cycling_index = BUILDING_BATHHOUSE;
                    break;
                }
                /* fall through */
            case BUILDING_BATHHOUSE:
                if (scenario_building_allowed(BUILDING_BARBER)) {
                    building_cycling_index = BUILDING_BARBER;
                    break;
                }
                /* fall through */
            case BUILDING_BARBER:
                if (scenario_building_allowed(BUILDING_WELL)) {
                    building_cycling_index = BUILDING_WELL;
                    break;
                }
                /* fall through */
            case BUILDING_WELL:
                if (scenario_building_allowed(BUILDING_FOUNTAIN)) {
                    building_cycling_index = BUILDING_FOUNTAIN;
                    break;
                }
                /* fall through */
            case BUILDING_FOUNTAIN:
                if (scenario_building_allowed(BUILDING_AQUEDUCT)) {
                    building_cycling_index = BUILDING_AQUEDUCT;
                    break;
                }
                /* fall through */
            case BUILDING_AQUEDUCT:
                if (scenario_building_allowed(BUILDING_RESERVOIR)) {
                    building_cycling_index = BUILDING_RESERVOIR;
                    break;
                }
                /* fall through */
            case BUILDING_RESERVOIR:
                if (scenario_building_allowed(BUILDING_WAREHOUSE)) {
                    building_cycling_index = BUILDING_WAREHOUSE;
                    break;
                }
                /* fall through */
            default:
                if (scenario_building_allowed(BUILDING_WAREHOUSE)) {
                    building_cycling_index = BUILDING_WAREHOUSE;
                    break;
                } else {
                    if (retried) {
                        return;
                    } else {
                        retried = 1;
                        goto reverse_label;
                    }
                }
        }
    } else {
        switch (building_cycling_index) {
        obverse_label:
            case BUILDING_RESERVOIR:
                if (scenario_building_allowed(BUILDING_AQUEDUCT)) {
                    building_cycling_index = BUILDING_AQUEDUCT;
                    break;
                }
                /* fall through */
            case BUILDING_AQUEDUCT:
                if (scenario_building_allowed(BUILDING_FOUNTAIN)) {
                    building_cycling_index = BUILDING_FOUNTAIN;
                    break;
                }
                /* fall through */
            case BUILDING_FOUNTAIN:
                if (scenario_building_allowed(BUILDING_WELL)) {
                    building_cycling_index = BUILDING_WELL;
                    break;
                }
                /* fall through */
            case BUILDING_WELL:
                if (scenario_building_allowed(BUILDING_BARBER)) {
                    building_cycling_index = BUILDING_BARBER;
                    break;
                }
                /* fall through */
            case BUILDING_BARBER:
                if (scenario_building_allowed(BUILDING_BATHHOUSE)) {
                    building_cycling_index = BUILDING_BATHHOUSE;
                    break;
                }
                /* fall through */
            case BUILDING_BATHHOUSE:
                if (scenario_building_allowed(BUILDING_DOCTOR)) {
                    building_cycling_index = BUILDING_DOCTOR;
                    break;
                }
                /* fall through */
            case BUILDING_DOCTOR:
                if (scenario_building_allowed(BUILDING_HOSPITAL)) {
                    building_cycling_index = BUILDING_HOSPITAL;
                    break;
                }
                /* fall through */
            case BUILDING_HOSPITAL:
                if (scenario_building_allowed(BUILDING_SMALL_TEMPLE_CERES)) {
                    building_cycling_index = BUILDING_SMALL_TEMPLE_CERES;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_TEMPLE_CERES:
                if (scenario_building_allowed(BUILDING_SMALL_TEMPLE_NEPTUNE)) {
                    building_cycling_index = BUILDING_SMALL_TEMPLE_NEPTUNE;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_TEMPLE_NEPTUNE:
                if (scenario_building_allowed(BUILDING_SMALL_TEMPLE_MERCURY)) {
                    building_cycling_index = BUILDING_SMALL_TEMPLE_MERCURY;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_TEMPLE_MERCURY:
                if (scenario_building_allowed(BUILDING_SMALL_TEMPLE_MARS)) {
                    building_cycling_index = BUILDING_SMALL_TEMPLE_MARS;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_TEMPLE_MARS:
                if (scenario_building_allowed(BUILDING_SMALL_TEMPLE_VENUS)) {
                    building_cycling_index = BUILDING_SMALL_TEMPLE_VENUS;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_TEMPLE_VENUS:
                if (scenario_building_allowed(BUILDING_LARGE_TEMPLE_CERES)) {
                    building_cycling_index = BUILDING_LARGE_TEMPLE_CERES;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_TEMPLE_CERES:
                if (scenario_building_allowed(BUILDING_LARGE_TEMPLE_NEPTUNE)) {
                    building_cycling_index = BUILDING_LARGE_TEMPLE_NEPTUNE;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_TEMPLE_NEPTUNE:
                if (scenario_building_allowed(BUILDING_LARGE_TEMPLE_MERCURY)) {
                    building_cycling_index = BUILDING_LARGE_TEMPLE_MERCURY;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_TEMPLE_MERCURY:
                if (scenario_building_allowed(BUILDING_LARGE_TEMPLE_MARS)) {
                    building_cycling_index = BUILDING_LARGE_TEMPLE_MARS;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_TEMPLE_MARS:
                if (scenario_building_allowed(BUILDING_LARGE_TEMPLE_VENUS)) {
                    building_cycling_index = BUILDING_LARGE_TEMPLE_VENUS;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_TEMPLE_VENUS:
                if (scenario_building_allowed(BUILDING_ORACLE)) {
                    building_cycling_index = BUILDING_ORACLE;
                    break;
                }
                /* fall through */
            case BUILDING_ORACLE:
                if (scenario_building_allowed(BUILDING_SCHOOL)) {
                    building_cycling_index = BUILDING_SCHOOL;
                    break;
                }
                /* fall through */
            case BUILDING_SCHOOL:
                if (scenario_building_allowed(BUILDING_ACADEMY)) {
                    building_cycling_index = BUILDING_ACADEMY;
                    break;
                }
                /* fall through */
            case BUILDING_ACADEMY:
                if (scenario_building_allowed(BUILDING_LIBRARY)) {
                    building_cycling_index = BUILDING_LIBRARY;
                    break;
                }
                /* fall through */
            case BUILDING_LIBRARY:
                if (scenario_building_allowed(BUILDING_MISSION_POST)) {
                    building_cycling_index = BUILDING_MISSION_POST;
                    break;
                }
                /* fall through */
            case BUILDING_MISSION_POST:
                if (scenario_building_allowed(BUILDING_THEATER)) {
                    building_cycling_index = BUILDING_THEATER;
                    break;
                }
                /* fall through */
            case BUILDING_THEATER:
                if (scenario_building_allowed(BUILDING_AMPHITHEATER)) {
                    building_cycling_index = BUILDING_AMPHITHEATER;
                    break;
                }
                /* fall through */
            case BUILDING_AMPHITHEATER:
                if (scenario_building_allowed(BUILDING_COLOSSEUM)) {
                    building_cycling_index = BUILDING_COLOSSEUM;
                    break;
                }
                /* fall through */
            case BUILDING_COLOSSEUM:
                if (scenario_building_allowed(BUILDING_HIPPODROME)) {
                    building_cycling_index = BUILDING_HIPPODROME;
                    break;
                }
                /* fall through */
            case BUILDING_HIPPODROME:
                if (scenario_building_allowed(BUILDING_GLADIATOR_SCHOOL)) {
                    building_cycling_index = BUILDING_GLADIATOR_SCHOOL;
                    break;
                }
                /* fall through */
            case BUILDING_GLADIATOR_SCHOOL:
                if (scenario_building_allowed(BUILDING_LION_HOUSE)) {
                    building_cycling_index = BUILDING_LION_HOUSE;
                    break;
                }
                /* fall through */
            case BUILDING_LION_HOUSE:
                if (scenario_building_allowed(BUILDING_ACTOR_COLONY)) {
                    building_cycling_index = BUILDING_ACTOR_COLONY;
                    break;
                }
                /* fall through */
            case BUILDING_ACTOR_COLONY:
                if (scenario_building_allowed(BUILDING_CHARIOT_MAKER)) {
                    building_cycling_index = BUILDING_CHARIOT_MAKER;
                    break;
                }
                /* fall through */
            case BUILDING_CHARIOT_MAKER:
                if (scenario_building_allowed(BUILDING_FORUM)) {
                    building_cycling_index = BUILDING_FORUM;
                    break;
                }
                /* fall through */
            case BUILDING_FORUM:
                if (scenario_building_allowed(BUILDING_SENATE_UPGRADED)) {
                    building_cycling_index = BUILDING_SENATE_UPGRADED;
                    break;
                }
                /* fall through */
            case BUILDING_SENATE_UPGRADED:
                if (scenario_building_allowed(BUILDING_GOVERNORS_HOUSE)) {
                    building_cycling_index = BUILDING_GOVERNORS_HOUSE;
                    break;
                }
                /* fall through */
            case BUILDING_GOVERNORS_HOUSE:
                if (scenario_building_allowed(BUILDING_GOVERNORS_VILLA)) {
                    building_cycling_index = BUILDING_GOVERNORS_VILLA;
                    break;
                }
                /* fall through */
            case BUILDING_GOVERNORS_VILLA:
                if (scenario_building_allowed(BUILDING_GOVERNORS_PALACE)) {
                    building_cycling_index = BUILDING_GOVERNORS_PALACE;
                    break;
                }
                /* fall through */
            case BUILDING_GOVERNORS_PALACE:
                if (scenario_building_allowed(BUILDING_SMALL_STATUE)) {
                    building_cycling_index = BUILDING_SMALL_STATUE;
                    break;
                }
                /* fall through */
            case BUILDING_SMALL_STATUE:
                if (scenario_building_allowed(BUILDING_MEDIUM_STATUE)) {
                    building_cycling_index = BUILDING_MEDIUM_STATUE;
                    break;
                }
                /* fall through */
            case BUILDING_MEDIUM_STATUE:
                if (scenario_building_allowed(BUILDING_LARGE_STATUE)) {
                    building_cycling_index = BUILDING_LARGE_STATUE;
                    break;
                }
                /* fall through */
            case BUILDING_LARGE_STATUE:
                if (scenario_building_allowed(BUILDING_GARDENS)) {
                    building_cycling_index = BUILDING_GARDENS;
                    break;
                }
                /* fall through */
            case BUILDING_GARDENS:
                if (scenario_building_allowed(BUILDING_PLAZA)) {
                    building_cycling_index = BUILDING_PLAZA;
                    break;
                }
                /* fall through */
            case BUILDING_PLAZA:
                if (scenario_building_allowed(BUILDING_ENGINEERS_POST)) {
                    building_cycling_index = BUILDING_ENGINEERS_POST;
                    break;
                }
                /* fall through */
            case BUILDING_ENGINEERS_POST:
                if (scenario_building_allowed(BUILDING_LOW_BRIDGE)) {
                    building_cycling_index = BUILDING_LOW_BRIDGE;
                    break;
                }
                /* fall through */
            case BUILDING_LOW_BRIDGE:
                if (scenario_building_allowed(BUILDING_SHIP_BRIDGE)) {
                    building_cycling_index = BUILDING_SHIP_BRIDGE;
                    break;
                }
                /* fall through */
            case BUILDING_SHIP_BRIDGE:
                if (scenario_building_allowed(BUILDING_SHIPYARD)) {
                    building_cycling_index = BUILDING_SHIPYARD;
                    break;
                }
                /* fall through */
            case BUILDING_SHIPYARD:
                if (scenario_building_allowed(BUILDING_DOCK)) {
                    building_cycling_index = BUILDING_DOCK;
                    break;
                }
                /* fall through */
            case BUILDING_DOCK:
                if (scenario_building_allowed(BUILDING_WHARF)) {
                    building_cycling_index = BUILDING_WHARF;
                    break;
                }
                /* fall through */
            case BUILDING_WHARF:
                if (scenario_building_allowed(BUILDING_WALL)) {
                    building_cycling_index = BUILDING_WALL;
                    break;
                }
                /* fall through */
            case BUILDING_WALL:
                if (scenario_building_allowed(BUILDING_TOWER)) {
                    building_cycling_index = BUILDING_TOWER;
                    break;
                }
                /* fall through */
            case BUILDING_TOWER:
                if (scenario_building_allowed(BUILDING_GATEHOUSE)) {
                    building_cycling_index = BUILDING_GATEHOUSE;
                    break;
                }
                /* fall through */
            case BUILDING_GATEHOUSE:
                if (scenario_building_allowed(BUILDING_PREFECTURE)) {
                    building_cycling_index = BUILDING_PREFECTURE;
                    break;
                }
                /* fall through */
            case BUILDING_PREFECTURE:
                if (scenario_building_allowed(BUILDING_FORT_LEGIONARIES)) {
                    building_cycling_index = BUILDING_FORT_LEGIONARIES;
                    break;
                }
                /* fall through */
            case BUILDING_FORT_LEGIONARIES:
                if (scenario_building_allowed(BUILDING_FORT_JAVELIN)) {
                    building_cycling_index = BUILDING_FORT_JAVELIN;
                    break;
                }
                /* fall through */
            case BUILDING_FORT_JAVELIN:
                if (scenario_building_allowed(BUILDING_FORT_MOUNTED)) {
                    building_cycling_index = BUILDING_FORT_MOUNTED;
                    break;
                }
                /* fall through */
            case BUILDING_FORT_MOUNTED:
                if (scenario_building_allowed(BUILDING_MILITARY_ACADEMY)) {
                    building_cycling_index = BUILDING_MILITARY_ACADEMY;
                    break;
                }
                /* fall through */
            case BUILDING_MILITARY_ACADEMY:
                if (scenario_building_allowed(BUILDING_BARRACKS)) {
                    building_cycling_index = BUILDING_BARRACKS;
                    break;
                }
                /* fall through */
            case BUILDING_BARRACKS:
                if (scenario_building_allowed(BUILDING_WHEAT_FARM)) {
                    building_cycling_index = BUILDING_WHEAT_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_WHEAT_FARM:
                if (scenario_building_allowed(BUILDING_VEGETABLE_FARM)) {
                    building_cycling_index = BUILDING_VEGETABLE_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_VEGETABLE_FARM:
                if (scenario_building_allowed(BUILDING_FRUIT_FARM)) {
                    building_cycling_index = BUILDING_FRUIT_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_FRUIT_FARM:
                if (scenario_building_allowed(BUILDING_OLIVE_FARM)) {
                    building_cycling_index = BUILDING_OLIVE_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_OLIVE_FARM:
                if (scenario_building_allowed(BUILDING_VINES_FARM)) {
                    building_cycling_index = BUILDING_VINES_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_VINES_FARM:
                if (scenario_building_allowed(BUILDING_PIG_FARM)) {
                    building_cycling_index = BUILDING_PIG_FARM;
                    break;
                }
                /* fall through */
            case BUILDING_PIG_FARM:
                if (scenario_building_allowed(BUILDING_CLAY_PIT)) {
                    building_cycling_index = BUILDING_CLAY_PIT;
                    break;
                }
                /* fall through */
            case BUILDING_CLAY_PIT:
                if (scenario_building_allowed(BUILDING_MARBLE_QUARRY)) {
                    building_cycling_index = BUILDING_MARBLE_QUARRY;
                    break;
                }
                /* fall through */
            case BUILDING_MARBLE_QUARRY:
                if (scenario_building_allowed(BUILDING_IRON_MINE)) {
                    building_cycling_index = BUILDING_IRON_MINE;
                    break;
                }
                /* fall through */
            case BUILDING_IRON_MINE:
                if (scenario_building_allowed(BUILDING_TIMBER_YARD)) {
                    building_cycling_index = BUILDING_TIMBER_YARD;
                    break;
                }
                /* fall through */
            case BUILDING_TIMBER_YARD:
                if (scenario_building_allowed(BUILDING_WINE_WORKSHOP)) {
                    building_cycling_index = BUILDING_WINE_WORKSHOP;
                    break;
                }
                /* fall through */
            case BUILDING_WINE_WORKSHOP:
                if (scenario_building_allowed(BUILDING_OIL_WORKSHOP)) {
                    building_cycling_index = BUILDING_OIL_WORKSHOP;
                    break;
                }
                /* fall through */
            case BUILDING_OIL_WORKSHOP:
                if (scenario_building_allowed(BUILDING_WEAPONS_WORKSHOP)) {
                    building_cycling_index = BUILDING_WEAPONS_WORKSHOP;
                    break;
                }
                /* fall through */
            case BUILDING_WEAPONS_WORKSHOP:
                if (scenario_building_allowed(BUILDING_FURNITURE_WORKSHOP)) {
                    building_cycling_index = BUILDING_FURNITURE_WORKSHOP;
                    break;
                }
                /* fall through */
            case BUILDING_FURNITURE_WORKSHOP:
                if (scenario_building_allowed(BUILDING_POTTERY_WORKSHOP)) {
                    building_cycling_index = BUILDING_POTTERY_WORKSHOP;
                    break;
                }
                /* fall through */
            case BUILDING_POTTERY_WORKSHOP:
                if (scenario_building_allowed(BUILDING_MARKET)) {
                    building_cycling_index = BUILDING_MARKET;
                    break;
                }
                /* fall through */
            case BUILDING_MARKET:
                if (scenario_building_allowed(BUILDING_GRANARY)) {
                    building_cycling_index = BUILDING_GRANARY;
                    break;
                }
                /* fall through */
            case BUILDING_GRANARY:
                if (scenario_building_allowed(BUILDING_WAREHOUSE)) {
                    building_cycling_index = BUILDING_WAREHOUSE;
                    break;
                }
                /* fall through */
            case BUILDING_WAREHOUSE:
                if (scenario_building_allowed(BUILDING_RESERVOIR)) {
                    building_cycling_index = BUILDING_RESERVOIR;
                    break;
                }
                /* fall through */
            default:
                if (scenario_building_allowed(BUILDING_RESERVOIR)) {
                    building_cycling_index = BUILDING_RESERVOIR;
                    break;
                } else {
                    if (retried) {
                        return;
                    } else {
                        retried = 1;
                        goto obverse_label;
                    }
                }
        }
    }
    set_construction_building_type(building_cycling_index);
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
        cycle_buildings(0);
    }
    if (h->cycle_buildings_reverse) {
        cycle_buildings(1);
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
        city_finance_process_cheat();
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
