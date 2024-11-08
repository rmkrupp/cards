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
#include "util/log.h"

#include "command/lex.h"
#include "command/parse.h"
#include "game.h"

#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <event2/listener.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

/* a networker holds the state of networking apparatus */
struct networker {
    struct logger * logger;
    struct event_base * base;
    struct evconnlistener * listener;
    struct connection ** connections;
    size_t n_connections;
    int errors;

    struct game * game;
};

/* a connection is the context given to each connection created by the
 * networker
 * */
struct connection {
    size_t id;
    struct networker * networker;
    struct bufferevent * bev;

    /* TODO */
    /* for now, store one of these here,
     * soon we will have a parser object
     */
    struct particle_buffer * buffer;
    struct parser * parser;
};

/* iterator over networker->connections */
struct networker_connection_iter {
    struct networker * networker;
    size_t index;
};

/* create a connection with the given networker and bufferevent */
[[nodiscard]] static struct connection * connection_create(
        struct networker * networker,
        struct bufferevent * bev
    ) [[gnu::nonnull(1, 2)]]
{
    struct connection * connection = malloc(sizeof(*connection));
    *connection = (struct connection) {
        .id = networker->n_connections,
        .networker = networker,
        .bev = bev,
        .buffer = particle_buffer_create(),
        .parser = parser_create(networker->game)
    };

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
static void connection_destroy(
        struct connection * connection) [[gnu::nonnull(1)]]
{
    for (size_t n = 0; n < connection->networker->n_connections; n++) {
        if (connection->networker->connections[n] == connection) {
            connection->networker->connections[n] = NULL;
            break;
        }
    }

    bufferevent_free(connection->bev);

    /* until we have a parser object */
    particle_buffer_destroy(connection->buffer);
    parser_destroy(connection->parser);

    free(connection);
}

/* dummy read callback */
static void example_read_cb(struct bufferevent * bev, void * ptr)
{
    struct connection * connection = ptr;

    struct evbuffer * input = bufferevent_get_input(bev);

    size_t n = 0;
    do {
        char * line = evbuffer_readln(input, &n, EVBUFFER_EOL_ANY);
        if (line) {

            if (strcmp(line, "exit") == 0) {
                connection_destroy(connection);
                free(line);
                return;
            }

            if (strncmp(line, "say ", 4) == 0) {
                for (size_t i = 0; i < connection->networker->n_connections; i++) {
                    if (connection->networker->connections[i]) {
                        struct evbuffer * output =
                            bufferevent_get_output(
                                    connection->networker->connections[i]->bev);
                        evbuffer_add_printf(output, "%s\n", &line[4]);
                    }
                }
                free(line);
                continue;
            }

            if (strcmp(line, "shutdown") == 0) {
                event_base_loopexit(connection->networker->base, NULL);
                free(line);
                continue;
            }

            /* minimal lexing code for testing */
            struct lex_result result;
            lex(line, connection->parser, connection->buffer, &result);

            if (result.type == LEX_ERROR) {
                evbuffer_add_printf(bufferevent_get_output(connection->bev), "error\n");
            } else {
                struct parse_result parse_result;
                parser_parse(connection->parser, connection->buffer, &parse_result);
            }
            particle_buffer_free_all(connection->buffer);
            /* ------------------------------- */

            free(line);
        }
    } while (n > 0);
}

/* dummy event callback */
static void example_event_cb(
        struct bufferevent * bev, short events, void * ptr)
{
    (void)bev;
    struct connection * connection = ptr;

    if (events & BEV_EVENT_ERROR) {
        connection->networker->errors++;
        LOGF_ERROR(
                connection->networker->logger,
                "echo_event_cb() called for error %s\n",
                strerror(errno)
            );
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
    (void)len;
    (void)addr;

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

    struct evbuffer * output = bufferevent_get_output(bev);
    evbuffer_add_printf(
            output,
            "[server] welcome, you are %" PRIu64 "\n",
            (uint64_t)connection->id
        );
}

/* listener error callback exits the eventloop on listener error */
static void networker_listener_error_cb(
        struct evconnlistener * listener, void * ptr)
{
    struct networker * networker = ptr;
    networker->errors++;

    struct event_base * base = evconnlistener_get_base(listener);
    int error = EVUTIL_SOCKET_ERROR();
    LOGF_ERROR(
            networker->logger,
            "[networker] listener error %d (%s)\n",
            error,
            evutil_socket_error_to_string(error)
        );
    event_base_loopexit(base, NULL);
}

/* reutrn a new networker based on config and holding game */
[[nodiscard]] struct networker * networker_create(
        struct config * config) [[gnu::nonnull(1)]]
{
    int port = (int)config->port;

    if ((long)port != config->port) {
        LOGF_ERROR(
                config->logger,
                "[networker] config.port does not fit in int\n"
            );
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
        .logger = config->logger,
        .game = game_create(config)
    };

    networker->base = event_base_new();
    if (!networker->base) {
        LOGF_ERROR(
                networker->logger,
                "[networker] event_base_new() failed\n"
            );
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
        LOGF_ERROR(
                networker->logger,
                "[networker] evconnlistener_new_bind() failed\n"
            );
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
void networker_destroy(struct networker * networker) [[gnu::nonnull(1)]]
{
    evconnlistener_free(networker->listener);
    for (size_t n = 0; n < networker->n_connections; n++) {
        if (networker->connections[n]) {
            connection_destroy(networker->connections[n]);
        }
    }
    free(networker->connections);
    event_base_free(networker->base);
    game_destroy(networker->game);
    free(networker);
}

/* run the eventloop of this networker */
int networker_run(struct networker * networker) [[gnu::nonnull(1)]]
{
    event_base_dispatch(networker->base);
    return networker->errors;
}

/* returns a new iterator over the networker's connections */
[[nodiscard]] struct networker_connection_iter * networker_connection_iter_create(
        struct networker * networker) [[gnu::nonnull(1)]]
{
    struct networker_connection_iter * iter = malloc(sizeof(*iter));
    *iter = (struct networker_connection_iter) {
        .networker = networker,
        .index = 0
    };
    return iter;
}

/* destroys this iterator */
void networker_connection_iter_destroy(
        struct networker_connection_iter * iter) [[gnu::nonnull(1)]]
{
    free(iter);
}

/* returns the next connection and advances the iterator */
struct connection * networker_connection_iter_iterate(
        struct networker_connection_iter * iter) [[gnu::nonnull(1)]]
{
    if (iter->index >= iter->networker->n_connections) {
        return NULL;
    }

    while (!iter->networker->connections[iter->index]) {
        iter->index++;
    }

    struct connection * connection = iter->networker->connections[iter->index];
    iter->index++;

    return connection;
}
