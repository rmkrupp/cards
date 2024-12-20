/* File: src/tools/cards_compile/args_argp.c
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
#include "tools/cards_inspect/args.h"

#include "util/strdup.h"

#include <stdlib.h>
#include <argp.h>

const char * argp_program_version =
    "cards_inspect " VERSION;

const char * argp_program_bug_address =
    "<beka.krupp@gmail.com>";

static char doc[] =
    "cards_inspect -- list (and optionally validate) a card bundle";

static char args_doc[] =
    "DATABASE";

static struct argp_option options[] = {
    { "validate", 'v', NULL, 0,
        "Validate the each Lua script" },
    { }
};

static error_t parse_opt(int key, char * argv, struct argp_state * state)
{
    struct arguments * args = state->input;

    switch (key) {
        case 'v':
            args->validate = true;
            break;

        case ARGP_KEY_ARG:
            if (args->database_name) {
                return ARGP_ERR_UNKNOWN;
            }
            args->database_name = util_strdup(argv);
            break;

        case ARGP_KEY_END:
            if (!args->database_name) {
                argp_usage(state);
                return 1;
            }
            break;

        case ARGP_KEY_ERROR:
            free(args->database_name);
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
        .doc = doc,
        .args_doc = args_doc
    };

    return argp_parse(&argp, argc, argv, 0, 0, args);
}
