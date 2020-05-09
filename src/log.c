/*
 * Copyright (c) 2017 rxi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "log.h"

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_INFO
#endif

#define DATEFORMAT_SIZE 32

static struct {
    void *     udata;
    log_LockFn lock;
    FILE *     fp;
    char       dateformat[32];
    int        level;

    struct {
        unsigned int quiet : 1;
        unsigned int fileinfo : 1;
    } config;
} L = {.fp = NULL, .level = LOG_LEVEL, .dateformat = "%d-%m-%Y %H:%M:%S", .udata = NULL, .lock = NULL};


static const char *level_names[] = {"TRACE", "DEBUG", "INFO", "WARN", "ERROR", "FATAL"};

#ifdef LOG_USE_COLOR
static const char *level_colors[] = {"\x1b[94m", "\x1b[36m", "\x1b[32m", "\x1b[33m", "\x1b[31m", "\x1b[35m"};
#endif


static void lock(void) {
    if (L.lock) {
        L.lock(L.udata, 1);
    }
}


static void unlock(void) {
    if (L.lock) {
        L.lock(L.udata, 0);
    }
}

static void _log(FILE *dest, char *date, int level, const char *file, int line) {
    if (L.config.fileinfo)
        fprintf(dest, "%s %-5s %s:%d: ", date, level_names[level], file, line);
    else
        fprintf(dest, "%s %-5s: ", date, level_names[level]);
}

void log_set_udata(void *udata) {
    L.udata = udata;
}

void log_set_lock(log_LockFn fn) {
    L.lock = fn;
}


void log_set_fp(FILE *fp) {
    L.fp = fp;
}


void log_set_level(int level) {
    L.level = level;
}


void log_set_quiet(int enable) {
    L.config.quiet = enable ? 1 : 0;
}

void log_set_fileinfo(int enable) {
    L.config.fileinfo = enable ? 1 : 0;
}

int log_set_dateformat(char *fmt) {
    if (strlen(fmt) >= DATEFORMAT_SIZE)
        return -1;
    else
        strcpy(L.dateformat, fmt);

    return 0;
}

void log_log(int level, const char *file, int line, const char *fmt, ...) {
    if (level < L.level) {
        return;
    }

    /* Acquire lock */
    lock();

    /* Get current time */
    time_t     t  = time(NULL);
    struct tm *lt = localtime(&t);

    /* Log to stderr */
    if (!L.config.quiet) {
        va_list args;
        char    buf[16];
        buf[strftime(buf, sizeof(buf), L.dateformat, lt)] = '\0';
#ifdef LOG_USE_COLOR
        fprintf(stderr, "%s\t%s%-5s\t\x1b[0m \x1b[90m%s:%d:\x1b[0m\t", buf, level_colors[level], level_names[level],
                file, line);
#else
        fprintf(stderr, "%s\t%-5s\t%s:%d:\t", buf, level_names[level], file, line);
#endif
        va_start(args, fmt);
        vfprintf(stderr, fmt, args);
        va_end(args);
        fprintf(stderr, "\n");
        fflush(stderr);
    }

    /* Log to file */
    if (L.fp) {
        va_list args;
        char    buf[32];
        buf[strftime(buf, sizeof(buf), L.dateformat, lt)] = '\0';
        _log(L.fp, buf, level, file, line);
        va_start(args, fmt);
        vfprintf(L.fp, fmt, args);
        va_end(args);
        fprintf(L.fp, "\n");
        fflush(L.fp);
    }

    /* Release lock */
    unlock();
}
