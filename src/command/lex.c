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

/* for perror() */
#include <stdio.h>

/*
struct lexer {
};

struct lexer * lexer_create()
{
    struct lexer * lexer = malloc(sizeof(*lexer));
    *lexer = (struct lexer) { };
    return lexer;
}

void lexer_destroy(struct lexer * lexer)
{
    free(lexer);
}
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

struct particle * particle_create(enum particle_type type)
{
    struct particle * particle = malloc(sizeof(*particle));
    *particle = (struct particle) {
        .type = type
    };
    return particle;
}

void particle_destroy(struct particle * particle)
{
    free(particle->value);
    free(particle);
}

/*
static struct particle * consume_begin_nest(
        const char * input,
        size_t * n_inout
    )
{
    struct particle * particle = particle_create(PARTICLE_BEGIN_NEST);
    *n_inout++;
    return particle;
}

static struct particle * consume_end_nest(
        const char * input,
        size_t * n_inout
    )
{
    struct particle * particle = particle_create(PARTICLE_END_NEST);
    *n_inout++;
    return particle;
}
*/

struct particle_buffer * particle_buffer_create()
{
    struct particle_buffer * buffer = malloc(sizeof(*buffer));
    *buffer = (struct particle_buffer) { };
    return buffer;
}

void particle_buffer_destroy(struct particle_buffer * buffer)
{
    particle_buffer_free_all(buffer);
    free(buffer->particles);
    free(buffer);
}

void particle_buffer_free_all(struct particle_buffer * buffer)
{
    for (size_t n = 0; n < buffer->n_particles; n++) {
        particle_destroy(buffer->particles[n]);
    }
    buffer->n_particles = 0;
}

void particle_buffer_grow(struct particle_buffer * buffer, size_t amount)
{
    buffer->particles = realloc(
            buffer->particles,
            sizeof(*buffer->particles) * (buffer->capacity + amount)
        );
    buffer->capacity += amount;
}

void particle_buffer_at_least(struct particle_buffer * buffer, size_t minimum)
{
    if (minimum > buffer->capacity) {
        particle_buffer_grow(buffer, minimum - buffer->capacity);
    }
}

static struct particle * consume_name(const char * input, size_t * n)
{
    for (size_t i = *n + 1; ; i++) {
        if (!input[i]) {
            return NULL;
        }
        if (input[i] == '"') {
            struct particle * particle = particle_create(PARTICLE_NAME);
            particle->value = strndup(&input[*n + 1], i - *n - 1);
            if (!particle->value) {
                perror("[lexer] strndup error");
                particle_destroy(particle);
                return NULL;
            }
            *n = i;
            return particle;
        }
    }
}

static struct particle * consume_number(const char * input, size_t * n)
{
    for (size_t i = *n + 1; ; i++) {
        if (!input[i]) {
            return NULL;
        }
        if (input[i] == ' ' || input[i] == '\n' || input[i] == ')') {
            struct particle * particle = particle_create(PARTICLE_NUMBER);
            particle->value = strndup(&input[*n], i - *n);
            if (!particle->value) {
                perror("[lexer] strndup error");
                particle_destroy(particle);
                return NULL;
            }
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

static struct particle * consume_keyword(const char * input, size_t * n)
{
    for (size_t i = *n + 1; ; i++) {
        if (!input[i]) {
            return NULL;
        }
        if (input[i] == ' ' || input[i] == '\n' || input[i] == ')') {
            struct particle * particle = particle_create(PARTICLE_KEYWORD);
            particle->value = strndup(&input[*n], i - *n);
            if (!particle->value) {
                perror("[lexer] strndup error");
                particle_destroy(particle);
                return NULL;
            }
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
#ifndef PARTICLE_BUFFER_GROW_INCREMENT
#define PARTICLE_BUFFER_GROW_INCREMENT 64
#endif /* PARTICLE_BUFFER_GROW_INCREMENT */

static_assert(PARTICLE_BUFFER_GROW_INCREMENT > 0);

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
            case '\n':
                particle = particle_create(PARTICLE_END);
                break;
            case '(':
                particle = particle_create(PARTICLE_BEGIN_NEST);
                break;
            case ')':
                particle = particle_create(PARTICLE_END_NEST);
                break;
            case ' ':
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
            if (buffer->n_particles == buffer->capacity) {
                particle_buffer_grow(buffer, PARTICLE_BUFFER_GROW_INCREMENT);
            }
            buffer->particles[buffer->n_particles] = particle;
            buffer->n_particles++;
        }
    }

    *result_out = (struct lex_result) {
        .type = LEX_OKAY,
        .index = n
    };
}