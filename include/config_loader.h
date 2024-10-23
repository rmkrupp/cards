/* File: include/config_loader.h
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
#ifndef CONFIG_LOADER_H
#define CONFIG_LOADER_H

#include "config.h"

enum config_option_type {
    CONFIG_BOOLEAN,
    CONFIG_INTEGER,
    CONFIG_STRING
};

struct config_option {
    enum config_option_type type;
    char * name;
    bool value_boolean;
    long value_integer;
    char * value_string;
    int (*callback)(struct config_option *);
    void * context;
};

struct config_loader {
    struct config_option * options;
    size_t n_options;
};

struct config_loader * config_loader_create();

void config_loader_destroy(struct config_loader * loader);

void config_loader_add_option_boolean(
        struct config_loader * loader,
        const char * name, 
        bool default_value,
        int (*callback)(struct config_option *),
        void * context
    );

void config_loader_add_option_integer(
        struct config_loader * loader,
        const char * name, 
        long default_value,
        int (*callback)(struct config_option *),
        void * context
    );

void config_loader_add_option_string(
        struct config_loader * loader,
        const char * name, 
        const char * default_value,
        int (*callback)(struct config_option *),
        void * context
    );

int config_loader_update(
        struct config_loader * loader,
        lua_State * L
    );

int config_load(struct config * config, int nfiles, char ** files);

#endif /* CONFIG_LOADER_H */
