/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2014-2016 Satlab ApS <satlab@satlab.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <slash/slash.h>
#include <slash/optparse.h>
#include <slash/completer.h>

#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <time.h>
#include <sys/time.h>

#ifdef SLASH_HAVE_TERMIOS_H
#include <termios.h>
#endif

#ifdef SLASH_HAVE_SELECT
#include <sys/select.h>
#endif

#include "builtins.h"

/* Terminal codes */
#define ESC '\x1b'
#define DEL '\x7f'

#define CONTROL(code) (code - '@')
#define ESCAPE(code) "\x1b[0" code
#define ESCAPE_NUM(code) "\x1b[%u" code

/* Command-line option parsing */
int slash_getopt(struct slash *slash, const char *opts)
{
	/* From "public domain AT&T getopt source" newsgroup posting */
	int c;
	char *cp;

	if (slash->sp == 1) {
		if (slash->optind >= slash->argc ||
		    slash->argv[slash->optind][0] != '-' ||
		    slash->argv[slash->optind][1] == '\0') {
			return EOF;
		} else if (!strcmp(slash->argv[slash->optind], "--")) {
			slash->optind++;
			return EOF;
		}
	}

	slash->optopt = c = slash->argv[slash->optind][slash->sp];

	if (c == ':' || (cp = strchr(opts, c)) == NULL) {
		slash_printf(slash, "Unknown option -%c\n", c);
		if (slash->argv[slash->optind][++(slash->sp)] == '\0') {
			slash->optind++;
			slash->sp = 1;
		}
		return '?';
	}

	if (*(++cp) == ':') {
		if (slash->argv[slash->optind][slash->sp+1] != '\0') {
			slash->optarg = &slash->argv[(slash->optind)++][slash->sp+1];
		} else if(++(slash->optind) >= slash->argc) {
			slash_printf(slash, "Option -%c requires an argument\n", c);
			slash->sp = 1;
			return '?';
		} else {
			slash->optarg = slash->argv[(slash->optind)++];
		}
		slash->sp = 1;
	} else {
		if (slash->argv[slash->optind][++(slash->sp)] == '\0') {
			slash->sp = 1;
			slash->optind++;
		}
		slash->optarg = NULL;
	}

	return c;
}

static int slash_rawmode_enable(struct slash *slash)
{
#ifdef SLASH_HAVE_TERMIOS_H
	struct termios raw;

	if (tcgetattr(slash->fd_read, &slash->original) < 0)
		return -ENOTTY;

	raw = slash->original;
	raw.c_cflag |= (CS8);
	raw.c_lflag &= ~(ECHO | ICANON | IEXTEN);
	raw.c_cc[VMIN] = 1;
	raw.c_cc[VTIME] = 0;

	if (tcsetattr(slash->fd_read, TCSANOW, &raw) < 0)
		return -ENOTTY;
#endif
	return 0;
}

static int slash_rawmode_disable(struct slash *slash)
{
#ifdef SLASH_HAVE_TERMIOS_H
	if (tcsetattr(slash->fd_read, TCSANOW, &slash->original) < 0)
		return -ENOTTY;
#endif
	return 0;
}

static int slash_configure_term(struct slash *slash)
{
	if (slash_rawmode_enable(slash) < 0)
		return -ENOTTY;

	return 0;
}

static int slash_restore_term(struct slash *slash)
{
	if (slash_rawmode_disable(slash) < 0)
		return -ENOTTY;

	return 0;
}

static int slash_wait_select(void *slashp, unsigned int ms);
void slash_acquire_std_in_out(struct slash *slash) {	
	slash_configure_term(slash);
#ifdef SLASH_HAVE_SELECT
	slash->waitfunc = slash_wait_select;
#endif
}

void slash_release_std_in_out(struct slash *slash) {	
	slash_restore_term(slash);
#ifdef SLASH_HAVE_SELECT
	slash->waitfunc = NULL;
#endif
}

int slash_write(struct slash *slash, const char *buf, size_t count)
{
	return write(slash->fd_write, buf, count);
}

static int slash_read(struct slash *slash, void *buf, size_t count)
{
	return read(slash->fd_read, buf, count);
}

int slash_putchar(struct slash *slash, char c)
{
	return slash_write(slash, &c, 1);
}

static int slash_getchar(struct slash *slash)
{
	unsigned char c;

	if (slash_read(slash, &c, 1) < 1) {
		return -EIO;
	}

	return c;
}

#ifdef SLASH_HAVE_SELECT
static int slash_wait_select(void *slashp, unsigned int ms)
{
	int ret = 0;
	char c;
	fd_set fds;
	struct timeval timeout;
	struct slash * slash = (struct slash *) slashp;

	timeout.tv_sec = ms / 1000;
	timeout.tv_usec = (ms % 1000) * 1000;

	FD_ZERO(&fds);
	FD_SET(slash->fd_read, &fds);

	fcntl(slash->fd_read, F_SETFL, fcntl(slash->fd_read, F_GETFL) |  O_NONBLOCK);

	ret = select(1, &fds, NULL, NULL, &timeout);
	if (ret == 1) {
		ret = -EINTR;
		slash_read(slash, &c, 1);
	}

	fcntl(slash->fd_read, F_SETFL, fcntl(slash->fd_read, F_GETFL) & ~O_NONBLOCK);

	return ret;
}
#endif

int slash_set_wait_interruptible(struct slash *slash, slash_waitfunc_t waitfunc)
{
	slash->waitfunc = waitfunc;
	return 0;
}

int slash_wait_interruptible(struct slash *slash, unsigned int ms)
{
	if (slash->waitfunc)
		return slash->waitfunc(slash, ms);

	return -ENOSYS;
}

int slash_printf(struct slash *slash, const char *format, ...)
{
	(void)slash;
	int ret;
	va_list args;

	va_start(args, format);

	ret = vprintf(format, args);

	va_end(args);

	return ret;
}

void slash_bell(struct slash *slash)
{
	slash_putchar(slash, '\a');
}

static bool slash_line_empty(char *line, size_t linelen)
{
	while (*line && linelen--)
		if (!isspace((unsigned int) *line++))
			return false;

	return true;
}


struct slash_command *
slash_command_find(struct slash *slash, char *line, size_t linelen, char **args)
{
	(void)slash;
	/* Maximum length match */
	size_t max_matchlen = 0;
	struct slash_command *max_match_cmd = NULL;

	struct slash_command * cmd;
	slash_list_iterator i = {};
	size_t cmd_length;
	while ((cmd = slash_list_iterate(&i)) != NULL) {
		cmd_length = strlen(cmd->name);
		/* Find an exact match */
		if (strncmp(line, cmd->name, cmd_length) == 0) {
			/* Now, let's see if it is an actual exact match, or simply a prefix match 
			as a prefix match can lead to "listttt" being interpreted as the valid "list" command
			*/
			if (linelen > cmd_length) {
				/* If the input > match, check that the next character is a separator */
				if(line[cmd_length] != ' ') {
					continue;
				}
			}
		} else {
			continue;
		}

		/* Update the max-length match */
		if (cmd_length > max_matchlen) {
			max_match_cmd = cmd;
			max_matchlen = cmd_length;

			/* Calculate arguments position */
			*args = line + cmd_length;
		}
	}

	return max_match_cmd;
}

int slash_build_args(char *args, char **argv, int *argc)
{
	/* Quote level */
	enum {
		SLASH_QUOTE_NONE,
		SLASH_QUOTE_SINGLE,
		SLASH_QUOTE_DOUBLE,
	} quote = SLASH_QUOTE_NONE;

	*argc = 0;

	while (*args && *argc < SLASH_ARG_MAX) {
		/* Check for quotes */
		if (*args == '\'') {
			quote = SLASH_QUOTE_SINGLE;
			args++;
		} else if (*args == '\"') {
			quote = SLASH_QUOTE_DOUBLE;
			args++;
		}

		/* Argument starts here */
		argv[(*argc)++] = args;

		/* Loop over input argument */
		while (*args) {
			if (quote == SLASH_QUOTE_SINGLE && *args == '\'') {
				quote = SLASH_QUOTE_NONE;
				break;
			} else if (quote == SLASH_QUOTE_DOUBLE &&
				   *args == '\"') {
				quote = SLASH_QUOTE_NONE;
				break;
			} else if (quote == SLASH_QUOTE_NONE && *args == ' ') {
				break;
			}

			args++;
		}

		/* End argument with zero byte */
		if (*args)
			*args++ = '\0';

		/* Skip trailing white space */
		while (*args && *args == ' ')
			args++;
	}

	/* Test for quote mismatch */
	if (quote != SLASH_QUOTE_NONE)
		return -1;

	/* According to C11 section 5.1.2.2.1, argv[argc] must be NULL */
	argv[*argc] = NULL;

	return 0;
}

void slash_command_usage(struct slash *slash, struct slash_command *command)
{
	const char *args = command->args ? command->args : "";
	const char *type = command->func ? "usage" : "group";
	const char *help = command->help;
	if(NULL != help) {
		slash_printf(slash, "%s: %s %s\n\n%s\n\n", type, command->name, args, help);
	} else {
		slash_printf(slash, "%s: %s %s\n", type, command->name, args);
	}
}

void slash_command_description(struct slash *slash, struct slash_command *command)
{
	slash_printf(slash, "%-15s\r\n", command->name);
}


/* Implement this function to perform logging for example */
__attribute__((weak)) void slash_on_execute_hook(const char *line) {
	(void)line;
}

__attribute__((weak)) void slash_on_execute_post_hook(const char *line, struct slash_command *command) {
	(void)line;
	(void)command;
}

/* A default no-prompt implementation is provided as a __attribute__((weak)) */
__attribute__((weak)) int slash_prompt(struct slash *slash) {
	(void)slash;
	return 0;
}

slash_process_cmd_line_hook_t slash_process_cmd_line_hook  = NULL;

int slash_execute(struct slash *slash, char *line)
{
	struct slash_command *command;
	char *args, *argv[SLASH_ARG_MAX];
	char *processed_cmd_line = NULL, *line_to_use;
	int ret, argc = 0;

	slash->busy = 1;
	slash->signal = 0;

	/* Skip comments */
	if (line[0] == '#') {
		return SLASH_SUCCESS;
	}

	if(NULL != slash_process_cmd_line_hook) {
		processed_cmd_line = slash_process_cmd_line_hook(line);
	}

	if (processed_cmd_line != NULL) {
		line_to_use = processed_cmd_line;
	} else {
		line_to_use = line;
	}

	command = slash_command_find(slash, line_to_use, strlen(line_to_use), &args);
	if (!command) {
		/* Print the original line here, not the possibly processed one */
		slash_printf(slash, "No such command: %s\n", line);
		/* Yes, processed_cmd_line maybe NULL, but the man page says it's ok, so we save an "if" statement */
		free(processed_cmd_line);
		return -ENOENT;
	}

	/* Implement this function to perform logging for example */
	slash_on_execute_hook(line);

	if (!command->func) {
		/* Yes, processed_cmd_line maybe NULL, but the free() man page says it's ok, so we save an "if" statement */
		free(processed_cmd_line);
		return -EINVAL;
	}

	/* Build args */
	if (slash_build_args(args, argv, &argc) < 0) {
		slash_printf(slash, "Mismatched quotes\n");
		/* Yes, processed_cmd_line maybe NULL, but the free() man page says it's ok, so we save an "if" statement */
		free(processed_cmd_line);
		return -EINVAL;
	}

	/* Reset state for slash_getopt */
	slash->optarg = 0;
	slash->optind = 1;
	slash->opterr = 1;
	slash->optopt = '?';
	slash->sp = 1;

	slash->argc = argc;
	slash->argv = argv;
	if (command->context) {
		/* If the user has attached context to the command,
			we assume they also specified a function which can accept it. */
		ret = command->func_ctx(slash, command->context);
	} else {
		/* Otherwise call the traditional (`slash_command()` macro) function without context. */
		ret = command->func(slash);
	}

	if (ret == SLASH_EUSAGE)
		slash_command_usage(slash, command);

	slash_on_execute_post_hook(line, command);

	/* Yes, processed_cmd_line maybe NULL, but the free() man page says it's ok, so we save an "if" statement */
	free(processed_cmd_line);

	slash->busy = 0;

	return ret;
}

/* History */
char *slash_history_increment(struct slash *slash, char *ptr)
{
	if (++ptr > &slash->history[slash->history_size-1])
		ptr = slash->history;
	return ptr;
}

static char *slash_history_decrement(struct slash *slash, char *ptr)
{
	if (--ptr < slash->history)
		ptr = &slash->history[slash->history_size-1];
	return ptr;
}

static void slash_history_push_head(struct slash *slash)
{
	*slash->history_head = '\0';
	slash->history_head = slash_history_increment(slash, slash->history_head);
	slash->history_avail++;
}

static void slash_history_push_tail(struct slash *slash, char c)
{
	*slash->history_tail = c;
	slash->history_tail = slash_history_increment(slash, slash->history_tail);
	slash->history_avail--;
}

static void slash_history_pull_tail(struct slash *slash)
{
	*slash->history_tail = '\0';
	slash->history_tail = slash_history_decrement(slash, slash->history_tail);
	slash->history_avail++;
}

static size_t slash_history_strlen(struct slash *slash, char *ptr)
{
	size_t len = 0;

	while (*ptr) {
		ptr = slash_history_increment(slash, ptr);
		len++;
	}

	return len;
}

static void slash_history_copy(struct slash *slash, char *dst, char *src, size_t len)
{
	while (len--) {
		*dst++ = *src;
		src = slash_history_increment(slash, src);
	}
}

static char *slash_history_search_back(struct slash *slash,
				       char *start, size_t *startlen)
{
	if (slash->history_cursor == slash->history_head)
		return NULL;

	/* Skip first two trailing zeros */
	start = slash_history_decrement(slash, start);
	start = slash_history_decrement(slash, start);

	while (*start) {
		if (start == slash->history_head)
			break;
		start = slash_history_decrement(slash, start);
	}

	/* Skip leading zero */
	if (start != slash->history_head)
		start = slash_history_increment(slash, start);

	*startlen = slash_history_strlen(slash, start);

	return start;
}

static char *slash_history_search_forward(struct slash *slash,
					  char *start, size_t *startlen)
{
	if (slash->history_cursor == slash->history_tail)
		return NULL;

	while (*start) {
		start = slash_history_increment(slash, start);
		if (start == slash->history_tail)
			return NULL;
	}

	/* Skip trailing zero */
	start = slash_history_increment(slash, start);

	if (start == slash->history_tail)
		*startlen = 0;
	else
		*startlen = slash_history_strlen(slash, start);

	return start;
}

static void slash_history_pull(struct slash *slash, size_t len)
{
	while (len-- > 0)
		slash_history_push_head(slash);
	while (*slash->history_head != '\0')
		slash_history_push_head(slash);

	/* Push past final zero byte */
	slash_history_push_head(slash);
}

static void slash_history_push(struct slash *slash, char *buf, size_t len)
{
	/* Remove oldest entry until space is available */
	if (len > slash->history_avail)
		slash_history_pull(slash, len - slash->history_avail);

	/* Copy to history */
	while (len--)
		slash_history_push_tail(slash, *buf++);

	slash->history_cursor = slash->history_tail;
}

static void slash_history_rewind(struct slash *slash, size_t len)
{
	while (len-- > 0)
		slash_history_pull_tail(slash);

	*slash->history_tail = '\0';

	slash->history_rewind_length = 0;
}

void slash_history_add(struct slash *slash, char *line)
{
	/* Check if we are browsing history and clear latest entry */
	if (slash->history_depth != 0 && slash->history_rewind_length != 0)
		slash_history_rewind(slash, slash->history_rewind_length);

	/* Reset history depth */
	slash->history_depth = 0;
	slash->history_rewind_length = 0;
	slash->history_cursor = slash->history_tail;

	/* Check if last command was similar */
	size_t srclen;
	char *src = slash_history_search_back(slash, slash->history_cursor, &srclen);
	if ((src) && (strlen(line) == srclen) && (strncmp(src, line, srclen) == 0))
		return;

	/* Push including trailing zero */
	if (!slash_line_empty(line, strlen(line)))
		slash_history_push(slash, line, strlen(line) + 1);
}

static void slash_history_next(struct slash *slash)
{
	char *src;
	size_t srclen;

	src = slash_history_search_forward(slash, slash->history_cursor, &srclen);
	if (!src)
		return;

	slash->history_depth--;
	slash_history_copy(slash, slash->buffer, src, srclen);
	slash->buffer[srclen] = '\0';
	slash->history_cursor = src;
	slash->cursor = slash->length = srclen;

	/* Rewind if use to store buffer temporarily */
	if (!slash->history_depth && slash->history_cursor != slash->history_tail)
		slash_history_rewind(slash, slash->history_rewind_length);
}

static void slash_history_previous(struct slash *slash)
{
	char *src;
	size_t srclen, buflen;

	src = slash_history_search_back(slash, slash->history_cursor, &srclen);
	if (!src)
		return;

	/* Store current buffer temporarily */
	buflen = strlen(slash->buffer);
	if (!slash->history_depth && buflen) {
		slash_history_add(slash, slash->buffer);
		slash->history_rewind_length = buflen + 1;
	}

	slash->history_depth++;
	slash_history_copy(slash, slash->buffer, src, srclen);
	slash->buffer[srclen] = '\0';
	slash->history_cursor = src;
	slash->cursor = slash->length = srclen;
}

/* Line editing */
static void slash_insert(struct slash *slash, int c)
{
	if (slash->length + 1 < slash->line_size) {
		memmove(&slash->buffer[slash->cursor + 1],
			&slash->buffer[slash->cursor],
			slash->length - slash->cursor);
		slash->buffer[slash->cursor] = c;
		slash->cursor++;
		slash->length++;
		slash->buffer[slash->length] = '\0';
	}
}

int slash_refresh(struct slash *slash, int printtime)
{
	char esc[16];

	/* Ensure line is zero terminated */
	slash->buffer[slash->length] = '\0';

	/* Move cursor to left edge */
	snprintf(esc, sizeof(esc), "\r");
	slash_write(slash, esc, strlen(esc));
	
	slash->prompt_print_length = slash_prompt(slash);

	if (slash->length > 0) {
		if (slash_write(slash, slash->buffer, slash->length) < 0)
			return -1;
	}

	int timelen = 0;
	if (printtime) {
		char buf[30];
#ifdef SLASH_TIMESTAMP
		struct timeval tmnow;
		struct tm *tm;
		gettimeofday(&tmnow, NULL);
		tm = localtime(&tmnow.tv_sec);
		strftime(buf, 30, " @ %H:%M:%S d. %d/%m/%y", tm);
		timelen = 21;
#endif

		/* Write the prompt and the current buffer content */
		slash_write(slash, "\033[1;30m", 7);
		slash_write(slash, buf, strlen(buf));
		slash_write(slash, "\033[0m", 4);
	}

	/* Erase to right */
	snprintf(esc, sizeof(esc), ESCAPE("K"));
	if (slash_write(slash, esc, strlen(esc)) < 0)
		return -1;

	/* Move cursor to original position. */
	snprintf(esc, sizeof(esc), "\r" ESCAPE_NUM("C"), (unsigned int)(slash->cursor + slash->prompt_print_length + timelen));
	if (slash_write(slash, esc, strlen(esc)) < 0)
		return -1;

	return 0;
}

static void slash_reset(struct slash *slash)
{
	slash->buffer[0] = '\0';
	slash->length = 0;
	slash->cursor = 0;
}

static void slash_arrow_up(struct slash *slash)
{
	slash_history_previous(slash);
}

static void slash_arrow_down(struct slash *slash)
{
	slash_history_next(slash);
}

static void slash_arrow_right(struct slash *slash)
{
	if (slash->cursor < slash->length)
		slash->cursor++;
}

static void slash_arrow_left(struct slash *slash)
{
	if (slash->cursor > 0)
		slash->cursor--;
}

static void slash_delete(struct slash *slash)
{
	if (slash->cursor < slash->length) {
		slash->length--;
		memmove(&slash->buffer[slash->cursor],
			&slash->buffer[slash->cursor + 1], slash->length - slash->cursor);
		slash->buffer[slash->length] = '\0';
	}
}

static void slash_backspace(struct slash *slash)
{
	if (slash->cursor > 0) {
		slash->cursor--;
		slash->length--;
		memmove(&slash->buffer[slash->cursor],
			&slash->buffer[slash->cursor + 1], slash->length - slash->cursor);
		slash->buffer[slash->length] = '\0';
	}
}

static void slash_delete_word(struct slash *slash)
{
	int old_cursor = slash->cursor, erased;

	while (slash->cursor > 0 && slash->buffer[slash->cursor-1] == ' ')
		slash->cursor--;
	while (slash->cursor > 0 && slash->buffer[slash->cursor-1] != ' ')
		slash->cursor--;

	erased = old_cursor - slash->cursor;

	memmove(slash->buffer + slash->cursor, slash->buffer + old_cursor, erased);
	slash->length -= erased;
}

static void next_word(struct slash *slash)
{
	if (slash->cursor < slash->length) {
		bool word_boundary = false;
		while (++slash->cursor < slash->length) {
			if (word_boundary) break;
			if(slash->buffer[slash->cursor] == ' ') {
				word_boundary = true;
			}
		}
	}
}

static void previous_word(struct slash *slash)
{
	if (slash->cursor > 0) {
		while(--slash->cursor > 0 && slash->buffer[slash->cursor-1] != ' ');
	}
}

static void slash_swap(struct slash *slash)
{
	char tmp;

	if (slash->cursor > 0 && slash->cursor < slash->length) {
		tmp = slash->buffer[slash->cursor-1];
		slash->buffer[slash->cursor-1] = slash->buffer[slash->cursor];
                slash->buffer[slash->cursor] = tmp;
                if (slash->cursor != slash->length-1)
			slash->cursor++;
	}
}


void slash_clear_screen(struct slash *slash) {
	const char *esc = ESCAPE("H") ESCAPE("2J");
	slash_write(slash, esc, strlen(esc));
}

void slash_sigint(struct slash *slash, int signum) {
	if (slash->busy) {
		slash->signal = signum;
	} else {
		slash_reset(slash);	
		slash_refresh(slash, 0);
	}
}

#include <stdlib.h>
char *slash_readline(struct slash *slash)
{
	char *ret = slash->buffer;
	int c, esc[3];
	bool done = false, escaped = false;

	/* Reset buffer */
	slash_reset(slash);
	slash_refresh(slash, 0);

	while (!done && ((c = slash_getchar(slash)) >= 0)) {
		if (escaped) {
			esc[0] = c;
			esc[1] = slash_getchar(slash);

			if (esc[0] == '[' && esc[1] == 'A') {
				slash_arrow_up(slash);
			} else if (esc[0] == '[' && esc[1] == 'B') {
				slash_arrow_down(slash);
			} else if (esc[0] == '[' && esc[1] == 'C') {
				slash_arrow_right(slash);
			} else if (esc[0] == '[' && esc[1] == 'D') {
				slash_arrow_left(slash);
			} else if (esc[0] == '[' && (esc[1] > '0' &&
						     esc[1] < '7')) {
				esc[2] = slash_getchar(slash);
				if (esc[1] == '3' && esc[2] == '~')
					slash_delete(slash);
				else {
					if(esc[1] == '1') {
						if(slash_getchar(slash) == '5') {
							switch(slash_getchar(slash)) {
								case 'D':
									previous_word(slash);
									break;
								case 'C':
									next_word(slash);
									break;
								default:
									break;
							}
						}
					}
				}
			} else if (esc[0] == '[' && esc[1] == 'H') {
				slash->cursor = 0;
			} else if (esc[0] == '[' && esc[1] == 'F') {
				slash->cursor = slash->length;
			} else if (esc[0] == 'O') {
				/* If the first escape character is 'O', we're likely in a TMUX session.
				The HOME and END keys (unless remapped by the user in their tmux config) send "OH" and "OF" respetively
				so we handle those too
				*/
				if(NULL != getenv("TMUX")) {
					if (esc[1] == 'H') {
						slash->cursor = 0;
					} else if (esc[1] == 'F') {
						slash->cursor = slash->length;
					}
				}
			} else if (esc[0] == '1' && esc[1] == '~') {
				slash->cursor = 0;
			} else if (esc[0] == '4' && esc[1] == '[') {
				esc[2] = slash_getchar(slash);
				if (esc[2] == '~')
					slash->cursor = slash->length;
			}
			escaped = false;
		} else if (iscntrl(c)) {
			switch (c) {
			case CONTROL('A'):
				slash->cursor = 0;
				break;
			case CONTROL('B'):
				slash_arrow_left(slash);
				break;
			case CONTROL('C'):
				slash_reset(slash);
				done = true;
				break;
			case CONTROL('D'):
				if (slash->length > 0) {
					slash_delete(slash);
				} else {
					ret = NULL;
					done = true;
				}
				break;
			case CONTROL('E'):
				slash->cursor = slash->length;
				break;
			case CONTROL('F'):
				slash_arrow_right(slash);
				break;
			case CONTROL('K'):
				slash->length = slash->cursor;
				break;
			case CONTROL('L'):
				slash_clear_screen(slash);
				break;
			case CONTROL('N'):
				slash_arrow_down(slash);
				break;
			case CONTROL('P'):
				slash_arrow_up(slash);
				break;
			case CONTROL('T'):
				slash_swap(slash);
				break;
			case CONTROL('U'):
				slash->cursor = 0;
				slash->length = 0;
				break;
			case CONTROL('W'):
				slash_delete_word(slash);
				break;
			case '\t':
				slash_complete(slash);
				break;
			case '\r':
			case '\n':
				done = true;
				break;
			case '\b':
			case DEL:
				slash_backspace(slash);
				break;
			case ESC:
				escaped = true;
				break;
			default:
				/* Unknown control */
				break;
			}
		} else if (isprint(c)) {
			/* Add to buffer */
			slash_insert(slash, c);
		}

		slash->last_char = c;

		if (!done)
			slash_refresh(slash, 0);
	}

	if (strlen(slash->buffer) == 0) {
		slash_refresh(slash, 0);
	} else {
		slash_refresh(slash, 1);
	}
	slash_putchar(slash, '\n');
	slash_history_add(slash, slash->buffer);

	return ret;
}




static void slash_trim(char *line, size_t line_len) {
	if (!line_len) {
		return;
	}

	while(--line_len) {
		if(!isspace(line[line_len])) {
			break;
		}
		line[line_len] = '\0';
	}
}


/* Core */
int slash_loop(struct slash *slash)
{
	int c, ret;
	char *line;

	if (slash_configure_term(slash) < 0)
		return -ENOTTY;

	if (slash->use_activate) {
		slash_printf(slash, "Press enter to activate this console ");
		do {
			c = slash_getchar(slash);
		} while (c != '\n' && c != '\r');
	}
	size_t line_len = 0;
	while ((line = slash_readline(slash))) {
		line_len = strlen(line);
		slash_trim(line, line_len);
		if (!slash_line_empty(line, strlen(line))) {
			/* Run command */
			ret = slash_execute(slash, line);
			if (ret == SLASH_EXIT)
				break;
		}
	}

	slash_restore_term(slash);

	return 0;
}

struct slash *slash_create(size_t line_size, size_t history_size)
{
	struct slash *slash;

	/* Allocate slash context */
	slash = calloc(1, sizeof(*slash));
	if (!slash)
		return NULL;

	/* Setup default values */
	slash->fd_read = STDIN_FILENO;
	slash->fd_write = STDOUT_FILENO;
#ifdef SLASH_HAVE_SELECT
	slash->waitfunc = slash_wait_select;
#endif

	/* Allocate zero-initialized line and history buffers */
	slash->line_size = line_size;
	slash->buffer = calloc(1, slash->line_size);
	if (!slash->buffer) {
		free(slash);
		return NULL;
	}

	slash->history_size = history_size - 1;
	slash->history = calloc(1, history_size);
	if (!slash->history) {
		free(slash->buffer);
		free(slash);
		return NULL;
	}

	/* Initialize history */
	slash->history_head = slash->history;
	slash->history_tail = slash->history;
	slash->history_cursor = slash->history;
	slash->history_avail = slash->history_size - 1;
	slash->complete_in_completion = true;

	slash_list_init();

	if (tcgetattr(slash->fd_read, &slash->original) < 0) {
		free(slash->buffer);
		free(slash);
		return NULL;
	}

	return slash;
}

void slash_create_static(struct slash *slash, char * line_buf, size_t line_size, char * hist_buf, size_t history_size)
{
	/* Setup default values */
	slash->fd_read = STDIN_FILENO;
	slash->fd_write = STDOUT_FILENO;
#ifdef SLASH_HAVE_SELECT
	slash->waitfunc = slash_wait_select;
#endif

	/* Allocate zero-initialized line and history buffers */
	slash->line_size = line_size;
	slash->buffer = line_buf;

	slash->history_size = history_size;
	slash->history = hist_buf;

	/* Initialize history */
	slash->history_head = slash->history;
	slash->history_tail = slash->history;
	slash->history_cursor = slash->history;
	slash->history_avail = slash->history_size - 1;

    /* Empty command list */
    slash->cmd_list = 0;

	slash->complete_in_completion = true;

	tcgetattr(slash->fd_read, &slash->original);
}

void slash_destroy(struct slash *slash)
{
	slash_restore_term(slash);

	if (slash->buffer) {
		free(slash->buffer);
		slash->buffer = NULL;
	}
	if (slash->history) {
		free(slash->history);
		slash->history = NULL;
	}

	free(slash);
}
