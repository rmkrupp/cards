/* File: include/command/parse.h
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
#ifndef COMMAND_PARSE_H
#define COMMAND_PARSE_H

#include "attributes.h"
#include "lex.h"

enum command_type {
    COMMAND_SUBCOMMAND,
    COMMAND_SAY,
    COMMAND_EXIT,
    COMMAND_SHUTDOWN
};

struct command {
    enum command_type type;
    struct argument * arguments;
    struct command * subcommands;
    size_t n_subcommands;
};

struct parser * parser_create();
void NONNULL(1) parser_destroy(struct parser * parser);

enum parse_result_type {
    PARSE_OKAY,
    PARSE_ERROR
};

struct parse_result {
    enum parse_result_type type;
};

void NONNULL(1) parser_parse(
        struct parser * parser,
        struct particle_buffer * particles,
        struct parse_result * result
    );

#endif /* COMMAND_PARSE_H */
