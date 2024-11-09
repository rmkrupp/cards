/* File: include/bundle.h
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
#ifndef BUNDLE_H
#define BUNDLE_H

#include <stddef.h>

/* forward declare */
struct name_set;
struct logger;

/* the result of loading a bundle
 *
 * see bundle_load()
 */
enum bundle_load_result {
    BUNDLE_LOAD_RESULT_OKAY,
    BUNDLE_LOAD_RESULT_ERROR_NONE,
    BUNDLE_LOAD_RESULT_ERROR_SOME
};

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
    ) [[gnu::nonnull(1, 2)]];

#endif /* BUNDLE_H */
