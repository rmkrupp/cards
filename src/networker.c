/* File: src/networker.c
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
#include "networker.h"
#include "config.h"
#include "game.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <arpa/inet.h>

/* a networker holds the state of networking aparatus */
struct networker {
    struct game * game;
    struct connection ** connections;
    size_t n_connections;
    struct event_base * base;
    struct evconnlistener * listener;
    size_t next_id;
};

/* a connection is the context given to each connection created by the
 * networker
 * */
struct connection {
    struct networker * networker;
    struct player * player;
    struct bufferevent * bev;
};

/* create a connection with the given networker and bufferevent */
static struct connection * connection_create(
        struct networker * networker,
        struct bufferevent * bev
    )
{
    struct connection * connection = malloc(sizeof(*connection));
    *connection = (struct connection) {
        .networker = networker,
        .player = player_create(networker->next_id++),
        .bev = bev
    };

    game_add_player(networker->game, connection->player);

    networker->connections = realloc(
            networker->connections,
            sizeof(*networker->connections) * (networker->n_connections + 1)
        );

    networker->connections[networker->n_connections] = connection;
    networker->n_connections++;

    return connection;
}

/* destroy a connection, remove it from its networker, and remove its player
 * from the game
 */
static void connection_destroy(struct connection * connection)
{
    for (size_t n = 0; n < connection->networker->n_connections; n++) {
        if (connection->networker->connections[n] == connection) {
            connection->networker->connections[n] = NULL;
        }
    }

    game_remove_player(connection->networker->game, connection->player);
    player_destroy(connection->player);
    bufferevent_free(connection->bev);

    free(connection);
}

/* dummy read callback */
static void example_read_cb(struct bufferevent * bev, void * ptr)
{
    struct connection * connection = ptr;

    struct evbuffer * input = bufferevent_get_input(bev);
    struct evbuffer * output = bufferevent_get_output(bev);

    size_t n = 0;
    do {
        char * line = evbuffer_readln(input, &n, EVBUFFER_EOL_LF);
        if (line) {

            if (strcmp(line, "exit") == 0) {
                connection_destroy(connection);
                free(line);
                return;
            }

            if (strcmp(line, "shutdown") == 0) {
                event_base_loopexit(connection->networker->base, NULL);
            }

            evbuffer_add_printf(
                    output,
                    "[%ld] %s\n",
                    player_get_id(connection->player),
                    line
                );
            free(line);
        }
    } while (n > 0);
}

/* dummy event callback */
static void example_event_cb(struct bufferevent * bev, short events, void * ptr)
{
    struct connection * connection = ptr;

    if (events & BEV_EVENT_ERROR) {
        perror("echo_event_cb() called for error");
    }

    if (events & (BEV_EVENT_EOF | BEV_EVENT_ERROR)) {
        connection_destroy(connection);
    }
}

/* listener callback creates connection objects for each new connection */
static void networker_listener_accept_cb(
        struct evconnlistener * listener,
        evutil_socket_t sock,
        struct sockaddr * addr,
        int len,
        void * ptr
    )
{
    struct networker * networker = ptr;
    struct event_base * base = evconnlistener_get_base(listener);

    struct bufferevent * bev = bufferevent_socket_new(
            base,
            sock,
            BEV_OPT_CLOSE_ON_FREE
        );

    struct connection * connection = connection_create(networker, bev);

    bufferevent_setcb(
            bev, &example_read_cb, NULL, &example_event_cb, connection);
    bufferevent_enable(bev, EV_READ | EV_WRITE);
}

/* listener error callback exits the eventloop on listener error */
static void networker_listener_error_cb(
        struct evconnlistener * listener,
        void * ptr
    )
{
    //struct networker * networker = ptr;
    struct event_base * base = evconnlistener_get_base(listener);
    int error = EVUTIL_SOCKET_ERROR();
    fprintf(
            stderr,
            "listener error %d (%s)\n",
            error,
            evutil_socket_error_to_string(error)
        );
    event_base_loopexit(base, NULL);
}

/* reutrn a new networker based on config and holding game */
struct networker * networker_create(struct config * config, struct game * game)
{
    int port = (int)config->port;

    if ((long)port != config->port) {
        fprintf(stderr, "[networker] config.port does not fit in int\n");
        return NULL;
    }

    struct sockaddr_in sin = (struct sockaddr_in) {
        .sin_family = AF_INET,
        .sin_addr = (struct in_addr) {
            .s_addr = htonl(0)
        },
        .sin_port = htons(port)
    };

    struct networker * networker = malloc(sizeof(*networker));

    *networker = (struct networker) {
        .game = game
    };

    networker->base = event_base_new();
    if (!networker->base) {
        fprintf(stderr, "[networker] event_base_new() failed\n");
        free(networker);
        return NULL;
    }

    networker->listener = evconnlistener_new_bind(
            networker->base,
            &networker_listener_accept_cb,
            networker,
            LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
            -1,
            (struct sockaddr *)&sin, sizeof(sin)
        );

    if (!networker->listener) {
        fprintf(stderr, "[networker] evconnlistener_new_bind() failed\n");
        event_base_free(networker->base);
        free(networker);
        return NULL;
    }

    evconnlistener_set_error_cb(
            networker->listener,
            &networker_listener_error_cb
        );

    return networker;
}

/* free the resources (connections, events, etc.) of this networker */
void networker_destroy(struct networker * networker)
{
    evconnlistener_free(networker->listener);
    for (size_t n = 0; n < networker->n_connections; n++) {
        if (networker->connections[n]) {
            connection_destroy(networker->connections[n]);
        }
    }
    free(networker->connections);
    event_base_free(networker->base);
    free(networker);
}

/* run the eventloop of this networker */
void networker_run(struct networker * networker)
{
    event_base_dispatch(networker->base);
}
