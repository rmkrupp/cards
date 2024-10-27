/* File: src/server.c
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
#include "server.h"

#include <stdlib.h>

#include "networker.h"

/* create a server with this config
 *
 * this will also create a networker inside this server
 */
struct server * server_create(struct config * config)
{
    struct server * server = malloc(sizeof(*server));

    *server = (struct server) {
        .config = config,
        .networker = networker_create(config)
    };

    if (!server->networker) {
        free(server);
        return NULL;
    }

    return server;
}

/* destroy this server and its networker
 *
 * does not free the config, since that was passed in and might be reused
 */
void server_destroy(struct server * server)
{
    networker_destroy(server->networker);
    free(server);
}

/* enter the server's event loop
 *
 * return result is the same as networker_run()
 * (because right now all this does is networker_run(server->networker))
 */
int server_run(struct server * server)
{
    return networker_run(server->networker);
}
