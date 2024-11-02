/* File: include/command/keyword.h
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
#ifndef COMMAND_KEYWORD_H
#define COMMAND_KEYWORD_H

#include <stddef.h>

enum keyword {
    KEYWORD_NO_MATCH,

    KEYWORD_SAY,
    KEYWORD_EXIT,
    KEYWORD_SHUTDOWN,

    KEYWORD_LIFE,

    KEYWORD_ENERGY,
    KEYWORD_SOURCES,

    KEYWORD_MY,
    KEYWORD_PLAYER,

    KEYWORD_HAND,
    KEYWORD_DECK,
    KEYWORD_DISCARD,
    KEYWORD_GRAVE,
    KEYWORD_ZONE,
    KEYWORD_ID,
    KEYWORD_SPECIAL,

    KEYWORD_POSITION,

    KEYWORD_CARD,
    KEYWORD_IN,

    KEYWORD_LOOK
};

struct keyword_lookup_result {
    int offset;
    enum keyword keyword;
};

/* offset->string function because we are using %pic */
const char * __attribute__ ((const))keyword_string(int offset);

/* this is the functions from gperf */
const struct keyword_lookup_result * __attribute__ ((const)) keyword_lookup(
        const char * str, size_t len) ;

#endif /* COMMAND_KEYWORD_H */
