#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <slash/slash.h>
#include <slash/dflopt.h>
#include <slash/optparse.h>
#include <slash/completer.h>

#include "builtins.h"


/* Builtin commands */
static int slash_builtin_help(struct slash *slash)
{
	char *args;
	char find[slash->line_size];
	int i;
	size_t available = sizeof(find);
	struct slash_command *command;

	/* If no arguments given, just list all top-level commands */
	if (slash->argc < 2) {
		struct slash_command * cmd;
		slash_list_iterator i = {};
		while ((cmd = slash_list_iterate(&i)) != NULL) {
			slash_command_description(slash, cmd);
		}
		return SLASH_SUCCESS;
	}

	find[0] = '\0';

	for (i = 1; i < slash->argc; i++) {
		if (strlen(slash->argv[i]) >= (size_t) available)
			return SLASH_ENOSPC;
		strcat(find, slash->argv[i]);
		strcat(find, " ");
	}
	command = slash_command_find(slash, find, strlen(find), &args);
	if (!command) {
		slash_printf(slash, "No such command: %s\n", find);
		return SLASH_EINVAL;
	}

	slash_command_usage(slash, command);

	return SLASH_SUCCESS;
}
slash_command(help, slash_builtin_help, "[command]",
	      "Show available commands");

static int slash_builtin_history(struct slash *slash)
{
	char *p = slash->history_head;

	while (p != slash->history_tail) {
		slash_putchar(slash, *p ? *p : '\n');
		p = slash_history_increment(slash, p);
	}

	return SLASH_SUCCESS;
}
slash_command(history, slash_builtin_history, NULL,
	      "Show previous commands");

#ifndef SLASH_NO_EXIT
static int slash_builtin_exit(struct slash *slash)
{
	(void)slash;
	return SLASH_EXIT;
}
slash_command(exit, slash_builtin_exit, NULL,
	      "Exit application");
#endif

void slash_require_activation(struct slash *slash, bool activate)
{
	slash->use_activate = activate;
}

static int slash_builtin_confirm(struct slash *slash) {
	optparse_t * parser __attribute__((cleanup(optparse_del))) = optparse_new("confirm", "[]");
	optparse_add_help(parser);

	printf("Confirm: Type 'yes' or 'y' + enter to continue:\n");
	char * c = slash_readline(slash);
	if (strcasecmp(c, "yes") == 0 || strcasecmp(c, "y") == 0) {
		return SLASH_SUCCESS;
	} else {
		return SLASH_EBREAK;
	}
}
slash_command(confirm, slash_builtin_confirm, "", "Block until user confirmation");

static int slash_builtin_watch(struct slash *slash)
{

	unsigned int interval = slash_dfl_timeout;
	unsigned int count = 0;

    optparse_t * parser __attribute__((cleanup(optparse_del))) = optparse_new("watch", "<command...>");
    optparse_add_help(parser);
	optparse_add_unsigned(parser, 'n', "interval", "NUM", 0, &interval, "interval in milliseconds (default = <env timeout>)");
	optparse_add_unsigned(parser, 'c', "count", "NUM", 0, &count, "number of times to repeat (default = infinite)");

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
	    return SLASH_EINVAL;
    }

	/* Build command string */

	char line[slash->line_size];
	line[0] = '\0';
	for (int arg = argi + 1; arg < slash->argc; arg++) {
		strncat(line, slash->argv[arg], slash->line_size - strlen(line));
		strncat(line, " ", slash->line_size - strlen(line));
	}

	printf("Executing \"%s\" each %u ms - press <enter> to stop\n", line, interval);

	while(1) {

		/* Make another copy, since slash_exec will modify this */
		char cmd_exec[slash->line_size];
		strncpy(cmd_exec, line, slash->line_size);

		/* Execute command */
		slash_execute(slash, cmd_exec);

		if ((count > 0) && (count-- == 1)) {
				break;
		}		

		/* Delay (press enter to exit) */
		if (slash_wait_interruptible(slash, interval) != 0)
			break;

	}

	return SLASH_SUCCESS;
}
slash_command_completer(watch, slash_builtin_watch, slash_watch_completer, "<command...>", "Repeat a command")