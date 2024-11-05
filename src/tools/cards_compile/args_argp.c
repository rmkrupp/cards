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
#include "tools/cards_compile/args.h"

#include "util/strdup.h"

#include <stdlib.h>
#include <argp.h>

const char * argp_program_version =
    "cards_compile " VERSION;

const char * argp_program_bug_address =
    "<beka.krupp@gmail.com>";

static char doc[] =
    "cards_compile -- bundle card scripts";

static char args_doc[] =
    "DATABASE [CARDS...]";

static struct argp_option options[] = {
    { "append", 'a', NULL, 0,
        "Append to the bundle" },
    { }
};

static error_t parse_opt(int key, char * argv, struct argp_state * state)
{
    struct arguments * args = state->input;

    switch (key) {
        case 'a':
            args->append = true;
            break;

        case ARGP_KEY_ARG:
            if (args->database_name) {
                args->filenames = realloc(
                        args->filenames,
                        sizeof(*args->filenames) * (args->n_filenames + 1)
                    );
                args->filenames[args->n_filenames] = util_strdup(argv);
                args->n_filenames++;
            } else {
                args->database_name = util_strdup(argv);
            }
            break;

        case ARGP_KEY_END:
            break;

        case ARGP_KEY_ERROR:
            for (size_t i = 0; i < args->n_filenames; i++) {
                free(args->filenames[i]);
            }
            free(args->filenames);
            free(args->database_name);
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    if (!args->database_name) {
        argp_usage(state);
        return 1;
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
