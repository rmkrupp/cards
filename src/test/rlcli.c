/* File: include/test/cli.h
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

#include <getopt.h>

#include <pthread.h>
#include <readline/readline.h>
#include <readline/history.h>

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>
#include <event2/dns.h>
#include <event2/util.h>

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

/*
static void net_readcb(struct bufferevent * bev, void * ctx)
{
    (void)ctx;
    struct evbuffer * input = bufferevent_get_input(bev);
    size_t n;
    char * line;
    
    while ((line = evbuffer_readln(input, &n, EVBUFFER_EOL_ANY))) {
        printf("%s\n", line);
        rl_on_new_line();
        free(line);
    }
}
*/

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

static struct option options[] = {
    { "port", required_argument, 0, 'p' },
    { "hostname", required_argument, 0, 'h' },
    { }
};

int main(int argc, char ** argv)
{

    char * portname = strdup("10101");
    char * hostname = strdup("127.0.0.1");

    int c;
    while (1) {
        int index = 0;
        c = getopt_long(argc, argv, "p:h:", options, &index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 'p':
                free(portname);
                portname = strdup(optarg);
                break;
            case 'h':
                free(hostname);
                hostname = strdup(optarg);
                break;
            case '?':
                free(portname);
                free(hostname);
                return 1;
            default:
                free(portname);
                free(hostname);
                return 2;
        }
    }

#if defined(__MINGW32__)
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    int err;
    if ((err = WSAStartup(wVersionRequested, &wsaData))) {
        fprintf(stderr, "[cli] WSAStartup() failed (code %d)\n", err);
        free(portname);
        free(hostname);
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
    int result = evutil_getaddrinfo(hostname, portname, &hints, &answer);

    if (result) {
        fprintf(
                stderr,
                "[cli] error resolving '%s': %s\n",
                hostname,
                evutil_gai_strerror(result)
            );
        free(hostname);
        free(portname);
        event_base_free(base);
        libevent_global_shutdown();

#if defined(__MINGW32__)
        WSACleanup();
#endif /* __MINGW32__ */

        return 1;
    }

    free(hostname);
    free(portname);

    struct bufferevent * bev_net = bufferevent_socket_new(
            base, -1, 0);

    bufferevent_setcb(bev_net, NULL, NULL, net_eventcb, NULL);

    if (bufferevent_socket_connect(
                bev_net,
                answer->ai_addr,
                sizeof(*answer->ai_addr)
            )) {
        fprintf(stderr, "[cli] error connecting\n");
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

    for (int i = optind; i < argc; i++) {
        printf("[cli] loading input from \"%s\"\n", argv[i]);
        FILE * f = fopen(argv[i], "r");
        if (!f) {
            fprintf(stderr, "[cli] error: %s\n", strerror(errno));
            continue;
        }
        char * line = malloc(1024 * 1024 * 1024);
        while (fgets(line, 1024 * 1024 * 1024, f)) {
            evbuffer_add_printf(bufferevent_get_output(bev_net), "%s", line);
        }
        free(line);
        fclose(f);
    }

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
