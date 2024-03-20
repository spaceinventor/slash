#ifndef SLASH_COMPLETER_H
#define SLASH_COMPLETER_H

#include <slash/slash.h>

void slash_completer_skip_flagged_prefix(struct slash *slash, char * tgt_prefix);
void slash_completer_revert_skip(struct slash *slash, char * orig_slash_buffer);
void slash_complete(struct slash * slash);
void slash_path_completer(struct slash * slash, char * token);
void slash_watch_completer(struct slash * slash, char * token);

#endif // SLASH_COMPLETER_H