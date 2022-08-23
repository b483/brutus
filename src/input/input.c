#include "input.h"

#include "input/hotkey.h"
#include "input/mouse.h"

int input_go_back_requested(const mouse *m, const hotkeys *h)
{
    return m->right.went_up || h->escape_pressed;
}
