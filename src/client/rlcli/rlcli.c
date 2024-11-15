/* File: src/client/rlcli/rlcli.c
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

#include "client/rlcli/args.h"

#include "linenoise.h"

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

/* NOTE: this does not actaully build under mingw/--build=w64 because lineoise
 *       depends on termios.h and is not trivial to port to Windows
 */
#if defined(__MINGW32__)
#include <windef.h>
#endif /* __MINGW32__ */

/* this client is not currently built
 *
 * to build it, clone a copy of linenoise in libs and add the linenoise.h and
 * linenoise.c to the build, and add a target to build this that uses this
 * source, the appropriate rlcli args source, and linenoise.c
 *
 * it doesn't need to be build with pthreads or readline
 */

struct in_readcb_arg {
    struct linenoiseState * ls;
    struct bufferevent * bev_net;
    char * buffer;
    size_t buffer_size;
};

void in_readcb(evutil_socket_t socket, short all, void * arg)
{
    (void)socket;
    (void)all;
    struct in_readcb_arg * in_readcb_arg = arg;
    struct linenoiseState * ls = in_readcb_arg->ls;
    char * buffer = in_readcb_arg->buffer;
    size_t buffer_size = in_readcb_arg->buffer_size;
    struct evbuffer * output = bufferevent_get_output(in_readcb_arg->bev_net);

    char * line = linenoiseEditFeed(ls);
    if (line == linenoiseEditMore) {
        return;
    }
    linenoiseEditStop(ls);
    if (!line) {
        evbuffer_add_printf(output, "EXIT\n");
        return;
    }
    linenoiseHistoryAdd(line);
    evbuffer_add_printf(output, "%s\n", line);
    linenoiseFree(line);
    linenoiseEditStart(ls, -1, -1, buffer, buffer_size, "# ");
}

static void net_readcb(struct bufferevent * bev, void * ctx)
{
    struct linenoiseState * ls = ctx;

    struct evbuffer * input = bufferevent_get_input(bev);
    size_t n;
    char * line;

    linenoiseHide(ls);
    while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_ANY))) {
        printf("%s\r\n", line);
        free(line);
    }
    linenoiseShow(ls);
}

static void net_eventcb(struct bufferevent * bev, short events, void * ctx)
{
    struct linenoiseState * ls = ctx;

    if (events & BEV_EVENT_CONNECTED) {
        printf("[cli] connected\r\n");
        linenoiseShow(ls);
        bufferevent_enable(bev, EV_READ);
    } else if (events & BEV_EVENT_ERROR) {
        linenoiseHide(ls);
        printf("[cli] error connecting\r\n");
        event_base_loopexit(bufferevent_get_base(bev), NULL);
    } else if (events & BEV_EVENT_EOF) {
        linenoiseHide(ls);
        printf("[cli] disconnected\r\n");
        event_base_loopexit(bufferevent_get_base(bev), NULL);
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
        .portname = strdup("10101"),
        .hostname = strdup("127.0.0.1")
    };

    int parse_result;
    if ((parse_result = parse_args(&args, argc, argv))) {
        free_args(&args);
        return parse_result;
    }

#if defined(__MINGW32__)
    SetConsoleMode(0, 0x4);
#endif /* __MINGW32__ */

    size_t ls_buffer_size = 16 * 1024;
    char * ls_buffer = malloc(ls_buffer_size);
    struct linenoiseState ls;
    linenoiseEditStart(&ls, -1, -1, ls_buffer, ls_buffer_size, "# ");
    linenoiseHide(&ls);

#if defined(__MINGW32__)
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    int err;
    if ((err = WSAStartup(wVersionRequested, &wsaData))) {
        fprintf(stderr, "[cli] WSAStartup() failed (code %d\r\n", err);
        free(args.portname);
        free(args.hostname);
        return 1;
    }
#endif /* __MINGW32__ */

    struct event_base * base = event_base_new();

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
                "[cli] error resolving '%s': %s\r\n",
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

    struct bufferevent * bev_net = bufferevent_socket_new(base, -1, 0);

    bufferevent_setcb(bev_net, net_readcb, NULL, net_eventcb, &ls);

    if (bufferevent_socket_connect(
                bev_net,
                answer->ai_addr,
                sizeof(*answer->ai_addr)
            )) {
        fprintf(stderr, "[cli] error connecting\r\n");
        free_args(&args);
        evutil_freeaddrinfo(answer);
        bufferevent_free(bev_net);
        event_base_free(base);
        libevent_global_shutdown();

#if defined(__MINGW32__)
        WSACleanup();
#endif /* __MINGW32__ */

        return 1;
    }

    evutil_freeaddrinfo(answer);

    char * line = NULL;
    /* TODO */
    size_t line_max = 1024 * 1024 * 1024;

    if (args.n_load_files > 0) {
        line = malloc(line_max);
    }

    for (size_t i = 0; i < args.n_load_files; i++) {
        printf("[cli] loading input from \"%s\"\r\n", args.load_files[i]);
        FILE * f = fopen(args.load_files[i], "r");
        if (!f) {
            fprintf(stderr, "[cli] error: %s\r\n", strerror(errno));
            continue;
        }
        while (fgets(line, line_max, f)) {
            evbuffer_add_printf(bufferevent_get_output(bev_net), "%s", line);
        }
        fclose(f);
    }

    if (line) {
        free(line);
    }

    free_args(&args);

    struct in_readcb_arg arg = (struct in_readcb_arg) {
        .ls = &ls,
        .bev_net = bev_net,
        .buffer = ls_buffer,
        .buffer_size = ls_buffer_size
    };

    struct event * in_ev = event_new(
            base, 0, EV_READ | EV_PERSIST, &in_readcb, &arg);
    event_add(in_ev, NULL);
    event_base_dispatch(base);

    linenoiseEditStop(&ls);
    free(ls_buffer);
    event_free(in_ev);
    bufferevent_free(bev_net);
    event_base_free(base);
    libevent_global_shutdown();

#if defined(__MINGW32__)
    WSACleanup();
#endif /* __MINGW32__ */
}
