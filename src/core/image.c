#include "image.h"

#include "core/buffer.h"
#include "core/file.h"
#include "core/io.h"
#include "core/log.h"

#include <stdlib.h>
#include <string.h>

#define HEADER_SIZE 20680
#define ENTRY_SIZE 64

#define MAIN_ENTRIES 10000
#define ENEMY_ENTRIES 801

#define MAIN_INDEX_SIZE 660680
#define ENEMY_INDEX_OFFSET HEADER_SIZE
#define ENEMY_INDEX_SIZE ENTRY_SIZE * ENEMY_ENTRIES
#define ENEMY_FILES_COUNT 11

#define MAIN_DATA_SIZE 30000000
#define EMPIRE_DATA_SIZE (2000*1000*4)
#define ENEMY_DATA_SIZE 2400000
#define SCRATCH_DATA_SIZE 12100000

#define NAME_SIZE 32

static const char MAIN_GRAPHICS_SG2[][NAME_SIZE] = {
    "c3_north.sg2",
    "c3.sg2",
    "c3_south.sg2"
};
static const char MAIN_GRAPHICS_555[][NAME_SIZE] = {
    "c3_north.555",
    "c3.555",
    "c3_south.555"
};
static const char EDITOR_GRAPHICS_SG2[][NAME_SIZE] = {
    "c3map_north.sg2",
    "c3map.sg2",
    "c3map_south.sg2"
};
static const char EDITOR_GRAPHICS_555[][NAME_SIZE] = {
    "c3map_north.555",
    "c3map.555",
    "c3map_south.555"
};
static const char EMPIRE_555[NAME_SIZE] = "The_empire.555";

static const char ENEMY_GRAPHICS_SG2[ENEMY_FILES_COUNT][NAME_SIZE] = {
    "carthage.sg2", // used for barbarians
    "carthage.sg2",
    "celts.sg2",
    "egyptians.sg2",
    "Etruscan.sg2",
    "Gaul.sg2",
    "goths.sg2",
    "Greek.sg2",
    "North African.sg2",
    "Persians.sg2",
    "Phoenician.sg2",
};
static const char ENEMY_GRAPHICS_555[ENEMY_FILES_COUNT][NAME_SIZE] = {
   "carthage.555", // used for barbarians
   "carthage.555",
   "celts.555",
   "egyptians.555",
   "Etruscan.555",
   "Gaul.555",
   "goths.555",
   "Greek.555",
   "North African.555",
   "Persians.555",
   "Phoenician.555",
};

static struct {
    int current_climate;
    int is_editor;
    int fonts_enabled;
    int font_base_offset;

    uint16_t group_image_ids[300];
    char bitmaps[100][200];
    struct image_t main[MAIN_ENTRIES];
    struct image_t enemy[ENEMY_FILES_COUNT][ENEMY_ENTRIES];
    struct image_t *font;
    color_t *main_data;
    color_t *empire_data;
    color_t *enemy_data[ENEMY_FILES_COUNT];
    color_t *font_data;
    uint8_t *tmp_data;
} data = { .current_climate = -1 };

int image_init(void)
{
    for (int i = 0; i < ENEMY_FILES_COUNT; i++) {
        data.enemy_data[i] = (color_t *) malloc(ENEMY_DATA_SIZE);
    }

    data.main_data = (color_t *) malloc(MAIN_DATA_SIZE);
    data.empire_data = (color_t *) malloc(EMPIRE_DATA_SIZE);
    data.tmp_data = (uint8_t *) malloc(SCRATCH_DATA_SIZE);
    if (!data.main_data || !data.empire_data || !data.enemy_data[0] || !data.tmp_data) {
        free(data.main_data);
        free(data.empire_data);
        free(data.enemy_data[0]);
        free(data.tmp_data);
        return 0;
    }
    return 1;
}

static void prepare_index(struct image_t *images, int size)
{
    int offset = 4;
    for (int i = 1; i < size; i++) {
        struct image_t *img = &images[i];
        if (img->draw.is_external) {
            if (!img->draw.offset) {
                img->draw.offset = 1;
            }
        } else {
            img->draw.offset = offset;
            offset += img->draw.data_length;
        }
    }
}

static void read_index_entry(struct buffer_t *buf, struct image_t *img)
{
    img->draw.offset = buffer_read_i32(buf);
    img->draw.data_length = buffer_read_i32(buf);
    img->draw.uncompressed_length = buffer_read_i32(buf);
    buffer_skip(buf, 8);
    img->width = buffer_read_u16(buf);
    img->height = buffer_read_u16(buf);
    buffer_skip(buf, 6);
    img->num_animation_sprites = buffer_read_u16(buf);
    buffer_skip(buf, 2);
    img->sprite_offset_x = buffer_read_i16(buf);
    img->sprite_offset_y = buffer_read_i16(buf);
    buffer_skip(buf, 10);
    img->animation_can_reverse = buffer_read_i8(buf);
    buffer_skip(buf, 1);
    img->draw.type = buffer_read_u8(buf);
    img->draw.is_fully_compressed = buffer_read_i8(buf);
    img->draw.is_external = buffer_read_i8(buf);
    img->draw.has_compressed_part = buffer_read_i8(buf);
    buffer_skip(buf, 2);
    img->draw.bitmap_id = buffer_read_u8(buf);
    buffer_skip(buf, 1);
    img->animation_speed_id = buffer_read_u8(buf);
    buffer_skip(buf, 5);
}

static void read_index(struct buffer_t *buf, struct image_t *images, int size)
{
    for (int i = 0; i < size; i++) {
        read_index_entry(buf, &images[i]);
    }
    prepare_index(images, size);
}

static void read_header(struct buffer_t *buf)
{
    buffer_skip(buf, 80); // header integers
    for (int i = 0; i < 300; i++) {
        data.group_image_ids[i] = buffer_read_u16(buf);
    }
    buffer_read_raw(buf, data.bitmaps, 20000);
}

static color_t to_32_bit(uint16_t c)
{
    return ((c & 0x7c00) << 9) | ((c & 0x7000) << 4) |
        ((c & 0x3e0) << 6) | ((c & 0x380) << 1) |
        ((c & 0x1f) << 3) | ((c & 0x1c) >> 2);
}

static int convert_uncompressed(struct buffer_t *buf, int buf_length, color_t *dst)
{
    for (int i = 0; i < buf_length; i += 2) {
        *dst = to_32_bit(buffer_read_u16(buf));
        dst++;
    }
    return buf_length / 2;
}

static int convert_compressed(struct buffer_t *buf, int buf_length, color_t *dst)
{
    int dst_length = 0;
    while (buf_length > 0) {
        int control = buffer_read_u8(buf);
        if (control == 255) {
            // next byte = transparent pixels to skip
            *dst++ = 255;
            *dst++ = buffer_read_u8(buf);
            dst_length += 2;
            buf_length -= 2;
        } else {
            // control = number of concrete pixels
            *dst++ = control;
            for (int i = 0; i < control; i++) {
                *dst++ = to_32_bit(buffer_read_u16(buf));
            }
            dst_length += control + 1;
            buf_length -= control * 2 + 1;
        }
    }
    return dst_length;
}

static void convert_images(struct image_t *images, int size, struct buffer_t *buf, color_t *dst)
{
    color_t *start_dst = dst;
    dst++; // make sure img->offset > 0
    for (int i = 0; i < size; i++) {
        struct image_t *img = &images[i];
        if (img->draw.is_external) {
            continue;
        }
        buffer_set(buf, img->draw.offset);
        int img_offset = (int) (dst - start_dst);
        if (img->draw.is_fully_compressed) {
            dst += convert_compressed(buf, img->draw.data_length, dst);
        } else if (img->draw.has_compressed_part) { // isometric tile
            dst += convert_uncompressed(buf, img->draw.uncompressed_length, dst);
            dst += convert_compressed(buf, img->draw.data_length - img->draw.uncompressed_length, dst);
        } else {
            dst += convert_uncompressed(buf, img->draw.data_length, dst);
        }
        img->draw.offset = img_offset;
        img->draw.uncompressed_length /= 2;
    }
}

static void load_empire(void)
{
    int size = io_read_file_into_buffer(EMPIRE_555, data.tmp_data, EMPIRE_DATA_SIZE);
    if (size != EMPIRE_DATA_SIZE / 2) {
        log_error("unable to load empire data", EMPIRE_555, 0);
        return;
    }
    struct buffer_t buf;
    buffer_init(&buf, data.tmp_data, size);
    convert_uncompressed(&buf, size, data.empire_data);
}

int image_load_climate(int climate_id, int is_editor, int force_reload)
{
    if (climate_id == data.current_climate && is_editor == data.is_editor && !force_reload) {
        return 1;
    }

    const char *filename_bmp = is_editor ? EDITOR_GRAPHICS_555[climate_id] : MAIN_GRAPHICS_555[climate_id];
    const char *filename_idx = is_editor ? EDITOR_GRAPHICS_SG2[climate_id] : MAIN_GRAPHICS_SG2[climate_id];

    if (MAIN_INDEX_SIZE != io_read_file_into_buffer(filename_idx, data.tmp_data, MAIN_INDEX_SIZE)) {
        return 0;
    }

    struct buffer_t buf;
    buffer_init(&buf, data.tmp_data, HEADER_SIZE);
    read_header(&buf);
    buffer_init(&buf, &data.tmp_data[HEADER_SIZE], ENTRY_SIZE * MAIN_ENTRIES);
    read_index(&buf, data.main, MAIN_ENTRIES);

    int data_size = io_read_file_into_buffer(filename_bmp, data.tmp_data, SCRATCH_DATA_SIZE);
    if (!data_size) {
        return 0;
    }
    buffer_init(&buf, data.tmp_data, data_size);
    convert_images(data.main, MAIN_ENTRIES, &buf, data.main_data);
    data.current_climate = climate_id;
    data.is_editor = is_editor;

    load_empire();
    return 1;
}

int image_load_enemy(void)
{
    struct buffer_t buf;
    for (int i = 0; i < ENEMY_FILES_COUNT; i++) {
        const char *filename_idx = ENEMY_GRAPHICS_SG2[i];

        if (ENEMY_INDEX_SIZE != io_read_file_part_into_buffer(
            filename_idx, data.tmp_data, ENEMY_INDEX_SIZE, ENEMY_INDEX_OFFSET)) {
            return 0;
        }
        buffer_init(&buf, data.tmp_data, ENEMY_INDEX_SIZE);
        read_index(&buf, data.enemy[i], ENEMY_ENTRIES);

        const char *filename_bmp = ENEMY_GRAPHICS_555[i];
        int data_size = io_read_file_into_buffer(filename_bmp, data.tmp_data, SCRATCH_DATA_SIZE);
        if (!data_size) {
            return 0;
        }
        buffer_init(&buf, data.tmp_data, data_size);
        convert_images(data.enemy[i], ENEMY_ENTRIES, &buf, data.enemy_data[i]);
    }

    return 1;
}

static const color_t *load_external_data(int image_id)
{
    struct image_t *img = &data.main[image_id];
    char filename[FILE_NAME_MAX] = "555/";
    strncpy(&filename[4], data.bitmaps[img->draw.bitmap_id], FILE_NAME_MAX - 6);
    file_change_extension(filename, "555");
    int size = io_read_file_part_into_buffer(
        &filename[4], data.tmp_data,
        img->draw.data_length, img->draw.offset - 1
    );
    if (!size) {
        // try in 555 dir
        size = io_read_file_part_into_buffer(
            filename, data.tmp_data,
            img->draw.data_length, img->draw.offset - 1
        );
        if (!size) {
            log_error("unable to load external image",
                      data.bitmaps[img->draw.bitmap_id], image_id);
            return NULL;
        }
    }
    struct buffer_t buf;
    buffer_init(&buf, data.tmp_data, size);
    color_t *dst = (color_t *) &data.tmp_data[4000000];
    // NB: isometric images are never external
    if (img->draw.is_fully_compressed) {
        convert_compressed(&buf, img->draw.data_length, dst);
    } else {
        convert_uncompressed(&buf, img->draw.data_length, dst);
    }
    return dst;
}

int image_group(int group)
{
    return data.group_image_ids[group];
}

struct image_t *image_get(int id)
{
    if (id >= 0 && id < MAIN_ENTRIES) {
        return &data.main[id];
    } else {
        return NULL;
    }
}

const struct image_t *image_letter(int letter_id)
{
    return &data.main[data.group_image_ids[GROUP_FONT] + letter_id];

}

struct image_t *image_get_enemy(struct figure_t *f)
{
    if (f->image_id < ENEMY_ENTRIES) {
        return &data.enemy[f->enemy_image_group][f->image_id];
    } else {
        return NULL;
    }
}

const color_t *image_data(int id)
{
    if (id < 0 || id >= MAIN_ENTRIES) {
        return NULL;
    }
    if (!data.main[id].draw.is_external) {
        return &data.main_data[data.main[id].draw.offset];
    } else if (id == image_group(GROUP_EMPIRE_MAP)) {
        return data.empire_data;
    } else {
        return load_external_data(id);
    }
}

const color_t *image_data_letter(int letter_id)
{
    int image_id = data.group_image_ids[GROUP_FONT] + letter_id;
    return &data.main_data[data.main[image_id].draw.offset];
}

const color_t *image_data_enemy(struct figure_t *f)
{
    int offset = data.enemy[f->enemy_image_group][f->image_id].draw.offset;
    if (offset > 0) {
        return &data.enemy_data[f->enemy_image_group][offset];
    }
    return NULL;
}
