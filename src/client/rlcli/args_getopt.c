/* File: src/client/rlcli/args_getopt.c
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

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

static void usage()
{
    fprintf(stderr, "Usage: rlcli [--help] [-p|--port PORT] [-h|--hostname HOSTNAME] [-l|--load FILE]\n");
}

static struct option options[] = {
    { "port", required_argument, 0, 'p' },
    { "hostname", required_argument, 0, 'h' },
    { "load", required_argument, 0, 'l' },
    { "help", optional_argument, 0, 1000 },
    { }
};

int NONNULL(1) parse_args(
        struct arguments * args, int argc, char ** argv)
{
    while (1) {
        int index = 0;
        int c = getopt_long(argc, argv, "p:h:", options, &index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 'p':
                free(args->portname);
                args->portname = util_strdup(optarg);
                break;

            case 'h':
                free(args->hostname);
                args->hostname = util_strdup(optarg);
                break;

            case 'l':
                args->load_files = realloc(
                        args->load_files,
                        sizeof(*args->load_files) * (args->n_load_files + 1)
                    );
                args->load_files[args->n_load_files] = util_strdup(optarg);
                args->n_load_files++;
                break;

            case 1000:
            case '?':
                usage();
                return 2;

            default:
                return 2;
        }
    }

    if (optind == argc) {
        return 0;
    } else {
        usage();
        return 1;
    }
}
