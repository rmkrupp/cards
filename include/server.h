/* File: include/server.h
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
#ifndef SERVER_H
#define SERVER_H

#include "attributes.h"

/* the state of a server */
struct server {
    struct config * config; /* the configuration, set on creation */
    struct networker * networker; /* the networker, created by the server */
    struct game * game; /* the game */
};

/* create a server with this config
 *
 * this will also create a networker inside this server
 */
struct server * NONNULL(1) server_create(struct config * config);

/* destroy this server and its networker
 *
 * does not free the config, since that was passed in and might be reused
 */
void NONNULL(1) server_destroy(struct server * server);

/* enter the server's event loop
 *
 * return result is the same as networker_run()
 * (because right now all this does is networker_run(server->networker))
 */
int NONNULL(1) server_run(struct server * server);

#endif /* SERVER_H */
