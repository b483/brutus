#include "trade.h"

#include "city/data.h"
#include "city/resource.h"
#include "city/resource.h"
#include "graphics/generic_button.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "window/empire.h"
#include "window/resource_settings.h"
#include "window/trade_prices.h"

#define ADVISOR_HEIGHT 27

static void button_prices(int param1, int param2);
static void button_empire(int param1, int param2);
static void button_resource(int resource_index, int param2);

static struct generic_button_t resource_buttons[] = {
    {400, 398, 200, 23, button_prices, button_none, 1, 0},
    {100, 398, 200, 23, button_empire, button_none, 1, 0},
    {80, 56, 480, 20, button_resource, button_none, 0, 0},
    {80, 78, 480, 20, button_resource, button_none, 1, 0},
    {80, 100, 480, 20, button_resource, button_none, 2, 0},
    {80, 122, 480, 20, button_resource, button_none, 3, 0},
    {80, 144, 480, 20, button_resource, button_none, 4, 0},
    {80, 166, 480, 20, button_resource, button_none, 5, 0},
    {80, 188, 480, 20, button_resource, button_none, 6, 0},
    {80, 210, 480, 20, button_resource, button_none, 7, 0},
    {80, 232, 480, 20, button_resource, button_none, 8, 0},
    {80, 254, 480, 20, button_resource, button_none, 9, 0},
    {80, 276, 480, 20, button_resource, button_none, 10, 0},
    {80, 298, 480, 20, button_resource, button_none, 11, 0},
    {80, 320, 480, 20, button_resource, button_none, 12, 0},
    {80, 342, 480, 20, button_resource, button_none, 13, 0},
    {80, 364, 480, 20, button_resource, button_none, 14, 0}
};

static int focus_button_id;

static int draw_background(void)
{
    city_resource_determine_available();

    outer_panel_draw(0, 0, 40, ADVISOR_HEIGHT);
    image_draw(image_group(GROUP_ADVISOR_ICONS) + 4, 10, 10);

    lang_text_draw(54, 0, 60, 12, FONT_LARGE_BLACK);
    int width = lang_text_get_width(54, 1, FONT_NORMAL_BLACK);
    lang_text_draw(54, 1, 600 - width, 38, FONT_NORMAL_BLACK);

    return ADVISOR_HEIGHT;
}

static void draw_foreground(void)
{
    inner_panel_draw(32, 52, 36, 21);
    struct resource_list_t *list = city_resource_get_available();
    for (int i = 0; i < list->size; i++) {
        int y_offset = 22 * i;
        int resource = list->items[i];
        image_draw(resource_images[resource].icon_img_id + resource_image_offset(resource, RESOURCE_IMAGE_ICON), 48, y_offset + 54);
        image_draw(resource_images[resource].icon_img_id + resource_image_offset(resource, RESOURCE_IMAGE_ICON), 568, y_offset + 54);

        if (focus_button_id - 3 == i) {
            button_border_draw(80, y_offset + 54, 480, 24, 1);
        }
        lang_text_draw(23, resource, 88, y_offset + 61, FONT_NORMAL_WHITE);
        text_draw_number_centered(city_data.resource.stored_in_warehouses[resource],
            180, y_offset + 61, 60, FONT_NORMAL_WHITE);
        if (city_data.resource.mothballed[resource]) {
            lang_text_draw_centered(18, 5, 240, y_offset + 61, 100, FONT_NORMAL_WHITE);
        }
        if (city_data.resource.stockpiled[resource]) {
            lang_text_draw(54, 3, 340, y_offset + 61, FONT_NORMAL_WHITE);
        } else {
            int trade_status = city_data.resource.trade_status[resource];
            if (trade_status == TRADE_STATUS_IMPORT) {
                lang_text_draw(54, 5, 340, y_offset + 61, FONT_NORMAL_WHITE);
            } else if (trade_status == TRADE_STATUS_EXPORT) {
                int width = lang_text_draw(54, 6, 340, y_offset + 61, FONT_NORMAL_WHITE);
                text_draw_number(city_data.resource.export_over[resource], '@', " ",
                    340 + width, y_offset + 61, FONT_NORMAL_WHITE);
            }
        }
    }

    button_border_draw(398, 396, 200, 24, focus_button_id == 1);
    lang_text_draw_centered(54, 2, 400, 402, 200, FONT_NORMAL_BLACK);

    button_border_draw(98, 396, 200, 24, focus_button_id == 2);
    lang_text_draw_centered(54, 30, 100, 402, 200, FONT_NORMAL_BLACK);
}

static int handle_mouse(const struct mouse_t *m)
{
    int num_resources = city_resource_get_available()->size;
    return generic_buttons_handle_mouse(m, 0, 0, resource_buttons, num_resources + 2, &focus_button_id);
}

static void button_prices(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_trade_prices_show();
}

static void button_empire(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    window_empire_show();
}

static void button_resource(int resource_index, __attribute__((unused)) int param2)
{
    window_resource_settings_show(city_resource_get_available()->items[resource_index]);
}

struct advisor_window_type_t *window_advisor_trade(void)
{
    static struct advisor_window_type_t window = {
        draw_background,
        draw_foreground,
        handle_mouse,
    };
    return &window;
}
