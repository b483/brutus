#include "sound_options.h"

#include "game/settings.h"
#include "graphics/graphics.h"
#include "editor/editor.h"
#include "sound/sound.h"
#include "window/city.h"

#include "SDL_mixer.h"

static void button_toggle(int type, int param2);

static void arrow_button_music(int is_down, int param2);
static void arrow_button_speech(int is_down, int param2);
static void arrow_button_effects(int is_down, int param2);
static void arrow_button_city(int is_down, int param2);

static struct generic_button_t buttons_sound_options[] = {
    {64, 162, 224, 20, button_toggle, button_none, SOUND_MUSIC, 0},
    {64, 192, 224, 20, button_toggle, button_none, SOUND_SPEECH, 0},
    {64, 222, 224, 20, button_toggle, button_none, SOUND_EFFECTS, 0},
    {64, 252, 224, 20, button_toggle, button_none, SOUND_CITY, 0},
};

static struct arrow_button_t arrow_buttons_sound_options[] = {
    {112, 100, 17, 24, arrow_button_music, 1, 0, 0, 0},
    {136, 100, 15, 24, arrow_button_music, 0, 0, 0, 0},
    {112, 130, 17, 24, arrow_button_speech, 1, 0, 0, 0},
    {136, 130, 15, 24, arrow_button_speech, 0, 0, 0, 0},
    {112, 160, 17, 24, arrow_button_effects, 1, 0, 0, 0},
    {136, 160, 15, 24, arrow_button_effects, 0, 0, 0, 0},
    {112, 190, 17, 24, arrow_button_city, 1, 0, 0, 0},
    {136, 190, 15, 24, arrow_button_city, 0, 0, 0, 0},
};

static struct {
    int focus_button_id;
    int from_editor;
    struct set_sound_t original_effects;
    struct set_sound_t original_music;
    struct set_sound_t original_speech;
    struct set_sound_t original_city;
} data;

static void init(int from_editor)
{
    data.focus_button_id = 0;
    data.from_editor = from_editor;

    data.original_effects = *get_sound(SOUND_EFFECTS);
    data.original_music = *get_sound(SOUND_MUSIC);
    data.original_speech = *get_sound(SOUND_SPEECH);
    data.original_city = *get_sound(SOUND_CITY);
}

static void draw_foreground(void)
{
    graphics_in_dialog();

    outer_panel_draw(48, 80, 24, 15);

    // on/off labels
    label_draw(64, 162, 14, data.focus_button_id == 1 ? 1 : 2);
    label_draw(64, 192, 14, data.focus_button_id == 2 ? 1 : 2);
    label_draw(64, 222, 14, data.focus_button_id == 3 ? 1 : 2);
    label_draw(64, 252, 14, data.focus_button_id == 4 ? 1 : 2);

    // title
    lang_text_draw_centered(46, 0, 96, 92, 288, FONT_LARGE_BLACK);

    lang_text_draw(46, 10, 112, 142, FONT_SMALL_PLAIN);
    lang_text_draw(46, 11, 336, 142, FONT_SMALL_PLAIN);

    struct set_sound_t *music = get_sound(SOUND_MUSIC);
    lang_text_draw_centered(46, music->enabled ? 2 : 1, 64, 166, 224, FONT_NORMAL_GREEN);
    text_draw_percentage(music->volume, 374, 166, FONT_NORMAL_PLAIN);

    struct set_sound_t *speech = get_sound(SOUND_SPEECH);
    lang_text_draw_centered(46, speech->enabled ? 4 : 3, 64, 196, 224, FONT_NORMAL_GREEN);
    text_draw_percentage(speech->volume, 374, 196, FONT_NORMAL_PLAIN);

    struct set_sound_t *effects = get_sound(SOUND_EFFECTS);
    lang_text_draw_centered(46, effects->enabled ? 6 : 5, 64, 226, 224, FONT_NORMAL_GREEN);
    text_draw_percentage(effects->volume, 374, 226, FONT_NORMAL_PLAIN);

    struct set_sound_t *city = get_sound(SOUND_CITY);
    lang_text_draw_centered(46, city->enabled ? 8 : 7, 64, 256, 224, FONT_NORMAL_GREEN);
    text_draw_percentage(city->volume, 374, 256, FONT_NORMAL_PLAIN);

    arrow_buttons_draw(208, 60, arrow_buttons_sound_options, sizeof(arrow_buttons_sound_options) / sizeof(struct arrow_button_t));

    graphics_reset_dialog();
}

static void handle_input(const struct mouse_t *m, const struct hotkeys_t *h)
{
    const struct mouse_t *m_dialog = mouse_in_dialog(m);
    if (generic_buttons_handle_mouse(m_dialog, 0, 0, buttons_sound_options, 6, &data.focus_button_id) ||
        arrow_buttons_handle_mouse(m_dialog, 208, 60, arrow_buttons_sound_options, sizeof(arrow_buttons_sound_options) / sizeof(struct arrow_button_t), 0)) {
        return;
    }
    if (m->right.went_up || h->escape_pressed) {
        if (data.from_editor) {
            show_editor_map();
        } else {
            window_city_return();
        }
    }
}

static void button_toggle(int type, __attribute__((unused)) int param2)
{
    setting_toggle_sound_enabled(type);
    if (type == SOUND_MUSIC) {
        if (get_sound(SOUND_MUSIC)->enabled) {
            update_music(1);
        } else {
            stop_music();
        }
    } else if (type == SOUND_SPEECH) {
        if (!get_sound(SOUND_SPEECH)->enabled) {
            stop_sound_channel(SOUND_CHANNEL_SPEECH);
        }
    }
}

static void update_volume(int type, int is_decrease)
{
    if (is_decrease) {
        setting_decrease_sound_volume(type);
    } else {
        setting_increase_sound_volume(type);
    }
}

static void arrow_button_music(int is_down, __attribute__((unused)) int param2)
{
    update_volume(SOUND_MUSIC, is_down);
    Mix_VolumeMusic(percentage_to_volume(get_sound(SOUND_MUSIC)->volume));
}

static void arrow_button_speech(int is_down, __attribute__((unused)) int param2)
{
    update_volume(SOUND_SPEECH, is_down);
    set_channel_volume(SOUND_CHANNEL_SPEECH, get_sound(SOUND_SPEECH)->volume);
}

static void arrow_button_effects(int is_down, __attribute__((unused)) int param2)
{
    update_volume(SOUND_EFFECTS, is_down);
    set_sound_effect_volume(get_sound(SOUND_EFFECTS)->volume);
}

static void arrow_button_city(int is_down, __attribute__((unused)) int param2)
{
    update_volume(SOUND_CITY, is_down);
    set_city_sounds_volume(get_sound(SOUND_CITY)->volume);
}

void window_sound_options_show(int from_editor)
{
    struct window_type_t window = {
        WINDOW_SOUND_OPTIONS,
        window_draw_underlying_window,
        draw_foreground,
        handle_input,
    };
    init(from_editor);
    window_show(&window);
}
