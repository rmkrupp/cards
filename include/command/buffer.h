/* File: include/command/lex.h
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
#ifndef COMMAND_BUFFER_H
#define COMMAND_BUFFER_H

#include "command/keyword.h"

#include <stddef.h>

/* the different types of particles */
enum particle_type {
    PARTICLE_END, /* always the last particle in a complete command
                   * in general, corresponds to a newline
                   */
    PARTICLE_KEYWORD, /* a keyword, has .value of string of the keyword,
                       * and a .keyword based on a lookup of that string
                       */
    PARTICLE_NUMBER, /* an (integer) number, has a .value of the string form of
                      * itself
                      */
    PARTICLE_NAME, /* a name, the .value is the string form without the ""s,
                    * the .name is the result of matching against the set
                    * supplied to the lexer
                    */
    PARTICLE_BEGIN_NEST, /* the open paren */
    PARTICLE_END_NEST /* the close paren */
};

/* a single particle of a type and value */
struct particle {
    enum particle_type type;
    char * value;
    size_t length;

    enum keyword keyword;
    const struct name * name;
};

/* a buffer for storing the results of a lex() call */
struct particle_buffer {
    struct particle ** particles; /* the particles generated */
    size_t n_particles; /* the number of particles */
    size_t capacity; /* the capacity of the buffer */
};

/* create a new buffer */
[[nodiscard]] struct particle_buffer * particle_buffer_create();

/* destroy this buffer */
void particle_buffer_destroy(
        struct particle_buffer * buffer) [[gnu::nonnull(1)]];

/* free every particle in this buffer and set buffer->n_particles to zero */
void particle_buffer_free_all(
        struct particle_buffer * buffer) [[gnu::nonnull(1)]];

/* grow buffer->capacity by amount */
void particle_buffer_grow(
        struct particle_buffer * buffer, size_t amount) [[gnu::nonnull(1)]];

/* if buffer->capacity < minimum, grow to (at least) minimum
 *
 * NOTE: the current implementation grows to exactly minimum, no matter
 *       what PARTICLE_BUFFER_GROW_INCREMENT is
 */
void particle_buffer_at_least(
        struct particle_buffer * buffer, size_t minimum) [[gnu::nonnull(1)]];

/* add this particle to this buffer, growing the buffer if necessary
 *
 * NOTE: again, allow a null particle here
 * TODO: should we? (unlike particle_string() there's no code change between
 *       allowing and not allowing for this function)
 */
void particle_buffer_add(
        struct particle_buffer * buffer,
        struct particle * particle
    ) [[gnu::nonnull(1)]];

#endif /* COMMAND_BUFFER_H */
