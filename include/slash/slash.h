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

#ifndef SLASH_SECTION
#define SLASH_SECTION "slash"
#endif

#ifdef __cplusplus
extern "C" {
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
	__attribute__((section(SLASH_SECTION)))\
	__attribute__((aligned(4)))\
	__attribute__((used))\
	const struct slash_command _ident = {\
		.name  = _name,\
		.func  = _func,\
		.args  = _args,\
		.help = _help, \
		.completer  = _completer,\
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
	const char *name;
	const slash_func_t func;
	const char *args;
	const char *help;
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
	int signal;

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

	/* Allow calling complete functions while already in in one of those functions.
	 * Use case: when completing "help", you only want to complete the *name* of commands to get help for
	 * BUT while completing "watch" (or watch-like commands), you want to carry on completing as much as possible,
	 * for instance, typing: "w<TAB>g<TAB>s<TAB>" would result in the completed command line "watch get serial0" in 6 keystrokes
	 */
	bool complete_in_completion;
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

void slash_sigint(struct slash *slash, int signum);

/**
 * @brief Implement this function to do something with the current line (logging, etc)
 * 
 * @param line the slash line about to be executed
 */
void slash_on_execute_hook(const char *line);


typedef char *(*slash_process_cmd_line_hook_t)(const char *line);
/**
 * @brief Set this variable to a function if you wish to modify the command line about to be executed
 * 
 * @param line the slash line about to be executed
 * @return a malloc'ed pointer to the processed command line, this will be free()'d for you, NULL is a valid
 * return value and will cause the original line to be used as-is.
 */
extern slash_process_cmd_line_hook_t slash_process_cmd_line_hook;

/**
 * @brief Set this variable to a function if you wish to execute general completions after specific completions have been executed
 * @see slash_completer_func_t
 */
extern slash_completer_func_t slash_global_completer;

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
struct slash_command * slash_list_find_name(const char * name);
int slash_list_add(struct slash_command * item);
int slash_list_remove(const struct slash_command * item);
int slash_list_init();

/**
 * @brief Register slash commands defined in a section (in the ELF sense)
 * @param start pointer to the first element in the section
 * @param stop pointer to the last element in the section
 */
void slash_load_cmds_from_section(struct slash_command *start, struct slash_command *stop);

/** 
 * @brief helper macro to load slash cmds from the given ELF section name
 * @example To load the slash commands defined in the "my_slash" ELF section:
 *      SLASH_LOAD_CMDS(my_slash);
 */
#define SLASH_LOAD_CMDS(section) { \
	extern struct slash_command __start_##section; \
	extern struct slash_command __stop_##section; \
	slash_load_cmds_from_section(&(__start_##section), &(__stop_##section)); \
}


#define DECLARE_SLASH_CMDS_SECTION(section) \
	extern struct slash_command __start_##section; \
	extern struct slash_command __stop_##section; \
	struct slash_command *section##_start = &__start_##section; \
	struct slash_command *section##_stop = &__stop_##section;

#ifdef __cplusplus
}
#endif

#endif /* _SLASH_H_ */
