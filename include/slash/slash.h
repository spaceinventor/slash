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

#ifndef _SLASH_H_
#define _SLASH_H_

#include <stddef.h>
#include <stdbool.h>

#include <slash_config.h>

#ifdef SLASH_HAVE_TERMIOS_H
#include <termios.h>
#endif


/* Helper macros */
#define slash_offsetof(type, member) ((size_t) &((type *)0)->member)

#define slash_container_of(ptr, type, member) ({		\
	const typeof( ((type *)0)->member ) *__mptr = (ptr);	\
	(type *)( (void *)__mptr - offsetof(type,member) );})

#define slash_max(a,b) \
	({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a > _b ? _a : _b; })

#define slash_min(a,b) \
	({ __typeof__ (a) _a = (a); \
	__typeof__ (b) _b = (b); \
	_a < _b ? _a : _b; })

#define __slash_command(_ident, _name, _func, _completer, _args, _help) 	\
	__attribute__((section("slash")))\
	__attribute__((aligned(4)))\
	__attribute__((used))\
	const struct slash_command _ident = {\
		.name  = _name,\
		.func  = _func,\
		.completer  = _completer,\
		.args  = _args,\
        .next = {NULL},  /* Next pointer in case the user wants to implement custom ordering within or across APMs.
							It should not required by the default implementation. */\
	};

#define slash_command(_name, _func, _args, _help)			\
	__slash_command(slash_cmd_ ## _name,				\
			#_name, _func, NULL, _args, _help)

#define slash_command_sub(_group, _name, _func, _args, _help)		\
	__slash_command(slash_cmd_##_group ## _ ## _name ,		\
			#_group" "#_name, _func, NULL, _args, _help)

#define slash_command_subsub(_group, _subgroup, _name, _func, _args, _help) \
	__slash_command(slash_cmd_ ## _group ## _ ## _subgroup ## _name, \
			#_group" "#_subgroup" "#_name, _func, NULL, _args, _help)

#define slash_command_completer(_name, _func, _completer, _args, _help)			\
	__slash_command(slash_cmd_ ## _name,				\
			#_name, _func, _completer, _args, _help)

#define slash_command_sub_completer(_group, _name, _func, _completer, _args, _help)		\
	__slash_command(slash_cmd_##_group ## _ ## _name ,		\
			#_group" "#_name, _func, _completer, _args, _help)

#define slash_command_subsub_completer(_group, _subgroup, _name, _func, _completer, _args, _help) \
	__slash_command(slash_cmd_ ## _group ## _ ## _subgroup ## _name, \
			#_group" "#_subgroup" "#_name, _func, _completer, _args, _help)


#define slash_command_group(_name, _help)

#define slash_command_subgroup(_group, _name, _help)

/* Command prototype */
struct slash;
typedef int (*slash_func_t)(struct slash *slash);

/* Wait function prototype,
 * this function is implemented by user, so use a void* instead of struct slash* */
typedef int (*slash_waitfunc_t)(void *slash, unsigned int ms);

/* Autocomplete function prototype */
typedef void (*slash_completer_func_t)(struct slash *slash, char * token);

/* Command return values */
#define SLASH_EXIT	( 1)
#define SLASH_SUCCESS	( 0)
#define SLASH_EUSAGE	(-1)
#define SLASH_EINVAL	(-2)
#define SLASH_ENOSPC	(-3)
#define SLASH_EIO	(-4)
#define SLASH_ENOMEM	(-5)
#define SLASH_ENOENT	(-6)
#define SLASH_EBREAK	(-7)

/* Command struct */
struct slash_command {
	/* Static data */
	char *name;
	const slash_func_t func;
	const char *args;
	const slash_completer_func_t completer;
	/* Next pointer in case the user wants to implement custom ordering within or across APMs.
		It should not required by the default implementation. */
	/* single linked list:
	 * The weird definition format comes from sys/queue.h SLINST_ENTRY() macro */
    struct { struct slash_command *sle_next; } next;
};

/* Slash context */
struct slash {

	/* Terminal handling */
#ifdef SLASH_HAVE_TERMIOS_H
	struct termios original;
#endif
	int fd_write;
	int fd_read;
	slash_waitfunc_t waitfunc;
	bool use_activate;

	/* Line editing */
	size_t line_size;
	const char *prompt;
	size_t prompt_length;
	size_t prompt_print_length;
	char *buffer;
	size_t cursor;
	size_t length;
	bool escaped;
	char last_char;

	/* History */
	size_t history_size;
	int history_depth;
	size_t history_avail;
	int history_rewind_length;
	char *history;
	char *history_head;
	char *history_tail;
	char *history_cursor;

	/* Command interface */
	char **argv;
	int argc;

	/* getopt state */
	char *optarg;
	int optind;
	int opterr;
	int optopt;
	int sp;

    /* Command list */
    struct slash_command * cmd_list;

	/* Completions */
	bool in_completion;
};

/**
 * @brief Initializes an APM using slash.
 * 
 * @param handle Dynamic linking handle to the APM.
 */
void slash_init_apm(void * handle);

struct slash *slash_create(size_t line_size, size_t history_size);

void slash_create_static(struct slash *slash, char * line_buf, size_t line_size, char * hist_buf, size_t history_size);

void slash_destroy(struct slash *slash);

char *slash_readline(struct slash *slash);

/**
 * @brief Implement this function to do something with the current line (logging, etc)
 * 
 * @warning the current __attribute__((weak)) implementation performs the LOKI logging
 * temporarily, this behaviour WILL be removed, leaving it up to slash-users (mainly (only?) CSH)
 * to provide this implementation
 * 
 * @param line the slash line about to be executed
 */
void slash_on_execute_hook(const char *line);

int slash_execute(struct slash *slash, char *line);

int slash_loop(struct slash *slash);

int slash_wait_interruptible(struct slash *slash, unsigned int ms);

int slash_set_wait_interruptible(struct slash *slash, slash_waitfunc_t waitfunc);

int slash_printf(struct slash *slash, const char *format, ...);

int slash_getopt(struct slash *slash, const char *optstring);

void slash_clear_screen(struct slash *slash);

void slash_require_activation(struct slash *slash, bool activate);

void slash_bell(struct slash *slash);

int slash_prefix_length(const char *s1, const char *s2);

int slash_refresh(struct slash *slash, int printtime);

int slash_write(struct slash *slash, const char *buf, size_t count);

/**
 * @brief Implement this function to create a custom prompt
 * 
 * A default no-prompt implementation is provided as a __attribute__((weak))
 * 
 * @return length of the custom prompt
 */
int slash_prompt(struct slash * slash);

void slash_command_description(struct slash *slash, struct slash_command *command);

int slash_run(struct slash *slash, char * filename, int printcmd);

void slash_history_add(struct slash *slash, char *line);

typedef struct slash_list_iterator_s {
	struct slash_command * element;
} slash_list_iterator;

struct slash_command * slash_list_iterate(slash_list_iterator * iterator);
int slash_list_add(struct slash_command * item);
int slash_list_init();

#endif /* _SLASH_H_ */
