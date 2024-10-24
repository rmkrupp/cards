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
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#if defined(USE_LUAJIT) && USE_LUAJIT
#include <luajit-2.1/lua.h>
#include <luajit-2.1/lualib.h>
#include <luajit-2.1/lauxlib.h>
#else
#include <lua5.1/lua.h>
#include <lua5.1/lualib.h>
#include <lua5.1/lauxlib.h>
#endif /* USE_LUAJIT */

#include "config_loader.h"
#include "config.h"

int main(int argc, char ** argv) {

    struct config config = (struct config) { };

    if (config_load(&config, argc, argv)) {
        if (config.s) free(config.s);
        return 1;
    }

    printf("port = %ld\n", config.port);
    printf("port2 = %ld\n", config.port2);
    printf("s = %s\n", config.s);
    printf("verbose = %d\n", config.verbose);

    free(config.s);
    free(config.build);

    return 0;
}
