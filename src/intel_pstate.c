/**
 *  intel_pstate.c - automatically control turbo frequency for MacBook Pro
 *  Copyright (C) 2021 Gokturk Yuksek <gokturk@gentoo.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>

#include "intel_pstate.h"
#include "util.h"
#include <syslog.h>

static int read_int(FILE *fp, int *val)
{
    char buf[4]; /* Maximum we can read in this module is '100\0' */
    ssize_t ret;

    ret = pread(fileno(fp), buf, sizeof(buf), 0);
    if (ret >= 0) {
        buf[ret] = '\0';
        *val = atoi(buf);
        return 0;
    } else {
        return (int)ret;
    }
}

static inline int write_str(FILE *fp, char *str, int len)
{
    ssize_t ret;
    ret = (int)pwrite(fileno(fp), str, len, 0);
    if (ret == len)
        return 0;
    else
        return -1;
}

static int write_int(FILE *fp, int val)
{
    char buf[4]; /* Maximum we can read in this module is '100\0' */
    int ret;

    ret = snprintf(buf, sizeof(buf), "%d", val);
    if (ret < 0)
        return ret;

    return write_str(fp, buf, ret);
}

int intel_pstate_init(t_intel_pstate *intel_pstate)
{
    const char *path_max_perf_pct = INTEL_PT_PATH "/max_perf_pct";
    const char *path_min_perf_pct = INTEL_PT_PATH "/min_perf_pct";
    FILE *fp;
    int err;

    fp = fopen(path_max_perf_pct, "w+");
    if (!fp)
        return -1;
    /* Save the initial value of max_perf_pct, to restore it on exit */
    err = read_int(fp, &intel_pstate->preserved_max_perf_pct);
    if (err)
        return err;
    /* Start with the maximum performance percentage at 100% */
    err = write_str(fp, "100", 4);
    if (err)
        return err;
    intel_pstate->f_max_perf_pct = fp;

    fp = fopen(path_min_perf_pct, "w+");
    if (!fp)
        return -1;
    /* Save the initial value of min_perf_pct, to restore it on exit */
    err = read_int(fp, &intel_pstate->preserved_min_perf_pct);
    if (err)
        return err;
    /* Set the minimum performance percentage to 0 */
    err = write_str(fp, "0", 2);
    if (err)
        return err;
    intel_pstate->f_min_perf_pct = fp;

    return 0;
}

int intel_pstate_is_available(void)
{
    DIR* dir = opendir(INTEL_PT_PATH);

    if ((!dir) && ENOENT == errno)
        return 0;

    closedir(dir);
    return 1;
}

int intel_pstate_adjust(t_intel_pstate *intel_pstate, int step)
{
    int val;
    int err;

    if (!intel_pstate)
        return 1;

    err = read_int(intel_pstate->f_max_perf_pct, &val);
    if (err)
        return err;

    mbp_log(LOG_INFO, "Adjusting intel_pstate: val: %d, step: %d", val, step);

    val += step;
    if (val < 0)
        val = 0;
    if (val > 100)
        val = 100;

    return write_int(intel_pstate->f_max_perf_pct, val);
}

void intel_pstate_exit(t_intel_pstate *intel_pstate)
{
    if (!intel_pstate)
        return;

    (void)write_int(intel_pstate->f_max_perf_pct, intel_pstate->preserved_max_perf_pct);
    (void)write_int(intel_pstate->f_min_perf_pct, intel_pstate->preserved_min_perf_pct);

    fclose(intel_pstate->f_max_perf_pct);
    fclose(intel_pstate->f_min_perf_pct);

    memset(intel_pstate, 0, sizeof(*intel_pstate));
}
