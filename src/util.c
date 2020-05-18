#include <stdarg.h>
#include <stdio.h>
#define __USE_MISC
#include <syslog.h>

#include "global.h"
#include "util.h"

bool is_file_readable(const char *path)
{
    FILE *f = fopen(path, "r");

    if (f) {
        fclose(f);
    }

    return f != NULL;
}

void mbp_log(int level, const char *fmt, ...)
{
    va_list args;
    if (daemonize) {
        va_start(args, fmt);
        vsyslog(level, fmt, args);
        va_end(args);
    }

    va_start(args, fmt);
    vprintf(fmt, args);
    puts("");  // trailing newline
    va_end(args);
}
