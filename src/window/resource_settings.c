#include "resource_settings.h"

#include "building/count.h"
#include "city/data_private.h"
#include "city/resource.h"
#include "core/calc.h"
#include "core/image_group.h"
#include "empire/object.h"
#include "graphics/arrow_button.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/image_button.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "window/advisor/trade.h"
#include "window/message_dialog.h"

static void button_help_resource_settings(int param1, int param2);
static void button_export_amount_adjust(int value, int param2);

static void button_toggle_industry(int param1, int param2);
static void button_toggle_trade(int param1, int param2);
static void button_toggle_stockpile(int param1, int param2);

static struct generic_button_t resource_generic_buttons[] = {
    {104, 204, 432, 30, button_toggle_trade, button_none, 0, 0},
    {104, 240, 432, 30, button_toggle_industry, button_none, 0, 0},
    {104, 276, 432, 50, button_toggle_stockpile, button_none, 0, 0},
};

static struct arrow_button_t resource_arrow_buttons[] = {
    {310, 207, 17, 24, button_export_amount_adjust, -1, 0, 0, 0},
    {334, 207, 15, 24, button_export_amount_adjust, 1, 0, 0, 0}
};

static struct image_button_t help_button_resource_settings[] = {
    {64, 297, 27, 27, IB_NORMAL, GROUP_CONTEXT_ICONS, 0, button_help_resource_settings, button_none, 0, 0, 1, 0, 0, 0},
};

static struct {
    int resource;
    int focus_button_id;
} data;

static void init(int resource)
{
    data.resource = resource;
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(48, 100, 34, 15);

    // Resource image
    image_draw(resource_images[data.resource].icon_img_id + resource_image_offset(data.resource, RESOURCE_IMAGE_ICON), 64, 116);

    // Resource name
    lang_text_draw_centered(23, data.resource, 48, 116, 544, FONT_LARGE_BLACK);

    if (empire_can_produce_resource(data.resource)) {
        int total_buildings = building_count_industry_total(data.resource);
        int active_buildings = building_count_industry_active(data.resource);
        if (total_buildings <= 0) {
            // No industries in the city
            lang_text_draw(54, 7, 109, 164, FONT_NORMAL_BLACK);
        } else if (city_data.resource.mothballed[data.resource]) {
            // [count of] mothballed industry/ies in the city
            int width = text_draw_number(total_buildings, 0, " ", 109, 164, FONT_NORMAL_BLACK);
            if (total_buildings == 1) {
                lang_text_draw(54, 10, 109 + width, 164, FONT_NORMAL_BLACK);
            } else {
                lang_text_draw(54, 11, 109 + width, 164, FONT_NORMAL_BLACK);
            }
        } else {
            if (total_buildings == active_buildings) {
                // [count of] working industry/ies in the city
                int width = text_draw_number(total_buildings, 0, " ", 109, 164, FONT_NORMAL_BLACK);
                if (total_buildings == 1) {
                    lang_text_draw(54, 8, 109 + width, 164, FONT_NORMAL_BLACK);
                } else {
                    lang_text_draw(54, 9, 109 + width, 164, FONT_NORMAL_BLACK);
                }
            } else if (total_buildings > active_buildings) {
                // [count of] working [count of] idle industry/ies in the city
                int idle_buildings_count = total_buildings - active_buildings;
                int width = text_draw_number(active_buildings, 0, " ", 109, 164, FONT_NORMAL_BLACK);
                width += lang_text_draw(54, 12, 109 + width, 164, FONT_NORMAL_BLACK);
                width += text_draw_number(idle_buildings_count, 0, " ", 109 + width, 164, FONT_NORMAL_BLACK);
                if (idle_buildings_count == 1) {
                    lang_text_draw(54, 14, 109 + width, 164, FONT_NORMAL_BLACK);
                } else {
                    lang_text_draw(54, 13, 109 + width, 164, FONT_NORMAL_BLACK);
                }
            }
        }
    }

    // Units stored in the city's warehouses
    int width = lang_text_draw_amount(8, 10, city_data.resource.stored_in_warehouses[data.resource], 104, 180, FONT_NORMAL_BLACK);
    lang_text_draw(54, 15, 104 + width, 180, FONT_NORMAL_BLACK);

    // Import/Export
    int trade_flags = TRADE_STATUS_NONE;
    int trade_status = city_data.resource.trade_status[data.resource];
    if (resource_import_trade_route_open(data.resource)) {
        trade_flags |= TRADE_STATUS_IMPORT;
    }
    if (resource_export_trade_route_open(data.resource)) {
        trade_flags |= TRADE_STATUS_EXPORT;
    }
    if (!trade_flags) {
        // There are no trade routes open for these goods
        lang_text_draw(54, 24, 109, 212, FONT_NORMAL_BLACK);
    } else {
        button_border_draw(104, 204, 432, 30, data.focus_button_id == 1);
        switch (trade_status) {
            case TRADE_STATUS_NONE:
                // Not trading
                lang_text_draw_centered(54, 18, 104, 212, 432, FONT_NORMAL_BLACK);
                break;
            case TRADE_STATUS_IMPORT:
                // Importing goods
                lang_text_draw_centered(54, 19, 104, 212, 432, FONT_NORMAL_BLACK);
                break;
            case TRADE_STATUS_EXPORT:
                // Export goods over
                lang_text_draw(54, 20, 152, 212, FONT_NORMAL_BLACK);
                break;
        }
    }
    if (trade_status == TRADE_STATUS_EXPORT) {
        arrow_buttons_draw(0, 0, resource_arrow_buttons, 2);
        lang_text_draw_amount(8, 10, city_data.resource.export_over[data.resource], 365, 212, FONT_NORMAL_BLACK);
    }

    if (building_count_industry_total(data.resource) > 0) {
        button_border_draw(104, 240, 432, 30, data.focus_button_id == 2);
        if (city_data.resource.mothballed[data.resource]) {
            // Industry is OFF
            lang_text_draw_centered(54, 17, 104, 248, 432, FONT_NORMAL_BLACK);
        } else {
            // Industry is ON
            lang_text_draw_centered(54, 16, 104, 248, 432, FONT_NORMAL_BLACK);
        }
    }

    button_border_draw(104, 276, 432, 50, data.focus_button_id == 3);
    if (city_data.resource.stockpiled[data.resource]) {
        // Stockpiling resource
        lang_text_draw_centered(54, 26, 104, 284, 432, FONT_NORMAL_BLACK);
        // Click here to turn off stockpiling
        lang_text_draw_centered(54, 27, 104, 304, 432, FONT_NORMAL_BLACK);
    } else {
        // Using and trading this resource
        lang_text_draw_centered(54, 28, 104, 284, 432, FONT_NORMAL_BLACK);
        // Click here to stockpile it
        lang_text_draw_centered(54, 29, 104, 304, 432, FONT_NORMAL_BLACK);
    }

    image_buttons_draw(0, 0, help_button_resource_settings, 1);

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_go_back();
        return;
    }
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (city_data.resource.trade_status[data.resource] == TRADE_STATUS_EXPORT) {
        int button = 0;
        arrow_buttons_handle_mouse(m_dialog, 0, 0, resource_arrow_buttons, 2, &button);
        if (button) {
            return;
        }
    }
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, resource_generic_buttons, sizeof(resource_generic_buttons) / sizeof(struct generic_button_t), &data.focus_button_id)) {
        return;
    }
    if (image_buttons_handle_mouse(m_dialog, 0, 0, help_button_resource_settings, 1, 0)) {
        return;
    }
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 48 || m_dialog->y < 100 || m_dialog->x > 592 || m_dialog->y > 340)) {
        window_go_back();
        return;
    }
}

static void button_help_resource_settings(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_message_dialog_show(MESSAGE_DIALOG_INDUSTRY, 0);
}

static void button_export_amount_adjust(int value, __attribute__((unused)) int param2)
{
    city_data.resource.export_over[data.resource] = calc_bound(city_data.resource.export_over[data.resource] + value, 0, 100);
}

static void button_toggle_industry(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    if (building_count_industry_total(data.resource) > 0) {
        city_data.resource.mothballed[data.resource] = city_data.resource.mothballed[data.resource] ? 0 : 1;
    }
}

static void button_toggle_trade(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    city_resource_cycle_trade_status(data.resource);
}

static void button_toggle_stockpile(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    city_resource_toggle_stockpiled(data.resource);
}

void window_resource_settings_show(int resource)
{
    struct window_type_t window = {
        WINDOW_RESOURCE_SETTINGS,
        window_draw_underlying_window,
        draw_foreground,
        handle_input,
    };
    init(resource);
    window_show(&window);
}
