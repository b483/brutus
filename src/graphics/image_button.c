#include "image_button.h"

#include "graphics/image.h"
#include "sound/sound.h"

#define PRESSED_EFFECT_MILLIS 100
#define PRESSED_REPEAT_INITIAL_MILLIS 300
#define PRESSED_REPEAT_MILLIS 50

static void fade_pressed_effect(struct image_button_t *buttons, int num_buttons)
{
    uint32_t current_time = time_get_millis();
    for (int i = 0; i < num_buttons; i++) {
        struct image_button_t *btn = &buttons[i];
        if (btn->pressed) {
            if (current_time - btn->pressed_since > PRESSED_EFFECT_MILLIS) {
                if (btn->button_type == IB_NORMAL) {
                    btn->pressed = 0;
                } else if (btn->button_type == IB_SCROLL && !mouse_get()->left.is_down) {
                    btn->pressed = 0;
                }
            }
        }
    }
}

static void fade_pressed_effect_build(struct image_button_t *buttons, int num_buttons)
{
    for (int i = 0; i < num_buttons; i++) {
        struct image_button_t *btn = &buttons[i];
        if (btn->pressed && btn->button_type == IB_BUILD) {
            btn->pressed--;
        }
    }
}

void image_buttons_draw(int x, int y, struct image_button_t *buttons, int num_buttons)
{
    fade_pressed_effect(buttons, num_buttons);
    for (int i = 0; i < num_buttons; i++) {
        struct image_button_t *btn = &buttons[i];
        int image_id = image_group(btn->image_collection) + btn->image_offset;
        if (btn->enabled) {
            if (btn->pressed) {
                image_id += 2;
            } else if (btn->focused) {
                image_id += 1;
            }
        } else {
            image_id += 3;
        }
        image_draw(image_id, x + btn->x_offset, y + btn->y_offset);
    }
}

static int should_be_pressed(const struct image_button_t *btn, const struct mouse_t *m)
{
    if ((m->left.went_down || m->left.is_down) && btn->left_click_handler != button_none) {
        return 1;
    }
    if ((m->right.went_down || m->right.is_down) && btn->right_click_handler != button_none) {
        return 1;
    }
    return 0;
}

int image_buttons_handle_mouse(
    const struct mouse_t *m, int x, int y, struct image_button_t *buttons, int num_buttons, int *focus_button_id)
{
    fade_pressed_effect(buttons, num_buttons);
    fade_pressed_effect_build(buttons, num_buttons);
    struct image_button_t *hit_button = 0;
    if (focus_button_id) {
        *focus_button_id = 0;
    }
    for (int i = 0; i < num_buttons; i++) {
        struct image_button_t *btn = &buttons[i];
        if (btn->focused) {
            btn->focused--;
        }
        if (x + btn->x_offset <= m->x &&
            x + btn->x_offset + btn->width > m->x &&
            y + btn->y_offset <= m->y &&
            y + btn->y_offset + btn->height > m->y) {
            if (focus_button_id) {
                *focus_button_id = i + 1;
            }
            if (btn->enabled) {
                btn->focused = 2;
                hit_button = btn;
            }
        }
    }
    if (!hit_button) {
        return 0;
    }
    if (hit_button->button_type == IB_SCROLL) {
        if (!m->left.went_down && !m->left.is_down) {
            return 0;
        }
    } else if (hit_button->button_type == IB_BUILD || hit_button->button_type == IB_NORMAL) {
        if (should_be_pressed(hit_button, m)) {
            hit_button->pressed = 2;
            hit_button->pressed_since = time_get_millis();
        }
        if (!m->left.went_up && !m->right.went_up) {
            return 0;
        }
    }
    if (m->left.went_up) {
        play_sound_effect(SOUND_EFFECT_ICON);
        hit_button->left_click_handler(hit_button->parameter1, hit_button->parameter2);
        return hit_button->left_click_handler != button_none;
    } else if (m->right.went_up) {
        hit_button->right_click_handler(hit_button->parameter1, hit_button->parameter2);
        return hit_button->right_click_handler != button_none;
    } else if (hit_button->button_type == IB_SCROLL && m->left.is_down) {
        uint32_t delay = hit_button->pressed == 2 ? PRESSED_REPEAT_MILLIS : PRESSED_REPEAT_INITIAL_MILLIS;
        if (time_get_millis() - hit_button->pressed_since >= delay) {
            hit_button->pressed = 2;
            hit_button->pressed_since = time_get_millis();
            hit_button->left_click_handler(hit_button->parameter1, hit_button->parameter2);
        }
        return 1;
    }
    return 0;
}
