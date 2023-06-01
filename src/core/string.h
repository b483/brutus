#ifndef CORE_STRING_H
#define CORE_STRING_H

#include <stdint.h>

char *get_first_char_occurrence_in_string(const char *str, char c); // strchr substitute

/**
 * Checks if the two strings are equal
 * @param a String A
 * @param b String B
 * @return Boolean true if the strings are equal, false if they differ
 */
int string_equals(const char *a, const char *b);

/**
 * Copies a string
 * @param src Source string
 * @param dst Destination string
 * @param maxlength Maximum length of the destination string
 */
void string_copy(const char *src, char *dst, int maxlength);

/**
 * Determines the length of the string
 * @param str String
 * @return Length of the string
 */
int string_length(const char *str);

/**
 * Convert (cast) C-string to internal string.
 * Only use this for known ASCII-only strings!
 * @param str C string
 * @return Game string, or NULL if non-ascii values are found in str
 */
const char *string_from_ascii(const char *str);

/**
 * Converts the string to integer
 * @return integer
 */
int string_to_int(const char *str);

/**
 * Converts integer to string
 * @param dst Output string
 * @param value Value to write
 * @param force_plus_sign Force plus sign in front of positive value
 * @return Total number of characters written to dst
 */
int string_from_int(char *dst, int value, int force_plus_sign);

#endif // CORE_STRING_H
