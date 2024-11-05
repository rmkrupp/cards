/* File: src/script.c
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
#include "script.h"

#include <stdlib.h>

#include "lua.h"

#include "loader.h"
#include "util/log.h"

/* a card script */
struct script {
    lua_State * L;
};

/* destroy this script */
void script_destroy(struct script * script) [[gnu::nonnull(1)]]
{
    lua_close(script->L);
    free(script);
}

/* create a script from this Lua data
 *
 * add its name and the names of any of its abilities to name_set, associating
 * them with this script (thus, it is okay to ignore the return value of this
 * call.)
 *
 * returns NULL if there is an error loading or running the Lua, or if the
 * cards name is not unique. Note that ability names do not need to be unique.
 */
struct script * script_load(
        const char * data,
        size_t length,
        const char * filename,
        struct name_set * name_set,
        struct logger * logger
    ) [[gnu::nonnull(1, 3, 4)]]
{
    lua_State * L = luaL_newstate();
    /* TODO: don't add every library */
    luaL_openlibs(L);

    if (luaL_loadbuffer(L, data, length, filename)) {
        LOGF_ERROR(logger, "lua syntax error %s\n", lua_tostring(L, -1));
        lua_close(L);
        return NULL;
    }
    if (lua_pcall(L, 0, 0, 0)) {
        LOGF_ERROR(logger, "lua error %s\n", lua_tostring(L, -1));
        lua_close(L);
        return NULL;
    }
    lua_pop(L, 1);

    lua_getfield(L, LUA_GLOBALSINDEX, "name");
    if (lua_isnil(L, -1) || lua_type(L, -1) != LUA_TSTRING) {
        LOGF_ERROR(logger, "name field must be string\n");
        lua_close(L);
        return NULL;
    }
    size_t name_length;
    const char * name = lua_tolstring(L, -1, &name_length);

    struct script * script = malloc(sizeof(*script));
    *script = (struct script) {
        .L = L
    };

    lua_pop(L, 1);

    /* TODO: names should have data and type */
    /* TODO: abilities */
    if (!name_set_add(name_set, name, name_length, script)) {
        LOGF_ERROR(logger, "duplicate card name '%s'\n", name);
        lua_close(L);
        free(script);
        return NULL;
    }

    return script;
}
