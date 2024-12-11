#include <slash/slash.h>
#include <slash/completer.h>
#include <slash/optparse.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>


/* Implement this function to set environment variables for example */
__attribute__((weak)) void slash_on_run_pre_hook(const char * const filename, void ** ctx_for_post) {  /* Set up environemnt variables for "run" command. */
}

/* Implement this function to clear environment variables for example */
__attribute__((weak)) void slash_on_run_post_hook(const char * const filename, void * ctx) {
}

int slash_run(struct slash *slash, char * filename, int printcmd) {

    char filename_local[256];
    if (filename[0] == '~') {
        strcpy(filename_local, getenv("HOME"));
        strcpy(&filename_local[strlen(filename_local)], &filename[1]);
    }
    else {
        strcpy(filename_local, filename);
    }

    /* Read from file */
	FILE * stream = fopen(filename_local, "r");
	if (stream == NULL) {
        printf("  File %s not found\n", filename);
		return SLASH_EIO;
    }


    void *ctx_for_post = NULL;
    slash_on_run_pre_hook(filename_local, &ctx_for_post);

    char line[512];
    int ret = SLASH_SUCCESS;
    while(fgets(line, sizeof(line), stream)) {
        
        /* Strip newline and carriage return */
        line[strcspn(line, "\n\r")] = 0;

        /* Skip short lines */
        if (strlen(line) <= 1)
            continue;

        /* Skip comments */
        if (line[0] == '/') {
            continue;
        }

        if (printcmd)
            printf("  run: %s\n", line);

        ret = slash_execute(slash, line);
        slash_history_add(slash, line);
        if (ret == SLASH_EXIT || ret == SLASH_EBREAK) {
            break;
        }
    }

    fclose(stream);

    slash_on_run_post_hook(filename_local, ctx_for_post);

    return ret;

}

static int cmd_run(struct slash *slash) {

    optparse_t * parser = optparse_new("run", "<filename>");
    optparse_add_help(parser);

    int argi = optparse_parse(parser, slash->argc - 1, (const char **) slash->argv + 1);
    if (argi < 0) {
        optparse_del(parser);
	    return SLASH_EINVAL;
    }

	/* Check if name is present */
	if (++argi >= slash->argc) {
		printf("missing parameter filename\n");
        optparse_del(parser);
		return SLASH_EINVAL;
	}

	char * name = slash->argv[argi];

    printf("Running %s\n", name);

    int res = slash_run(slash, name, 1);
    optparse_del(parser);
    return res;

}
slash_command_completer(run, cmd_run, slash_path_completer, "<file>", "Runs commands in specified file\n"\
    "Sets the following environemnt variables during execution:\n\n"\
    "- __FILE__ to the name of the executed file\n"\
    "- __FILE_DIR__ to the directory containing the executed file, useful for running other files located relative to __FILE__");
	