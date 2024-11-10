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
#include "name_set.h"

#include <stdlib.h>
#include <assert.h>

#include <unistr.h>
#include <uniname.h>
#include <uninorm.h>
#include <unicase.h>
#include <unistdio.h>

#include "util/strdup.h"
#include "util/refstring.h"
#include "util/log.h"

#ifndef VERBOSE_LEXER
#define VERBOSE_LEXER 0
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
 * TODO: is this function needed? it is not used by the lexer.
 */
[[nodiscard]] struct particle * particle_create_value(
        enum particle_type type, const uint8_t * value, size_t n)
{
    struct particle * particle = malloc(sizeof(*particle));

    *particle = (struct particle) {
        .type = type,
        .value = malloc(n + 1),
        .length = n
    };

    for (size_t i = 0; i < n; i++) {
        particle->value[i] = value[i];
    }
    particle->value[n] = 0;

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

        case PARTICLE_ERROR:
            free(particle->error);
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
                    "KEYWORD<%.*U>%s",
                    particle->length,
                    particle->value,
                    particle->keyword == KEYWORD_NO_MATCH ? "*" : ""
                );

        case PARTICLE_NUMBER:
            return refstring_createf(
                    "NUMBER<%.*U>",
                    particle->length,
                    particle->value
                );

        case PARTICLE_NAME:
            return refstring_createf(
                    "NAME<%.*U>%s",
                    particle->length,
                    particle->value,
                    particle->name ? "" : "*"
                );

        case PARTICLE_BEGIN_NEST:
            return refstring_createf("(");

        case PARTICLE_END_NEST:
            return refstring_createf(")");

        case PARTICLE_ERROR:
            if (particle->error) {
                return refstring_createf(
                        "ERROR<%.*U>",
                        particle->error_length,
                        particle->error
                    );
            } else {
                return refstring_createf("ERROR");
            }

        default:
            return refstring_createf("UNKNOWN");
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

/* add this particle to this buffer */
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

/* an offset into some lexer_inputs */
struct lex_ptr {
    size_t n_input;
    size_t index;
};

/* the total bytes offset by this ptr */
static size_t lex_ptr_sum(
        const struct lexer_input * inputs, const struct lex_ptr * ptr)
{
    size_t sum = ptr->index;
    for (size_t i = 0; i < ptr->n_input; i++) {
        sum += inputs[i].length;
    }
    return sum;
}

/* returns true if this ptr is at the end of these inputs */
static bool lex_ptr_at_end(size_t n_inputs, const struct lex_ptr * ptr)
{
    return ptr->n_input == n_inputs;
}

/* returns the next uint8_t in the stream */
static uint8_t lex_ptr_peek(
        const struct lexer_input * inputs, const struct lex_ptr * ptr)
{
    return inputs[ptr->n_input].input[ptr->index];
}

/* advance the pointer by one byte
 *
 * returns true if lex_ptr_at_end() would return true on the new ptr
 */
static bool lex_ptr_advance(
        const struct lexer_input * inputs,
        size_t n_inputs,
        struct lex_ptr * ptr
    )
{
    ptr->index++;
    if (ptr->index == inputs[ptr->n_input].length) {
        ptr->index = 0;
        ptr->n_input++;
    }
    return ptr->n_input == n_inputs;
}

/* advance the pointer by n bytes
 *
 * note: this does not check against the length of inputs, use only if there
 *       are at least n bytes available
 *
 * TODO: this is not used, but would be required if we switch the parser to
 *       going by ucs4_t (if that happens, drop the maybe_unused)
 */
[[maybe_unused]] static bool lex_ptr_advance_n(
        const struct lexer_input * inputs,
        size_t n_inputs,
        struct lex_ptr * ptr,
        size_t n
    )
{
    ptr->index += n;
    while (ptr->index >= inputs[ptr->n_input].length) {
        ptr->index -= inputs[ptr->n_input].length;
        ptr->n_input++;
    }
    return ptr->n_input == n_inputs;
}

/* return a buffer that holds all the data between start and stop, inclusive
 *
 * if start and stop point to the same lexer_input, just return a pointer to
 * (an offset into) that. store the size into size_out and false into
 * need_free_out
 *
 * otherwise, allocate a buffer and copy the data over into it. store the size
 * and true into need_free_out
 *
 * returns either one of those
 *
 * this casts away the const of the lexer_input if it returns a buffer from
 * that, you just have to triple promise to follow needs_free_out and to in
 * either case not modify it beyond free'ing it, and then it's all copacetic
 */
[[nodiscard]] static uint8_t * lex_ptr_buffer(
        const struct lexer_input * inputs,
        const struct lex_ptr * start,
        const struct lex_ptr * stop,
        size_t * size_out,
        bool * needs_free_out
    )
{
    if (start->n_input == stop->n_input) {
        *size_out = stop->index - start->index;
        *needs_free_out = false;
        /* cast away the const, see note */
        return (uint8_t *)&inputs[start->n_input].input[start->index];
    }

    size_t size = inputs[start->n_input].length - start->index;
    for (size_t n_input = start->n_input; n_input < stop->n_input; n_input++) {
        size += inputs[n_input].length;
    }
    size += stop->index;

    uint8_t * buffer = malloc(size);
    size_t index = 0;
    for (size_t i = start->index; i < inputs[start->n_input].length; i++) {
        buffer[index] = inputs[start->n_input].input[i];
        index++;
    }
    for (size_t n_input = start->n_input; n_input < stop->n_input; n_input++) {
        for (size_t i = 0; i < inputs[n_input].length; i++) {
            buffer[index] = inputs[n_input].input[i];
            index++;
        }
    }
    for (size_t i = 0; i < stop->index; i++) {
        buffer[index] = inputs[stop->n_input].input[i];
    }

    *size_out = size;
    *needs_free_out = true;

    return buffer;
}

/* like lex_ptr_buffer except it always returns a copy of the underlying
 * memory, never a reference to the inputs
 *
 * thus, it has no needs_free_out (because it always needs to be free'd)
 */
[[nodiscard]] static uint8_t * lex_ptr_buffer_always_copy(
        const struct lexer_input * inputs,
        const struct lex_ptr * start,
        const struct lex_ptr * stop,
        size_t * size_out
    )
{
    size_t size;
    bool needs_free_out;

    uint8_t * buffer = lex_ptr_buffer(
            inputs, start, stop, &size, &needs_free_out);

    if (!needs_free_out) {
        uint8_t * new_buffer = malloc(size);
        for (size_t i = 0; i < size; i++) {
            new_buffer[i] = buffer[i];
        }
        buffer = new_buffer;
    }

    *size_out = size;
    return buffer;
}

/* peek into inputs and place the leading ucs4_t into c_out
 *
 * returns the number of bytes peeked, or -1 if the input is invalid, or -2 if
 * the input is incomplete
 */
static int lex_ptr_peek_ucs4(
        const struct lexer_input * inputs,
        size_t n_inputs,
        const struct lex_ptr * ptr,
        ucs4_t * c_out
    )
{
    const uint8_t * buffer = &inputs[ptr->n_input].input[ptr->index];
    size_t size = inputs[ptr->n_input].length - ptr->index;

    int result = u8_mbtoucr(c_out, buffer, size);

    /* try again if we needed more data and we HAVE more data */
    if (result == -2) {
        assert(size < 4);
        uint8_t new_buffer[4];
        for (size_t i = 0; i < size; i++) {
            new_buffer[i] = buffer[i];
        }
        size_t n_input = ptr->n_input + 1;
        size_t index = 0;
        while (size < 4) {
            if (n_input == n_inputs) {
                /* the inputs end on a boundary */
                return -2;
            }
            while (size < 4 && index < inputs[n_input].length) {
                new_buffer[size] = inputs[n_input].input[index];
                size++;
                index++;
            }
            n_input++;
        }

        return u8_mbtoucr(c_out, new_buffer, 4);
    }

    return result;
}

#if VERBOSE_LEXER

/* for displaying errors
 *
 * takes the first ucs4_t starting at ptr and gets its name, or else
 * <incomplete> if the input ends before a complete ucs4_t can be read
 *
 * returns a pointer to static memory (good until charmsg is called again)
 * with that name, or with the special strings <invalid> if the character was
 * invalid (i.e. u8_mbtoucr() returned -1), or <incomplete> if, even with the
 * complete input, u8_mbtoucr() returns -2, or the character in hex if
 * unicode_character_name() returned null.
 */
static const uint8_t * charmsg(
        const struct lexer_input * inputs,
        size_t n_inputs,
        const struct lex_ptr * ptr
    )
{
    static uint8_t buffer_out[UNINAME_MAX];
    static char buffer_name[UNINAME_MAX];

    static_assert(UNINAME_MAX >= sizeof("<incomplete>"));

    ucs4_t c;
    int result = lex_ptr_peek_ucs4(inputs, n_inputs, ptr, &c);

    if (result == -1) {
        u8_snprintf(buffer_out, UNINAME_MAX, "<invalid>");
        return buffer_out;
    }

    if (result == -2) {
        u8_snprintf(buffer_out, UNINAME_MAX, "<incomplete>");
        return buffer_out;
    }

    if (!unicode_character_name(c, buffer_name)) {
        u8_snprintf(buffer_out, UNINAME_MAX, "0x%x", c);
        return buffer_out;
    }

    u8_snprintf(buffer_out, UNINAME_MAX, "%s", buffer_name);
    return buffer_out;
}

#endif /* VERBOSE_LEXER */

/* subfunction of lex() */
/* TODO: align the stop conditions between these consume functions, e.g. for
 *       \r
 */
static struct particle * consume_end_nest(
        const struct lexer_input * inputs,
        size_t n_inputs,
        struct lex_ptr * ptr
    )
{
    uint8_t c = lex_ptr_peek(inputs, ptr);
    if (c == '\0' || c == ' ' || c == '\n' || c == ')') {
        lex_ptr_advance(inputs, n_inputs, ptr);
        return particle_create(PARTICLE_END_NEST);
    } else {
        struct particle * particle = particle_create(PARTICLE_ERROR);
        /* TODO: error value / position */
#if VERBOSE_LEXER
        particle->error_length = u8_asprintf(
                &particle->error,
                "lexer error 8 (bad char %U following end nest)",
                charmsg(inputs, n_inputs, ptr)
            );
#endif /* VERBOSE_LEXER */
        lex_ptr_advance(inputs, n_inputs, ptr);
        return particle;
    }
}

/* subfunction of lex() */
/* TODO: align the stop conditions between these consume functions, e.g. for
 *       \r
 */
static struct particle * consume_name(
        const struct lexer_input * inputs,
        size_t n_inputs,
        struct lex_ptr * ptr,
        struct name_set * name_set
    )
{
    struct lex_ptr ptr_start = *ptr;
    lex_ptr_advance(inputs, n_inputs, &ptr_start);
    struct lex_ptr ptr_copy = ptr_start;
    while (!lex_ptr_at_end(n_inputs, &ptr_copy)) {
        uint8_t c = lex_ptr_peek(inputs, &ptr_copy);
        if (c == '\0' || c == '\n' || c == '\r' || c == 0xb || c == 0xc) {
            /* seek to the end */
            struct lex_ptr ptr_copy_2 = ptr_copy;
            while (!lex_ptr_advance(inputs, n_inputs, &ptr_copy_2)) {
                uint8_t c = lex_ptr_peek(inputs, &ptr_copy_2);
                if (c == '"') {
                    break;
                }
            }
            if (lex_ptr_at_end(n_inputs, &ptr_copy_2)) {
                /* input ends before particle ends / we recover */
                return NULL;
            }
            struct particle * particle = particle_create(PARTICLE_ERROR);
#if VERBOSE_LEXER
            /* TODO: error value / position */
            particle->error_length = u8_asprintf(
                &particle->error,
                "lexer error 6 (invalid character %U in name)",
                charmsg(inputs, n_inputs, &ptr_copy)
            );
#endif /* VERBOSE_LEXER */
            *ptr = ptr_copy_2;
            lex_ptr_advance(inputs, n_inputs, ptr);
            return particle;
        }
        if (c == '"') {
            break;
        }
        lex_ptr_advance(inputs, n_inputs, &ptr_copy);
    }

    if (lex_ptr_at_end(n_inputs, &ptr_copy)) {
        /* input ends before particle ends */
        return NULL;
    }

    struct particle * particle = particle_create(PARTICLE_NAME);

    size_t size;
    bool needs_free;
    uint8_t * buffer = lex_ptr_buffer(
            inputs, &ptr_start, &ptr_copy, &size, &needs_free);

    particle->name = name_set_lookup(
            name_set,
            buffer,
            size
        );

    if (particle->name) {
        particle->value = particle->name->display_name;
        particle->length = particle->name->display_name_length;
        if (needs_free) {
            free(buffer);
        }
    } else {
        /* no need to transform an unmatched name */
        if (needs_free) {
            particle->value = buffer;
            particle->length = size;
        } else {
            particle->value = malloc(size);
            for (size_t i = 0; i < size; i++) {
                particle->value[i] = buffer[i];
            }
            particle->length = size;
        }
    }

    lex_ptr_advance(inputs, n_inputs, &ptr_copy);
    *ptr = ptr_copy;

    return particle;
}

/* subfunction of lex() */
/* TODO: align the stop conditions between these consume functions, e.g. for
 *       \r
 */
static struct particle * consume_number(
        const struct lexer_input * inputs,
        size_t n_inputs,
        struct lex_ptr * ptr
    )
{
    struct lex_ptr ptr_copy = *ptr;

    /* seek to find the end */
    while (!lex_ptr_at_end(n_inputs, &ptr_copy)) {
        uint8_t c = lex_ptr_peek(inputs, &ptr_copy);
        /* we found the end, return a particle */
        if (c == '\0' || c == ' ' || c == '\n' || c == ')') {
            struct particle * particle = particle_create(PARTICLE_NUMBER);
            size_t size;
            uint8_t * buffer = lex_ptr_buffer_always_copy(
                    inputs, ptr, &ptr_copy, &size);
            particle->value = buffer;
            particle->length = size;
            *ptr = ptr_copy;
            return particle;
        }

        if (c >= '0' && c <= '9') {
            // okay
        } else {
            struct particle * particle = particle_create(PARTICLE_ERROR);
            /* TODO: there are actually a few things we could do in this case
             *       to try and "recover"
             *
             *       the current solution is to consume until we reach
             *       something that WOULD have stopped the number (and been
             *       valid)
             */
            struct lex_ptr ptr_copy_2 = ptr_copy;
            while (!lex_ptr_at_end(n_inputs, &ptr_copy_2)) {
                uint8_t c = lex_ptr_peek(inputs, &ptr_copy_2);
                if (c == '\0' || c == '\n' || c == ')') {
                    break;
                }
                lex_ptr_advance(inputs, n_inputs, &ptr_copy_2);
            }
            if (lex_ptr_at_end(n_inputs, &ptr_copy_2)) {
                /* input ends before particle ends / we recover */
                return NULL;
            }
#if VERBOSE_LEXER
            /* TODO: error value / position */
            particle->error_length = u8_asprintf(
                    &particle->error,
                    "lexer error 4 (bad char %U in number)",
                    charmsg(inputs, n_inputs, &ptr_copy)
                );
#endif /* VERBOSE_LEXER */
            *ptr = ptr_copy_2;
            return particle;
        }

        lex_ptr_advance(inputs, n_inputs, &ptr_copy);
    }

    /* input ends before particle ends */
    return NULL;
}

/* subfunction of lex() */
/* TODO: align the stop conditions between these consume functions, e.g. for
 *       \r
 */
static struct particle * consume_keyword(
        const struct lexer_input * inputs,
        size_t n_inputs,
        struct lex_ptr * ptr
    )
{
    struct lex_ptr ptr_copy = *ptr;
    while (!lex_ptr_at_end(n_inputs, &ptr_copy)) {
        uint8_t c = lex_ptr_peek(inputs, &ptr_copy);

        if (c == '\0' || c == ' ' || c == '\n' || c == ')') {
            struct particle * particle = particle_create(PARTICLE_KEYWORD);

            size_t size;
            bool needs_free;
            uint8_t * buffer = lex_ptr_buffer(
                    inputs, ptr, &ptr_copy, &size, &needs_free);

            /* these two casts will go away if we switch to internal hash
             * library for keywords
             */
            const struct keyword_lookup_result * lookup_result =
                keyword_lookup(/* here */(char *)buffer, size);

            if (lookup_result) {
                particle->keyword = lookup_result->keyword;
                particle->value = /* and here */(uint8_t *)keyword_string(
                        lookup_result->offset);
                /* size is the same because we don't normalize or case-change
                 * (yet)
                 */
                particle->length = size;
                if (needs_free) {
                    free(buffer);
                }
            } else {
                if (needs_free) {
                    particle->value = buffer;
                    particle->length = size;
                } else {
                    particle->value = malloc(size);
                    for (size_t i = 0; i < size; i++) {
                        particle->value[i] = buffer[i];
                    }
                    particle->length = size;
                }
            }

            *ptr = ptr_copy;

            return particle;
        }

        /* TODO: figure out the case stuff */
        if (c >= 'a' && c <= 'z') {
            // okay
        } else if (c >= 'A' && c <= 'Z') {
            // okay
        } else if (c >= '0' && c <= '9') {
            // okay
        } else if (c == '!' || c == '?' || c == '-' || c == '*' ||
                c == '+' || c == '/') {
            // okay
        } else {
            struct particle * particle = particle_create(PARTICLE_ERROR);
#if VERBOSE_LEXER
            particle->error_length = u8_asprintf(
                    &particle->error,
                    "lexer error 2 (bad char %U in keyword)",
                    charmsg(inputs, n_inputs, &ptr_copy)
                );
#endif /* VERBOSE_LEXER */
            *ptr = ptr_copy;
            return particle;
        }

        lex_ptr_advance(inputs, n_inputs, &ptr_copy);
    }

    /* input ends before particle ends */
    return NULL;
}

/* turn input strings into particles
 *
 * lexer->buffer must be a particle buffer
 * if it is not empty, the particles will be appended,
 * thus it must be emptied after the particles have been consumed
 * (e.g. via a call to particle_buffer_free_all())
 *
 * otherwise, the caller must track its own offset into the buffer
 *
 * the result status is written to result_out
 *
 * returns the number of bytes consumed from the inputs
 */
size_t lex(
        const struct lexer_input * inputs,
        size_t n_inputs,
        struct name_set * name_set,
        struct particle_buffer * buffer
    ) [[gnu::nonnull(1, 3, 4)]]
{
    struct lex_ptr ptr = { };

    while (!lex_ptr_at_end(n_inputs, &ptr)) {
        struct particle * particle = NULL;

        /* TODO: we could check against a ucs4_t instead of uint8_t if:
         *        - we supported unicode keywords (check is_letter)
         *        - we supported numbers with scripts other than 0-9 (use
         *          uc_decimal_value() to extract the 0-9)
         *        - we supported separators other than space and tab
         *
         *       if we support utf-8 keywords, we need to use our internal
         *       name sets / hash tables instead of gperf, because gperf
         *       doesn't support unicode (allegedly; TODO: test this)
         *
         *       there's also stuff like being smart about mirror characters
         *       etc. that's probably not needed. (or using alternatives in
         *       some cases to "s for names)
         */

        switch (lex_ptr_peek(inputs, &ptr)) {
            case ' ':
            case '\r':
            case '\t':
                lex_ptr_advance(inputs, n_inputs, &ptr);
                break;

            case '\n':
                particle = particle_create(PARTICLE_END);
                lex_ptr_advance(inputs, n_inputs, &ptr);
                break;

            case '(':
                particle = particle_create(PARTICLE_BEGIN_NEST);
                lex_ptr_advance(inputs, n_inputs, &ptr);
                break;

            case ')':
                particle = consume_end_nest(inputs, n_inputs, &ptr);
                if (!particle) {
                    return lex_ptr_sum(inputs, &ptr);
                }
                break;

            case '"':
                particle = consume_name(
                        inputs, n_inputs, &ptr, name_set);
                if (!particle) {
                    return lex_ptr_sum(inputs, &ptr);
                }
                break;

            case '0' ... '9':
                particle = consume_number(inputs, n_inputs, &ptr);
                if (!particle) {
                    return lex_ptr_sum(inputs, &ptr);
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
                particle = consume_keyword(inputs, n_inputs, &ptr);
                if (!particle) {
                    return lex_ptr_sum(inputs, &ptr);
                }
                break;

            default:
                particle = particle_create(PARTICLE_ERROR);
                /* TODO: error value / position */
#if VERBOSE_LEXER
                particle->error_length = u8_asprintf(
                        &particle->error,
                        "lexer error 1 (bad char %U in toplevel)",
                        charmsg(inputs, n_inputs, &ptr)
                    );
#endif /* VERBOSE_LEXER */
                lex_ptr_advance(inputs, n_inputs, &ptr);
                break;
        }

        if (particle) {
            particle_buffer_add(buffer, particle);
        }
    }

    return lex_ptr_sum(inputs, &ptr);
}
