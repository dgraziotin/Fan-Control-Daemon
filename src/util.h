#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdbool.h>

bool is_file_readable(const char *path);
void mbp_log(int level, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));

#endif
