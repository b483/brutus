#include "input_box.h"

#include "SDL.h"

#include "graphics/graphics.h"
#include "input/input.h"
#include "platform/brutus.h"

static int draw_cursor;

void input_box_start(struct input_box_t *box)
{
    int text_width = (box->width_blocks - 2) * BLOCK_SIZE;
    keyboard_start_capture(box->text, box->text_length, box->allow_punctuation, text_width, box->font);
    SDL_Rect rect = { box->x, box->y, box->width_blocks * BLOCK_SIZE, box->height_blocks * BLOCK_SIZE };
    SDL_SetTextInputRect(&rect);
    draw_cursor = 1;
}

void input_box_stop(__attribute__((unused)) struct input_box_t *box)
{
    keyboard_stop_capture();
    SDL_Rect rect = { 0, 0, 0, 0 };
    SDL_SetTextInputRect(&rect);
    draw_cursor = 0;
}

void input_box_refresh_text(__attribute__((unused)) struct input_box_t *box)
{
    keyboard_refresh();
}

int input_box_is_accepted(__attribute__((unused)) struct input_box_t *box)
{
    return keyboard_input_is_accepted();
}

int input_box_is_mouse_inside_input(const struct mouse_t *m, const struct input_box_t *box)
{
    return m->x >= box->x && m->x < box->x + box->width_blocks * BLOCK_SIZE &&
        m->y >= box->y && m->y < box->y + box->height_blocks * BLOCK_SIZE;
}

void input_box_draw(const struct input_box_t *box)
{
    inner_panel_draw(box->x, box->y, box->width_blocks, box->height_blocks);
    int text_x = box->x + 16;
    int text_y = box->y + 10;
    // text_capture_cursor and text_draw_cursor must be before and after text_draw respectively...
    if (draw_cursor) {
        text_capture_cursor(keyboard_cursor_position(), keyboard_offset_start(), keyboard_offset_end());
    }
    text_draw(box->text, text_x, text_y, box->font, 0);
    if (draw_cursor) {
        text_draw_cursor(text_x, text_y + 1);
    }
}
