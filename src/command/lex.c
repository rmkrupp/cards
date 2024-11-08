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
#include "command/parse.h"
#include "game.h"
#include "loader.h"

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "util/strdup.h"
#include "util/refstring.h"

#ifndef VERBOSE_LEXER
#define VERBOSE_LEXER 0
#endif /* VERBOSE_LEXER */

#if VERBOSE_LEXER

#include <stdio.h>
#include <ctype.h>

/* buffer of at least 5 bytes */
static const char * charmsg(char c)
{
    static char buffer[5];

    if (isprint(c) && !isspace(c)) {
        buffer[0] = '\'';
        buffer[1] = c;
        buffer[2] = '\'';
        buffer[3] = '\0';
        return buffer;
    } else {
        snprintf(buffer, 5, "%hhx", c);
        return buffer;
    }
}

#endif /* VERBOSE_LEXER */

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
 *
 * 4) TODO: UTF-8 Support
 *
 *    Right now, the lexer handles UTF-8 in name values, in that it correctly
 *    finds the "" around all UTF-8 strings wthout mangling them, as long as
 *    we do not support strings with embedded null bytes, though it does not
 *    correctly locale-aware case folding on them, and they are not normalized
 *    before comparison with the name_set (also, loaded bundles aren't
 *    normalized either)
 *
 *    If we want to support keywords, or more kinds of tokenization, or numbers
 *    with digits beyond ASCII 0-9, that will take a bit of work.
 *
 *    Also, we'd be introducing uint8_t *s into the mix and may need to update
 *    the API for that.
 *
 *    At a minimum, for proper UTF-8 support, normalization and case-folding
 *    of names being locale-aware is probably necessary. As long as keywords
 *    aren't more complex, the lexer can basically stay as is. Note that both
 *    normalization and case-folding involve memory allocations because they
 *    can change the length of the string. We could use a buffer, though.
 *
 * 5) TODO: re-entrant lexer
 *
 *    Instead of always receiving input a complete newline-termianted string
 *    at a time, we could switch to a reentrant lexing strategy that takes
 *    in any amount of data and handles incomplete lexing. This might make
 *    sense to do as part of the Unicode handling, though I don't know how easy
 *    it is to handle "interrupted" multibyte codepoints?
 */

/* how much to grow the particle_buffer by, in number of particles,
 * every time its capacity is exceeded.
 */
#ifndef PARTICLE_BUFFER_GROW_INCREMENT
#define PARTICLE_BUFFER_GROW_INCREMENT 64
#endif /* PARTICLE_BUFFER_GROW_INCREMENT */

static_assert(PARTICLE_BUFFER_GROW_INCREMENT > 0);

/* create a particle with the given type and NULL value */
[[nodiscard]] struct particle * particle_create(enum particle_type type)
{
    struct particle * particle = malloc(sizeof(*particle));
    *particle = (struct particle) {
        .type = type
    };
    return particle;
}

/* create a particle with the given type and and value
 *
 * makes a duplicate of the value, consuming at most n bytes from the argument.
 *
 * TODO: this does duplicate work between strndup and strnlen finding
 *       the length.
 *
 * TODO: is this function needed? it is not used by the lexer.
 */
[[nodiscard]] struct particle * particle_create_value(
        enum particle_type type, const char * value, size_t n)
{
    struct particle * particle = malloc(sizeof(*particle));
    *particle = (struct particle) {
        .type = type,
        .value = util_strndup(value, n),
        .length = strnlen(value, n)
    };
    return particle;
}

/* destroy this particle */
void particle_destroy(struct particle * particle) [[gnu::nonnull(1)]]
{
    switch (particle->type) {
        case PARTICLE_KEYWORD:
            if (particle->keyword == KEYWORD_NO_MATCH) {
                free(particle->value);
            }
            break;

        case PARTICLE_NAME:
            if (!particle->name) {
                free(particle->value);
            }
            break;

        case PARTICLE_NUMBER:
            free(particle->value);
            break;

        case PARTICLE_BEGIN_NEST:
        case PARTICLE_END_NEST:
        case PARTICLE_END:
            break;
    }
    free(particle);
}

/* return a refstring describing the particle
 *
 * the caller is the owner of the refstring, and so must call
 * refstring_destroy() on it when finished
 */
[[nodiscard]] struct refstring * particle_string(struct particle * particle)
{
    if (!particle) {
        return refstring_createf("NULL");
    }

    switch (particle->type) {
        case PARTICLE_END:
            return refstring_createf("END");

        case PARTICLE_KEYWORD:
            return refstring_createf(
                    "KEYWORD<%s>%s",
                    particle->value,
                    particle->keyword == KEYWORD_NO_MATCH ? "*" : ""
                );

        case PARTICLE_NUMBER:
            return refstring_createf("NUMBER<%s>", particle->value);

        case PARTICLE_NAME:
            return refstring_createf(
                    "NAME<%s>%s",
                    particle->value,
                    particle->name ? "" : "*"
                );

        case PARTICLE_BEGIN_NEST:
            return refstring_createf("(");

        case PARTICLE_END_NEST:
            return refstring_createf(")");

        default:
            return refstring_create("UNKNOWN");
    }
}

/* create a new buffer */
[[nodiscard]] struct particle_buffer * particle_buffer_create()
{
    struct particle_buffer * buffer = malloc(sizeof(*buffer));
    *buffer = (struct particle_buffer) { };
    return buffer;
}

/* destroy this buffer */
void particle_buffer_destroy(
        struct particle_buffer * buffer) [[gnu::nonnull(1)]]
{
    particle_buffer_free_all(buffer);
    free(buffer->particles);
    free(buffer);
}

/* free every particle in this buffer and set buffer->n_particles to zero */
void particle_buffer_free_all(
        struct particle_buffer * buffer) [[gnu::nonnull(1)]]
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
void particle_buffer_grow(
        struct particle_buffer * buffer, size_t amount) [[gnu::nonnull(1)]]
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
void particle_buffer_at_least(
        struct particle_buffer * buffer, size_t minimum) [[gnu::nonnull(1)]]
{
    if (minimum > buffer->capacity) {
        particle_buffer_grow(buffer, minimum - buffer->capacity);
    }
}

/* */
void particle_buffer_add(
        struct particle_buffer * buffer,
        struct particle * particle
    ) [[gnu::nonnull(1)]]
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
#if VERBOSE_LEXER
        fprintf(
                stderr,
                "lexer error 8 (bad char %s following end nest)\n",
                charmsg(input[*n + 1])
            );
#endif /* VERBOSE_LEXER */
        return NULL;
    }
}

/* subfunction of lex() */
static struct particle * consume_name(
        const char * input, size_t * n, struct name_set * name_set)
{
    for (size_t i = *n + 1; ; i++) {
        if (!input[i]) {
#if VERBOSE_LEXER
            fprintf(stderr, "lexer error 7 (null char in name)\n");
#endif /* VERBOSE_LEXER */
            return NULL;
        }
        if (input[i] == '\n' || input[i] == '\r' ||
                input[i] == 0xb || input[i] == 0xc) {
#if VERBOSE_LEXER
            fprintf(
                    stderr,
                    "lexer error 6 (char %s in name)\n",
                    charmsg(input[i])
                );
#endif /* VERBOSE_LEXER */
            return NULL;
        }
        if (input[i] == '"') {
            struct particle * particle = particle_create(PARTICLE_NAME);

            /* TODO (UTF-8 support) normalize the input before looking it up */
            particle->name =
                name_set_lookup(name_set, &input[*n + 1], i - *n - 1);

            if (particle->name) {
                particle->value = (char *)particle->name->name;
                particle->length = particle->name->length;
            } else {
                /* TODO (UTF-8 support) use a UTF-8 aware case change function
                 */
                particle->value = malloc(i - *n);
                for (size_t j = 0; j < i - *n - 1; j++) {
                    particle->value[j] = input[*n + 1 + j];
                }
                particle->value[i - *n - 1] = '\0';
                particle->length = i - *n - 1;
            }

            *n = i;

            return particle;
        }
    }
}

/* subfunction of lex() */
static struct particle * consume_number(const char * input, size_t * n)
{
    for (size_t i = *n + 1; ; i++) {
        if (!input[i] || input[i] == ' ' ||
                input[i] == '\n' || input[i] == ')') {
            struct particle * particle = particle_create(PARTICLE_NUMBER);
            particle->value = malloc(i - *n + 1);
            for (size_t j = 0; j < i - *n; j++) {
                particle->value[j] = input[*n + j];
            }
            particle->value[i - *n] = '\0';
            particle->length = i - *n;
            *n = i - 1;
            return particle;
        }
        /* TODO (UTF-8 support) if supporting, use uc_decimal_value */
        if (input[i] >= '0' && input[i] <= '9') {
            // okay
        } else {
#if VERBOSE_LEXER
            fprintf(
                    stderr,
                    "lexer error 4 (bad char %s in number)\n",
                    charmsg(input[i])
                );
#endif /* VERBOSE_LEXER */
            return NULL;
        }
    }
}

/* subfunction of lex() */
static struct particle * consume_keyword(char * input, size_t * n)
{
    size_t start = *n;
    for (size_t i = start + 1; ; i++) {
        if (!input[i] || input[i] == ' ' ||
                input[i] == '\n' || input[i] == ')') {
            struct particle * particle = particle_create(PARTICLE_KEYWORD);
            size_t length = i - start;

            /* TODO (UTF-8 support) if we support UTF-8 keywords, normalize
             *      before lookup
             */
            const struct keyword_lookup_result * lookup_result =
                keyword_lookup(&input[start], i - start);

            if (lookup_result) {
                particle->keyword = lookup_result->keyword;
                /* TODO: this is one of those suspect casting-away of consts
                 *       that are multiplying rapidly */
                particle->value =
                    (char *)keyword_string(lookup_result->offset);
                particle->length = length;
            } else {
                /* TODO (UTF-8 support) if we support UTF-8 keywords, use a
                 *      proper case change function
                 */
                particle->keyword = KEYWORD_NO_MATCH;
                particle->value = malloc(length + 1);
                particle->length = length;
                for (size_t j = 0; j < length; j++) {
                    particle->value[j] = input[start + j];
                }
                particle->value[length] = '\0';
            }
            *n = i - 1;
            return particle;
        }
        /* TODO (UTF-8 support) if we support UTF-8 keywords, handle a wider
         *      set of possible characters (or use a different tokenization
         *      scheme altogether)
         */
        if (input[i] >= 'a' && input[i] <= 'z') {
            input[i] = input[i] - 'a' + 'A';
            // okay
        } else if (input[i] >= 'A' && input[i] <= 'Z') {
            // okay
        } else if (input[i] >= '0' && input[i] <= '9') {
            // okay
        } else if (input[i] == '!' || input[i] == '?' || input[i] == '-') {
            // okay
        } else {
#if VERBOSE_LEXER
            fprintf(
                    stderr,
                    "lexer error 2 (bad char %s in keyword)\n",
                    charmsg(input[i])
                );
#endif /* VERBOSE_LEXER */
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
        char * input,
        struct parser * parser,
        struct particle_buffer * buffer,
        struct lex_result * result_out
    ) [[gnu::nonnull(1, 2, 3)]]
{
    struct particle * particle;
    size_t n;

    for (n = 0; input[n]; n++) {
        particle = NULL;

        switch (input[n]) {
            case ' ':
            case '\t':
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
                particle = consume_name(input, &n, parser->game->name_set);
                if (!particle) {
                    *result_out = (struct lex_result) {
                        .type = LEX_ERROR,
                        .index = n
                    };
                    return;
                }
                break;

            /* TODO (UTF-8 support) potentially use e.g. uc_decimal_value */
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

            /* TODO (UTF-8 support) if UTF-8 keywords, support a wider range of
             *      leading characters
             */
            case 'a' ... 'z':
                /* TODO (UTF-8 support) if UTF-8 keywords, use smarter case
                 *      change
                 */
                input[n] = input[n] - 'a' + 'A';
                [[fallthrough]];
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
#if VERBOSE_LEXER
                fprintf(
                        stderr,
                        "lexer error 1 (bad char %s in toplevel)\n",
                        charmsg(input[n])
                    );
#endif /* VERBOSE_LEXER */
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

    particle_buffer_add(buffer, particle_create(PARTICLE_END));

    *result_out = (struct lex_result) {
        .type = LEX_OKAY,
        .index = n
    };
}
