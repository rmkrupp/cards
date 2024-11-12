/* File: src/test/lex_test2.c
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>

#include <unistdio.h>

#include "command/lex.h"
#include "name_set.h"
#include "util/refstring.h"

/* sanity check for the lexer */

static const char * type_to_string(enum particle_type type)
{

    static const char * particle_strings[] = {
        [PARTICLE_END] = "PARTICLE_END",
        [PARTICLE_KEYWORD] = "PARTICLE_KEYWORD",
        [PARTICLE_NUMBER] = "PARTICLE_NUMBER",
        [PARTICLE_NAME] = "PARTICLE_NAME",
        [PARTICLE_BEGIN_NEST] = "PARTICLE_BEGIN_NEST",
        [PARTICLE_END_NEST] = "PARTICLE_END_NEST",
        [PARTICLE_ERROR] = "PARTICLE_ERROR"
    };

    static const char * null_string = "NULL";

    if (type >= sizeof(particle_strings) / sizeof(*particle_strings)) {
        return null_string;
    }

    if (particle_strings[type]) {
        return particle_strings[type];
    }

    return null_string;
}

static void ensure_particle_type(
        struct particle * particle, enum particle_type type, size_t * n_errors)
{
    if (particle->type != type) {
        fprintf(
                stderr,
                "particle type %s does not match expected type %s\n",
                type_to_string(particle->type),
                type_to_string(type)
            );
        *n_errors += 1;
    }
}


static void ensure_particle_value(
        struct particle * particle, const uint8_t * value, size_t * n_errors)
{
    size_t index = particle->length;

    size_t len = 0;
    for (; value[len]; len++);

    if (len != index) {
        ulc_fprintf(
                stderr,
                "particle value %.*U is not the same length (%zu) as expected value %U (%zu)\n",
                particle->length,
                particle->value,
                particle->length,
                value,
                len
            );

        *n_errors += 1;
        return;
    }

    for (size_t i = 0; i < index; i++) {
        if (value[i] != particle->value[i]) {
            ulc_fprintf(
                    stderr,
                    "particle value %.*U (%x) does not match expected value %U (%x)\n",
                    particle->length,
                    particle->value,
                    particle->value[i],
                    value,
                    value[i]
            );
            *n_errors += 1;
        }
    }
}


static void ensure_result_value(
        size_t result, size_t expected, size_t * n_errors)
{
    if (result != expected) {
        fprintf(
                stderr,
                "result of %zu does not equal expected result %zu\n",
                result,
                expected
            );

        *n_errors += 1;
    }
}

static void ensure_particle_keyword(
        struct particle * particle, enum keyword keyword, size_t * n_errors)
{
    if (particle->keyword != keyword) {
        fprintf(
                stderr,
                "keyword particle keyword %d did not match expected keyword %d\n",
                particle->keyword,
                keyword
            );
        *n_errors += 1;
    }
}

static void ensure_particle_name(
        struct particle * particle, bool has_name, size_t * n_errors)
{
    if (has_name && !particle->name) {
        fprintf(
                stderr,
                "name particle does not have name when one was expected\n"
            );
        *n_errors += 1;
    }
    if (!has_name && particle->name) {
        fprintf(
                stderr,
                "name particle has name when one was not expected\n"
            );
        *n_errors += 1;
    }
}

int main(int argc, char ** argv)
{
    /* TODO: set locale based on environment */
    setlocale(LC_ALL, "en_US.UTF-8");

    (void)argc;
    (void)argv;

    size_t errors = 0;

    printf("Begin sanity check...\n");

    /* simple two particle check */
    struct lexer_input word[] = {
        {
            .input = u8"word\n",
            .length = 5
        }
    };

    struct particle_buffer * buffer = particle_buffer_create();
    struct name_set * name_set = name_set_create();

    name_set_add(name_set, u8"scone", 5, NULL, NAME_TYPE_PLAYER);

    size_t result = lex(word, sizeof(word) / sizeof(*word), name_set, buffer);

    ensure_result_value(result, 5, &errors);

    ensure_particle_type(buffer->particles[0], PARTICLE_KEYWORD, &errors);
    ensure_particle_type(buffer->particles[1], PARTICLE_END, &errors);

    ensure_particle_value(buffer->particles[0], u8"word", &errors);
    ensure_particle_keyword(buffer->particles[0], KEYWORD_NO_MATCH, &errors);

    particle_buffer_free_all(buffer);

    /* two keyword inputs terminated with an end */
    struct lexer_input two_words[] = {
        {
            .input = u8"SAY\n",
            .length = 4
        },
        {
            .input = u8"berries\n",
            .length = 8
        }
    };

    result = lex(two_words, sizeof(two_words) / sizeof(*two_words), name_set, buffer);

    ensure_result_value(result, 12, &errors);

    ensure_particle_type(buffer->particles[0], PARTICLE_KEYWORD, &errors);
    ensure_particle_type(buffer->particles[1], PARTICLE_END, &errors);
    ensure_particle_type(buffer->particles[2], PARTICLE_KEYWORD, &errors);
    ensure_particle_type(buffer->particles[3], PARTICLE_END, &errors);

    ensure_particle_value(buffer->particles[0], u8"SAY", &errors);
    ensure_particle_value(buffer->particles[2], u8"berries", &errors);

    ensure_particle_keyword(buffer->particles[0], KEYWORD_SAY, &errors);

    particle_buffer_free_all(buffer);

    /* test over two inputs */
    struct lexer_input carryover[] = {
        {
            .input = u8"LOOK cute",
            .length = 9
        },
        {
            .input = u8"st thing\n",
            .length = 9
        }
    };

    result = lex(carryover, sizeof(carryover) / sizeof(*carryover), name_set, buffer);

    ensure_result_value(result, 18, &errors);

    ensure_particle_type(buffer->particles[0], PARTICLE_KEYWORD, &errors);
    ensure_particle_type(buffer->particles[1], PARTICLE_KEYWORD, &errors);
    ensure_particle_type(buffer->particles[2], PARTICLE_KEYWORD, &errors);
    ensure_particle_type(buffer->particles[3], PARTICLE_END, &errors);

    ensure_particle_value(buffer->particles[0], u8"LOOK", &errors);
    ensure_particle_value(buffer->particles[1], u8"cutest", &errors);
    ensure_particle_value(buffer->particles[2], u8"thing", &errors);

    ensure_particle_keyword(buffer->particles[0], KEYWORD_LOOK, &errors);

    particle_buffer_free_all(buffer);

    /* test if keywords can be spaced out over three inputs */
    struct lexer_input carryover_three[] = {
        {
            .input = u8"cut",
            .length = 3
        },
        {
            .input = u8"est",
            .length = 3
        },
        {
            .input = u8"erest\n",
            .length = 6
        }
    };

    result = lex(
            carryover_three,
            sizeof(carryover_three) / sizeof(*carryover_three),
            name_set,
            buffer
        );

    ensure_result_value(result, 12, &errors);

    ensure_particle_type(buffer->particles[0], PARTICLE_KEYWORD, &errors);
    ensure_particle_type(buffer->particles[1], PARTICLE_END, &errors);

    ensure_particle_value(buffer->particles[0], u8"cutesterest", &errors);

    particle_buffer_free_all(buffer);

    /* test if numbers can be spaced out over three inputs */
    struct lexer_input carryover_number[] = {
        {
            .input = u8"56",
            .length = 2
        },
        {
            .input = u8"72",
            .length = 2
        },
        {
            .input = u8"16\n",
            .length = 3
        }
    };

    result = lex(
            carryover_number,
            sizeof(carryover_number) / sizeof(*carryover_number),
            name_set,
            buffer
        );

    ensure_result_value(result, 7, &errors);

    ensure_particle_type(buffer->particles[0], PARTICLE_NUMBER, &errors);
    ensure_particle_type(buffer->particles[1], PARTICLE_END, &errors);

    ensure_particle_value(buffer->particles[0], u8"567216", &errors);

    particle_buffer_free_all(buffer);

     /* test if names can be spaced out over three inputs */
    struct lexer_input carryover_names[] = {
        {
            .input = u8"\"The big",
            .length = 8
        },
        {
            .input = u8" bucket",
            .length = 7
        },
        {
            .input = u8" here.\"\n",
            .length = 8
        }
    };

    result = lex(
            carryover_names,
            sizeof(carryover_names) / sizeof(*carryover_names),
            name_set,
            buffer
        );

    ensure_result_value(result, 23, &errors);

    ensure_particle_type(buffer->particles[0], PARTICLE_NAME, &errors);
    ensure_particle_type(buffer->particles[1], PARTICLE_END, &errors);

    ensure_particle_value(buffer->particles[0], u8"The big bucket here.", &errors);

    particle_buffer_free_all(buffer);

    /* test of parenthesis without spaces */
    struct lexer_input no_space_paren[] = {
        {
            .input = u8"(LOOK) )SAY DO(\n",
            .length = 16
        }
    };

    result = lex(
            no_space_paren,
            sizeof(no_space_paren) / sizeof(*no_space_paren),
            name_set,
            buffer
        );

    ensure_result_value(result, 16, &errors);

    ensure_particle_type(buffer->particles[0], PARTICLE_BEGIN_NEST, &errors);
    ensure_particle_type(buffer->particles[1], PARTICLE_KEYWORD, &errors);
    ensure_particle_type(buffer->particles[2], PARTICLE_END_NEST, &errors);
    ensure_particle_type(buffer->particles[3], PARTICLE_END_NEST, &errors);
    ensure_particle_type(buffer->particles[4], PARTICLE_KEYWORD, &errors);
    ensure_particle_type(buffer->particles[5], PARTICLE_ERROR, &errors);
    ensure_particle_type(buffer->particles[6], PARTICLE_BEGIN_NEST, &errors);
    ensure_particle_type(buffer->particles[7], PARTICLE_END, &errors);

    ensure_particle_value(buffer->particles[1], u8"LOOK", &errors);
    ensure_particle_value(buffer->particles[4], u8"SAY", &errors);

    ensure_particle_keyword(buffer->particles[1], KEYWORD_LOOK, &errors);
    ensure_particle_keyword(buffer->particles[4], KEYWORD_SAY, &errors);

    particle_buffer_free_all(buffer);

    /* test of number and name particles */
    struct lexer_input num_and_string[] = {
        {
            .input = u8"\"scone\" \"'another'\" 32",
            .length = 22
        },
        {
            .input = u8" 56\n",
            .length = 4
        }
    };

    result = lex(
            num_and_string,
            sizeof(num_and_string) / sizeof(*num_and_string),
            name_set,
            buffer
        );

    ensure_result_value(result, 26, &errors);

    ensure_particle_type(buffer->particles[0], PARTICLE_NAME, &errors);
    ensure_particle_type(buffer->particles[1], PARTICLE_NAME, &errors);
    ensure_particle_type(buffer->particles[2], PARTICLE_NUMBER, &errors);
    ensure_particle_type(buffer->particles[3], PARTICLE_NUMBER, &errors);
    ensure_particle_type(buffer->particles[4], PARTICLE_END, &errors);

    ensure_particle_value(buffer->particles[0], u8"scone", &errors);
    ensure_particle_value(buffer->particles[1], u8"'another'", &errors);
    ensure_particle_value(buffer->particles[2], u8"32", &errors);
    ensure_particle_value(buffer->particles[3], u8"56", &errors);

    ensure_particle_name(buffer->particles[0], true, &errors);
    ensure_particle_name(buffer->particles[1], false, &errors);

    particle_buffer_free_all(buffer);

    /* test of a unicode character */
    struct lexer_input unicode[] = {
        {
            .input = u8"\"\xf0\x9f\x98\x80\"\n",
            .length = 7
        }
    };

    result = lex(
            unicode,
            sizeof(unicode) / sizeof(*unicode),
            name_set,
            buffer
        );

    ensure_result_value(result, 7, &errors);

    ensure_particle_type(buffer->particles[0], PARTICLE_NAME, &errors);
    ensure_particle_type(buffer->particles[1], PARTICLE_END, &errors);

    ensure_particle_value(buffer->particles[0], u8"\xf0\x9f\x98\x80", &errors);

    particle_buffer_free_all(buffer);


    /* test for null bytes in names */
    struct lexer_input null_bytes_names[] = {
        {
            .input = u8"\"na\x00me\"\n",
            .length = 8
        }
    };

    result = lex(
            null_bytes_names,
            sizeof(null_bytes_names) / sizeof(*null_bytes_names),
            name_set,
            buffer
        );

    ensure_result_value(result, 8, &errors);

    ensure_particle_type(buffer->particles[0], PARTICLE_ERROR, &errors);
    ensure_particle_type(buffer->particles[1], PARTICLE_END, &errors);

    particle_buffer_free_all(buffer);

    /* test for unicode split over lines */
    struct lexer_input unicode_over_inputs[] = {
        {
            .input = u8"\"\xf0\x9f",
            .length = 3
        },
        {
            .input = u8"\x98\x80\"\n",
            .length = 4
        }
    };

    result = lex(
            unicode_over_inputs,
            sizeof(unicode_over_inputs) / sizeof(*unicode_over_inputs),
            name_set,
            buffer
        );

    ensure_result_value(result, 7, &errors);

    ensure_particle_type(buffer->particles[0], PARTICLE_NAME, &errors);
    ensure_particle_type(buffer->particles[1], PARTICLE_END, &errors);

    ensure_particle_value(buffer->particles[0], u8"\xf0\x9f\x98\x80", &errors);

    particle_buffer_free_all(buffer);

    printf("Sanity check done.\n");
    if (errors) {
        printf("%zu errors found\n", errors);
    } else {
        printf("No errors found\n");
    }
    particle_buffer_destroy(buffer);
    name_set_destroy(name_set);

}
