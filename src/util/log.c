/* File: src/util/log.c
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
#include "util/log.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "config.h"

struct logger {
    // empty
};

struct logger * logger_create(struct config * config)
{
    struct logger * logger = malloc(sizeof(*logger));
    *logger = (struct logger) { };
    return logger;
}

void logger_destroy(struct logger * logger)
{
    free(logger);
}

void logger_logf(
        struct logger * logger,
        enum log_level level,
        const char * format,
        ...
    )
{
    va_list args;
    va_start(args, format);

    FILE * f;

    switch (level) {
        case LOG_VERBOSE:
        case LOG_INFO:
            f = stdout;
            break;

        case LOG_ERROR:
            f = stderr;
            break;
    }

    vfprintf(f, format, args);

    va_end(args);
}
