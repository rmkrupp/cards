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

#include <event2/event.h>
#include <event2/bufferevent.h>
#include <event2/buffer.h>

static void net_readcb(struct bufferevent * bev, void * ctx)
{
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
    if (events & BEV_EVENT_CONNECTED) {
        fprintf(stderr, "[cli] connected\n");
        bufferevent_enable(bev, EV_READ);
        bufferevent_enable(ctx, EV_READ);
    } else if (events & BEV_EVENT_ERROR ){
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


int main(int argc, char ** argv)
{
    int port = 10101;

    struct event_base * base = event_base_new();

    struct sockaddr_in sin = (struct sockaddr_in) {
        .sin_family = AF_INET,
        .sin_addr = (struct in_addr) {
            .s_addr = htonl(0)
        },
        .sin_port = htons(port)
    };
    
    struct bufferevent * bev_in = bufferevent_socket_new(
        base, 0, BEV_OPT_CLOSE_ON_FREE);

    struct bufferevent * bev_net = bufferevent_socket_new(
            base, -1, 0);

    bufferevent_setcb(bev_net, net_readcb, NULL, net_eventcb, bev_in);
    bufferevent_setcb(bev_in, in_readcb, NULL, in_eventcb, bev_net);

    if (bufferevent_socket_connect(
                bev_net,
                (struct sockaddr *)&sin,
                sizeof(sin)
            )) {
        bufferevent_free(bev_net);
        bufferevent_free(bev_in);
        event_base_free(base);
        libevent_global_shutdown();
        return 1;
    }

    event_base_dispatch(base);

    bufferevent_free(bev_net);
    bufferevent_free(bev_in);
    event_base_free(base);
    libevent_global_shutdown();
}
