#ifndef WINDOW_EDITOR_EDIT_CUSTOM_MESSAGE_H
#define WINDOW_EDITOR_EDIT_CUSTOM_MESSAGE_H

enum {
    CUSTOM_MESSAGE_ATTRIBUTES = 0,
    CUSTOM_MESSAGE_TITLE = 1,
    CUSTOM_MESSAGE_TEXT = 2,
};

void window_editor_edit_custom_message_show(int id, int category);

#endif // WINDOW_EDITOR_EDIT_CUSTOM_MESSAGE_H
