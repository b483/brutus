#include "labor_priority.h"

#include "city/city_new.h"
#include "graphics/graphics.h"

static void button_set_priority(int new_priority, int param2);
static void button_remove_priority(int param1, int param2);

static struct {
    int category;
    int max_items;
    int focus_id_button_set_priority;
    int focus_id_button_remove_priority;
} data;

static struct generic_button_t priority_buttons[] = {
    {176, 170, 27, 27, button_set_priority, button_none, 1, 0},
    {208, 170, 27, 27, button_set_priority, button_none, 2, 0},
    {240, 170, 27, 27, button_set_priority, button_none, 3, 0},
    {272, 170, 27, 27, button_set_priority, button_none, 4, 0},
    {304, 170, 27, 27, button_set_priority, button_none, 5, 0},
    {336, 170, 27, 27, button_set_priority, button_none, 6, 0},
    {368, 170, 27, 27, button_set_priority, button_none, 7, 0},
    {400, 170, 27, 27, button_set_priority, button_none, 8, 0},
    {432, 170, 27, 27, button_set_priority, button_none, 9, 0},
};

static struct generic_button_t remove_priority_button[] = {
    {220, 206, 200, 25, button_remove_priority, button_none, 0, 0}
};

static void init(int category)
{
    data.category = category;
    data.max_items = 0;
    for (int i = 0; i < 9; i++) {
        if (city_data.labor.categories[i].priority > 0) {
            data.max_items++;
        }
    }
    if (data.max_items < 9 && !city_data.labor.categories[category].priority) {
        // allow space for new priority
        data.max_items++;
    }
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(160, 122, 20, 8);

    // Priority level
    lang_text_draw_centered(50, 25, 160, 138, 320, FONT_LARGE_BLACK);

    for (int i = 1; i < 10; i++) {
        button_border_draw(144 + 32 * i, 170, 27, 27, data.focus_id_button_set_priority == i);
        lang_text_draw_centered(50, 26 + i, 145 + 32 * i, 172, 27, FONT_LARGE_BLACK);
        if (i > data.max_items) {
            graphics_shade_rect(145 + 32 * i, 171, 25, 25, 1);
        }
    }

    // No priority
    button_border_draw(220, 206, 200, 25, data.focus_id_button_remove_priority);
    lang_text_draw_centered(50, 26, 220, 212, 200, FONT_NORMAL_BLACK);

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->right.went_up || h->escape_pressed) {
        window_go_back();
        return;
    }

    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0,
        priority_buttons, data.max_items, &data.focus_id_button_set_priority)) {
        return;
    }
    if (generic_buttons_handle_mouse(m_dialog, 0, 0,
        remove_priority_button, 1, &data.focus_id_button_remove_priority)) {
        return;
    }
    // exit window on click outside of outer panel boundaries
    if (m_dialog->left.went_up && (m_dialog->x < 160 || m_dialog->y < 122 || m_dialog->x > 480 || m_dialog->y > 250)) {
        window_go_back();
        return;
    }
}

static void button_set_priority(int new_priority, __attribute__((unused)) int param2)
{
    city_labor_set_priority(data.category, new_priority);
    window_go_back();
}

static void button_remove_priority(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{
    city_labor_set_priority(data.category, 0);
    window_go_back();
}

void window_labor_priority_show(int category)
{
    struct window_type_t window = {
        WINDOW_LABOR_PRIORITY,
        window_draw_underlying_window,
        draw_foreground,
        handle_input,
    };
    init(category);
    window_show(&window);
}
