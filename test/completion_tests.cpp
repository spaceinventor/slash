#include <gtest/gtest.h>
#include <stdio.h>
#include <sys/mman.h>

extern "C"{
    #include <slash/slash.h>
}
char line_buf[256] = {0};
char history_buf[256] = {0};

TEST(slash, completion_test) {
    struct slash s;
    slash_create_static(&s, line_buf, sizeof(line_buf), history_buf, sizeof(history_buf));
    char input_buf[256];
    int input = memfd_create("fake_stdin", 0);
    dup2(input, STDIN_FILENO);
    write(input, "he\t\n", sizeof("he\t\n"));
    lseek(input, 0, SEEK_SET);
    char *line;
    int saved_stdout = dup(STDOUT_FILENO);
    int output = memfd_create("fake_stdout", 0);
    dup2(output, STDOUT_FILENO);
	line = slash_readline(&s);
    printf("%s", line);
    /* Run command */
    int ret = slash_execute(&s, line);
    ASSERT_EQ(ret, SLASH_SUCCESS);
    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);    
    lseek(output, 0, SEEK_SET);
    int res;
    while((res = read(output, input_buf, sizeof(input_buf))) > 0) {
        write(STDOUT_FILENO, input_buf, res);
    }
}