/* File: src/client/cli.c
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
#include <stdlib.h>
#include <string.h>

#include "util/strdup.h"

#include "client/cli/args.h"

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#if defined(__MINGW32__)
#include <windef.h>
#endif /* __MINGW32__ */

static void net_readcb(struct bufferevent * bev, void * ctx)
{
    (void)ctx;
    struct evbuffer * input = bufferevent_get_input(bev);
    size_t n;
    char * line;
    
    while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_ANY))) {
        printf("%s\n", line);
        free(line);
    }
}

static void in_readcb(struct bufferevent * bev, void * ctx)
{
    struct bufferevent * bev_net = ctx;

    struct evbuffer * input = bufferevent_get_input(bev);
    struct evbuffer * output = bufferevent_get_output(bev_net);
    size_t n;
    char * line;
    
    while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_ANY))) {
        evbuffer_add_printf(output, "%s\n", line);
        free(line);
    }
}

static void net_eventcb(struct bufferevent * bev, short events, void * ctx)
{
    struct bufferevent * bev_in = ctx;
    if (events & BEV_EVENT_CONNECTED) {
        fprintf(stderr, "[cli] connected\n");
        bufferevent_enable(bev, EV_READ);
        bufferevent_enable(bev_in, EV_READ);
    } else if (events & BEV_EVENT_ERROR) {
        fprintf(stderr, "[cli] error connecting\n");
        event_base_loopexit(bufferevent_get_base(bev), NULL);
    } else if (events & BEV_EVENT_EOF) {
        fprintf(stderr, "[cli] disconnected\n");
        event_base_loopexit(bufferevent_get_base(bev), NULL);
    }
}

static void in_eventcb(struct bufferevent * bev, short events, void * ctx)
{
    struct bufferevent * bev_net = ctx;

    if (events & BEV_EVENT_ERROR ){
        fprintf(stderr, "[cli] input error\n");
        event_base_loopexit(bufferevent_get_base(bev), NULL);
    } else if (events & BEV_EVENT_EOF) {
        fprintf(stderr, "[cli] input closed, requesting disconnect\n");
        evbuffer_add_printf(bufferevent_get_output(bev_net), "exit\n");
    }
}

static void free_args(struct arguments * args)
{
    free(args->portname);
    free(args->hostname);
    for (size_t i = 0; i < args->n_load_files; i++) {
        free(args->load_files[i]);
    }
    free(args->load_files);
}

int main(int argc, char ** argv)
{

    struct arguments args = {
        .portname = util_strdup("10101"),
        .hostname = util_strdup("127.0.0.1")
    };

    int parse_result;
    if ((parse_result = parse_args(&args, argc, argv))) {
        free_args(&args);
        return parse_result;
    }
    
#if defined(__MINGW32__)
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    int err;
    if ((err = WSAStartup(wVersionRequested, &wsaData))) {
        fprintf(stderr, "[cli] WSAStartup() failed (code %d)\n", err);
        free_args(&args);
        return 1;
    }
#endif /* __MINGW32__ */

    struct event_base * base = event_base_new();

    /*
    struct sockaddr_in sin = (struct sockaddr_in) {
        .sin_family = AF_INET,
        .sin_addr = (struct in_addr) {
            .s_addr = htonl((192L << 24) | (168L << 16) | 11L)
        },
        .sin_port = htons(port)
    };
    */

    struct evutil_addrinfo hints = (struct evutil_addrinfo) {
        .ai_family = AF_UNSPEC,
        .ai_socktype = SOCK_STREAM,
        .ai_protocol = IPPROTO_TCP,
        .ai_flags = EVUTIL_AI_ADDRCONFIG
    };
    struct evutil_addrinfo * answer = NULL;
    int result = evutil_getaddrinfo(
            args.hostname, args.portname, &hints, &answer);

    if (result) {
        fprintf(
                stderr,
                "[cli] error resolving '%s': %s\n",
                args.hostname,
                evutil_gai_strerror(result)
            );
        free_args(&args);
        event_base_free(base);
        libevent_global_shutdown();

#if defined(__MINGW32__)
        WSACleanup();
#endif /* __MINGW32__ */

        return 1;
    }

    struct bufferevent * bev_in = bufferevent_socket_new(
        base, 0, BEV_OPT_CLOSE_ON_FREE);

    struct bufferevent * bev_net = bufferevent_socket_new(
            base, -1, 0);

    bufferevent_setcb(bev_net, net_readcb, NULL, net_eventcb, bev_in);
    bufferevent_setcb(bev_in, in_readcb, NULL, in_eventcb, bev_net);

    if (bufferevent_socket_connect(
                bev_net,
                answer->ai_addr,
                sizeof(*answer->ai_addr)
            )) {
        fprintf(stderr, "[cli] error connecting\n");
        free_args(&args);
        evutil_freeaddrinfo(answer);
        bufferevent_free(bev_net);
        bufferevent_free(bev_in);
        event_base_free(base);
        libevent_global_shutdown();

#if defined(__MINGW32__)
        WSACleanup();
#endif /* __MINGW32__ */

        return 1;
    }

    evutil_freeaddrinfo(answer);

    for (size_t i = 0; i < args.n_load_files; i++) {
        printf("[cli] loading input from \"%s\"\n", args.load_files[i]);
        FILE * f = fopen(args.load_files[i], "r");
        if (!f) {
            fprintf(stderr, "[cli] error: %s\n", strerror(errno));
            continue;
        }
        /* TODO */
        char * line = malloc(1024 * 1024 * 1024);
        if (!line) {
            fprintf(stderr, "OOM (malloc returned NULL)\n");
            fclose(f);
            return 1;
        }
        while (fgets(line, 1024 * 1024 * 1024, f)) {
            evbuffer_add_printf(bufferevent_get_output(bev_net), "%s", line);
        }
        free(line);
        fclose(f);
    }

    if (args.send_exit) {
        evbuffer_add_printf(bufferevent_get_output(bev_net), "EXIT\n");
    }

    free_args(&args);

    event_base_dispatch(base);

    bufferevent_free(bev_net);
    bufferevent_free(bev_in);
    event_base_free(base);
    libevent_global_shutdown();

#if defined(__MINGW32__)
    WSACleanup();
#endif /* __MINGW32__ */

}
