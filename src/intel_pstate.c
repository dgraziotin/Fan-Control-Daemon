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

int intel_pstate_init(t_intel_pstate *intel_pstate)
{
    const char *path_max_perf_pct = INTEL_PT_PATH "/max_perf_pct";
    const char *path_min_perf_pct = INTEL_PT_PATH "/min_perf_pct";
    FILE *fp;
    int ret;

    fp = fopen(path_max_perf_pct, "w+");
    if (!fp)
        return -1;
    /* Save the initial value of max_perf_pct, to restore it on exit */
    ret = fscanf(fp, "%d", &intel_pstate->preserved_max_perf_pct);
    if (ret != 1)
        return -1;
    /* Start with the maximum performance percentage at 100% */
    ret = fprintf(fp, "100");
    if (ret != 3) /* "100" -> 3 characters */
        return -1;
    intel_pstate->f_max_perf_pct = fp;

    fp = fopen(path_min_perf_pct, "w+");
    if (!fp)
        return -1;
    /* Save the initial value of min_perf_pct, to restore it on exit */
    ret = fscanf(fp, "%d", &intel_pstate->preserved_min_perf_pct);
    if (ret != 1)
        return -1;
    /* Set the minimum performance percentage to 0 */
    ret = fprintf(fp, "0");
    if (ret != 1) /* "0" -> 1 character */
        return -1;
    intel_pstate->f_min_perf_pct = fp;

    return 0;
}

int intel_pstate_is_available(void)
{
    DIR* dir = opendir(INTEL_PT_PATH);

    if ((!dir) && ENOENT == errno)
        return 0;

    if (dir)
      closedir(dir);
    return 1;
}

int intel_pstate_adjust(t_intel_pstate *intel_pstate, int step)
{
    int cur_val, new_val;
    int ret;

    if (!intel_pstate)
        return 1;

    rewind(intel_pstate->f_max_perf_pct);
    ret = fscanf(intel_pstate->f_max_perf_pct, "%d", &cur_val);
    if (ret != 1)
        return -1;

    new_val = cur_val + step;
    if (new_val < 0)
        new_val = 0;
    if (new_val > 100)
        new_val = 100;

    if (new_val == cur_val)
	return 0;

    mbp_log(LOG_INFO, "Adjusting intel_pstate: cur_val: %d, step: %d", cur_val, step);

    ret = fprintf(intel_pstate->f_max_perf_pct, "%d", new_val);
    return (ret > 0) ? 0 : -1;
}

void intel_pstate_exit(t_intel_pstate *intel_pstate)
{
    if (!intel_pstate)
        return;

    (void)fprintf(intel_pstate->f_max_perf_pct, "%d", intel_pstate->preserved_max_perf_pct);
    (void)fprintf(intel_pstate->f_min_perf_pct, "%d", intel_pstate->preserved_min_perf_pct);

    fclose(intel_pstate->f_max_perf_pct);
    fclose(intel_pstate->f_min_perf_pct);

    memset(intel_pstate, 0, sizeof(*intel_pstate));
}
