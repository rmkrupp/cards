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

#ifndef VERSION
#error no VERSION defined
#endif /* VERSION */

#ifndef CONFIG_DUMMY_DEFAULT
#define CONFIG_DUMMY_DEFAULT false
#endif /* CONFIG_DUMMY_DEFAULT */

#ifndef CONFIG_PORT_DEFAULT
#define CONFIG_PORT_DEFAULT 10101
#endif /* CONFIG_PORT_DEFAULT */

/* holds values populated by a config_loader */
struct config {
    long port;
    bool dummy;
};

/* free resources used by this config */
void config_free(struct config * config);

/* populate a config from a list of Lua scripts */
int config_load(struct config * config, int nfiles, char ** files);

#endif /* CONFIG_H */
