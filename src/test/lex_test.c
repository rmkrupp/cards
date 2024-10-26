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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define LINE_MAX 16 * 1024

int main(int argc, char ** argv)
{
    struct particle_buffer * buffer = particle_buffer_create();
    struct lex_result result;

    char * input = malloc(LINE_MAX);

    while (!feof(stdin)) {
        fgets(input, LINE_MAX, stdin);
        if (feof(stdin)) break;

        lex(input, buffer, &result);

        if (result.type == LEX_ERROR) {
            if (!isatty(fileno(stdin))) {
                size_t i;
                for (i = 0; input[i]; i++);
                if (i == 0 || input[i-1] != '\n') {
                    printf("%s\n", input);
                } else {
                    printf("%s", input);
                }
            }
            for (size_t i = 0; i < result.index; i++) {
                printf(" ");
            }
            printf("^error\n");
        } else {

            for (size_t n = 0; n < buffer->n_particles; n++) {
                struct refstring * string = particle_string(buffer->particles[n]);
                printf("%s%s", (n == 0) ? "" : " ", refstring_string(string));
                refstring_destroy(string);
            }
            printf("\n");

        }

        particle_buffer_free_all(buffer);
    }

    free(input);

    particle_buffer_destroy(buffer);
}
