#include "font.h"

#include "core/image.h"

static int image_y_offset_default(uint8_t c, int image_height, int line_height);

static const int CHAR_TO_FONT_IMAGE_DEFAULT[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x3F, 0x40, 0x00, 0x00, 0x41, 0x00, 0x4A, 0x43, 0x44, 0x42, 0x46, 0x4E, 0x45, 0x4F, 0x4D,
    0x3E, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x48, 0x49, 0x00, 0x47, 0x00, 0x4B,
    0x00, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
    0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x00, 0x00, 0x00, 0x00, 0x50,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x81, 0x00, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x00, 0x00, 0x00,
    0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x82, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80,
    0x72, 0x70, 0x71, 0x71, 0x69, 0x83, 0x6D, 0x65, 0x74, 0x6A, 0x73, 0x73, 0x77, 0x75, 0x76, 0x76,
    0x00, 0x6C, 0x7A, 0x78, 0x79, 0x79, 0x7B, 0x00, 0x84, 0x7E, 0x7C, 0x7D, 0x6B, 0x33, 0x00, 0x68,
    0x53, 0x52, 0x54, 0x51, 0x51, 0x85, 0x67, 0x65, 0x57, 0x56, 0x58, 0x55, 0x5B, 0x5A, 0x5C, 0x59,
    0x00, 0x66, 0x5F, 0x5E, 0x60, 0x60, 0x5D, 0x00, 0x86, 0x63, 0x62, 0x64, 0x61, 0x19, 0x00, 0x19,
};

static const font_definition DEFINITIONS_DEFAULT[] = {
    {FONT_NORMAL_PLAIN,   0, 0, 6, 1, 11, image_y_offset_default},
    {FONT_NORMAL_BLACK, 134, 0, 6, 0, 11, image_y_offset_default},
    {FONT_NORMAL_WHITE, 268, 0, 6, 0, 11, image_y_offset_default},
    {FONT_NORMAL_RED,   402, 0, 6, 0, 11, image_y_offset_default},
    {FONT_LARGE_PLAIN,  536, 0, 8, 1, 23, image_y_offset_default},
    {FONT_LARGE_BLACK,  670, 0, 8, 0, 23, image_y_offset_default},
    {FONT_LARGE_BROWN,  804, 0, 8, 0, 24, image_y_offset_default},
    {FONT_SMALL_PLAIN,  938, 0, 4, 1, 9, image_y_offset_default},
    {FONT_NORMAL_GREEN,1072, 0, 6, 0, 11, image_y_offset_default},
    {FONT_NORMAL_BROWN, 1206, 0, 6, 0, 11, image_y_offset_default}
};

enum {
    MULTIBYTE_NONE = 0,
};

static struct {
    const int *font_mapping;
    const font_definition *font_definitions;
    int multibyte;
} data;

static int image_y_offset_default(uint8_t c, int image_height, int line_height)
{
    int offset = image_height - line_height;
    if (offset < 0) {
        offset = 0;
    }
    if (c < 0x80 || c == 0xE7) {
        offset = 0;
    }
    return offset;
}

void font_set_encoding(void)
{
    data.multibyte = MULTIBYTE_NONE;
    data.font_mapping = CHAR_TO_FONT_IMAGE_DEFAULT;
    data.font_definitions = DEFINITIONS_DEFAULT;

}

const font_definition *font_definition_for(font_t font)
{
    return &data.font_definitions[font];
}

int font_can_display(const uint8_t *character)
{
    int dummy;
    return font_letter_id(&data.font_definitions[FONT_NORMAL_BLACK], character, &dummy) >= 0;
}

int font_letter_id(const font_definition *def, const uint8_t *str, int *num_bytes)
{
    *num_bytes = 1;
    if (!data.font_mapping[*str]) {
        return -1;
    }
    return data.font_mapping[*str] + def->image_offset - 1;
}
