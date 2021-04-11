/**
 *  Copyright (C) (2012-present) Daniel Graziotin <daniel@ineed.coffee>
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

#ifndef _DAEMON_H_
#define _DAEMON_H_

/**
 * Write the PID of the forked daemon to the
 * .pid file defined in char *program_pid
 * Return TRUE on success
 * Return FALSE otherwise
 */
int write_pid(int pid);

/**
 * Read PID of the forked daemon from the
 * .pid file defined in char *program_pid
 * Return the pid on success
 * Return -1 otherwise
 */
int read_pid();

/**
 * Deletes the .pid file defined in
 * char *program_pid
 * Return TRUE on success
 * Return FALSE otherwise
 */
int delete_pid();

/**
 * ...handles signals :-)
 */
void signal_handler(int signal);

/**
 * Daemonizes
 */
void go_daemon(void (*function)());

#endif
