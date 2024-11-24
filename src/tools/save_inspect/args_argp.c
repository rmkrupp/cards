/* File: src/tools/save_inspect/args_argp.c
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

#include <stdlib.h>
#include <argp.h>

const char * argp_program_version =
    "save_inspect " VERSION;

const char * argp_program_bug_address =
    "<beka.krupp@gmail.com>";

static char doc[] =
    "saves_inspect -- displays information about a save database";

static char args_doc[] =
    "DATABASE";

static struct argp_option options[] = {
    { "validate", 'v', NULL, 0,
        "validates the database after printing it" },
    { "metadata", 'm', NULL, 0,
        "displays data from the metadata table" },
    { "rules", 'r', NULL, 0,
        "displays data from the rules table"},
    { "players", 'p', NULL, 0,
        "displays data from the players table"},
    { "log", 'l', NULL, 0,
        "displays the data from the log table"},
    { "cards", 'c', NULL, 0,
        "displays data from the cards table" },
    { "key", 1000, "KEY", 0,
        "displays only those entries in the metadata or rules tables whose keys match KEY" },
    { "player", 1001, "ID", 0,
        "displays only the commands from the log whose player id matches ID" },
    { "checksum", 1002, "CHECKSUM", 0,
        "displays only cards that match the selected checksums" },
    { "filename", 1003, "FILENAME", 0,
        "displays only the cards that match the selected filename" },
    { "extract-bundle", 1004, "FILENAME", 0,
        "extracts the cards in the save as a bundle" },
    { "extract-json", 1005, "FILENAME", 0,
        "extracts a JSON representation of the save" },
    { "separator", 's', "SEPARATOR", 0,
        "print SEPARATOR between the columns in a row of the table (default: a space)"},
    { }
};

static error_t parse_opt(int key, char * argv, struct argp_state * state)
{
    struct arguments * args = state->input;

    switch (key) {
        case 'v':
            args->validate = true;
            break;

        case 's':
            free(args->sep);
            args->sep = util_strdup(argv);
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
                argp_usage(state);
                return 1;
            }
            args->key = util_strdup(argv);
            break;

        case 1001:
            if (args->id) {
                argp_usage(state);
                return 1;
            }
            args->id = util_strdup(argv);
            break;

        case 1002:
            if (args->checksum) {
                argp_usage(state);
                return 1;
            }
            args->checksum = util_strdup(argv);
            break;

        case 1003:
            if (args->filename) {
                argp_usage(state);
                return 1;
            }
            args->filename = util_strdup(argv);
            break;

        case 1004:
            if (args->bundle_name) {
                free(args->bundle_name);
            }
            args->bundle_name = util_strdup(argv);
            break;

        case 1005:
            if (args->json_file) {
                free(args->json_file);
            }
            args->json_file = util_strdup(argv);
            break;

        case ARGP_KEY_ARG:
            if (args->database_name) {
                return ARGP_ERR_UNKNOWN;
            } else {
                args->database_name = util_strdup(argv);
            }
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
