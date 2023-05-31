#include "arrow_button.h"

#include "core/time.h"
#include "graphics/image.h"

static const int REPEATS[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
    1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0
};

static const uint32_t REPEAT_MILLIS = 30;
static const unsigned int BUTTON_PRESSED_FRAMES = 3;

void arrow_buttons_draw(int x, int y, struct arrow_button_t *buttons, int num_buttons)
{
    for (int i = 0; i < num_buttons; i++) {
        int image_id = buttons[i].image_id;
        if (buttons[i].pressed) {
            image_id += 1;
        }
        image_draw(image_id, x + buttons[i].x_offset, y + buttons[i].y_offset);
    }
}

static int get_button(const struct mouse_t *m, int x, int y, struct arrow_button_t *buttons, int num_buttons)
{
    for (int i = 0; i < num_buttons; i++) {
        if (x + buttons[i].x_offset <= m->x &&
            x + buttons[i].x_offset + buttons[i].size > m->x &&
            y + buttons[i].y_offset <= m->y &&
            y + buttons[i].y_offset + buttons[i].size > m->y) {
            return i + 1;
        }
    }
    return 0;
}

int arrow_buttons_handle_mouse(
    const struct mouse_t *m, int x, int y, struct arrow_button_t *buttons, int num_buttons, int *focus_button_id)
{
    static uint32_t last_time = 0;

    uint32_t curr_time = time_get_millis();
    int should_repeat = 0;
    if (curr_time - last_time >= REPEAT_MILLIS) {
        should_repeat = 1;
        last_time = curr_time;
    }
    for (int i = 0; i < num_buttons; i++) {
        struct arrow_button_t *btn = &buttons[i];
        if (btn->pressed) {
            btn->pressed--;
            if (!btn->pressed) {
                btn->repeats = 0;
            }
        } else {
            btn->repeats = 0;
        }
    }
    int button_id = get_button(m, x, y, buttons, num_buttons);
    if (focus_button_id) {
        *focus_button_id = button_id;
    }
    if (!button_id) {
        return 0;
    }
    struct arrow_button_t *btn = &buttons[button_id -1];
    if (m->left.went_down) {
        btn->pressed = BUTTON_PRESSED_FRAMES;
        btn->repeats = 0;
        btn->left_click_handler(btn->parameter1, btn->parameter2);
        return button_id;
    }
    if (m->left.is_down) {
        btn->pressed = BUTTON_PRESSED_FRAMES;
        if (should_repeat) {
            btn->repeats++;
            if (btn->repeats < 48) {
                if (!REPEATS[btn->repeats]) {
                    return 0;
                }
            } else {
                btn->repeats = 47;
            }
            btn->left_click_handler(btn->parameter1, btn->parameter2);
        }
        return button_id;
    }
    return 0;
}
