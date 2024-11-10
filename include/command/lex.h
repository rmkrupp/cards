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
#ifndef COMMAND_LEX_H
#define COMMAND_LEX_H

#include <stddef.h>
#include <unitypes.h>

#include "command/keyword.h"

/* forward declare these */
struct refstring;
struct particle_buffer;
struct name_set;

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
    PARTICLE_END_NEST, /* the close paren */

    PARTICLE_ERROR /* the value is the lexical input from the beginning of the
                    * error until the input recovers (i.e. there's a \n,
                    * whitespace after a KEYWORD or NUMBER, the " to close a
                    * NAME, or just the next character if the type couldn't be
                    * otherwise determined)
                    * TODO: make add the above functionality
                    *
                    * if VERBOSE_LEXER, .error is filled with an error message
                    * describing the error, otherwise it is NULL
                    */
};

/* a single particle of a type and value */
struct particle {
    enum particle_type type;
    uint8_t * value;
    size_t length;

    enum keyword keyword;
    struct name * name;

    uint8_t * error;
    size_t error_length;
};

/* return a new particle of type */
[[nodiscard]] struct particle * particle_create(enum particle_type type);

/* create a particle with the given type and and value
 *
 * makes a duplicate of the value, consuming at exactly n bytes from the
 * argument
 */
[[nodiscard]] struct particle * particle_create_value(
        enum particle_type type, const uint8_t * value, size_t n);

/* destroy this particle */
void particle_destroy(struct particle * particle) [[gnu::nonnull(1)]];

/* return a refstring describing the particle
 *
 * the caller is the owner of the refstring, and so must call
 * refstring_destroy() on it when finished
 *
 * note: this CAN accept a NULL particle
 * TODO: do we need this?
 */
[[nodiscard]] struct refstring * particle_string(struct particle * particle);

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

/* inputs to a lexer */
struct lexer_input {
    const uint8_t * input;
    size_t length;
};

/* turn this list of inputs into particles and puts them into the particle
 * buffer (appending them after any particles already there)
 *
 * this does not modify the inputs or the data pointed to by the inputs, and
 * they need not stay valid after this call except for any input that was not
 * consumed (see return value)
 *
 * name_set is used for matching PARTICLE_NAME tokens. at some point, keywords
 * may match against a provided set, but for now they don't
 *
 * returns the total bytes consumed across all inputs, which may be less than
 * the size of all the inputs
 */
size_t lex(
        const struct lexer_input * inputs,
        size_t n_inputs,
        struct name_set * name_set,
        struct particle_buffer * buffer
    ) [[gnu::nonnull(1, 3, 4)]];

#endif /* COMMAND_LEX_H */

