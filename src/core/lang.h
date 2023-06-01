#ifndef CORE_LANG_H
#define CORE_LANG_H

#include <stdint.h>

/**
 * @file
 * Language functions for strings and messages
 */

 /**
  * Type
  */
enum {
    TYPE_MANUAL = 0,
    TYPE_ABOUT = 1,
    TYPE_MESSAGE = 2,
    TYPE_MISSION = 3
};

/**
 * Message type
 */
enum {
    MESSAGE_TYPE_GENERAL = 0,
    MESSAGE_TYPE_DISASTER = 1,
    MESSAGE_TYPE_IMPERIAL = 2,
    MESSAGE_TYPE_EMIGRATION = 3,
    MESSAGE_TYPE_TRADE_CHANGE = 5,
    MESSAGE_TYPE_PRICE_CHANGE = 6,
    MESSAGE_TYPE_INVASION = 7
};

struct lang_message_image {
    int id; /**< ID of the image */
    int x; /**< X offset */
    int y; /**< Y offset */
};

/**
 * Message string
 */
struct lang_message_string {
    const char *text; /**< Text */
    int x; /**< X offset */
    int y; /**< Y offset */
};

/**
 * Message
 */
struct lang_message_t {
    int type;
    int message_type;
    int x;
    int y;
    int width_blocks;
    int height_blocks;
    int urgent;
    struct lang_message_image image;
    struct lang_message_string title;
    struct lang_message_string subtitle;
    struct lang_message_string video;
    struct lang_message_string content;
};

/**
 * Loads the language files
 * @param is_editor Whether to load the editor language files or the regular ones
 * @return boolean true on success, false on failure
 */
int lang_load(int is_editor);

/**
 * Loads messages that are strings in the codebase
 */
void load_custom_messages(void);

/**
 * Gets the string for the specified group/index
 * @param group Text group
 * @param index Index within the group
 * @return String
 */
const char *lang_get_string(int group, int index);

/**
 * Gets the message for the specified ID
 * @param id ID of the message
 * @return Message
 */
const struct lang_message_t *lang_get_message(int id);

#endif // CORE_LANG_H
