/* File: src/tools/cards_compile/args_getopt.c
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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

static void usage()
{
    fprintf(stderr, "Usage: cards_compile [--help] [-v|--validate] DATABASE\n");
}

static struct option options[] = {
    { "validate", 0, 0, 'v' },
    { "help", 0, 0, 1000 },
    { }
};

int parse_args(
        struct arguments * args, int argc, char ** argv) [[gnu::nonnull(1)]]
{
    while (1) {
        int index = 0;
        int c = getopt_long(argc, argv, "v", options, &index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 'v':
                args->validate = true;
                break;

            case 1000:
            case '?':
                usage();
                return 2;

            default:
                return 2;
        }
    }

    if ((optind + 1) != argc) {
        usage();
        return 1;
    }

    args->database_name = util_strdup(argv[optind]);
    optind++;

    return 0;
}
