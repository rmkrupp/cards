/* File: src/game.c
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
#include "game.h"

#include <stdlib.h>
#include <stdbool.h>

/* a player with an id and a connectedness */
struct player {
    size_t id;
    bool connected;
};

/* a game with and expandable array of player slots
 *
 * new players will fill holes in these slots left by the prior removal of any
 * players (these slots will hold NULL until filled again.)
 *
 * if there are no holes, the player_slots array will be realloc'd and expanded
 */
struct game {
    struct player ** player_slots;
    size_t n_player_slots;
    size_t n_players;
};

/* create and return a new player with this id */
struct player * player_create(size_t id)
{
    struct player * player = malloc(sizeof(*player));
    *player = (struct player) {
        .id = id,
        .connected = false
    };
    return player;
}

/* free the resources of this player */
void player_destroy(struct player * player)
{
    free(player);
}

/* get whether this player is connected */
void player_set_connected(struct player * player, bool connected)
{
    player->connected = connected;
}

/* set whether this player is connected */
bool player_get_connected(struct player * player)
{
    return player->connected;
}

/* get the id of this player */
size_t player_get_id(struct player * player)
{
    return player->id;
}
/* create and return a game */

/* create and return a game */
struct game * game_create()
{
    struct game * game = malloc(sizeof(*game));
    *game = (struct game) { 0 };
    return game;
}

/* free the resources of this game */
void game_destroy(struct game * game)
{
    for (size_t n = 0; n < game->n_player_slots; n++) {
        if (game->player_slots[n]) {
            player_destroy(game->player_slots[n]);
        }
    }
    free(game->player_slots);

    free(game);
}

/* add this player to this game */
void game_add_player(struct game * game, struct player * player)
{
    game->n_players++;

    for (size_t n = 0; n < game->n_player_slots; n++) {
        if (!game->player_slots[n]) {
            game->player_slots[n] = player;
            return;
        }
    }

    game->player_slots = realloc(game->player_slots,
            sizeof(*game->player_slots) * (game->n_player_slots + 1));
    game->player_slots[game->n_player_slots] = player;
    game->n_player_slots++;
}

/* remove this player from this game
 * returns: 1 if succesful, 0 if this player was not in this game
 */
int game_remove_player(struct game * game, struct player * player)
{
    for (size_t n = 0; n < game->n_player_slots; n++) {
        if (game->player_slots[n] == player) {
            game->player_slots[n] = NULL;
            game->n_players--;
            return 1;
        }
    }

    return 0;
}
