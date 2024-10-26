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

int main(int argc, char ** argv)
{
    struct particle_buffer * buffer = particle_buffer_create();

    if (argc > 1) {
        struct lex_result result;
        lex(argv[1], buffer, &result);
        printf("n = %lu\n", result.index);
        for (size_t n = 0; n < buffer->n_particles; n++) {
            struct refstring * string = particle_string(buffer->particles[n]);
            printf("%s\n", refstring_string(string));
            refstring_destroy(string);
        }
    };

    particle_buffer_destroy(buffer);
}
