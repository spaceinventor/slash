#include <gtest/gtest.h>
#include <stdio.h>
#include <sys/mman.h>
#include <slash/slash.h>

char line_buf[256] = {0};
char history_buf[256] = {0};

static int cmd_long(struct slash *slash)
{
	printf("Running long command\n");

	return SLASH_SUCCESS;
}
slash_command(long_command, cmd_long, NULL,
	      "A long command\n");

static int cmd_longer(struct slash *slash)
{
	printf("Running longer command\n");

	return SLASH_SUCCESS;
}
slash_command(longer_command, cmd_longer, NULL,
	      "An even longer command\n");

static int cmd_test(struct slash *slash)
{
	if (slash->argc < 2)
		return SLASH_EUSAGE;

	printf("Running test command\n");

	return SLASH_SUCCESS;
}
slash_command(test, cmd_test, "<arg>",
	      "A simple test command\n\n"
	      "Requires a single command which must be present\n");

static int cmd_subcommand(struct slash *slash)
{
	printf("Running the test subcommand\n");

	return SLASH_SUCCESS;
}
slash_command_sub(test, sub_command, cmd_subcommand, NULL,
		  "A subcommand for test");

static int cmd_args(struct slash *slash)
{
	int i;

	printf("Got %d args:\n", slash->argc);

	for (i = 0; i < slash->argc; i++)
		printf("arg %d: \'%s\'\n", i, slash->argv[i]);

	return SLASH_SUCCESS;
}
slash_command_group(group, "Empty command group");
slash_command_sub(group, args, cmd_args, "[arg ...]",
		  "Print argument list");


TEST(slash, completion_test) {
    struct slash s;
    SLASH_LOAD_CMDS(slash_tests);
    slash_create_static(&s, line_buf, sizeof(line_buf), history_buf, sizeof(history_buf));
    char input_buf[256];
    int input = memfd_create("fake_stdin", 0);
    dup2(input, STDIN_FILENO);
    write(input, "help he\t\n", sizeof("help he\t\n"));
    lseek(input, 0, SEEK_SET);
    char *line;
    int saved_stdout = dup(STDOUT_FILENO);
    int output = memfd_create("fake_stdout", 0);
    dup2(output, STDOUT_FILENO);
	line = slash_readline(&s);
    // printf("%s", line);
    /* Run command */
    int ret = slash_execute(&s, line);
    ASSERT_EQ(ret, SLASH_SUCCESS);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);    
    lseek(output, 0, SEEK_SET);
    int res;
    FILE *out = fdopen(output, "r");
    while(fgets(input_buf, sizeof(input_buf), out)) {
        printf("STDOUT: %s", input_buf);
    }
    // while((res = read(output, input_buf, sizeof(input_buf))) > 0) {
    //     write(STDOUT_FILENO, input_buf, res);
    // }
}