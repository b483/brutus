#include "keys.h"

#include "core/string.h"
#include "graphics/graphics.h"
#include "platform/brutus.h"

#include <string.h>

static const char *key_names[KEY_TYPE_MAX_ITEMS] = {
    "", "A", "B", "C", "D", "E", "F", "G", "H", "I",
    "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S",
    "T", "U", "V", "W", "X", "Y", "Z", "1", "2", "3",
    "4", "5", "6", "7", "8", "9", "0", "-", "=", "Enter",
    "Esc", "Backspace", "Tab", "Space", "[", "]", "\\", ";", "'", "`",
    ",", ".", "/", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
    "F8", "F9", "F10", "F11", "F12", "Insert", "Delete", "Home", "End", "PageUp",
    "PageDown", "Right", "Left", "Down", "Up",
    "Kp1", "Kp2", "Kp3", "Kp4", "Kp5", "Kp6", "Kp7", "Kp8", "Kp9", "Kp0",
    "Kp.", "Kp+", "Kp-", "Kp*", "Kp/", "NonUS"
};

static const char *key_display_names[KEY_TYPE_MAX_ITEMS] = {
    "", "A", "B", "C", "D", "E", "F", "G", "H", "I",
    "J", "K", "L", "M", "N", "O", "P", "Q", "R", "S",
    "T", "U", "V", "W", "X", "Y", "Z", "1", "2", "3",
    "4", "5", "6", "7", "8", "9", "0", "-", "=", "Enter",
    "Esc", "Backspace", "Tab", "Space", "Left bracket", "Right bracket", "Backslash", ";", "'", "Backtick",
    ",", ".", "/", "F1", "F2", "F3", "F4", "F5", "F6", "F7",
    "F8", "F9", "F10", "F11", "F12", "Insert", "Delete", "Home", "End", "PageUp",
    "PageDown", "Right", "Left", "Down", "Up",
    "Keypad 1", "Keypad 2", "Keypad 3", "Keypad 4", "Keypad 5",
    "Keypad 6", "Keypad 7", "Keypad 8", "Keypad 9", "Keypad 0",
    "Keypad .", "Keypad +", "Keypad -", "Keypad *", "Keypad /", "NonUS"
};

struct modifier_name_t {
    int modifier;
    const char *name;
};

static const struct modifier_name_t modifier_names[] = {
    {KEY_MOD_CTRL, "Ctrl"},
    {KEY_MOD_ALT, "Alt"},
    {KEY_MOD_GUI, "Gui"},
    {KEY_MOD_SHIFT, "Shift"},
    {KEY_MOD_NONE, 0}
};

const char *key_combination_name(int key, int modifiers)
{
    static char name[100];
    name[0] = 0;
    for (const struct modifier_name_t *modname = modifier_names; modname->modifier; modname++) {
        if (modifiers & modname->modifier) {
            strcat(name, modname->name);
            strcat(name, " ");
        }
    }
    strcat(name, key_names[key]);
    return name;
}

static int parse_modifier(const char *name)
{
    for (const struct modifier_name_t *modname = modifier_names; modname->modifier; modname++) {
        if (string_equals(modname->name, name)) {
            return modname->modifier;
        }
    }
    return KEY_MOD_NONE;
}

static int parse_key(const char *name)
{
    for (int i = 1; i < KEY_TYPE_MAX_ITEMS; i++) {
        if (string_equals(key_names[i], name)) {
            return i;
        }
    }
    return KEY_TYPE_NONE;
}

int key_combination_from_name(const char *name, int *key, int *modifiers)
{
    char editable_name[100] = { 0 };
    string_copy(name, editable_name, 99);

    *key = KEY_TYPE_NONE;
    *modifiers = KEY_MOD_NONE;

    char *token = strtok(editable_name, " ");
    while (token) {
        if (token[0]) {
            int mod = parse_modifier(token);
            if (mod != KEY_MOD_NONE) {
                *modifiers |= mod;
            } else {
                *key = parse_key(token);
                if (*key == KEY_TYPE_NONE) {
                    return 0;
                }
            }
        }
        token = strtok(0, " ");
    }
    if (*key == KEY_TYPE_NONE) {
        return 0;
    }
    return 1;
}

static SDL_Scancode get_scancode_from_key(int key)
{
    switch (key) {
        case KEY_TYPE_A: return SDL_SCANCODE_A;
        case KEY_TYPE_B: return SDL_SCANCODE_B;
        case KEY_TYPE_C: return SDL_SCANCODE_C;
        case KEY_TYPE_D: return SDL_SCANCODE_D;
        case KEY_TYPE_E: return SDL_SCANCODE_E;
        case KEY_TYPE_F: return SDL_SCANCODE_F;
        case KEY_TYPE_G: return SDL_SCANCODE_G;
        case KEY_TYPE_H: return SDL_SCANCODE_H;
        case KEY_TYPE_I: return SDL_SCANCODE_I;
        case KEY_TYPE_J: return SDL_SCANCODE_J;
        case KEY_TYPE_K: return SDL_SCANCODE_K;
        case KEY_TYPE_L: return SDL_SCANCODE_L;
        case KEY_TYPE_M: return SDL_SCANCODE_M;
        case KEY_TYPE_N: return SDL_SCANCODE_N;
        case KEY_TYPE_O: return SDL_SCANCODE_O;
        case KEY_TYPE_P: return SDL_SCANCODE_P;
        case KEY_TYPE_Q: return SDL_SCANCODE_Q;
        case KEY_TYPE_R: return SDL_SCANCODE_R;
        case KEY_TYPE_S: return SDL_SCANCODE_S;
        case KEY_TYPE_T: return SDL_SCANCODE_T;
        case KEY_TYPE_U: return SDL_SCANCODE_U;
        case KEY_TYPE_V: return SDL_SCANCODE_V;
        case KEY_TYPE_W: return SDL_SCANCODE_W;
        case KEY_TYPE_X: return SDL_SCANCODE_X;
        case KEY_TYPE_Y: return SDL_SCANCODE_Y;
        case KEY_TYPE_Z: return SDL_SCANCODE_Z;
        case KEY_TYPE_1: return SDL_SCANCODE_1;
        case KEY_TYPE_2: return SDL_SCANCODE_2;
        case KEY_TYPE_3: return SDL_SCANCODE_3;
        case KEY_TYPE_4: return SDL_SCANCODE_4;
        case KEY_TYPE_5: return SDL_SCANCODE_5;
        case KEY_TYPE_6: return SDL_SCANCODE_6;
        case KEY_TYPE_7: return SDL_SCANCODE_7;
        case KEY_TYPE_8: return SDL_SCANCODE_8;
        case KEY_TYPE_9: return SDL_SCANCODE_9;
        case KEY_TYPE_0: return SDL_SCANCODE_0;
        case KEY_TYPE_ENTER: return SDL_SCANCODE_RETURN;
        case KEY_TYPE_ESCAPE: return SDL_SCANCODE_ESCAPE;
        case KEY_TYPE_BACKSPACE: return SDL_SCANCODE_BACKSPACE;
        case KEY_TYPE_TAB: return SDL_SCANCODE_TAB;
        case KEY_TYPE_SPACE: return SDL_SCANCODE_SPACE;
        case KEY_TYPE_MINUS: return SDL_SCANCODE_MINUS;
        case KEY_TYPE_EQUALS: return SDL_SCANCODE_EQUALS;
        case KEY_TYPE_LEFTBRACKET: return SDL_SCANCODE_LEFTBRACKET;
        case KEY_TYPE_RIGHTBRACKET: return SDL_SCANCODE_RIGHTBRACKET;
        case KEY_TYPE_BACKSLASH: return SDL_SCANCODE_BACKSLASH;
        case KEY_TYPE_SEMICOLON: return SDL_SCANCODE_SEMICOLON;
        case KEY_TYPE_APOSTROPHE: return SDL_SCANCODE_APOSTROPHE;
        case KEY_TYPE_GRAVE: return SDL_SCANCODE_GRAVE;
        case KEY_TYPE_COMMA: return SDL_SCANCODE_COMMA;
        case KEY_TYPE_PERIOD: return SDL_SCANCODE_PERIOD;
        case KEY_TYPE_SLASH: return SDL_SCANCODE_SLASH;
        case KEY_TYPE_CAPSLOCK: return SDL_SCANCODE_CAPSLOCK;
        case KEY_TYPE_F1: return SDL_SCANCODE_F1;
        case KEY_TYPE_F2: return SDL_SCANCODE_F2;
        case KEY_TYPE_F3: return SDL_SCANCODE_F3;
        case KEY_TYPE_F4: return SDL_SCANCODE_F4;
        case KEY_TYPE_F5: return SDL_SCANCODE_F5;
        case KEY_TYPE_F6: return SDL_SCANCODE_F6;
        case KEY_TYPE_F7: return SDL_SCANCODE_F7;
        case KEY_TYPE_F8: return SDL_SCANCODE_F8;
        case KEY_TYPE_F9: return SDL_SCANCODE_F9;
        case KEY_TYPE_F10: return SDL_SCANCODE_F10;
        case KEY_TYPE_F11: return SDL_SCANCODE_F11;
        case KEY_TYPE_F12: return SDL_SCANCODE_F12;
        case KEY_TYPE_INSERT: return SDL_SCANCODE_INSERT;
        case KEY_TYPE_HOME: return SDL_SCANCODE_HOME;
        case KEY_TYPE_PAGEUP: return SDL_SCANCODE_PAGEUP;
        case KEY_TYPE_DELETE: return SDL_SCANCODE_DELETE;
        case KEY_TYPE_END: return SDL_SCANCODE_END;
        case KEY_TYPE_PAGEDOWN: return SDL_SCANCODE_PAGEDOWN;
        case KEY_TYPE_RIGHT: return SDL_SCANCODE_RIGHT;
        case KEY_TYPE_LEFT: return SDL_SCANCODE_LEFT;
        case KEY_TYPE_DOWN: return SDL_SCANCODE_DOWN;
        case KEY_TYPE_UP: return SDL_SCANCODE_UP;
        case KEY_TYPE_KP_1: return SDL_SCANCODE_KP_1;
        case KEY_TYPE_KP_2: return SDL_SCANCODE_KP_2;
        case KEY_TYPE_KP_3: return SDL_SCANCODE_KP_3;
        case KEY_TYPE_KP_4: return SDL_SCANCODE_KP_4;
        case KEY_TYPE_KP_5: return SDL_SCANCODE_KP_5;
        case KEY_TYPE_KP_6: return SDL_SCANCODE_KP_6;
        case KEY_TYPE_KP_7: return SDL_SCANCODE_KP_7;
        case KEY_TYPE_KP_8: return SDL_SCANCODE_KP_8;
        case KEY_TYPE_KP_9: return SDL_SCANCODE_KP_9;
        case KEY_TYPE_KP_0: return SDL_SCANCODE_KP_0;
        case KEY_TYPE_KP_PERIOD: return SDL_SCANCODE_KP_PERIOD;
        case KEY_TYPE_KP_PLUS: return SDL_SCANCODE_KP_PLUS;
        case KEY_TYPE_KP_MINUS: return SDL_SCANCODE_KP_MINUS;
        case KEY_TYPE_KP_MULTIPLY: return SDL_SCANCODE_KP_MULTIPLY;
        case KEY_TYPE_KP_DIVIDE: return SDL_SCANCODE_KP_DIVIDE;
        case KEY_TYPE_NON_US: return SDL_SCANCODE_NONUSBACKSLASH;
        default: return SDL_SCANCODE_UNKNOWN;
    }
}

const char *key_combination_display_name(int key, int modifiers)
{
    static char result[100];

    result[0] = 0;
    if (modifiers & KEY_MOD_CTRL) {
        strcat(result, "Ctrl");
        strcat(result, " ");
    }
    if (modifiers & KEY_MOD_ALT) {
        strcat(result, "Alt");
        strcat(result, " ");
    }
    if (modifiers & KEY_MOD_GUI) {
        strcat(result, "Gui");
        strcat(result, " ");
    }
    if (modifiers & KEY_MOD_SHIFT) {
        strcat(result, "Shift");
        strcat(result, " ");
    }

    // Modifiers are easy, now for key name...
    const char *key_name = SDL_GetKeyName(SDL_GetKeyFromScancode(get_scancode_from_key(key)));
    if ((key_name[0] & 0x80) == 0) {
        // Special cases where we know the key is not displayable using the internal font
        switch (key_name[0]) {
            case '[': key_name = "Left bracket"; break;
            case ']': key_name = "Right bracket"; break;
            case '\\': key_name = "Backslash"; break;
            case '`': key_name = "Backtick"; break;
            case '~': key_name = "Tilde"; break;
            case '#': key_name = "Hash"; break;
            case '$': key_name = "Dollar"; break;
            case '&': key_name = "Ampersand"; break;
            case '<': key_name = "Less than"; break;
            case '>': key_name = "Greater than"; break;
            case '@': key_name = "At-sign"; break;
            case '^': key_name = "Caret"; break;
            case '_': key_name = "Underscore"; break;
            case '|': key_name = "Pipe"; break;
            case '{': key_name = "Left curly brace"; break;
            case '}': key_name = "Right curly brace"; break;
            case '\0': key_name = key_display_names[key];
        }
        strcat(result, key_name);
    } else if (font_can_display(key_name)) {
        strcat(result, key_name);
    } else {
        strcat(result, "? (");
        strcat(result, key_display_names[key]);
        strcat(result, ")");
    }
    return result;
}
