/* File: include/card.h
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
#ifndef CARD_H
#define CARD_H

#include <stddef.h>

/* fordward declare */
struct name_set;
struct logger;

/* a card */
struct card;

/* a card ability */
struct ability;

/* a card subtype */
struct subtype;

/* destroy this card */
void card_destroy(struct card * card) [[gnu::nonnull(1)]];

/* destroy this ability */
void ability_destroy(struct ability * ability) [[gnu::nonnull(1)]];

/* destroy this subtype */
void subtype_destroy(struct subtype * subtype) [[gnu::nonnull(1)]];

/* create a card from this Lua data
 *
 * add its name and the names of any of its abilities to name_set, associating
 * them with this card (thus, it is okay to ignore the return value of this
 * call.)
 *
 * returns NULL if there is an error loading or running the Lua, or if the
 * card's name is not unique, or if the name field of the card or its abilities
 * are absent or not string, or if the indices of abilities tables are not
 * numbers. Note that ability names do not need to be unique.
 */
struct card * card_load(
        const char * data,
        size_t length,
        const char * filename,
        struct name_set * name_set,
        struct logger * logger
    ) [[gnu::nonnull(1, 3, 4)]];

#endif /* CARD_H */
