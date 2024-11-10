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

#include "server.h"
#include "config.h"
#include "util/log.h"

#if defined(__MINGW32__)
#include <windef.h>
#endif /* __MINGW32__ */

#define __USE_GNU
#include <stdlib.h>
#include <locale.h>

#include <uniconv.h>
#include <unistdio.h>

#include <event2/event.h>

#include <getopt.h>

static struct option options[] = {
    { "locale", required_argument, 0, 'l' },
    { "help", 0, 0, 1000 },
    { }
};

static void usage()
{
    ulc_fprintf(
            stderr,
            "Usage: cards [--help] [-l|--locale LOCALE] [CONFIG...]\n"
        );
}

int main(int argc, char ** argv) {
    /* read the locale from the system, first attempting to use the
     * environment variable LC_ALL, then LANG
     */
#if defined(__linux__)
    char * lc_all = secure_getenv("LC_ALL");
    if (!lc_all) {
        lc_all = secure_getenv("LANG");
    }
    setlocale(LC_ALL, lc_all);
#else
    /* TODO: does this work? */
    SetConsoleOutputCP(65001);
    /* TODO: is this needed? */
    printf("%s\n", setlocale(LC_ALL, ".UTF8"));
#endif /* __linux__ */

    /* now we can try and read the locale from the command line */
    while (1) {
        int index = 0;
        int c = getopt_long(argc, argv, "l:", options, &index);

        if (c == -1) {
            break;
        }

        switch (c) {
            case 'l':
                setlocale(LC_ALL, optarg);
                break;

            case 1000:
            case '?':
            default:
                usage();
                return 1;
        }
    }

    printf("locale = %s\n", locale_charset());

    struct config config = (struct config) { };

    if (config_load(&config, argc - optind, &argv[optind])) {
        return 1;
    }

    config.logger = logger_create(&config);

    LOGF_VERBOSE(config.logger, "version = %s\n", VERSION);
    LOGF_VERBOSE(config.logger, "port = %ld\n", config.port);

#if defined(__MINGW32__)
    WORD wVersionRequested = MAKEWORD(2, 2);
    WSADATA wsaData;
    int err;
    if ((err = WSAStartup(wVersionRequested, &wsaData))) {
        LOGF_ERROR(config.logger, "WSAStartup() failed (code %d)\n", err);
        logger_destroy(config.logger);
        return 1;
    }
#endif /* __MINGW32__ */

    struct server * server = server_create(&config);

    if (!server) {
#if defined(__MINGW32__)
        WSACleanup();
#endif /* __MINGW32__ */
        return 1;
    }

    LOGF_VERBOSE(config.logger, "server_run()\n");

    server_run(server);

    LOGF_VERBOSE(config.logger, "server_destroy()\n");

    server_destroy(server);

    logger_destroy(config.logger);

    config_free(&config);

    libevent_global_shutdown();

#if defined(__MINGW32__)
    WSACleanup();
#endif /* __MINGW32__ */

    return 0;
}
