/* File: include/constants.h
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
#ifndef CONSTANTS_H
#define CONSTANTS_H

/* the maximum size of any card script blob
 *
 * enforced by the cards_compile tool and bundle_load()
 *
 * checked by the cards_inspect tool
 *
 * this is not a implementation-derived limit. the underlying maximum is
 * limited only by whatever SQLITE_MAX_LENGTH our sqlite was compiled with
 * (which defaults to 1 billion bytes), any limit Lua may have when loading
 * scripts, and the memory available to the program.
 */
constexpr size_t card_script_size_max = 16 * 1024;

#endif /* CONSTANTS_H */
