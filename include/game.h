/* File: include/game.h
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
#ifndef GAME_H
#define GAME_H

/* forward declare */
struct config;
struct logger;
struct name_set;

/* a game */
struct game {
    struct logger * logger;
    struct name_set * name_set;
};

/* create a game with this config */
[[nodiscard]] struct game * game_create(
        struct config * config) [[gnu::nonnull(1)]];

/* destroy this game */
void game_destroy(struct game * game) [[gnu::nonnull(1)]];

#endif /* GAME_H */
