#pragma once

#include <slash/slash.h>

#define SLASH_STATUSLINE_MAX_ITEMS  16
#define SLASH_STATUSLINE_KEY_SIZE   32
#define SLASH_STATUSLINE_TEXT_SIZE  128

typedef enum {
    SLASH_STATUS_NORMAL = 0,
    SLASH_STATUS_WARNING,
    SLASH_STATUS_ERROR
} slash_status_type_t;

typedef struct {
    slash_status_type_t type;
    //int timeout_sec;
    //bool blink;
} slash_status_opts_t;

/**
 * @brief Set or update a statusline item by key with formatting and options.
 * If key already exists, update its text and options. Otherwise add a new item.
 *
 * @param key        Identifier string (max 31 chars).
 * @param opts_brace Configuration options wrapped in curly braces (e.g., { .type = SLASH_STATUS_ERROR }).
 * Pass {0} to use default options (NORMAL type, no timeout).
 * @param format     Printf-style format string for the display text (rendered max 127 chars).
 * @param ...        Variadic arguments matching the format string.
 * @return 0 on success, -1 if no space available.
 * * @note This is a macro that wraps slash_statusline_set_impl to allow inline struct initialization.
 */
int _slash_statusline_set_impl(const char *key, const slash_status_opts_t *opts, const char *format, ...) __attribute__((format(printf, 3, 4)));

#define slash_statusline_set(key, opts_brace, ...) \
    _slash_statusline_set_impl(key, &(slash_status_opts_t)opts_brace, __VA_ARGS__)

/**
 * @brief Remove a statusline item by key.
 * @param key   Identifier string to remove
 * @return 0 on success, -1 if key not found
 */
int slash_statusline_remove(const char *key);

/**
 * @brief Get the number of active statusline items.
 * @return count of active items
 */
int slash_statusline_count(void);

/**
 * @brief Render the statusline content to the terminal.
 *        Called internally by slash_refresh(). Not normally called by users.
 * @param slash  Slash context (for slash_write)
 */
void slash_statusline_render(struct slash *slash);
