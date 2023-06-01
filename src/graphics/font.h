#ifndef GRAPHICS_FONT_H
#define GRAPHICS_FONT_H

#include <stdint.h>

enum {
    FONT_NORMAL_PLAIN,
    FONT_NORMAL_BLACK,
    FONT_NORMAL_WHITE,
    FONT_NORMAL_RED,
    FONT_LARGE_PLAIN,
    FONT_LARGE_BLACK,
    FONT_LARGE_BROWN,
    FONT_SMALL_PLAIN,
    FONT_NORMAL_GREEN,
    FONT_NORMAL_BROWN,
    FONT_TYPES_MAX
};

struct font_definition_t {
    int font;
    int image_offset;
    int space_width;
    int letter_spacing;
    int line_height;

    /**
    * Returns the height offset for the specified character
    * @param c Character
    * @param image_height Height of the letter image
    * @param line_height Line height for the font
    * @return Offset to subtract from y coordinate
    */
    int (*image_y_offset)(uint8_t c, int image_height, int line_height);
};

void font_set_encoding(void);

/**
 * Gets the font definition for the specified font
 * @param font Font
 * @return Font definition
 */
const struct font_definition_t *font_definition_for(int font);

/**
 * Checks whether the font has a glyph for the passed character
 * @param character Character to check
 * @return Boolean true if this character can be drawn on the screen, false otherwise
 */
int font_can_display(const char *character);

/**
 * Gets the letter ID for the specified character and font
 * @param def Font definition
 * @param str Character string
 * @return Letter ID to feed into image_letter(), or -1 if c is no letter
 */
int font_letter_id(const struct font_definition_t *def, const char *str);

#endif // GRAPHICS_FONT_H
