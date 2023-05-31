#include "input/mouse.h"

#include "core/time.h"
#include "graphics/screen.h"

enum {
    SYSTEM_NONE = 0,
    SYSTEM_UP = 1,
    SYSTEM_DOWN = 2,
    SYSTEM_DOUBLE_CLICK = 4
};

#define DOUBLE_CLICK_TIME 300

static struct mouse_t data;
static struct mouse_t dialog;
static uint32_t last_click;

const struct mouse_t *mouse_get(void)
{
    return &data;
}

static void clear_mouse_button(struct mouse_button_t *button)
{
    button->is_down = 0;
    button->went_down = 0;
    button->went_up = 0;
    button->double_click = 0;
    button->system_change = SYSTEM_NONE;
}

void mouse_set_position(int x, int y)
{
    if (x != data.x || y != data.y) {
        last_click = 0;
    }
    data.x = x;
    data.y = y;
    data.is_inside_window = 1;
}

void mouse_set_left_down(int down)
{
    data.left.system_change |= down ? SYSTEM_DOWN : SYSTEM_UP;
    data.is_inside_window = 1;
    if (!down) {
        uint32_t now = time_get_millis();
        int is_double_click = (last_click < now) && ((now - last_click) <= DOUBLE_CLICK_TIME);
        data.left.system_change |= is_double_click ? SYSTEM_DOUBLE_CLICK : SYSTEM_NONE;
        last_click = now;
    }
}

void mouse_set_right_down(int down)
{
    data.right.system_change |= down ? SYSTEM_DOWN : SYSTEM_UP;
    data.is_inside_window = 1;
    last_click = 0;
}

void mouse_set_inside_window(int inside)
{
    data.is_inside_window = inside;
}

static void update_button_state(struct mouse_button_t *button)
{
    button->went_down = (button->system_change & SYSTEM_DOWN) == SYSTEM_DOWN;
    button->went_up = (button->system_change & SYSTEM_UP) == SYSTEM_UP;
    button->double_click = (button->system_change & SYSTEM_DOUBLE_CLICK) == SYSTEM_DOUBLE_CLICK;
    button->system_change = SYSTEM_NONE;
    button->is_down = (button->is_down || button->went_down) && !button->went_up;
}

void mouse_determine_button_state(void)
{
    update_button_state(&data.left);
    update_button_state(&data.right);
}

void mouse_set_scroll(int state)
{
    data.scrolled = state;
    data.is_inside_window = 1;
}

void mouse_reset_scroll(void)
{
    data.scrolled = SCROLL_NONE;
}

void mouse_reset_up_state(void)
{
    data.left.went_up = 0;
    data.right.went_up = 0;
}

void mouse_reset_button_state(void)
{
    last_click = 0;
    clear_mouse_button(&data.left);
    clear_mouse_button(&data.right);
}

const struct mouse_t *mouse_in_dialog(const struct mouse_t *m)
{
    dialog.left = m->left;
    dialog.right = m->right;
    dialog.scrolled = m->scrolled;
    dialog.is_inside_window = m->is_inside_window;

    dialog.x = m->x - screen_dialog_offset_x();
    dialog.y = m->y - screen_dialog_offset_y();
    return &dialog;
}
