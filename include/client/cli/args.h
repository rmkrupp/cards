/* File: include/client/cli/args.h
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
#ifndef CLIENT_CLI_ARGS
#define CLIENT_CLI_ARGS

#include "attributes.h"
#include <stdbool.h>
#include <stddef.h>

/* the result of parse_args */
struct arguments
{
    char * portname;
    char * hostname;
    char ** load_files;
    size_t n_load_files;
    bool send_exit;
};

/* parse this argv and argc, storing the result in args
 *
 * whether this invokes argp or getopt code depends on whether we are compiled
 * using src/client/cli/args_argp.c or src/client/cli/args_getopt.c,
 * which is controlled by the configure.py --disable-argp flag and --build
 * option (argp is off automatically for w64 builds.)
 */
int NONNULL(1) parse_args(
        struct arguments * args, int argc, char ** argv);

#endif /* CLIENT_CLI_ARGS */
