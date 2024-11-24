/* File: src/tools/save_inspect/args_getopt.c
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
#include "tools/save_inspect/args.h"

#include "util/strdup.h"

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

static void usage()
{
    fprintf(stderr, "Usage: save_inspect [--help] [-v|--validate] [-m|--metadata] [-r|--rules] [-p|--players] [-l|--log] [-c|--cards] [-s|--separator SEPARATOR] [--key KEY] [--checksum CHECKSUM] [--filename FILENAME] [--extract-bundle FILENAME] [--extract-json FILENAME] DATABASE\n");
}

static struct option options[] = {
    { "validate", 0, 0, 'v' },
    { "metadata", 0, 0, 'm' },
    { "rules", 0, 0, 'r' },
    { "players", 0, 0, 'p' },
    { "log", 0, 0, 'l' },
    { "cards", 0, 0, 'c' },
    { "key", required_argument, 0, 1000 },
    { "player", required_argument, 0, 1001 },
    { "checksum", required_argument, 0, 1002 },
    { "filename", required_argument, 0, 1003 },
    { "extract-json", required_argument, 0, 1004 },
    { "extract-bundle", required_argument, 0, 1005 },
    { "separator", required_argument, 0, 's' },
    { "help", 0, 0, 1006 },
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
            case 'v':
                args->validate = true;
                break;

            case 's':
                free(args->sep);
                args->sep = util_strdup(optarg);
                break;

            case 'm':
                args->want_metadata = true;
                break;

            case 'r':
                args->want_rules = true;
                break;

            case 'p':
                args->want_players = true;
                break;

            case 'l':
                args->want_log = true;
                break;

            case 'c':
                args->want_cards = true;
                break;

            case 1000:
                if (args->key) {
                    usage();
                    return 1;
                }
                args->key = util_strdup(optarg);
                break;

            case 1001:
                if (args->id) {
                    usage();
                    return 1;
                }
                args->id = util_strdup(optarg);
                break;

            case 1002:
                if (args->checksum) {
                    usage();
                    return 1;
                }
                args->checksum = util_strdup(optarg);
                break;

            case 1003:
                if (args->filename) {
                    usage();
                    return 1;
                }
                args->filename = util_strdup(optarg);
                break;

            case 1004:
                if (args->bundle_name) {
                    usage();
                    return 1;
                }
                args->bundle_name = util_strdup(optarg);
                break;

            case 1005:
                if (args->json_file) {
                    usage();
                    return 1;
                }
                args->json_file = util_strdup(optarg);
                break;

            case 1006:
            case '?':
                usage();
                return 2;

            default:
                return 2;
        }
    }

    if (optind < argc) {
        usage();
        return 1;
    }

    args->database_name = util_strdup(argv[optind]);
    optind++;

    return 0;
}
