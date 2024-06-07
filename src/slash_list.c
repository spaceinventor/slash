#include <slash/slash.h>
#include <stdint.h>
#include <string.h>

#include <sys/queue.h>

/**
 * The storage size (i.e. how closely two slash_command structs are packed in memory)
 * varies from platform to platform (in example on x64 and arm32). This macro
 * defines two param_t structs and saves the storage size in a define.
 */
#ifndef SLASH_STORAGE_SIZE
static struct slash_command command_size_set[2] __attribute__((aligned(1)));
#define SLASH_STORAGE_SIZE ((intptr_t) &command_size_set[1] - (intptr_t) &command_size_set[0])
#endif

static SLIST_HEAD(slash_list_head_s, slash_command) slash_list_head = {};

struct slash_command * slash_list_iterate(slash_list_iterator * iterator) {

	/* First element */
	if (iterator->element == NULL) {
		iterator->element = SLIST_FIRST(&slash_list_head);
		return iterator->element;
	}

	iterator->element = SLIST_NEXT(iterator->element, next);
	return iterator->element;
}

struct slash_command * slash_list_find_name(const char * name) {

	struct slash_command * found = NULL;
	struct slash_command * cmd;
	slash_list_iterator i = {};
	while ((cmd = slash_list_iterate(&i)) != NULL) {

		if (strcmp(cmd->name, name) == 0) {
			found = cmd;
			break;
		}

		continue;
	}

	return found;
}

int slash_list_add(struct slash_command * item) {

	struct slash_command * cmd;
	if ((cmd = slash_list_find_name(item->name)) != NULL) {
		SLIST_REMOVE(&slash_list_head, cmd, slash_command, next);
		SLIST_INSERT_HEAD(&slash_list_head, item, next);
		return 1;
	} else {
		SLIST_INSERT_HEAD(&slash_list_head, item, next);
		return 0;
	}
}

int slash_list_remove(struct slash_command * item) {

	struct slash_command * cmd;
	if ((cmd = slash_list_find_name(item->name)) != NULL) {  // Check that the command exists in the global list
		SLIST_REMOVE(&slash_list_head, cmd, slash_command, next);
		return 0;
	} else {
		return -1;
	}
}

void slash_load_cmds_from_section(struct slash_command *start, struct slash_command *stop) {
	slash_list_iterator iterator = {};
	iterator.element = start;

	while (iterator.element != stop) {
		slash_list_add(iterator.element);

		iterator.element = (struct slash_command *)(intptr_t)((char *)iterator.element + SLASH_STORAGE_SIZE);
	}

}

DECLARE_SLASH_CMDS_SECTION(slash)

int slash_list_init() {
	SLASH_LOAD_CMDS(slash);

	return 0;
}
