#include "map.h"

#include "city/view.h"
#include "editor/editor.h"
#include "editor/tool.h"
#include "game/game.h"
#include "graphics/generic_button.h"
#include "graphics/graphics.h"
#include "graphics/image.h"
#include "graphics/lang_text.h"
#include "graphics/panel.h"
#include "graphics/text.h"
#include "graphics/window.h"
#include "widget/map_editor.h"
#include "widget/top_menu_editor.h"
#include "widget/sidebar/editor.h"
#include "window/file_dialog.h"
#include "window/popup_dialog.h"
#include "window/editor/attributes.h"

static void draw_background(void)
{
    graphics_clear_screen();
    widget_sidebar_editor_draw_background();
    widget_top_menu_editor_draw();
}

static void draw_foreground(void)
{
    widget_sidebar_editor_draw_foreground();
    widget_map_editor_draw();
}

static void handle_hotkeys(const hotkeys *h)
{
    if (h->load_file) {
        window_file_dialog_show(FILE_TYPE_SCENARIO, FILE_DIALOG_LOAD);
    }
    if (h->save_file) {
        window_file_dialog_show(FILE_TYPE_SCENARIO, FILE_DIALOG_SAVE);
    }
}

static void handle_input(const mouse *m, const hotkeys *h)
{
    handle_hotkeys(h);
    if (widget_top_menu_editor_handle_input(m, h)) {
        return;
    }
    if (widget_sidebar_editor_handle_mouse(m)) {
        return;
    }
    widget_map_editor_handle_input(m, h);
}

void window_editor_map_draw_all(void)
{
    draw_background();
    draw_foreground();
}

void window_editor_map_draw_panels(void)
{
    draw_background();
}

void window_editor_map_draw(void)
{
    widget_map_editor_draw();
}

void window_editor_map_show(void)
{
    window_type window = {
        WINDOW_EDITOR_MAP,
        draw_background,
        draw_foreground,
        handle_input
    };
    window_show(&window);
}
