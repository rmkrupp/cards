/* File: src/main.c
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

#include <event2/event.h>

#include "config.h"
#include "game.h"
#include "networker.h"

int main(int argc, char ** argv) {

    struct config config = (struct config) { };

    if (config_load(&config, argc, argv)) {
        return 1;
    }

    printf("port = %ld\n", config.port);

    struct game * game = game_create();

    struct networker * networker = networker_create(&config, game);

    printf("networker_run()\n");

    networker_run(networker);

    printf("networker_destroy()\n");

    networker_destroy(networker);
    game_destroy(game);

    config_free(&config);

    libevent_global_shutdown();

    return 0;
}
