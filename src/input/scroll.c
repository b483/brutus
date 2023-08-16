#include "scroll.h"

#include "core/calc.h"
#include "core/config.h"
#include "core/speed.h"
#include "core/time.h"
#include "game/settings.h"
#include "graphics/graphics.h"
#include "platform/brutus.h"

#include <math.h>
#include <stdlib.h>

#define MOUSE_BORDER 5
#define TOUCH_BORDER 100
#define SCROLL_DRAG_MIN_DELTA 4
#define SCROLL_KEY_PRESSED 1
#define SCROLL_KEY_MAX_VALUE 30000.0f

static const int DIRECTION_X[] = { 0,  1,  1,  1,  0, -1, -1, -1, 0 };
static const int DIRECTION_Y[] = { -1, -1,  0,  1,  1,  1,  0, -1, 0 };
static const int SCROLL_STEP[SCROLL_TYPE_MAX][11] = {
    {60, 44, 30, 20, 16, 12, 10, 8, 6, 4, 2},
    {20, 15, 10,  7,  5,  4,  3, 3, 2, 2, 1}
};

enum {
    KEY_STATE_UNPRESSED = 0,
    KEY_STATE_PRESSED = 1,
    KEY_STATE_HELD = 2,
    KEY_STATE_AXIS = 3
};

struct key_t {
    int state;
    int value;
    uint32_t last_change;
};

static struct {
    int is_scrolling;
    int constant_input;
    struct {
        struct key_t up;
        struct key_t down;
        struct key_t left;
        struct key_t right;
    } arrow_key;
    struct {
        int active;
        int has_started;
        struct pixel_view_coordinates_t delta;
    } drag;
    struct {
        struct speed_type_t x;
        struct speed_type_t y;
        float modifier_x;
        float modifier_y;
    } speed;
    int x_align_direction;
    int y_align_direction;
    uint32_t last_time;
    struct {
        int active;
        int x;
        int y;
        int width;
        int height;
    } limits;
} data;

static void clear_scroll_speed(void)
{
    speed_clear(&data.speed.x);
    speed_clear(&data.speed.y);
    data.x_align_direction = SPEED_DIRECTION_STOPPED;
    data.y_align_direction = SPEED_DIRECTION_STOPPED;
}

static int get_arrow_key_value(struct key_t *arrow)
{
    if (arrow->state == KEY_STATE_AXIS) {
        return arrow->value;
    }
    return arrow->state != KEY_STATE_UNPRESSED;
}

static float get_normalized_arrow_key_value(struct key_t *arrow)
{
    int value = get_arrow_key_value(arrow);
    if (value == SCROLL_KEY_PRESSED) {
        return 1.0f;
    } else {
        return fminf(arrow->value / SCROLL_KEY_MAX_VALUE, 1.0f);
    }
}

static int is_arrow_active(const struct key_t *arrow)
{
    return arrow->value != 0;
}

static int get_key_state_for_value(int value)
{
    if (!value) {
        return KEY_STATE_UNPRESSED;
    }
    if (value == SCROLL_KEY_PRESSED) {
        return KEY_STATE_PRESSED;
    }
    return KEY_STATE_AXIS;
}

static void set_arrow_key(struct key_t *arrow, int value)
{
    int state = get_key_state_for_value(value);
    if (state != KEY_STATE_AXIS && state != KEY_STATE_UNPRESSED &&
        arrow->state != KEY_STATE_AXIS && arrow->state != KEY_STATE_UNPRESSED) {
        return;
    }
    // Key should retain axis state even if its value is zero
    if (arrow->state != KEY_STATE_AXIS || state != KEY_STATE_UNPRESSED) {
        arrow->state = state;
    }
    arrow->value = value;
    arrow->last_change = time_get_millis();
}

int scroll_in_progress(void)
{
    return data.is_scrolling || data.drag.active;
}

static int get_scroll_speed_factor(void)
{
    return calc_bound((100 - setting_scroll_speed()) / 10, 0, 10);
}

static int direction_from_sides(int top, int left, int bottom, int right)
{
    // two sides
    if (left && top) {
        return DIR_7_TOP_LEFT;
    } else if (left && bottom) {
        return DIR_5_BOTTOM_LEFT;
    } else if (right && top) {
        return DIR_1_TOP_RIGHT;
    } else if (right && bottom) {
        return DIR_3_BOTTOM_RIGHT;
    }
    // one side
    if (left) {
        return DIR_6_LEFT;
    } else if (right) {
        return DIR_2_RIGHT;
    } else if (top) {
        return DIR_0_TOP;
    } else if (bottom) {
        return DIR_4_BOTTOM;
    }
    // none of them
    return DIR_8_NONE;
}

void scroll_set_custom_margins(int x, int y, int width, int height)
{
    data.limits.active = 1;
    data.limits.x = x;
    data.limits.y = y;
    data.limits.width = width;
    data.limits.height = height;
}

void scroll_restore_margins(void)
{
    data.limits.active = 0;
}

void scroll_drag_start(void)
{
    if (data.drag.active || config_get(CONFIG_UI_DISABLE_RIGHT_CLICK_MAP_DRAG)) {
        return;
    }
    data.drag.active = 1;
    data.drag.delta.x = 0;
    data.drag.delta.y = 0;
    SDL_GetRelativeMouseState(0, 0);
    clear_scroll_speed();
}

static int set_scroll_speed_from_drag(void)
{
    if (!data.drag.active) {
        return 0;
    }

    int delta_x = 0;
    int delta_y = 0;
    SDL_GetRelativeMouseState(&delta_x, &delta_y);

    data.drag.delta.x += delta_x;
    data.drag.delta.y += delta_y;
    if ((delta_x != 0 || delta_y != 0)) {
        set_relative_mouse_mode(1);
        // Store tiny movements until we decide that it's enough to move into scroll mode
        if (!data.drag.has_started) {
            data.drag.has_started = abs(data.drag.delta.x) > SCROLL_DRAG_MIN_DELTA
                || abs(data.drag.delta.y) > SCROLL_DRAG_MIN_DELTA;
        }
    }
    if (data.drag.has_started) {
        speed_set_target(&data.speed.x, data.drag.delta.x, SPEED_CHANGE_IMMEDIATE, 0);
        speed_set_target(&data.speed.y, data.drag.delta.y, SPEED_CHANGE_IMMEDIATE, 0);
        data.drag.delta.x = 0;
        data.drag.delta.y = 0;
    }
    return 1;
}

int scroll_drag_end(void)
{
    if (!data.drag.active) {
        return 0;
    }

    int has_scrolled = data.drag.has_started;

    data.drag.active = 0;
    data.drag.has_started = 0;

    set_relative_mouse_mode(0);
    data.x_align_direction = speed_get_current_direction(&data.speed.x);
    data.y_align_direction = speed_get_current_direction(&data.speed.y);
    speed_set_target(&data.speed.x, 0, SPEED_CHANGE_IMMEDIATE, 1);
    speed_set_target(&data.speed.y, 0, SPEED_CHANGE_IMMEDIATE, 1);

    return has_scrolled;
}

static int set_arrow_input(struct key_t *arrow, const struct key_t *opposite_arrow, float *modifier)
{
    if (get_arrow_key_value(arrow) && (!opposite_arrow || !is_arrow_active(opposite_arrow))) {
        if (arrow->state == KEY_STATE_AXIS) {
            data.constant_input = 1;
            *modifier = get_normalized_arrow_key_value(arrow);
        }
        return 1;
    }
    return 0;
}

static int get_direction_scrolling(const struct mouse_t *m)
{
    int is_inside_window = m->is_inside_window;
    int width = screen_width();
    int height = screen_height();
    if (setting_fullscreen() && m->x < width && m->y < height) {
        // For Windows 10, in fullscreen mode, on HiDPI screens, this is needed
        // to get scrolling to work
        is_inside_window = 1;
    }
    if (!is_inside_window) {
        return DIR_8_NONE;
    }
    int top = 0;
    int bottom = 0;
    int left = 0;
    int right = 0;
    int border = MOUSE_BORDER;
    int x = m->x;
    int y = m->y;
    data.constant_input = 0;
    data.speed.modifier_x = 0.0f;
    data.speed.modifier_y = 0.0f;
    if (data.limits.active) {
        border = TOUCH_BORDER;
        width = data.limits.width;
        height = data.limits.height;
        x -= data.limits.x;
        y -= data.limits.y;
        data.constant_input = 1;
    }
    // mouse near map edge
    // NOTE: using <= width/height (instead of <) to compensate for rounding
    // errors caused by scaling the display. SDL adds a 1px border to either
    // the right or the bottom when the aspect ratio does not match exactly.
    if (((!config_get(CONFIG_UI_DISABLE_MOUSE_EDGE_SCROLLING)) || data.limits.active) &&
        (x >= 0 && x <= width && y >= 0 && y <= height)) {
        if (x < border) {
            left = 1;
            data.speed.modifier_x = 1 - x / (float) border;
        } else if (x >= width - border) {
            right = 1;
            data.speed.modifier_x = 1 - (width - x) / (float) border;
        }
        if (y < border) {
            top = 1;
            data.speed.modifier_y = 1 - y / (float) border;
        } else if (y >= height - border) {
            bottom = 1;
            data.speed.modifier_y = 1 - (height - y) / (float) border;
        }
    }
    // keyboard/joystick arrow keys
    left |= set_arrow_input(&data.arrow_key.left, 0, &data.speed.modifier_x);
    right |= set_arrow_input(&data.arrow_key.right, &data.arrow_key.left, &data.speed.modifier_x);
    top |= set_arrow_input(&data.arrow_key.up, 0, &data.speed.modifier_y);
    bottom |= set_arrow_input(&data.arrow_key.down, &data.arrow_key.up, &data.speed.modifier_y);

    if (data.constant_input) {
        if (!data.speed.modifier_x) {
            data.speed.modifier_x = data.speed.modifier_y;
        }
        if (!data.speed.modifier_y) {
            data.speed.modifier_y = data.speed.modifier_x;
        }
    }

    return direction_from_sides(top, left, bottom, right);
}

static int set_scroll_speed_from_input(const struct mouse_t *m, int type)
{
    if (set_scroll_speed_from_drag()) {
        return 1;
    }
    int direction = get_direction_scrolling(m);
    if (direction == DIR_8_NONE) {
        uint32_t time = SPEED_CHANGE_IMMEDIATE;
        speed_set_target(&data.speed.x, 0, time, 1);
        speed_set_target(&data.speed.y, 0, time, 1);
        return 0;
    }
    int dir_x = DIRECTION_X[direction];
    int dir_y = DIRECTION_Y[direction];
    int y_fraction = type == SCROLL_TYPE_CITY ? 2 : 1;

    int max_speed = SCROLL_STEP[type][get_scroll_speed_factor()];
    int max_speed_x = max_speed * dir_x;
    int max_speed_y = (max_speed / y_fraction) * dir_y;

    if (!data.constant_input) {
        if (speed_get_current_direction(&data.speed.x) * dir_x < 0) {
            speed_invert(&data.speed.x);
        } else if (data.speed.x.desired_speed != max_speed_x) {
            speed_set_target(&data.speed.x, max_speed_x, SPEED_CHANGE_IMMEDIATE, 1);
        }
        if (speed_get_current_direction(&data.speed.y) * dir_y < 0) {
            speed_invert(&data.speed.y);
        } else if (data.speed.y.desired_speed != max_speed_y) {
            speed_set_target(&data.speed.y, max_speed_y, SPEED_CHANGE_IMMEDIATE, 1);
        }
    } else {
        speed_set_target(&data.speed.x, (int) (max_speed_x * data.speed.modifier_x), SPEED_CHANGE_IMMEDIATE, 1);
        speed_set_target(&data.speed.y, (int) (max_speed_y * data.speed.modifier_y), SPEED_CHANGE_IMMEDIATE, 1);
    }
    return 1;
}

int scroll_get_delta(const struct mouse_t *m, struct pixel_view_coordinates_t *delta, int type)
{
    data.is_scrolling = set_scroll_speed_from_input(m, type);
    delta->x = speed_get_delta(&data.speed.x);
    delta->y = speed_get_delta(&data.speed.y);
    return delta->x != 0 || delta->y != 0;
}

void scroll_stop(void)
{
    clear_scroll_speed();
    set_relative_mouse_mode(0);
    data.is_scrolling = 0;
    data.constant_input = 0;
    data.drag.active = 0;
    data.limits.active = 0;
}

void scroll_arrow_left(int value)
{
    set_arrow_key(&data.arrow_key.left, value);
}

void scroll_arrow_right(int value)
{
    set_arrow_key(&data.arrow_key.right, value);
}

void scroll_arrow_up(int value)
{
    set_arrow_key(&data.arrow_key.up, value);
}

void scroll_arrow_down(int value)
{
    set_arrow_key(&data.arrow_key.down, value);
}
