#ifndef GRAPHICS_TEXT_H
#define GRAPHICS_TEXT_H

#include "graphics/color.h"
#include "graphics/font.h"
#include "input/mouse.h"

#include <stdint.h>

void text_capture_cursor(int cursor_position, int offset_start, int offset_end);
void text_draw_cursor(int x_offset, int y_offset);

int text_get_width(const char *str, int font);
unsigned int text_get_max_length_for_width(
    const char *str, int length, int font, unsigned int requested_width, int invert);
void text_ellipsize(char *str, int font, int requested_width);

int text_draw(const char *str, int x, int y, int font, color_t color);

void text_draw_centered(const char *str, int x, int y, int box_width, int font, color_t color);
void text_draw_ellipsized(const char *str, int x, int y, int box_width, int font, color_t color);

int text_draw_number(int value, char prefix, const char *postfix, int x_offset, int y_offset, int font);
int text_draw_number_colored(
    int value, char prefix, const char *postfix, int x_offset, int y_offset, int font, color_t color);
int text_draw_money(int value, int x_offset, int y_offset, int font);
int text_draw_percentage(int value, int x_offset, int y_offset, int font);

void text_draw_number_centered(int value, int x_offset, int y_offset, int box_width, int font);
void text_draw_number_centered_prefix(
    int value, char prefix, int x_offset, int y_offset, int box_width, int font);
void text_draw_number_centered_colored(
    int value, int x_offset, int y_offset, int box_width, int font, color_t color);

int text_draw_multiline(const char *str, int x_offset, int y_offset, int box_width, int font, uint32_t color);
/**
 * @return Number of lines required to draw the text
 */
int text_measure_multiline(const char *str, int box_width, int font);

#endif // GRAPHICS_TEXT_H
