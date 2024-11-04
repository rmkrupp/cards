/* File: src/tools/cards_inspect/cards_inspect.c
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
#include <errno.h>
#include <string.h>
#include <sqlite3.h>

#include "lua.h"

#include "tools/cards_inspect/args.h"

static void free_args(struct arguments * args)
{
    free(args->database_name);
}

int main(int argc, char ** argv)
{
    struct arguments args = (struct arguments) { };

    int parse_result;
    if ((parse_result = parse_args(&args, argc, argv))) {
        free_args(&args);
        return parse_result;
    }

    sqlite3 * db;
    if (sqlite3_open_v2(args.database_name, &db, SQLITE_OPEN_READONLY, NULL)) {
        fprintf(stderr, "error opening database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    const char statement[] =
        "SELECT filename, script FROM cards";
    sqlite3_stmt * stmt;
    if (sqlite3_prepare_v2(db, statement, sizeof(statement), &stmt, NULL)) {
        fprintf(
                stderr,
                "error preparing statement: %s\n",
                sqlite3_errmsg(db)
            );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    size_t errors = 0;

    int result;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char * filename = sqlite3_column_text(stmt, 0);
        const void * script = sqlite3_column_blob(stmt, 1);
        int size = sqlite3_column_bytes(stmt, 1);

        if (args.validate) {
            lua_State * L = luaL_newstate();

            if (luaL_loadbuffer(L, script, size, (const char *)filename)) {
                /* no need to say filename, lua will include it (and line) for
                 * us because it's passed to luaL_loadbuffer
                 */
                fprintf(
                        stderr,
                        "lua error: %s\n",
                        lua_tostring(L, -1)
                    );
                errors++;
            }

            lua_close(L);
        } else {
            printf("%s\n", filename);
        }
    }

    if (result != SQLITE_DONE) {
        fprintf(
                stderr,
                "error stepping statement: %s\n",
                sqlite3_errmsg(db)
           );
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    if (args.validate) {
        printf("%lu errors ocurred\n", (unsigned long)errors);
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    free_args(&args);
    return errors > 0 ? 1 : 0;
}

