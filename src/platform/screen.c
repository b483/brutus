#include "platform/screen.h"

#include "core/calc.h"
#include "core/config.h"
#include "game/settings.h"
#include "game/system.h"
#include "graphics/graphics.h"
#include "graphics/screen.h"
#include "input/cursor.h"
#include "platform/cursor.h"

#include "SDL.h"

#include <stdlib.h>

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
static color_t *framebuffer;

static int scale_logical_to_pixels(int logical_value)
{
    return logical_value * scale_percentage / 100;
}

static int scale_pixels_to_logical(int pixel_value)
{
    return pixel_value * 100 / scale_percentage;
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
#ifndef __APPLE__
    // Scale using nearest neighbour when we scale a multiple of 100%: makes it look sharper.
    // But not on MacOS: users are used to the linear interpolation since that's what Apple also does.
    if (scale_percentage % 100 == 0) {
        scale_quality = "nearest";
    }
#endif
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, scale_quality);
}

int platform_screen_create(const char *title, int display_scale_percentage)
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

    platform_screen_destroy();

    SDL_Log("Creating screen %d x %d, %s, driver: %s", width, height,
        fullscreen ? "fullscreen" : "windowed", SDL_GetCurrentVideoDriver());
    Uint32 flags = SDL_WINDOW_RESIZABLE;

#if SDL_VERSION_ATLEAST(2, 0, 1)
    flags |= SDL_WINDOW_ALLOW_HIGHDPI;
#endif

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

#if !defined(__APPLE__)
    if (fullscreen && SDL_GetNumVideoDisplays() > 1) {
        SDL_SetWindowGrab(SDL.window, SDL_TRUE);
    }
#endif

    set_scale_percentage(display_scale_percentage, width, height);
    return platform_screen_resize(width, height);
}

static void destroy_screen_texture(void)
{
    SDL_DestroyTexture(SDL.texture);
    SDL.texture = 0;
}

void platform_screen_destroy(void)
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

int platform_screen_resize(int pixel_width, int pixel_height)
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

int system_scale_display(int display_scale_percentage)
{
    int width, height;
    SDL_GetWindowSize(SDL.window, &width, &height);
    set_scale_percentage(display_scale_percentage, width, height);
    platform_screen_resize(width, height);
    return scale_percentage;
}

int system_get_max_display_scale(void)
{
    int width, height;
    SDL_GetWindowSize(SDL.window, &width, &height);
    return get_max_scale_percentage(width, height);
}

void platform_screen_move(int x, int y)
{
    if (!setting_fullscreen()) {
        window_pos.x = x;
        window_pos.y = y;
        window_pos.centered = 0;
    }
}

void platform_screen_set_fullscreen(void)
{
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

#if !defined(__APPLE__)
    if (SDL_GetNumVideoDisplays() > 1) {
        SDL_SetWindowGrab(SDL.window, SDL_TRUE);
    }
#endif
    setting_set_display(1, mode.w, mode.h);
}

void platform_screen_set_windowed(void)
{
    int logical_width, logical_height;
    setting_window(&logical_width, &logical_height);
    int pixel_width = scale_logical_to_pixels(logical_width);
    int pixel_height = scale_logical_to_pixels(logical_height);
    int display = SDL_GetWindowDisplayIndex(SDL.window);
    SDL_Log("User to windowed %d x %d on display %d", pixel_width, pixel_height, display);
    SDL_SetWindowFullscreen(SDL.window, 0);
    SDL_SetWindowSize(SDL.window, pixel_width, pixel_height);
    if (window_pos.centered) {
        platform_screen_center_window();
    }
    if (SDL_GetWindowGrab(SDL.window) == SDL_TRUE) {
        SDL_SetWindowGrab(SDL.window, SDL_FALSE);
    }
    setting_set_display(0, pixel_width, pixel_height);
}

void platform_screen_set_window_size(int logical_width, int logical_height)
{
    int pixel_width = scale_logical_to_pixels(logical_width);
    int pixel_height = scale_logical_to_pixels(logical_height);
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
        platform_screen_center_window();
    }
    SDL_Log("User resize to %d x %d on display %d", pixel_width, pixel_height, display);
    if (SDL_GetWindowGrab(SDL.window) == SDL_TRUE) {
        SDL_SetWindowGrab(SDL.window, SDL_FALSE);
    }
    setting_set_display(0, pixel_width, pixel_height);
}

void platform_screen_center_window(void)
{
    int display = SDL_GetWindowDisplayIndex(SDL.window);
    SDL_SetWindowPosition(SDL.window,
        SDL_WINDOWPOS_CENTERED_DISPLAY(display), SDL_WINDOWPOS_CENTERED_DISPLAY(display));
    window_pos.centered = 1;
}

#ifdef _WIN32
void platform_screen_recreate_texture(void)
{
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
}
#endif

void platform_screen_clear(void)
{
    SDL_RenderClear(SDL.renderer);
}

void platform_screen_update(void)
{
    SDL_RenderClear(SDL.renderer);
    SDL_UpdateTexture(SDL.texture, NULL, graphics_canvas(), screen_width() * 4);
    SDL_RenderCopy(SDL.renderer, SDL.texture, NULL, NULL);
}

void platform_screen_render(void)
{
    SDL_RenderPresent(SDL.renderer);
}

void platform_screen_generate_mouse_cursor_texture(int cursor_id, int scale, const color_t *cursor_colors)
{
    if (SDL.cursors[cursor_id]) {
        SDL_DestroyTexture(SDL.cursors[cursor_id]);
        SDL.cursors[cursor_id] = 0;
    }
    const cursor *c = input_cursor_data(cursor_id, scale);
    int size = platform_cursor_get_texture_size(c->width, c->height);
    SDL.cursors[cursor_id] = SDL_CreateTexture(SDL.renderer,
        SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC,
        size, size);
    if (!SDL.cursors[cursor_id]) {
        return;
    }
    SDL_UpdateTexture(SDL.cursors[cursor_id], NULL, cursor_colors, size * sizeof(color_t));
    SDL_SetTextureBlendMode(SDL.cursors[cursor_id], SDL_BLENDMODE_BLEND);
}

void system_set_mouse_position(int *x, int *y)
{
    *x = calc_bound(*x, 0, screen_width() - 1);
    *y = calc_bound(*y, 0, screen_height() - 1);
    SDL_WarpMouseInWindow(SDL.window, scale_logical_to_pixels(*x), scale_logical_to_pixels(*y));
}

color_t *system_create_framebuffer(int width, int height)
{
    free(framebuffer);
    framebuffer = (color_t *) malloc((size_t) width * height * sizeof(color_t));
    return framebuffer;
}
