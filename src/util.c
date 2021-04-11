#include <stdarg.h>
#include <stdio.h>
#define __USE_MISC
#include <syslog.h>

#include "global.h"

void mbp_log(int level, const char *fmt, ...) {
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
