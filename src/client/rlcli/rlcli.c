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

#include "client/rlcli/args.h"

#include <pthread.h>

/* these defines are needed to make rl_message work right */
#define USE_VARARGS
#define PREFER_STDARG
#include <readline/readline.h>
#include <readline/history.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

#if defined(__MINGW32__)
#include <windef.h>
#endif /* __MINGW32__ */

struct bufferevent * global_bev = NULL;

static int hook()
{
    struct bufferevent * bev = global_bev;
    if (!bev) {
        return 0;
    }
    struct evbuffer * input = bufferevent_get_input(bev);
    int needsclear = 1;
    size_t n;
    char * linein;
    while ((linein = evbuffer_readln(input, &n, EVBUFFER_EOL_ANY))) {
        if (needsclear) {
            rl_save_prompt();
            needsclear = 0;
        }
        rl_message("%s\n", linein);
        free(linein);
    }
    if (needsclear == 0) {
        rl_restore_prompt();
        rl_redisplay();
    }
    return 0;
}

static void * readline_fn(void * ctx)
{
    struct bufferevent ** bevptr = ctx;

    while (*bevptr) {
        char * line = readline("# ");
        struct bufferevent * bev = *bevptr;
        if (!bev) {
            free(line);
            return NULL;
        }
        if (line && line[0]) {
            add_history(line);

            struct evbuffer * output = bufferevent_get_output(bev);
            evbuffer_add_printf(output, "%s\n", line);
        }
        free(line);
    }

    return NULL;
}

static void net_eventcb(struct bufferevent * bev, short events, void * ctx)
{
    (void)ctx;
    if (events & BEV_EVENT_CONNECTED) {
        if (rl_readline_state == RL_STATE_NONE) {
            fprintf(stderr, "[cli] connected\n");
        } else {
            rl_message("[cli] connected\n");
        }
        bufferevent_enable(bev, EV_READ);
    } else if (events & BEV_EVENT_ERROR ) {
        if (rl_readline_state == RL_STATE_NONE) {
            fprintf(stderr, "[cli] error connecting\n");
        } else {
            rl_message("[cli] error connecting\n");
            rl_redisplay();
        }
        event_base_loopexit(bufferevent_get_base(bev), NULL);
    } else if (events & BEV_EVENT_EOF) {
        if (rl_readline_state == RL_STATE_NONE) {
            fprintf(stderr, "[cli] disconnected\n");
        } else {
            rl_message("[cli] disconnected\n");
            rl_redisplay();
        }
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

    struct arguments args = (struct arguments) {
        .portname = strdup("10101"),
        .hostname = strdup("127.0.0.1")
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

    struct bufferevent * bev_net = bufferevent_socket_new(
            base, -1, 0);

    bufferevent_setcb(bev_net, NULL, NULL, net_eventcb, NULL);

    if (bufferevent_socket_connect(
                bev_net,
                answer->ai_addr,
                sizeof(*answer->ai_addr)
            )) {
        fprintf(stderr, "[cli] error connecting\n");
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
    size_t line_max = 1024 * 1024 * 1024;

    if (args.n_load_files > 0) {
        line = malloc(line_max);
    }

    for (size_t i = 0; i < args.n_load_files; i++) {
        printf("[cli] loading input from \"%s\"\n", args.load_files[i]);
        FILE * f = fopen(args.load_files[i], "r");
        if (!f) {
            fprintf(stderr, "[cli] error: %s\n", strerror(errno));
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

    rl_event_hook = &hook;

    pthread_t readline_thread;
    int res = pthread_create(&readline_thread, NULL, &readline_fn, &global_bev);

    if (res) {
        fprintf(stderr, "[cli] error creating thread (%d)\n", res);

        bufferevent_free(bev_net);
        event_base_free(base);
        libevent_global_shutdown();

#if defined(__MINGW32__)
        WSACleanup();
#endif /* __MINGW32__ */
    }

    global_bev = bev_net;

    event_base_dispatch(base);

    global_bev = NULL;
    rl_done = 1;

    pthread_join(readline_thread, NULL);

    bufferevent_free(bev_net);
    event_base_free(base);
    libevent_global_shutdown();

#if defined(__MINGW32__)
    WSACleanup();
#endif /* __MINGW32__ */
}
