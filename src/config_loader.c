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

#include "lua.h"
#include "config.h"

/* the type of config option */
enum config_option_type {
    CONFIG_BOOLEAN, /* a bool option */
    CONFIG_INTEGER, /* a long option */
    CONFIG_STRING   /* a (config-managed) char * option */
};

/* one option supported by the loader */
struct config_option {
     /* the option type */
    enum config_option_type type;

    /* the option name, as it will appear in Lua (as config.<name>) */
    char * name;

    /* the values, depending on type */
    bool value_boolean;
    long value_integer;
    char * value_string;

    /* the callback that will run after the option has loaded
     * default: stores the option into a pointer held in context
     */
    int (*callback)(struct config_option *);
    void * context;
};

/* a loader that holds a number of config_options */
struct config_loader {
    struct config_option * options;
    size_t n_options;
};

/* the default callback */
static int default_config_callback(struct config_option * option)
{
    if (!option->context) {
        fprintf(stderr, "[config] warning: option %s with default callback has NULL target\n", option->name);
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

/* returns a new config_loader (i.e. with no options) */
static struct config_loader * config_loader_create()
{
    struct config_loader * loader = malloc(sizeof(*loader));
    *loader = (struct config_loader) { };
    return loader;
}

/* frees the resources of a config loader */
static void config_loader_destroy(struct config_loader * loader)
{
    for (size_t n = 0; n < loader->n_options; n++) {
        free(loader->options[n].name);
        free(loader->options[n].value_string);
    }

    free(loader->options);
    free(loader);
}

/* add a boolean option to the loader
 *
 * if callback is null use default_config_callback which treats
 * context as a pointer to something the size of the value, and
 * stores that value into the pointer when loading is done
 */
static void config_loader_add_option_boolean(
        struct config_loader * loader,
        const char * name, 
        bool default_value,
        int (*callback)(struct config_option *),
        void * context
    )
{
    if (!callback && context) {
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

/* add a integer (long) option to the loader
 *
 * if callback is null use default_config_callback which treats
 * context as a pointer to something the size of the value, and
 * stores that value into the pointer when loading is done
 */
static void config_loader_add_option_integer(
        struct config_loader * loader,
        const char * name, 
        long default_value,
        int (*callback)(struct config_option *),
        void * context
    )
{
    if (!callback && context) {
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

/* add a string option to the loader
 *
 * if callback is null use default_config_callback which treats
 * context as a pointer to something the size of the value, and
 * stores that value into the pointer when loading is done
 */
static void config_loader_add_option_string(
        struct config_loader * loader,
        const char * name, 
        const char * default_value,
        int (*callback)(struct config_option *),
        void * context
    )
{
    if (!callback && context) {
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

/* extract config values from a lua_State, presumably where config scripts
 * have been run that might have changed those values
 *
 * this also runs a check that no config.<whatever> values have been set in
 * the Lua state that don't match any options known to the loader, and that
 * the values match the type expected by the loader.
 *
 * note that Lua only has one number type, so for options of type
 * CONFIG_OPTION_INTEGER we issue a warning if the truncated (i.e. integer)
 * form of the config.<whatever> value doesn't match the value in the Lua
 * state (e.g. because it's 1.1 and getting truncated to just 1)
 */
static int config_loader_update(struct config_loader * loader, lua_State * L)
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

/* populate a config from a list of Lua scripts */
int NONNULL(1) config_load(struct config * config, int nfiles, char ** files)
{
    int err;

    struct config_loader * loader = config_loader_create();

    config_loader_add_option_string(
            loader, "version", VERSION, NULL, NULL);
    config_loader_add_option_integer(
            loader, "port", CONFIG_PORT_DEFAULT, NULL, &config->port);
    config_loader_add_option_boolean(
            loader, "dummy", CONFIG_DUMMY_DEFAULT, NULL, &config->dummy);

    lua_State * L = luaL_newstate();
    if (!L) {
        config_loader_destroy(loader);
        return 1;
    }
    luaL_openlibs(L);

    lua_newtable(L);

    for (size_t n = 0; n < loader->n_options; n++) {
        struct config_option * option = &loader->options[n];

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

        lua_setfield(L, -2, option->name);
    }

    lua_setfield(L, LUA_GLOBALSINDEX, "config");

    for (int i = 0; i < nfiles; i++) {
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
            config_loader_destroy(loader);
            return 1;
        }

        if (lua_pcall(L, 0, 0, 0)) {
            fprintf(stderr, "[config] Lua error in %s: %s\n",
                    files[i], lua_tostring(L, -1));
            lua_close(L);
            config_loader_destroy(loader);
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
