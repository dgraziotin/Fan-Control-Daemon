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

#ifndef  _INTEL_PT_H_
#define _INTEL_PT_H_

#define INTEL_PT_PATH "/sys/devices/system/cpu/intel_pstate/"

struct s_intel_pstate {
    FILE *f_max_perf_pct;
    FILE *f_min_perf_pct;
    int preserved_max_perf_pct;
    int preserved_min_perf_pct;
};

typedef struct s_intel_pstate t_intel_pstate;

extern t_intel_pstate *intel_pstate;

extern int intel_pstate_init(t_intel_pstate *intel_pstate);
extern int intel_pstate_is_available(void);
extern int intel_pstate_adjust(t_intel_pstate *intel_pstate, int step);
extern void intel_pstate_exit(t_intel_pstate *intel_pstate);

#endif /*_INTEL_PT_H_*/
