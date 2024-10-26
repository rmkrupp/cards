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

#include "refstring.h"

//struct lexer * lexer_create();
//void lexer_destroy(struct lexer * lexer);

/* the different types of particles */
enum particle_type {
    PARTICLE_END, /* always the last particle in a complete command
                   * in general, corresponds to a newline
                   */
    PARTICLE_KEYWORD, /* a keyword, has a value of the keyword */
    PARTICLE_NUMBER, /* a (integer) number, has a value of the string form of
                      * itself
                      */
    PARTICLE_NAME, /* a name, the value is the string form without the ""s */
    PARTICLE_BEGIN_NEST, /* the open paren */
    PARTICLE_END_NEST /* the close paren */
};

/* a single particle of a type and value */
struct particle {
    enum particle_type type;
    char * value;
};

/* return a new particle of type */
struct particle * particle_create(enum particle_type type);

/* destroy this particle */
void particle_destroy(struct particle * particle);

/* return a refstring describing the particle
 *
 * the caller is the owner of the refstring, and so must call
 * refstring_destroy() on it when finished
 */
struct refstring * particle_string(struct particle * particle);

/* the type of lex results
 *
 * TODO: expand to handle errors in a nuanced way
 */
enum lex_result_type {
    LEX_OKAY,
    LEX_ERROR
};

/* the result of a lex command */
struct lex_result {
    enum lex_result_type type; /* OKAY or ERROR */
    size_t index; /* the index (in characters) consumed
                   * if OKAY, index equals the size of the buffer
                   * if ERROR, points to right after the last thing consumed
                   */
};

/* a buffer for storing the results of a lex() call */
struct particle_buffer {
    struct particle ** particles; /* the particles generated */
    size_t n_particles; /* the number of particles */
    size_t capacity; /* the capacity of the buffer */
};

/* create a new buffer */
struct particle_buffer * particle_buffer_create();

/* destroy this buffer */
void particle_buffer_destroy(struct particle_buffer * buffer);

/* free every particle in this buffer and set buffer->n_particles to zero */
void particle_buffer_free_all(struct particle_buffer * buffer);

/* grow buffer->capacity by amount */
void particle_buffer_grow(struct particle_buffer * buffer, size_t amount);

/* if buffer->capacity < minimum, grow to (at least) minimum
 *
 * NOTE: the current implementation grows to exactly minimum, no matter
 *       what PARTICLE_BUFFER_GROW_INCREMENT is
 */
void particle_buffer_at_least(struct particle_buffer * buffer, size_t minimum);

/* turn input string into particles
 *
 * buffer must be a particle buffer
 * if it is not empty, the particles will be appended,
 * thus is must be emptied after the particles have been consumed
 * (e.g. via a call to particle_buffer_free_all())
 *
 * otherwise, the caller must track its own offset into the buffer
 *
 * the result status is written to result_out
 */
void lex(
        const char * input,
        struct particle_buffer * buffer,
        struct lex_result * result_out
    );

#endif /* COMMAND_LEX_H */
