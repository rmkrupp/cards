/* File: src/test/lex_test.c
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
#include "config.h"
#include "util/refstring.h"

#include <unistdio.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

/* TODO: fix casts */

static constexpr size_t line_max = 1024 * 1024 * 1024;

static void lex_test()
{
    struct particle_buffer * buffer = particle_buffer_create();
    struct lex_result result;

    struct game * game = game_create(&(struct config) {});
    struct parser * parser = parser_create(game);

    char * input = malloc(line_max);

    while (!feof(stdin)) {
        fgets(input, line_max, stdin);
        if (feof(stdin)) break;

        lex((uint8_t *)input, parser, buffer, &result);

        if (result.type == LEX_ERROR) {
            if (!isatty(fileno(stdin))) {
                size_t i;
                for (i = 0; input[i]; i++);
                if (i == 0 || input[i-1] != '\n') {
                    ulc_fprintf(stdout, "%U\n", input);
                } else {
                    ulc_fprintf(stdout, "%U", input);
                }
            }
            for (size_t i = 0; i < result.index; i++) {
                printf(" ");
            }
            printf("^error\n");
        } else {

            for (size_t n = 0; n < buffer->n_particles; n++) {
                struct refstring * string = particle_string(buffer->particles[n]);
                ulc_fprintf(stdout, "%s%U", (n == 0) ? "" : " ", refstring_string(string));
                refstring_destroy(string);
            }
            printf("\n");

        }

        particle_buffer_free_all(buffer);
    }

    free(input);

    particle_buffer_destroy(buffer);

    parser_destroy(parser);
    game_destroy(game);
}

static void silent_lex_test(size_t * total_out, size_t * errors_out)
{
    struct particle_buffer * buffer = particle_buffer_create();
    struct lex_result result;

    struct game * game = game_create(&(struct config) {});
    struct parser * parser = parser_create(game);

    char * input = malloc(line_max);

    size_t total = 0;
    size_t errors = 0;

    while (!feof(stdin)) {
        fgets(input, line_max, stdin);
        if (feof(stdin)) break;

        lex((uint8_t *)input, parser, buffer, &result);

        total++;
        if (result.type == LEX_ERROR) {
            errors++;
        }

        particle_buffer_free_all(buffer);
    }

    free(input);

    particle_buffer_destroy(buffer);
    parser_destroy(parser);
    game_destroy(game);

    *errors_out = errors;
    *total_out = total;
}

static void errors_only_lex_test(size_t * total_out, size_t * errors_out)
{
    struct particle_buffer * buffer = particle_buffer_create();
    struct lex_result result;

    struct game * game = game_create(&(struct config) {});
    struct parser * parser = parser_create(game);

    char * input = malloc(line_max);

    size_t total = 0;
    size_t errors = 0;

    while (!feof(stdin)) {
        fgets(input, line_max, stdin);
        if (feof(stdin)) break;

        lex((uint8_t *)input, parser, buffer, &result);

        total++;
        if (result.type == LEX_ERROR) {
            errors++;
            ulc_fprintf(stdout, "%U", input);
        }

        particle_buffer_free_all(buffer);
    }

    free(input);

    particle_buffer_destroy(buffer);
    parser_destroy(parser);
    game_destroy(game);

    *errors_out = errors;
    *total_out = total;
}

enum mode {
    NORMAL,
    SILENT,
    ERRORS
};

int main(int argc, char ** argv)
{
    enum mode mode = NORMAL;

    for (int arg = 1; arg < argc; arg++) {
        if (strcmp(argv[arg], "--silent") == 0) {
            mode = SILENT;
        } else if (strcmp(argv[arg], "--errors") == 0) {
            mode = ERRORS;
        } else {
            fprintf(stderr, "unknown argument \"%s\"\n", argv[1]);
            return 1;
        }
    }

    size_t errors, total;

    switch (mode) {
        case NORMAL:
            lex_test();
            break;

        case SILENT:
            silent_lex_test(&total, &errors);
            printf("%zu/%zu\n", errors, total);
            break;

        case ERRORS:
            errors_only_lex_test(&total, &errors);
            printf("%zu/%zu\n", errors, total);
            break;

        default:
            return 1;
    }
}

