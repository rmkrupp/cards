/* File: include/tools/cards_inspect/args.h
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
#ifndef TOOLS_CARDS_INSPECT_ARGS
#define TOOLS_CARDS_INSPECT_ARGS

#include <stddef.h>
#include <stdbool.h>

/* the result of parse_args */
struct arguments
{
    bool validate;
    char * database_name;
};

/* parse this argv and argc, storing the result in args
 *
 * whether this invokes argp or getopt code depends on whether we are
 * compiled using src/tools/cards_compile/args_argp.c or
 * src/tools/cards_compile/args_getopt.c, which is controlled by the
 * configure.py --use-argp flag, and the --build option (argp is off
 * automatically for w64 builds.)
 */
int parse_args(
        struct arguments * args, int argc, char ** argv) [[gnu::nonnull(1)]];

#endif /* TOOLS_CARDS_INSPECT_ARGS */
