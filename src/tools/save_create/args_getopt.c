/* File: src/tools/save_create/args_getopt.c
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
#include "tools/save_create/args.h"

#include "util/strdup.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

static void usage()
{
    fprintf(stderr, "Usage: save_create [--help] DATABASE JSON_FILE [BUNDLES...]\n");
}

static struct option options[] = {
    { "help", 0, 0, 1000 },
    { }
};

int parse_args(
        struct arguments * args, int argc, char ** argv) [[gnu::nonnull(1)]]
{
    while (1) {
        int index = 0;
        int c = getopt_long(argc, argv, "", options, &index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 1000:
            case '?':
                usage();
                return 2;

            default:
                return 2;
        }
    }

    if (optind + 2 > argc) {
        usage();
        return 1;
    }

    args->database_name = util_strdup(argv[optind++]);
    args->json_name = util_strdup(argv[optind++]);
    if (optind < argc) {
        args->filenames = malloc(sizeof(*args->filenames) * (argc - optind));
        args->n_filenames = argc - optind;
        for (size_t i = 0; i < args->n_filenames; i++) {
            args->filenames[i] = util_strdup(argv[optind++]);
        }
    }

    return 0;
}
