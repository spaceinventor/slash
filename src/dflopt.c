#include <stdlib.h>
#include <stdio.h>
#include <slash/slash.h>

unsigned int slash_dfl_node = 0;
unsigned int slash_dfl_timeout = 1000;

static int cmd_node(slash_t *slash) {

	if (slash->argc < 1) {
		/* Only print node when explicitly asked,
			as it should be shown in the prompt.
			(it may be truncated when the hostname is too long) */
		printf("Default node = %d\n", slash_dfl_node);
	}

	if (slash->argc == 2) {

		/* We rely on user to provide known hosts implementation */
		int known_hosts_get_node(char * find_name);
		slash_dfl_node = known_hosts_get_node(slash->argv[1]);
		if (slash_dfl_node == 0)
			slash_dfl_node = atoi(slash->argv[1]);
	}

	return SLASH_SUCCESS;
}
slash_command(node, cmd_node, "[node]", "Set global default node");


static int cmd_timeout(slash_t *slash) {

	if (slash->argc == 2) {
		slash_dfl_timeout = atoi(slash->argv[1]);
	}

    printf("timeout = %d\n", slash_dfl_timeout);

	return SLASH_SUCCESS;
}
slash_command(timeout, cmd_timeout, "[timeout ms]", "Set global default timeout");
