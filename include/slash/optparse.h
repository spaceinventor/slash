/* definition of the optparse API

   Copyright (c) 2009-2014 Christer Weinigel <christer@weinigel.se>.

   This software is licensed under the MIT License.
*/

#ifndef _OPTPARSE_H
#define _OPTPARSE_H

#include <stdio.h>

typedef struct optparse optparse_t;
typedef struct optparse_opt optparse_opt_t;

/**
 * @brief Frees and sets the parser pointer to NULL
 */
void optparse_del(optparse_t **parser);

/**
 * @brief Called implicitly by optparse_del()
 * 
 * I believe __attribute__((cleanup())) expects a "optparse_t ** parser" parameter,
 * but __attribute__((malloc())) expects "optparse_t * parser"
 * so we need to have this extra function.
 * 
 * @param parser pointer to free.
 */
void __optparse_cleanup(optparse_t * parser);

__attribute__((malloc, malloc(__optparse_cleanup, 1)))
optparse_t *optparse_new(const char *progname, const char *arg_summary);

int optparse_parse(optparse_t *parser, int argc, const char *argv[]);
void optparse_help(optparse_t *parser, FILE *fp);

optparse_opt_t *optparse_arg_optional(optparse_opt_t *opt);

optparse_opt_t *optparse_add_help(optparse_t *parser);

optparse_opt_t *optparse_add_set(optparse_t *parser, int short_opt, const char *long_opt, int value, int *ptr, char *help);

optparse_opt_t *optparse_add_int(optparse_t *parser, int short_opt, const char *long_opt, const char *arg_desc, unsigned base, int *ptr, char *help);

optparse_opt_t *optparse_add_unsigned(optparse_t *parser, int short_opt, const char *long_opt, const char *arg_desc, unsigned base, unsigned *ptr, char *help);

optparse_opt_t *optparse_add_double(optparse_t *parser, int short_opt, const char *long_opt, const char *arg_desc, double *ptr, char *help);

optparse_opt_t *optparse_add_string(optparse_t *parser, int short_opt, const char *long_opt, const char *arg_desc, char **ptr, char *help);

#endif /* _OPTPARSE_H */