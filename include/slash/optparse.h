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
 * @brief Create a new option parser context, remeber to free it using optparse_del() when done
 * @param progname name to be associated with the option parser
 * @param arg_summary short list of optional command line options and arguments
 * @return pointer to new context, NULL if something went wrong
 * @warning the progname and arg_summary arguments, if not NULL, must be valid for the entire lifecycle of the option parser!
 */
optparse_t *optparse_new(const char *progname, const char *arg_summary);

/**
 * @brief Create a new options parser context, remeber to free it using optparse_del() when done
 * @param progname name to be associated with the option parser
 * @param arg_summary short list of optional command line options and arguments, can be NULL if not relevant
 * @param help longer description of the command, can be NULL if not relevant.
 * @return pointer to new context, NULL if something went wrong
 * @warning the progname, arg_summary and help arguments, if not NULL, must be valid for the entire lifecycle of the option parser!
 */
optparse_t *optparse_new_ex(const char *progname, const char *arg_summary, const char *help);

/**
 * @brief Release the memory for the given option parser object
 * @param parser pointer to valid option parser obtained by calling optparse_new() or optparse_new_ex()
 */
void optparse_del(optparse_t *parser);

int optparse_parse(optparse_t *parser, int argc, const char *argv[]);
void optparse_help(optparse_t *parser, FILE *fp);

optparse_opt_t *optparse_arg_optional(optparse_opt_t *opt);

optparse_opt_t *optparse_add_help(optparse_t *parser);

optparse_opt_t *optparse_add_set(optparse_t *parser, int short_opt, const char *long_opt, int value, int *ptr, char *help);

optparse_opt_t *optparse_add_int(optparse_t *parser, int short_opt, const char *long_opt, const char *arg_desc, unsigned base, int *ptr, char *help);

optparse_opt_t *optparse_add_unsigned(optparse_t *parser, int short_opt, const char *long_opt, const char *arg_desc, unsigned base, unsigned *ptr, char *help);

optparse_opt_t *optparse_add_double(optparse_t *parser, int short_opt, const char *long_opt, const char *arg_desc, double *ptr, char *help);

optparse_opt_t *optparse_add_string(optparse_t *parser, int short_opt, const char *long_opt, const char *arg_desc, char **ptr, char *help);

/**
 * @brief interface to be implemented for custom option parsers
 * 
 * This function will be called by the optparse framework when it processes an
 * option for which this custom option value function was registered
 * @param data an opaque pointer where the parsed option value must be stored
 * @param arg the part of the command line containing the option value as a string to be parsed
 */
typedef int (*optparse_custom_func_t)(void *data, const char * arg);

/**
 * @brief Register an option with a custom parser (see the "func" param)
 * @param parser pointer to valid optparse object the option will be attached to
 * @param short_opt single character option (for example: 'c' for the corresponding "-c" cmd line option). Set to '\0' if you do not want a short option
 * @param long_opt string option (for example "test" for the corresponding "--test" cmd line option). Set to NULL if you do not want a long option
 * @param arg_desc string describing the argument (will be shown when help is invoked), may be NULL
 * @param help string describing waht this option is/does
 * @param func ptr to a cutom option value parser, see the optparse_custom_func_t typedef documentation
 * @param data opaque pointer to a valid memory where the "func" above will store the parsed option value
 * @return reference to newly created optparse_opt_t object
 */
optparse_opt_t *optparse_add_custom(optparse_t * parser, int short_opt, const char * long_opt, const char * arg_desc, const char * help, optparse_custom_func_t func, void * data);

#endif /* _OPTPARSE_H */