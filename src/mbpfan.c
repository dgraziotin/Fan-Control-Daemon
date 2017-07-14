/**
 *  mbpfan.c - automatically control fan for MacBook Pro
 *  Copyright (C) 2010  Allan McRae <allan@archlinux.org>
 *  Modifications by Rafael Vega <rvega@elsoftwarehamuerto.org>
 *  Modifications (2012) by Ismail Khatib <ikhatib@gmail.com>
 *  Modifications (2012-present) by Daniel Graziotin <daniel@ineed.coffee> [CURRENT MAINTAINER]
 *  Modifications (2017-present) by Robert Musial <rmusial@fastmail.com>
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
 *    Assumes any number of processors and fans (max. 10)
 *    It uses only the temperatures from the processors as input.
 *    Requires coretemp and applesmc kernel modules to be loaded.
 *    Requires root use
 *
 *  Tested models: see README.md
 */


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

int min_fan_speed = 2000;
int max_fan_speed = 6200;

/* temperature thresholds
 * low_temp - temperature below which fan speed will be at minimum
 * high_temp - fan will increase speed when higher than this temperature
 * max_temp - fan will run at full speed above this temperature */
int low_temp = 63;   // try ranges 55-63
int high_temp = 66;  // try ranges 58-66
int max_temp = 86;   // do not set it > 90

int polling_interval = 7;

t_sensors* sensors = NULL;
t_fans* fans = NULL;


bool is_legacy_sensors_path()
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

    
    // thanks http://stackoverflow.com/questions/18192998/plain-c-opening-a-directory-with-fopen
    fopen("/sys/devices/platform/coretemp.0/hwmon", "wb");

    if (errno == EISDIR) {
        return 0;
    } else {
        return 1;
    }

    // 
    // str_kernel_version = strtok(NULL, ".");
    // int kernel_version = atoi(str_kernel_version);

    // if(verbose) {
    //     printf("Detected kernel version: %s\n", kernel.release);
    //     printf("Detected kernel minor revision: %s\n", str_kernel_version);

    //     if(daemonize) {
    //         syslog(LOG_INFO, "Kernel version: %s", kernel.release);
    //         syslog(LOG_INFO, "Detected kernel minor revision: %s", str_kernel_version);
    //     }
    // }

    // return (atoi(kernel.release) == 3 && kernel_version < 15);
}


t_sensors *retrieve_sensors()
{

    t_sensors *sensors_head = NULL;
    t_sensors *s = NULL;

    char *path = NULL;
    char *path_begin = NULL;

    if (is_legacy_sensors_path()) {
        if(verbose) {
            printf("Using legacy sensor path for kernel < 3.15.0\n");

            if(daemonize) {
                syslog(LOG_INFO, "Using legacy path for kernel < 3.15.0");
            }
        }

        path_begin = (char *) "/sys/devices/platform/coretemp.0/temp";

    } else {

        if(verbose) {
            printf("Using new sensor path for kernel >= 3.15.0 or some CentOS versions with kernel 3.10.0\n");

            if(daemonize) {
                syslog(LOG_INFO, "Using new sensor path for kernel >= 3.15.0 or some CentOS versions with kernel 3.10.0 ");
            }
        }

        path_begin = (char *) "/sys/devices/platform/coretemp.0/hwmon/hwmon";

        int counter;
        for (counter = 0; counter < 10; counter++) {

            char hwmon_path[strlen(path_begin)+2];

            sprintf(hwmon_path, "%s%d", path_begin, counter);

            // thanks http://stackoverflow.com/questions/18192998/plain-c-opening-a-directory-with-fopen
            fopen(hwmon_path, "wb");

            if (errno == EISDIR) {

                path_begin = (char*) malloc(sizeof( char ) * (strlen(hwmon_path) + strlen("/temp") + 1));
                strcpy(path_begin, hwmon_path);
                strcat(path_begin, "/temp");

                if(verbose) {
                    printf("Found hwmon path at %s\n", path_begin);

                    if(daemonize) {
                        syslog(LOG_INFO, "Found hwmon path at %s\n", path_begin);
                    }

                }

                break;
            }
        }
    }

    const char *path_end = "_input";

    int path_size = strlen(path_begin) + strlen(path_end) + 2;
    char number[2];
    sprintf(number,"%d",0);

    int sensors_found = 0;

    int counter = 0;
    for(counter = 0; counter<10; counter++) {
        path = (char*) malloc(sizeof( char ) * path_size);

        sprintf(number,"%d",counter);
        path[0] = '\0';
        strncat( path, path_begin, strlen(path_begin) );
        strncat( path, number, strlen(number) );
        strncat( path, path_end, strlen(path_begin) );

        FILE *file = fopen(path, "r");

        if(file != NULL) {
            s = (t_sensors *) malloc( sizeof( t_sensors ) );
            s->path = (char *) malloc(sizeof( char ) * path_size);
            strcpy(s->path, path);
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

    int path_min_size = strlen(path_begin) + strlen(path_output_end) + 2;
    int path_man_size = strlen(path_begin) + strlen(path_man_end) + 2;
    char number[2];
    sprintf(number,"%d",0);

    int counter = 0;
    int fans_found = 0;

    for(counter = 0; counter<10; counter++) {

        path_output = (char*) malloc(sizeof( char ) * path_min_size);
        path_output[0] = '\0';
        path_manual = (char*) malloc(sizeof( char ) * path_man_size);
        path_manual[0] = '\0';
        sprintf(number,"%d",counter);

        strncat( path_output, path_begin, strlen(path_begin) );
        strncat( path_output, number, strlen(number) );
        strncat( path_output, path_output_end, strlen(path_begin) );

        strncat( path_manual, path_begin, strlen(path_begin) );
        strncat( path_manual, number, strlen(number) );
        strncat( path_manual, path_man_end, strlen(path_begin) );


        FILE *file = fopen(path_output, "w");

        if(file != NULL) {
            fan = (t_fans *) malloc( sizeof( t_fans ) );
            fan->fan_output_path = (char *) malloc(sizeof( char ) * path_min_size);
            fan->fan_manual_path = (char *) malloc(sizeof( char ) * path_man_size);
            strcpy(fan->fan_output_path, path_output);
            strcpy(fan->fan_manual_path, path_manual);

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

    if (!fans_found > 0){
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
        if(tmp->file != NULL) {
            char buf[16];
            int len = snprintf(buf, sizeof(buf), "%d", speed);
            pwrite(fileno(tmp->file), buf, len, /*offset=*/ 0);
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
