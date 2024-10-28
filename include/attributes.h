/* File: include/attributes.h
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
#ifndef ATTRIBUTES_H
#define ATTRIBUTES_H

/* TODO: windows defines a PURE macro so figure out if there are better names
 *       than sticking ATTR_ in front of our PURE... (which is also not
 *       currently used in any places.)
 *
 *       it also defines a CONST (as just the CONST keyword, so similar
 *       thoughts there.)
 */

#if defined(NO_COMMON_ATTRIBUTES) && NO_COMMON_ATTRIBUTES

#define ATTR_CONST
#define FORMAT(type, string_index, first_to_check)
#define FORMAT_ARG(string_index)
#define NONNULL(...)
#define ATTR_PURE
#define WARN_UNUSED_RESULT

#else

#define ATTR_CONST __attribute__ (( const ))
#define FORMAT(type, string_index, first_to_check) \
    __attribute__ (( format(type, string_index, first_to_check) ))
#define FORMAT_ARG(string_index) \
    __attribute__ (( format_arg(string_index) ))
#define NONNULL(...) __attribute__(( nonnull(__VA_ARGS__) ))
#define ATTR_PURE __attribute__(( pure ))
#define WARN_UNUSED_RESULT __attribute__(( warn_unused_result ))

#endif /* NO_COMMON_ATTRIBUTES */

#endif /* ATTRIBUTES_H */
