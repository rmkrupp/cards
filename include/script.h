/* File: include/script.h
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
#ifndef SCRIPT_H
#define SCRIPT_H

#include "util/log.h"
#include "loader.h"
#include <stddef.h>

/* a card script */
struct script;

/* destroy this script */
void script_destroy(struct script * script) [[gnu::nonnull(1)]];

/* create a script from this Lua data
 *
 * add its name and the names of any of its abilities to name_set
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
    ) [[gnu::nonnull(1, 3, 4)]];

#endif /* SCRIPT_H */
