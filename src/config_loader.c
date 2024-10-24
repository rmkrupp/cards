/* File: src/config_loader.c
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

static int default_config_callback(struct config_option * option)
{
    if (!option->context) {
        printf("[config] warning: option %s with default callback has NULL target\n", option->name);
        return 0;
    }

    if (option->type == CONFIG_INTEGER) {
        *(long *)(option->context) = option->value_integer;
    }

    if (option->type == CONFIG_BOOLEAN) {
        *(bool *)(option->context) = option->value_boolean;
    }

    if (option->type == CONFIG_STRING) {
        *(char **)(option->context) = strdup(option->value_string);
    }

    return 0;
}

struct config_loader * config_loader_create()
{
    struct config_loader * loader = malloc(sizeof(*loader));
    *loader = (struct config_loader) { };
    return loader;
}

void config_loader_destroy(struct config_loader * loader)
{
    for (size_t n = 0; n < loader->n_options; n++) {
        free(loader->options[n].name);
        free(loader->options[n].value_string);
    }

    free(loader->options);
    free(loader);
}

void config_loader_add_option_boolean(
        struct config_loader * loader,
        const char * name, 
        bool default_value,
        int (*callback)(struct config_option *),
        void * context
    )
{
    if (!callback) {
        callback = &default_config_callback;
    }

    loader->options = realloc(loader->options,
            sizeof(*loader->options) * (loader->n_options + 1));

    loader->options[loader->n_options] = (struct config_option) {
        .type = CONFIG_BOOLEAN,
        .name = strdup(name),
        .value_boolean = default_value,
        .callback = callback,
        .context = context
    };

    loader->n_options++;
}

void config_loader_add_option_integer(
        struct config_loader * loader,
        const char * name, 
        long default_value,
        int (*callback)(struct config_option *),
        void * context
    )
{
    if (!callback) {
        callback = &default_config_callback;
    }

    loader->options = realloc(loader->options,
            sizeof(*loader->options) * (loader->n_options + 1));

    loader->options[loader->n_options] = (struct config_option) {
        .type = CONFIG_INTEGER,
        .name = strdup(name),
        .value_integer = default_value,
        .callback = callback,
        .context = context
    };

    loader->n_options++;
}

void config_loader_add_option_string(
        struct config_loader * loader,
        const char * name, 
        const char * default_value,
        int (*callback)(struct config_option *),
        void * context
    )
{
    if (!callback) {
        callback = &default_config_callback;
    }

    loader->options = realloc(loader->options,
            sizeof(*loader->options) * (loader->n_options + 1));

    loader->options[loader->n_options] = (struct config_option) {
        .type = CONFIG_STRING,
        .name = strdup(name),
        .value_string = strdup(default_value),
        .callback = callback,
        .context = context
    };

    loader->n_options++;
}

int config_loader_update(
        struct config_loader * loader,
        lua_State * L
    )
{
    lua_getfield(L, LUA_GLOBALSINDEX, "config");

    if (lua_isnil(L, -1)) {
        fprintf(stderr, "[config] no \"config\" table\n");
        return 1;
    }

    int error = 0;

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        // key -2 value -1
        const char * k = lua_tostring(L, -2);

        struct config_option * option = NULL;
        for (size_t n = 0; n < loader->n_options; n++) {
            if (strcmp(k, loader->options[n].name) == 0) {
                option = &loader->options[n];
                break;
            }
        }

        if (option == NULL) {
            fprintf(stderr, "[config] config.%s matches no options\n", k);
            error++;
            lua_pop(L, 1);
            continue;
        }

        switch (option->type) {
            case CONFIG_BOOLEAN:
                if (lua_type(L, -1) != LUA_TBOOLEAN) {
                    fprintf(stderr, "[config] config.%s must be of type boolean, not %s\n", k, lua_typename(L, lua_type(L, -1)));
                    error++;
                    lua_pop(L, 1);
                    continue;
                }

                option->value_boolean = lua_toboolean(L, -1);
                break;

            case CONFIG_INTEGER:
                if (lua_type(L, -1) != LUA_TNUMBER) {
                    fprintf(stderr, "[config] config.%s must be of type integer, not %s\n", k, lua_typename(L, lua_type(L, -1)));
                    error++;
                    lua_pop(L, 1);
                    continue;
                }

                lua_Number num = lua_tonumber(L, -1);

                if (num != (lua_Number)(long)num) {
                    fprintf(stderr, "[config] warning: config.%s will truncate from %g to %ld\n", k, num, (long)num);
                }

                option->value_integer = num;
                break;

            case CONFIG_STRING:
                if (lua_type(L, -1) != LUA_TSTRING) {
                    fprintf(stderr, "[config] config.%s must be of type string, not %s\n", k, lua_typename(L, lua_type(L, -1)));
                    error++;
                    lua_pop(L, 1);
                    continue;
                }

                free(option->value_string);
                option->value_string = strdup(lua_tostring(L, -1));
                break;
        }

        lua_pop(L, 1);
    }

    lua_pop(L, 1); // the config table

    return error;
}

int config_load(struct config * config, int nfiles, char ** files)
{
    int err;

    struct config_loader * loader = config_loader_create();

    config_loader_add_option_integer(
            loader, "port", 10101, NULL, &config->port);
    config_loader_add_option_integer(
            loader, "port2", 3, NULL, &config->port2);
    config_loader_add_option_string(
            loader, "s", "foo", NULL, &config->s);
    config_loader_add_option_boolean(
            loader, "verbose", false, NULL, &config->verbose);
    config_loader_add_option_string(
            loader, "build", BUILDTYPE, NULL, &config->build);

    lua_State * L = luaL_newstate();
    if (!L) return 1;
    luaL_openlibs(L);

    lua_newtable(L);

    for (size_t n = 0; n < loader->n_options; n++) {
        struct config_option * option = &loader->options[n];

        lua_pushstring(L, option->name);

        switch (option->type) {
            case CONFIG_BOOLEAN:
                lua_pushboolean(L, option->value_boolean);
                break;
            case CONFIG_INTEGER:
                lua_pushinteger(L, option->value_integer);
                break;
            case CONFIG_STRING:
                lua_pushstring(L, option->value_string);
                break;
        }

        lua_settable(L, -3);
    }

    lua_setfield(L, LUA_GLOBALSINDEX, "config");

    for (int i = 1; i < nfiles; i++) {
        err = luaL_loadfile(L, files[i]);

        if (err == LUA_ERRFILE) {
            fprintf(stderr, "[config] LUA_ERRFILE opening %s: %s\n",
                    files[i], lua_tostring(L, -1));
        } else if (err == LUA_ERRSYNTAX) {
            fprintf(stderr, "[config] LUA_ERRSYNTAX opening %s: %s\n",
                    files[i], lua_tostring(L, -1));
        } else if (err == LUA_ERRMEM) {
            fprintf(stderr, "[config] LUA_ERRMEM opening %s: %s\n",
                    files[i], lua_tostring(L, -1));
        }

        if (err) {
            lua_close(L);
            return 1;
        }

        if (lua_pcall(L, 0, 0, 0)) {
            fprintf(stderr, "[config] Lua error in %s: %s\n",
                    files[i], lua_tostring(L, -1));
            lua_close(L);
            return 1;
        }
    }

    int update_errors = config_loader_update(loader, L);

    if (update_errors) {
        fprintf(stderr, "[config] errors occured updating configuration\n");
        lua_close(L);
        config_loader_destroy(loader);
        return 1;

    }

    printf("[config] loaded okay\n");

    for (size_t n = 0; n < loader->n_options; n++) {
        struct config_option * option = &loader->options[n];
        if (option->callback) {
            if (option->callback(option)) {
                lua_close(L);
                config_loader_destroy(loader);
                return 1;
            }
        }
    }

    lua_close(L);
    config_loader_destroy(loader);

    return 0;
}
