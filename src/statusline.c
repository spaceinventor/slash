#include <slash/statusline.h>

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdarg.h>

// Base theme: White text on Orange background
#define STATUS_COLOR_NORMAL "\033[0;38;5;255;48;5;202m"

// Warning: Bold Yellow text on Orange background
#define STATUS_COLOR_WARNING "\033[1;38;5;11;48;5;202m"

// Error: Bold White text on Red background
#define STATUS_COLOR_ERROR "\033[1;38;5;15;48;5;196m"

struct slash_statusline_item {
    char key[SLASH_STATUSLINE_KEY_SIZE];
    char text[SLASH_STATUSLINE_TEXT_SIZE];
    bool active;
    slash_status_type_t type;
};

static struct slash_statusline_item items[SLASH_STATUSLINE_MAX_ITEMS];
static int item_count = 0;

int _slash_statusline_set_impl(const char * key, const slash_status_opts_t * opts, const char * format, ...) {
    va_list args;

    // Search for existing key
    for (int i = 0; i < SLASH_STATUSLINE_MAX_ITEMS; i++) {
        if (items[i].active && strncmp(items[i].key, key, SLASH_STATUSLINE_KEY_SIZE) == 0) {

            items[i].type = opts->type;

            va_start(args, format);
            vsnprintf(items[i].text, SLASH_STATUSLINE_TEXT_SIZE, format, args);
            va_end(args);
            return 0;
        }
    }

    // Find empty slot
    for (int i = 0; i < SLASH_STATUSLINE_MAX_ITEMS; i++) {
        if (!items[i].active) {
            strncpy(items[i].key, key, SLASH_STATUSLINE_KEY_SIZE - 1);
            items[i].key[SLASH_STATUSLINE_KEY_SIZE - 1] = '\0';

            items[i].type = opts->type;

            va_start(args, format);
            vsnprintf(items[i].text, SLASH_STATUSLINE_TEXT_SIZE, format, args);
            va_end(args);

            items[i].active = true;
            item_count++;
            return 0;
        }
    }

    return -1;
}

int slash_statusline_remove(const char * key) {
    for (int i = 0; i < SLASH_STATUSLINE_MAX_ITEMS; i++) {
        if (items[i].active && strncmp(items[i].key, key, SLASH_STATUSLINE_KEY_SIZE) == 0) {
            items[i].active = false;
            items[i].key[0] = '\0';
            items[i].text[0] = '\0';
            item_count--;
            return 0;
        }
    }
    return -1;
}

int slash_statusline_count(void) {
    return item_count;
}

void slash_statusline_render(struct slash * slash) {
    if (item_count == 0) {
        slash_write(slash, "\033[K", 3);
        return;
    }

    slash_write(slash, STATUS_COLOR_NORMAL, strlen(STATUS_COLOR_NORMAL));

    bool first = true;
    for (int i = 0; i < SLASH_STATUSLINE_MAX_ITEMS; i++) {
        if (!items[i].active)
            continue;

        if (!first) {
            slash_write(slash, " \xe2\x94\x83 ", 5);
        } else {
            slash_write(slash, " ", 1);
        }

        if (items[i].type == SLASH_STATUS_WARNING) {
            slash_write(slash, STATUS_COLOR_WARNING, strlen(STATUS_COLOR_WARNING));
        } else if (items[i].type == SLASH_STATUS_ERROR) {
            slash_write(slash, STATUS_COLOR_ERROR, strlen(STATUS_COLOR_ERROR));
        }

        slash_write(slash, items[i].text, strlen(items[i].text));

        if (items[i].type != SLASH_STATUS_NORMAL) {
            slash_write(slash, STATUS_COLOR_NORMAL, strlen(STATUS_COLOR_NORMAL));
        }

        first = false;
    }

    // Fill rest of line with background color then hard reset
    slash_write(slash, " \033[K\033[0m", 8);
}
