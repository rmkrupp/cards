/* File: src/bundle.c
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
#include "bundle.h"

#include "card.h"
#include "constants.h"
#include "name_set.h"
#include "util/log.h"

#include <sqlite3.h>

/* load the bundle with this filename, adding any new names to this name set
 *
 * if n_errors_out is non-null, it is filled with the number of cards that
 * couldn't be loaded
 *
 * returns BUNDLE_LOAD_RESULT_OKAY if the bundle was loaded successfully and
 * for every card loaded, the card was either added to the set or n_errors_out
 * (if non-NULL) was incremented
 *
 * returns BUNDLE_LOAD_RESULT_ERROR_NONE if the bundle could not be opened or
 * the data was missing and no loading was attempted. in this case n_errors_out
 * is not modified.
 *
 * returns BUNDLE_LOAD_RESULT_ERROR_SOME if there is an error with the bundle
 * but one or more cards may have been loaded (or errors recorded.) in this
 * case n_errors_out is modified.
 */
enum bundle_load_result bundle_load(
        const char * bundle_name,
        struct name_set * name_set,
        size_t * n_errors_out,
        struct logger * logger
    ) [[gnu::nonnull(1, 2)]]
{
    size_t errors = 0;

    sqlite3 * db;
    if (sqlite3_open_v2(bundle_name, &db, SQLITE_OPEN_READONLY, NULL)) {
        LOGF_ERROR(logger, "error opening bundle: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return BUNDLE_LOAD_RESULT_ERROR_NONE;
    }

    const char statement[] =
        "SELECT filename, script FROM cards";
    sqlite3_stmt * stmt;
    if (sqlite3_prepare_v2(db, statement, sizeof(statement), &stmt, NULL)) {
        LOGF_ERROR(logger,
                "error preparing statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return BUNDLE_LOAD_RESULT_ERROR_NONE;
    }

    int result;
    while ((result = sqlite3_step(stmt)) == SQLITE_ROW) {
        const char * filename = (const char *)sqlite3_column_text(stmt, 0);
        const void * data = sqlite3_column_blob(stmt, 1);
        int size = sqlite3_column_bytes(stmt, 1);

        if (size >= 0 && (size_t)size > card_script_size_max) {
            LOGF_ERROR(
                    logger,
                    "error loading %s from bundle %s: blob exceeeds maximum card script size.\n",
                    filename,
                    bundle_name
                );
            errors++;
            continue;
        }

        if (!card_load(data, size, filename, name_set, logger)) {
            errors++;
        }
    }

    if (result != SQLITE_DONE) {
        LOGF_ERROR(logger,
                "error stepping statement: %s\n", sqlite3_errmsg(db));
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        if (n_errors_out) {
            *n_errors_out = errors;
        }
        return BUNDLE_LOAD_RESULT_ERROR_SOME;
    }

    sqlite3_finalize(stmt);
    sqlite3_close(db);
    if (n_errors_out) {
        *n_errors_out = errors;
    }
    return BUNDLE_LOAD_RESULT_OKAY;
}
