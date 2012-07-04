/**
 *  mbpfan.c - automatically control fan for MacBook Pro
 *  Copyright (C) 2010  Allan McRae <allan@archlinux.org>
 *  Modifications by Rafael Vega <rvega@elsoftwarehamuerto.org>
 *  Modifications (2012) by Daniel Graziotin <dgraziotin@task3.cc>
 *  Modifications (2012) by Ismail Khatib <ikhatib@gmail.com>
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
 *  2012-06-09 - v1.2
 *
 *  Notes:
 *    Assumes any number of processors and fans (max. 10)
 *    It uses only the temperatures from the processors as input.
 *    Requires coretemp and applesmc kernel modules to be loaded.
 *    Requires root use
 *
 *  Tested models:
 *    MacBook Pro 8.1 13"  (Intel i7 - Linux 3.2)
 *    MacBook Pro 6,2 15"  (Intel i7 - Linux 3.2)
 *    MacBook Pro 2,2 15"  (Intel Core 2 Duo - Linux 3.4.4)
 */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <syslog.h>
#include "mbpfan.h"
#include "global.h"
#include "settings.h"

/* lazy min/max... */
#define min(a,b) a < b ? a : b
#define max(a,b) a > b ? a : b

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


struct s_sensors
{
  char* path;
  char* fan_output_path;
  char* fan_manual_path;
  unsigned int temperature;
  struct s_sensors *next;
};


t_sensors *retrieve_sensors()
{

  t_sensors *sensors_head = NULL;
  t_sensors *s = NULL;

  char *path = NULL;
  const char *path_begin = "/sys/devices/platform/coretemp.0/temp";
  const char *path_end = "_input";

  int path_size = strlen(path_begin) + strlen(path_end) + 2;
  char number[1];
  sprintf(number,"%d",0);

  int i = 0;
  for(i = 0; i<10; i++)
    {
      path = (char*) malloc(sizeof( char ) * path_size);

      sprintf(number,"%d",i);
      path[0] = '\0';
      strncat( path, path_begin, strlen(path_begin) );
      strncat( path, number, strlen(number) );
      strncat( path, path_end, strlen(path_begin) );

      FILE *file = fopen(path, "r");

      if(file != NULL)
        {
          s = (t_sensors *) malloc( sizeof( t_sensors ) );
          s->path = (char *) malloc(sizeof( char ) * path_size);
          strcpy(s->path, path);
          fscanf(file, "%d", &s->temperature);
          if (sensors_head == NULL)
            {
              sensors_head = s;
              sensors_head->next = NULL;
            }
          else
            {
              t_sensors *tmp = sensors_head;
              while (tmp->next != NULL)
                {
                  tmp = tmp->next;
                }
              tmp->next = s;
              tmp->next->next = NULL;
            }
          fclose(file);
        }
      free(path);
      path = NULL;
    }
  if(sensors_head != NULL)
    find_fans(sensors_head);
  return sensors_head;
}

void find_fans(t_sensors* sensors)
{
  t_sensors *tmp = sensors;

  char *path_output = NULL;
  char *path_manual = NULL;

  const char *path_begin = "/sys/devices/platform/applesmc.768/fan";
  const char *path_output_end = "_output";
  const char *path_man_end = "_manual";

  int path_min_size = strlen(path_begin) + strlen(path_output_end) + 2;
  int path_man_size = strlen(path_begin) + strlen(path_man_end) + 2;
  char number[1];
  sprintf(number,"%d",0);

  int n_sensors = 0;
  int n_fans = 0;

  for(n_sensors = 0; n_sensors<10; n_sensors++)
    {
      path_output = (char*) malloc(sizeof( char ) * path_min_size);
      path_output[0] = '\0';
      path_manual = (char*) malloc(sizeof( char ) * path_man_size);
      path_manual[0] = '\0';
      sprintf(number,"%d",n_sensors);

      strncat( path_output, path_begin, strlen(path_begin) );
      strncat( path_output, number, strlen(number) );
      strncat( path_output, path_output_end, strlen(path_begin) );

      strncat( path_manual, path_begin, strlen(path_begin) );
      strncat( path_manual, number, strlen(number) );
      strncat( path_manual, path_man_end, strlen(path_begin) );


      FILE *file = fopen(path_output, "r");

      if(file != NULL)
        {
          if (tmp->path != NULL)
            {
              tmp->fan_output_path = (char *) malloc(sizeof( char ) * path_min_size);
              tmp->fan_manual_path = (char *) malloc(sizeof( char ) * path_man_size);
            }
          strcpy(tmp->fan_output_path, path_output);
          strcpy(tmp->fan_manual_path, path_manual);
          tmp = tmp->next;
          n_fans++;
          fclose(file);
        }
    }

  if(verbose)
    {
      printf("Found %d: sensors and %d fans\n", n_sensors, n_fans);
      if(daemonize)
        {
          syslog(LOG_INFO, "Found %d: sensors and %d fans", n_sensors, n_fans);
        }
    }

  free(path_output);
  path_output = NULL;
  free(path_manual);
  path_manual = NULL;
}

void set_fans_man(t_sensors *sensors)
{

  t_sensors *tmp = sensors;
  FILE *file;
  while(tmp != NULL)
    {
      file = fopen(tmp->fan_manual_path, "rw+");
      if(file != NULL)
        {
          fprintf(file, "%d", 1);
          fclose(file);
        }
      tmp = tmp->next;
    }
}

t_sensors *refresh_sensors(t_sensors *sensors)
{

  t_sensors *tmp = sensors;

  while(tmp != NULL)
    {
      FILE *file = fopen(tmp->path, "r");

      if(file != NULL)
        {
          fscanf(file, "%d", &tmp->temperature);
          fclose(file);
        }

      tmp = tmp->next;
    }
  return sensors;
}



/* Controls the speed of the fan */
void set_fan_speed(t_sensors* sensors, int speed)
{
  t_sensors *tmp = sensors;
  FILE *file;
  while(tmp != NULL)
    {
      file = fopen(tmp->fan_output_path, "rw+");
      if(file != NULL)
        {
          fprintf(file, "%d", speed);
          fclose(file);
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
  while(tmp != NULL)
    {
      sum_temp += tmp->temperature;
      tmp = tmp->next;
    }
  temp = (unsigned short)( ceil( (float)( sum_temp ) / 2000. ) );
  return temp;
}


void mbpfan()
{
  int old_temp, new_temp, fan_speed, steps;
  int temp_change;
  int step_up, step_down;
  FILE *f = NULL;
  Settings *settings = NULL;
  int result = 0;

  t_sensors* sensors = retrieve_sensors();
  set_fans_man(sensors);
  new_temp = get_temp(sensors);
  fan_speed = 2000;
  set_fan_speed(sensors, fan_speed);

  f = fopen("/etc/mbpfan.conf", "r");
  if (f == NULL)
    {
      /* Could not open configfile */
      if(verbose)
        {
          printf("Couldn't open configfile, using defaults\n");
          if(daemonize)
            {
              syslog(LOG_INFO, "Couldn't open configfile, using defaults");
            }
        }
    }
  else
    {
      settings = settings_open(f);
      fclose(f);
      if (settings == NULL)
        {
          /* Could not read configfile */
          if(verbose)
            {
              printf("Couldn't read configfile\n");
              if(daemonize)
                {
                  syslog(LOG_INFO, "Couldn't read configfile");
                }
            }
        }
      else
        {
          /* Read configfile values */
          result = settings_get_int(settings, "general", "min_fan_speed");
          if (result != 0) min_fan_speed = result;
          result = settings_get_int(settings, "general", "max_fan_speed");
          if (result != 0) max_fan_speed = result;
          result = settings_get_int(settings, "general", "low_temp");
          if (result != 0) low_temp = result;
          result = settings_get_int(settings, "general", "high_temp");
          if (result != 0) high_temp = result;
          result = settings_get_int(settings, "general", "max_temp");
          if (result != 0) max_temp = result;
          result = settings_get_int(settings, "general", "polling_interval");
          if (result != 0) polling_interval = result;

          /* Destroy the settings object */
          settings_delete(settings);
        }
    }

  if(verbose)
    {
      printf("Sleeping for %d seconds\n", polling_interval);
      if(daemonize)
        {
          syslog(LOG_INFO, "Sleeping for %d seconds", polling_interval);
        }
    }
  sleep(polling_interval);

  step_up = (float)( max_fan_speed - min_fan_speed ) /
            (float)( ( max_temp - high_temp ) * ( max_temp - high_temp + 1 ) / 2 );

  step_down = (float)( max_fan_speed - min_fan_speed ) /
              (float)( ( max_temp - low_temp ) * ( max_temp - low_temp + 1 ) / 2 );

  while(1)
    {
      old_temp = new_temp;
      new_temp = get_temp(sensors);

      if(new_temp >= max_temp && fan_speed != max_fan_speed)
        {
          fan_speed = max_fan_speed;
        }

      if(new_temp <= low_temp && fan_speed != min_fan_speed)
        {
          fan_speed = min_fan_speed;
        }

      temp_change = new_temp - old_temp;

      if(temp_change > 0 && new_temp > high_temp && new_temp < max_temp)
        {
          steps = ( new_temp - high_temp ) * ( new_temp - high_temp + 1 ) / 2;
          fan_speed = max( fan_speed, ceil(min_fan_speed + steps * step_up) );
        }

      if(temp_change < 0 && new_temp > low_temp && new_temp < max_temp)
        {
          steps = ( max_temp - new_temp ) * ( max_temp - new_temp + 1 ) / 2;
          fan_speed = min( fan_speed, floor(max_fan_speed - steps * step_down) );
        }

      if(verbose)
        {
          printf("Old Temp %d: New Temp: %d, Fan Speed: %d\n", old_temp, new_temp, fan_speed);
          if(daemonize)
            {
              syslog(LOG_INFO, "Old Temp %d: New Temp: %d, Fan Speed: %d", old_temp, new_temp, fan_speed);
            }
        }

      set_fan_speed(sensors, fan_speed);

      if(verbose)
        {
          printf("Sleeping for %d seconds\n", polling_interval);
          if(daemonize)
            {
              syslog(LOG_INFO, "Sleeping for %d seconds", polling_interval);
            }
        }
      sleep(polling_interval);
    }
}
