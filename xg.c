/* xg.c - main program
 *
 * Copyright (C) 2005, 2006 Momchil Velikov
 *
 * This file is part of XG.
 *
 * XG is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * XG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with XG; if not, write to the Free Software Foundation,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */
#include "grammar.h"
#include "lr0.h"
#include "lalr.h"
#include "gen-parser.h"
#include "xg.h"

#include <ulib/cache.h>
#include <ulib/options.h>
#include <stdio.h>

/* XG message log.  */
ulib_log *xg_log;

/* Instrument the parser for debugging info.  */
int xg_flag_debug = 0;

/* Prefix for all externally visible symbols.  */
const char *xg_namespace = 0;

/* Input file name.  */
const char *xg_input = 0;

/* Output file name.  */
const char *xg_output = 0;

/* Sentence size.  */
int xg_sentence_size = 10;

/* Output token codes, instead of token names.  */
int xg_flag_token_codes = 0;

/* Report file name.  */
const char *xg_report = 0;

/* Generate report.  */
int xg_flag_report = 0;

/* Output type.  */
enum output_type { output_defines = 1, output_slr, output_lalr, output_random_sentence };
int xg_flag_output_type = output_lalr;

static int
print_version() {
    fputs("xg (XG) 0.1 (alpha)\n", stderr);
    fputs("Copyright (C) 2005, 2006 Momchil Velikov\n", stderr);
    fputs(
        "This is free software; see the source for copying conditions."
        "  There is NO\n",
        stderr);
    fputs(
        "warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR"
        " PURPOSE.\n",
        stderr);
    return -1;
}

static ulib_option options[];
static int
print_usage() {
    fputs("usage: xg [<options>] <grammar>\n", stderr);
    fputs("Options are:\n", stderr);
    ulib_options_help(stderr, options);

    return -1;
}

static int
handle_namespace(const char *arg) {
    xg_namespace = arg;
    return 0;
}

static int
handle_output(const char *arg) {
    xg_output = arg;
    return 0;
}

static int
handle_sentence_size(const char *arg) {
    long sz;
    char *end;

    sz = strtol(arg, &end, 0);
    if (*end != 0) {
        fprintf(stderr, "xg: ERROR: invalid argument to --sentence\n");
        return -1;
    }
    xg_sentence_size = sz;
    return 0;
}

static int
handle_report(const char *arg) {
    xg_report = arg;
    return 0;
}

static int
handle_non_option_arg(const char *arg) {
    if (xg_input)
        fprintf(
            stderr, "xg: WARNING: input file ``%s'' overrides ``%s''\n", arg, xg_input);
    xg_input = arg;
    return 0;
}

/* Command line options.  */
static ulib_option options[]
    = {{.key = 'V',
        .name = "version",
        .cb = print_version,
        .help = "\toutput version information and exit"},

       {.key = 'h',
        .name = "help",
        .cb = print_usage,
        .help = "\t\tdisplay this help and exit"},

       {.key = 't',
        .name = "debug",
        .flag = &xg_flag_debug,
        .value = 1,
        .help = "\t\tinstrument the parser for debugging (TODO)"},

       {.key = 'n',
        .name = "namespace",
        .cb = handle_namespace,
        .flags = ulib_option_required_arg,
        .arg = "<name>",
        .help = "\n\t\t\tset the namespace for the external symbols (TODO)"},

       {.key = 'o',
        .name = "output",
        .cb = handle_output,
        .flags = ulib_option_required_arg,
        .arg = "<path>",
        .help = "\n\t\t\tname output file"},

       {.key = 'D',
        .name = "defines",
        .flag = &xg_flag_output_type,
        .value = output_defines,
        .help = "\toutput a header file with token #defines (TODO)"},

       {.key = 'S',
        .name = "slr",
        .flag = &xg_flag_output_type,
        .value = output_slr,
        .help = "\t\toutput an SLR(1) parser"},

       {.key = 'L',
        .name = "lalr",
        .flag = &xg_flag_output_type,
        .value = output_lalr,
        .help = "\t\toutput a LALR(1) parser"},

       {.key = 's',
        .name = "sentence",
        .flag = &xg_flag_output_type,
        .value = output_random_sentence,
        .help = "\tgenerate random sentence"},

       {.key = 'z',
        .name = "sentence-size",
        .cb = handle_sentence_size,
        .flags = ulib_option_required_arg,
        .arg = "<number>",
        .help = "\n\t\t\trandom sentence size"},

       {.key = 'c',
        .name = "codes",
        .flag = &xg_flag_token_codes,
        .value = 1,
        .help = "\t\tgenerate token codes, instead of token names"},

       {.key = 'r',
        .name = "report",
        .flag = &xg_flag_report,
        .value = 1,
        .cb = handle_report,
        .flags = ulib_option_optional_arg,
        .arg = "<path>",
        .help = "\n\t\t\tproduce a report with details on the parser"},

       {.key = 0, .cb = handle_non_option_arg}};

int
main(int argc, const char *argv[]) {
    int sts;
    static ulib_log log;
    xg_grammar *g = 0;
    xg_lr0dfa *dfa = 0;
    FILE *out;

    /* Create the message log.  */
    xg_log = &log;
    if (ulib_log_init(xg_log, "xg") < 0) {
        fprintf(stderr, "xg: ERROR: unable to create the message log.\n");
        return -1;
    }

    argv[0] = "xg";
    if (ulib_options_parse(options, argc, argv, stderr) < 0)
        return -1;

    /* Check options sanity.  */
    if (!xg_input) {
        fputs("xg: ERROR: missing input file name\n", stderr);
        return -1;
    }

    /* Initialize memory management.  */
    if (xg__init_grammar() < 0 || xg__init_lr0dfa() < 0 || xg__init_lalr() < 0)
        goto error;

    /* Parse the input file. */
    if ((g = xg_grammar_read(xg_input)) == 0)
        goto error;

    /* Compute FIRST and FOLLOW sets.  */
    if (xg_grammar_compute_first(g) < 0 || xg_grammar_compute_follow(g) < 0)
        goto error;

    /* Create the parsing automaton.  */
    if (xg_flag_output_type == output_slr || xg_flag_output_type == output_lalr) {
        if ((dfa = xg_lr0dfa_new(g)) == 0)
            goto error;

        if ((xg_flag_output_type == output_slr && xg_make_slr_reductions(g, dfa) < 0)
            || (xg_flag_output_type == output_lalr
                && xg_make_lalr_reductions(g, dfa) < 0))
            goto error;

        xg_resolve_conflicts(g, dfa);
    }

    /* Open the output file.  */
    if (xg_output == 0)
        out = stdout;
    else {
        if ((out = fopen(xg_output, "w")) == 0) {
            fprintf(stderr, "xg: ERROR: Cannot open output file  ``%s''\n", xg_output);
            goto error;
        }
    }

    /* Write the required output.  */
    if (xg_flag_output_type == output_random_sentence)
        xg_make_random_sentence(out, g, xg_sentence_size, xg_flag_token_codes);
    else if (xg_flag_output_type == output_defines) {
    } else {
        if (xg_gen_c_parser(out, g, dfa) < 0) {
            if (xg_output != 0) {
                fclose(out);
                goto error;
            }
        }
    }

    if (xg_output)
        fclose(out);

    /* Report details about the parser.  */
    if (xg_flag_report) {
        if (xg_report) {
            if ((out = fopen(xg_report, "w")) == 0)
                goto error;
        } else
            out = stderr;

        xg_grammar_debug(out, g);
        if (dfa)
            xg_lr0dfa_debug(out, g, dfa);

        if (xg_report)
            fclose(out);
    }

    sts = 0;
    goto exit;

error:
    sts = -1;
    if (xg_output)
        remove(xg_output);
exit:
    if (dfa)
        xg_lr0dfa_del(dfa);

    if (g)
        xg_grammar_del(g);
    ulib_gcrun();

    ulib_log_write(xg_log, stderr);
    ulib_log_destroy(xg_log);
    return sts;
}

/*
 * Local variables:
 * mode: C
 * indent-tabs-mode: nil
 * End:
 */
