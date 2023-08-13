#include "core/lang.h"

#include "core/buffer.h"
#include "core/file.h"
#include "core/string.h"
#include "platform/brutus.h"
#include "scenario/scenario.h"

#include <stdlib.h>
#include <string.h>

#define MAX_TEXT_ENTRIES 1000
#define MAX_TEXT_DATA 200000
#define MIN_TEXT_SIZE (28 + MAX_TEXT_ENTRIES * 8)
#define MAX_TEXT_SIZE (MIN_TEXT_SIZE + MAX_TEXT_DATA)

#define MAX_MESSAGE_ENTRIES 400
#define MAX_MESSAGE_DATA 460000
#define MIN_MESSAGE_SIZE 32024
#define MAX_MESSAGE_SIZE (MIN_MESSAGE_SIZE + MAX_MESSAGE_DATA)

#define BUFFER_SIZE 400000

#define FILE_TEXT_ENG "c3.eng"
#define FILE_MM_ENG "c3_mm.eng"
#define FILE_EDITOR_TEXT_ENG "c3_map.eng"
#define FILE_EDITOR_MM_ENG "c3_map_mm.eng"

static struct {
    struct {
        int32_t offset;
        int32_t in_use;
    } text_entries[MAX_TEXT_ENTRIES];
    char text_data[MAX_TEXT_DATA];

    struct lang_message_t message_entries[MAX_MESSAGE_ENTRIES];
    char message_data[MAX_MESSAGE_DATA];
} data;

static void parse_text(struct buffer_t *buf)
{
    buffer_skip(buf, 28); // header
    for (int i = 0; i < MAX_TEXT_ENTRIES; i++) {
        data.text_entries[i].offset = buffer_read_i32(buf);
        data.text_entries[i].in_use = buffer_read_i32(buf);
    }
    buffer_read_raw(buf, data.text_data, MAX_TEXT_DATA);
}

static int load_text(const char *filename, uint8_t *buf_data)
{
    struct buffer_t buf;
    int filesize = io_read_file_into_buffer(filename, buf_data, BUFFER_SIZE);
    if (filesize < MIN_TEXT_SIZE || filesize > MAX_TEXT_SIZE) {
        return 0;
    }
    buffer_init(&buf, buf_data, filesize);
    parse_text(&buf);
    return 1;
}

static char *get_message_text(int32_t offset)
{
    if (!offset) {
        return 0;
    }
    return &data.message_data[offset];
}

static void parse_message(struct buffer_t *buf)
{
    buffer_skip(buf, 24); // header
    for (int i = 0; i < MAX_MESSAGE_ENTRIES; i++) {
        struct lang_message_t *m = &data.message_entries[i];
        m->type = buffer_read_i16(buf);
        m->message_type = buffer_read_i16(buf);
        buffer_skip(buf, 2);
        m->x = buffer_read_i16(buf);
        m->y = buffer_read_i16(buf);
        m->width_blocks = buffer_read_i16(buf);
        m->height_blocks = buffer_read_i16(buf);
        m->image.id = buffer_read_i16(buf);
        m->image.x = buffer_read_i16(buf);
        m->image.y = buffer_read_i16(buf);
        buffer_skip(buf, 6); // unused image2 id, x, y
        m->title.x = buffer_read_i16(buf);
        m->title.y = buffer_read_i16(buf);
        m->subtitle.x = buffer_read_i16(buf);
        m->subtitle.y = buffer_read_i16(buf);
        buffer_skip(buf, 4);
        m->video.x = buffer_read_i16(buf);
        m->video.y = buffer_read_i16(buf);
        buffer_skip(buf, 14);
        m->urgent = buffer_read_i32(buf);

        m->video.text = get_message_text(buffer_read_i32(buf));
        buffer_skip(buf, 4);
        m->title.text = get_message_text(buffer_read_i32(buf));
        m->subtitle.text = get_message_text(buffer_read_i32(buf));
        m->content.text = get_message_text(buffer_read_i32(buf));
    }
    buffer_read_raw(buf, &data.message_data, MAX_MESSAGE_DATA);
}

static void set_message_parameters(struct lang_message_t *m, char *title, char *text, int urgent, int message_type)
{
    m->type = TYPE_MESSAGE;
    m->message_type = message_type;
    m->x = 0;
    m->y = 0;
    m->width_blocks = 30;
    m->height_blocks = 20;
    m->title.x = 0;
    m->title.y = 0;
    m->urgent = urgent;

    m->title.text = title;
    m->content.text = text;
}

void load_custom_messages(void)
{
    int i = 321;
    while (i < MAX_MESSAGE_ENTRIES) {
        if (!data.message_entries[i].content.text) {
            break;
        }
        i++;
    }

    if (i >= MAX_MESSAGE_ENTRIES) {
        log_error("Message entry max exceeded", "", 0);
        return;
    }

    // distant battle won but triumphal arch disabled from the editor
    set_message_parameters(&data.message_entries[i], "Roman city saved", "Your relief force defeated the invading barbarians. Caesar is pleased.", 0, MESSAGE_TYPE_GENERAL);
    data.message_entries[i].video.text = "smk/army_win.smk";
    i += 1;

    // editor custom messages
    for (int j = 0; j < MAX_EDITOR_CUSTOM_MESSAGES; j++) {
        if (scenario.editor_custom_messages[j].enabled) {
            data.message_entries[i].type = TYPE_MESSAGE;
            data.message_entries[i].message_type = MESSAGE_TYPE_GENERAL;
            data.message_entries[i].x = 0;
            data.message_entries[i].y = 0;
            data.message_entries[i].width_blocks = 30;
            data.message_entries[i].height_blocks = 20;
            data.message_entries[i].title.x = 0;
            data.message_entries[i].title.y = 0;
            data.message_entries[i].urgent = scenario.editor_custom_messages[j].urgent;
            data.message_entries[i].title.text = scenario.editor_custom_messages[j].title;
            data.message_entries[i].content.text = scenario.editor_custom_messages[j].text;
            data.message_entries[i].video.text = scenario.editor_custom_messages[j].video_file;
            i += 1;
        }
    }
}

static int load_message(const char *filename, uint8_t *data_buffer)
{
    struct buffer_t buf;
    int filesize = io_read_file_into_buffer(filename, data_buffer, BUFFER_SIZE);
    if (filesize < MIN_MESSAGE_SIZE || filesize > MAX_MESSAGE_SIZE) {
        return 0;
    }
    buffer_init(&buf, data_buffer, filesize);
    parse_message(&buf);
    return 1;
}

static int load_files(const char *text_filename, const char *message_filename)
{
    uint8_t *buffer = (uint8_t *) malloc(BUFFER_SIZE);
    if (!buffer) {
        return 0;
    }
    int success = load_text(text_filename, buffer) && load_message(message_filename, buffer);
    free(buffer);
    return success;
}

int lang_load(int is_editor)
{
    if (is_editor) {
        return load_files(FILE_EDITOR_TEXT_ENG, FILE_EDITOR_MM_ENG);
    }
    return
        load_files(FILE_TEXT_ENG, FILE_MM_ENG) ||
        load_files(FILE_TEXT_ENG, FILE_MM_ENG);
}

const char *lang_get_string(int group, int index)
{
    const char *str = &data.text_data[data.text_entries[group].offset];
    char prev = 0;
    while (index > 0) {
        if (!*str && (prev >= ' ' || prev == 0)) {
            --index;
        }
        prev = *str;
        ++str;
    }
    while (*str < ' ') { // skip non-printables
        ++str;
    }
    return str;
}

const struct lang_message_t *lang_get_message(int id)
{
    return &data.message_entries[id];
}
