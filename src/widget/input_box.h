#ifndef WIDGET_INPUT_BOX_H
#define WIDGET_INPUT_BOX_H

#include "graphics/font.h"
#include "input/mouse.h"

struct input_box_t {
    int x;
    int y;
    int width_blocks;
    int height_blocks;
    int font;
    int allow_punctuation;
    uint8_t *text;
    int text_length;
};

/**
 * This will start text input. The `text` variable of the box will be used to capture
 * input until @link input_box_stop @endlink is called
 * @param box Input box
 */
void input_box_start(struct input_box_t *box);
void input_box_stop(struct input_box_t *box);

void input_box_refresh_text(struct input_box_t *box);
int input_box_is_accepted(struct input_box_t *box);

int input_box_is_mouse_inside_input(const struct mouse_t *m, const struct input_box_t *box);

void input_box_draw(const struct input_box_t *box);

#endif // WIDGET_INPUT_BOX_H
