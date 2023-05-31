#include "SDL.h"

#include "core/config.h"
#include "core/encoding.h"
#include "core/file.h"
#include "core/lang.h"
#include "core/time.h"
#include "game/game.h"
#include "game/settings.h"
#include "game/system.h"
#include "graphics/screen.h"
#include "input/mouse.h"
#include "platform/arguments.h"
#include "platform/keyboard_input.h"
#include "platform/platform.h"
#include "platform/screen.h"

#include <stdnoreturn.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#include <direct.h>
#elif(__linux__)
#include <sys/stat.h>
#include <sys/types.h>
#endif

#ifdef DRAW_FPS
#include "graphics/window.h"
#include "graphics/graphics.h"
#include "graphics/text.h"
#endif

#define INTPTR(d) (*(int*)(d))

enum {
    USER_EVENT_QUIT,
    USER_EVENT_RESIZE,
    USER_EVENT_FULLSCREEN,
    USER_EVENT_WINDOWED,
    USER_EVENT_CENTER_WINDOW,
};

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

noreturn static void exit_with_status(int status)
{
    _exit(status);
}

noreturn static void handler(int sig)
{
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Oops, crashed with signal %d :(", sig);
    exit_with_status(1);
}

#if defined(_WIN32)
/* Log to separate file on windows, since we don't have a console there */
static FILE *log_file = 0;

static void write_log(__attribute__((unused)) void *userdata, __attribute__((unused)) int category, SDL_LogPriority priority, const char *message)
{
    if (log_file) {
        if (priority == SDL_LOG_PRIORITY_ERROR) {
            fwrite("ERROR: ", sizeof(char), 7, log_file);
        } else {
            fwrite("INFO: ", sizeof(char), 6, log_file);
        }
        fwrite(message, sizeof(char), strlen(message), log_file);
        fwrite("\n", sizeof(char), 1, log_file);
        fflush(log_file);
    }
}

static void setup_logging(void)
{
    log_file = fopen("brutus-log.txt", "wt");
    SDL_LogSetOutputFunction(write_log, NULL);
}

static void teardown_logging(void)
{
    if (log_file) {
        fclose(log_file);
    }
}

#else
static void setup_logging(void) {}
static void teardown_logging(void) {}
#endif

static void post_event(int code)
{
    SDL_Event event;
    event.user.type = SDL_USEREVENT;
    event.user.code = code;
    SDL_PushEvent(&event);
}

void system_exit(void)
{
    post_event(USER_EVENT_QUIT);
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

void system_center(void)
{
    post_event(USER_EVENT_CENTER_WINDOW);
}

void system_set_fullscreen(int fullscreen)
{
    post_event(fullscreen ? USER_EVENT_FULLSCREEN : USER_EVENT_WINDOWED);
}

#ifdef _WIN32
#define PLATFORM_ENABLE_PER_FRAME_CALLBACK
static void platform_per_frame_callback(void)
{
    platform_screen_recreate_texture();
}
#endif

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
    platform_screen_update();
    platform_screen_render();
}
#else
static void run_and_draw(void)
{
    time_set_millis(SDL_GetTicks());

    game_run();
    game_draw();

    platform_screen_update();
    platform_screen_render();
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
            platform_screen_resize(event->data1, event->data2);
            break;
        case SDL_WINDOWEVENT_RESIZED:
            SDL_Log("System resize to %d x %d", (int) event->data1, (int) event->data2);
            break;
        case SDL_WINDOWEVENT_MOVED:
            SDL_Log("Window move to coordinates x: %d y: %d\n", (int) event->data1, (int) event->data2);
            platform_screen_move(event->data1, event->data2);
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

static void handle_event(SDL_Event *event)
{
    switch (event->type) {
        case SDL_WINDOWEVENT:
            handle_window_event(&event->window, &data.active);
            break;
        case SDL_KEYDOWN:
            platform_handle_key_down(&event->key);
            break;
        case SDL_KEYUP:
            platform_handle_key_up(&event->key);
            break;
        case SDL_TEXTINPUT:
            platform_handle_text(&event->text);
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
                platform_screen_set_window_size(INTPTR(event->user.data1), INTPTR(event->user.data2));
            } else if (event->user.code == USER_EVENT_FULLSCREEN) {
                platform_screen_set_fullscreen();
            } else if (event->user.code == USER_EVENT_WINDOWED) {
                platform_screen_set_windowed();
            } else if (event->user.code == USER_EVENT_CENTER_WINDOW) {
                platform_screen_center_window();
            }
            break;

        default:
            break;
    }
}

static void teardown(void)
{
    SDL_Log("Exiting game");
    game_exit();
    platform_screen_destroy();
    SDL_Quit();
    teardown_logging();
}

static void main_loop(void)
{
    SDL_Event event;
#ifdef PLATFORM_ENABLE_PER_FRAME_CALLBACK
    platform_per_frame_callback();
#endif
    /* Process event queue */
    while (SDL_PollEvent(&event)) {
        handle_event(&event);
    }
    if (data.quit) {
        teardown();
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
#if SDL_VERSION_ATLEAST(2, 0, 10)
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS, "0");
#endif
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
        if (strlen(executable_path) < DIR_PATH_MAX - strlen("brutus.hconfigs")) {
            strncpy(EXECUTABLE_DIR_PATH, executable_path, DIR_PATH_MAX - 1);

            strncpy(DATA_TEXT_FILE_PATH, executable_path, DIR_PATH_MAX - 1);
            strcat(DATA_TEXT_FILE_PATH, "data_dir.txt");

            strncpy(SETTINGS_FILE_PATH, executable_path, DIR_PATH_MAX - 1);
            strcat(SETTINGS_FILE_PATH, "brutus.settings");

            strncpy(CONFIGS_FILE_PATH, executable_path, DIR_PATH_MAX - 1);
            strcat(CONFIGS_FILE_PATH, "brutus.configs");

            strncpy(HOTKEY_CONFIGS_FILE_PATH, executable_path, DIR_PATH_MAX - 1);
            strcat(HOTKEY_CONFIGS_FILE_PATH, "brutus.hconfigs");

            strncpy(MAPS_DIR_PATH, executable_path, DIR_PATH_MAX - 1);
            strcat(MAPS_DIR_PATH, "maps");
            strncpy(SAVES_DIR_PATH, executable_path, DIR_PATH_MAX - 1);
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
#ifdef _WIN32
        _mkdir(MAPS_DIR_PATH);
        _mkdir(SAVES_DIR_PATH);
#elif(__linux__)
        mkdir(MAPS_DIR_PATH, 0700);
        mkdir(SAVES_DIR_PATH, 0700);
#endif
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

static void setup(struct brutus_args_t *args)
{
    signal(SIGSEGV, handler);
    setup_logging();

    SDL_Log("Brutus version %s", system_version());

    if (!init_sdl()) {
        SDL_Log("Exiting: SDL init failed");
        exit_with_status(-1);
    }

    if (!pre_init()) {
        SDL_Log("Exiting: game pre-init failed");
        exit_with_status(1);
    }

    if (args->force_windowed && setting_fullscreen()) {
        int w, h;
        setting_window(&w, &h);
        setting_set_display(0, w, h);
        SDL_Log("Forcing windowed mode with size %d x %d", w, h);
    }

    // handle arguments
    if (args->display_scale_percentage) {
        config_set(CONFIG_SCREEN_DISPLAY_SCALE, args->display_scale_percentage);
    }
    if (args->cursor_scale_percentage) {
        config_set(CONFIG_SCREEN_CURSOR_SCALE, args->cursor_scale_percentage);
    }

    char title[100];
    encoding_to_utf8(lang_get_string(9, 0), title, 100, 0);
    if (!platform_screen_create(title, config_get(CONFIG_SCREEN_DISPLAY_SCALE))) {
        SDL_Log("Exiting: SDL create window failed");
        exit_with_status(-2);
    }
    // this has to come after platform_screen_create, otherwise it fails on Nintendo Switch
    system_init_cursors(config_get(CONFIG_SCREEN_CURSOR_SCALE));

#ifdef PLATFORM_ENABLE_INIT_CALLBACK
    platform_init_callback();
#endif

    time_set_millis(SDL_GetTicks());

    if (!game_init()) {
        SDL_Log("Exiting: game init failed");
        exit_with_status(2);
    }

    data.quit = 0;
    data.active = 1;
}

int main(int argc, char **argv)
{
    struct brutus_args_t args;
    platform_parse_arguments(argc, argv, &args);

    setup(&args);

    mouse_set_inside_window(1);
    run_and_draw();

    while (!data.quit) {
        main_loop();
    }

    return 0;
}
