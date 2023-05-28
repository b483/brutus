#include "config.h"

#include "core/file.h"
#include "core/log.h"
#include "scenario/scenario.h"

#include <stdio.h>
#include <string.h>

#define MAX_LINE 100

// Keep this in the same order as the config_keys in config.h
static const char *ini_keys[] = {
    "screen_display_scale",
    "screen_cursor_scale",
    "ui_sidebar_info",
    "ui_show_intro_video",
    "ui_disable_mouse_edge_scrolling",
    "ui_disable_map_drag",
    "ui_visual_feedback_on_delete",
    "ui_highlight_legions",
};

static const char *ini_string_keys[] = {
    "player_name",
};

static int values[CONFIG_MAX_ENTRIES];
static char string_values[CONFIG_STRING_MAX_ENTRIES][CONFIG_STRING_VALUE_MAX];

static int default_values[CONFIG_MAX_ENTRIES] = {
    [CONFIG_SCREEN_DISPLAY_SCALE] = 100,
    [CONFIG_SCREEN_CURSOR_SCALE] = 100
};
static const char default_string_values[CONFIG_STRING_MAX_ENTRIES][CONFIG_STRING_VALUE_MAX];

int config_get(config_key key)
{
    return values[key];
}

void config_set(config_key key, int value)
{
    values[key] = value;
}

const char *config_get_string(config_string_key key)
{
    return string_values[key];
}

void config_set_string(config_string_key key, const char *value)
{
    if (!value) {
        string_values[key][0] = 0;
    } else {
        strncpy(string_values[key], value, CONFIG_STRING_VALUE_MAX - 1);
    }
}

int config_get_default_value(config_key key)
{
    return default_values[key];
}

const char *config_get_default_string_value(config_string_key key)
{
    return default_string_values[key];
}

void set_player_name_from_config(void)
{
    scenario_settings_set_player_name((const uint8_t *) string_values[CONFIG_STRING_PLAYER_NAME]);
}

static void set_defaults(void)
{
    for (int i = 0; i < CONFIG_MAX_ENTRIES; ++i) {
        values[i] = default_values[i];
    }
    strncpy(string_values[CONFIG_STRING_PLAYER_NAME], "BRUTUS", CONFIG_STRING_VALUE_MAX - 1);
    set_player_name_from_config();
}

void config_load(void)
{
    set_defaults();
    FILE *fp = fopen(CONFIGS_FILE_PATH, "rt");
    if (!fp) {
        return;
    }
    char line_buffer[MAX_LINE];
    char *line;
    while ((line = fgets(line_buffer, MAX_LINE, fp))) {
        // Remove newline from string
        size_t size = strlen(line);
        while (size > 0 && (line[size - 1] == '\n' || line[size - 1] == '\r')) {
            line[--size] = 0;
        }
        char *equals = strchr(line, '=');
        if (equals) {
            *equals = 0;
            for (int i = 0; i < CONFIG_MAX_ENTRIES; i++) {
                if (strcmp(ini_keys[i], line) == 0) {
                    int value = atoi(&equals[1]);
                    log_info("Config key", ini_keys[i], value);
                    values[i] = value;
                    break;
                }
            }
            for (int i = 0; i < CONFIG_STRING_MAX_ENTRIES; i++) {
                if (strcmp(ini_string_keys[i], line) == 0) {
                    const char *value = &equals[1];
                    log_info("Config key", ini_string_keys[i], 0);
                    log_info("Config value", value, 0);
                    strncpy(string_values[i], value, CONFIG_STRING_VALUE_MAX - 1);
                    break;
                }
            }
        }
    }
    fclose(fp);

    set_player_name_from_config();
}

void config_save(void)
{
    FILE *fp = fopen(CONFIGS_FILE_PATH, "wt");
    if (!fp) {
        log_error("Unable to write configuration file", CONFIGS_FILE_PATH, 0);
        return;
    }
    for (int i = 0; i < CONFIG_MAX_ENTRIES; i++) {
        fprintf(fp, "%s=%d\n", ini_keys[i], values[i]);
    }
    for (int i = 0; i < CONFIG_STRING_MAX_ENTRIES; i++) {
        fprintf(fp, "%s=%s\n", ini_string_keys[i], string_values[i]);
    }
    fclose(fp);
}
