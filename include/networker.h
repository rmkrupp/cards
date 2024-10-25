/* File: include/networker.h
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
#ifndef NETWORKER_H
#define NETWORKER_H

#include "game.h"
#include "config.h"

/* returns a new networker based on config and holding game */
struct networker * networker_create(
        struct config * config);

/* destroy this networker */
void networker_destroy(struct networker * networker);

/* begin this networker's event loop
 * returns 0 if exited without error, number of errors otherwise
 */
int networker_run(struct networker * networker);

/* returns a new iterator over the networker's connections */
struct networker_connection_iter * networker_connection_iter_create(
        struct networker * networker);

/* destroys this iterator */
void networker_connection_iter_destroy(
        struct networker_connection_iter * iter);

/* returns the next connection and advances the iterator */
struct connection * networker_connection_iter_iterate(
        struct networker_connection_iter * iter
    );

#endif /* NETWORKER_H */
