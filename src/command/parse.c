/* File: include/command/parse.c
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
#include "command/parse.h"
#include "command/keyword.h"

#include <stdio.h>
#include <stdlib.h>

struct parser {
    // nothing
};

struct parser * parser_create()
{
    struct parser * parser = malloc(sizeof(*parser));
    *parser = (struct parser) { };
    return parser;
}

void NONNULL(1) parser_destroy(struct parser * parser)
{
    free(parser);
}

void NONNULL(1) parser_parse(
        struct parser * parser,
        struct particle_buffer * particles,
        struct parse_result * result
    )
{
    for (size_t i = 0; i < particles->n_particles; i++) {
        struct particle * particle = particles->particles[i];
        struct refstring * s = particle_string(particle);
        if (particle->type == PARTICLE_KEYWORD) {
            if (keyword_lookup(particle->value, particle->length)) {
                printf("%s: yes\n", refstring_string(s));
            } else {
                printf("%s: no\n", refstring_string(s));
            }
        } else {
            printf("%s: n/a\n", refstring_string(s));
        }
        refstring_destroy(s);
    }
}
