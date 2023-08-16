#include "brutus.h"

#include "core/calc.h"
#include "core/config.h"
#include "core/file.h"
#include "core/lang.h"
#include "core/string.h"
#include "core/time.h"
#include "game/game.h"
#include "game/settings.h"
#include "input/cursor.h"
#include "input/keyboard.h"
#include "input/mouse.h"

#include "SDL.h"

#include <direct.h>
#include <stdnoreturn.h>
#include <signal.h>

#ifdef DRAW_FPS
#include "graphics/graphics.h"
#endif

#define INTPTR(d) (*(int*)(d))

#define MSG_SIZE 1000

char EXECUTABLE_DIR_PATH[DIR_PATH_MAX];
char DATA_TEXT_FILE_PATH[DIR_PATH_MAX];
char SETTINGS_FILE_PATH[DIR_PATH_MAX];
char CONFIGS_FILE_PATH[DIR_PATH_MAX];
char HOTKEY_CONFIGS_FILE_PATH[DIR_PATH_MAX];
char MAPS_DIR_PATH[DIR_PATH_MAX];
char SAVES_DIR_PATH[DIR_PATH_MAX];
char GAME_DATA_PATH[DIR_PATH_MAX];

static struct {
    int active;
    int quit;
} data = { 1, 0 };

static struct {
    SDL_Cursor *cursors[CURSOR_MAX];
    SDL_Surface *surfaces[CURSOR_MAX];
    int current_shape;
    int current_scale;
} cursor_data;

static const color_t mouse_colors[] = {
    ALPHA_TRANSPARENT,
    ALPHA_TRANSPARENT,
    ALPHA_TRANSPARENT,
    ALPHA_OPAQUE | COLOR_BLACK,
    ALPHA_OPAQUE | COLOR_MOUSE_DARK_GRAY,
    ALPHA_OPAQUE | COLOR_MOUSE_MEDIUM_GRAY,
    ALPHA_OPAQUE | COLOR_MOUSE_LIGHT_GRAY,
    ALPHA_OPAQUE | COLOR_WHITE
};

static struct {
    int x;
    int y;
    int enabled;
} mouse_data;

static struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Texture *texture;
    SDL_Texture *cursors[CURSOR_MAX];
} SDL;

static struct {
    int x;
    int y;
    int centered;
} window_pos = { 0, 0, 1 };

static struct {
    const int WIDTH;
    const int HEIGHT;
} MINIMUM = { 640, 480 };

static int scale_percentage = 100;

static char log_buffer[MSG_SIZE];

noreturn static void handler(int sig)
{
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Oops, crashed with signal %d :(", sig);
    _exit(1);
}

static FILE *log_file = 0;

static void write_log(__attribute__((unused)) void *userdata, __attribute__((unused)) int category, SDL_LogPriority priority, const char *message)
{
    if (log_file) {
        if (priority == SDL_LOG_PRIORITY_ERROR) {
            fwrite("ERROR: ", sizeof(char), 7, log_file);
        } else {
            fwrite("INFO: ", sizeof(char), 6, log_file);
        }
        fwrite(message, sizeof(char), string_length(message), log_file);
        fwrite("\n", sizeof(char), 1, log_file);
        fflush(log_file);
    }
}

static void setup_logging(void)
{
    log_file = fopen("brutus-log.txt", "wt");
    SDL_LogSetOutputFunction(write_log, NULL);
}

void post_event(int code)
{
    SDL_Event event;
    event.user.type = SDL_USEREVENT;
    event.user.code = code;
    SDL_PushEvent(&event);
}

void system_resize(int width, int height)
{
    static int s_width;
    static int s_height;

    s_width = width;
    s_height = height;
    SDL_Event event;
    event.user.type = SDL_USEREVENT;
    event.user.code = USER_EVENT_RESIZE;
    event.user.data1 = &s_width;
    event.user.data2 = &s_height;
    SDL_PushEvent(&event);
}

#ifdef DRAW_FPS
static struct {
    int frame_count;
    int last_fps;
    Uint32 last_update_time;
} fps;

static void run_and_draw(void)
{
    uint32_t time_before_run = SDL_GetTicks();
    time_set_millis(time_before_run);

    game_run();
    Uint32 time_between_run_and_draw = SDL_GetTicks();
    game_draw();
    Uint32 time_after_draw = SDL_GetTicks();

    fps.frame_count++;
    if (time_after_draw - fps.last_update_time > 1000) {
        fps.last_fps = fps.frame_count;
        fps.last_update_time = time_after_draw;
        fps.frame_count = 0;
    }
    if (window_is(WINDOW_CITY) || window_is(WINDOW_CITY_MILITARY) || window_is(WINDOW_SLIDING_SIDEBAR)) {
        int y_offset = 24;
        int y_offset_text = y_offset + 5;
        graphics_fill_rect(0, y_offset, 100, 20, COLOR_WHITE);
        text_draw_number_colored(fps.last_fps,
            'f', "", 5, y_offset_text, FONT_NORMAL_PLAIN, COLOR_FONT_RED);
        text_draw_number_colored(time_between_run_and_draw - time_before_run,
            'g', "", 40, y_offset_text, FONT_NORMAL_PLAIN, COLOR_FONT_RED);
        text_draw_number_colored(time_after_draw - time_between_run_and_draw,
            'd', "", 70, y_offset_text, FONT_NORMAL_PLAIN, COLOR_FONT_RED);
    }
    update_screen();
    SDL_RenderPresent(SDL.renderer);
}
#else
static void run_and_draw(void)
{
    time_set_millis(SDL_GetTicks());

    game_run();
    game_draw();

    update_screen();
    SDL_RenderPresent(SDL.renderer);
}
#endif

static void handle_mouse_button(SDL_MouseButtonEvent *event, int is_down)
{
    if (!SDL_GetRelativeMouseMode()) {
        mouse_set_position(event->x, event->y);
    }
    if (event->button == SDL_BUTTON_LEFT) {
        mouse_set_left_down(is_down);
    } else if (event->button == SDL_BUTTON_RIGHT) {
        mouse_set_right_down(is_down);
    }
}

static int scale_pixels_to_logical(int pixel_value)
{
    return pixel_value * 100 / scale_percentage;
}

static void destroy_screen_texture(void)
{
    SDL_DestroyTexture(SDL.texture);
    SDL.texture = 0;
}

static int resize_screen(int pixel_width, int pixel_height)
{
    int logical_width = scale_pixels_to_logical(pixel_width);
    int logical_height = scale_pixels_to_logical(pixel_height);

    if (SDL.texture) {
        if (logical_width == screen_width() && logical_height == screen_height()) {
            return 1;
        }
        destroy_screen_texture();
    }

    SDL_RenderSetLogicalSize(SDL.renderer, logical_width, logical_height);

    setting_set_display(setting_fullscreen(), logical_width, logical_height);
    SDL.texture = SDL_CreateTexture(SDL.renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
        logical_width, logical_height);

    if (SDL.texture) {
        SDL_Log("Texture created: %d x %d", logical_width, logical_height);
        screen_set_resolution(logical_width, logical_height);
        return 1;
    } else {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create texture: %s", SDL_GetError());
        return 0;
    }
}

static void handle_window_event(SDL_WindowEvent *event, int *window_active)
{
    switch (event->event) {
        case SDL_WINDOWEVENT_ENTER:
            mouse_set_inside_window(1);
            break;
        case SDL_WINDOWEVENT_LEAVE:
            mouse_set_inside_window(0);
            break;
        case SDL_WINDOWEVENT_SIZE_CHANGED:
            SDL_Log("Window resized to %d x %d", (int) event->data1, (int) event->data2);
            resize_screen(event->data1, event->data2);
            break;
        case SDL_WINDOWEVENT_RESIZED:
            SDL_Log("System resize to %d x %d", (int) event->data1, (int) event->data2);
            break;
        case SDL_WINDOWEVENT_MOVED:
            SDL_Log("Window move to coordinates x: %d y: %d\n", (int) event->data1, (int) event->data2);
            if (!setting_fullscreen()) {
                window_pos.x = event->data1;
                window_pos.y = event->data2;
                window_pos.centered = 0;
            }
            break;
        case SDL_WINDOWEVENT_SHOWN:
            SDL_Log("Window %u shown", event->windowID);
            *window_active = 1;
            break;
        case SDL_WINDOWEVENT_HIDDEN:
            SDL_Log("Window %u hidden", event->windowID);
            *window_active = 0;
            break;
    }
}

static int get_key_from_scancode(SDL_Scancode scancode)
{
    switch (scancode) {
        case SDL_SCANCODE_A: return KEY_TYPE_A;
        case SDL_SCANCODE_B: return KEY_TYPE_B;
        case SDL_SCANCODE_C: return KEY_TYPE_C;
        case SDL_SCANCODE_D: return KEY_TYPE_D;
        case SDL_SCANCODE_E: return KEY_TYPE_E;
        case SDL_SCANCODE_F: return KEY_TYPE_F;
        case SDL_SCANCODE_G: return KEY_TYPE_G;
        case SDL_SCANCODE_H: return KEY_TYPE_H;
        case SDL_SCANCODE_I: return KEY_TYPE_I;
        case SDL_SCANCODE_J: return KEY_TYPE_J;
        case SDL_SCANCODE_K: return KEY_TYPE_K;
        case SDL_SCANCODE_L: return KEY_TYPE_L;
        case SDL_SCANCODE_M: return KEY_TYPE_M;
        case SDL_SCANCODE_N: return KEY_TYPE_N;
        case SDL_SCANCODE_O: return KEY_TYPE_O;
        case SDL_SCANCODE_P: return KEY_TYPE_P;
        case SDL_SCANCODE_Q: return KEY_TYPE_Q;
        case SDL_SCANCODE_R: return KEY_TYPE_R;
        case SDL_SCANCODE_S: return KEY_TYPE_S;
        case SDL_SCANCODE_T: return KEY_TYPE_T;
        case SDL_SCANCODE_U: return KEY_TYPE_U;
        case SDL_SCANCODE_V: return KEY_TYPE_V;
        case SDL_SCANCODE_W: return KEY_TYPE_W;
        case SDL_SCANCODE_X: return KEY_TYPE_X;
        case SDL_SCANCODE_Y: return KEY_TYPE_Y;
        case SDL_SCANCODE_Z: return KEY_TYPE_Z;
        case SDL_SCANCODE_1: return KEY_TYPE_1;
        case SDL_SCANCODE_2: return KEY_TYPE_2;
        case SDL_SCANCODE_3: return KEY_TYPE_3;
        case SDL_SCANCODE_4: return KEY_TYPE_4;
        case SDL_SCANCODE_5: return KEY_TYPE_5;
        case SDL_SCANCODE_6: return KEY_TYPE_6;
        case SDL_SCANCODE_7: return KEY_TYPE_7;
        case SDL_SCANCODE_8: return KEY_TYPE_8;
        case SDL_SCANCODE_9: return KEY_TYPE_9;
        case SDL_SCANCODE_0: return KEY_TYPE_0;
        case SDL_SCANCODE_RETURN: return KEY_TYPE_ENTER;
        case SDL_SCANCODE_ESCAPE: return KEY_TYPE_ESCAPE;
        case SDL_SCANCODE_BACKSPACE: return KEY_TYPE_BACKSPACE;
        case SDL_SCANCODE_TAB: return KEY_TYPE_TAB;
        case SDL_SCANCODE_SPACE: return KEY_TYPE_SPACE;
        case SDL_SCANCODE_MINUS: return KEY_TYPE_MINUS;
        case SDL_SCANCODE_EQUALS: return KEY_TYPE_EQUALS;
        case SDL_SCANCODE_LEFTBRACKET: return KEY_TYPE_LEFTBRACKET;
        case SDL_SCANCODE_RIGHTBRACKET: return KEY_TYPE_RIGHTBRACKET;
        case SDL_SCANCODE_BACKSLASH: return KEY_TYPE_BACKSLASH;
        case SDL_SCANCODE_SEMICOLON: return KEY_TYPE_SEMICOLON;
        case SDL_SCANCODE_APOSTROPHE: return KEY_TYPE_APOSTROPHE;
        case SDL_SCANCODE_GRAVE: return KEY_TYPE_GRAVE;
        case SDL_SCANCODE_COMMA: return KEY_TYPE_COMMA;
        case SDL_SCANCODE_PERIOD: return KEY_TYPE_PERIOD;
        case SDL_SCANCODE_SLASH: return KEY_TYPE_SLASH;
        case SDL_SCANCODE_CAPSLOCK: return KEY_TYPE_CAPSLOCK;
        case SDL_SCANCODE_F1: return KEY_TYPE_F1;
        case SDL_SCANCODE_F2: return KEY_TYPE_F2;
        case SDL_SCANCODE_F3: return KEY_TYPE_F3;
        case SDL_SCANCODE_F4: return KEY_TYPE_F4;
        case SDL_SCANCODE_F5: return KEY_TYPE_F5;
        case SDL_SCANCODE_F6: return KEY_TYPE_F6;
        case SDL_SCANCODE_F7: return KEY_TYPE_F7;
        case SDL_SCANCODE_F8: return KEY_TYPE_F8;
        case SDL_SCANCODE_F9: return KEY_TYPE_F9;
        case SDL_SCANCODE_F10: return KEY_TYPE_F10;
        case SDL_SCANCODE_F11: return KEY_TYPE_F11;
        case SDL_SCANCODE_F12: return KEY_TYPE_F12;
        case SDL_SCANCODE_INSERT: return KEY_TYPE_INSERT;
        case SDL_SCANCODE_HOME: return KEY_TYPE_HOME;
        case SDL_SCANCODE_PAGEUP: return KEY_TYPE_PAGEUP;
        case SDL_SCANCODE_DELETE: return KEY_TYPE_DELETE;
        case SDL_SCANCODE_END: return KEY_TYPE_END;
        case SDL_SCANCODE_PAGEDOWN: return KEY_TYPE_PAGEDOWN;
        case SDL_SCANCODE_RIGHT: return KEY_TYPE_RIGHT;
        case SDL_SCANCODE_LEFT: return KEY_TYPE_LEFT;
        case SDL_SCANCODE_DOWN: return KEY_TYPE_DOWN;
        case SDL_SCANCODE_UP: return KEY_TYPE_UP;
        case SDL_SCANCODE_KP_ENTER: return KEY_TYPE_ENTER;
        case SDL_SCANCODE_KP_1: return KEY_TYPE_KP_1;
        case SDL_SCANCODE_KP_2: return KEY_TYPE_KP_2;
        case SDL_SCANCODE_KP_3: return KEY_TYPE_KP_3;
        case SDL_SCANCODE_KP_4: return KEY_TYPE_KP_4;
        case SDL_SCANCODE_KP_5: return KEY_TYPE_KP_5;
        case SDL_SCANCODE_KP_6: return KEY_TYPE_KP_6;
        case SDL_SCANCODE_KP_7: return KEY_TYPE_KP_7;
        case SDL_SCANCODE_KP_8: return KEY_TYPE_KP_8;
        case SDL_SCANCODE_KP_9: return KEY_TYPE_KP_9;
        case SDL_SCANCODE_KP_0: return KEY_TYPE_KP_0;
        case SDL_SCANCODE_KP_PERIOD: return KEY_TYPE_KP_PERIOD;
        case SDL_SCANCODE_KP_PLUS: return KEY_TYPE_KP_PLUS;
        case SDL_SCANCODE_KP_MINUS: return KEY_TYPE_KP_MINUS;
        case SDL_SCANCODE_KP_MULTIPLY: return KEY_TYPE_KP_MULTIPLY;
        case SDL_SCANCODE_KP_DIVIDE: return KEY_TYPE_KP_DIVIDE;
        case SDL_SCANCODE_NONUSBACKSLASH: return KEY_TYPE_NON_US;
        default: return KEY_TYPE_NONE;
    }
}

static int get_modifier(int mod)
{
    int key_mod = KEY_MOD_NONE;
    if (mod & KMOD_SHIFT) {
        key_mod |= KEY_MOD_SHIFT;
    }
    if (mod & KMOD_CTRL) {
        key_mod |= KEY_MOD_CTRL;
    }
    if (mod & KMOD_ALT) {
        key_mod |= KEY_MOD_ALT;
    }
    if (mod & KMOD_GUI) {
        key_mod |= KEY_MOD_GUI;
    }
    return key_mod;
}

static void center_window(void)
{
    int display = SDL_GetWindowDisplayIndex(SDL.window);
    SDL_SetWindowPosition(SDL.window,
        SDL_WINDOWPOS_CENTERED_DISPLAY(display), SDL_WINDOWPOS_CENTERED_DISPLAY(display));
    window_pos.centered = 1;
}

static int scale_logical_to_pixels(int logical_value)
{
    return logical_value * scale_percentage / 100;
}

static void handle_event(SDL_Event *event)
{
    switch (event->type) {
        case SDL_WINDOWEVENT:
            handle_window_event(&event->window, &data.active);
            break;
        case SDL_KEYDOWN:
            // handle keyboard input keys
            switch (event->key.keysym.sym) {
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    // only send enter if no modifier is also down
                    if ((event->key.keysym.mod & (KMOD_CTRL | KMOD_ALT | KMOD_GUI)) == 0) {
                        keyboard_return();
                    }
                    break;
                case SDLK_BACKSPACE:
                    keyboard_backspace();
                    break;
                case SDLK_DELETE:
                    keyboard_delete();
                    break;
                case SDLK_LEFT:
                    keyboard_left();
                    break;
                case SDLK_RIGHT:
                    keyboard_right();
                    break;
                case SDLK_UP:
                    keyboard_left();
                    break;
                case SDLK_DOWN:
                    keyboard_right();
                    break;
                case SDLK_HOME:
                    keyboard_home();
                    break;
                case SDLK_END:
                    keyboard_end();
                    break;
                case SDLK_AC_BACK:
                    event->key.keysym.scancode = SDL_SCANCODE_ESCAPE;
                    break;
            }

            // handle struct hotkeys_t
            hotkey_key_pressed(get_key_from_scancode(event->key.keysym.scancode), get_modifier(event->key.keysym.mod), event->key.repeat);
            break;
        case SDL_KEYUP:
            hotkey_key_released(get_key_from_scancode(event->key.keysym.scancode), get_modifier(event->key.keysym.mod));
            break;
        case SDL_TEXTINPUT:
            keyboard_text(event->text.text);
            break;
        case SDL_MOUSEMOTION:
            if (event->motion.which != SDL_TOUCH_MOUSEID && !SDL_GetRelativeMouseMode()) {
                mouse_set_position(event->motion.x, event->motion.y);
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.which != SDL_TOUCH_MOUSEID) {
                handle_mouse_button(&event->button, 1);
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event->button.which != SDL_TOUCH_MOUSEID) {
                handle_mouse_button(&event->button, 0);
            }
            break;
        case SDL_MOUSEWHEEL:
            if (event->wheel.which != SDL_TOUCH_MOUSEID) {
                mouse_set_scroll(event->wheel.y > 0 ? SCROLL_UP : event->wheel.y < 0 ? SCROLL_DOWN : SCROLL_NONE);
            }
            break;
        case SDL_QUIT:
            data.quit = 1;
            break;
        case SDL_USEREVENT:
            if (event->user.code == USER_EVENT_QUIT) {
                data.quit = 1;
            } else if (event->user.code == USER_EVENT_RESIZE) {
                int pixel_width = scale_logical_to_pixels(INTPTR(event->user.data1));
                int pixel_height = scale_logical_to_pixels(INTPTR(event->user.data2));
                int display = SDL_GetWindowDisplayIndex(SDL.window);
                if (setting_fullscreen()) {
                    SDL_SetWindowFullscreen(SDL.window, 0);
                } else {
                    SDL_GetWindowPosition(SDL.window, &window_pos.x, &window_pos.y);
                }
                if (SDL_GetWindowFlags(SDL.window) & SDL_WINDOW_MAXIMIZED) {
                    SDL_RestoreWindow(SDL.window);
                }
                SDL_SetWindowSize(SDL.window, pixel_width, pixel_height);
                if (window_pos.centered) {
                    center_window();
                }
                SDL_Log("User resize to %d x %d on display %d", pixel_width, pixel_height, display);
                if (SDL_GetWindowGrab(SDL.window) == SDL_TRUE) {
                    SDL_SetWindowGrab(SDL.window, SDL_FALSE);
                }
                setting_set_display(0, pixel_width, pixel_height);
            } else if (event->user.code == USER_EVENT_FULLSCREEN) {
                SDL_GetWindowPosition(SDL.window, &window_pos.x, &window_pos.y);
                int display = SDL_GetWindowDisplayIndex(SDL.window);
                SDL_DisplayMode mode;
                SDL_GetDesktopDisplayMode(display, &mode);
                SDL_Log("User to fullscreen %d x %d on display %d", mode.w, mode.h, display);
                if (0 != SDL_SetWindowFullscreen(SDL.window, SDL_WINDOW_FULLSCREEN_DESKTOP)) {
                    SDL_Log("Unable to enter fullscreen: %s", SDL_GetError());
                    return;
                }
                SDL_SetWindowDisplayMode(SDL.window, &mode);

                if (SDL_GetNumVideoDisplays() > 1) {
                    SDL_SetWindowGrab(SDL.window, SDL_TRUE);
                }
                setting_set_display(1, mode.w, mode.h);
            } else if (event->user.code == USER_EVENT_WINDOWED) {
                int logical_width, logical_height;
                setting_window(&logical_width, &logical_height);
                int pixel_width = scale_logical_to_pixels(logical_width);
                int pixel_height = scale_logical_to_pixels(logical_height);
                int display = SDL_GetWindowDisplayIndex(SDL.window);
                SDL_Log("User to windowed %d x %d on display %d", pixel_width, pixel_height, display);
                SDL_SetWindowFullscreen(SDL.window, 0);
                SDL_SetWindowSize(SDL.window, pixel_width, pixel_height);
                if (window_pos.centered) {
                    center_window();
                }
                if (SDL_GetWindowGrab(SDL.window) == SDL_TRUE) {
                    SDL_SetWindowGrab(SDL.window, SDL_FALSE);
                }
                setting_set_display(0, pixel_width, pixel_height);
            } else if (event->user.code == USER_EVENT_CENTER_WINDOW) {
                center_window();
            }
            break;

        default:
            break;
    }
}

static void destroy_screen(void)
{
    if (SDL.texture) {
        destroy_screen_texture();
    }
    if (SDL.renderer) {
        SDL_DestroyRenderer(SDL.renderer);
        SDL.renderer = 0;
    }
    if (SDL.window) {
        SDL_DestroyWindow(SDL.window);
        SDL.window = 0;
    }
}

static void main_loop(void)
{
    SDL_Event event;
    // On Windows, if ctrl + alt + del is pressed during fullscreen, the rendering context may be lost for a few frames
    // after restoring the window, preventing the texture from being recreated. This forces an attempt to recreate the
    // texture every frame to bypass that issue.
    if (!SDL.texture && SDL.renderer && setting_fullscreen()) {
        SDL_DisplayMode mode;
        SDL_GetWindowDisplayMode(SDL.window, &mode);
        screen_set_resolution(scale_pixels_to_logical(mode.w), scale_pixels_to_logical(mode.h));
        SDL.texture = SDL_CreateTexture(SDL.renderer,
            SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
            screen_width(), screen_height());
    }
    /* Process event queue */
    while (SDL_PollEvent(&event)) {
        handle_event(&event);
    }
    if (data.quit) {
        SDL_Log("Exiting game");
        game_exit();
        destroy_screen();
        SDL_Quit();
        if (log_file) {
            fclose(log_file);
        }
        return;
    }
    if (data.active) {
        run_and_draw();
    } else {
        SDL_WaitEvent(NULL);
    }
}

static int init_sdl(void)
{
    SDL_Log("Initializing SDL");

    if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_VIDEO | SDL_INIT_JOYSTICK) != 0) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Could not initialize SDL: %s", SDL_GetError());
        return 0;
    }
    SDL_Log("SDL initialized");
    return 1;
}

static int load_data_dir(void)
{
    FILE *fp = fopen(DATA_TEXT_FILE_PATH, "r");
    if (fp) {
        size_t length = fread(GAME_DATA_PATH, 1, 1000, fp);
        fclose(fp);
        if (length > 0) {
            return 1;
        }
    } else {
        fp = fopen(DATA_TEXT_FILE_PATH, "w");
        fclose(fp);
        return 0;
    }
    return 0;
}

static int pre_init(void)
{
    char *executable_path = SDL_GetBasePath();
    if (executable_path) {
        if (string_length(executable_path) < DIR_PATH_MAX - string_length("brutus.hconfigs")) {
            string_copy(executable_path, EXECUTABLE_DIR_PATH, DIR_PATH_MAX - 1);

            string_copy(executable_path, DATA_TEXT_FILE_PATH, DIR_PATH_MAX - 1);
            strcat(DATA_TEXT_FILE_PATH, "data_dir.txt");

            string_copy(executable_path, SETTINGS_FILE_PATH, DIR_PATH_MAX - 1);
            strcat(SETTINGS_FILE_PATH, "brutus.settings");

            string_copy(executable_path, CONFIGS_FILE_PATH, DIR_PATH_MAX - 1);
            strcat(CONFIGS_FILE_PATH, "brutus.configs");

            string_copy(executable_path, HOTKEY_CONFIGS_FILE_PATH, DIR_PATH_MAX - 1);
            strcat(HOTKEY_CONFIGS_FILE_PATH, "brutus.hconfigs");

            string_copy(executable_path, MAPS_DIR_PATH, DIR_PATH_MAX - 1);
            strcat(MAPS_DIR_PATH, "maps");
            string_copy(executable_path, SAVES_DIR_PATH, DIR_PATH_MAX - 1);
            strcat(SAVES_DIR_PATH, "saves");
        } else {
            SDL_Log("Brutus directory path too long, exiting");
            return 0;
        }
    } else {
        SDL_Log("Brutus directory not found, exiting");
        return 0;
    }
    SDL_free(executable_path);

    if (load_data_dir()) {
        _mkdir(MAPS_DIR_PATH);
        _mkdir(SAVES_DIR_PATH);
        SDL_Log("Loading game from user pref %s", GAME_DATA_PATH);
        if (_chdir(GAME_DATA_PATH) == 0 && game_pre_init()) {
            return 1;
        } else {
            SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
                "Ya dun goofed",
                "Incorrect game path specified in data_dir.txt",
                NULL);
        }
    } else {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION,
            "Game path not specified",
            "Brutus requires Caesar 3 to run. Provide the path to the game in data_dir.txt.",
            NULL);
    }

    return 0;
}

static int get_max_scale_percentage(int pixel_width, int pixel_height)
{
    int width_scale_pct = pixel_width * 100 / MINIMUM.WIDTH;
    int height_scale_pct = pixel_height * 100 / MINIMUM.HEIGHT;
    return SDL_min(width_scale_pct, height_scale_pct);
}

static void set_scale_percentage(int new_scale, int pixel_width, int pixel_height)
{
    scale_percentage = calc_bound(new_scale, 50, 500);

    if (!pixel_width || !pixel_height) {
        return;
    }

    int max_scale_pct = get_max_scale_percentage(pixel_width, pixel_height);
    if (max_scale_pct < scale_percentage) {
        scale_percentage = max_scale_pct;
        SDL_Log("Maximum scale of %i applied", scale_percentage);
    }

    SDL_SetWindowMinimumSize(SDL.window,
        scale_logical_to_pixels(MINIMUM.WIDTH), scale_logical_to_pixels(MINIMUM.HEIGHT));

    const char *scale_quality = "linear";
    // Scale using nearest neighbour when we scale a multiple of 100%: makes it look sharper.
    if (scale_percentage % 100 == 0) {
        scale_quality = "nearest";
    }
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, scale_quality);
}

static int create_screen(const char *title, int display_scale_percentage)
{
    set_scale_percentage(display_scale_percentage, 0, 0);

    int width, height;
    int fullscreen = setting_fullscreen();
    if (fullscreen) {
        SDL_DisplayMode mode;
        SDL_GetDesktopDisplayMode(0, &mode);
        width = mode.w;
        height = mode.h;
    } else {
        setting_window(&width, &height);
        width = scale_logical_to_pixels(width);
        height = scale_logical_to_pixels(height);
    }

    destroy_screen();

    SDL_Log("Creating screen %d x %d, %s, driver: %s", width, height,
        fullscreen ? "fullscreen" : "windowed", SDL_GetCurrentVideoDriver());
    Uint32 flags = SDL_WINDOW_RESIZABLE;

    flags |= SDL_WINDOW_ALLOW_HIGHDPI;

    if (fullscreen) {
        flags |= SDL_WINDOW_FULLSCREEN_DESKTOP;
    }
    SDL.window = SDL_CreateWindow(title,
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        width, height, flags);

    if (!SDL.window) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create window: %s", SDL_GetError());
        return 0;
    }

    SDL_Log("Creating renderer");
    SDL.renderer = SDL_CreateRenderer(SDL.window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!SDL.renderer) {
        SDL_Log("Unable to create renderer, trying software renderer: %s", SDL_GetError());
        SDL.renderer = SDL_CreateRenderer(SDL.window, -1, SDL_RENDERER_SOFTWARE);
        if (!SDL.renderer) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Unable to create renderer: %s", SDL_GetError());
            return 0;
        }
    }

    if (fullscreen && SDL_GetNumVideoDisplays() > 1) {
        SDL_SetWindowGrab(SDL.window, SDL_TRUE);
    }

    set_scale_percentage(display_scale_percentage, width, height);
    return resize_screen(width, height);
}

int main(__attribute__((unused)) int argc, __attribute__((unused)) char **argv) // actually SDL_main
{
    // setup
    signal(SIGSEGV, handler);
    setup_logging();
    SDL_Log("Brutus version %s", system_version());
    if (!init_sdl()) {
        SDL_Log("Exiting: SDL init failed");
        exit(-1);
    }
    if (!pre_init()) {
        SDL_Log("Exiting: game pre-init failed");
        exit(1);
    }
    if (!create_screen("Caesar III", config_get(CONFIG_SCREEN_DISPLAY_SCALE))) {
        SDL_Log("Exiting: SDL create window failed");
        exit(-2);
    }
    init_cursors(config_get(CONFIG_SCREEN_CURSOR_SCALE));
    time_set_millis(SDL_GetTicks());
    if (!game_init()) {
        SDL_Log("Exiting: game init failed");
        exit(2);
    }
    data.quit = 0;
    data.active = 1;

    mouse_set_inside_window(1);
    run_and_draw();

    while (!data.quit) {
        main_loop();
    }

    return 0;
}

void init_cursors(int scale_percentage)
{
    if (scale_percentage <= 100) {
        cursor_data.current_scale = CURSOR_SCALE_1;
    } else if (scale_percentage <= 150) {
        cursor_data.current_scale = CURSOR_SCALE_1_5;
    } else {
        cursor_data.current_scale = CURSOR_SCALE_2;
    }
    for (int i = 0; i < CURSOR_MAX; i++) {
        const struct cursor_t *c = input_cursor_data(i, cursor_data.current_scale);
        if (cursor_data.surfaces[i]) {
            SDL_FreeSurface(cursor_data.surfaces[i]);
        }
        if (cursor_data.cursors[i]) {
            SDL_FreeCursor(cursor_data.cursors[i]);
        }

        int size = 32;
        while (size <= c->width || size <= c->height) {
            size *= 2;
        }

        SDL_Surface *cursor_surface = SDL_CreateRGBSurface(0, size, size, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000);
        color_t *pixels = cursor_surface->pixels;
        SDL_memset(pixels, 0, sizeof(color_t) * size * size);
        for (int y = 0; y < c->height; y++) {
            for (int x = 0; x < c->width; x++) {
                pixels[y * size + x] = mouse_colors[c->data[y * c->width + x] - 32];
            }
        }
        cursor_data.surfaces[i] = cursor_surface;
        cursor_data.cursors[i] = SDL_CreateColorCursor(cursor_data.surfaces[i], c->hotspot_x, c->hotspot_y);
    }
    set_cursor(cursor_data.current_shape);
}

void set_cursor(int cursor_id)
{
    cursor_data.current_shape = cursor_id;
    SDL_SetCursor(cursor_data.cursors[cursor_id]);
}

static const char *build_message(const char *msg, const char *param_str, int param_int)
{
    int index = 0;
    index += snprintf(&log_buffer[index], MSG_SIZE - index, "%s", msg);
    if (param_str) {
        index += snprintf(&log_buffer[index], MSG_SIZE - index, "  %s", param_str);
    }
    if (param_int) {
        snprintf(&log_buffer[index], MSG_SIZE - index, "  %d", param_int);
    }
    return log_buffer;
}

void log_info(const char *msg, const char *param_str, int param_int)
{
    SDL_LogInfo(SDL_LOG_CATEGORY_APPLICATION, "%s", build_message(msg, param_str, param_int));
}

void log_error(const char *msg, const char *param_str, int param_int)
{
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "%s", build_message(msg, param_str, param_int));
}

void set_relative_mouse_mode(int enabled)
{
    if (enabled == mouse_data.enabled) {
        return;
    }
    if (enabled) {
        SDL_GetMouseState(&mouse_data.x, &mouse_data.y);
        SDL_SetRelativeMouseMode(SDL_TRUE);
        // Discard the first value, which is incorrect
        // (the first one gives the relative position to center of window)
        SDL_GetRelativeMouseState(NULL, NULL);
    } else {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        mouse_data.x = calc_bound(mouse_data.x, 0, screen_width() - 1);
        mouse_data.y = calc_bound(mouse_data.y, 0, screen_height() - 1);
        SDL_WarpMouseInWindow(SDL.window, scale_logical_to_pixels(mouse_data.x), scale_logical_to_pixels(mouse_data.y));
        mouse_set_position(mouse_data.x, mouse_data.y);
    }
    mouse_data.enabled = enabled;
}

int scale_display(int display_scale_percentage)
{
    int width, height;
    SDL_GetWindowSize(SDL.window, &width, &height);
    set_scale_percentage(display_scale_percentage, width, height);
    resize_screen(width, height);
    return scale_percentage;
}

int get_max_display_scale(void)
{
    int width, height;
    SDL_GetWindowSize(SDL.window, &width, &height);
    return get_max_scale_percentage(width, height);
}

void update_screen(void)
{
    SDL_RenderClear(SDL.renderer);
    SDL_UpdateTexture(SDL.texture, NULL, graphics_canvas(), screen_width() * 4);
    SDL_RenderCopy(SDL.renderer, SDL.texture, NULL, NULL);
}
