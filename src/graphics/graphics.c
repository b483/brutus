#include "graphics.h"

#include "city/view.h"
#include "city/warning.h"
#include "core/calc.h"
#include "core/file.h"
#include "core/image.h"
#include "core/lang.h"
#include "core/smacker.h"
#include "core/string.h"
#include "core/time.h"
#include "game/game.h"
#include "input/input.h"
#include "map/map.h"
#include "platform/brutus.h"
#include "sound/sound.h"
#include "widget/city_without_overlay.h"

#include "png.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PRESSED_EFFECT_MILLIS 100
#define PRESSED_REPEAT_INITIAL_MILLIS 300
#define PRESSED_REPEAT_MILLIS 50

#define FOOTPRINT_WIDTH 58
#define FOOTPRINT_HEIGHT 30
#define COMPONENT(c, shift) ((c >> shift) & 0xff)
#define MIX_RB(src, dst, alpha) ((((src & 0xff00ff) * alpha + (dst & 0xff00ff) * (256 - alpha)) >> 8) & 0xff00ff)
#define MIX_G(src, dst, alpha) ((((src & 0x00ff00) * alpha + (dst & 0x00ff00) * (256 - alpha)) >> 8) & 0x00ff00)

#define TOP_MENU_BASE_X_OFFSET 10
#define MENU_BASE_TEXT_Y_OFFSET 6
#define MENU_ITEM_HEIGHT 20

#define MAX_LINKS 50

#define TILE_X_SIZE 60
#define TILE_Y_SIZE 30
#define IMAGE_HEIGHT_CHUNK TILE_Y_SIZE
#define IMAGE_BYTES_PER_PIXEL 3

#define SCROLL_BUTTON_HEIGHT 26
#define SCROLL_BUTTON_WIDTH 39
#define SCROLL_DOT_SIZE 25
#define TOTAL_BUTTON_HEIGHT (2 * SCROLL_BUTTON_HEIGHT + SCROLL_DOT_SIZE)

#define ELLIPSIS_LENGTH 4
#define NUMBER_BUFFER_LENGTH 100

#define MAX_WINDOW_QUEUE 3

static struct {
    color_t *pixels;
    int width;
    int height;
} canvas;

static struct {
    int x_start;
    int x_end;
    int y_start;
    int y_end;
} clip_rectangle = { 0, 800, 0, 600 };

static struct {
    int x;
    int y;
} translation;

static struct clip_info_t clip;

static const int REPEATS[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0,
    1, 0, 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0
};

static const uint32_t REPEAT_MILLIS = 30;
static const unsigned int BUTTON_PRESSED_FRAMES = 3;

static const char CHAR_TO_FONT_IMAGE_DEFAULT[] = {
    0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x00, 0x01,
    0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x3F, 0x40, 0x00, 0x00, 0x41, 0x00, 0x4A, 0x43, 0x44, 0x42, 0x46, 0x4E, 0x45, 0x4F, 0x4D,
    0x3E, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x48, 0x49, 0x00, 0x47, 0x00, 0x4B,
    0x00, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29,
    0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F, 0x30, 0x31, 0x32, 0x33, 0x34, 0x00, 0x00, 0x00, 0x00, 0x50,
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6E, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x6F, 0x00, 0x00, 0x00,
    0x00, 0x7F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x72, 0x70, 0x71, 0x71, 0x69, 0x00, 0x6D, 0x65, 0x74, 0x6A, 0x73, 0x73, 0x77, 0x75, 0x76, 0x76,
    0x00, 0x6C, 0x7A, 0x78, 0x79, 0x79, 0x7B, 0x00, 0x00, 0x7E, 0x7C, 0x7D, 0x6B, 0x33, 0x00, 0x68,
    0x53, 0x52, 0x54, 0x51, 0x51, 0x00, 0x67, 0x65, 0x57, 0x56, 0x58, 0x55, 0x5B, 0x5A, 0x5C, 0x59,
    0x00, 0x66, 0x5F, 0x5E, 0x60, 0x60, 0x5D, 0x00, 0x00, 0x63, 0x62, 0x64, 0x61, 0x19, 0x00, 0x19,
};

static struct {
    const char *font_mapping;
    const struct font_definition_t *font_definitions;
} font_data;

enum {
    DRAW_TYPE_SET,
    DRAW_TYPE_AND,
    DRAW_TYPE_NONE,
    DRAW_TYPE_BLEND,
    DRAW_TYPE_BLEND_ALPHA
};

static const int FOOTPRINT_X_START_PER_HEIGHT[] = {
    28, 26, 24, 22, 20, 18, 16, 14, 12, 10, 8, 6, 4, 2, 0,
    0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28
};

static const int FOOTPRINT_OFFSET_PER_HEIGHT[] = {
    0, 2, 8, 18, 32, 50, 72, 98, 128, 162, 200, 242, 288, 338, 392, 450,
    508, 562, 612, 658, 700, 738, 772, 802, 828, 850, 868, 882, 892, 898
};

static char editor_top_menu_clear_earthquakes_string[] = "Clear earthquakes";

static struct {
    int message_id;
    int x_min;
    int y_min;
    int x_max;
    int y_max;
} links[MAX_LINKS];

static char tmp_line_rich_text[200];

static struct {
    const struct font_definition_t *normal_font;
    const struct font_definition_t *link_font;
    int line_height;
    int paragraph_indent;

    int x_text;
    int y_text;
    int text_width_blocks;
    int text_height_blocks;
    int text_height_lines;
    int num_lines;
    int max_scroll_position;
    int num_links;
} rich_text_data;

static struct {
    int width;
    int height;
    struct {
        int x;
        int y;
    } dialog_offset;
} screen_data;

enum {
    FULL_CITY_SCREENSHOT = 0,
    DISPLAY_SCREENSHOT = 1,
    MAX_SCREENSHOT_TYPES = 2
};

static const char filename_formats[MAX_SCREENSHOT_TYPES][32] = {
    "full city %Y-%m-%d %H.%M.%S.png",
    "city %Y-%m-%d %H.%M.%S.png",
};

static struct {
    int width;
    int height;
    int row_size;
    int rows_in_memory;
    int current_y;
    int final_y;
    uint8_t *pixels;
    FILE *fp;
    png_structp png_ptr;
    png_infop info_ptr;
} image;

static struct scrollbar_type_t *current;

static char tmp_line_text[200];

static struct {
    int capture;
    int seen;
    int position;
    int cursor_position;
    int width;
    int x_offset;
    int y_offset;
    int text_offset_start;
    int text_offset_end;
} input_cursor;

static struct {
    const char string[ELLIPSIS_LENGTH];
    int width[FONT_TYPES_MAX];
} ellipsis = { {'.', '.', '.', 0}, {0} };

static struct {
    int is_playing;
    int is_ended;
    smacker s;
    struct {
        int width;
        int height;
        int y_scale;
        int micros_per_frame;
        uint32_t start_render_millis;
        int current_frame;
    } video;
    struct {
        int has_audio;
        int bitdepth;
        int channels;
        int rate;
    } audio;
    int restart_music;
} data_video;

static struct {
    struct window_type_t window_queue[MAX_WINDOW_QUEUE];
    int queue_index;
    struct window_type_t *current_window;
    int refresh_immediate;
    int refresh_on_draw;
    int underlying_windows_redrawing;
} window_data;

void *graphics_canvas(void)
{
    return canvas.pixels;
}

static void translate_clip(int dx, int dy)
{
    clip_rectangle.x_start -= dx;
    clip_rectangle.x_end -= dx;
    clip_rectangle.y_start -= dy;
    clip_rectangle.y_end -= dy;
}

static void set_translation(int x, int y)
{
    int dx = x - translation.x;
    int dy = y - translation.y;
    translation.x = x;
    translation.y = y;
    translate_clip(dx, dy);
}

void graphics_in_dialog(void)
{
    set_translation(screen_dialog_offset_x(), screen_dialog_offset_y());
}

void graphics_reset_dialog(void)
{
    set_translation(0, 0);
}

void graphics_set_clip_rectangle(int x, int y, int width, int height)
{
    clip_rectangle.x_start = x;
    clip_rectangle.x_end = x + width;
    clip_rectangle.y_start = y;
    clip_rectangle.y_end = y + height;
    // fix clip rectangle going over the edges of the screen
    if (translation.x + clip_rectangle.x_start < 0) {
        clip_rectangle.x_start = -translation.x;
    }
    if (translation.y + clip_rectangle.y_start < 0) {
        clip_rectangle.y_start = -translation.y;
    }
    if (translation.x + clip_rectangle.x_end > canvas.width) {
        clip_rectangle.x_end = canvas.width - translation.x;
    }
    if (translation.y + clip_rectangle.y_end > canvas.height) {
        clip_rectangle.y_end = canvas.height - translation.y;
    }
}

void graphics_reset_clip_rectangle(void)
{
    clip_rectangle.x_start = 0;
    clip_rectangle.x_end = canvas.width;
    clip_rectangle.y_start = 0;
    clip_rectangle.y_end = canvas.height;
    translate_clip(translation.x, translation.y);
}

const struct clip_info_t *graphics_get_clip_info(int x, int y, int width, int height)
{
    clip.clipped_pixels_left = 0;
    clip.clipped_pixels_right = 0;
    if (width <= 0
        || x + width <= clip_rectangle.x_start
        || x >= clip_rectangle.x_end) {
        clip.clip_x = CLIP_INVISIBLE;
        clip.visible_pixels_x = 0;
        return 0;
    }
    if (x < clip_rectangle.x_start) {
        // clipped on the left
        clip.clipped_pixels_left = clip_rectangle.x_start - x;
        if (x + width <= clip_rectangle.x_end) {
            clip.clip_x = CLIP_LEFT;
        } else {
            clip.clip_x = CLIP_BOTH;
            clip.clipped_pixels_right = x + width - clip_rectangle.x_end;
        }
    } else if (x + width > clip_rectangle.x_end) {
        clip.clip_x = CLIP_RIGHT;
        clip.clipped_pixels_right = x + width - clip_rectangle.x_end;
    } else {
        clip.clip_x = CLIP_NONE;
    }
    clip.visible_pixels_x = width - clip.clipped_pixels_left - clip.clipped_pixels_right;
    clip.clipped_pixels_top = 0;
    clip.clipped_pixels_bottom = 0;
    if (height <= 0
        || y + height <= clip_rectangle.y_start
        || y >= clip_rectangle.y_end) {
        clip.clip_y = CLIP_INVISIBLE;
    } else if (y < clip_rectangle.y_start) {
        // clipped on the top
        clip.clipped_pixels_top = clip_rectangle.y_start - y;
        if (y + height <= clip_rectangle.y_end) {
            clip.clip_y = CLIP_TOP;
        } else {
            clip.clip_y = CLIP_BOTH;
            clip.clipped_pixels_bottom = y + height - clip_rectangle.y_end;
        }
    } else if (y + height > clip_rectangle.y_end) {
        clip.clip_y = CLIP_BOTTOM;
        clip.clipped_pixels_bottom = y + height - clip_rectangle.y_end;
    } else {
        clip.clip_y = CLIP_NONE;
    }
    clip.visible_pixels_y = height - clip.clipped_pixels_top - clip.clipped_pixels_bottom;
    if (clip.clip_x == CLIP_INVISIBLE || clip.clip_y == CLIP_INVISIBLE) {
        clip.is_visible = 0;
    } else {
        clip.is_visible = 1;
    }
    return &clip;
}

void graphics_save_to_buffer(int x, int y, int width, int height, color_t *buffer)
{
    const struct clip_info_t *current_clip = graphics_get_clip_info(x, y, width, height);
    if (!current_clip->is_visible) {
        return;
    }
    int min_x = x + current_clip->clipped_pixels_left;
    int min_dy = current_clip->clipped_pixels_top;
    int max_dy = height - current_clip->clipped_pixels_bottom;
    for (int dy = min_dy; dy < max_dy; dy++) {
        memcpy(&buffer[dy * width], graphics_get_pixel(min_x, y + dy),
            sizeof(color_t) * current_clip->visible_pixels_x);
    }
}

void graphics_draw_from_buffer(int x, int y, int width, int height, const color_t *buffer)
{
    const struct clip_info_t *current_clip = graphics_get_clip_info(x, y, width, height);
    if (!current_clip->is_visible) {
        return;
    }
    int min_x = x + current_clip->clipped_pixels_left;
    int min_dy = current_clip->clipped_pixels_top;
    int max_dy = height - current_clip->clipped_pixels_bottom;
    for (int dy = min_dy; dy < max_dy; dy++) {
        memcpy(graphics_get_pixel(min_x, y + dy), &buffer[dy * width],
            sizeof(color_t) * current_clip->visible_pixels_x);
    }
}

color_t *graphics_get_pixel(int x, int y)
{
    return &canvas.pixels[(translation.y + y) * canvas.width + (translation.x + x)];
}

void graphics_clear_screen(void)
{
    memset(canvas.pixels, 0, sizeof(color_t) * canvas.width * canvas.height);
}

void graphics_draw_vertical_line(int x, int y1, int y2, color_t color)
{
    if (x < clip_rectangle.x_start || x >= clip_rectangle.x_end) {
        return;
    }
    int y_min = y1 < y2 ? y1 : y2;
    int y_max = y1 < y2 ? y2 : y1;
    y_min = y_min < clip_rectangle.y_start ? clip_rectangle.y_start : y_min;
    y_max = y_max >= clip_rectangle.y_end ? clip_rectangle.y_end - 1 : y_max;
    color_t *pixel = graphics_get_pixel(x, y_min);
    color_t *end_pixel = pixel + ((y_max - y_min) * canvas.width);
    while (pixel <= end_pixel) {
        *pixel = color;
        pixel += canvas.width;
    }
}

void graphics_draw_horizontal_line(int x1, int x2, int y, color_t color)
{
    if (y < clip_rectangle.y_start || y >= clip_rectangle.y_end) {
        return;
    }
    int x_min = x1 < x2 ? x1 : x2;
    int x_max = x1 < x2 ? x2 : x1;
    x_min = x_min < clip_rectangle.x_start ? clip_rectangle.x_start : x_min;
    x_max = x_max >= clip_rectangle.x_end ? clip_rectangle.x_end - 1 : x_max;
    color_t *pixel = graphics_get_pixel(x_min, y);
    color_t *end_pixel = pixel + (x_max - x_min);
    while (pixel <= end_pixel) {
        *pixel = color;
        ++pixel;
    }
}

void graphics_draw_rect(int x, int y, int width, int height, color_t color)
{
    graphics_draw_horizontal_line(x, x + width - 1, y, color);
    graphics_draw_horizontal_line(x, x + width - 1, y + height - 1, color);
    graphics_draw_vertical_line(x, y, y + height - 1, color);
    graphics_draw_vertical_line(x + width - 1, y, y + height - 1, color);
}

void graphics_draw_inset_rect(int x, int y, int width, int height)
{
    graphics_draw_horizontal_line(x, x + width - 1, y, COLOR_INSET_DARK);
    graphics_draw_vertical_line(x + width - 1, y, y + height - 1, COLOR_INSET_LIGHT);
    graphics_draw_horizontal_line(x, x + width - 1, y + height - 1, COLOR_INSET_LIGHT);
    graphics_draw_vertical_line(x, y, y + height - 1, COLOR_INSET_DARK);
}

void graphics_fill_rect(int x, int y, int width, int height, color_t color)
{
    for (int yy = y; yy < height + y; yy++) {
        graphics_draw_horizontal_line(x, x + width - 1, yy, color);
    }
}

void graphics_shade_rect(int x, int y, int width, int height, int darkness)
{
    const struct clip_info_t *cur_clip = graphics_get_clip_info(x, y, width, height);
    if (!cur_clip->is_visible) {
        return;
    }
    for (int yy = y + cur_clip->clipped_pixels_top; yy < y + height - cur_clip->clipped_pixels_bottom; yy++) {
        for (int xx = x + cur_clip->clipped_pixels_left; xx < x + width - cur_clip->clipped_pixels_right; xx++) {
            color_t *pixel = graphics_get_pixel(xx, yy);
            int r = (*pixel & 0xff0000) >> 16;
            int g = (*pixel & 0xff00) >> 8;
            int b = (*pixel & 0xff);
            int grey = (r + g + b) / 3 >> darkness;
            color_t new_pixel = (color_t) (grey << 16 | grey << 8 | grey);
            *pixel = new_pixel;
        }
    }
}

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

int arrow_buttons_handle_mouse(const struct mouse_t *m, int x, int y, struct arrow_button_t *buttons, int num_buttons, int *focus_button_id)
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
    int button_id = 0;
    for (int i = 0; i < num_buttons; i++) {
        if (x + buttons[i].x_offset <= m->x &&
            x + buttons[i].x_offset + buttons[i].size > m->x &&
            y + buttons[i].y_offset <= m->y &&
            y + buttons[i].y_offset + buttons[i].size > m->y) {
            button_id = i + 1;
            break;
        }
    }

    if (focus_button_id) {
        *focus_button_id = button_id;
    }
    if (!button_id) {
        return 0;
    }
    struct arrow_button_t *btn = &buttons[button_id - 1];
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

void button_none(__attribute__((unused)) int param1, __attribute__((unused)) int param2)
{}

void button_border_draw(int x, int y, int width_pixels, int height_pixels, int has_focus)
{
    int width_blocks = width_pixels / BLOCK_SIZE;
    if (width_pixels % BLOCK_SIZE) {
        width_blocks++;
    }
    int height_blocks = height_pixels / BLOCK_SIZE;
    if (height_pixels % BLOCK_SIZE) {
        height_blocks++;
    }
    int last_block_offset_x = BLOCK_SIZE * width_blocks - width_pixels;
    int last_block_offset_y = BLOCK_SIZE * height_blocks - height_pixels;

    int image_base = image_group(GROUP_BORDERED_BUTTON);
    if (has_focus) {
        image_base += 8;
    }

    for (int yy = 0; yy < height_blocks; yy++) {
        int draw_offset_y = y + BLOCK_SIZE * yy;
        for (int xx = 0; xx < width_blocks; xx++) {
            int draw_offset_x = x + BLOCK_SIZE * xx;
            if (yy == 0) {
                if (xx == 0) {
                    image_draw(image_base, draw_offset_x, draw_offset_y);
                } else if (xx < width_blocks - 1) {
                    image_draw(image_base + 1, draw_offset_x, draw_offset_y);
                } else {
                    image_draw(image_base + 2, draw_offset_x - last_block_offset_x, draw_offset_y);
                }
            } else if (yy < height_blocks - 1) {
                if (xx == 0) {
                    image_draw(image_base + 7, draw_offset_x, draw_offset_y);
                } else if (xx >= width_blocks - 1) {
                    image_draw(image_base + 3, draw_offset_x - last_block_offset_x, draw_offset_y);
                }
            } else {
                if (xx == 0) {
                    image_draw(image_base + 6, draw_offset_x, draw_offset_y - last_block_offset_y);
                } else if (xx < width_blocks - 1) {
                    image_draw(image_base + 5, draw_offset_x, draw_offset_y - last_block_offset_y);
                } else {
                    image_draw(image_base + 4,
                        draw_offset_x - last_block_offset_x, draw_offset_y - last_block_offset_y);
                }
            }
        }
    }
}

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

static const struct font_definition_t DEFINITIONS_DEFAULT[] = {
    {FONT_NORMAL_PLAIN, 0, 6, 1, 11, image_y_offset_default},
    {FONT_NORMAL_BLACK, 134, 6, 0, 11, image_y_offset_default},
    {FONT_NORMAL_WHITE, 268, 6, 0, 11, image_y_offset_default},
    {FONT_NORMAL_RED, 402, 6, 0, 11, image_y_offset_default},
    {FONT_LARGE_PLAIN, 536, 8, 1, 23, image_y_offset_default},
    {FONT_LARGE_BLACK, 670, 8, 0, 23, image_y_offset_default},
    {FONT_LARGE_BROWN, 804, 8, 0, 24, image_y_offset_default},
    {FONT_SMALL_PLAIN, 938, 4, 1, 9, image_y_offset_default},
    {FONT_NORMAL_GREEN, 1072, 6, 0, 11, image_y_offset_default},
    {FONT_NORMAL_BROWN, 1206, 6, 0, 11, image_y_offset_default}
};

void font_set_encoding(void)
{
    font_data.font_mapping = CHAR_TO_FONT_IMAGE_DEFAULT;
    font_data.font_definitions = DEFINITIONS_DEFAULT;
}

const struct font_definition_t *font_definition_for(int font)
{
    return &font_data.font_definitions[font];
}

static int font_letter_id(const struct font_definition_t *def, const char *str)
{
    if (!font_data.font_mapping[(uint8_t) *str]) {
        return -1;
    }
    return font_data.font_mapping[(uint8_t) *str] + def->image_offset - 1;
}

int font_can_display(const char *character)
{
    return font_letter_id(&font_data.font_definitions[FONT_NORMAL_BLACK], character) >= 0;
}

int generic_buttons_handle_mouse(const struct mouse_t *m, int x, int y, struct generic_button_t *buttons, int num_buttons, int *focus_button_id)
{
    int button_id = 0;
    for (int i = 0; i < num_buttons; i++) {
        if (x + buttons[i].x <= m->x &&
            x + buttons[i].x + buttons[i].width > m->x &&
            y + buttons[i].y <= m->y &&
            y + buttons[i].y + buttons[i].height > m->y) {
            button_id = i + 1;
            break;
        }
    }
    if (focus_button_id) {
        *focus_button_id = button_id;
    }
    if (!button_id) {
        return 0;
    }
    struct generic_button_t *button = &buttons[button_id - 1];
    if (m->left.went_up) {
        button->left_click_handler(button->parameter1, button->parameter2);
        return button->left_click_handler != button_none;
    } else if (m->right.went_up) {
        button->right_click_handler(button->parameter1, button->parameter2);
        return button->right_click_handler != button_none;
    } else {
        return 0;
    }
}

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

int image_buttons_handle_mouse(const struct mouse_t *m, int x, int y, struct image_button_t *buttons, int num_buttons, int *focus_button_id)
{
    fade_pressed_effect(buttons, num_buttons);

    for (int i = 0; i < num_buttons; i++) {
        struct image_button_t *btn = &buttons[i];
        if (btn->pressed && btn->button_type == IB_BUILD) {
            btn->pressed--;
        }
    }
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
        int should_be_pressed = 0;
        if (((m->left.went_down || m->left.is_down) && hit_button->left_click_handler != button_none)
        || ((m->right.went_down || m->right.is_down) && hit_button->right_click_handler != button_none)) {
            should_be_pressed = 1;
        }
        if (should_be_pressed) {
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

static void draw_uncompressed(const struct image_t *img, const color_t *data, int x_offset, int y_offset, color_t color, int type)
{
    const struct clip_info_t *clip = graphics_get_clip_info(x_offset, y_offset, img->width, img->height);
    if (!clip || !clip->is_visible) {
        return;
    }
    data += img->width * clip->clipped_pixels_top;
    for (int y = clip->clipped_pixels_top; y < img->height - clip->clipped_pixels_bottom; y++) {
        data += clip->clipped_pixels_left;
        color_t *dst = graphics_get_pixel(x_offset + clip->clipped_pixels_left, y_offset + y);
        int x_max = img->width - clip->clipped_pixels_right;
        if (type == DRAW_TYPE_NONE) {
            if (img->draw.type == IMAGE_TYPE_WITH_TRANSPARENCY || img->draw.is_external) { // can be transparent
                for (int x = clip->clipped_pixels_left; x < x_max; x++, dst++) {
                    if (*data != COLOR_SG2_TRANSPARENT) {
                        *dst = *data;
                    }
                    data++;
                }
            } else {
                int num_pixels = x_max - clip->clipped_pixels_left;
                memcpy(dst, data, num_pixels * sizeof(color_t));
                data += num_pixels;
            }
        } else if (type == DRAW_TYPE_SET) {
            for (int x = clip->clipped_pixels_left; x < x_max; x++, dst++) {
                if (*data != COLOR_SG2_TRANSPARENT) {
                    *dst = color;
                }
                data++;
            }
        } else if (type == DRAW_TYPE_AND) {
            for (int x = clip->clipped_pixels_left; x < x_max; x++, dst++) {
                if (*data != COLOR_SG2_TRANSPARENT) {
                    *dst = *data & color;
                }
                data++;
            }
        } else if (type == DRAW_TYPE_BLEND) {
            for (int x = clip->clipped_pixels_left; x < x_max; x++, dst++) {
                if (*data != COLOR_SG2_TRANSPARENT) {
                    *dst &= color;
                }
                data++;
            }
        } else if (type == DRAW_TYPE_BLEND_ALPHA) {
            for (int x = clip->clipped_pixels_left; x < x_max; x++, dst++) {
                if (*data != COLOR_SG2_TRANSPARENT) {
                    color_t alpha = COMPONENT(*data, 24);
                    if (alpha == 255) {
                        *dst = color;
                    } else {
                        color_t s = color;
                        color_t d = *dst;
                        *dst = MIX_RB(s, d, alpha) | MIX_G(s, d, alpha);
                    }
                }
                data++;
            }
        }
        data += clip->clipped_pixels_right;
    }
}

static void draw_compressed(const struct image_t *img, const color_t *data, int x_offset, int y_offset, int height)
{
    const struct clip_info_t *clip = graphics_get_clip_info(x_offset, y_offset, img->width, height);
    if (!clip || !clip->is_visible) {
        return;
    }
    int unclipped = clip->clip_x == CLIP_NONE;

    for (int y = 0; y < height - clip->clipped_pixels_bottom; y++) {
        int x = 0;
        while (x < img->width) {
            color_t b = *data;
            data++;
            if (b == 255) {
                // transparent pixels to skip
                x += *data;
                data++;
            } else if (y < clip->clipped_pixels_top) {
                data += b;
                x += b;
            } else {
                // number of concrete pixels
                const color_t *pixels = data;
                data += b;
                color_t *dst = graphics_get_pixel(x_offset + x, y_offset + y);
                if (unclipped) {
                    x += b;
                    memcpy(dst, pixels, b * sizeof(color_t));
                } else {
                    while (b) {
                        if (x >= clip->clipped_pixels_left && x < img->width - clip->clipped_pixels_right) {
                            *dst = *pixels;
                        }
                        dst++;
                        x++;
                        pixels++;
                        b--;
                    }
                }
            }
        }
    }
}

static void draw_compressed_set(const struct image_t *img, const color_t *data, int x_offset, int y_offset, int height, color_t color)
{
    const struct clip_info_t *clip = graphics_get_clip_info(x_offset, y_offset, img->width, height);
    if (!clip || !clip->is_visible) {
        return;
    }
    int unclipped = clip->clip_x == CLIP_NONE;

    for (int y = 0; y < height - clip->clipped_pixels_bottom; y++) {
        int x = 0;
        while (x < img->width) {
            color_t b = *data;
            data++;
            if (b == 255) {
                // transparent pixels to skip
                x += *data;
                data++;
            } else if (y < clip->clipped_pixels_top) {
                data += b;
                x += b;
            } else {
                data += b;
                color_t *dst = graphics_get_pixel(x_offset + x, y_offset + y);
                if (unclipped) {
                    x += b;
                    while (b) {
                        *dst = color;
                        dst++;
                        b--;
                    }
                } else {
                    while (b) {
                        if (x >= clip->clipped_pixels_left && x < img->width - clip->clipped_pixels_right) {
                            *dst = color;
                        }
                        dst++;
                        x++;
                        b--;
                    }
                }
            }
        }
    }
}

static void draw_compressed_and(const struct image_t *img, const color_t *data, int x_offset, int y_offset, int height, color_t color)
{
    const struct clip_info_t *clip = graphics_get_clip_info(x_offset, y_offset, img->width, height);
    if (!clip || !clip->is_visible) {
        return;
    }
    int unclipped = clip->clip_x == CLIP_NONE;

    for (int y = 0; y < height - clip->clipped_pixels_bottom; y++) {
        int x = 0;
        while (x < img->width) {
            color_t b = *data;
            data++;
            if (b == 255) {
                // transparent pixels to skip
                x += *data;
                data++;
            } else if (y < clip->clipped_pixels_top) {
                data += b;
                x += b;
            } else {
                // number of concrete pixels
                const color_t *pixels = data;
                data += b;
                color_t *dst = graphics_get_pixel(x_offset + x, y_offset + y);
                if (unclipped) {
                    x += b;
                    while (b) {
                        *dst = *pixels & color;
                        dst++;
                        pixels++;
                        b--;
                    }
                } else {
                    while (b) {
                        if (x >= clip->clipped_pixels_left && x < img->width - clip->clipped_pixels_right) {
                            *dst = *pixels & color;
                        }
                        dst++;
                        x++;
                        pixels++;
                        b--;
                    }
                }
            }
        }
    }
}

static void draw_footprint_tile(const color_t *data, int x_offset, int y_offset, color_t color_mask)
{
    if (!color_mask) {
        color_mask = COLOR_MASK_NONE;
    }
    const struct clip_info_t *clip = graphics_get_clip_info(x_offset, y_offset, FOOTPRINT_WIDTH, FOOTPRINT_HEIGHT);
    if (!clip || !clip->is_visible) {
        return;
    }
    // If the current tile neither clipped nor color masked, just draw it normally
    if (clip->clip_y == CLIP_NONE && clip->clip_x == CLIP_NONE && color_mask == COLOR_MASK_NONE) {
        memcpy(graphics_get_pixel(x_offset + 28, y_offset + 0), &data[0], 2 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 26, y_offset + 1), &data[2], 6 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 24, y_offset + 2), &data[8], 10 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 22, y_offset + 3), &data[18], 14 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 20, y_offset + 4), &data[32], 18 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 18, y_offset + 5), &data[50], 22 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 16, y_offset + 6), &data[72], 26 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 14, y_offset + 7), &data[98], 30 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 12, y_offset + 8), &data[128], 34 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 10, y_offset + 9), &data[162], 38 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 8, y_offset + 10), &data[200], 42 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 6, y_offset + 11), &data[242], 46 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 4, y_offset + 12), &data[288], 50 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 2, y_offset + 13), &data[338], 54 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 0, y_offset + 14), &data[392], 58 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 0, y_offset + 15), &data[450], 58 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 2, y_offset + 16), &data[508], 54 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 4, y_offset + 17), &data[562], 50 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 6, y_offset + 18), &data[612], 46 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 8, y_offset + 19), &data[658], 42 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 10, y_offset + 20), &data[700], 38 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 12, y_offset + 21), &data[738], 34 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 14, y_offset + 22), &data[772], 30 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 16, y_offset + 23), &data[802], 26 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 18, y_offset + 24), &data[828], 22 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 20, y_offset + 25), &data[850], 18 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 22, y_offset + 26), &data[868], 14 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 24, y_offset + 27), &data[882], 10 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 26, y_offset + 28), &data[892], 6 * sizeof(color_t));
        memcpy(graphics_get_pixel(x_offset + 28, y_offset + 29), &data[898], 2 * sizeof(color_t));
        return;
    }
    int clip_left = clip->clip_x == CLIP_LEFT || clip->clip_x == CLIP_BOTH;
    int clip_right = clip->clip_x == CLIP_RIGHT || clip->clip_x == CLIP_BOTH;
    const color_t *src = &data[FOOTPRINT_OFFSET_PER_HEIGHT[clip->clipped_pixels_top]];
    for (int y = clip->clipped_pixels_top; y < clip->clipped_pixels_top + clip->visible_pixels_y; y++) {
        int x_start = FOOTPRINT_X_START_PER_HEIGHT[y];
        int x_max = 58 - x_start * 2;
        int x_pixel_advance = 0;
        if (clip_left) {
            if (clip->clipped_pixels_left + clip->visible_pixels_x < x_start) {
                src += x_max;
                continue;
            }
            if (clip->clipped_pixels_left > x_start) {
                int pixels_to_reduce = clip->clipped_pixels_left - x_start;
                if (pixels_to_reduce >= x_max) {
                    src += x_max;
                    continue;
                }
                src += pixels_to_reduce;
                x_max -= pixels_to_reduce;
                x_start = clip->clipped_pixels_left;
            }
        }
        if (clip_right) {
            int clip_x = 58 - clip->clipped_pixels_right;
            if (clip_x < x_start) {
                src += x_max;
                continue;
            }
            if (x_start + x_max > clip_x) {
                int temp_x_max = clip_x - x_start;
                x_pixel_advance = x_max - temp_x_max;
                x_max = temp_x_max;
            }
        }
        color_t *buffer = graphics_get_pixel(x_offset + x_start, y_offset + y);
        if (color_mask == COLOR_MASK_NONE) {
            memcpy(buffer, src, x_max * sizeof(color_t));
            src += x_max + x_pixel_advance;
        } else {
            for (int x = 0; x < x_max; x++, buffer++, src++) {
                *buffer = *src & color_mask;
            }
            src += x_pixel_advance;
        }
    }
}

void image_draw(int image_id, int x, int y)
{
    const struct image_t *img = image_get(image_id);
    const color_t *data = image_data(image_id);
    if (!data) {
        return;
    }

    if (img->draw.is_fully_compressed) {
        draw_compressed(img, data, x, y, img->height);
    } else {
        draw_uncompressed(img, data, x, y, 0, DRAW_TYPE_NONE);
    }
}

void image_draw_enemy(struct figure_t *f, int x, int y)
{
    if (f->image_id <= 0 || f->image_id >= 801) {
        return;
    }
    const struct image_t *img = image_get_enemy(f);
    const color_t *data = image_data_enemy(f);
    if (data) {
        draw_compressed(img, data, x, y, img->height);
    }
}

void image_draw_masked(int image_id, int x, int y, color_t color_mask)
{
    const struct image_t *img = image_get(image_id);
    const color_t *data = image_data(image_id);
    if (!data) {
        return;
    }

    if (img->draw.type == IMAGE_TYPE_ISOMETRIC) {
        log_error("use image_draw_isometric_footprint for isometric!", 0, image_id);
        return;
    }

    if (img->draw.is_fully_compressed) {
        if (!color_mask) {
            draw_compressed(img, data, x, y, img->height);
        } else {
            draw_compressed_and(img, data, x, y, img->height, color_mask);
        }
    } else {
        draw_uncompressed(img, data, x, y,
                              color_mask, color_mask ? DRAW_TYPE_AND : DRAW_TYPE_NONE);
    }
}

void image_draw_blend(int image_id, int x, int y, color_t color)
{
    const struct image_t *img = image_get(image_id);
    const color_t *data = image_data(image_id);
    if (!data) {
        return;
    }

    if (img->draw.type == IMAGE_TYPE_ISOMETRIC) {
        return;
    }

    if (img->draw.is_fully_compressed) {
        const struct clip_info_t *clip = graphics_get_clip_info(x, y, img->width, img->height);
        if (!clip || !clip->is_visible) {
            return;
        }
        int unclipped = clip->clip_x == CLIP_NONE;

        for (int yy = 0; yy < img->height - clip->clipped_pixels_bottom; yy++) {
            int xx = 0;
            while (xx < img->width) {
                color_t b = *data;
                data++;
                if (b == 255) {
                    // transparent pixels to skip
                    xx += *data;
                    data++;
                } else if (yy < clip->clipped_pixels_top) {
                    data += b;
                    xx += b;
                } else {
                    data += b;
                    color_t *dst = graphics_get_pixel(x + xx, y + yy);
                    if (unclipped) {
                        xx += b;
                        while (b) {
                            *dst &= color;
                            dst++;
                            b--;
                        }
                    } else {
                        while (b) {
                            if (xx >= clip->clipped_pixels_left && xx < img->width - clip->clipped_pixels_right) {
                                *dst &= color;
                            }
                            dst++;
                            xx++;
                            b--;
                        }
                    }
                }
            }
        }
    } else {
        draw_uncompressed(img, data, x, y, color, DRAW_TYPE_BLEND);
    }
}

void image_draw_blend_alpha(int image_id, int x, int y, color_t color)
{
    const struct image_t *img = image_get(image_id);
    const color_t *data = image_data(image_id);
    if (!data) {
        return;
    }

    if (img->draw.type == IMAGE_TYPE_ISOMETRIC) {
        return;
    }

    if (img->draw.is_fully_compressed) {
        const struct clip_info_t *clip = graphics_get_clip_info(x, y, img->width, img->height);
        if (!clip || !clip->is_visible) {
            return;
        }
        color_t alpha = COMPONENT(color, 24);
        if (!alpha) {
            return;
        }
        if (alpha == 255) {
            draw_compressed_set(img, data, x, y, img->height, color);
            return;
        }
        color_t alpha_dst = 256 - alpha;
        color_t src_rb = (color & 0xff00ff) * alpha;
        color_t src_g = (color & 0x00ff00) * alpha;
        int unclipped = clip->clip_x == CLIP_NONE;
        for (int yy = 0; yy < img->height - clip->clipped_pixels_bottom; yy++) {
            int xx = 0;
            color_t *dst = graphics_get_pixel(x, y + yy);
            while (xx < img->width) {
                color_t b = *data;
                data++;
                if (b == 255) {
                    // transparent pixels to skip
                    xx += *data;
                    dst += *data;
                    data++;
                } else if (yy < clip->clipped_pixels_top) {
                    data += b;
                    xx += b;
                    dst += b;
                } else {
                    data += b;
                    if (unclipped) {
                        xx += b;
                        while (b) {
                            color_t d = *dst;
                            *dst = (((src_rb + (d & 0xff00ff) * alpha_dst) & 0xff00ff00) |
                                    ((src_g + (d & 0x00ff00) * alpha_dst) & 0x00ff0000)) >> 8;
                            b--;
                            dst++;
                        }
                    } else {
                        while (b) {
                            if (xx >= clip->clipped_pixels_left && xx < img->width - clip->clipped_pixels_right) {
                                color_t d = *dst;
                                *dst = (((src_rb + (d & 0xff00ff) * alpha_dst) & 0xff00ff00) |
                                       ((src_g + (d & 0x00ff00) * alpha_dst) & 0x00ff0000)) >> 8;
                            }
                            dst++;
                            xx++;
                            b--;
                        }
                    }
                }
            }
        }
    } else {
        draw_uncompressed(img, data, x, y, color, DRAW_TYPE_BLEND_ALPHA);
    }
}

void image_draw_fullscreen_background(int image_id)
{
    int s_width = screen_width();
    int s_height = screen_height();
    const struct image_t *img = image_get(image_id);
    double scale_w = screen_width() / (double) img->width;
    double scale_h = screen_height() / (double) img->height;
    double scale = scale_w > scale_h ? scale_w : scale_h;

    if (scale <= 1.0f) {
        image_draw(image_id, (s_width - img->width) / 2, (s_height - img->height) / 2);
    } else {
        int x_offset = (int) ((s_width - img->width * scale) / 2);
        int y_offset = (int) ((s_height - img->height * scale) / 2);
        const color_t *data = image_data(image_id);
        if (!data || img->draw.type == IMAGE_TYPE_ISOMETRIC || img->draw.is_fully_compressed || !scale) {
            return;
        }
        int width = (int) (img->width * scale);
        int height = (int) (img->height * scale);
        const struct clip_info_t *clip = graphics_get_clip_info(x_offset, y_offset, width, height);
        if (!clip || !clip->is_visible) {
            return;
        }
        for (int y = clip->clipped_pixels_top; y < height - clip->clipped_pixels_bottom; y++) {
            color_t *dst = graphics_get_pixel(x_offset + clip->clipped_pixels_left, y_offset + y);
            int x_max = width - clip->clipped_pixels_right;
            int image_y_offset = (int) (y / scale) * img->width;
            for (int x = clip->clipped_pixels_left; x < x_max; x++, dst++) {
                color_t pixel = data[(int) (image_y_offset + x / scale)];
                if (pixel != COLOR_SG2_TRANSPARENT) {
                    *dst = pixel;
                }
            }
        }
    }
}

static const color_t *tile_data(const color_t *data, int index)
{
    return &data[900 * index];
}

static void draw_footprint_size1(int image_id, int x, int y, color_t color_mask)
{
    const color_t *data = image_data(image_id);

    draw_footprint_tile(tile_data(data, 0), x, y, color_mask);
}

static void draw_footprint_size2(int image_id, int x, int y, color_t color_mask)
{
    const color_t *data = image_data(image_id);

    int index = 0;
    draw_footprint_tile(tile_data(data, index++), x, y, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 30, y + 15, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 30, y + 15, color_mask);

    draw_footprint_tile(tile_data(data, index++), x, y + 30, color_mask);
}

static void draw_footprint_size3(int image_id, int x, int y, color_t color_mask)
{
    const color_t *data = image_data(image_id);

    int index = 0;
    draw_footprint_tile(tile_data(data, index++), x, y, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 30, y + 15, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 30, y + 15, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 60, y + 30, color_mask);
    draw_footprint_tile(tile_data(data, index++), x, y + 30, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 60, y + 30, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 30, y + 45, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 30, y + 45, color_mask);

    draw_footprint_tile(tile_data(data, index++), x, y + 60, color_mask);
}

static void draw_footprint_size4(int image_id, int x, int y, color_t color_mask)
{
    const color_t *data = image_data(image_id);

    int index = 0;
    draw_footprint_tile(tile_data(data, index++), x, y, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 30, y + 15, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 30, y + 15, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 60, y + 30, color_mask);
    draw_footprint_tile(tile_data(data, index++), x, y + 30, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 60, y + 30, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 90, y + 45, color_mask);
    draw_footprint_tile(tile_data(data, index++), x - 30, y + 45, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 30, y + 45, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 90, y + 45, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 60, y + 60, color_mask);
    draw_footprint_tile(tile_data(data, index++), x, y + 60, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 60, y + 60, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 30, y + 75, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 30, y + 75, color_mask);

    draw_footprint_tile(tile_data(data, index++), x, y + 90, color_mask);
}

static void draw_footprint_size5(int image_id, int x, int y, color_t color_mask)
{
    const color_t *data = image_data(image_id);

    int index = 0;
    draw_footprint_tile(tile_data(data, index++), x, y, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 30, y + 15, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 30, y + 15, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 60, y + 30, color_mask);
    draw_footprint_tile(tile_data(data, index++), x, y + 30, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 60, y + 30, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 90, y + 45, color_mask);
    draw_footprint_tile(tile_data(data, index++), x - 30, y + 45, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 30, y + 45, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 90, y + 45, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 120, y + 60, color_mask);
    draw_footprint_tile(tile_data(data, index++), x - 60, y + 60, color_mask);
    draw_footprint_tile(tile_data(data, index++), x, y + 60, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 60, y + 60, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 120, y + 60, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 90, y + 75, color_mask);
    draw_footprint_tile(tile_data(data, index++), x - 30, y + 75, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 30, y + 75, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 90, y + 75, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 60, y + 90, color_mask);
    draw_footprint_tile(tile_data(data, index++), x, y + 90, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 60, y + 90, color_mask);

    draw_footprint_tile(tile_data(data, index++), x - 30, y + 105, color_mask);
    draw_footprint_tile(tile_data(data, index++), x + 30, y + 105, color_mask);

    draw_footprint_tile(tile_data(data, index++), x, y + 120, color_mask);
}

void image_draw_isometric_footprint(int image_id, int x, int y, color_t color_mask)
{
    const struct image_t *img = image_get(image_id);
    if (img->draw.type != IMAGE_TYPE_ISOMETRIC) {
        return;
    }
    switch (img->width) {
        case 58:
            draw_footprint_size1(image_id, x, y, color_mask);
            break;
        case 118:
            draw_footprint_size2(image_id, x, y, color_mask);
            break;
        case 178:
            draw_footprint_size3(image_id, x, y, color_mask);
            break;
        case 238:
            draw_footprint_size4(image_id, x, y, color_mask);
            break;
        case 298:
            draw_footprint_size5(image_id, x, y, color_mask);
            break;
    }
}

void image_draw_isometric_footprint_from_draw_tile(int image_id, int x, int y, color_t color_mask)
{
    const struct image_t *img = image_get(image_id);
    if (img->draw.type != IMAGE_TYPE_ISOMETRIC) {
        return;
    }
    switch (img->width) {
        case 58:
            draw_footprint_size1(image_id, x, y, color_mask);
            break;
        case 118:
            draw_footprint_size2(image_id, x + 30, y - 15, color_mask);
            break;
        case 178:
            draw_footprint_size3(image_id, x + 60, y - 30, color_mask);
            break;
        case 238:
            draw_footprint_size4(image_id, x + 90, y - 45, color_mask);
            break;
        case 298:
            draw_footprint_size5(image_id, x + 120, y - 60, color_mask);
            break;
    }
}

void image_draw_isometric_top(int image_id, int x, int y, color_t color_mask)
{
    const struct image_t *img = image_get(image_id);
    if (img->draw.type != IMAGE_TYPE_ISOMETRIC) {
        return;
    }
    if (!img->draw.has_compressed_part) {
        return;
    }
    const color_t *data = &image_data(image_id)[img->draw.uncompressed_length];

    int height = img->height;
    switch (img->width) {
        case 58:
            y -= img->height - 30;
            height -= 16;
            break;
        case 118:
            x -= 30;
            y -= img->height - 60;
            height -= 31;
            break;
        case 178:
            x -= 60;
            y -= img->height - 90;
            height -= 46;
            break;
        case 238:
            x -= 90;
            y -= img->height - 120;
            height -= 61;
            break;
        case 298:
            x -= 120;
            y -= img->height - 150;
            height -= 76;
            break;
    }
    if (!color_mask) {
        draw_compressed(img, data, x, y, height);
    } else {
        draw_compressed_and(img, data, x, y, height, color_mask);
    }
}

void image_draw_isometric_top_from_draw_tile(int image_id, int x, int y, color_t color_mask)
{
    const struct image_t *img = image_get(image_id);
    if (img->draw.type != IMAGE_TYPE_ISOMETRIC) {
        return;
    }
    if (!img->draw.has_compressed_part) {
        return;
    }
    const color_t *data = &image_data(image_id)[img->draw.uncompressed_length];

    int height = img->height;
    switch (img->width) {
        case 58:
            y -= img->height - 30;
            height -= 16;
            break;
        case 118:
            y -= img->height - 45;
            height -= 31;
            break;
        case 178:
            y -= img->height - 60;
            height -= 46;
            break;
        case 238:
            y -= img->height - 75;
            height -= 61;
            break;
        case 298:
            y -= img->height - 90;
            height -= 76;
            break;
    }
    if (!color_mask) {
        draw_compressed(img, data, x, y, height);
    } else {
        draw_compressed_and(img, data, x, y, height, color_mask);
    }
}

int lang_text_get_width(int group, int number, int font)
{
    const char *str = lang_get_string(group, number);
    return text_get_width(str, font) + font_definition_for(font)->space_width;
}

int lang_text_draw(int group, int number, int x_offset, int y_offset, int font)
{
    const char *str = lang_get_string(group, number);
    return text_draw(str, x_offset, y_offset, font, 0);
}

int lang_text_draw_colored(int group, int number, int x_offset, int y_offset, int font, color_t color)
{
    const char *str = lang_get_string(group, number);
    return text_draw(str, x_offset, y_offset, font, color);
}

void  lang_text_draw_centered(int group, int number, int x_offset, int y_offset, int box_width, int font)
{
    const char *str = lang_get_string(group, number);
    text_draw_centered(str, x_offset, y_offset, box_width, font, 0);
}

void lang_text_draw_centered_colored(int group, int number, int x_offset, int y_offset, int box_width, int font, color_t color)
{
    const char *str = lang_get_string(group, number);
    text_draw_centered(str, x_offset, y_offset, box_width, font, color);
}

int lang_text_draw_amount(int group, int number, int amount, int x_offset, int y_offset, int font)
{
    int amount_offset = 1;
    if (amount == 1 || amount == -1) {
        amount_offset = 0;
    }
    int desc_offset_x;
    if (amount >= 0) {
        desc_offset_x = text_draw_number(amount, ' ', " ",
            x_offset, y_offset, font);
    } else {
        desc_offset_x = text_draw_number(-amount, '-', " ",
            x_offset, y_offset, font);
    }
    return desc_offset_x + lang_text_draw(group, number + amount_offset,
        x_offset + desc_offset_x, y_offset, font);
}

int lang_text_draw_year(int year, int x_offset, int y_offset, int font)
{
    int width = 0;
    if (year >= 0) {
        width += text_draw_number(year, ' ', " ", x_offset + width, y_offset, font);
        width += lang_text_draw(20, 1, x_offset + width, y_offset, font);
    } else {
        width += text_draw_number(-year, ' ', " ", x_offset + width, y_offset, font);
        width += lang_text_draw(20, 0, x_offset + width, y_offset, font);
    }
    return width;
}

void lang_text_draw_month_year_max_width(int month, int year, int x_offset, int y_offset, int box_width, int font, color_t color)
{
    int month_width = lang_text_get_width(25, month, font);
    int ad_bc_width = lang_text_get_width(20, year >= 0 ? 1 : 0, font);
    int space_width = font_definition_for(font)->space_width;

    int negative_padding = 0;
    // assume 3 digits in the year times 11 pixels plus letter spacing = approx 35px
    int total_width = month_width + ad_bc_width + 35 + 2 * space_width;
    if (total_width > box_width) {
        // take the overflow and divide it by two since we have two places to correct: after month, and after year
        negative_padding = (box_width - total_width) / 2;
        if (negative_padding < -2 * (space_width - 2)) {
            negative_padding = -2 * (space_width - 2);
        }
    }

    int width = negative_padding + lang_text_draw_colored(25, month, x_offset, y_offset, font, color);
    if (year >= 0) {
        width += negative_padding +
            text_draw_number_colored(year, ' ', " ", x_offset + width, y_offset, font, color);
        lang_text_draw_colored(20, 1, x_offset + width, y_offset, font, color);
    } else {
        width += negative_padding + text_draw_number_colored(-year, ' ', " ", x_offset + width, y_offset, font, color);
        lang_text_draw_colored(20, 0, x_offset + width, y_offset, font, color);
    }
}

int lang_text_draw_multiline(int group, int number, int x_offset, int y_offset, int box_width, int font)
{
    const char *str = lang_get_string(group, number);
    return text_draw_multiline(str, x_offset, y_offset, box_width, font, 0);
}

void menu_bar_draw(struct menu_bar_item_t *items, int num_items, int max_width)
{
    int total_text_width = 0;
    for (int i = 0; i < num_items; i++) {
        total_text_width += lang_text_get_width(items[i].text_group, 0, FONT_NORMAL_GREEN);
    }
    if (num_items > 1) {
        int spacing_width = (max_width - total_text_width - TOP_MENU_BASE_X_OFFSET) / (num_items - 1);
        spacing_width = calc_bound(spacing_width, 0, 32);

        short x_offset = TOP_MENU_BASE_X_OFFSET;
        for (int i = 0; i < num_items; i++) {
            items[i].x_start = x_offset;
            x_offset += lang_text_draw(items[i].text_group, 0, x_offset, MENU_BASE_TEXT_Y_OFFSET, FONT_NORMAL_GREEN);
            items[i].x_end = x_offset;
            x_offset += spacing_width;
        }
    }
}

int menu_bar_handle_mouse(const struct mouse_t *m, struct menu_bar_item_t *items, int num_items, int *focus_menu_id)
{
    int menu_id = 0;
    for (int i = 0; i < num_items; i++) {
        if (items[i].x_start <= m->x &&
            items[i].x_end > m->x &&
            MENU_BASE_TEXT_Y_OFFSET <= m->y &&
            MENU_BASE_TEXT_Y_OFFSET + 12 > m->y) {
            menu_id = i + 1;
            break;
        }
    }

    if (focus_menu_id) {
        *focus_menu_id = menu_id;
    }
    return menu_id;
}

void menu_draw(struct menu_bar_item_t *menu, int focus_item_id)
{
    if (menu->calculated_width_blocks == 0 || menu->calculated_height_blocks == 0) {
        int max_width = 0;
        int height_pixels = MENU_ITEM_HEIGHT;
        for (int i = 0; i < menu->num_items; i++) {
            struct menu_item_t *sub = &menu->items[i];
            if (sub->hidden) {
                continue;
            }
            int width_pixels = lang_text_get_width(
                sub->text_group, sub->text_number, FONT_NORMAL_BLACK);
            if (width_pixels > max_width) {
                max_width = width_pixels;
            }
            height_pixels += MENU_ITEM_HEIGHT;
        }
        int blocks = (max_width + 8) / BLOCK_SIZE + 1; // 1 block padding
        menu->calculated_width_blocks = blocks < 10 ? 10 : blocks;
        menu->calculated_height_blocks = height_pixels / BLOCK_SIZE;
    }
    int image_base = image_group(GROUP_DIALOG_BACKGROUND);
    int image_y = 0;
    for (int yy = 0; yy < menu->calculated_height_blocks; yy++) {
        int image_x = 0;
        for (int xx = 0; xx < menu->calculated_width_blocks; xx++) {
            int image_id = 13 + image_y + image_x++;
            image_draw(image_base + image_id, menu->x_start + BLOCK_SIZE * xx, TOP_MENU_HEIGHT + BLOCK_SIZE * yy);
            if (image_x >= 10) {
                image_x = 0;
            }
        }
        image_y += 12;
        if (image_y >= 120) {
            image_y = 0;
        }
    }
    int y_offset = TOP_MENU_HEIGHT + MENU_BASE_TEXT_Y_OFFSET * 2;
    for (int i = 0; i < menu->num_items; i++) {
        struct menu_item_t *sub = &menu->items[i];
        if (sub->hidden) {
            continue;
        }
        if (i == focus_item_id - 1) {
            graphics_fill_rect(menu->x_start, y_offset - 4, BLOCK_SIZE * menu->calculated_width_blocks, 20, COLOR_BLACK);
            if (sub->text_group == 10 && sub->text_number == 4) {
                text_draw(editor_top_menu_clear_earthquakes_string, menu->x_start + 8, y_offset, FONT_NORMAL_PLAIN, COLOR_FONT_ORANGE);
            } else {
                lang_text_draw_colored(sub->text_group, sub->text_number, menu->x_start + 8, y_offset, FONT_NORMAL_PLAIN, COLOR_FONT_ORANGE);
            }
        } else {
            if (sub->text_group == 10 && sub->text_number == 4) {
                text_draw(editor_top_menu_clear_earthquakes_string, menu->x_start + 8, y_offset, FONT_NORMAL_BLACK, COLOR_BLACK);
            } else {
                lang_text_draw(sub->text_group, sub->text_number, menu->x_start + 8, y_offset, FONT_NORMAL_BLACK);
            }
        }
        y_offset += MENU_ITEM_HEIGHT;
    }
}

int menu_handle_mouse(const struct mouse_t *m, struct menu_bar_item_t *menu, int *focus_item_id)
{
    int item_id = 0;
    int y_offset = TOP_MENU_HEIGHT + MENU_BASE_TEXT_Y_OFFSET * 2;
    for (int i = 0; i < menu->num_items; i++) {
        if (menu->items[i].hidden) {
            continue;
        }
        if (menu->x_start <= m->x &&
            menu->x_start + BLOCK_SIZE * menu->calculated_width_blocks > m->x &&
            y_offset - 2 <= m->y &&
            y_offset + 19 > m->y) {
            item_id = i + 1;
            break;
        }
        y_offset += MENU_ITEM_HEIGHT;
    }

    if (focus_item_id) {
        *focus_item_id = item_id;
    }
    if (!item_id) {
        return 0;
    }
    if (m->left.went_up) {
        struct menu_item_t *item = &menu->items[item_id - 1];
        item->left_click_handler(item->parameter);
    }
    return item_id;
}

void menu_update_text(struct menu_bar_item_t *menu, int index, int text_number)
{
    menu->items[index].text_number = text_number;
    if (menu->calculated_width_blocks > 0) {
        int item_width = lang_text_get_width(
            menu->items[index].text_group, text_number, FONT_NORMAL_BLACK);
        int blocks = (item_width + 8) / BLOCK_SIZE + 1;
        if (blocks > menu->calculated_width_blocks) {
            menu->calculated_width_blocks = blocks;
        }
    }
}

void outer_panel_draw(int x, int y, int width_blocks, int height_blocks)
{
    int image_base = image_group(GROUP_DIALOG_BACKGROUND);
    int image_id;
    int image_y = 0;
    int y_add = 0;
    for (int yy = 0; yy < height_blocks; yy++) {
        int image_x = 0;
        for (int xx = 0; xx < width_blocks; xx++) {
            if (yy == 0) {
                if (xx == 0) {
                    image_id = 0;
                } else if (xx < width_blocks - 1) {
                    image_id = 1 + image_x++;
                } else {
                    image_id = 11;
                }
                y_add = 0;
            } else if (yy < height_blocks - 1) {
                if (xx == 0) {
                    image_id = 12 + image_y;
                } else if (xx < width_blocks - 1) {
                    image_id = 13 + image_y + image_x++;
                } else {
                    image_id = 23 + image_y;
                }
                y_add = 12;
            } else {
                if (xx == 0) {
                    image_id = 132;
                } else if (xx < width_blocks - 1) {
                    image_id = 133 + image_x++;
                } else {
                    image_id = 143;
                }
                y_add = 0;
            }
            image_draw(image_base + image_id, x + BLOCK_SIZE * xx, y + BLOCK_SIZE * yy);
            if (image_x >= 10) {
                image_x = 0;
            }
        }
        image_y += y_add;
        if (image_y >= 120) {
            image_y = 0;
        }
    }
}

void inner_panel_draw(int x, int y, int width_blocks, int height_blocks)
{
    int image_base = image_group(GROUP_SUNKEN_TEXTBOX_BACKGROUND);
    int image_y = 0;
    int y_add = 0;
    for (int yy = 0; yy < height_blocks; yy++) {
        int image_x = 0;
        for (int xx = 0; xx < width_blocks; xx++) {
            int image_id;
            if (yy == 0) {
                if (xx == 0) {
                    image_id = 0;
                } else if (xx < width_blocks - 1) {
                    image_id = 1 + image_x++;
                } else {
                    image_id = 6;
                }
                y_add = 0;
            } else if (yy < height_blocks - 1) {
                if (xx == 0) {
                    image_id = 7 + image_y;
                } else if (xx < width_blocks - 1) {
                    image_id = 8 + image_y + image_x++;
                } else {
                    image_id = 13 + image_y;
                }
                y_add = 7;
            } else {
                if (xx == 0) {
                    image_id = 42;
                } else if (xx < width_blocks - 1) {
                    image_id = 43 + image_x++;
                } else {
                    image_id = 48;
                }
                y_add = 0;
            }
            image_draw(image_base + image_id, x + BLOCK_SIZE * xx, y + BLOCK_SIZE * yy);
            if (image_x >= 5) {
                image_x = 0;
            }
        }
        image_y += y_add;
        if (image_y >= 35) {
            image_y = 0;
        }
    }
}

void label_draw(int x, int y, int width_blocks, int type)
{
    int image_base = image_group(GROUP_PANEL_BUTTON);
    for (int i = 0; i < width_blocks; i++) {
        int image_id;
        if (i == 0) {
            image_id = 3 * type + 40;
        } else if (i < width_blocks - 1) {
            image_id = 3 * type + 41;
        } else {
            image_id = 3 * type + 42;
        }
        image_draw(image_base + image_id, x + BLOCK_SIZE * i, y);
    }
}

void large_label_draw(int x, int y, int width_blocks, int type)
{
    int image_base = image_group(GROUP_PANEL_BUTTON);
    for (int i = 0; i < width_blocks; i++) {
        int image_id;
        if (i == 0) {
            image_id = 3 * type;
        } else if (i < width_blocks - 1) {
            image_id = 3 * type + 1;
        } else {
            image_id = 3 * type + 2;
        }
        image_draw(image_base + image_id, x + BLOCK_SIZE * i, y);
    }
}

static void on_scroll(void)
{
    rich_text_clear_links();
    window_invalidate();
}

static struct scrollbar_type_t scrollbar = { 0, 0, 0, on_scroll, 0, 0, 0, 0, 0, 0 };

int rich_text_init(const char *text, int x_text, int y_text, int width_blocks, int height_blocks, int adjust_width_on_no_scroll)
{
    rich_text_data.x_text = x_text;
    rich_text_data.y_text = y_text;
    if (!rich_text_data.num_lines) {
        rich_text_data.text_height_blocks = height_blocks;
        rich_text_data.text_height_lines = (height_blocks - 1) * BLOCK_SIZE / rich_text_data.line_height;
        rich_text_data.text_width_blocks = width_blocks;

        rich_text_data.num_lines = rich_text_draw(text,
            rich_text_data.x_text + 8, rich_text_data.y_text + 6,
            BLOCK_SIZE * rich_text_data.text_width_blocks - BLOCK_SIZE, rich_text_data.text_height_lines, 1);
        scrollbar.x = rich_text_data.x_text + BLOCK_SIZE * rich_text_data.text_width_blocks - 1;
        scrollbar.y = rich_text_data.y_text;
        scrollbar.height = BLOCK_SIZE * rich_text_data.text_height_blocks;
        scrollbar_init(&scrollbar, scrollbar.scroll_position, rich_text_data.num_lines - rich_text_data.text_height_lines);
        if (rich_text_data.num_lines <= rich_text_data.text_height_lines && adjust_width_on_no_scroll) {
            rich_text_data.text_width_blocks += 2;
        }
        window_invalidate();
    }
    return rich_text_data.text_width_blocks;
}

void rich_text_set_fonts(int normal_font, int link_font, int line_spacing)
{
    rich_text_data.normal_font = font_definition_for(normal_font);
    rich_text_data.link_font = font_definition_for(link_font);
    rich_text_data.line_height = rich_text_data.normal_font->line_height + line_spacing;
    rich_text_data.paragraph_indent = 50;
}

void rich_text_reset(int scroll_position)
{
    scrollbar_reset(&scrollbar, scroll_position);
    rich_text_data.num_lines = 0;
    rich_text_clear_links();
}

void rich_text_reset_lines_only(void)
{
    rich_text_data.num_lines = 0;
}

void rich_text_clear_links(void)
{
    for (int i = 0; i < MAX_LINKS; i++) {
        links[i].message_id = 0;
        links[i].x_min = 0;
        links[i].x_max = 0;
        links[i].y_min = 0;
        links[i].y_max = 0;
    }
    rich_text_data.num_links = 0;
}

int rich_text_get_clicked_link(const struct mouse_t *m)
{
    if (m->left.went_up) {
        for (int i = 0; i < rich_text_data.num_links; i++) {
            if (m->x >= links[i].x_min && m->x <= links[i].x_max &&
                m->y >= links[i].y_min && m->y <= links[i].y_max) {
                return links[i].message_id;
            }
        }
    }
    return -1;
}

static int get_word_width_rich_text(const char *str, int *num_chars)
{
    int width = 0;
    int guard = 0;
    int word_char_seen = 0;
    *num_chars = 0;
    while (*str && ++guard < 2000) {
        if (*str == ' ') {
            if (word_char_seen) {
                break;
            }
            width += 4;
        } else if (*str > ' ') {
            // normal char
            int letter_id = font_letter_id(rich_text_data.normal_font, str);
            if (letter_id >= 0) {
                width += 1 + image_letter(letter_id)->width;
            }
            word_char_seen = 1;
        }
        str += 1;
        *num_chars += 1;
    }
    return width;
}

static void image_draw_letter(int letter_id, int x, int y, color_t color)
{
    const struct image_t *img = image_letter(letter_id);
    const color_t *data = image_data_letter(letter_id);
    if (!data) {
        return;
    }

    if (img->draw.is_fully_compressed) {
        if (color) {
            draw_compressed_set(img, data, x, y, img->height, color);
        } else {
            draw_compressed(img, data, x, y, img->height);
        }
    } else {
        draw_uncompressed(img, data, x, y,
            color, color ? DRAW_TYPE_SET : DRAW_TYPE_NONE);
    }
}

static void draw_line(const char *str, int x, int y, color_t color, int measure_only)
{
    int num_link_chars = 0;
    while (*str) {
        if (*str == '@') {
            int message_id = string_to_int(++str);
            while (*str >= '0' && *str <= '9') {
                str++;
            }
            int width = get_word_width_rich_text(str, &num_link_chars);
            if (rich_text_data.num_links < MAX_LINKS) {
                links[rich_text_data.num_links].message_id = message_id;
                links[rich_text_data.num_links].x_min = x - 2;
                links[rich_text_data.num_links].x_max = x + width + 2;
                links[rich_text_data.num_links].y_min = y - 1;
                links[rich_text_data.num_links].y_max = y + 13;
                rich_text_data.num_links++;
            }
        }
        if (*str >= ' ') {
            const struct font_definition_t *def = rich_text_data.normal_font;
            if (num_link_chars > 0) {
                def = rich_text_data.link_font;
            }
            int letter_id = font_letter_id(def, str);
            if (letter_id < 0) {
                x += def->space_width;
            } else {
                const struct image_t *img = image_letter(letter_id);
                if (!measure_only) {
                    int height = def->image_y_offset(*str, img->height, def->line_height);
                    image_draw_letter(letter_id, x, y - height, color);
                }
                x += img->width + def->letter_spacing;
            }
            if (num_link_chars > 0) {
                num_link_chars -= 1;
            }
            str += 1;
        } else {
            str++;
        }
    }
}

static int draw_text(const char *text, int x_offset, int y_offset, int box_width, int height_lines, color_t color, int measure_only)
{
    int image_height_lines = 0;
    int image_id = 0;
    int lines_before_image = 0;
    int paragraph = 0;
    int has_more_characters = 1;
    int y = y_offset;
    int guard = 0;
    int line = 0;
    int num_lines = 0;
    while (has_more_characters || image_height_lines) {
        if (++guard >= 1000) {
            break;
        }
        // clear line
        for (int i = 0; i < 200; i++) {
            tmp_line_rich_text[i] = 0;
        }
        int line_index = 0;
        int current_width, x_line_offset;
        current_width = x_line_offset = paragraph ? rich_text_data.paragraph_indent : 0;
        paragraph = 0;
        while ((has_more_characters || image_height_lines) && current_width < box_width) {
            if (image_height_lines) {
                image_height_lines--;
                break;
            }
            int word_num_chars;
            current_width += get_word_width_rich_text(text, &word_num_chars);
            if (current_width >= box_width) {
                if (current_width == 0) {
                    has_more_characters = 0;
                }
            } else {
                for (int i = 0; i < word_num_chars; i++) {
                    char c = *text++;
                    if (c == '@') {
                        if (*text == 'P') {
                            paragraph = 1;
                            text++;
                            current_width = box_width;
                            break;
                        } else if (*text == 'L') {
                            text++;
                            current_width = box_width;
                            break;
                        } else if (*text == 'G') {
                            if (line_index) {
                                num_lines++;
                            }
                            text++; // skip 'G'
                            current_width = box_width;
                            image_id = string_to_int(text);
                            c = *text++;
                            while (c >= '0' && c <= '9') {
                                c = *text++;
                            }
                            image_id += image_group(GROUP_MESSAGE_IMAGES) - 1;
                            image_height_lines = image_get(image_id)->height / rich_text_data.line_height + 2;
                            if (line > 0) {
                                lines_before_image = 1;
                            }
                            break;
                        }
                    }
                    if (line_index || c != ' ') { // no space at start of line
                        tmp_line_rich_text[line_index++] = c;
                    }
                }
                if (!*text) {
                    has_more_characters = 0;
                }
            }
        }

        int outside_viewport = 0;
        if (!measure_only) {
            if (line < scrollbar.scroll_position || line >= scrollbar.scroll_position + height_lines) {
                outside_viewport = 1;
            }
        }
        if (!outside_viewport) {
            draw_line(tmp_line_rich_text, x_line_offset + x_offset, y, color, measure_only);
        }
        if (!measure_only) {
            if (image_id) {
                if (lines_before_image) {
                    lines_before_image--;
                } else {
                    const struct image_t *img = image_get(image_id);
                    image_height_lines = img->height / rich_text_data.line_height + 2;
                    int image_offset_x = x_offset + (box_width - img->width) / 2 - 4;
                    if (line < height_lines + scrollbar.scroll_position) {
                        if (line >= scrollbar.scroll_position) {
                            image_draw(image_id, image_offset_x, y + 8);
                        } else {
                            image_draw(image_id, image_offset_x,
                                y + 8 - rich_text_data.line_height * (scrollbar.scroll_position - line));
                        }
                    }
                    image_id = 0;
                }
            }
        }
        line++;
        num_lines++;
        if (!outside_viewport) {
            y += rich_text_data.line_height;
        }
    }
    return num_lines;
}

int rich_text_draw(const char *text, int x_offset, int y_offset, int box_width, int height_lines, int measure_only)
{
    return draw_text(text, x_offset, y_offset, box_width, height_lines, 0, measure_only);
}

int rich_text_draw_colored(
    const char *text, int x_offset, int y_offset, int box_width, int height_lines, color_t color)
{
    return draw_text(text, x_offset, y_offset, box_width, height_lines, color, 0);
}

void rich_text_draw_scrollbar(void)
{
    scrollbar_draw(&scrollbar);
}

int rich_text_handle_mouse(const struct mouse_t *m)
{
    return scrollbar_handle_mouse(&scrollbar, m);
}

int rich_text_scroll_position(void)
{
    return scrollbar.scroll_position;
}

void screen_set_resolution(int width, int height)
{
    screen_data.width = width;
    screen_data.height = height;
    screen_data.dialog_offset.x = (width - 640) / 2;
    screen_data.dialog_offset.y = (height - 480) / 2;

    canvas.pixels = (color_t *) malloc((size_t) width * height * sizeof(color_t));
    if (!canvas.pixels) {
        return;
    }

    memset(canvas.pixels, 0, (size_t) width * height * sizeof(color_t));
    canvas.width = width;
    canvas.height = height;

    graphics_set_clip_rectangle(0, 0, width, height);

    city_view_set_viewport(width, height);
    city_warning_clear_all();
    window_invalidate();
}

int screen_width(void)
{
    return screen_data.width;
}

int screen_height(void)
{
    return screen_data.height;
}

int screen_dialog_offset_x(void)
{
    return screen_data.dialog_offset.x;
}

int screen_dialog_offset_y(void)
{
    return screen_data.dialog_offset.y;
}

static void image_free(void)
{
    image.width = 0;
    image.height = 0;
    image.row_size = 0;
    image.rows_in_memory = 0;
    free(image.pixels);
    image.pixels = 0;
    if (image.fp) {
        fclose(image.fp);
        image.fp = 0;
    }
    png_destroy_write_struct(&image.png_ptr, &image.info_ptr);
}

static int image_create(int width, int height, int rows_in_memory)
{
    image_free();
    if (!width || !height || !rows_in_memory) {
        return 0;
    }
    image.png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, 0, 0, 0);
    if (!image.png_ptr) {
        return 0;
    }
    image.info_ptr = png_create_info_struct(image.png_ptr);
    if (!image.info_ptr) {
        image_free();
        return 0;
    }
    png_set_compression_level(image.png_ptr, 3);
    image.width = width;
    image.height = height;
    image.row_size = width * IMAGE_BYTES_PER_PIXEL;
    image.rows_in_memory = rows_in_memory;
    image.pixels = (uint8_t *) malloc(image.row_size);
    if (!image.pixels) {
        image_free();
        return 0;
    }
    memset(image.pixels, 0, image.row_size);
    return 1;
}

static const char *generate_filename(int city_screenshot)
{
    static char filename[FILE_NAME_MAX];
    time_t curtime = time(NULL);
    struct tm *loctime = localtime(&curtime);
    strftime(filename, FILE_NAME_MAX, filename_formats[city_screenshot], loctime);
    return filename;
}

static int image_begin_io(const char *filename)
{
    FILE *fp = fopen(filename, "wb");
    if (!fp) {
        return 0;
    }
    image.fp = fp;
    png_init_io(image.png_ptr, fp);
    return 1;
}

static int image_write_header(void)
{
    if (setjmp(png_jmpbuf(image.png_ptr))) {
        return 0;
    }
    png_set_IHDR(image.png_ptr, image.info_ptr, image.width, image.height, 8, PNG_COLOR_TYPE_RGB,
                    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    png_write_info(image.png_ptr, image.info_ptr);
    return 1;
}

static int image_set_loop_height_limits(int min, int max)
{
    image.current_y = min;
    image.final_y = max;
    return image.current_y;
}

static int image_request_rows(void)
{
    if (image.current_y != image.final_y) {
        image.current_y += image.rows_in_memory;
        return image.rows_in_memory;
    }
    return 0;
}

static int image_write_rows(const color_t *canvas, int canvas_width)
{
    if (setjmp(png_jmpbuf(image.png_ptr))) {
        return 0;
    }
    for (int y = 0; y < image.rows_in_memory; ++y) {
        uint8_t *pixel = image.pixels;
        for (int x = 0; x < image.width; x++) {
            color_t input = canvas[y * canvas_width + x];
            *(pixel + 0) = (uint8_t) ((input & 0xff0000) >> 16);
            *(pixel + 1) = (uint8_t) ((input & 0x00ff00) >> 8);
            *(pixel + 2) = (uint8_t) ((input & 0x0000ff) >> 0);
            pixel += 3;
        }
        png_write_row(image.png_ptr, image.pixels);
    }
    return 1;
}

static void show_saved_notice(const char *filename)
{
    char notice_text[FILE_NAME_MAX] = "Screenshot saved: ";
    int prefix_length = string_length("Screenshot saved: ");
    string_copy(string_from_ascii(filename), &notice_text[prefix_length], FILE_NAME_MAX - prefix_length);

    city_warning_show_custom(notice_text);
}

void graphics_save_screenshot(int full_city)
{
    if (full_city) {
        if (!window_is(WINDOW_CITY) && !window_is(WINDOW_CITY_MILITARY)) {
            return;
        }
        struct pixel_view_coordinates_t original_camera_pixels;
        city_view_get_camera_in_pixels(&original_camera_pixels.x, &original_camera_pixels.y);
        int width = screen_width();
        int height = screen_height();

        int city_width_pixels = map_data.width * TILE_X_SIZE;
        int city_height_pixels = map_data.height * TILE_Y_SIZE;

        if (!image_create(city_width_pixels, city_height_pixels + TILE_Y_SIZE, IMAGE_HEIGHT_CHUNK)) {
            log_error("Unable to set memory for full city screenshot", 0, 0);
            return;
        }
        const char *filename = generate_filename(FULL_CITY_SCREENSHOT);
        if (!image_begin_io(filename) || !image_write_header()) {
            log_error("Unable to write screenshot to:", filename, 0);
            image_free();
            return;
        }

        int canvas_width = city_width_pixels + (city_view_is_sidebar_collapsed() ? 40 : 160);
        screen_set_resolution(canvas_width, TOP_MENU_HEIGHT + IMAGE_HEIGHT_CHUNK);
        graphics_set_clip_rectangle(0, TOP_MENU_HEIGHT, city_width_pixels, IMAGE_HEIGHT_CHUNK);

        int base_width = (GRID_SIZE * TILE_X_SIZE - city_width_pixels) / 2 + TILE_X_SIZE;
        int max_height = (GRID_SIZE * TILE_Y_SIZE + city_height_pixels) / 2;
        int min_height = max_height - city_height_pixels - TILE_Y_SIZE;
        struct map_tile_t dummy_tile = { 0, 0, 0 };
        int error = 0;
        int current_height = image_set_loop_height_limits(min_height, max_height);
        int size;
        const color_t *canvas = (color_t *) graphics_canvas() + TOP_MENU_HEIGHT * canvas_width;
        while ((size = image_request_rows())) {
            city_view_set_camera_from_pixel_position(base_width, current_height);
            city_without_overlay_draw(0, 0, &dummy_tile);
            if (!image_write_rows(canvas, canvas_width)) {
                log_error("Error writing image", 0, 0);
                error = 1;
                break;
            }
            current_height += size;
        }
        graphics_reset_clip_rectangle();
        screen_set_resolution(width, height);
        city_view_set_camera_from_pixel_position(original_camera_pixels.x, original_camera_pixels.y);
        if (!error) {
            png_write_end(image.png_ptr, image.info_ptr);
            log_info("Saved full city screenshot:", filename, 0);
            show_saved_notice(filename);
        }
        image_free();
    } else {
        int width = screen_width();
        int height = screen_height();

        if (!image_create(width, height, 1)) {
            log_error("Unable to create memory for screenshot", 0, 0);
            return;
        }

        const char *filename = generate_filename(DISPLAY_SCREENSHOT);
        if (!image_begin_io(filename) || !image_write_header()) {
            log_error("Unable to write screenshot to:", filename, 0);
            image_free();
            return;
        }


        const color_t *canvas = graphics_canvas();
        int current_height = image_set_loop_height_limits(0, image.height);
        int size;
        while ((size = image_request_rows())) {
            if (!image_write_rows(canvas + current_height * image.width, image.width)) {
                log_error("Error writing image", 0, 0);
                image_free();
                return;
            }
            current_height += size;
        }

        png_write_end(image.png_ptr, image.info_ptr);
        log_info("Saved screenshot:", filename, 0);
        show_saved_notice(filename);
        image_free();
    }
}

static void text_scroll(int is_down, int num_lines)
{
    struct scrollbar_type_t *scrollbar = current;
    if (is_down) {
        scrollbar->scroll_position += num_lines;
        if (scrollbar->scroll_position > scrollbar->max_scroll_position) {
            scrollbar->scroll_position = scrollbar->max_scroll_position;
        }
    } else {
        scrollbar->scroll_position -= num_lines;
        if (scrollbar->scroll_position < 0) {
            scrollbar->scroll_position = 0;
        }
    }
    scrollbar->is_dragging_scroll = 0;
    if (scrollbar->on_scroll_callback) {
        scrollbar->on_scroll_callback();
    }
}

static struct image_button_t image_button_scroll_up = {
    0, 0, SCROLL_BUTTON_WIDTH, SCROLL_BUTTON_HEIGHT, IB_SCROLL,
    GROUP_OK_CANCEL_SCROLL_BUTTONS, 8, text_scroll, button_none, 0, 1, 1, 0, 0, 0
};
static struct image_button_t image_button_scroll_down = {
    0, 0, SCROLL_BUTTON_WIDTH, SCROLL_BUTTON_HEIGHT, IB_SCROLL,
    GROUP_OK_CANCEL_SCROLL_BUTTONS, 12, text_scroll, button_none, 1, 1, 1, 0, 0, 0
};

void scrollbar_init(struct scrollbar_type_t *scrollbar, int scroll_position, int max_scroll_position)
{
    if (max_scroll_position < 0) {
        max_scroll_position = 0;
    }
    scrollbar->scroll_position = calc_bound(scroll_position, 0, max_scroll_position);
    scrollbar->max_scroll_position = max_scroll_position;
    scrollbar->is_dragging_scroll = 0;
}

void scrollbar_reset(struct scrollbar_type_t *scrollbar, int scroll_position)
{
    scrollbar->scroll_position = scroll_position;
    scrollbar->is_dragging_scroll = 0;
}

void scrollbar_update_max(struct scrollbar_type_t *scrollbar, int max_scroll_position)
{
    if (max_scroll_position < 0) {
        max_scroll_position = 0;
    }
    scrollbar->max_scroll_position = max_scroll_position;
    if (scrollbar->scroll_position > max_scroll_position) {
        scrollbar->scroll_position = max_scroll_position;
    }
}

void scrollbar_draw(struct scrollbar_type_t *scrollbar)
{
    if (scrollbar->max_scroll_position > 0 || scrollbar->always_visible) {
        image_buttons_draw(scrollbar->x, scrollbar->y, &image_button_scroll_up, 1);
        image_buttons_draw(scrollbar->x, scrollbar->y + scrollbar->height - SCROLL_BUTTON_HEIGHT,
            &image_button_scroll_down, 1);

        int pct;
        if (scrollbar->scroll_position <= 0) {
            pct = 0;
        } else if (scrollbar->scroll_position >= scrollbar->max_scroll_position) {
            pct = 100;
        } else {
            pct = calc_percentage(scrollbar->scroll_position, scrollbar->max_scroll_position);
        }
        int offset = calc_adjust_with_percentage(
            scrollbar->height - TOTAL_BUTTON_HEIGHT - 2 * scrollbar->dot_padding, pct);
        if (scrollbar->is_dragging_scroll) {
            offset = scrollbar->scroll_position_drag;
        }
        image_draw(image_group(GROUP_PANEL_BUTTON) + 39,
            scrollbar->x + (SCROLL_BUTTON_WIDTH - SCROLL_DOT_SIZE) / 2,
            scrollbar->y + offset + SCROLL_BUTTON_HEIGHT + scrollbar->dot_padding);
    }
}

int scrollbar_handle_mouse(struct scrollbar_type_t *scrollbar, const struct mouse_t *m)
{
    if (scrollbar->max_scroll_position <= 0) {
        return 0;
    }
    current = scrollbar;
    if (m->scrolled == SCROLL_DOWN) {
        text_scroll(1, 3);
    } else if (m->scrolled == SCROLL_UP) {
        text_scroll(0, 3);
    }

    if (image_buttons_handle_mouse(m,
        scrollbar->x, scrollbar->y, &image_button_scroll_up, 1, 0)) {
        return 1;
    }
    if (image_buttons_handle_mouse(m,
        scrollbar->x, scrollbar->y + scrollbar->height - SCROLL_BUTTON_HEIGHT,
        &image_button_scroll_down, 1, 0)) {
        return 1;
    }

    if (scrollbar->max_scroll_position <= 0 || !m->left.is_down) {
        return 0;
    }
    int track_height = scrollbar->height - TOTAL_BUTTON_HEIGHT - 2 * scrollbar->dot_padding;
    if (m->x < scrollbar->x || m->x >= scrollbar->x + SCROLL_BUTTON_WIDTH) {
        return 0;
    }
    if (m->y < scrollbar->y + SCROLL_BUTTON_HEIGHT + scrollbar->dot_padding ||
        m->y > scrollbar->y + scrollbar->height - SCROLL_BUTTON_HEIGHT - scrollbar->dot_padding) {
        return 0;
    }
    int dot_offset = m->y - scrollbar->y - SCROLL_DOT_SIZE / 2 - SCROLL_BUTTON_HEIGHT;
    if (dot_offset < 0) {
        dot_offset = 0;
    }
    if (dot_offset > track_height) {
        dot_offset = track_height;
    }
    int pct_scrolled = calc_percentage(dot_offset, track_height);
    scrollbar->scroll_position = calc_adjust_with_percentage(
        scrollbar->max_scroll_position, pct_scrolled);
    scrollbar->is_dragging_scroll = 1;
    scrollbar->scroll_position_drag = dot_offset;
    if (scrollbar->scroll_position_drag < 0) {
        scrollbar->scroll_position_drag = 0;
    }
    if (scrollbar->on_scroll_callback) {
        scrollbar->on_scroll_callback();
    }
    return 1;
}

void text_capture_cursor(int cursor_position, int offset_start, int offset_end)
{
    input_cursor.capture = 1;
    input_cursor.seen = 0;
    input_cursor.position = 0;
    input_cursor.width = 0;
    input_cursor.cursor_position = cursor_position;
    input_cursor.text_offset_start = offset_start;
    input_cursor.text_offset_end = offset_end;
}

void text_draw_cursor(int x_offset, int y_offset)
{
    if (!input_cursor.capture) {
        return;
    }
    input_cursor.capture = 0;
    graphics_fill_rect(x_offset + input_cursor.x_offset, y_offset + input_cursor.y_offset + 14, input_cursor.width, 2, COLOR_WHITE);
}

int text_get_width(const char *str, int font)
{
    const struct font_definition_t *def = font_definition_for(font);
    int maxlen = 10000;
    int width = 0;
    while (*str && maxlen > 0) {
        int num_bytes = 1;
        if (*str == ' ') {
            width += def->space_width;
        } else {
            int letter_id = font_letter_id(def, str);
            if (letter_id >= 0) {
                width += def->letter_spacing + image_letter(letter_id)->width;
            }
        }
        str += num_bytes;
        maxlen -= num_bytes;
    }
    return width;
}

static int get_letter_width(const char *str, const struct font_definition_t *def, int *num_bytes)
{
    *num_bytes = 1;
    if (*str == ' ') {
        return def->space_width;
    }
    int letter_id = font_letter_id(def, str);
    if (letter_id >= 0) {
        return def->letter_spacing + image_letter(letter_id)->width;
    } else {
        return 0;
    }
}

unsigned int text_get_max_length_for_width(const char *str, int length, int font, unsigned int requested_width, int invert)
{
    const struct font_definition_t *def = font_definition_for(font);
    if (!length) {
        length = string_length(str);
    }
    if (invert) {
        unsigned int maxlen = length;
        unsigned int width = 0;
        const char *s = str;
        while (maxlen) {
            int num_bytes;
            width += get_letter_width(s, def, &num_bytes);
            s += num_bytes;
            maxlen -= num_bytes;
        }

        maxlen = length;
        while (maxlen && width > requested_width) {
            int num_bytes;
            width -= get_letter_width(str, def, &num_bytes);
            str += num_bytes;
            maxlen -= num_bytes;
        }
        return maxlen;
    } else {
        unsigned int maxlen = length;
        unsigned int width = 0;
        while (maxlen) {
            int num_bytes;
            width += get_letter_width(str, def, &num_bytes);
            if (width > requested_width) {
                break;
            }
            str += num_bytes;
            maxlen -= num_bytes;
        }
        return length - maxlen;
    }
}

void text_ellipsize(char *str, int font, int requested_width)
{
    char *orig_str = str;
    const struct font_definition_t *def = font_definition_for(font);
    int ellipsis_width = 0;
    if (!ellipsis.width[font]) {
        ellipsis.width[font] = text_get_width(ellipsis.string, font);
    }
    ellipsis_width = ellipsis.width[font];
    int maxlen = 10000;
    int width = 0;
    int length_with_ellipsis = 0;
    while (*str && maxlen > 0) {
        if (*str == ' ') {
            width += def->space_width;
        } else {
            int letter_id = font_letter_id(def, str);
            if (letter_id >= 0) {
                width += def->letter_spacing + image_letter(letter_id)->width;
            }
        }
        if (ellipsis_width + width <= requested_width) {
            length_with_ellipsis += 1;
        }
        if (width > requested_width) {
            break;
        }
        str += 1;
        maxlen -= 1;
    }
    if (10000 - maxlen < string_length(orig_str)) {
        string_copy(ellipsis.string, orig_str + length_with_ellipsis, ELLIPSIS_LENGTH);
    }
}

void text_draw_centered(const char *str, int x, int y, int box_width, int font, color_t color)
{
    int offset = (box_width - text_get_width(str, font)) / 2;
    if (offset < 0) {
        offset = 0;
    }
    text_draw(str, offset + x, y, font, color);
}

void text_draw_ellipsized(const char *str, int x, int y, int box_width, int font, color_t color)
{
    static char buffer[1000];
    string_copy(str, buffer, 1000);
    text_ellipsize(buffer, font, box_width);
    text_draw(buffer, x, y, font, color);
}

int text_draw(const char *str, int x, int y, int font, color_t color)
{
    const struct font_definition_t *def = font_definition_for(font);

    int length = string_length(str);
    if (input_cursor.capture) {
        str += input_cursor.text_offset_start;
        length = input_cursor.text_offset_end - input_cursor.text_offset_start;
    }

    int current_x = x;
    while (length > 0) {
        if (*str >= ' ') {
            int letter_id = font_letter_id(def, str);
            int width;
            if (*str == ' ' || *str == '_' || letter_id < 0) {
                width = def->space_width;
            } else {
                const struct image_t *img = image_letter(letter_id);
                int height = def->image_y_offset(*str, img->height, def->line_height);
                image_draw_letter(letter_id, current_x, y - height, color);
                width = def->letter_spacing + img->width;
            }
            if (input_cursor.capture && input_cursor.position == input_cursor.cursor_position) {
                if (!input_cursor.seen) {
                    input_cursor.width = width;
                    input_cursor.x_offset = current_x - x;
                    input_cursor.seen = 1;
                }
            }
            current_x += width;
        }

        str += 1;
        length -= 1;
        input_cursor.position += 1;
    }
    if (input_cursor.capture && !input_cursor.seen) {
        input_cursor.width = 4;
        input_cursor.x_offset = current_x - x;
        input_cursor.seen = 1;
    }
    current_x += def->space_width;
    return current_x - x;
}

static int number_to_string(char *str, int value, char prefix, const char *postfix)
{
    int offset = 0;
    if (prefix) {
        str[offset++] = prefix;
    }
    offset += string_from_int(&str[offset], value, 0);
    if (postfix) {
        while (*postfix) {
            str[offset++] = *postfix;
            postfix++;
        }
    }
    str[offset] = 0;
    return offset;
}

int text_draw_number(int value, char prefix, const char *postfix, int x_offset, int y_offset, int font)
{
    char str[NUMBER_BUFFER_LENGTH];
    number_to_string(str, value, prefix, postfix);
    return text_draw(str, x_offset, y_offset, font, 0);
}

int text_draw_number_colored(
    int value, char prefix, const char *postfix, int x_offset, int y_offset, int font, color_t color)
{
    char str[NUMBER_BUFFER_LENGTH];
    number_to_string(str, value, prefix, postfix);
    return text_draw(str, x_offset, y_offset, font, color);
}

int text_draw_money(int value, int x_offset, int y_offset, int font)
{
    char str[NUMBER_BUFFER_LENGTH];
    int money_len = number_to_string(str, value, '@', " ");
    const char *postfix;
    postfix = lang_get_string(6, 0);
    string_copy(postfix, str + money_len, NUMBER_BUFFER_LENGTH - money_len - 1);
    return text_draw(str, x_offset, y_offset, font, 0);
}

int text_draw_percentage(int value, int x_offset, int y_offset, int font)
{
    char str[NUMBER_BUFFER_LENGTH];
    number_to_string(str, value, '@', "%");
    return text_draw(str, x_offset, y_offset, font, 0);
}

void text_draw_number_centered(int value, int x_offset, int y_offset, int box_width, int font)
{
    char str[NUMBER_BUFFER_LENGTH];
    number_to_string(str, value, '@', " ");
    text_draw_centered(str, x_offset, y_offset, box_width, font, 0);
}

void text_draw_number_centered_prefix(int value, char prefix, int x_offset, int y_offset, int box_width, int font)
{
    char str[NUMBER_BUFFER_LENGTH];
    number_to_string(str, value, prefix, " ");
    text_draw_centered(str, x_offset, y_offset, box_width, font, 0);
}

void text_draw_number_centered_colored(
    int value, int x_offset, int y_offset, int box_width, int font, color_t color)
{
    char str[NUMBER_BUFFER_LENGTH];
    number_to_string(str, value, '@', " ");
    text_draw_centered(str, x_offset, y_offset, box_width, font, color);
}

static int get_word_width(const char *str, int font, int *out_num_chars)
{
    const struct font_definition_t *def = font_definition_for(font);
    int width = 0;
    int guard = 0;
    int word_char_seen = 0;
    int num_chars = 0;
    while (*str && ++guard < 200) {
        int num_bytes = 1;
        if (*str == ' ' || *str == '\n') {
            if (word_char_seen) {
                break;
            }
            width += def->space_width;
        } else if (*str == '$') {
            if (word_char_seen) {
                break;
            }
        } else if (*str > ' ') {
            // normal char
            int letter_id = font_letter_id(def, str);
            if (letter_id >= 0) {
                width += image_letter(letter_id)->width + def->letter_spacing;
            }
            word_char_seen = 1;
            if (num_bytes > 1) {
                num_chars += num_bytes;
                break;
            }
        }
        str += num_bytes;
        num_chars += num_bytes;
    }
    *out_num_chars = num_chars;
    return width;
}

int text_draw_multiline(const char *str, int x_offset, int y_offset, int box_width, int font, uint32_t color)
{
    int line_height = font_definition_for(font)->line_height;
    if (line_height < 11) {
        line_height = 11;
    }
    int has_more_characters = 1;
    int guard = 0;
    int y = y_offset;
    while (has_more_characters) {
        if (++guard >= 100) {
            break;
        }
        // clear line
        for (int i = 0; i < 200; i++) {
            tmp_line_text[i] = 0;
        }
        int current_width = 0;
        int line_index = 0;
        while (has_more_characters && current_width < box_width) {
            int word_num_chars;
            int word_width = get_word_width(str, font, &word_num_chars);
            current_width += word_width;
            if (current_width >= box_width) {
                if (current_width == 0) {
                    has_more_characters = 0;
                }
            } else {
                for (int i = 0; i < word_num_chars; i++) {
                    if (line_index == 0 && *str <= ' ') {
                        str++; // skip whitespace at start of line
                    } else {
                        tmp_line_text[line_index++] = *str++;
                    }
                }
                if (!*str) {
                    has_more_characters = 0;
                } else if (*str == '\n') {
                    str++;
                    break;
                }
            }
        }
        text_draw(tmp_line_text, x_offset, y, font, color);
        y += line_height + 5;
    }
    return y - y_offset;
}

static void close_smk(void)
{
    if (data_video.s) {
        smacker_close(data_video.s);
        data_video.s = 0;
    }
}

int video_start(const char *filename)
{
    data_video.is_playing = 0;
    data_video.is_ended = 0;

    FILE *fp = fopen(filename, "rb");
    data_video.s = smacker_open(fp);
    if (!data_video.s) {
        // smacker_open() closes the stream on error: no need to close fp
        return 0;
    }
    int width, height, y_scale, micros_per_frame;
    smacker_get_frames_info(data_video.s, 0, &micros_per_frame);
    smacker_get_video_info(data_video.s, &width, &height, &y_scale);
    data_video.video.width = width;
    data_video.video.height = y_scale == SMACKER_Y_SCALE_NONE ? height : height * 2;
    data_video.video.y_scale = y_scale;
    data_video.video.current_frame = 0;
    data_video.video.micros_per_frame = micros_per_frame;

    data_video.audio.has_audio = 0;
    if (get_sound(SOUND_EFFECTS)->enabled) {
        int has_track, channels, bitdepth, rate;
        smacker_get_audio_info(data_video.s, 0, &has_track, &channels, &bitdepth, &rate);
        if (has_track) {
            data_video.audio.has_audio = 1;
            data_video.audio.bitdepth = bitdepth;
            data_video.audio.channels = channels;
            data_video.audio.rate = rate;
        }
    }
    if (smacker_first_frame(data_video.s) != SMACKER_FRAME_OK) {
        close_smk();
        return 0;
    }
    stop_music();
    stop_sound_channel(SOUND_CHANNEL_SPEECH);
    data_video.is_playing = 1;
    return 1;
}

void video_init(int restart_music)
{
    data_video.video.start_render_millis = time_get_millis();
    data_video.restart_music = restart_music;

    if (data_video.audio.has_audio) {
        int audio_len = smacker_get_frame_audio_size(data_video.s, 0);
        if (audio_len > 0) {
            use_custom_music_player(
                data_video.audio.bitdepth, data_video.audio.channels, data_video.audio.rate,
                smacker_get_frame_audio(data_video.s, 0), audio_len
            );
        }
    }
}

int video_is_finished(void)
{
    return data_video.is_ended;
}

static void end_video(void)
{
    use_default_music_player();
    if (data_video.restart_music) {
        update_music(1);
    }
}

void video_stop(void)
{
    if (data_video.is_playing) {
        if (!data_video.is_ended) {
            end_video();
        }
        close_smk();
        data_video.is_playing = 0;
    }
}

void video_shutdown(void)
{
    if (data_video.is_playing) {
        close_smk();
        data_video.is_playing = 0;
    }
}

static int get_next_frame(void)
{
    if (!data_video.s) {
        return 0;
    }
    uint32_t now_millis = time_get_millis();

    int frame_no = (now_millis - data_video.video.start_render_millis) * 1000 / data_video.video.micros_per_frame;
    int draw_frame = data_video.video.current_frame == 0;
    while (frame_no > data_video.video.current_frame) {
        if (smacker_next_frame(data_video.s) != SMACKER_FRAME_OK) {
            close_smk();
            data_video.is_ended = 1;
            data_video.is_playing = 0;
            end_video();
            return 0;
        }
        data_video.video.current_frame++;
        draw_frame = 1;

        if (data_video.audio.has_audio) {
            int audio_len = smacker_get_frame_audio_size(data_video.s, 0);
            if (audio_len > 0) {
                write_custom_music_data(smacker_get_frame_audio(data_video.s, 0), audio_len);
            }
        }
    }
    return draw_frame;
}

void video_draw(int x_offset, int y_offset)
{
    if (!get_next_frame()) {
        return;
    }
    const struct clip_info_t *clip = graphics_get_clip_info(x_offset, y_offset, data_video.video.width, data_video.video.height);
    if (!clip || !clip->is_visible) {
        return;
    }
    const unsigned char *frame = smacker_get_frame_video(data_video.s);
    const uint32_t *pal = smacker_get_frame_palette(data_video.s);
    if (frame && pal) {
        for (int y = clip->clipped_pixels_top; y < clip->visible_pixels_y; y++) {
            color_t *pixel = graphics_get_pixel(
                x_offset + clip->clipped_pixels_left, y + y_offset + clip->clipped_pixels_top);
            int video_y = data_video.video.y_scale == SMACKER_Y_SCALE_NONE ? y : y / 2;
            const unsigned char *line = frame + (video_y * data_video.video.width);
            for (int x = clip->clipped_pixels_left; x < clip->visible_pixels_x; x++) {
                *pixel = pal[line[x]];
                ++pixel;
            }
        }
    }
}

void video_draw_fullscreen(void)
{
    if (!get_next_frame()) {
        return;
    }
    int s_width = screen_width();
    int s_height = screen_height();
    const unsigned char *frame = smacker_get_frame_video(data_video.s);
    const uint32_t *pal = smacker_get_frame_palette(data_video.s);
    if (frame && pal) {
        double scale_w = s_width / (double) data_video.video.width;
        double scale_h = s_height / (double) data_video.video.height * (data_video.video.y_scale == SMACKER_Y_SCALE_NONE ? 1 : 2);
        double scale = scale_w < scale_h ? scale_w : scale_h;
        int video_width = (int) (scale * data_video.video.width);
        int video_height = (int) (scale * data_video.video.height);
        int x_offset = (s_width - video_width) / 2;
        int y_offset = (s_height - video_height) / 2;
        const struct clip_info_t *clip = graphics_get_clip_info(x_offset, y_offset, video_width, video_height);
        if (!clip || !clip->is_visible) {
            return;
        }
        for (int y = clip->clipped_pixels_top; y < video_height - clip->clipped_pixels_bottom; y++) {
            color_t *pixel = graphics_get_pixel(x_offset + clip->clipped_pixels_left, y_offset + y);
            int x_max = video_width - clip->clipped_pixels_right;
            int video_y = (int) ((data_video.video.y_scale == SMACKER_Y_SCALE_NONE ? y : y / 2) / scale);
            const unsigned char *line = frame + (video_y * data_video.video.width);
            for (int x = clip->clipped_pixels_left; x < x_max; x++) {
                *pixel = ALPHA_OPAQUE | pal[line[(int) (x / scale)]];
                ++pixel;
            }
        }
    }
}

static void noop(void)
{}
static void noop_input(__attribute__((unused)) const struct mouse_t *m, __attribute__((unused)) const struct hotkeys_t *h)
{}

static void reset_input(void)
{
    mouse_reset_button_state();
    scroll_stop();
}

static void increase_queue_index(void)
{
    window_data.queue_index++;
    if (window_data.queue_index >= MAX_WINDOW_QUEUE) {
        window_data.queue_index = 0;
    }
}

void window_show(const struct window_type_t *window)
{
    reset_input();
    increase_queue_index();
    window_data.window_queue[window_data.queue_index] = *window;
    window_data.current_window = &window_data.window_queue[window_data.queue_index];
    if (!window_data.current_window->draw_background) {
        window_data.current_window->draw_background = noop;
    }
    if (!window_data.current_window->draw_foreground) {
        window_data.current_window->draw_foreground = noop;
    }
    if (!window_data.current_window->handle_input) {
        window_data.current_window->handle_input = noop_input;
    }
    window_invalidate();
}

void window_invalidate(void)
{
    window_data.refresh_immediate = 1;
    window_data.refresh_on_draw = 1;
}

int window_is_invalid(void)
{
    return window_data.refresh_immediate;
}

void window_request_refresh(void)
{
    window_data.refresh_on_draw = 1;
}

int window_is(int id)
{
    return window_data.current_window->id == id;
}

int window_get_id(void)
{
    return window_data.current_window->id;
}

static void decrease_queue_index(void)
{
    window_data.queue_index--;
    if (window_data.queue_index < 0) {
        window_data.queue_index = MAX_WINDOW_QUEUE - 1;
    }
}

void window_go_back(void)
{
    reset_input();
    decrease_queue_index();
    window_data.current_window = &window_data.window_queue[window_data.queue_index];
    window_invalidate();
}

static void warning_draw(void)
{
    if (!window_is(WINDOW_CITY) && !window_is(WINDOW_EDITOR_MAP)) {
        city_warning_clear_all();
        return;
    }

    int center = (screen_width() - 180) / 2;
    for (int i = 0; i < MAX_WARNINGS; i++) {
        const char *text = city_warning_get(i);
        if (!text) {
            continue;
        }
        int top_offset = 30 + i * 25;
        if (game_state_is_paused()) {
            top_offset += 70;
        }
        int box_width = 0;
        int width = text_get_width(text, FONT_NORMAL_BLACK);
        if (width <= 100) {
            box_width = 200;
        } else if (width <= 200) {
            box_width = 300;
        } else if (width <= 300) {
            box_width = 400;
        } else {
            box_width = 460;
        }
        label_draw(center - box_width / 2 + 1, top_offset, box_width / BLOCK_SIZE + 1, 1);
        if (box_width < 460) {
            // ornaments at the side
            image_draw(image_group(GROUP_CONTEXT_ICONS) + 15, center - box_width / 2 + 2, top_offset + 2);
            image_draw(image_group(GROUP_CONTEXT_ICONS) + 15, center + box_width / 2 - 30, top_offset + 2);
        }
        text_draw_centered(text, center - box_width / 2 + 1, top_offset + 4, box_width, FONT_NORMAL_WHITE, 0);
    }
    city_warning_clear_outdated();
}

void window_draw(int force)
{
    mouse_determine_button_state();
    hotkey_handle_global_keys();
    struct window_type_t *w = window_data.current_window;
    if (force || window_data.refresh_on_draw) {
        w->draw_background();
        window_data.refresh_on_draw = 0;
        window_data.refresh_immediate = 0;
    }
    w->draw_foreground();

    const struct mouse_t *m = mouse_get();
    const struct hotkeys_t *h = hotkey_state();
    w->handle_input(m, h);
    warning_draw();
    mouse_reset_scroll();
    input_cursor_update(window_data.current_window->id);
    hotkey_reset_state();
}

void window_draw_underlying_window(void)
{
    if (window_data.underlying_windows_redrawing < MAX_WINDOW_QUEUE) {
        ++window_data.underlying_windows_redrawing;
        decrease_queue_index();
        struct window_type_t *window_behind = &window_data.window_queue[window_data.queue_index];
        if (window_behind->draw_background) {
            window_behind->draw_background();
        }
        if (window_behind->draw_foreground) {
            window_behind->draw_foreground();
        }
        increase_queue_index();
        --window_data.underlying_windows_redrawing;
    }
}