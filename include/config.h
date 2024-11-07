/* File: include/config.h
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
#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

/* holds values populated by config_load
 *
 * options must be one of:
 *  - char *
 *  - long
 *  - bool
 * because they are cast to a void pointer and then a pointer to that type
 * before they are writen to
 */
struct config {
    struct logger * logger;
    long port;
    char * default_card_db;
    bool dummy;
};

/* free resources used by this config */
void config_free(struct config * config) [[gnu::nonnull(1)]];

/* populate a config from a list of Lua scripts */
int config_load(
        struct config * config, int nfiles, char ** files) [[gnu::nonnull(1)]];

#endif /* CONFIG_H */
