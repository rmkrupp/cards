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
#include "bundle.h"
#include "name_set.h"
#include "config.h"
#include "util/log.h"

#include <stdlib.h>

/* create a game with this config */
[[nodiscard]] struct game * game_create(
        struct config * config) [[gnu::nonnull(1)]]
{
    struct game * game = malloc(sizeof(*game));
    if (!game) {
        return NULL;
    }
    *game = (struct game) {
        .name_set = name_set_create(),
        .logger = config->logger
    };

    if (!game->name_set) {
        free(game);
        return NULL;
    }

    if (config->default_card_db) {
        LOGF_INFO(
                game->logger, "loading bundle %s\n", config->default_card_db);
        size_t errors;
        enum bundle_load_result result = bundle_load(
                config->default_card_db,
                game->name_set,
                &errors,
                config->logger
            );
        /* TODO */
        (void)result;
    }
    return game;
}

/* destroy this game */
void game_destroy(struct game * game) [[gnu::nonnull(1)]]
{
    name_set_destroy(game->name_set);
    free(game);
}
