#ifndef SLASH_COMPLETER_H
#define SLASH_COMPLETER_H

#include <slash/slash.h>

void slash_completer_skip_flagged_prefix(slash_t *slash, char * tgt_prefix);
void slash_completer_revert_skip(slash_t *slash, char * orig_slash_buffer);
void slash_complete(slash_t * slash);
void slash_path_completer(slash_t * slash, char * token);
void slash_watch_completer(slash_t * slash, char * token);

#endif // SLASH_COMPLETER_H