#include "logo.h"

#include "core/config.h"
#include "game/system.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/window.h"
#include "sound/sound.h"
#include "window/intro_video.h"
#include "window/main_menu.h"
#include "window/plain_message_dialog.h"

static void draw_background(void)
{
    graphics_clear_screen();

    graphics_in_dialog();
    image_draw(image_group(GROUP_LOGO), 0, 0);
    lang_text_draw_centered_colored(13, 7, 160, 462, 320, FONT_NORMAL_PLAIN, COLOR_WHITE);
    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    if (m->left.went_up || m->right.went_up) {
        window_main_menu_show(0);
        return;
    }
    if (h->escape_pressed) {
        system_exit();
    }
}

void window_logo_show(int show_patch_message)
{
    struct window_type_t window = {
        WINDOW_LOGO,
        draw_background,
        0,
        handle_input,
    };
    play_intro_music();
    window_show(&window);
    if (show_patch_message == MESSAGE_MISSING_PATCH) {
        window_plain_message_dialog_show("Patch 1.0.1.0 not installed", "Your Caesar 3 installation does not have the 1.0.1.0 patch installed.\n\
        You can download the patch from : https://github.com/bvschaik/julius/wiki/Patches.\nContinue at your own risk.");
    }
    if (config_get(CONFIG_UI_SHOW_INTRO_VIDEO)) {
        window_intro_video_show();
    }
}
