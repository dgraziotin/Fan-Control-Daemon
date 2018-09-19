/**
 *  mbpfan.c - automatically control fan for MacBook Pro
 *  Copyright (C) 2010  Allan McRae <allan@archlinux.org>
 *  Modifications by Rafael Vega <rvega@elsoftwarehamuerto.org>
 *  Modifications (2012) by Ismail Khatib <ikhatib@gmail.com>
 *  Modifications (2012-present) by Daniel Graziotin <daniel@ineed.coffee> [CURRENT MAINTAINER]
 *  Modifications (2017-present) by Robert Musial <rmusial@fastmail.com>
 *  Modifications (2018-present) by Ati Sharma <ati.sharma@gmail.com>
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
 *
 *  Notes:
 *    Assumes any number of processors, cores and fans (max. 6, 16, 12
 *    as defined in NUM_PROCESSORS, NUM_HWMONS, NUM_TEMP_INPUTS and NUM_FANS)
 *    It uses only the temperatures from the processors as input.
 *    Requires coretemp and applesmc kernel modules to be loaded.
 *    Requires root use
 *
 *  Tested models: see README.md
 */


#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <syslog.h>
#include <stdbool.h>
#include <sys/utsname.h>
#include <sys/errno.h>
#include "mbpfan.h"
#include "global.h"
#include "settings.h"

/* lazy min/max... */
#define min(a,b) ((a) < (b) ? (a) : (b))
#define max(a,b) ((a) > (b) ? (a) : (b))

// TODO: per-fan minimum and maximum?
int min_fan_speed = -1;
int max_fan_speed = -1;

/* temperature thresholds
 * low_temp - temperature below which fan speed will be at minimum
 * high_temp - fan will increase speed when higher than this temperature
 * max_temp - fan will run at full speed above this temperature */
int low_temp = 63;   // try ranges 55-63
int high_temp = 66;  // try ranges 58-66
int max_temp = 86;   // do not set it > 90

// maximum number of processors etc supported
#define NUM_PROCESSORS 6
#define NUM_HWMONS 12
#define NUM_TEMP_INPUTS 16
#define NUM_FANS 10

int polling_interval = 7;

t_sensors* sensors = NULL;
t_fans* fans = NULL;


char *smprintf(const char *fmt, ...)
{
    char *buf;
    int cnt;
    va_list ap;

    // find buffer length
    va_start(ap, fmt);
    cnt = vsnprintf(NULL, 0, fmt, ap);
    va_end(ap);
    if (cnt < 0) {
        return NULL;
    }

    // create and write to buffer
    buf = malloc(cnt + 1);
    va_start(ap, fmt);
    vsnprintf(buf, cnt + 1, fmt, ap);
    va_end(ap);
    return buf;
}

bool is_modern_sensors_path()
{
    struct utsname kernel;
    uname(&kernel);

    char *str_kernel_version;
    str_kernel_version = strtok(kernel.release, ".");

    if (atoi(str_kernel_version) < 3){
        syslog(LOG_INFO, "mbpfan detected a pre-3.x.x linux kernel. Detected version: %s. Exiting.\n", kernel.release);
        printf("mbpfan detected a pre-3.x.x linux kernel. Detected version: %s. Exiting.\n", kernel.release);
        exit(EXIT_FAILURE);
    }

    int counter;

    for (counter = 0; counter < NUM_HWMONS; counter++) {
        int temp;
        for (temp = 1; temp < NUM_TEMP_INPUTS; ++temp) {
            char *path = smprintf("/sys/devices/platform/coretemp.0/hwmon/hwmon%d/temp%d_input", counter, temp);
            int res = access(path, R_OK);
            free(path);
            if (res == 0) {
                return 1;
            }
        }
    }

    return 0;
}


t_sensors *retrieve_sensors()
{

    t_sensors *sensors_head = NULL;
    t_sensors *s = NULL;

    char *path = NULL;
    char *path_begin = NULL;

    const char *path_end = "_input";
    int sensors_found = 0;

    if (!is_modern_sensors_path()) {
        if(verbose) {
            printf("Using legacy sensor path for kernel < 3.15.0\n");

            if(daemonize) {
                syslog(LOG_INFO, "Using legacy path for kernel < 3.15.0");
            }
        }

        path_begin = strdup("/sys/devices/platform/coretemp.0/temp");

    } else {

        if(verbose) {
            printf("Using new sensor path for kernel >= 3.15.0 or some CentOS versions with kernel 3.10.0\n");

            if(daemonize) {
                syslog(LOG_INFO, "Using new sensor path for kernel >= 3.15.0 or some CentOS versions with kernel 3.10.0 ");
            }
        }

	// loop over up to 6 processors
	int processor;
	for (processor = 0; processor < NUM_PROCESSORS; processor++) {

	    path_begin = smprintf("/sys/devices/platform/coretemp.%d/hwmon/hwmon", processor);

	    int counter;
	    for (counter = 0; counter < NUM_HWMONS; counter++) {

		char hwmon_path[strlen(path_begin)+2];

		sprintf(hwmon_path, "%s%d", path_begin, counter);

		int res = access(hwmon_path, R_OK);
		if (res == 0) {

		    free(path_begin);
		    path_begin = smprintf("%s/temp", hwmon_path);

		    if(verbose) {
			printf("Found hwmon path at %s\n", path_begin);

			if(daemonize) {
			    syslog(LOG_INFO, "Found hwmon path at %s\n", path_begin);
			}

		    }

		    break;
		}
	    }

	    int core = 0;
	    for(core = 0; core<NUM_TEMP_INPUTS; core++) {
		path = smprintf("%s%d%s", path_begin, core, path_end);

		FILE *file = fopen(path, "r");

		if(file != NULL) {
		    s = (t_sensors *) malloc( sizeof( t_sensors ) );
		    s->path = strdup(path);
		    fscanf(file, "%d", &s->temperature);

		    if (sensors_head == NULL) {
			sensors_head = s;
			sensors_head->next = NULL;

		    } else {
			t_sensors *tmp = sensors_head;

			while (tmp->next != NULL) {
			    tmp = tmp->next;
			}

			tmp->next = s;
			tmp->next->next = NULL;
		    }

		    s->file = file;
		    sensors_found++;
		}

		free(path);
		path = NULL;
	    }
	}
    }

    if(verbose) {
        printf("Found %d sensors\n", sensors_found);

        if(daemonize) {
            syslog(LOG_INFO, "Found %d sensors", sensors_found);
        }
    }

    if (sensors_found == 0){
        syslog(LOG_CRIT, "mbpfan could not detect any temp sensor. Please contact the developer.\n");
        printf("mbpfan could not detect any temp sensor. Please contact the developer.\n");
        exit(EXIT_FAILURE);
    }

    free(path_begin);
    path_begin = NULL;

    return sensors_head;
}


t_fans *retrieve_fans()
{

    t_fans *fans_head = NULL;
    t_fans *fan = NULL;

    char *path_output = NULL;
    char *path_manual = NULL;

    const char *path_begin = "/sys/devices/platform/applesmc.768/fan";
    const char *path_output_end = "_output";
    const char *path_man_end = "_manual";

    int counter = 0;
    int fans_found = 0;

    for(counter = 0; counter<NUM_FANS; counter++) {

        path_output = smprintf("%s%d%s", path_begin, counter, path_output_end);
        path_manual = smprintf("%s%d%s", path_begin, counter, path_man_end);

        FILE *file = fopen(path_output, "w");

        if(file != NULL) {
            fan = (t_fans *) malloc( sizeof( t_fans ) );
            fan->fan_output_path = strdup(path_output);
            fan->fan_manual_path = strdup(path_manual);
            fan->old_speed = 0;

            if (fans_head == NULL) {
                fans_head = fan;
                fans_head->next = NULL;

            } else {
                t_fans *tmp = fans_head;

                while (tmp->next != NULL) {
                    tmp = tmp->next;
                }

                tmp->next = fan;
                tmp->next->next = NULL;
            }

            fan->file = file;
            fans_found++;
        }

        free(path_output);
        path_output = NULL;
        free(path_manual);
        path_manual = NULL;
    }

    if(verbose) {
        printf("Found %d fans\n", fans_found);

        if(daemonize) {
            syslog(LOG_INFO, "Found %d fans", fans_found);
        }
    }

    if (fans_found == 0){
        syslog(LOG_CRIT, "mbpfan could not detect any fan. Please contact the developer.\n");
        printf("mbpfan could not detect any fan. Please contact the developer.\n");
        exit(EXIT_FAILURE);
    }


    return fans_head;
}


static void set_fans_mode(t_fans *fans, int mode)
{

    t_fans *tmp = fans;
    FILE *file;

    while(tmp != NULL) {
        file = fopen(tmp->fan_manual_path, "rw+");

        if(file != NULL) {
            fprintf(file, "%d", mode);
            fclose(file);
        }

        tmp = tmp->next;
    }
}

void set_fans_man(t_fans *fans)
{

    set_fans_mode(fans, 1);
}

void set_fans_auto(t_fans *fans)
{

    set_fans_mode(fans, 0);
}

t_sensors *refresh_sensors(t_sensors *sensors)
{

    t_sensors *tmp = sensors;

    while(tmp != NULL) {
        if(tmp->file != NULL) {
            char buf[16];
            int len = pread(fileno(tmp->file), buf, sizeof(buf), /*offset=*/ 0);
            buf[len] = '\0';
            sscanf(buf, "%d", &tmp->temperature);
        }

        tmp = tmp->next;
    }

    return sensors;
}


/* Controls the speed of the fan */
void set_fan_speed(t_fans* fans, int speed)
{
    t_fans *tmp = fans;

    while(tmp != NULL) {
        if(tmp->file != NULL && tmp->old_speed != speed) {
            char buf[16];
            int len = snprintf(buf, sizeof(buf), "%d", speed);
            int res = pwrite(fileno(tmp->file), buf, len, /*offset=*/ 0);
            if (res == -1) {
                perror("Could not set fan speed");
            }
            tmp->old_speed = speed;
        }

        tmp = tmp->next;
    }

}


unsigned short get_temp(t_sensors* sensors)
{
    sensors = refresh_sensors(sensors);
    int sum_temp = 0;
    unsigned short temp = 0;

    t_sensors* tmp = sensors;

    int number_sensors = 0;

    while(tmp != NULL) {
        sum_temp += tmp->temperature;
        tmp = tmp->next;
        number_sensors++;
    }

    // just to be safe
    if (number_sensors == 0) {
        number_sensors++;
    }

    temp = (unsigned short)( ceil( (float)( sum_temp ) / (number_sensors * 1000) ) );
    return temp;
}


void retrieve_settings(const char* settings_path)
{
    Settings *settings = NULL;
    int result = 0;
    FILE *f = NULL;

    if (settings_path == NULL) {
        f = fopen("/etc/mbpfan.conf", "r");

    } else {
        f = fopen(settings_path, "r");
    }


    if (f == NULL) {
        /* Could not open configfile */
        if(verbose) {
            printf("Couldn't open configfile, using defaults\n");

            if(daemonize) {
                syslog(LOG_INFO, "Couldn't open configfile, using defaults");
            }
        }

    } else {
        settings = settings_open(f);
        fclose(f);

        if (settings == NULL) {
            /* Could not read configfile */
            if(verbose) {
                printf("Couldn't read configfile\n");

                if(daemonize) {
                    syslog(LOG_WARNING, "Couldn't read configfile");
                }
            }

        } else {
            /* Read configfile values */
            result = settings_get_int(settings, "general", "min_fan_speed");

            if (result != 0) {
                min_fan_speed = result;
            }

            result = settings_get_int(settings, "general", "max_fan_speed");

            if (result != 0) {
                max_fan_speed = result;
            }

            result = settings_get_int(settings, "general", "low_temp");

            if (result != 0) {
                low_temp = result;
            }

            result = settings_get_int(settings, "general", "high_temp");

            if (result != 0) {
                high_temp = result;
            }

            result = settings_get_int(settings, "general", "max_temp");

            if (result != 0) {
                max_temp = result;
            }

            result = settings_get_int(settings, "general", "polling_interval");

            if (result != 0) {
                polling_interval = result;
            }

            /* Destroy the settings object */
            settings_delete(settings);
        }
    }
}


void mbpfan()
{
    int old_temp, new_temp, fan_speed, steps;
    int temp_change;
    int step_up, step_down;

    retrieve_settings(NULL);

    if (min_fan_speed > max_fan_speed) {
        syslog(LOG_INFO, "Invalid fan speeds: %d %d", min_fan_speed, max_fan_speed);
        printf("Invalid fan speeds: %d %d\n", min_fan_speed, max_fan_speed);
        exit(EXIT_FAILURE);
    }
    if (low_temp > high_temp || high_temp > max_temp) {
        syslog(LOG_INFO, "Invalid temperatures: %d %d %d", low_temp, high_temp, max_temp);
        printf("Invalid temperatures: %d %d %d\n", low_temp, high_temp, max_temp);
        exit(EXIT_FAILURE);
    }

    sensors = retrieve_sensors();
    fans = retrieve_fans();
    set_fans_man(fans);

    new_temp = get_temp(sensors);

    fan_speed = min_fan_speed;
    set_fan_speed(fans, fan_speed);

    if(verbose) {
        printf("Sleeping for 2 seconds to get first temp delta\n");

        if(daemonize) {
            syslog(LOG_INFO, "Sleeping for 2 seconds to get first temp delta");
        }
    }
    sleep(2);

    step_up = (float)( max_fan_speed - min_fan_speed ) /
              (float)( ( max_temp - high_temp ) * ( max_temp - high_temp + 1 ) / 2 );

    step_down = (float)( max_fan_speed - min_fan_speed ) /
                (float)( ( max_temp - low_temp ) * ( max_temp - low_temp + 1 ) / 2 );

    while(1) {
        old_temp = new_temp;
        new_temp = get_temp(sensors);

        if(new_temp >= max_temp && fan_speed != max_fan_speed) {
            fan_speed = max_fan_speed;
        }

        if(new_temp <= low_temp && fan_speed != min_fan_speed) {
            fan_speed = min_fan_speed;
        }

        temp_change = new_temp - old_temp;

        if(temp_change > 0 && new_temp > high_temp && new_temp < max_temp) {
            steps = ( new_temp - high_temp ) * ( new_temp - high_temp + 1 ) / 2;
            fan_speed = max( fan_speed, ceil(min_fan_speed + steps * step_up) );
        }

        if(temp_change < 0 && new_temp > low_temp && new_temp < max_temp) {
            steps = ( max_temp - new_temp ) * ( max_temp - new_temp + 1 ) / 2;
            fan_speed = min( fan_speed, floor(max_fan_speed - steps * step_down) );
        }

        if(verbose) {
            printf("Old Temp %d: New Temp: %d, Fan Speed: %d\n", old_temp, new_temp, fan_speed);

            if(daemonize) {
                syslog(LOG_INFO, "Old Temp %d: New Temp: %d, Fan Speed: %d", old_temp, new_temp, fan_speed);
            }
        }

        set_fan_speed(fans, fan_speed);

        if(verbose) {
            printf("Sleeping for %d seconds\n", polling_interval);
            fflush(stdout);

            if(daemonize) {
                syslog(LOG_INFO, "Sleeping for %d seconds", polling_interval);
            }
        }

        // call nanosleep instead of sleep to avoid rt_sigprocmask and
        // rt_sigaction
        struct timespec ts;
        ts.tv_sec = polling_interval;
        ts.tv_nsec = 0;
        nanosleep(&ts, NULL);
    }
}
