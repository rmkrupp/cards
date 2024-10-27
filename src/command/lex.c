/* File: src/lex.c
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
#include "command/lex.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "util/strdup.h"

/* GENERAL NOTES ON THE LEXER
 *
 * 1) Right now, the Lexer does not support interpreting particles across the
 *    boundary from one call of lex() to another. Changing this could be done
 *    in a few ways, and would involve not interpreting \0 as an end-like
 *    character (it already doesn't create an END token)
 *
 *    Either the lexer would need to maintain state (and thus require some
 *    sort of lexer object, or hidden state in a particlebuffer), or it would
 *    need to communicate using the current returned lexer_result that it wants
 *    to be given some of the old buffer again.
 *
 *    One option could be to accept an *array* of strings to go over, so that
 *    the caller could just provide a slice of the previous input, but this
 *    would still complicate things because there's no guarantee that the
 *    particle isn't some enormous things stretching over multiple buffers...
 *
 *    The fact is, the server is going to buffer network input and retrieve
 *    it in newline-terminated chunks through libevent already, so adding extra
 *    logic isn't going to change anything. The more important piece is 2)
 *    which wouldn't require this type of change.
 *
 * 2) We need a way to handle embedded newlines in multi-line blocks for
 *    trigger conditions. Probably a [ and ] particle will need to be added
 *    to hold a series of further particles (and END tokens) and this will
 *    be fine from the lexer's POV, with the true logic in the parser.
 *
 *    We will also need some way to decide whether ()s inside []s are executed
 *    now or saved as part of the trigger condition and run when the conditon
 *    script is.
 *
 * 3) Performance! This has not been tested (TODO: test it.)
 *
 *    Some thoughts: it may be faster to use a lookup table and state changes
 *    versus the current "is it this?" "no? is it this?" setup.
 */

/* how much to grow the particle_buffer by, in number of particles,
 * every time its capacity is exceeded.
 */
#ifndef PARTICLE_BUFFER_GROW_INCREMENT
#define PARTICLE_BUFFER_GROW_INCREMENT 64
#endif /* PARTICLE_BUFFER_GROW_INCREMENT */

static_assert(PARTICLE_BUFFER_GROW_INCREMENT > 0);

/* create a particle with the given type and NULL value */
struct particle * particle_create(enum particle_type type)
{
    struct particle * particle = malloc(sizeof(*particle));
    *particle = (struct particle) {
        .type = type
    };
    return particle;
}

/* create a particle with the given type and and value
 *
 * makes a duplicate of the value, consuming at most n bytes from the argument
 */
struct particle * particle_create_value(
        enum particle_type type, const char * value, size_t n)
{
    struct particle * particle = malloc(sizeof(*particle));
    *particle = (struct particle) {
        .type = type,
        .value = util_strndup(value, n)
    };
    return particle;
}

/* destroy this particle */
void particle_destroy(struct particle * particle)
{
    free(particle->value);
    free(particle);
}

/* return a refstring describing the particle
 *
 * the caller is the owner of the refstring, and so must call
 * refstring_destroy() on it when finished
 */
struct refstring * particle_string(struct particle * particle)
{
    if (!particle) {
        return refstring_createf("NULL");
    }

    switch (particle->type) {
        case PARTICLE_END:
            return refstring_createf("END");

        case PARTICLE_KEYWORD:
            return refstring_createf("KEYWORD<%s>", particle->value);

        case PARTICLE_NUMBER:
            return refstring_createf("NUMBER<%s>", particle->value);

        case PARTICLE_NAME:
            return refstring_createf("NAME<%s>", particle->value);

        case PARTICLE_BEGIN_NEST:
            return refstring_createf("(");

        case PARTICLE_END_NEST:
            return refstring_createf(")");

        default:
            return refstring_create("UNKNOWN");
    }
}

/* create a new buffer */
struct particle_buffer * particle_buffer_create()
{
    struct particle_buffer * buffer = malloc(sizeof(*buffer));
    *buffer = (struct particle_buffer) { };
    return buffer;
}

/* destroy this buffer */
void particle_buffer_destroy(struct particle_buffer * buffer)
{
    particle_buffer_free_all(buffer);
    free(buffer->particles);
    free(buffer);
}

/* free every particle in this buffer and set buffer->n_particles to zero */
void particle_buffer_free_all(struct particle_buffer * buffer)
{
    for (size_t n = 0; n < buffer->n_particles; n++) {
        particle_destroy(buffer->particles[n]);
    }
    buffer->n_particles = 0;
}

/* if buffer->capacity < minimum, grow to (at least) minimum
 *
 * NOTE: the current implementation grows to exactly minimum, no matter
 *       what PARTICLE_BUFFER_GROW_INCREMENT is
 */
void particle_buffer_grow(struct particle_buffer * buffer, size_t amount)
{
    buffer->particles = realloc(
            buffer->particles,
            sizeof(*buffer->particles) * (buffer->capacity + amount)
        );
    buffer->capacity += amount;
}

/* if buffer->capacity < minimum, grow to (at least) minimum
 *
 * NOTE: the current implementation grows to exactly minimum, no matter
 *       what PARTICLE_BUFFER_GROW_INCREMENT is
 */
void particle_buffer_at_least(struct particle_buffer * buffer, size_t minimum)
{
    if (minimum > buffer->capacity) {
        particle_buffer_grow(buffer, minimum - buffer->capacity);
    }
}

/* */
void particle_buffer_add(
        struct particle_buffer * buffer, struct particle * particle)
{
    if (buffer->n_particles == buffer->capacity) {
        particle_buffer_grow(buffer, PARTICLE_BUFFER_GROW_INCREMENT);
    }
    buffer->particles[buffer->n_particles] = particle;
    buffer->n_particles++;
}

/* subfunction of lex() */
static struct particle * consume_end_nest(const char * input, size_t * n)
{
    if (input[*n + 1] == '\0'
            || input[*n + 1] == ' '
            || input[*n + 1] == '\n'
            || input[*n + 1] == ')') {
        return particle_create(PARTICLE_END_NEST);
    } else {
        return NULL;
    }
}

/* subfunction of lex() */
static struct particle * consume_name(const char * input, size_t * n)
{
    for (size_t i = *n + 1; ; i++) {
        if (!input[i]) {
            return NULL;
        }
        if (input[i] == '"') {
            struct particle * particle = particle_create(PARTICLE_NAME);
            particle->value = util_strndup(&input[*n + 1], i - *n - 1);
            *n = i;
            return particle;
        }
    }
}

/* subfunction of lex() */
static struct particle * consume_number(const char * input, size_t * n)
{
    for (size_t i = *n + 1; ; i++) {
        if (!input[i]) {
            return NULL;
        }
        if (input[i] == ' ' || input[i] == '\n' || input[i] == ')') {
            struct particle * particle = particle_create(PARTICLE_NUMBER);
            particle->value = util_strndup(&input[*n], i - *n);
            *n = i - 1;
            return particle;
        }
        if (input[i] >= '0' && input[i] <= '9') {
            // okay
        } else {
            return NULL;
        }
    }
}

/* subfunction of lex() */
static struct particle * consume_keyword(const char * input, size_t * n)
{
    for (size_t i = *n + 1; ; i++) {
        if (!input[i]) {
            return NULL;
        }
        if (input[i] == ' ' || input[i] == '\n' || input[i] == ')') {
            struct particle * particle = particle_create(PARTICLE_KEYWORD);
            particle->value = util_strndup(&input[*n], i - *n);
            for (size_t j = 0; j < i - *n; j++) {
                if (particle->value[j] >= 'a' && particle->value[j] <= 'z') {
                    particle->value[j] = particle->value[j] - 'a' + 'A';
                }
            }
            *n = i - 1;
            return particle;
        }
        if (input[i] >= 'a' && input[i] <= 'z') {
            // okay
        } else if (input[i] >= 'A' && input[i] <= 'Z') {
            // okay
        } else if (input[i] >= '0' && input[i] <= '9') {
            // okay
        } else if (input[i] == '!' || input[i] == '?' || input[i] == '-') {
            // okay
        } else {
            return NULL;
        }
    }
}

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
    )
{
    struct particle * particle;
    size_t n;

    for (n = 0; input[n]; n++) {
        particle = NULL;

        switch (input[n]) {
            case ' ':
                break;

            case '\n':
                particle = particle_create(PARTICLE_END);
                break;

            case '(':
                particle = particle_create(PARTICLE_BEGIN_NEST);
                break;

            case ')':
                particle = consume_end_nest(input, &n);
                if (!particle) {
                    *result_out = (struct lex_result) {
                        .type = LEX_ERROR,
                        .index = n
                    };
                    return;
                }
                break;

            case '"':
                particle = consume_name(input, &n);
                if (!particle) {
                    *result_out = (struct lex_result) {
                        .type = LEX_ERROR,
                        .index = n
                    };
                    return;
                }
                break;

            case '0' ... '9':
                particle = consume_number(input, &n);
                if (!particle) {
                    *result_out = (struct lex_result) {
                        .type = LEX_ERROR,
                        .index = n
                    };
                    return;
                }
                break;

            case 'a' ... 'z':
            case 'A' ... 'Z':
            case '+':
            case '-':
            case '*':
            case '/':
            case '?':
            case '!':
                particle = consume_keyword(input, &n);
                if (!particle) {
                    *result_out = (struct lex_result) {
                        .type = LEX_ERROR,
                        .index = n
                    };
                    return;
                }
                break;

            default:
                *result_out = (struct lex_result) {
                    .type = LEX_ERROR,
                    .index = n
                };
                return;
        }

        if (particle) {
            particle_buffer_add(buffer, particle);
        }
    }

    *result_out = (struct lex_result) {
        .type = LEX_OKAY,
        .index = n
    };
}
