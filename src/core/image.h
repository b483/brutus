#ifndef CORE_IMAGE_H
#define CORE_IMAGE_H

#include "core/encoding.h"
#include "core/image_group.h"
#include "figure/figure.h"
#include "graphics/color.h"

#define IMAGE_FONT_MULTIBYTE_OFFSET 10000

enum {
    IMAGE_TYPE_WITH_TRANSPARENCY = 0,
    IMAGE_TYPE_ISOMETRIC = 30
};

struct image_t {
    int width;
    int height;
    int num_animation_sprites;
    int sprite_offset_x;
    int sprite_offset_y;
    int animation_can_reverse;
    int animation_speed_id;
    struct {
        int type;
        int is_fully_compressed;
        int is_external;
        int has_compressed_part;
        int bitmap_id;
        int offset;
        int data_length;
        int uncompressed_length;
    } draw;
};

int image_init(void);

int image_load_climate(int climate_id, int is_editor, int force_reload);

int image_load_enemy(void);

int image_group(int group);

struct image_t *image_get(int id);

const struct image_t *image_letter(int letter_id);

struct image_t *image_get_enemy(struct figure_t *f);

const color_t *image_data(int id);

const color_t *image_data_letter(int letter_id);

const color_t *image_data_enemy(struct figure_t *f);

#endif // CORE_IMAGE_H
