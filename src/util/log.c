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

/* a logger
 *
 * at the moment, loggers hold no state, have no configuration, and do nothing
 * meaningful. we pass them around in case they start doing something.
 *
 * the logger_logf() command and associated macros just call fprintf with
 * stdout (if LOG_VERBOSE or LOG_INFO) or stderr (if LOG_ERROR)
 */
struct logger {
    // empty
};

/* create a logger for/with this config
 *
 * normally this is then put into the config->logger property,
 * but this function cannot assume one of those already exists,
 * so it must not rely on that logger instance for logging
 * (which is fine, it doesn't log anything.)
 */
[[nodiscard]] struct logger * logger_create(
        struct config * config) [[gnu::nonnull(1)]]
{
    /* TODO */
    (void)config;

    struct logger * logger = malloc(sizeof(*logger));
    *logger = (struct logger) { };
    return logger;
}

/* destroy this logger */
void logger_destroy(struct logger * logger) [[gnu::nonnull(1)]]
{
    free(logger);
}

/* log using this logger at this level with this format and args */
void logger_logf(
        struct logger * logger,
        enum log_level level,
        const char * format,
        ...
    ) [[gnu::format(printf, 3, 4), gnu::nonnull(1, 3)]]
{
    /* TODO: use the logger */
    (void)logger;

    va_list args;
    va_start(args, format);

    FILE * f = stderr;

    switch (level) {
        case LOG_VERBOSE:
        case LOG_INFO:
            f = stdout;
            break;

        case LOG_ERROR:
            f = stderr;
            break;

        default:
            fprintf(f, "logger_logf() warning: treating log_level %d as LOG_ERROR\n", level);
            break;
    }

    vfprintf(f, format, args);

    va_end(args);
}
