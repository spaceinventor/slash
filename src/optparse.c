/* implementation of the optparse API

   Copyright (c) 2009-2014 Christer Weinigel <christer@weinigel.se>.

   This software is licensed under the MIT License.
*/

#include <slash/optparse.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

enum OPTPARSE_FLAGS {
	OPTPARSE_FLAG_OPTIONAL = (1 << 0),
};

typedef int (*optparse_func_t)(optparse_opt_t * opt, const char * arg);

struct optparse {
	const char * progname;

	const char * arg_summary;

	optparse_opt_t * options;
	optparse_opt_t ** last_option;

	optparse_opt_t * current_option;

	int argi;
	int argc;
	const char ** argv;
	const char *help;
	void * ptr;
};

struct optparse_opt {
	optparse_t * parser;
	optparse_opt_t * next;

	int short_opt;
	const char * long_opt;
	unsigned long_opt_len;
	const char * arg_desc;
	const char * help;
	unsigned flags;

	optparse_func_t func;
	void * data;

	union {
		unsigned num_base;
		int set_value;
	} spec;
	bool should_free_data;
};

optparse_t *
optparse_new(const char * progname, const char * arg_summary) {
	return optparse_new_ex(progname, arg_summary, NULL);
}

optparse_t *optparse_new_ex(const char *progname, const char *arg_summary, const char *help) {
	optparse_t * parser;

	parser = malloc(sizeof(*parser));
	memset(parser, 0, sizeof(*parser));

	parser->last_option = &parser->options;

	if (!progname)
		progname = "unknown";
	parser->progname = progname;

	if (arg_summary)
		parser->arg_summary = arg_summary;

	parser->help = help;

	return parser;
}

void optparse_del(optparse_t * parser) {
	optparse_opt_t * opt;

	while ((opt = parser->options)) {
		parser->options = opt->next;
		if(opt->should_free_data) {
			free(opt->data);
		}
		free(opt);
	}
	free(parser);
}

static int
handle_long_opt(optparse_t * parser, const char * s) {
	optparse_opt_t * opt;

	for (opt = parser->options; opt; opt = opt->next) {
		if (!strncmp(s, opt->long_opt, opt->long_opt_len)) {
			if (!s[opt->long_opt_len]) {
				if (opt->arg_desc && !(opt->flags & OPTPARSE_FLAG_OPTIONAL)) {
					fprintf(stderr, "%s: \"--%s\" requires an argument\n",
							parser->progname, s);
					return 0;
				}

				return opt->func(opt, NULL);
			} else if (s[opt->long_opt_len] == '=') {
				if (!opt->arg_desc) {
					fprintf(stderr, "%s: \"--%s\" does not take an argument\n",
							parser->progname, s);
					return 0;
				}

				return opt->func(opt, s + opt->long_opt_len + 1);
			}
		}
	}

	fprintf(stderr, "%s: invalid option \"--%s\"\n",
			parser->progname, s);
	fprintf(stderr, "Try \"%s --help\" for more information.\n",
			parser->progname);

	return 0;
}

static int
handle_short_opt(optparse_t * parser, char c, char c2) {
	optparse_opt_t * opt;

	for (opt = parser->options; opt; opt = opt->next) {
		if (opt->short_opt && c == opt->short_opt) {
			if (opt->arg_desc) {
				if (parser->argi > parser->argc) {
					fprintf(stderr, "%s: \"-%c\" requires an argument\n", parser->progname, c);
					return 0;
				}
				if (c2 == '\0') {
					if (parser->argi >= parser->argc) {
						fprintf(stderr, "%s: \"-%c\" requires an argument\n", parser->progname, c);
						return 0;
					}
					return opt->func(opt, parser->argv[parser->argi++]);
				} else {
					return opt->func(opt, parser->argv[parser->argi - 1] + 2);
				}
			} else {
				return opt->func(opt, NULL);
			}
		}
	}

	fprintf(stderr, "%s: invalid option \"-%c\"\n",
			parser->progname, c);
	fprintf(stderr, "Try \"%s --help\" for more information.\n",
			parser->progname);

	return 0;
}

int optparse_parse(optparse_t * parser, int argc, const char * argv[]) {
	parser->argc = argc;
	parser->argv = argv;

	parser->argi = 0;
	while (parser->argi < parser->argc) {
		const char * s = argv[parser->argi++];
		if(!s) {
			break;
		}
		if (s[0] == '-' && s[1] == '-') {
			if (!s[2])
				break;

			if (!handle_long_opt(parser, s + 2))
				return -1;
		} else if (s[0] == '-') {
			if (!s[1])
				break;

			if (!handle_short_opt(parser, s[1], s[2]))
				return -1;
		} else {
			parser->argi--;
			break;
		}
	}

	return parser->argi;
}

static optparse_opt_t *
optparse_opt_new(optparse_t * parser,
				 int short_opt, const char * long_opt, const char * arg_desc,
				 const char * help,
				 optparse_func_t func, void * data) {
	optparse_opt_t * opt;

	opt = malloc(sizeof(*opt));
	memset(opt, 0, sizeof(*opt));

	opt->parser = parser;
	opt->short_opt = short_opt;
	opt->long_opt = long_opt;
	if (opt->long_opt)
		opt->long_opt_len = strlen(opt->long_opt);
	opt->arg_desc = arg_desc;
	opt->help = help;
	opt->func = func;
	opt->data = data;
	opt->should_free_data = false;

	*parser->last_option = opt;
	parser->last_option = &opt->next;

	return opt;
}

struct custom_opt_ctx {
	optparse_custom_func_t func;
	void *org_data;
};

static int optparse_custom_func_trampoline(optparse_opt_t * opt, const char * arg) {
	struct custom_opt_ctx * custom_ctx = opt->data;
	return custom_ctx->func(custom_ctx->org_data, arg);
}

optparse_opt_t *optparse_add_custom(optparse_t * parser, int short_opt, const char * long_opt, const char * arg_desc, const char * help, optparse_custom_func_t func, void * data) {
	optparse_opt_t * opt = NULL;
	struct custom_opt_ctx *custom_ctx = calloc(1, sizeof(*custom_ctx));
	if (custom_ctx) {
		custom_ctx->func = func;
		custom_ctx->org_data = data;
		opt = malloc(sizeof(*opt));
		if(opt) {
			memset(opt, 0, sizeof(*opt));

			opt->parser = parser;
			opt->short_opt = short_opt;
			opt->long_opt = long_opt;
			if (opt->long_opt)
				opt->long_opt_len = strlen(opt->long_opt);
			opt->arg_desc = arg_desc;
			opt->help = help;
			opt->func = optparse_custom_func_trampoline;
			opt->data = custom_ctx;
			opt->should_free_data = true;
			*parser->last_option = opt;
			parser->last_option = &opt->next;
		} else {
			free(custom_ctx);
		}
	}
	return opt;
}

optparse_opt_t *
optparse_arg_optional(optparse_opt_t * opt) {
	opt->flags |= OPTPARSE_FLAG_OPTIONAL;

	return opt;
}

void optparse_help(optparse_t * parser, FILE * fp) {
	optparse_opt_t * opt;

	fprintf(fp, "Usage: %s [OPTIONS...]", parser->progname);
	if (parser->arg_summary)
		fprintf(fp, " %s", parser->arg_summary);
	fprintf(fp, "\n\n");

	if (parser->help) {
		fprintf(fp, "%s", parser->help);
		fprintf(fp, "\n\n");
	}

	for (opt = parser->options; opt; opt = opt->next) {
		int col = 0;
		col += fprintf(fp, "  ");
		if (opt->short_opt) {
			col += fprintf(fp, "-%c", opt->short_opt);
			if (opt->long_opt)
				col += fprintf(fp, ", ");
			else if (opt->arg_desc)
				col += fprintf(fp, " %s", opt->arg_desc);
			else
				col += fprintf(fp, "  ");
		} else
			col += fprintf(fp, "    ");
		if (opt->long_opt) {
			col += fprintf(fp, "--%s", opt->long_opt);
			if (opt->arg_desc)
				col += fprintf(fp, "=%s", opt->arg_desc);
		}

		if (opt->help) {
			if (col >= 28) {
				putc('\n', fp);
				col = 0;
			}
			while (col++ < 28)
				putc(' ', fp);
			col += fprintf(fp, "%s", opt->help);
		}
		fprintf(fp, "\n");
	}
}

static int
optparse_handle_help(optparse_opt_t * opt, const char * arg) {
	optparse_help(opt->parser, stdout);
	return 0;
}

optparse_opt_t *
optparse_add_help(optparse_t * parser) {
	return optparse_opt_new(parser,
							'h', "help", NULL, "display this help",
							optparse_handle_help, NULL);
}

static int
optparse_handle_set(optparse_opt_t * opt, const char * arg) {
	int * ptr = opt->data;

	*ptr = opt->spec.set_value;

	return 1;
}

optparse_opt_t *
optparse_add_set(optparse_t * parser,
				 int short_opt, const char * long_opt, int value,
				 int * ptr, char * help) {
	optparse_opt_t * opt;

	opt = optparse_opt_new(parser,
						   short_opt, long_opt, NULL, help,
						   optparse_handle_set, ptr);

	opt->spec.set_value = value;

	return opt;
}

static int
optparse_handle_int(optparse_opt_t * opt, const char * arg) {
	int * ptr = opt->data;
	long l;
	char * p;

	l = strtol(arg, &p, opt->spec.num_base);
	if (*p || l < INT_MIN || l > INT_MAX) {
		fprintf(stderr, "%s: invalid number \"%s\"\n",
				opt->parser->progname, arg);
		return 0;
	}

	*ptr = l;

	return 1;
}

optparse_opt_t *
optparse_add_int(optparse_t * parser,
				 int short_opt, const char * long_opt,
				 const char * arg_desc, unsigned base,
				 int * ptr, char * help) {
	optparse_opt_t * opt;

	if (!arg_desc)
		arg_desc = "NUM";

	opt = optparse_opt_new(parser,
						   short_opt, long_opt, arg_desc, help,
						   optparse_handle_int, ptr);

	opt->spec.num_base = base;

	return opt;
}

static int
optparse_handle_unsigned(optparse_opt_t * opt, const char * arg) {
	unsigned * ptr = opt->data;
	unsigned long l;
	char * p;

	l = strtoul(arg, &p, opt->spec.num_base);
	if (*p || l > UINT_MAX) {
		fprintf(stderr, "%s: invalid number \"%s\"\n",
				opt->parser->progname, arg);
		return 0;
	}

	*ptr = l;

	return 1;
}

optparse_opt_t *
optparse_add_unsigned(optparse_t * parser,
					  int short_opt, const char * long_opt,
					  const char * arg_desc, unsigned base,
					  unsigned * ptr, char * help) {
	optparse_opt_t * opt;

	if (!arg_desc)
		arg_desc = "NUM";

	opt = optparse_opt_new(parser,
						   short_opt, long_opt, arg_desc, help,
						   optparse_handle_unsigned, ptr);

	opt->spec.num_base = base;

	return opt;
}

static int
optparse_handle_double(optparse_opt_t * opt, const char * arg) {
	double * ptr = opt->data;
	double long d;
	char * p;

	d = strtod(arg, &p);
	if (*p) {
		fprintf(stderr, "%s: invalid number \"%s\"\n",
				opt->parser->progname, arg);
		return 0;
	}

	*ptr = d;

	return 1;
}

optparse_opt_t *
optparse_add_double(optparse_t * parser,
					int short_opt, const char * long_opt,
					const char * arg_desc, double * ptr, char * help) {
	optparse_opt_t * opt;

	if (!arg_desc)
		arg_desc = "NUM";

	opt = optparse_opt_new(parser,
						   short_opt, long_opt, arg_desc, help,
						   optparse_handle_double, ptr);

	return opt;
}

static int
optparse_handle_string(optparse_opt_t * opt, const char * arg) {
	const char ** ptr = opt->data;

	*ptr = arg;

	return 1;
}

optparse_opt_t *
optparse_add_string(optparse_t * parser,
					int short_opt, const char * long_opt, const char * arg_desc,
					char ** ptr, char * help) {
	if (!arg_desc)
		arg_desc = "STRING";

	return optparse_opt_new(parser,
							short_opt, long_opt, arg_desc, help,
							optparse_handle_string, ptr);
}