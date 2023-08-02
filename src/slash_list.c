#include <slash/slash.h>
#include <stdint.h>
#include <string.h>

#define SLASH_HAVE_SYS_QUEUE 
#ifdef SLASH_HAVE_SYS_QUEUE
#include <sys/queue.h>
#endif

/**
 * The storage size (i.e. how closely two slash_command structs are packed in memory)
 * varies from platform to platform (in example on x64 and arm32). This macro
 * defines two param_t structs and saves the storage size in a define.
 */
#ifndef SLASH_STORAGE_SIZE
static struct slash_command command_size_set[2] __attribute__((aligned(1)));
#define SLASH_STORAGE_SIZE ((intptr_t) &command_size_set[1] - (intptr_t) &command_size_set[0])
#endif

#ifdef SLASH_HAVE_SYS_QUEUE
static SLIST_HEAD(slash_list_head_s, slash_command) slash_list_head = {};
#endif

struct slash_command * slash_list_iterate(slash_list_iterator * iterator) {

	/**
	 * GNU Linker symbols. These will be autogenerate by GCC when using
	 * __attribute__((section("slash"))
	 */
	__attribute__((weak)) extern struct slash_command __start_slash;
	__attribute__((weak)) extern struct slash_command __stop_slash;

	/* First element */
	if (iterator->element == NULL) {

		/* Static */
		if ((&__start_slash != NULL) && (&__start_slash != &__stop_slash)) {
			iterator->phase = 0;
			iterator->element = &__start_slash;
		} else {
			iterator->phase = 1;
#ifdef SLASH_HAVE_SYS_QUEUE
			iterator->element = SLIST_FIRST(&slash_list_head);
#endif
		}

		return iterator->element;
	}

	/* Static phase */
	if (iterator->phase == 0) {

		/* Increment in static memory */
		iterator->element = (struct slash_command *)(intptr_t)((char *)iterator->element + SLASH_STORAGE_SIZE);

		/* Check if we are still within the bounds of the static memory area */
		if (iterator->element < &__stop_slash)
			return iterator->element;

		/* Otherwise, switch to dynamic phase */
		iterator->phase = 1;
#ifdef SLASH_HAVE_SYS_QUEUE
		iterator->element = SLIST_FIRST(&slash_list_head);
		return iterator->element;
#else
		return NULL;
#endif
	}

#ifdef SLASH_HAVE_SYS_QUEUE
	/* Dynamic phase */
	if (iterator->phase == 1) {

		iterator->element = SLIST_NEXT(iterator->element, next);
		return iterator->element;
	}
#endif

	return NULL;

}

struct slash_command * slash_list_find_name(char * name) {

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
		return 1;
	} else {
#ifdef SLASH_HAVE_SYS_QUEUE
		SLIST_INSERT_HEAD(&slash_list_head, item, next);
#else
		return -1;
#endif
		return 0;
	}
}
