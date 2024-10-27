/* File: include/util/log.h
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
#ifndef LOG_H
#define LOG_H

#include "config.h"

enum log_level {
    LOG_VERBOSE,
    LOG_INFO,
    LOG_ERROR
};

struct logger * logger_create(struct config * config);
void logger_destroy(struct logger * logger);

void logger_logf(
        struct logger * logger,
        enum log_level level,
        const char * format,
        ...
    );

#define LOGF_VERBOSE(logger, format, ...) logger_logf(logger, LOG_VERBOSE, format __VA_OPT__(,) __VA_ARGS__)
#define LOGF_INFO(logger, format, ...) logger_logf(logger, LOG_INFO, format __VA_OPT__(,) __VA_ARGS__)
#define LOGF_ERROR(logger, format, ...) logger_logf(logger, LOG_ERROR, format __VA_OPT__(,)  __VA_ARGS__)

#endif /* LOG_H */
