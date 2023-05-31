#ifndef CORE_BUFFER_H
#define CORE_BUFFER_H

#include <stdint.h>

/**
* @file
* Read to or write from memory buffer.
*/

/**
* Struct representing a buffer to read from / write to
*/
struct buffer_t {
    uint8_t *data; /**< Read-only: data */
    int size; /**< Read-only: size of the data */
    int index; /**< Read-only: bytes read/written so far */
    int overflow; /**< Read-only: indicates attempt to read/write beyond end of buffer */
};

/**
 * Initializes the buffer with the given data and size.
 * @param buffer Buffer
 * @param data Data to use
 * @param size Size of the data
 */
void buffer_init(struct buffer_t *buffer, void *data, int size);

/**
 * Resets the buffer so that reading/writing starts at the beginning again
 * @param buffer Buffer
 */
void buffer_reset(struct buffer_t *buffer);

/**
 * Sets the buffer so that reading/writing starts at the specififed offset
 * @param buffer Buffer
 * @param offset Offset to set it to
 */
void buffer_set(struct buffer_t *buffer, int offset);

/**
 * Writes an unsigned 8-bit integer
 * @param buffer Buffer
 * @param value Value to write
 */
void buffer_write_u8(struct buffer_t *buffer, uint8_t value);

/**
 * Writes an unsigned 16-bit integer
 * @param buffer Buffer
 * @param value Value to write
 */
void buffer_write_u16(struct buffer_t *buffer, uint16_t value);

/**
 * Writes an unsigned 32-bit integer
 * @param buffer Buffer
 * @param value Value to write
 */
void buffer_write_u32(struct buffer_t *buffer, uint32_t value);

/**
 * Writes a signed 8-bit integer
 * @param buffer Buffer
 * @param value Value to write
 */
void buffer_write_i8(struct buffer_t *buffer, int8_t value);

/**
 * Writes a signed 16-bit integer
 * @param buffer Buffer
 * @param value Value to write
 */
void buffer_write_i16(struct buffer_t *buffer, int16_t value);

/**
 * Writes a signed 32-bit integer
 * @param buffer Buffer
 * @param value Value to write
 */
void buffer_write_i32(struct buffer_t *buffer, int32_t value);

/**
 * Writes raw data
 * @param buffer Buffer
 * @param value Value to write
 * @param size Size in bytes
 */
void buffer_write_raw(struct buffer_t *buffer, const void *value, int size);

/**
 * Reads an unsigned 8-bit integer
 * @param buffer Buffer
 * @return Read value
 */
uint8_t buffer_read_u8(struct buffer_t *buffer);

/**
 * Reads an unsigned 16-bit integer
 * @param buffer Buffer
 * @return Read value
 */
uint16_t buffer_read_u16(struct buffer_t *buffer);

/**
 * Reads an unsigned 32-bit integer
 * @param buffer Buffer
 * @return Read value
 */
uint32_t buffer_read_u32(struct buffer_t *buffer);

/**
 * Reads a signed 8-bit integer
 * @param buffer Buffer
 * @return Read value
 */
int8_t buffer_read_i8(struct buffer_t *buffer);

/**
 * Reads a signed 16-bit integer
 * @param buffer Buffer
 * @return Read value
 */
int16_t buffer_read_i16(struct buffer_t *buffer);

/**
 * Reads a signed 32-bit integer
 * @param buffer Buffer
 * @return Read value
 */
int32_t buffer_read_i32(struct buffer_t *buffer);

/**
 * Reads raw data
 * @param buffer Buffer
 * @param value Value to read into
 * @param max_size Size of the value, max bytes to read
 * @return Bytes read
 */
int buffer_read_raw(struct buffer_t *buffer, void *value, int max_size);

/**
 * Skip data in the buffer
 * @param buffer Buffer
 * @param size Bytes to skip
 */
void buffer_skip(struct buffer_t *buffer, int size);

/**
 * Returns whether the pointer of this buffer is at the end of the buffer
 * @param buffer Buffer
 * @return True if pointer is at end of buffer, false otherwise
 */
int buffer_at_end(struct buffer_t *buffer);

#endif // CORE_BUFFER_H
