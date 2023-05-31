#include "core/buffer.h"

#include <string.h>

void buffer_init(struct buffer_t *buf, void *data, int size)
{
    buf->data = data;
    buf->size = size;
    buf->index = 0;
    buf->overflow = 0;
}

void buffer_reset(struct buffer_t *buf)
{
    buf->index = 0;
    buf->overflow = 0;
}

void buffer_set(struct buffer_t *buf, int offset)
{
    buf->index = offset;
}

static int check_size(struct buffer_t *buf, int size)
{
    if (buf->index + size > buf->size) {
        buf->overflow = 1;
        return 0;
    }
    return 1;
}

void buffer_write_u8(struct buffer_t *buf, uint8_t value)
{
    if (check_size(buf, 1)) {
        buf->data[buf->index++] = value;
    }
}

void buffer_write_u16(struct buffer_t *buf, uint16_t value)
{
    if (check_size(buf, 2)) {
        buf->data[buf->index++] = value & 0xff;
        buf->data[buf->index++] = (value >> 8) & 0xff;
    }
}

void buffer_write_u32(struct buffer_t *buf, uint32_t value)
{
    if (check_size(buf, 4)) {
        buf->data[buf->index++] = value & 0xff;
        buf->data[buf->index++] = (value >> 8) & 0xff;
        buf->data[buf->index++] = (value >> 16) & 0xff;
        buf->data[buf->index++] = (value >> 24) & 0xff;
    }
}

void buffer_write_i8(struct buffer_t *buf, int8_t value)
{
    if (check_size(buf, 1)) {
        buf->data[buf->index++] = value & 0xff;
    }
}

void buffer_write_i16(struct buffer_t *buf, int16_t value)
{
    if (check_size(buf, 2)) {
        buf->data[buf->index++] = value & 0xff;
        buf->data[buf->index++] = (value >> 8) & 0xff;
    }
}

void buffer_write_i32(struct buffer_t *buf, int32_t value)
{
    if (check_size(buf, 4)) {
        buf->data[buf->index++] = value & 0xff;
        buf->data[buf->index++] = (value >> 8) & 0xff;
        buf->data[buf->index++] = (value >> 16) & 0xff;
        buf->data[buf->index++] = (value >> 24) & 0xff;
    }
}

void buffer_write_raw(struct buffer_t *buf, const void *value, int size)
{
    if (check_size(buf, size)) {
        memcpy(&buf->data[buf->index], value, size);
        buf->index += size;
    }
}

uint8_t buffer_read_u8(struct buffer_t *buf)
{
    if (check_size(buf, 1)) {
        return buf->data[buf->index++];
    } else {
        return 0;
    }
}

uint16_t buffer_read_u16(struct buffer_t *buf)
{
    if (check_size(buf, 2)) {
        uint8_t b0 = buf->data[buf->index++];
        uint8_t b1 = buf->data[buf->index++];
        return (uint16_t) (b0 | (b1 << 8));
    } else {
        return 0;
    }
}

uint32_t buffer_read_u32(struct buffer_t *buf)
{
    if (check_size(buf, 4)) {
        uint8_t b0 = buf->data[buf->index++];
        uint8_t b1 = buf->data[buf->index++];
        uint8_t b2 = buf->data[buf->index++];
        uint8_t b3 = buf->data[buf->index++];
        return (uint32_t) (b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
    } else {
        return 0;
    }
}

int8_t buffer_read_i8(struct buffer_t *buf)
{
    if (check_size(buf, 1)) {
        return (int8_t) buf->data[buf->index++];
    } else {
        return 0;
    }
}

int16_t buffer_read_i16(struct buffer_t *buf)
{
    if (check_size(buf, 2)) {
        uint8_t b0 = buf->data[buf->index++];
        uint8_t b1 = buf->data[buf->index++];
        return (int16_t) (b0 | (b1 << 8));
    } else {
        return 0;
    }
}

int32_t buffer_read_i32(struct buffer_t *buf)
{
    if (check_size(buf, 4)) {
        uint8_t b0 = buf->data[buf->index++];
        uint8_t b1 = buf->data[buf->index++];
        uint8_t b2 = buf->data[buf->index++];
        uint8_t b3 = buf->data[buf->index++];
        return (int32_t) (b0 | (b1 << 8) | (b2 << 16) | (b3 << 24));
    } else {
        return 0;
    }
}

int buffer_read_raw(struct buffer_t *buf, void *value, int max_size)
{
    int size = buf->size - buf->index;
    if (size > max_size) {
        size = max_size;
    }
    memcpy(value, &buf->data[buf->index], size);
    buf->index += size;
    return size;
}

void buffer_skip(struct buffer_t *buf, int size)
{
    buf->index += size;
}

int buffer_at_end(struct buffer_t *buf)
{
    return buf->index >= buf->size;
}
