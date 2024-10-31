/* File: src/client/rlcli/args_argp.c
 * Part of cards <github.com/rmkrupp/cards>
 *
 * Copyright (C) 2024 Noah Santer <n.ed.santer@gmail.com>
 * Copyright (C) 2024 Rebecca Krupp <beka.krupp@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "client/rlcli/args.h"

#include "util/strdup.h"

#include <stdlib.h>
#include <argp.h>

const char * argp_program_version =
    "rlcli " VERSION;

const char * argp_program_bug_address =
    "<beka.krupp@gmail.com>";

static char doc[] =
    "rlcli -- the readline-enabled client";

static struct argp_option options[] = {
    { "port", 'p', "PORT", 0,
        "Specify the port to connect to" },
    { "hostname", 'h', "HOST", 0,
        "Specify the hostname (or address) to connect to" },
    { "load", 'l', "FILE", 0,
        "Automatically load and send the contents of FILE (specify repeatedly "
        "for multiple files.)" },
    { }
};

static error_t parse_opt(int key, char * argv, struct argp_state * state)
{
    struct arguments * args = state->input;

    switch (key) {
        case 'p':
            free(args->portname);
            args->portname = util_strdup(argv);
            break;

        case 'h':
            free(args->hostname);
            args->hostname = util_strdup(argv);
            break;

        case 'l':
            args->load_files = realloc(
                    args->load_files,
                    sizeof(*args->load_files) * (args->n_load_files + 1)
                );
            args->load_files[args->n_load_files] = util_strdup(argv);
            args->n_load_files++;
            break;

        case ARGP_KEY_ARG:
            argp_usage(state);
            break;

        case ARGP_KEY_END:
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

int parse_args(
        struct arguments * args, int argc, char ** argv) [[gnu::nonnull(1)]]
{
    struct argp argp = (struct argp) {
        .options = options,
        .parser = parse_opt,
        .doc = doc
    };

    return argp_parse(&argp, argc, argv, ARGP_NO_EXIT, 0, args);
}
