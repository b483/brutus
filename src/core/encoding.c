#include "core/encoding.h"

#include "core/string.h"

#include <stdlib.h>

#define HIGH_CHAR_COUNT 128

typedef struct {
    uint8_t internal_value;
    int bytes;
    uint8_t utf8_value[3];
    int bytes_decomposed;
    uint8_t utf8_decomposed[4];
} letter_code;

typedef struct {
    uint32_t utf8;
    const letter_code *code;
} from_utf8_lookup;

static const letter_code HIGH_TO_UTF8_DEFAULT[HIGH_CHAR_COUNT] = {
    {0x80, 3, {0xe2, 0x82, 0xac}},
    {0x81, 1, {0x3f}},
    {0x82, 3, {0xe2, 0x80, 0x9a}},
    {0x83, 2, {0xc6, 0x92}},
    {0x84, 3, {0xe2, 0x80, 0x9e}},
    {0x85, 3, {0xe2, 0x80, 0xa6}},
    {0x86, 3, {0xe2, 0x80, 0xa0}},
    {0x87, 3, {0xe2, 0x80, 0xa1}},
    {0x88, 2, {0xcb, 0x86}},
    {0x89, 3, {0xe2, 0x80, 0xb0}},
    {0x8a, 2, {0xc5, 0xa0}},
    {0x8b, 3, {0xe2, 0x80, 0xb9}},
    {0x8c, 2, {0xc5, 0x92}},
    {0x8d, 1, {0x3f}},
    {0x8e, 2, {0xc5, 0xbd}},
    {0x8f, 1, {0x3f}},
    {0x90, 1, {0x3f}},
    {0x91, 3, {0xe2, 0x80, 0x98}},
    {0x92, 3, {0xe2, 0x80, 0x99}},
    {0x93, 3, {0xe2, 0x80, 0x9c}},
    {0x94, 3, {0xe2, 0x80, 0x9d}},
    {0x95, 3, {0xe2, 0x80, 0xa2}},
    {0x96, 3, {0xe2, 0x80, 0x93}},
    {0x97, 3, {0xe2, 0x80, 0x94}},
    {0x98, 2, {0xcb, 0x9c}},
    {0x99, 3, {0xe2, 0x84, 0xa2}},
    {0x9a, 2, {0xc5, 0xa1}},
    {0x9b, 3, {0xe2, 0x80, 0xba}},
    {0x9c, 2, {0xc5, 0x93}},
    {0x9d, 1, {0x3f}},
    {0x9e, 2, {0xc5, 0xbe}},
    {0x9f, 2, {0xc5, 0xb8}},
    {0xa0, 2, {0xc2, 0xa0}},
    {0xa1, 2, {0xc2, 0xa1}},
    {0xa2, 2, {0xc2, 0xa2}},
    {0xa3, 2, {0xc2, 0xa3}},
    {0xa4, 2, {0xc2, 0xa4}},
    {0xa5, 2, {0xc2, 0xa5}},
    {0xa6, 2, {0xc2, 0xa6}},
    {0xa7, 2, {0xc2, 0xa7}},
    {0xa8, 2, {0xc2, 0xa8}},
    {0xa9, 2, {0xc2, 0xa9}},
    {0xaa, 2, {0xc2, 0xaa}},
    {0xab, 2, {0xc2, 0xab}},
    {0xac, 2, {0xc2, 0xac}},
    {0xad, 2, {0xc2, 0xad}},
    {0xae, 2, {0xc2, 0xae}},
    {0xaf, 2, {0xc2, 0xaf}},
    {0xb0, 2, {0xc2, 0xb0}},
    {0xb1, 2, {0xc2, 0xb1}},
    {0xb2, 2, {0xc2, 0xb2}},
    {0xb3, 2, {0xc2, 0xb3}},
    {0xb4, 2, {0xc2, 0xb4}},
    {0xb5, 2, {0xc2, 0xb5}},
    {0xb6, 2, {0xc2, 0xb6}},
    {0xb7, 2, {0xc2, 0xb7}},
    {0xb8, 2, {0xc2, 0xb8}},
    {0xb9, 2, {0xc2, 0xb9}},
    {0xba, 2, {0xc2, 0xba}},
    {0xbb, 2, {0xc2, 0xbb}},
    {0xbc, 2, {0xc2, 0xbc}},
    {0xbd, 2, {0xc2, 0xbd}},
    {0xbe, 2, {0xc2, 0xbe}},
    {0xbf, 2, {0xc2, 0xbf}},
    {0xc0, 2, {0xc3, 0x80}, 3, {0x41, 0xcc, 0x80}},
    {0xc1, 2, {0xc3, 0x81}, 3, {0x41, 0xcc, 0x81}},
    {0xc2, 2, {0xc3, 0x82}, 3, {0x41, 0xcc, 0x82}},
    {0xc3, 2, {0xc3, 0x83}, 3, {0x41, 0xcc, 0x83}},
    {0xc4, 2, {0xc3, 0x84}, 3, {0x41, 0xcc, 0x88}},
    {0xc5, 2, {0xc3, 0x85}, 3, {0x41, 0xcc, 0x8a}},
    {0xc6, 2, {0xc3, 0x86}}, // AE
    {0xc7, 2, {0xc3, 0x87}, 3, {0x43, 0xcc, 0xa7}},
    {0xc8, 2, {0xc3, 0x88}, 3, {0x45, 0xcc, 0x80}},
    {0xc9, 2, {0xc3, 0x89}, 3, {0x45, 0xcc, 0x81}},
    {0xca, 2, {0xc3, 0x8a}, 3, {0x45, 0xcc, 0x82}},
    {0xcb, 2, {0xc3, 0x8b}, 3, {0x45, 0xcc, 0x88}},
    {0xcc, 2, {0xc3, 0x8c}, 3, {0x49, 0xcc, 0x80}},
    {0xcd, 2, {0xc3, 0x8d}, 3, {0x49, 0xcc, 0x81}},
    {0xce, 2, {0xc3, 0x8e}, 3, {0x49, 0xcc, 0x82}},
    {0xcf, 2, {0xc3, 0x8f}, 3, {0x49, 0xcc, 0x88}},
    {0xd0, 1, {0xc3, 0x90}}, // ETH
    {0xd1, 2, {0xc3, 0x91}, 3, {0x4e, 0xcc, 0x83}},
    {0xd2, 2, {0xc3, 0x92}, 3, {0x4f, 0xcc, 0x80}},
    {0xd3, 2, {0xc3, 0x93}, 3, {0x4f, 0xcc, 0x81}},
    {0xd4, 2, {0xc3, 0x94}, 3, {0x4f, 0xcc, 0x82}},
    {0xd5, 2, {0xc3, 0x95}, 3, {0x4f, 0xcc, 0x83}},
    {0xd6, 2, {0xc3, 0x96}, 3, {0x4f, 0xcc, 0x88}},
    {0xd7, 1, {0x3f}}, // multiplication
    {0xd8, 2, {0xc3, 0x98}},
    {0xd9, 2, {0xc3, 0x99}, 3, {0x55, 0xcc, 0x80}},
    {0xda, 2, {0xc3, 0x9a}, 3, {0x55, 0xcc, 0x81}},
    {0xdb, 2, {0xc3, 0x9b}, 3, {0x55, 0xcc, 0x82}},
    {0xdc, 2, {0xc3, 0x9c}, 3, {0x55, 0xcc, 0x88}},
    {0xdd, 2, {0xc3, 0x9d}, 3, {0x59, 0xcc, 0x81}},
    {0xde, 2, {0xc3, 0x90}}, // THORN
    {0xdf, 2, {0xc3, 0x9f}}, // ss
    {0xe0, 2, {0xc3, 0xa0}, 3, {0x61, 0xcc, 0x80}},
    {0xe1, 2, {0xc3, 0xa1}, 3, {0x61, 0xcc, 0x81}},
    {0xe2, 2, {0xc3, 0xa2}, 3, {0x61, 0xcc, 0x82}},
    {0xe3, 2, {0xc3, 0xa3}, 3, {0x61, 0xcc, 0x83}},
    {0xe4, 2, {0xc3, 0xa4}, 3, {0x61, 0xcc, 0x88}},
    {0xe5, 2, {0xc3, 0xa5}, 3, {0x61, 0xcc, 0x8a}},
    {0xe6, 2, {0xc3, 0xa6}}, // ae
    {0xe7, 2, {0xc3, 0xa7}, 3, {0x63, 0xcc, 0xa7}},
    {0xe8, 2, {0xc3, 0xa8}, 3, {0x65, 0xcc, 0x80}},
    {0xe9, 2, {0xc3, 0xa9}, 3, {0x65, 0xcc, 0x81}},
    {0xea, 2, {0xc3, 0xaa}, 3, {0x65, 0xcc, 0x82}},
    {0xeb, 2, {0xc3, 0xab}, 3, {0x65, 0xcc, 0x88}},
    {0xec, 2, {0xc3, 0xac}, 3, {0x69, 0xcc, 0x80}},
    {0xed, 2, {0xc3, 0xad}, 3, {0x69, 0xcc, 0x81}},
    {0xee, 2, {0xc3, 0xae}, 3, {0x69, 0xcc, 0x82}},
    {0xef, 2, {0xc3, 0xaf}, 3, {0x69, 0xcc, 0x88}},
    {0xf0, 2, {0xc3, 0xb0}}, // eth
    {0xf1, 2, {0xc3, 0xb1}, 3, {0x6e, 0xcc, 0x83}},
    {0xf2, 2, {0xc3, 0xb2}, 3, {0x6f, 0xcc, 0x80}},
    {0xf3, 2, {0xc3, 0xb3}, 3, {0x6f, 0xcc, 0x81}},
    {0xf4, 2, {0xc3, 0xb4}, 3, {0x6f, 0xcc, 0x82}},
    {0xf5, 2, {0xc3, 0xb5}, 3, {0x6f, 0xcc, 0x83}},
    {0xf6, 2, {0xc3, 0xb6}, 3, {0x6f, 0xcc, 0x88}},
    {0xf7, 2, {0xc3, 0xb7}}, // division
    {0xf8, 2, {0xc3, 0xb8}}, // o/
    {0xf9, 2, {0xc3, 0xb9}, 3, {0x75, 0xcc, 0x80}},
    {0xfa, 2, {0xc3, 0xba}, 3, {0x75, 0xcc, 0x81}},
    {0xfb, 2, {0xc3, 0xbb}, 3, {0x75, 0xcc, 0x82}},
    {0xfc, 2, {0xc3, 0xbc}, 3, {0x75, 0xcc, 0x88}},
    {0xfd, 2, {0xc3, 0xbd}, 3, {0x79, 0xcc, 0x81}},
    {0xfe, 2, {0xc3, 0xbe}}, // thorn
    {0xff, 2, {0xc3, 0xbf}, 3, {0x79, 0xcc, 0x88}},
};

static struct {
    encoding_type encoding;
    const letter_code *to_utf8_table;
    from_utf8_lookup from_utf8_table[HIGH_CHAR_COUNT];
    from_utf8_lookup from_utf8_decomposed_table[HIGH_CHAR_COUNT];
    int utf8_table_size;
    int decomposed_table_size;
} data;

static uint32_t calculate_utf8_value(const uint8_t *bytes, int length)
{
    uint32_t value = 0;
    if (length >= 1) {
        value |= bytes[0];
    }
    if (length >= 2) {
        value |= bytes[1] << 8;
    }
    if (length >= 3) {
        value |= bytes[2] << 16;
    }
    if (length >= 4) {
        value |= bytes[3] << 24;
    }
    return value;
}

static int compare_utf8_lookup(const void *a, const void *b)
{
    uint32_t va = ((const from_utf8_lookup *) a)->utf8;
    uint32_t vb = ((const from_utf8_lookup *) b)->utf8;
    return va == vb ? 0 : (va < vb ? -1 : 1);
}

static void build_reverse_lookup_table(void)
{
    if (!data.to_utf8_table) {
        data.utf8_table_size = 0;
        return;
    }
    for (int i = 0; i < HIGH_CHAR_COUNT; i++) {
        const letter_code *code = &data.to_utf8_table[i];
        data.from_utf8_table[i].code = code;
        data.from_utf8_table[i].utf8 = calculate_utf8_value(code->utf8_value, code->bytes);
    }
    data.utf8_table_size = HIGH_CHAR_COUNT;
    qsort(data.from_utf8_table, data.utf8_table_size, sizeof(from_utf8_lookup), compare_utf8_lookup);
}

static void build_decomposed_lookup_table(void)
{
    if (!data.to_utf8_table) {
        data.decomposed_table_size = 0;
        return;
    }
    int index = 0;
    for (int i = 0; i < HIGH_CHAR_COUNT; i++) {
        const letter_code *code = &data.to_utf8_table[i];
        if (code->bytes_decomposed > 0) {
            data.from_utf8_decomposed_table[index].code = code;
            data.from_utf8_decomposed_table[index].utf8 =
                calculate_utf8_value(code->utf8_decomposed, code->bytes_decomposed);
            index++;
        }
    }
    data.decomposed_table_size = index;
    qsort(data.from_utf8_decomposed_table, data.decomposed_table_size, sizeof(from_utf8_lookup), compare_utf8_lookup);
}

static const letter_code *get_letter_code_for_internal(uint8_t c)
{
    if (c < 0x80 || !data.to_utf8_table) {
        return NULL;
    }
    return &data.to_utf8_table[c - 0x80];
}

static int get_utf8_code(const char *c, int *num_bytes)
{
    const uint8_t *uc = (const uint8_t *) c;
    if (uc[0] < 0x80) {
        *num_bytes = 1;
        return uc[0];
    } else if ((uc[0] & 0xe0) == 0xc0 && (uc[1] & 0xc0) == 0x80) {
        *num_bytes = 2;
        return uc[0] | uc[1] << 8;
    } else if ((uc[0] & 0xf0) == 0xe0 && (uc[1] & 0xc0) == 0x80 && (uc[2] & 0xc0) == 0x80) {
        *num_bytes = 3;
        return uc[0] | uc[1] << 8 | uc[2] << 16;
    } else {
        *num_bytes = 1;
        return 0;
    }
}

static int is_combining_char(uint8_t b1, uint8_t b2)
{
    if (b1 == 0xcc && b2 >= 0x80) {
        return 1;
    } else if (b1 == 0xcd && b2 <= 0xaf) {
        return 1;
    }
    return 0;
}

static const letter_code *search_utf8_table(const from_utf8_lookup *key, const from_utf8_lookup *table, int size)
{
    const from_utf8_lookup *result = bsearch(key, table, size, sizeof(from_utf8_lookup), compare_utf8_lookup);
    return result ? result->code : NULL;
}

static const letter_code *get_letter_code_for_utf8(const char *c, int *num_bytes, int *is_accent)
{
    static letter_code single_char = { 0, 1 };
    from_utf8_lookup key = { 0, NULL };
    if (is_accent) *is_accent = 0;
    const uint8_t *uc = (const uint8_t *) c;

    if (uc[0] < 0x80) {
        if (num_bytes) *num_bytes = 1;
        single_char.internal_value = uc[0];
        single_char.utf8_value[0] = uc[0];
        return &single_char;
    } else if ((uc[0] & 0xe0) == 0xc0 && (uc[1] & 0xc0) == 0x80) {
        // 2-byte character
        if (num_bytes) *num_bytes = 2;
        key.utf8 = uc[0] | uc[1] << 8;
        if (is_combining_char(uc[0], uc[1])) {
            if (is_accent) *is_accent = 1;
            return NULL;
        }
    } else if ((uc[0] & 0xf0) == 0xe0 && (uc[1] & 0xc0) == 0x80 && (uc[2] & 0xc0) == 0x80) {
        // 3-byte character
        if (num_bytes) *num_bytes = 3;
        key.utf8 = uc[0] | uc[1] << 8 | uc[2] << 16;
    } else {
        if (num_bytes) *num_bytes = 1;
    }
    if (key.utf8 == 0) {
        return NULL;
    }
    return search_utf8_table(&key, data.from_utf8_table, data.utf8_table_size);
}

static const letter_code *get_letter_code_for_combining_utf8(const char *prev_char, const char *combining_char)
{
    int prev_bytes, comb_bytes;
    uint32_t prev_code = get_utf8_code(prev_char, &prev_bytes);
    uint32_t code = get_utf8_code(combining_char, &comb_bytes);

    switch (prev_bytes) {
        default: return NULL;
        case 2: code <<= 8; // fallthrough
        case 1: code <<= 8; break;
    }
    code |= prev_code;

    from_utf8_lookup key = { code };
    return search_utf8_table(&key, data.from_utf8_decomposed_table, data.decomposed_table_size);
}

encoding_type encoding_determine(void)
{
    // Determine encoding based on language:
    // - Windows-1252 (Western Europe) is used in all other languages

    data.to_utf8_table = HIGH_TO_UTF8_DEFAULT;
    data.encoding = ENCODING_WESTERN_EUROPE;

    build_reverse_lookup_table();
    build_decomposed_lookup_table();
    return data.encoding;
}

int encoding_is_multibyte(void)
{
    return !data.to_utf8_table;
}

int encoding_system_uses_decomposed(void)
{
#ifdef __APPLE__
    return 1;
#else
    return 0;
#endif
}

static int is_ascii(const char *utf8_char)
{
    return ((uint8_t) *utf8_char & 0x80) == 0;
}

int encoding_can_display(const char *utf8_char)
{
    return is_ascii(utf8_char) || get_letter_code_for_utf8(utf8_char, NULL, NULL) != NULL;
}

void encoding_to_utf8(const uint8_t *input, char *output, int output_length, int decomposed)
{
    if (!data.to_utf8_table) {
        *output = 0;
        return;
    }
    const char *max_output = &output[output_length - 1];

    while (*input && output < max_output) {
        uint8_t c = *input;
        if (c < 0x80) {
            *output = c;
            ++output;
        } else {
            // multi-byte char
            const letter_code *code = get_letter_code_for_internal(c);
            int num_bytes;
            const uint8_t *bytes;
            if (decomposed && code->bytes_decomposed) {
                num_bytes = code->bytes_decomposed;
                bytes = code->utf8_decomposed;
            } else {
                num_bytes = code->bytes;
                bytes = code->utf8_value;
            }
            if (num_bytes) {
                if (output + num_bytes >= max_output) {
                    break;
                }
                for (int i = 0; i < num_bytes; i++) {
                    *output = bytes[i];
                    ++output;
                }
            }
        }
        ++input;
    }
    *output = 0;
}

void encoding_from_utf8(const char *input, uint8_t *output, int output_length)
{
    const uint8_t *max_output = &output[output_length - 1];

    const char *prev_input = input;
    while (*input && output < max_output) {
        if (is_ascii(input)) {
            *output = *input;
            prev_input = input;
            ++output;
            ++input;
        } else {
            // multi-byte char
            int bytes;
            int is_accent;
            const letter_code *code = get_letter_code_for_utf8(input, &bytes, &is_accent);
            if (code) {
                *output = code->internal_value;
            } else if (is_accent) {
                code = get_letter_code_for_combining_utf8(prev_input, input);
                if (code) {
                    --output;
                    *output = code->internal_value;
                } else {
                    *output = '?';
                }
            } else {
                *output = '?';
            }
            ++output;
            prev_input = input;
            input += bytes;
        }
    }
    *output = 0;
}

