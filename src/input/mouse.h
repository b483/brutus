#ifndef GRAPHICS_MOUSE_H
#define GRAPHICS_MOUSE_H

struct mouse_button_t {
    int is_down; /**< mouse button is down */
    int went_down; /**< mouse button went down during this cycle */
    int went_up; /**< mouse button went up during this cycle */
    int double_click; /**< mouse double clicked during this cycle */
    int system_change;
};

enum {
    SCROLL_NONE = 0,
    SCROLL_UP = -1,
    SCROLL_DOWN = 1
};

struct mouse_t {
    int x; /**< Global position X */
    int y; /**< Global position Y */
    int scrolled; /**< Scroll state (up/down/none) */
    struct mouse_button_t left; /**< Left mouse button */
    struct mouse_button_t right; /**< Right mouse button */
    int is_inside_window; /**< Whether the mouse is in the window */
};

/**
 * Gets the mouse state
 * @return mouse state
 */
const struct mouse_t *mouse_get(void);

/**
 * Sets the mouse position
 * @param x X
 * @param y Y
 */
void mouse_set_position(int x, int y);

void mouse_set_left_down(int down);

void mouse_set_right_down(int down);

void mouse_set_scroll(int state);

void mouse_set_inside_window(int inside);

void mouse_reset_up_state(void);

void mouse_reset_scroll(void);

void mouse_reset_button_state(void);

void mouse_determine_button_state(void);

const struct mouse_t *mouse_in_dialog(const struct mouse_t *m);

#endif // GRAPHICS_MOUSE_H
