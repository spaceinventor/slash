#include <slash/completer.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <inttypes.h>
#include <limits.h>
#include <dirent.h>

static void ls_appended(const char* tok, const char* app) {
    char cmd[PATH_MAX + 3];

    if ((tok != NULL && strlen(tok) + 1 > PATH_MAX) ||
        (app != NULL && strlen(app) + (tok != NULL ? strlen(tok) : 0) + 1 > PATH_MAX)) {
        printf("Path or argument too long\n");
        return;
    }

    sprintf(cmd, "ls -p %s%s", (tok != NULL ? tok : ""), (app != NULL ? app : ""));
    
    int ret = system(cmd);
    (void)ret;
}

static char * path_cmp(char * tok, char * path) {
    size_t tok_len = strlen(tok);
    if (strlen(path) < tok_len)
        return NULL;

    if (strncmp(tok, path, tok_len) == 0) {
        return path;
    }
    return NULL;
}

static int common_prefix_idx(char ** strings, size_t str_count) {
    int prefix_idx = 0;
    if (str_count == 0)
        return 0;

    // outer loop: iterate each char, inner loop: iterate each string in list
    for (int i = 0; strings[0][i] != '\0'; ++i) {
        for (int j = 0; j < str_count; ++j) {
            if (strings[j][i] != strings[0][i]) {
                prefix_idx = i;
                return prefix_idx;
            }
        }
    }
    return strlen(strings[0]);
}

static int last_char_occ(const char * path_str, char target_char) {
    // TODO: should probably use strrchr() instead
    if (path_str == NULL) {
        return -1;
    }

    for (int i = strlen(path_str)-1; i >= 0; i--) {
        if (path_str[i] == target_char) {
            return i;
        } 
    }

    return -1;
}

void slash_completer_skip_flagged_prefix(struct slash *slash, char * tgt_prefix) {
    if (tgt_prefix == NULL) tgt_prefix = "";
    size_t buffer_len = strlen(slash->buffer);
    size_t prefix_len = strlen(tgt_prefix);

    /* if slash buffer begins with tgt_prefix */
	if (!strncmp(slash->buffer, tgt_prefix, prefix_len)) {
		char * tmp_buf = (char *) malloc(buffer_len+1);
		if (tmp_buf != NULL) { 
			strncpy(tmp_buf, slash->buffer + prefix_len, buffer_len-prefix_len);
		} else {
			printf("Memory allocation error");
			return;
		}
		char * token = strtok(tmp_buf, " ");
		
		/* start at 1 in case first word following tgt_prefix is not a flag */
		int consecutive_ctr = !strcmp(tgt_prefix, "") ? 0 : 1;

		/* if flagged, search for 2nd word not starting with '-' or first word not starting with '--' */
		while (token != NULL) {
			if (token[0] != '-') {
				consecutive_ctr++;
				
				if (consecutive_ctr == 2) {
					/* move buffer pointer ahead of "tgt_prefix [OPTIONS] ..." */
					slash->buffer = slash->buffer + (prefix_len+(token-tmp_buf));
					slash->length = strlen(slash->buffer);
					slash->cursor = slash->length;
					break;
            	}
			} else if (token[1] == '-') {
				consecutive_ctr++;
			} else {
				consecutive_ctr = 0;
			}
			token = strtok(NULL, " ");
		}
		free(tmp_buf);
	}
}

void slash_completer_revert_skip(struct slash *slash, char * orig_slash_buf) {
    /* return buffer to original mem address */
	slash->buffer = orig_slash_buf;
	slash->length = strlen(slash->buffer);
	slash->cursor = slash->length;
}

int slash_prefix_length(const char *s1, const char *s2)
{
	int len = 0;

	while (*s1 && *s2 && *s1 == *s2) {
		len++;
		s1++;
		s2++;
	}

	return len;
}

/**
 * @brief For tab auto completion, calls other completion functions when matched command has them
 * 
 * @param slash Slash context
 */
void slash_complete(struct slash *slash)
{
	int matches = 0;
	size_t prefixlen = -1;
	struct slash_command *prefix = NULL;
	
	struct slash_command * cmd;
	slash_list_iterator i = {};
    	while ((cmd = slash_list_iterate(&i)) != NULL) {

        if (strncmp(slash->buffer, cmd->name, slash_min(strlen(cmd->name), slash->length)) == 0) {

            if(strlen(cmd->name) < slash->length && slash->buffer[slash->length - 1] != ' ') {
                if (NULL == prefix) {
                    /* Count matches */
                    matches++;
                    slash->in_completion = true;
                    prefix = cmd;
                    if (matches == 1)
                        slash_printf(slash, "\n");
                }
                continue;
            }
            if(false == slash->in_completion) {
                if(strlen(cmd->name) < (slash->length - 1) && slash->buffer[slash->length - 1] == ' ') {
                    continue;
                }
            }
            /* Count matches */
            matches++;
            slash->in_completion = true;
			/* Find common prefix */
			if (prefixlen == (size_t) -1) {
				prefix = cmd;
				prefixlen = strlen(prefix->name);
			} else {
				size_t new_prefixlen = slash_prefix_length(prefix->name, cmd->name);
				if (new_prefixlen < prefixlen)
					prefixlen = new_prefixlen;
			}

			/* Print newline on first match */
			if (matches == 1)
				slash_printf(slash, "\n");

			/* We only print all commands over 1 match here */
			if (matches > 1)
				slash_command_description(slash, cmd);

		}

	}

	if (!matches) {
        slash->in_completion = false;
		slash_bell(slash);
	} else if (matches == 1) {        
		if (prefixlen != -1) { 
            if (slash->cursor <= prefixlen) {
                strncpy(slash->buffer, prefix->name, prefixlen);
                slash->buffer[prefixlen] = '\0';
                strcat(slash->buffer, " ");
                slash->cursor = slash->length = strlen(slash->buffer);
            } else {
                if (prefix->completer) {
                    prefix->completer(slash, slash->buffer + prefixlen + 1);
                }
            }
		} else {
			if (prefix->completer) {
				prefix->completer(slash, slash->buffer + strlen(prefix->name) + 1);
			}
		}
	} else if (slash->last_char != '\t') {
		/* Print the first match as well */
		slash_command_description(slash, prefix);
 		strncpy(slash->buffer, prefix->name, prefixlen);
		slash->buffer[prefixlen] = '\0';
		slash->cursor = slash->length = strlen(slash->buffer);
		slash_bell(slash);
	}
}

/**
 * @brief For file system path tab auto completion
 * 
 * @param slash Slash context
 * @param token Slash buffer after first space
 */
void slash_path_completer(struct slash * slash, char * token) {
    // TODO: Add windows support
    char cwd_buf[256];
    char file_name_buf[256];
    size_t match_list_size;
    uint16_t match_count;
    char ** match_list;
    DIR * cwd_ptr;
    struct dirent * entry;

    if (token[0]=='\0') {
        ls_appended(NULL, NULL);
        return;
    }

    /* skip flags in cmd */
    char * orig_slash_buffer = slash->buffer;
    slash_completer_skip_flagged_prefix(slash, NULL);

    /* lazy fix */
    token = slash->buffer;

    char* res = getcwd(cwd_buf, sizeof(cwd_buf));
    
    if (res != cwd_buf) {
        printf("Path error\n");
        slash_completer_revert_skip(slash, orig_slash_buffer);
        return;
    }
    
    // TODO: Add support for absolute paths
    /* handle home shortcut (~) and subdirectories in token */
    int subdir_idx = last_char_occ(token, '/');
    if (token[0] == '~') {
        memset(cwd_buf, '\0', 256);
        strcpy(cwd_buf, getenv("HOME"));
        strncpy(&cwd_buf[strlen(cwd_buf)], &token[1], subdir_idx);
    } else if (subdir_idx != -1) {
        strcat(cwd_buf, "/");
        strncat(cwd_buf, token, subdir_idx);
    }
    cwd_ptr = opendir(cwd_buf);

    if (cwd_ptr == NULL) {
        printf("No such file or directory:\n");
        ls_appended(NULL, NULL);
        slash_completer_revert_skip(slash, orig_slash_buffer);
        return;
    }

    strcpy(file_name_buf, token+subdir_idx+1);
    
    /* allocate memory dynamically as we don't know no. of matches */
    match_list_size = 16;
    match_count = 0;
    match_list = (char**) malloc(match_list_size * sizeof(char*));
    
    while ((entry = readdir(cwd_ptr)) != NULL) {

        /* compare token with filename */
        char* pmatch = path_cmp(file_name_buf, entry->d_name);

        if (pmatch != NULL && strcmp(pmatch, ".")) {
            /* allocate more memory if necessary */
            if (match_count+1 >= match_list_size) {
                match_list_size += 16;
                char** tmp = (char**) reallocarray(match_list, match_list_size, sizeof(char*));
                if (tmp == NULL) {
                    printf("Unable to find all matches: No memory\n");
                    break;
                }
                match_list = tmp;
            }

            match_list[match_count] = (char*)malloc(strlen(pmatch) + 3);
            strcpy(match_list[match_count], pmatch); 
            if (entry->d_type == DT_DIR) {
                strcat(match_list[match_count], "/");
            }
            match_count++;
        }
    }

    switch (match_count)
    {
        case 0:
            ls_appended((subdir_idx > -1) ? cwd_buf : NULL, NULL);
            slash_bell(slash);
            break;

        case 1:
            strcpy(token+subdir_idx+1, match_list[0]);
            slash->length = (token - slash->buffer) + strlen(token);
            slash->cursor = slash->length;
            break;
        
        default:
            int prefix_idx = common_prefix_idx(match_list, match_count);

            strncpy(token+subdir_idx+1, match_list[0], prefix_idx);
            token[subdir_idx+prefix_idx+1] = 0;
            slash->length = (token - slash->buffer) + strlen(token);
            slash->cursor = slash->length;
            ls_appended(token, "* -d");
            break;
    }

    slash_completer_revert_skip(slash, orig_slash_buffer);
    
    closedir(cwd_ptr);

    /* free all memory allocated for string matches */
    for (int i = 0; i < match_count; i++)
    {
        free(match_list[i]);
    }
    free((void*)match_list);
}

/**
 * @brief For tab auto completion when using "watch"
 * 
 * @param slash Slash context
 * @param token Slash buffer after first space
 */
void slash_watch_completer(struct slash *slash, char * token) {
    // skip watch
	char * orig_slash_buffer = slash->buffer;
	slash_completer_skip_flagged_prefix(slash, "watch");

    slash_complete(slash);

    slash_completer_revert_skip(slash, orig_slash_buffer);
}