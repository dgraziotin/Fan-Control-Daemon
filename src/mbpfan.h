/**
 *  Copyright (C) 2010  Allan McRae <allan@archlinux.org>
 *  Modifications (2012-present) by Daniel Graziotin <daniel@ineed.coffee>
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

#ifndef _MBPFAN_H_
#define _MBPFAN_H_

#include <stdbool.h>

/** Temperature Thresholds
 *  low_temp - temperature below which fan speed will be at minimum
 *  high_temp - fan will increase speed when higher than this temperature
 *  max_temp - fan will run at full speed above this temperature */
extern int low_temp;
extern int high_temp;
extern int max_temp;

/** Temperature polling interval
 *  Default value was 10 (seconds)
 */
extern int polling_interval;

/** Represents a Temperature sensor
 */
struct s_sensors;
typedef struct s_sensors t_sensors;

struct s_fans;
typedef struct s_fans t_fans;

char *smprintf(const char *fmt, ...) __attribute__((format (printf, 1, 2)));

/**
 * Return true if the kernel is < 3.15.0
 */
bool is_legacy_sensors_path();

/**
 * Tries to use the settings located in
 * /etc/mbpfan.conf
 * If it fails, the default hardcoded settings are used
 */
void retrieve_settings(const char* settings_path, t_fans *fans);

/**
 * Detect the sensors in /sys/devices/platform/coretemp.0/temp
 * and /sys/devices/platform/coretemp.1/temp etc
 * Return a linked list of t_sensors (first temperature detected)
 */
t_sensors *retrieve_sensors();


/**
 * Read a Single Sensor
 */
void read_sensor(t_sensors *sensor);


/**
 * Given a linked list of t_sensors, refresh their detected
 * temperature
 */
t_sensors *refresh_sensors(t_sensors *sensors);

/**
 * Detect the fans in /sys/devices/platform/applesmc.768/
 * Associate each fan to a sensor
 */
t_fans* retrieve_fans();

/**
 * Given a list of sensors with associated fans
 * Set them to manual control
 */
void set_fans_man(t_fans *fans);

/**
 * Given a list of sensors with associated fans
 * Set them to automatic control
 */
void set_fans_auto(t_fans *fans);

/**
 * Given a sensors with associated fans
 * Change their speed
 */
void set_fan_speed(t_fans* fan, int speed);

/**
 * Given a list of fans set their minumum fan speed
 */
void set_fan_minimum_speed(t_fans* fans);
/**
 *  Return maximum CPU temp in degrees
 */
unsigned short get_temp(t_sensors* sensors);

/**
 * Check if user has proper access and that required
 * kernel modules are available
 */
void check_requirements(const char* program_path);

/**
 * Main Program
 */
void mbpfan();

#endif
