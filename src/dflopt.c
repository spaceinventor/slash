#include <stdlib.h>
#include <stdio.h>
#include <slash/slash.h>

unsigned int slash_dfl_node = 0;
unsigned int slash_dfl_timeout = 1000;

static int cmd_node(struct slash *slash) {

	if (slash->argc >= 1) {

		/* We rely on user to provide known hosts implemetnation */
		int known_hosts_get_node(char * find_name);
		slash_dfl_node = known_hosts_get_node(slash->argv[1]);
		if (slash_dfl_node == 0)
			slash_dfl_node = atoi(slash->argv[1]);
	}

	return SLASH_SUCCESS;
}
slash_command(node, cmd_node, "<node>", NULL);


static int cmd_timeout(struct slash *slash) {

	if (slash->argc >= 1) {
		slash_dfl_timeout = atoi(slash->argv[1]);
	}

    printf("timeout = %d\n", slash_dfl_timeout);

	return SLASH_SUCCESS;
}
slash_command(timeout, cmd_timeout, "<timeout ms>", NULL);
