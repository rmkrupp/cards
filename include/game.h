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

#include <stdbool.h>
#include <stddef.h>

/* create and return a new player with this id */
struct player * player_create(size_t id);

/* free the resources of this player */
void player_destroy(struct player * player);

/* set whether this player is connected */
void player_set_connected(struct player * player, bool connected);

/* get whether this player is connected */
bool player_get_connected(struct player * player);

/* get the id of this player */
size_t player_get_id(struct player * player);

/* create and return a game */
struct game * game_create();

/* free the resources of this game */
void game_destroy(struct game * game);

/* add this player to this game */
void game_add_player(struct game * game, struct player * player);

/* remove this player from this game
 * returns: 1 if succesful, 0 if this player was not in this game
 */
int game_remove_player(struct game * game, struct player * player);

#endif /* GAME_H */
