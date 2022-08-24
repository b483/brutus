#ifndef WINDOW_PLAIN_MESSAGE_DIALOG_H
#define WINDOW_PLAIN_MESSAGE_DIALOG_H

#include "game/custom_strings.h"

void window_plain_message_dialog_show(custom_string_key title, custom_string_key message);
void window_plain_message_dialog_show_with_extra(custom_string_key title, custom_string_key message, const uint8_t *extra);

#endif // WINDOW_PLAIN_MESSAGE_DIALOG_H
