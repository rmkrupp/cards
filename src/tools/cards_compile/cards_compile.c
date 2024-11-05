/* File: src/tools/cards_compile/cards_compile.c
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

#include "tools/cards_compile/args.h"

#include "constants.h"

static void free_args(struct arguments * args)
{
    for (size_t i = 0; i < args->n_filenames; i++) {
        free(args->filenames[i]);
    }
    free(args->filenames);
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

    char * errmsg = NULL;
    sqlite3 * db;
    if (sqlite3_open(args.database_name, &db)) {
        fprintf(
                stderr,
                "error opening database \"%s\": %s\n",
                args.database_name,
                sqlite3_errmsg(db)
            );
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    if (!args.append) {
        if (sqlite3_exec(
                    db,
                    "DROP TABLE IF EXISTS cards",
                    NULL,
                    NULL,
                    &errmsg
                )) {
            fprintf(stderr, "error dropping table: %s\n", errmsg);
            sqlite3_free(errmsg);
            sqlite3_close(db);
            free_args(&args);
            return 1;
        }
    }

    if (sqlite3_exec(
                db,
                "CREATE TABLE IF NOT EXISTS cards (filename, script)",
                NULL,
                NULL,
                &errmsg
            )) {
        fprintf(stderr, "error creating table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        free_args(&args);
        return 1;
    }

    const char statement[] =
        "INSERT INTO cards (filename, script) VALUES (?, ?)";
    sqlite3_stmt * stmt = NULL;
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
    size_t okay = 0;
    for (size_t i = 0; i < args.n_filenames; i++) {
        char * filename = args.filenames[i];
        FILE * f = fopen(filename, "r");

        if (!f) {
            fprintf(
                    stderr,
                    "error opening \"%s\": %s\n",
                    filename,
                    strerror(ferror(f))
                );
            errors++;
            continue;
        }

        if (fseek(f, 0, SEEK_END)) {
            fprintf(
                    stderr,
                    "error fseeking \"%s\": %s\n",
                    filename,
                    strerror(ferror(f))
                );
            fclose(f);
            errors++;
            continue;
        }

        long tell_size = ftell(f);
        printf("tell_size = %ld\n", tell_size);

        if (tell_size < 0) {
            fprintf(
                    stderr,
                    "error ftelling \"%s\": %s\n",
                    filename,
                    strerror(ferror(f))
                );
            fclose(f);
            errors++;
            continue;
        }

        if (fseek(f, 0, SEEK_SET)) {
            fprintf(
                    stderr,
                    "error fseeking \"%s\": %s\n",
                    filename,
                    strerror(ferror(f))
                );
            fclose(f);
            errors++;
            continue;
        }

        static_assert(sizeof(size_t) >= sizeof(tell_size));
        size_t size = (size_t)tell_size;

        if (size >= card_script_size_max) {
            fprintf(
                    stderr,
                    "error with \"%s\": size %ld is greater than maximum of %lu\n",
                    filename,
                    (unsigned long)size,
                    (unsigned long)card_script_size_max
                );
            fclose(f);
            errors++;
            continue;
        }

        char * buffer = malloc(size + 1);

        if (fread(buffer, size, 1, f) != 1) {
            fprintf(
                    stderr,
                    "error reading \"%s\": %s\n",
                    filename,
                    strerror(ferror(f))
                );
            fclose(f);
            free(buffer);
            errors++;
            continue;
        }

        buffer[size] = '\0';

        fclose(f);

        if (sqlite3_reset(stmt)) {
            fprintf(
                    stderr,
                    "error resetting statement (%s): %s\n",
                    filename,
                    sqlite3_errmsg(db)
               );
            free(buffer);
            errors++;
            continue;
        }

        if (sqlite3_bind_text(stmt, 1, filename, strlen(filename), NULL)) {
            fprintf(
                    stderr,
                    "error binding statement (%s): %s\n",
                    filename,
                    sqlite3_errmsg(db)
               );
            free(buffer);
            errors++;
            continue;
        }

        if (sqlite3_bind_blob(stmt, 2, buffer, size, NULL)) {
            fprintf(
                    stderr,
                    "error binding statement (%s): %s\n",
                    filename,
                    sqlite3_errmsg(db)
               );
            free(buffer);
            errors++;
            continue;
        }

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            fprintf(
                    stderr,
                    "error stepping statement (%s): %s\n",
                    filename,
                    sqlite3_errmsg(db)
                );
            free(buffer);
            errors++;
            continue;
        }

        free(buffer);
        okay++;
    }

    if (errors) {
        fprintf(stderr, "%lu errors ocurred\n", (unsigned long)errors);
    }
    printf("%lu lines added\n", (unsigned long)okay);

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    free_args(&args);
    return errors > 0 ? 1 : 0;
}

