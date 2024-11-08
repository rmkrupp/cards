/* File: src/card.c
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
#include "card.h"

#include <stdlib.h>

#include "lua.h"

#include "loader.h"
#include "util/log.h"

#include <unistr.h>
#include <unitypes.h>
#include <unicase.h>
#include <uninorm.h>

/* a card script */
struct card {
    struct ability ** abilities;
    size_t n_abilities;
    lua_State * L;
};

/* a card ability */
struct ability {
    struct card ** owners;
    size_t n_owners;
};

struct ability_candidate {
    char * name;
    struct ability * ability;
};

/* destroy this script */
void card_destroy(struct card * card) [[gnu::nonnull(1)]]
{
    lua_close(card->L);
    free(card->abilities);
    free(card);
}

/* destroy this ability */
void ability_destroy(struct ability * ability) [[gnu::nonnull(1)]]
{
    free(ability->owners);
    free(ability);
}

/* create a card from this Lua data
 *
 * add its name and the names of any of its abilities to name_set, associating
 * them with this card (thus, it is okay to ignore the return value of this
 * call.)
 *
 * returns NULL if there is an error loading or running the Lua, or if the
 * card's name is not unique, or if the name field of the card or its abilities
 * are absent or not string, or if the indices of abilities tables are not
 * numbers. Note that ability names do not need to be unique.
 */
/* TODO: tidy variable names (name, ability_name, key, etc.) */
struct card * card_load(
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

    lua_getfield(L, LUA_GLOBALSINDEX, "name");
    if (lua_isnil(L, -1) || lua_type(L, -1) != LUA_TSTRING) {
        LOGF_ERROR(logger, "%s: name field must be a string\n", filename);
        lua_close(L);
        return NULL;
    }

    size_t name_length;
    /* TODO: triple check this cast is fine */
    const uint8_t * name = (const uint8_t *)lua_tolstring(L, -1, &name_length);

    struct card * card = malloc(sizeof(*card));

    *card = (struct card) {
        .L = L
    };

    lua_pop(L, 1);

    if (!name_set_add(name_set, name, name_length, card, NAME_TYPE_CARD)) {
        LOGF_ERROR(logger, "duplicate card name '%.*U'\n", name_length, name);
        lua_close(L);
        free(card);
        return NULL;
    }

    lua_getfield(L, LUA_GLOBALSINDEX, "abilities");

    if (lua_isnil(L, -1)) {
        LOGF_INFO(logger, "%.*s: no attributes", name_length, name);
        return card;
    }

    if (lua_type(L, -1) != LUA_TTABLE) {
        LOGF_ERROR(
                logger,
                "%.*s: attributes field must be a table\n",
                name_length,
                name
            );
        /* TODO: handle this remove */
        //name_set_remove(name_set, name, name_length);
        free(card);
        return NULL;
    }

    struct ability ** abilities = NULL;
    size_t n_abilities = 0;

    lua_pushnil(L);
    while (lua_next(L, -2) != 0) {
        if (lua_type(L, -2) != LUA_TNUMBER) {
            LOGF_ERROR(
                    logger,
                    "%.*s: abilities must be indexed by number\n",
                    name_length,
                    name
                );
            lua_pop(L, 1);
            continue;
        }
        if (lua_type(L, -1) != LUA_TTABLE) {
            LOGF_ERROR(
                    logger,
                    "%.*s: each ability must be a table\n",
                    name_length,
                    name
                );
            lua_pop(L, 1);
            continue;
        }
        lua_getfield(L, -1, "name");
        if (lua_isnil(L, -1) || lua_type(L, -1) != LUA_TSTRING) {
            LOGF_ERROR(
                    logger,
                    "%.*s: abilities must have a string-type name field\n",
                    name_length,
                    name
                );
            lua_pop(L, 2);
            continue;
        }
        size_t length;
        const char * key = lua_tolstring(L, -1, &length);
        const struct name * ability_name =
            name_set_lookup(name_set, (const uint8_t *)key, length);
        if (ability_name && ability_name->type == NAME_TYPE_CARD) {
            LOGF_ERROR(
                    logger,
                    "%.*s: ability name %.*s conflicts with card name\n",
                    name_length,
                    name,
                    length,
                    key
                );
            lua_pop(L, 2);
            continue;
        }
        
        abilities = realloc(abilities, sizeof(*abilities) * (n_abilities + 1));
        struct ability * ability;

        if (ability_name) {
            LOGF_INFO(
                    logger,
                    "%.*s: ability %.*s already exists\n",
                    name_length,
                    name,
                    length,
                    key
                );
            ability = (struct ability *)ability_name->data;
        } else {
            ability = malloc(sizeof(*abilities[n_abilities]));
            *ability = (struct ability) { };
        }

        /* TODO */
        // this doesn't work, what if you have to get rid of it...
        // --> add all to array and only name_set_add when owners == 1
        // ...what if a card has an ability more than once?
        // (is that an error? if so, we should check for it...)
        ability->owners = realloc(
                ability->owners,
                sizeof(*ability->owners) * (ability->n_owners + 1)
            );
        ability->owners[ability->n_owners] = card;

        if (!ability_name) {
            abilities[n_abilities] = ability;
            n_abilities++;
        }

        lua_pop(L, 2);
    }

    /* TODO */
    /*
    for (size_t i = 0; i < n_abilities; i++) {
        struct ability * ability = abilities[i];
        if (!name_set_add(name_set, key, length, ability, NAME_TYPE_ABILITY)) {
            LOGF_ERROR(
                    logger,
                    "%s: error adding ability %s\n",
                    name,
                    key
                    );
            free(ability);
        }
        lua_pop(L, 2);
    }
    */

    lua_pop(L, 1);

    return card;
}
